#include "libhttp.h"

/*
    tailFilter.c -- Filter for the start/end of request pipeline.

    The TailFilter is the last stage in a request pipline. After this is the Http*Filter and NetConnector.
    This filter multiplexes onto the net->outputq which is the http1Filter or http2Filter.

 */

/********************************* Includes ***********************************/



/********************************** Forwards **********************************/

static bool willQueueAcceptPacket(HttpQueue *q, HttpPacket *packet);
static HttpPacket *createAltBodyPacket(HttpQueue *q);
static void incomingTail(HttpQueue *q, HttpPacket *packet);
static void outgoingTail(HttpQueue *q, HttpPacket *packet);
static void incomingTailService(HttpQueue *q);
static void outgoingTailService(HttpQueue *q);

/*********************************** Code *************************************/

PUBLIC int httpOpenTailFilter()
{
    HttpStage     *filter;

    if ((filter = httpCreateFilter("tailFilter", NULL)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    HTTP->tailFilter = filter;
    filter->incoming = incomingTail;
    filter->outgoing = outgoingTail;
    filter->incomingService = incomingTailService;
    filter->outgoingService = outgoingTailService;
    return 0;
}


static void incomingTail(HttpQueue *q, HttpPacket *packet)
{
    HttpStream  *stream;
    HttpRx      *rx;
    ssize       count;

    stream = q->stream;
    rx = stream->rx;

    count = stream->readq->count + httpGetPacketLength(packet);
    stream->lastActivity = stream->http->now;

    if (rx->upload && count >= stream->limits->uploadSize && stream->limits->uploadSize != HTTP_UNLIMITED) {
        httpLimitError(stream, HTTP_CLOSE | HTTP_CODE_REQUEST_TOO_LARGE,
            "Request upload of %d bytes is too big. Limit %lld", (int) count, stream->limits->uploadSize);

    } else if (rx->form && count >= stream->limits->rxFormSize && stream->limits->rxFormSize != HTTP_UNLIMITED) {
        httpLimitError(stream, HTTP_CLOSE | HTTP_CODE_REQUEST_TOO_LARGE,
            "Request form of %d bytes is too big. Limit %lld", (int) count, stream->limits->rxFormSize);

    } else if (count >= stream->limits->rxBodySize && stream->limits->rxBodySize != HTTP_UNLIMITED) {
        httpLimitError(stream, HTTP_CLOSE | HTTP_CODE_REQUEST_TOO_LARGE,
            "Request body of %d bytes is too big. Limit %lld", (int) count, stream->limits->rxFormSize);

    } else {
        httpDefaultIncoming(q, packet);
        if (q->net->eof) {
            httpAddInputEndPacket(stream, q);
        }
    }
}


static void incomingTailService(HttpQueue *q)
{
    HttpStream  *stream;
    HttpPacket  *packet;

    if (httpIsNextQueueSuspended(q)) {
        //  To prevent ping-pong suspensions, don't call httpGetPacket which resumes upstream queues if not required.
        httpSuspendQueue(q);
        return;
    }
    stream = q->stream;
    stream->lastActivity = stream->net->lastActivity = stream->http->now;

    for (packet = httpGetPacket(q); packet; packet = httpGetPacket(q)) {
        if (!httpWillNextQueueAcceptPacket(q, packet)) {
            httpPutBackPacket(q, packet);
            return;
        }
        httpPutPacketToNext(q, packet);
    }
    /*
        Resume the Http* filter if suspended and we are not suspended.
        Special case flow control because tail service is a one-to-many and auto flow control does not handle this
    */
    if (httpIsQueueSuspended(q->net->inputq)) {
        httpResumeQueue(q->net->inputq, 1);
    }
}


static void outgoingTail(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;
    HttpStream  *stream;
    HttpTx      *tx;
    HttpPacket  *headers, *tail;

    stream = q->stream;
    tx = stream->tx;
    net = q->net;

    if (httpIsServer(stream) && stream->state < HTTP_STATE_PARSED) {
        assert(stream->state >= HTTP_STATE_PARSED);
        return;
    }
    stream->lastActivity = stream->http->now;

    if (!(tx->flags & HTTP_TX_HEADERS_CREATED)) {
        headers = httpCreateHeaders(q, NULL);
        while (httpGetPacketLength(headers) > net->outputq->packetSize) {
            tail = httpSplitPacket(headers, net->outputq->packetSize);
            httpPutForService(q, headers, 1);
            headers = tail;
        }
        httpPutForService(q, headers, 1);
        if (tx->altBody) {
            httpPutForService(q, createAltBodyPacket(q), 1);
        }
    }
    if (packet->flags & HTTP_PACKET_DATA) {
        tx->bytesWritten += httpGetPacketLength(packet);
        if (tx->bytesWritten > stream->limits->txBodySize) {
            httpLimitError(stream, HTTP_CODE_REQUEST_TOO_LARGE | ((tx->bytesWritten) ? HTTP_ABORT : 0),
                "Http transmission aborted. Exceeded transmission max body of %lld bytes", stream->limits->txBodySize);
        }
    }
    httpPutForService(q, packet, HTTP_SCHEDULE_QUEUE);
}


static void outgoingTailService(HttpQueue *q)
{
    HttpPacket  *packet;
    HttpStream  *stream;

    stream = q->stream;
    stream->lastActivity = stream->net->lastActivity = stream->http->now;

    for (packet = httpGetPacket(q); packet; packet = httpGetPacket(q)) {
        if (!willQueueAcceptPacket(q, packet)) {
            httpPutBackPacket(q, packet);
            return;
        }
        //  Onto the HttpFilter
        httpPutPacket(q->net->outputq, packet);
    }
}


/*
    Similar to httpWillQueueAcceptPacket, but also handles HTTP/2 flow control.
    This MUST be done here because this is the last per-stream control point. The Http2Filter has all streams
    multiplexed together.
 */
static bool willQueueAcceptPacket(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;
    HttpStream  *stream;
    HttpQueue   *nextQ;
    ssize       room, size;

    net = q->net;
    stream = q->stream;

    /*
        Get the maximum the output stream can absorb that is less than the downstream
        queue packet size and the per-stream window size if HTTP/2.
     */
    nextQ = stream->net->outputq;
    room = nextQ->max - nextQ->count;
    if (packet->flags & HTTP_PACKET_DATA && net->protocol >= 2) {
        room = min(stream->outputq->window, room);
    }
    size = httpGetPacketLength(packet);
    if (size <= room) {
        //  Packet fits
        return 1;
    }
    if (net->protocol < 2 && (size - room) < HTTP_QUEUE_ALLOW) {
        //  Packet almost fits. Allow for HTTP/1. HTTP/2 we must strictly observe the flow control window.
        return 1;
    }
    if (room > 0) {
        /*
            Resize the packet to fit downstream. This will putback the tail if required.
         */
        httpResizePacket(q, packet, room);
        size = httpGetPacketLength(packet);
        if (size > 0) {
            return 1;
        }
    }
    /*
        The downstream queue cannot accept this packet, so suspend this queue and schedule the next if required.
     */
    httpSuspendQueue(q);
    if (!(nextQ->flags & HTTP_QUEUE_SUSPENDED)) {
        httpScheduleQueue(nextQ);
    }
    return 0;
}


/*
    Create an alternate response body for error responses.
 */
static HttpPacket *createAltBodyPacket(HttpQueue *q)
{
    HttpTx      *tx;
    HttpPacket  *packet;

    tx = q->stream->tx;
    packet = httpCreateDataPacket(slen(tx->altBody));
    mprPutStringToBuf(packet->content, tx->altBody);
    return packet;
}





