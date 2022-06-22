#include "libhttp.h"

/*
    chunkFilter.c - Transfer chunk encoding filter.

 */

/********************************* Includes ***********************************/



/********************************** Forwards **********************************/

static void incomingChunk(HttpQueue *q, HttpPacket *packet);
static bool needChunking(HttpQueue *q);
static void outgoingChunkService(HttpQueue *q);
static void setChunkPrefix(HttpQueue *q, HttpPacket *packet);

/*********************************** Code *************************************/
/*
   Loadable module initialization
 */
PUBLIC int httpOpenChunkFilter()
{
    HttpStage     *filter;

    if ((filter = httpCreateFilter("chunkFilter", NULL)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    HTTP->chunkFilter = filter;
    filter->flags |= HTTP_STAGE_INTERNAL;
    filter->incoming = incomingChunk;
    filter->outgoingService = outgoingChunkService;
    return 0;
}


PUBLIC void httpInitChunking(HttpStream *stream)
{
    HttpRx      *rx;

    rx = stream->rx;

    /*
        remainingContent will be revised by the chunk filter as chunks are processed and will
        be set to zero when the tail chunk has been received.
     */
    rx->flags |= HTTP_CHUNKED;
    rx->chunkState = HTTP_CHUNK_START;
    rx->remainingContent = HTTP_UNLIMITED;
    rx->needInputPipeline = 1;
}


/*
    Filter chunk headers and leave behind pure data. This is called for chunked and unchunked data.
    Unchunked data is simply passed upstream. Chunked data format is:
        Chunk spec <CRLF>
        Data <CRLF>
        Chunk spec (size == 0) <CRLF>
        <CRLF>
    Chunk spec is: "HEX_COUNT; chunk length DECIMAL_COUNT\r\n". The "; chunk length DECIMAL_COUNT is optional.
    As an optimization, use "\r\nSIZE ...\r\n" as the delimiter so that the CRLF after data does not special consideration.
    Achive this by parseHeaders reversing the input start by 2.

    Return number of bytes available to read.
    NOTE: may set rx->eof and return 0 bytes on EOF.
 */
static void incomingChunk(HttpQueue *q, HttpPacket *packet)
{
    HttpStream  *stream;
    HttpPacket  *tail;
    HttpRx      *rx;
    MprBuf      *buf;
    ssize       chunkSize, len, nbytes;
    char        *start, *cp;
    int         bad;

    stream = q->stream;
    rx = stream->rx;
    len = httpGetPacketLength(packet);

    if (rx->chunkState == HTTP_CHUNK_UNCHUNKED) {
        if (rx->remainingContent > 0) {
            nbytes = min(rx->remainingContent, httpGetPacketLength(packet));
            rx->bytesRead += nbytes;
            rx->remainingContent -= nbytes;
        }
        httpPutPacketToNext(q, packet);
        if (rx->remainingContent <= 0 && !(packet->flags & HTTP_PACKET_END)) {
            httpAddInputEndPacket(stream, q->nextQ);
        }
        return;
    }

    //  Aggregate all packets
    httpPutForService(q, packet, HTTP_DELAY_SERVICE);
    httpJoinPackets(q, -1);

    for (packet = httpGetPacket(q); packet && !rx->eof; packet = httpGetPacket(q)) {
        while (packet && !stream->error && !rx->eof) {
            switch (rx->chunkState) {
            case HTTP_CHUNK_UNCHUNKED:
                httpError(stream, HTTP_CLOSE | HTTP_CODE_BAD_REQUEST, "Bad chunk state");
                return;

            case HTTP_CHUNK_DATA:
                len = httpGetPacketLength(packet);
                nbytes = min(rx->remainingContent, len);
                rx->remainingContent -= nbytes;
                rx->bytesRead += nbytes;
                if (nbytes < len) {
                    tail = httpSplitPacket(packet, nbytes);
                    httpPutPacketToNext(q, packet);
                    packet = tail;

                } else if (len > 0) {
                    //  Pure data
                    httpPutPacketToNext(q, packet);
                    packet = 0;
                }
                if (rx->remainingContent <= 0) {
                    // End of chunk - prep for the next chunk
                    rx->remainingContent = ME_BUFSIZE;
                    rx->chunkState = HTTP_CHUNK_START;
                }
                if (!packet) {
                    break;
                }
                /* Fall through */

            case HTTP_CHUNK_START:
                /*
                    Validate:  "\r\nSIZE.*\r\n"
                 */
                buf = packet->content;
                len = mprGetBufLength(buf);
                if (len == 0) {
                    return;
                } else if (len < 5) {
                    httpPutBackPacket(q, packet);
                    return;
                }
                start = mprGetBufStart(buf);
                bad = (start[0] != '\r' || start[1] != '\n');

                //  Find trailing '\n'
                for (cp = &start[2]; cp < buf->end && *cp != '\n'; cp++) {}

                //  If not found, put back packet to wait for more data.
                if (cp >= buf->end) {
                    httpPutBackPacket(q, packet);
                    return;
                }
                bad += (cp[-1] != '\r' || cp[0] != '\n');
                if (bad) {
                    httpError(stream, HTTP_CLOSE | HTTP_CODE_BAD_REQUEST, "Bad chunk specification");
                    return;
                }
                chunkSize = (int) stoiradix(&start[2], 16, NULL);
                if (!isxdigit((uchar) start[2]) || chunkSize < 0) {
                    httpError(stream, HTTP_CLOSE | HTTP_CODE_BAD_REQUEST, "Bad chunk specification");
                    return;
                }
                if (chunkSize == 0) {
                    /*
                        Last chunk. Consume the final "\r\n".
                     */
                    if ((cp + 2) >= buf->end) {
                        return;
                    }
                    cp += 2;
                    bad += (cp[-1] != '\r' || cp[0] != '\n');
                    if (bad) {
                        httpError(stream, HTTP_CLOSE | HTTP_CODE_BAD_REQUEST, "Bad final chunk specification");
                        return;
                    }
                }
                mprAdjustBufStart(buf, (cp - start + 1));

                // Remaining content is set to the next chunk size
                rx->remainingContent = chunkSize;

                if (chunkSize == 0) {
                    rx->chunkState = HTTP_CHUNK_EOF;
                    httpAddInputEndPacket(stream, q->nextQ);
                } else {
                    assert(!rx->eof);
                    rx->chunkState = HTTP_CHUNK_DATA;
                }
                break;

            case HTTP_CHUNK_EOF:
                break;

            default:
                httpError(stream, HTTP_CLOSE | HTTP_CODE_BAD_REQUEST, "Bad chunk state %d", rx->chunkState);
                return;
            }
        }
#if HTTP_PIPELINING
        /* HTTP/1.1 pipelining is not implemented reliably by modern browsers (WARNING: dont use) */
        if (packet && httpGetPacketLength(packet)) {
            httpPutPacket(stream->inputq, tail);
        }
#endif
    }
    if (packet) {
        // Transfer END packet
        httpPutPacketToNext(q, packet);
    }
}


static void outgoingChunkService(HttpQueue *q)
{
    HttpStream  *stream;
    HttpPacket  *packet, *finalChunk;
    HttpTx      *tx;

    stream = q->stream;
    tx = stream->tx;

    /*
        Try to determine a content length. If all the data is buffered, we know the length and can do an optimized
        transfer without encoding. We do this here rather than in a put() routine to allow data to buffer.
     */
    if (!(q->flags & HTTP_QUEUE_SERVICED)) {
        tx->needChunking = needChunking(q);
    }
    if (!tx->needChunking) {
        /*
            Join packets if possible to get as few writes as possible. httpWriteBlock can create many small packets.
         */
        if (!stream->upgraded) {
            httpJoinPackets(q, -1);
        }
        //  Must transfer all packets before removing the queue from the pipeline.
        httpTransferPackets(q, q->nextQ);
        httpRemoveQueue(q);
        httpLog(stream->trace, "chunkFilter", "detail", "msg:remove outgoing chunk filter");
        httpTraceQueues(stream);
        return;
    }
    for (packet = httpGetPacket(q); packet && !q->net->error; packet = httpGetPacket(q)) {
        if (packet->flags & HTTP_PACKET_DATA) {
            httpPutBackPacket(q, packet);
            httpJoinPackets(q, tx->chunkSize);
            packet = httpGetPacket(q);

            //  FUTURE - may not be required
            if (httpGetPacketLength(packet) > tx->chunkSize) {
                httpResizePacket(q, packet, tx->chunkSize);
            }
        }
        if (!httpWillNextQueueAcceptPacket(q, packet)) {
            httpPutBackPacket(q, packet);
            return;
        }
        if (packet->flags & HTTP_PACKET_DATA) {
            setChunkPrefix(q, packet);

        } else if (packet->flags & HTTP_PACKET_END) {
            // Insert a packet for the final chunk
            finalChunk = httpCreateDataPacket(0);
            setChunkPrefix(q, finalChunk);
            httpPutPacketToNext(q, finalChunk);
        }
        httpPutPacketToNext(q, packet);
    }
}


static bool needChunking(HttpQueue *q)
{
    HttpStream  *stream;
    HttpTx      *tx;
    cchar       *value;

    stream = q->stream;
    tx = stream->tx;

    if (stream->net->protocol >= 2 || stream->upgraded) {
        return 0;
    }
    /*
        If we don't know the content length yet (tx->length < 0) and if the tail packet is the end packet. Then
        we have all the data. Thus we can determine the actual content length and can bypass the chunk handler.
     */
    if (tx->length < 0 && (value = mprLookupKey(tx->headers, "Content-Length")) != 0) {
        tx->length = stoi(value);
    }
    if (tx->length < 0 && tx->chunkSize < 0) {
        if (q->last && q->last->flags & HTTP_PACKET_END) {
            if (q->count > 0) {
                tx->length = q->count;
            }
        } else {
            tx->chunkSize = min(stream->limits->chunkSize, q->max);
        }
    }
    if (tx->flags & HTTP_TX_USE_OWN_HEADERS || stream->net->protocol != 1) {
        tx->chunkSize = -1;
    }
    return tx->chunkSize > 0;
}


static void setChunkPrefix(HttpQueue *q, HttpPacket *packet)
{
    if (packet->prefix) {
        return;
    }
    packet->prefix = mprCreateBuf(32, 32);
    /*
        NOTE: prefixes don't count in the queue length. No need to adjust q->count
     */
    if (httpGetPacketLength(packet)) {
        mprPutToBuf(packet->prefix, "\r\n%zx\r\n", httpGetPacketLength(packet));
    } else {
        mprPutStringToBuf(packet->prefix, "\r\n0\r\n\r\n");
    }
}



