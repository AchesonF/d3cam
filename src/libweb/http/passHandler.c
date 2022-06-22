#include "libhttp.h"

/*
    passHandler.c -- Pass through handler

    This handler simply relays all content to a network connector. It is used for the ErrorHandler and
    when there is no handler defined. It is configured as the "passHandler" and "errorHandler".
    It also handles OPTIONS and TRACE methods for all.

 */

/********************************* Includes ***********************************/



/********************************** Forwards **********************************/

static void handleTraceMethod(HttpStream *stream);
static void outgoingPassService(HttpQueue *q);
static void readyPass(HttpQueue *q);
static void startPass(HttpQueue *q);

/*********************************** Code *************************************/

PUBLIC int httpOpenPassHandler()
{
    HttpStage     *stage;

    if ((stage = httpCreateHandler("passHandler", NULL)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    HTTP->passHandler = stage;
    stage->start = startPass;
    stage->ready = readyPass;
    stage->outgoingService = outgoingPassService;
    return 0;
}


static void startPass(HttpQueue *q)
{
    HttpStream  *stream;

    stream = q->stream;

    if (stream->rx->flags & HTTP_TRACE && !stream->error) {
        handleTraceMethod(stream);
    }
}


static void readyPass(HttpQueue *q)
{
    httpFinalize(q->stream);
    httpScheduleQueue(q);
}


static void outgoingPassService(HttpQueue *q)
{
    HttpPacket  *packet;

    for (packet = httpGetPacket(q); packet; packet = httpGetPacket(q)) {
        if (packet->flags & HTTP_PACKET_END) {
            httpPutPacketToNext(q, packet);
        }
    }
}


PUBLIC void httpHandleOptions(HttpStream *stream)
{
    httpSetHeaderString(stream, "Allow", httpGetRouteMethods(stream->rx->route));
    httpFinalize(stream);
}


static void handleTraceMethod(HttpStream *stream)
{
    HttpTx      *tx;
    HttpQueue   *q;
    HttpPacket  *traceData;

    tx = stream->tx;
    q = stream->writeq;


    /*
        Create a dummy set of headers to use as the response body. Then reset so the connector will create
        the headers in the normal fashion. Need to be careful not to have a content length in the headers in the body.
     */
    tx->flags |= HTTP_TX_NO_LENGTH;
    httpDiscardData(stream, HTTP_QUEUE_TX);
    traceData = httpCreateDataPacket(q->packetSize);
    httpCreateHeaders1(q, traceData);
    tx->flags &= ~(HTTP_TX_NO_LENGTH | HTTP_TX_HEADERS_CREATED | HTTP_TX_HEADERS_PREPARED);
    httpRemoveHeader(stream, "Content-Type");
    httpSetContentType(stream, "message/http");
    httpPutPacketToNext(q, traceData);
    httpFinalize(stream);
}





