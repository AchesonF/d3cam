#include "libhttp.h"

/*
    http2Filter.c - HTTP/2 protocol handling.

    HTTP/2 protocol filter for HTTP/2 frame processing.

    For historical reasons, the HttpStream object is used to implement HTTP2 streams and HttpNet is
    used to implement HTTP2 network connections.

 */

/********************************* Includes ***********************************/



#if CONFIG_HTTP_HTTP2
/********************************** Locals ************************************/

#define httpGetPrefixMask(bits) ((1 << (bits)) - 1)
#define httpSetPrefix(bits)     (1 << (bits))

typedef void (*FrameHandler)(HttpQueue *q, HttpPacket *packet);

/************************************ Forwards ********************************/

static bool addHeaderToSet(HttpStream *stream, cchar *key, cchar *value);
static int setState(HttpStream *stream, int event);
static void closeNetworkWhenDone(HttpQueue *q);
static HttpFrame *createFrame(HttpQueue *q, HttpPacket *packet, int type, int flags, int streamID);
static int decodeInt(HttpPacket *packet, uint prefix);
static HttpPacket *defineFrame(HttpQueue *q, HttpPacket *packet, int type, uchar flags, int stream);
static void definePseudoHeaders(HttpStream *stream, HttpPacket *packet);
static void encodeHeader(HttpStream *stream, HttpPacket *packet, cchar *key, cchar *value);
static void encodeInt(HttpPacket *packet, uint prefix, uint bits, uint value);
static void encodeString(HttpPacket *packet, cchar *src, uint lower);
static HttpStream *findStream(HttpNet *net, int stream);
static int getFrameFlags(HttpQueue *q, HttpPacket *packet);
static HttpStream *getStream(HttpQueue *q, HttpPacket *packet);
static void incomingHttp2(HttpQueue *q, HttpPacket *packet);
static void invalidState(HttpNet *net, HttpStream *stream, int event);
static void outgoingHttp2(HttpQueue *q, HttpPacket *packet);
static void outgoingHttp2Service(HttpQueue *q);
static void manageFrame(HttpFrame *frame, int flags);
static void parseDataFrame(HttpQueue *q, HttpPacket *packet);
static HttpFrame *parseFrame(HttpQueue *q, HttpPacket *packet, int *done);
static void parseGoAwayFrame(HttpQueue *q, HttpPacket *packet);
static void parseHeaderFrame(HttpQueue *q, HttpPacket *packet);
static cchar *parseHeaderField(HttpQueue *q, HttpStream *stream, HttpPacket *packet);
static bool parseHeader(HttpQueue *q, HttpStream *stream, HttpPacket *packet);
static void parseHeaders2(HttpQueue *q, HttpStream *stream);
static void parsePriorityFrame(HttpQueue *q, HttpPacket *packet);
static void parsePushFrame(HttpQueue *q, HttpPacket *packet);
static void parsePingFrame(HttpQueue *q, HttpPacket *packet);
static void parseResetFrame(HttpQueue *q, HttpPacket *packet);
static void parseSettingsFrame(HttpQueue *q, HttpPacket *packet);
static void parseWindowFrame(HttpQueue *q, HttpPacket *packet);
static void pickStreamNumber(HttpStream *stream);
static void processDataFrame(HttpQueue *q, HttpPacket *packet);
static void resetStream(HttpStream *stream, cchar *msg, int error);
static ssize resizePacket(HttpQueue *q, ssize max, HttpPacket *packet);
static void restartSuspendedStreams(HttpNet *net);
static void sendFrame(HttpQueue *q, HttpPacket *packet);
static void sendGoAway(HttpQueue *q, int status, cchar *fmt, ...);
static void sendPreface(HttpQueue *q);
static void sendReset(HttpQueue *q, HttpStream *stream, int status, cchar *fmt, ...);
static void sendSettings(HttpQueue *q);
static void sendSettingsFrame(HttpQueue *q);
static void sendWindowFrame(HttpQueue *q, int streamID, ssize size);
static void setLastPacket(HttpQueue *q, HttpPacket *packet);
static bool validateHeader(cchar *key, cchar *value);

/*
    Frame callback handlers (order matters)
 */
static FrameHandler frameHandlers[] = {
    parseDataFrame,
    parseHeaderFrame,
    parsePriorityFrame,
    parseResetFrame,
    parseSettingsFrame,
    parsePushFrame,
    parsePingFrame,
    parseGoAwayFrame,
    parseWindowFrame,
    /* ContinuationFrame */ parseHeaderFrame,
};

/*
    Just for debug
 */
static char *packetTypes[] = {
    "DATA",
    "HEADERS",
    "PRIORITY",
    "RESET",
    "SETTINGS",
    "PUSH",
    "PING",
    "GOAWAY",
    "WINDOW",
    "CONTINUE",
};

/*
    Target states
 */
 #define H2_ERR      -1          /* Invalid state transition */
 #define H2_SAME     -2          /* Don't change state */
 #define H2_IDLE     0           /* Initial state */
 #define H2_RLOCAL   1           /* Reserved local */
 #define H2_RREMOTE  2           /* Reserved remote */
 #define H2_OPEN     3
 #define H2_CLOCAL   4           /* Half closed local */
 #define H2_CREMOTE  5           /* Half closed remote */
 #define H2_CLOSED   6
 #define H2_GONE     7           /* Connection gone */
 #define H2_MAX      8


static const char* States[] = {
    "Idle",
    "Reserved Local",
    "Reserved Remote",
    "Open",
    "Closed Local",
    "Closed Remote",
    "Closed",
    "Gone"
};

/*
    State events
 */
#define E2_DATA     HTTP2_DATA_FRAME
#define E2_HDR      HTTP2_HEADERS_FRAME
#define E2_PRI      HTTP2_PRIORITY_FRAME
#define E2_RESET    HTTP2_RESET_FRAME
#define E2_SETTINGS HTTP2_SETTINGS_FRAME
#define E2_PUSH     HTTP2_PUSH_FRAME
#define E2_PING     HTTP2_PING_FRAME
#define E2_GOAWAY   HTTP2_GOAWAY_FRAME
#define E2_WINDOW   HTTP2_WINDOW_FRAME
#define E2_CONT     HTTP2_CONT_FRAME
#define E2_END      HTTP2_MAX_FRAME + 0         /* Receive end stream */
#define E2_TX_END   HTTP2_MAX_FRAME +1          /* Transmit end stream */
#define E2_TX_PUSH  HTTP2_MAX_FRAME + 2         /* Transmit push promise */
#define E2_TX_RESET HTTP2_MAX_FRAME + 3         /* Transmit push promise */
#define E2_TX_HDR   HTTP2_MAX_FRAME + 4         /* Transmit header */
#define E2_TX_DATA  HTTP2_MAX_FRAME + 5         /* Transmit header */
#define E2_MAX      HTTP2_MAX_FRAME + 6

static const char *Events[] = {
    "Receive Data Frame",
    "Receive Header Frame",
    "Receive Priority Frame",
    "Receive Reset Frame",
    "Receive Settings Frame",
    "Receive Push Frame",
    "Receive Ping Frame",
    "Receive GoAway Frame",
    "Receive Window Frame",
    "Receive Continue Frame",
    "Receive End Stream Flag",
    "Send End Stream Flag",
    "Send Push Frame",
    "Send Reset Frame",
    "Send Header Frame",
    "Send Data Frame",
};

static int StateMatrix[E2_MAX][H2_MAX] = {
/* States         idle        rsv-local   rsv-remote  open         clocal      cremote     closed      gone */

/* Receive */
/* data */      { H2_ERR,     H2_ERR,     H2_ERR,     H2_SAME,     H2_SAME,    H2_ERR,     H2_ERR,     H2_ERR  },
/* header */    { H2_OPEN,    H2_OPEN,    H2_ERR,     H2_SAME,     H2_SAME,    H2_ERR,     H2_ERR,     H2_ERR  },
/* pri */       { H2_SAME,    H2_SAME,    H2_SAME,    H2_SAME,     H2_SAME,    H2_SAME,    H2_SAME,    H2_SAME },
/* reset */     { H2_CLOSED,  H2_CLOSED,  H2_CLOSED,  H2_CLOSED,   H2_CLOSED,  H2_CLOSED,  H2_SAME,    H2_ERR  },
/* settings */  { H2_SAME,    H2_SAME,    H2_SAME,    H2_SAME,     H2_SAME,    H2_SAME,    H2_SAME,    H2_ERR  },
/* rx push */   { H2_RREMOTE, H2_ERR,     H2_ERR,     H2_ERR,      H2_ERR,     H2_ERR,     H2_ERR,     H2_ERR  },
/* ping */      { H2_SAME,    H2_SAME,    H2_SAME,    H2_SAME,     H2_SAME,    H2_SAME,    H2_SAME,    H2_SAME },
/* goaway */    { H2_GONE,    H2_GONE,    H2_GONE,    H2_GONE,     H2_GONE,    H2_GONE,    H2_GONE,    H2_GONE },
/* window */    { H2_SAME,    H2_SAME,    H2_SAME,    H2_SAME,     H2_SAME,    H2_SAME,    H2_SAME,    H2_ERR  },
/* cont  */     { H2_OPEN,    H2_ERR,     H2_CLOCAL,  H2_SAME,     H2_ERR,     H2_SAME,    H2_ERR,     H2_ERR  },
/* end */       { H2_ERR,     H2_ERR,     H2_ERR,     H2_CREMOTE,  H2_CLOSED,  H2_ERR,     H2_ERR,     H2_ERR  },

/* Transmit */
/* end */       { H2_ERR,     H2_ERR,     H2_ERR,     H2_CLOCAL,   H2_ERR,     H2_CLOSED,  H2_ERR,     H2_ERR  },
/* push */      { H2_RLOCAL,  H2_ERR,     H2_ERR,     H2_ERR,      H2_ERR,     H2_ERR,     H2_ERR,     H2_ERR  },
/* reset */     { H2_CLOSED,  H2_CLOSED,  H2_CLOSED,  H2_CLOSED,   H2_CLOSED,  H2_CLOSED,  H2_ERR,     H2_ERR  },
/* header */    { H2_OPEN,    H2_OPEN,    H2_ERR,     H2_SAME,     H2_ERR,     H2_SAME,    H2_ERR,     H2_ERR  },
/* data */      { H2_ERR,     H2_ERR,     H2_ERR,     H2_SAME,     H2_ERR,     H2_SAME,    H2_ERR,     H2_ERR  },
};

/*********************************** Code *************************************/
/*
    Loadable module initialization
 */
PUBLIC int httpOpenHttp2Filter()
{
    HttpStage     *filter;

    if ((filter = httpCreateConnector("Http2Filter", NULL)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    HTTP->http2Filter = filter;
    filter->incoming = incomingHttp2;
    filter->outgoing = outgoingHttp2;
    filter->outgoingService = outgoingHttp2Service;
    httpCreatePackedHeaders();
    return 0;
}


/*
    Receive and process incoming HTTP/2 packets.
 */
static void incomingHttp2(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;
    HttpStream  *stream;
    HttpFrame   *frame;
    int         done;

    net = q->net;

    /*
        Join packets into a single packet for processing. Typically will be only one packet and do nothing.
     */
    httpJoinPacketForService(q, packet, HTTP_DELAY_SERVICE);
    sendSettings(q);
    /*
        Process frames until can process no more. Initially will be only one packet, but the frame handlers
        may split packets as required and put back the tail for processing here.
     */
    for (done = 0, packet = httpGetPacket(q); packet && !net->receivedGoaway; packet = httpGetPacket(q)) {

        if ((frame = parseFrame(q, packet, &done)) != 0) {
            net->frame = frame;
            frameHandlers[frame->type](q, packet);
            net->frame = 0;
            stream = frame->stream;

            httpServiceNetQueues(net, 0);

            if (stream && !stream->destroyed) {
                if (stream->disconnect) {
                    sendReset(q, stream, HTTP2_INTERNAL_ERROR, "Stream request error %s", stream->errorMsg);
                }
                if (stream->h2State >= H2_CLOSED) {
                    httpDestroyStream(stream);
                }
            }
        }
        if (done) {
            break;
        }
        if (mprNeedYield()) {
            mprYield(0);
        }
    }
    closeNetworkWhenDone(q);
}


/*
    Accept packet for sending
 */
static void outgoingHttp2(HttpQueue *q, HttpPacket *packet)
{
    HttpStream  *stream;

    stream = packet->stream;

    sendSettings(q);
    if (stream->streamID == 0 && !httpIsServer(stream->net)) {
        pickStreamNumber(stream);
    }
    if (packet->flags & HTTP_PACKET_DATA) {
        // Must compute window here as this is run synchronously from the upstream sender
        assert(stream->outputq->window >= 0);
        stream->outputq->window -= httpGetPacketLength(packet);
        assert(stream->outputq->window >= 0);
    }
    httpPutForService(q, packet, HTTP_SCHEDULE_QUEUE);
}


/*
    Service the outgoing queue of packets. This uses HTTP/2 for end to end flow control rather than
    using httpWillQueueAcceptPacket for downstream flow control.
 */
static void outgoingHttp2Service(HttpQueue *q)
{
    HttpNet     *net;
    HttpStream  *stream;
    HttpPacket  *packet, *frame;
    HttpTx      *tx;
    int         flags;

    net = q->net;
    assert(!(q->flags & HTTP_QUEUE_SUSPENDED));

    /*
        Note: httpGetPacket will not automatically resume the previous queue (which logically is each streams' tailFilter).
        Note: q->nextQ == q->prevQ == socketq, so we must explicitly re-enable the stream's tail filter below.
     */
    for (packet = httpGetPacket(q); packet && !net->error; packet = httpGetPacket(q)) {
        net->lastActivity = net->http->now;

        setLastPacket(q, packet);
        if (net->outputq->window <= 0 || net->socketq->count >= net->socketq->max) {
            /*
                The output queue has depleted the HTTP/2 transmit window. Flow control and wait for
                a window update message from the peer.
             */
            httpSuspendQueue(q);
            httpPutBackPacket(q, packet);
            break;
        }
        stream = packet->stream;

        if (stream && !stream->destroyed) {
            if (packet->flags & HTTP_PACKET_HEADER) {
                if (setState(stream, E2_TX_HDR) == H2_ERR) {
                    invalidState(net, stream, E2_TX_HDR);
                    break;
                }
            } else if (packet->flags & HTTP_PACKET_DATA) {
                if (setState(stream, E2_TX_DATA) == H2_ERR) {
                    invalidState(net, stream, E2_TX_DATA);
                    break;
                }
                /*
                    Resize data packets to not exceed the remaining HTTP/2 window flow control credits.
                 */
                resizePacket(net->outputq, net->outputq->window, packet);
            }

            if (net->receivedGoaway && (net->lastStreamID && stream->streamID >= net->lastStreamID)) {
                /* Network is being closed. Continue to process existing streams but accept no new streams */
                continue;
            }
            if (stream->disconnect) {
                sendReset(q, stream, HTTP2_INTERNAL_ERROR, "Stream request error %s", stream->errorMsg);
                continue;
            }
            stream->lastActivity = stream->http->now;
            tx = stream->tx;

            if (packet->flags & HTTP_PACKET_END && tx->allDataSent) {
                httpPutPacket(q->net->socketq, packet);
                packet = 0;
            }
            assert(net->outputq->window > 0);

            if (packet) {
                flags = getFrameFlags(q, packet);
                if (flags & HTTP2_END_STREAM_FLAG) {
                    if (setState(stream, E2_TX_END) == H2_ERR) {
                        invalidState(net, stream, E2_TX_END);
                        break;
                    }
                }
                if (packet->flags & HTTP_PACKET_HEADER) {
                    if (stream->tx->startedHeader) {
                        packet->type = HTTP2_CONT_FRAME;
                    } else {
                        packet->type = HTTP2_HEADERS_FRAME;
                        stream->tx->startedHeader = 1;
                    }
                } else if (packet->flags & (HTTP_PACKET_DATA | HTTP_PACKET_END)) {
                    packet->type = HTTP2_DATA_FRAME;
                }
                frame = defineFrame(q, packet, packet->type, flags, stream->streamID);
                sendFrame(q, frame);
            }

            /*
                Resume upstream if there is now room
             */
            if (q->count <= q->low && (stream->outputq->flags & HTTP_QUEUE_SUSPENDED)) {
                httpResumeQueue(stream->outputq, 0);
            }
        }
        if (net->outputq->window <= 0) {
            httpSuspendQueue(q);
            break;
        }
    }

    if (net->outputq->window >= 0 && q->count <= q->low && !(q->flags & HTTP_QUEUE_SUSPENDED)) {
        restartSuspendedStreams(net);
    }
    closeNetworkWhenDone(q);
}


/*
    Determine if this is the last packet in the stream. If packet is END, or if packet is data and the next packet is END,
    or packet is header and it is followed by NOT a header.
 */
static void setLastPacket(HttpQueue *q, HttpPacket *packet)
{
    //HttpNet     *net;
    HttpPacket  *next;
    int         last;

    //net = q->net;
    next = q->first;
    last = (
        (packet->flags & HTTP_PACKET_HEADER && !(!next || next->flags & HTTP_PACKET_HEADER)) ||
        (packet->flags & HTTP_PACKET_DATA && next && next->flags & HTTP_PACKET_END) ||
        (packet->flags & HTTP_PACKET_END));
    packet->last = last;
}


/*
    Get the HTTP/2 frame flags for this packet
 */
static int getFrameFlags(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;
    HttpStream  *stream;
    HttpTx      *tx;
    int         flags;

    net = q->net;
    stream = packet->stream;
    tx = stream->tx;
    flags = 0;

    if (packet->flags & HTTP_PACKET_HEADER && packet->last) {
        flags |= HTTP2_END_HEADERS_FLAG;

    } else if (packet->flags & HTTP_PACKET_DATA && packet->last) {
        flags |= HTTP2_END_STREAM_FLAG;
        tx->allDataSent = 1;

    } else if (packet->flags & HTTP_PACKET_END && !tx->allDataSent) {
        flags |= HTTP2_END_STREAM_FLAG;
        tx->allDataSent = 1;
    }
    if (packet->flags & HTTP_PACKET_DATA) {
        assert(net->outputq->window > 0);
        net->outputq->window -= httpGetPacketLength(packet);
    }
    return flags;
}


/*
    Resize a packet to utilize the remaining HTTP/2 window credits. Must not exceed the remaining window size.
    OPT - would be more optimal to not split packets
 */
static ssize resizePacket(HttpQueue *q, ssize max, HttpPacket *packet)
{
    HttpPacket  *tail;
    ssize       len;

    len = httpGetPacketLength(packet);
    if (len > max) {
        if ((tail = httpSplitPacket(packet, max)) == 0) {
            /* Memory error - centrally reported */
            return len;
        }
        httpPutBackPacket(q, tail);
        len = httpGetPacketLength(packet);
    }
    return len;
}


PUBLIC void httpFinalizeHttp2Stream(HttpStream *stream)
{
#if UNUSED
    /*
        Half closed stream must still receive certain packets (see h2spec)
    */
    if (stream->h2State >= H2_CLOSED) {
        httpDestroyStream(stream);
    }
#endif
}



/*
    Close the network connection on errors of if instructed to go away.
 */
static void closeNetworkWhenDone(HttpQueue *q)
{
    HttpNet     *net;

    net = q->net;
    if (net->error) {
        if (!net->sentGoaway) {
            sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Closing network");
        }
    }
    if (net->receivedGoaway) {
        if (net->sentGoaway) {
            if (mprGetListLength(net->streams) == 0) {
                /* This ensures a recall on the netConnector IOEvent handler */
                mprDisconnectSocket(net->sock);
            }
        } else {
            sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Closing network");
        }
    }
}

static void logIncomingPacket(HttpQueue *q, HttpPacket *packet, ssize payloadLen, int type, int streamID, int flags)
{
    MprBuf      *buf;
    cchar       *typeStr;
    ssize       excess, len;

    buf = packet->content;
    if (httpTracing(q->net)) {
        len = httpGetPacketLength(packet);
        excess = len - payloadLen;
        if (excess > 0) {
            mprAdjustBufEnd(buf, -excess);
        }
        mprAdjustBufStart(buf, -HTTP2_FRAME_OVERHEAD);

        typeStr = (type < HTTP2_MAX_FRAME) ? packetTypes[type] : "unknown";
        httpLog(q->net->trace, "rx.http2", "packet",
            "frame=%s flags=%x stream=%d length=%zd", typeStr, flags, streamID, httpGetPacketLength(packet));

        mprAdjustBufStart(buf, HTTP2_FRAME_OVERHEAD);
        if (excess > 0) {
            mprAdjustBufEnd(buf, excess);
        }
    }
}


/*
    Parse an incoming HTTP/2 frame. Return true to keep going with this or subsequent request, zero means
    insufficient data to proceed.
 */
static HttpFrame *parseFrame(HttpQueue *q, HttpPacket *packet, int *done)
{
    HttpNet     *net;
    HttpPacket  *tail;
    MprBuf      *buf;
    ssize       payloadLen, size;
    uint32      lenType;
    uint        flags, type;
    int         streamID;

    net = q->net;
    buf = packet->content;
    *done = 0;

    size = mprGetBufLength(buf);
    if (size < HTTP2_FRAME_OVERHEAD) {
        if (size > 0) {
            /* Insufficient data */
            httpPutBackPacket(q, packet);
        }
        *done = 1;
        return 0;
    }
    /*
        Length is 24 bits. Read with type
     */
    lenType = mprGetUint32FromBuf(buf);
    type = lenType & 0xFF;
    payloadLen = lenType >> 8;
    flags = mprGetCharFromBuf(buf);
    streamID = mprGetUint32FromBuf(buf) & HTTP_STREAM_MASK;

    if (payloadLen > q->packetSize || payloadLen > HTTP2_MAX_FRAME_SIZE) {
        logIncomingPacket(q, packet, payloadLen, type, streamID, flags);
        sendGoAway(q, HTTP2_FRAME_SIZE_ERROR, "Bad frame size %d vs %d", payloadLen, q->packetSize);
        return 0;
    }
    if (net->sentGoaway && net->lastStreamID && streamID >= net->lastStreamID) {
        logIncomingPacket(q, packet, payloadLen, type, streamID, flags);
        /* Network is being closed. Continue to process existing streams but accept no new streams */
        return 0;
    }

    /*
        Split data for a following frame and put back on the queue for later servicing.
     */
    size = mprGetBufLength(buf);

    if (payloadLen > size) {
        mprAdjustBufStart(buf, -HTTP2_FRAME_OVERHEAD);
        httpPutBackPacket(q, packet);
        //  Don't log packet yet
        *done = 1;
        return 0;
    }
    logIncomingPacket(q, packet, payloadLen, type, streamID, flags);

    if (payloadLen < size) {
        if ((tail = httpSplitPacket(packet, payloadLen)) == 0) {
            /* Memory error - centrally reported */
            *done = 1;
            return 0;
        }
        httpPutBackPacket(q, tail);
        buf = packet->content;
    }
    return createFrame(q, packet, type, flags, streamID);
}


static HttpFrame *createFrame(HttpQueue *q, HttpPacket *packet, int type, int flags, int streamID)
{
    HttpNet     *net;
    HttpFrame   *frame;
    HttpStream  *stream;
    int         state;

    /*
        Parse the various HTTP/2 frame fields and store in a local HttpFrame object.
     */
    if ((frame = mprAllocObj(HttpFrame, manageFrame)) == NULL) {
        /* Memory error - centrally reported */
        return 0;
    }
    net = q->net;
    packet->data = frame;
    frame->type = type;
    frame->flags = flags;
    frame->streamID = streamID;

    stream = frame->stream = findStream(net, streamID);

    if (frame->type < 0 || frame->type >= HTTP2_MAX_FRAME) {
        //  Must ignore invalid or unknown frame types except when parsing headers
        if (net->parsingHeaders) {
            sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Invalid frame while parsing headers type %d, stream %d", frame->type, frame->streamID);
        }
        return 0;
    }
    if (stream) {
        if ((state = setState(stream, type)) == H2_ERR) {
            invalidState(net, stream, type);
            return 0;

        } else if (flags & HTTP2_END_STREAM_FLAG && type != HTTP2_HEADERS_FRAME && type != HTTP2_CONT_FRAME) {
            stream->rx->endStream = 1;
            state = setState(stream, E2_END);
            if (state == H2_ERR) {
                invalidState(net, stream, E2_END);
                return 0;
            }
        }
        if (net->parsingHeaders && type != HTTP2_CONT_FRAME) {
            invalidState(net, stream, type);
            return 0;
        }
    }
    return frame;
}


/*
    Always get a settings frame at the start of any network connection
 */
static void parseSettingsFrame(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;
    HttpFrame   *frame;
    HttpLimits  *limits;
    HttpStream  *stream;
    MprBuf      *buf;
    uint        field, value;
    int         diff, next;

    net = q->net;
    limits = net->limits;
    buf = packet->content;
    frame = packet->data;

    if (frame->flags & HTTP2_ACK_FLAG || net->sentGoaway || net->receivedGoaway) {
        if (httpGetPacketLength(packet) > 0) {
            sendGoAway(q, HTTP2_FRAME_SIZE_ERROR, "Invalid settings packet with both Ack and payload");
        } else {
            /* Simple ack */
        }
        return;
    }
    if (frame->streamID) {
        sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Invalid settings packet with streamID %d", frame->streamID);
        return;
    }
    while (httpGetPacketLength(packet) >= HTTP2_SETTINGS_SIZE) {
        field = mprGetUint16FromBuf(buf);
        value = mprGetUint32FromBuf(buf);

        switch (field) {
        case HTTP2_HEADER_TABLE_SIZE_SETTING:
            value = min((int) value, limits->hpackMax);
            httpSetPackedHeadersMax(net->txHeaders, value);
            break;

        case HTTP2_ENABLE_PUSH_SETTING:
            if (value != 0 && value != 1) {
                sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Invalid push value");
                return;
            }
            /* Push is not yet supported, we just store the value but do nothing */
            net->push = value;
            break;

        case HTTP2_MAX_STREAMS_SETTING:
            /* Permit peer supporting more streams, but don't ever create more than streamsMax limit */
            if (value <= 0) {
                sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Too many streams setting %d max %d", value, ME_MAX_STREAMS);
                return;
            }
            limits->txStreamsMax = min((int) value, limits->streamsMax);
            break;

        case HTTP2_INIT_WINDOW_SIZE_SETTING:
            if (value > HTTP2_MAX_WINDOW) {
                sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Invalid window size setting %d max %d", value, HTTP2_MAX_WINDOW);
                return;
            }
            if (mprGetListLength(net->streams) > 0) {
                diff = (int) value - (int) net->window;
                for (ITERATE_ITEMS(net->streams, stream, next)) {
                    if (stream->h2State == H2_OPEN || stream->h2State == H2_CREMOTE) {
                        stream->outputq->window += diff;
                    }
                }
            } else {
                net->window = value;
            }
            break;

        case HTTP2_MAX_FRAME_SIZE_SETTING:
            /* Permit peer supporting bigger frame sizes, but don't ever create packets larger than the packetSize limit */
            if (value < HTTP2_MIN_FRAME_SIZE || value > HTTP2_MAX_FRAME_SIZE) {
                sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Invalid frame size setting %d max %d", value, ME_PACKET_SIZE);
                return;
            }
            if (value < net->outputq->packetSize) {
                net->outputq->packetSize = min(value, ME_PACKET_SIZE);
            }
            break;

        case HTTP2_MAX_HEADER_SIZE_SETTING:
            if (value <= 0 || value > ME_MAX_HEADERS) {
                sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Invalid header size setting %d max %d", value, ME_MAX_HEADERS);
                return;
            }
            if ((int) value < limits->headerSize) {
                if ((limits = httpCreateLimits(1)) != 0) {
                    memcpy(limits, net->limits, sizeof(HttpLimits));
                    limits->headerSize = value;
                    net->limits = limits;
                }
            }
            break;

        default:
            /* Ignore unknown settings values (per spec) */
            break;
        }
    }
    if (httpGetPacketLength(packet) > 0) {
        sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Invalid setting packet length");
        return;
    }
    mprFlushBuf(packet->content);
    sendFrame(q, defineFrame(q, packet, HTTP2_SETTINGS_FRAME, HTTP2_ACK_FLAG, 0));
    restartSuspendedStreams(net);
}


/*
    Parse a HTTP header or HTTP header continuation frame
 */
static void parseHeaderFrame(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;
    HttpStream  *stream;
    HttpFrame   *frame;
    MprBuf      *buf;
    bool        padded, priority;
    ssize       size, payloadLen;
    int         value, padLen;

    net = q->net;
    buf = packet->content;
    frame = packet->data;
    stream = frame->stream;
    padded = frame->flags & HTTP2_PADDED_FLAG;
    priority = frame->flags & HTTP2_PRIORITY_FLAG;

    size = 0;
    if (padded) {
        size++;
    }
    if (priority) {
        /* dependency + weight */
        size += sizeof(uint32) + 1;
    }
    payloadLen = mprGetBufLength(buf);
    if (payloadLen <= size) {
        sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Incorrect header length");
        return;
    }
    if (padded) {
        padLen = (int) mprGetCharFromBuf(buf);
        if (padLen >= payloadLen) {
            sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Incorrect padding length");
            return;
        }
        mprAdjustBufEnd(buf, -padLen);
    }
    frame->weight = HTTP2_DEFAULT_WEIGHT;
    if (priority) {
        /* Priority Frames */
        value = mprGetUint32FromBuf(buf);
        frame->depend = value & 0x7fffffff;
        frame->exclusive = value >> 31;
        frame->weight = mprGetCharFromBuf(buf) + 1;
    }
    if ((frame->streamID % 2) != 1 || (net->lastStreamID && frame->streamID < net->lastStreamID)) {
        sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Bad sesssion");
        return;
    }
    if ((stream = getStream(q, packet)) != 0) {
        if (setState(stream, frame->type) == H2_ERR) {
            invalidState(net, stream, frame->type);
            return;
        }
        if (frame->flags & HTTP2_END_STREAM_FLAG) {
            /*
                Replicate here from parseFrame because parseFrame did not have the stream
             */
            if (setState(stream, E2_END) == H2_ERR) {
                invalidState(net, stream, E2_END);
                return;
            }
            stream->rx->endStream = 1;

        } else if (stream->state > HTTP_STATE_PARSED) {
            //  Trailer must have end stream
            sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Trailer headers must have end stream");
            return;
        }
        if (frame->type == HTTP2_CONT_FRAME && !net->parsingHeaders) {
            sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Unexpected continue frame");
            return;
        }
        net->parsingHeaders = 1;
        if (frame->flags & HTTP2_END_HEADERS_FLAG) {
            net->parsingHeaders = 0;
            parseHeaders2(q, stream);
        }
        /*
            Must only update for a successfully received frame
         */
        if (!net->error) {
            net->lastStreamID = frame->streamID;
        }
    }
}


/*
    Once the header frame and all continuation frames are received, they are joined into a single rx->headerPacket.
 */
static void parseHeaders2(HttpQueue *q, HttpStream *stream)
{
    HttpNet     *net;
    HttpPacket  *packet;
    HttpRx      *rx;

    net = stream->net;
    rx = stream->rx;
    packet = rx->headerPacket;

    while (httpGetPacketLength(packet) > 0 && !net->sentGoaway) {
        if (!parseHeader(q, stream, packet)) {
            sendReset(q, stream, HTTP2_STREAM_CLOSED, "Cannot parse headers");
            break;
        }
    }
    if (httpIsServer(net)) {
        if (!rx->method || !rx->scheme || !rx->uri) {
            sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Missing :method, :path or :scheme in request headers");
            return;
        }
    }
    if (rx->endStream) {
        httpAddInputEndPacket(stream, stream->inputq);
    }
    if (!net->sentGoaway) {
        rx->protocol = sclone("HTTP/2.0");
        httpSetState(stream, HTTP_STATE_PARSED);
    }
}


/*
    Parse the next header item in the packet of headers
 */
static bool parseHeader(HttpQueue *q, HttpStream *stream, HttpPacket *packet)
{
    HttpNet     *net;
    MprBuf      *buf;
    MprKeyValue *kp;
    cchar       *name, *value;
    uchar       ch;
    int         index, max;

    net = stream->net;
    buf = packet->content;

    /*
        Decode the type of header record. It can be:
        1. Fully indexed header field.
        2. Literal header that should be added to the header table.
        3. Literal header without updating the header table.
     */
    ch = mprLookAtNextCharInBuf(buf);
    if ((ch >> 7) == 1) {
        /*
            Fully indexed header field
         */
        index = decodeInt(packet, 7);
        if ((kp = httpGetPackedHeader(net->rxHeaders, index)) == 0) {
            sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Bad header prefix");
            return 0;
        }
        if (!addHeaderToSet(stream, kp->key, kp->value)) {
            return 0;
        }

    } else if ((ch >> 6) == 1) {
        /*
            Literal header and add to index
         */
        if ((index = decodeInt(packet, 6)) < 0) {
            sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Bad header prefix");
            return 0;
        } else if (index > 0) {
            if ((kp = httpGetPackedHeader(net->rxHeaders, index)) == 0) {
                sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Unknown header index");
                return 0;
            }
            name = kp->key;
        } else {
            name = parseHeaderField(q, stream, packet);
        }
        value = parseHeaderField(q, stream, packet);
        if (!addHeaderToSet(stream, name, value)) {
            return 0;
        }
        if (httpAddPackedHeader(net->rxHeaders, name, value) < 0) {
            sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Cannot fit header in hpack table");
            return 0;
        }

    } else if ((ch >> 5) == 1) {
        /* Dynamic table max size update */
        if (stream->rx->headers->length) {
            sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Bad hpack dynamic table update after started parsing headers");
            return 0;
        }
        max = decodeInt(packet, 5);
        if (httpSetPackedHeadersMax(net->rxHeaders, max) < 0) {
            sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Cannot add indexed header");
            return 0;
        }

    } else /* if ((ch >> 4) == 1 || (ch >> 4) == 0)) */ {
        /* Literal header field without indexing */
        if ((index = decodeInt(packet, 4)) < 0) {
            sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Bad header prefix");
            return 0;
        } else if (index > 0) {
            if ((kp = httpGetPackedHeader(net->rxHeaders, index)) == 0) {
                sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Unknown header index");
                return 0;
            }
            name = kp->key;
        } else {
            name = parseHeaderField(q, stream, packet);
        }
        value = parseHeaderField(q, stream, packet);
        if (!addHeaderToSet(stream, name, value)) {
            return 0;
        }
    }
    return 1;
}


/*
    Parse a single header field
 */
static cchar *parseHeaderField(HttpQueue *q, HttpStream *stream, HttpPacket *packet)
{
    MprBuf      *buf;
    cchar       *value;
    int         huff, len;

    buf = packet->content;

    huff = ((uchar) mprLookAtNextCharInBuf(buf)) >> 7;
    len = decodeInt(packet, 7);
    if (len < 0 || len > mprGetBufLength(buf)) {
        sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Invalid header field length");
        return 0;
    }
    if (huff) {
        /*
            Huffman encoded
         */
        if ((value = httpHuffDecode((uchar*) mprGetBufStart(buf), len)) == 0) {
            sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Invalid encoded header field");
            return 0;
        }
    } else {
        /* Literal */
        value = snclone(buf->start, len);
    }
    mprAdjustBufStart(buf, len);
    return value;
}


/*
    Add a header key/value pair to the set of headers for the stream (stream)
 */
static bool addHeaderToSet(HttpStream *stream, cchar *key, cchar *value)
{
    HttpNet     *net;
    HttpRx      *rx;
    HttpLimits  *limits;
    ssize       len;

    net = stream ->net;
    rx = stream->rx;
    limits = stream->limits;

    if (!key || !value) {
        sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Invalid header missing key or value");
        return 0;
    }
    if (!validateHeader(key, value)) {
        sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Invalid header name/value");
        return 0;
    }

    if (key[0] == ':') {
        if (rx->seenRegularHeader) {
            sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Invalid pseudo header after regular header");
            return 0;
        }
        if (httpIsServer(net)) {
            if (key[1] == 'a' && smatch(key, ":authority")) {
                mprAddKey(stream->rx->headers, "host", value);

            } else if (key[1] == 'm' && smatch(key, ":method")) {
                if (rx->method || *value == '\0') {
                    sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Invalid or duplicate :method in headers");
                    return 0;
                } else {
                    rx->originalMethod = rx->method = supper(value);
                    httpParseMethod(stream);
                }

            } else if (key[1] == 'p' && smatch(key, ":path")) {
                if (rx->uri) {
                    sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Duplicate :path in headers");
                    return 0;
                } else {
                    len = slen(value);
                    if (*value == '\0') {
                        sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Bad request path");
                        return 0;
                    } else if (len >= limits->uriSize) {
                        httpLimitError(stream, HTTP_CLOSE | HTTP_CODE_REQUEST_URL_TOO_LARGE,
                            "Bad request. URI too long. Length %zd vs limit %d", len, limits->uriSize);
                        return 0;
                    }
                    rx->uri = sclone(value);
                    if (!rx->originalUri) {
                        rx->originalUri = rx->uri;
                    }
                }

            } else if (smatch(key, ":scheme")) {
                if (rx->scheme) {
                    sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Invalid duplicate pseudo header");
                } else {
                    rx->scheme = sclone(value);
                }

            } else {
                sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Invalid pseudo header");
                return 0;
            }
        } else {
            if (key[1] == 's' && smatch(key, ":status")) {
                rx->status = atoi(value);
            } else {
                sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Invalid pseudo header");
                return 0;
            }
        }
    } else {
        rx->seenRegularHeader = 1;
        if (scaselessmatch(key, "connection")) {
            sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Invalid connection header");
        } else if (scaselessmatch(key, "te") && !smatch(value, "trailers")) {
            sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Invalid connection header");
        } else if (scaselessmatch(key, "set-cookie") || scaselessmatch(key, "cookie")) {
            mprAddDuplicateKey(rx->headers, key, value);
        } else if (scaselessmatch(key, "content-length")) {
            rx->http2ContentLength = stoi(value);
        } else {
            mprAddKey(rx->headers, key, value);
        }
    }
    return 1;
}


/*
    Briefly, do some validation
 */
static bool validateHeader(cchar *key, cchar *value)
{
    uchar   *cp, c;

    if (!key || *key == 0 || !value || !value) {
        return 0;
    }
    if (*key == ':') {
        key++;
    }
    for (cp = (uchar*) key; *cp; cp++) {
        c = *cp;
        if (('a' <= c && c <= 'z') || c == '-' || c == '_' || ('0' <= c && c <= '9')) {
            continue;
        }
        if (c == '\0' || c == '\n' || c == '\r' || c == ':' || ('A' <= c && c <= 'Z')) {
            mprLog("info http", 5, "Invalid header name %s", key);
            return 0;
        }
    }
    for (cp = (uchar*) value; *cp; cp++) {
        c = *cp;
        if (c == '\0' || c == '\n' || c == '\r') {
            mprLog("info http", 5, "Invalid header value %s", value);
            return 0;
        }
    }
    return 1;
}


/*
    Priority frames are not yet implemented. They are parsed but not validated or implemented.
 */
static void parsePriorityFrame(HttpQueue *q, HttpPacket *packet)
{
    HttpFrame   *frame;
    //HttpStream  *stream;
    MprBuf      *buf;
    int         value;

    frame = packet->data;
    //stream = frame->stream;

    //  Firefox sends priority before stream allocated (has streamID)
    if (/* !stream || */ q->net->parsingHeaders || httpGetPacketLength(packet) != HTTP2_PRIORITY_SIZE) {
        sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Bad priority frame");
        return;
    }
    buf = packet->content;
    value = mprGetUint32FromBuf(buf);
    frame->depend = value & 0x7fffffff;
    frame->exclusive = value >> 31;
    frame->weight = mprGetCharFromBuf(buf) + 1;

    if (frame->depend == frame->streamID) {
        sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Bad stream dependency in priority frame");
    }
}


/*
    Push frames are not yet implemented
 */
static void parsePushFrame(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;

    net = q->net;
    if (httpIsServer(net)) {
        sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Invalid PUSH frame from client");
    } else {
        sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "PUSH frames not implemented");
    }
#if FUTURE
    stream->h2State = H2_RESERVED;
#endif
}


/*
    Receive a ping frame
 */
static void parsePingFrame(HttpQueue *q, HttpPacket *packet)
{
    HttpFrame   *frame;

    frame = packet->data;

    if (q->net->sentGoaway) {
        sendGoAway(q, HTTP2_STREAM_CLOSED, "Network already closed");
        return;
    }
    if (frame->streamID) {
        sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Bad stream in ping frame");
        return;
    }
    if (!(frame->flags & HTTP2_ACK_FLAG)) {
        /* Resend the ping payload with the acknowledgement */
        sendFrame(q, defineFrame(q, packet, HTTP2_PING_FRAME, HTTP2_ACK_FLAG, 0));
    }
}


/*
    Peer is instructing the stream to be closed.
    5.1, 6.4 -- Must protocol error reset on an idle stream
    2..1 -- Must accept reset on 1/2 closed stream
 */
static void parseResetFrame(HttpQueue *q, HttpPacket *packet)
{
    HttpStream  *stream;
    HttpFrame   *frame;
    uint32      error;

    frame = packet->data;
    stream = frame->stream;

    if (httpGetPacketLength(packet) != sizeof(uint32) || frame->streamID == 0) {
        sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Bad reset frame");
        return;
    }
    if (!stream) {
        if (frame->streamID >= q->net->lastStreamID) {
            //  Reset on an idle stream
            sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Bad reset frame");
        } else {
            packet = httpCreatePacket(HTTP2_RESET_SIZE);
            mprPutUint32ToBuf(packet->content, HTTP2_STREAM_CLOSED);
            sendFrame(q, defineFrame(q, packet, HTTP2_RESET_FRAME, 0, frame->streamID));
        }
        return;
    }
    /*
        Received reset packets can race with the stream being closed
     */
    if (stream->h2State != H2_CLOSED) {
        error = mprGetUint32FromBuf(packet->content) & HTTP_STREAM_MASK;
        resetStream(frame->stream, "Stream reset by peer", error);
    }
}


/*
    Receive a GoAway which informs us that this network should not be used anymore.
 */
static void parseGoAwayFrame(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;
    HttpStream  *stream;
    MprBuf      *buf;
    cchar       *msg;
    ssize       len;
    int         error, lastStreamID, next;

    net = q->net;
    buf = packet->content;
    lastStreamID = mprGetUint32FromBuf(buf) & HTTP_STREAM_MASK;
    error = mprGetUint32FromBuf(buf);
    len = mprGetBufLength(buf);
    msg = len ? snclone(buf->start, len) : "";

    net->receivedGoaway = 1;

    httpLog(net->trace, "rx.http2", "context", "msg:Receive GoAway. %s, error:%d, lastStream:%d", msg, error, lastStreamID);

    for (ITERATE_ITEMS(net->streams, stream, next)) {
        if (stream->streamID > lastStreamID) {
            resetStream(stream, "Stream reset by peer", HTTP2_REFUSED_STREAM);
        }
    }
}


/*
    Receive a window update frame that increases the window size of permissible data to send.
    This is a credit based system for flow control of both the network and the stream.
 */
static void parseWindowFrame(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;
    HttpStream  *stream;
    HttpFrame   *frame;
    int         increment;

    net = q->net;
    frame = packet->data;
    increment = mprGetUint32FromBuf(packet->content);
    if (increment == 0) {
        sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Bad window frame size of zero");
        return;
    }
    stream = frame->stream;
    if (stream) {
        if (increment > (HTTP2_MAX_WINDOW - stream->outputq->window)) {
            sendReset(q, stream, HTTP2_FLOW_CONTROL_ERROR, "Invalid window update for stream %d", stream->streamID);
        } else {
            stream->outputq->window += increment;
            httpResumeQueue(stream->outputq, 0);
        }
    } else {
        if (frame->streamID) {
            if (frame->streamID >= net->lastStreamID) {
                sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Bad window frame");
            } else {
                //  Ignore window frames for recent streams that have been closed
            }
        } else if (increment > (HTTP2_MAX_WINDOW + 1 - net->outputq->window)) {
            sendGoAway(q, HTTP2_FLOW_CONTROL_ERROR, "Invalid window update for network");

        } else {
            net->outputq->window += increment;
            httpResumeQueue(net->outputq, 0);
        }
    }
}


/*
    Receive an application data frame
 */
static void parseDataFrame(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;
    HttpFrame   *frame;
    HttpStream  *stream;
    HttpLimits  *limits;
    MprBuf      *buf;
    ssize       len, padLen, payloadLen;
    int         padded;

    net = q->net;
    limits = net->limits;
    buf = packet->content;
    frame = packet->data;
    stream = frame->stream;

    if (!stream) {
        sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Received data frame without stream");
        return;
    }
    padded = frame->flags & HTTP2_PADDED_FLAG;
    if (padded) {
        payloadLen = mprGetBufLength(buf);
        padLen = (int) mprGetCharFromBuf(buf);
        if (padLen >= payloadLen) {
            sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Incorrect padding length");
            return;
        }
        mprAdjustBufEnd(buf, -padLen);
    }
    len = httpGetPacketLength(packet);

    processDataFrame(q, packet);

    /*
        Network flow control, do after processing the data frame incase the stream is now complete.
     */
    if (len > net->inputq->window) {
        sendGoAway(q, HTTP2_FLOW_CONTROL_ERROR, "Peer exceeded flow control window");
        return;
    }
    net->inputq->window -= len;
    assert(net->inputq->window >= 0);

    if (net->inputq->window <= net->inputq->packetSize) {
        /*
            Update the remote window size for network flow control
         */
        sendWindowFrame(q, 0, limits->window - net->inputq->window);
        net->inputq->window = limits->window;
    }

    /*
        Stream flow control
     */
    if (!stream->destroyed) {
        if (len > stream->inputq->window) {
            sendReset(q, stream, HTTP2_FLOW_CONTROL_ERROR, "Receive data exceeds window for stream");
            return;
        }
        stream->inputq->window -= len;

        if (stream->inputq->window <= net->inputq->packetSize) {
            /*
                Update the remote window size for stream flow control
             */
            sendWindowFrame(q, stream->streamID, limits->window - stream->inputq->window);
            stream->inputq->window = limits->window;
        }
    }
}


/*
    Process the frame and add to the stream input queue
 */
static void processDataFrame(HttpQueue *q, HttpPacket *packet)
{
    HttpFrame   *frame;
    HttpStream  *stream;
    HttpRx      *rx;
    ssize       len;

    frame = packet->data;
    stream = frame->stream;
    rx = stream->rx;

    len = httpGetPacketLength(packet);
    rx->dataFrameLength += len;

    if ((rx->http2ContentLength >= 0) &&
            ((rx->dataFrameLength > rx->http2ContentLength) || (rx->eof && rx->dataFrameLength < rx->http2ContentLength))) {
        sendGoAway(q->net->socketq, HTTP2_PROTOCOL_ERROR, "Data content vs content-length mismatch");
        return;
    }
    if (httpGetPacketLength(packet) > 0) {
        httpPutPacket(stream->inputq, packet);
    }
    if (rx->endStream) {
        httpAddInputEndPacket(stream, stream->inputq);
    }
}


/*
    Public API to terminate a network connection
 */
PUBLIC void httpSendGoAway(HttpNet *net, int status, cchar *fmt, ...)
{
    va_list     ap;
    cchar       *msg;

    va_start(ap, fmt);
    msg = sfmtv(fmt, ap);
    va_end(ap);
    sendGoAway(net->outputq, status, "%s", msg);
}


/*
    Send a ping packet. Some intermediaries or peers may use pings to keep a connection alive.
 */
PUBLIC bool sendPing(HttpQueue *q, uchar *data)
{
    HttpPacket  *packet;

    if ((packet = httpCreatePacket(HTTP2_WINDOW_SIZE)) == 0) {
        return 0;
    }
    mprPutBlockToBuf(packet->content, (char*) data, 64);
    sendFrame(q, defineFrame(q, packet, HTTP2_PING_FRAME, 0, 0));
    return 1;
}


/*
    Immediately close a stream. The peer is informed and the stream is disconnected.
 */
static void sendReset(HttpQueue *q, HttpStream *stream, int status, cchar *fmt, ...)
{
    HttpPacket  *packet;
    va_list     ap;
    char        *msg;

    assert(stream);

    if (!stream || stream->destroyed) {
        return;
    }
    if (setState(stream, E2_RESET) == H2_ERR) {
        invalidState(q->net, stream, E2_RESET);
        return;
    }
    if ((packet = httpCreatePacket(HTTP2_RESET_SIZE)) == 0) {
        return;
    }
    va_start(ap, fmt);
    msg = sfmtv(fmt, ap);
    httpLog(stream->trace, "tx.http2", "context", "msg:Send stream reset, stream:%d, status:%d, msg:%s",
        stream->streamID, status, msg);
    va_end(ap);

    mprPutUint32ToBuf(packet->content, status);
    sendFrame(q, defineFrame(q, packet, HTTP2_RESET_FRAME, 0, stream->streamID));

    httpError(stream, HTTP_CODE_COMMS_ERROR, "%s", msg);
}


/*
    Mark a stream as being reset (terminated). Called in response to receipt of GoAway or Reset frame.
 */
static void resetStream(HttpStream *stream, cchar *msg, int error)
{
    httpLog(stream->trace, "rx.http2", "context", "msg:Receive GoAway. %s, error:%d", msg, error);
    setState(stream, E2_RESET);
}


/*
    A stream must exchange settings before it is used
 */
static void sendSettings(HttpQueue *q)
{
    HttpNet     *net;
    //HttpStream  *stream;

    net = q->net;
    //stream = q->stream;

    if (!net->init) {
        sendSettingsFrame(q);
        net->init = 1;
    }
}


static void pickStreamNumber(HttpStream *stream)
{
    HttpNet     *net;

    net = stream->net;
    stream->streamID = net->nextStreamID;
    net->nextStreamID += 2;
    //  TEST
    if (stream->streamID >= HTTP2_MAX_STREAM) {
        httpError(stream, HTTP_CODE_BAD_REQUEST, "Stream ID overflow");
    }
}


/*
    Clients must send a preface before settings
 */
static void sendPreface(HttpQueue *q)
{
    HttpPacket  *packet;

    if ((packet = httpCreatePacket(HTTP2_PREFACE_SIZE)) == 0) {
        return;
    }
    packet->flags = 0;
    mprPutBlockToBuf(packet->content, HTTP2_PREFACE, HTTP2_PREFACE_SIZE);
    httpPutPacket(q->net->socketq, packet);
}


/*
    Send a settings packet before using the stream
 */
static void sendSettingsFrame(HttpQueue *q)
{
    HttpNet     *net;
    HttpPacket  *packet;
    ssize       size;

    net = q->net;
    if (!net->init && httpIsClient(net)) {
        sendPreface(q);
    }
    if ((packet = httpCreatePacket(HTTP2_SETTINGS_SIZE * 3)) == 0) {
        return;
    }
    mprPutUint16ToBuf(packet->content, HTTP2_MAX_STREAMS_SETTING);
    mprPutUint32ToBuf(packet->content, net->limits->streamsMax - net->ownStreams);

    mprPutUint16ToBuf(packet->content, HTTP2_INIT_WINDOW_SIZE_SETTING);
    mprPutUint32ToBuf(packet->content, (uint32) net->limits->window);

    mprPutUint16ToBuf(packet->content, HTTP2_MAX_FRAME_SIZE_SETTING);
    size = max(net->inputq->packetSize, HTTP2_MIN_FRAME_SIZE);
    mprPutUint32ToBuf(packet->content, (uint32) size);

    mprPutUint16ToBuf(packet->content, HTTP2_HEADER_TABLE_SIZE_SETTING);
    mprPutUint32ToBuf(packet->content, (uint32) HTTP2_TABLE_SIZE);

#if FUTURE
    mprPutUint32ToBuf(packet->content, HTTP2_TABLE_SIZE);
    mprPutUint16ToBuf(packet->content, HTTP2_MAX_HEADER_SIZE_SETTING);
    mprPutUint32ToBuf(packet->content, (uint32) net->limits->headerSize);
    mprPutUint16ToBuf(packet->content, HTTP2_ENABLE_PUSH_SETTING);
    mprPutUint32ToBuf(packet->content, 0);
#endif

    sendFrame(q, defineFrame(q, packet, HTTP2_SETTINGS_FRAME, 0, 0));
}


static void sendWindowFrame(HttpQueue *q, int streamID, ssize inc)
{
    HttpPacket  *packet;

    assert(inc >= 0);
    if ((packet = httpCreatePacket(HTTP2_WINDOW_SIZE)) == 0) {
        return;
    }
    mprPutUint32ToBuf(packet->content, (uint32) inc);
    sendFrame(q, defineFrame(q, packet, HTTP2_WINDOW_FRAME, 0, streamID));
}


/*
    Populate the HTTP headers as a HTTP/2 header packet in the given packet
    This is called from the tailFilter and the packet is then split into packetSize chunks and passed to outgoingHttp2.
    There, the relevant HTTP/2 packet type is assigned HTTP2_HEADERS_FRAME or HTTP2_CONT_FRAME.
 */
PUBLIC void httpCreateHeaders2(HttpQueue *q, HttpPacket *packet)
{
    HttpStream  *stream;
    HttpTx      *tx;
    MprKey      *kp;

    assert(packet->flags == HTTP_PACKET_HEADER);

    stream = packet->stream;
    tx = stream->tx;
    if (tx->flags & HTTP_TX_HEADERS_CREATED) {
        return;
    }
    tx->responded = 1;

    httpPrepareHeaders(stream);
    definePseudoHeaders(stream, packet);
    if (httpTracing(q->net)) {
        if (httpServerStream(stream)) {
            httpLog(stream->trace, "tx.http2", "result", "@%s %d %s\n",
                httpGetProtocol(stream->net), tx->status, httpLookupStatus(tx->status));
            httpLog(stream->trace, "tx.http2", "headers", "@%s", httpTraceHeaders(stream->tx->headers));
        } else {
            httpLog(stream->trace, "tx.http2", "request", "@%s %s %s\n",
                tx->method, tx->parsedUri->path, httpGetProtocol(stream->net));
            httpLog(stream->trace, "tx.http2", "headers", "@%s", httpTraceHeaders(stream->tx->headers));
        }
    }

    /*
        Not emitting any padding, dependencies or weights.
     */
    for (ITERATE_KEYS(tx->headers, kp)) {
        if (kp->key[0] == ':') {
            if (smatch(kp->key, ":status")) {
                switch (tx->status) {
                case 200:
                    encodeInt(packet, httpSetPrefix(7), 7, HTTP2_STATUS_200); break;
                case 204:
                    encodeInt(packet, httpSetPrefix(7), 7, HTTP2_STATUS_204); break;
                case 206:
                    encodeInt(packet, httpSetPrefix(7), 7, HTTP2_STATUS_206); break;
                case 304:
                    encodeInt(packet, httpSetPrefix(7), 7, HTTP2_STATUS_304); break;
                case 400:
                    encodeInt(packet, httpSetPrefix(7), 7, HTTP2_STATUS_400); break;
                case 404:
                    encodeInt(packet, httpSetPrefix(7), 7, HTTP2_STATUS_404); break;
                case 500:
                    encodeInt(packet, httpSetPrefix(7), 7, HTTP2_STATUS_500); break;
                default:
                    encodeHeader(stream, packet, kp->key, kp->data);
                }
            } else if (smatch(kp->key, ":method")){
                if (smatch(kp->data, "GET")) {
                    encodeInt(packet, httpSetPrefix(7), 7, HTTP2_METHOD_GET);
                } else if (smatch(kp->data, "POST")) {
                    encodeInt(packet, httpSetPrefix(7), 7, HTTP2_METHOD_POST);
                } else {
                    encodeHeader(stream, packet, kp->key, kp->data);
                }
            } else if (smatch(kp->key, ":path")) {
                if (smatch(kp->data, "/")) {
                    encodeInt(packet, httpSetPrefix(7), 7, HTTP2_PATH_ROOT);
                } else if (smatch(kp->data, "/index.html")) {
                    encodeInt(packet, httpSetPrefix(7), 7, HTTP2_PATH_INDEX);
                } else {
                    encodeHeader(stream, packet, kp->key, kp->data);
                }
            } else {
                encodeHeader(stream, packet, kp->key, kp->data);
            }
        }
    }
    for (ITERATE_KEYS(tx->headers, kp)) {
        if (kp->key[0] != ':') {
            encodeHeader(stream, packet, kp->key, kp->data);
        }
    }
    tx->flags |= HTTP_TX_HEADERS_CREATED;
}


/*
    Define the pseudo headers for status, method, scheme and authority.
 */
static void definePseudoHeaders(HttpStream *stream, HttpPacket *packet)
{
    HttpUri     *parsedUri;
    HttpTx      *tx;
    Http        *http;
    cchar       *authority, *path;

    http = stream->http;
    tx = stream->tx;

    if (httpServerStream(stream)) {
        httpAddHeaderString(stream, ":status", itos(tx->status));

    } else {
        authority = stream->rx->hostHeader ? stream->rx->hostHeader : stream->ip;
        httpAddHeaderString(stream, ":method", tx->method);
        httpAddHeaderString(stream, ":scheme", stream->secure ? "https" : "http");
        httpAddHeaderString(stream, ":authority", authority);

        parsedUri = tx->parsedUri;
        if (http->proxyHost && *http->proxyHost) {
            if (parsedUri->query && *parsedUri->query) {
                path = sfmt("http://%s:%d%s?%s", http->proxyHost, http->proxyPort, parsedUri->path, parsedUri->query);
            } else {
                path = sfmt("http://%s:%d%s", http->proxyHost, http->proxyPort, parsedUri->path);
            }
        } else {
            if (parsedUri->query && *parsedUri->query) {
                path = sfmt("%s?%s", parsedUri->path, parsedUri->query);
            } else {
                path = parsedUri->path;
            }
        }
        httpAddHeaderString(stream, ":path", path);
    }
}


/*
    Encode headers using the HPACK and huffman encoding.
 */
static void encodeHeader(HttpStream *stream, HttpPacket *packet, cchar *key, cchar *value)
{
    HttpNet     *net;
    int         index;
    bool        indexedValue;

    net = stream->net;
    stream->tx->headerSize = 0;

    if ((index = httpLookupPackedHeader(net->txHeaders, key, value, &indexedValue)) > 0) {
        if (indexedValue) {
            /* Fully indexed header key and value */
            encodeInt(packet, httpSetPrefix(7), 7, index);
        } else {
            encodeInt(packet, httpSetPrefix(6), 6, index);
            index = httpAddPackedHeader(net->txHeaders, key, value);
            encodeString(packet, value, 0);
        }
    } else {
        index = httpAddPackedHeader(net->txHeaders, key, value);
        encodeInt(packet, httpSetPrefix(6), 6, 0);
        encodeString(packet, key, 1);
        encodeString(packet, value, 0);
#if FUTURE
        //  no indexing
        encodeInt(packet, 0, 4, 0);
        encodeString(packet, key, 1);
        encodeString(packet, value, 0);
#endif
    }
}


/*
    Decode a HPACK encoded integer
 */
static int decodeInt(HttpPacket *packet, uint bits)
{
    MprBuf      *buf;
    uchar       *bp, *end, *start;
    uint        mask, shift, value;
    int         done;

    if (bits < 0 || bits > 8 || !packet || httpGetPacketLength(packet) == 0) {
        return MPR_ERR_BAD_STATE;
    }
    buf = packet->content;
    bp = start = (uchar*) mprGetBufStart(buf);
    end = (uchar*) mprGetBufEnd(buf);

    mask = httpGetPrefixMask(bits);
    value = *bp++ & mask;
    if (value == mask) {
        value = 0;
        shift = 0;
        do {
            if (bp >= end) {
                return MPR_ERR_BAD_STATE;
            }
            done = (*bp & 0x80) != 0x80;
            value += (*bp++ & 0x7f) << shift;
            shift += 7;
        } while (!done);
        value += mask;
    }
    mprAdjustBufStart(buf, bp - start);
    return value;
}


/*
    Encode an integer using HPACK.
 */
static void encodeInt(HttpPacket *packet, uint flags, uint bits, uint value)
{
    MprBuf      *buf;
    uint        mask;

    buf = packet->content;
    mask = (1 << bits) - 1;

    if (value < mask) {
        mprPutCharToBuf(buf, flags | value);
    } else {
        mprPutCharToBuf(buf, flags | mask);
        value -= mask;
        while (value >= 128) {
            mprPutCharToBuf(buf, value % 128 + 128);
            value /= 128;
        }
        mprPutCharToBuf(buf, (uchar) value);
    }
}


/*
    Encode a string using HPACK.
 */
static void encodeString(HttpPacket *packet, cchar *src, uint lower)
{
    MprBuf      *buf;
    cchar       *cp;
    char        *dp, *encoded;
    ssize       len, hlen, extra;

    buf = packet->content;
    len = slen(src);

    /*
        Encode the string in the buffer. Allow some extra space incase the huff encoding is bigger then src and
        some room after the end of the buffer for an encoded integer length.
     */
    extra = 16;
    if (mprGetBufSpace(buf) < (len + extra)) {
        mprGrowBuf(buf, (len + extra) - mprGetBufSpace(buf));
    }
    /*
        Leave room at the end of the buffer for an encoded integer.
     */
    encoded = mprGetBufEnd(buf) + (extra / 2);
    hlen = httpHuffEncode(src, slen(src), encoded, lower);
    assert((encoded + hlen) < buf->endbuf);
    assert(hlen < len);

    if (hlen > 0) {
        encodeInt(packet, HTTP2_ENCODE_HUFF, 7, (uint) hlen);
        memmove(mprGetBufEnd(buf), encoded, hlen);
        mprAdjustBufEnd(buf, hlen);
    } else {
        encodeInt(packet, 0, 7, (uint) len);
        if (lower) {
            for (cp = src, dp = mprGetBufEnd(buf); cp < &src[len]; cp++) {
                *dp++ = tolower(*cp);
            }
            assert(dp < buf->endbuf);
        } else {
            memmove(mprGetBufEnd(buf), src, len);
        }
        mprAdjustBufEnd(buf, len);
    }
    assert(buf->end < buf->endbuf);
}


/*
    Define a frame in the given packet. If null, allocate a packet.
 */
static HttpPacket *defineFrame(HttpQueue *q, HttpPacket *packet, int type, uchar flags, int stream)
{
    HttpNet     *net;
    MprBuf      *buf;
    ssize       length;
    cchar       *typeStr;

    net = q->net;
    if (!packet) {
        packet = httpCreatePacket(0);
    }
    packet->type = type;
    if ((buf = packet->prefix) == 0) {
        buf = packet->prefix = mprCreateBuf(HTTP2_FRAME_OVERHEAD, HTTP2_FRAME_OVERHEAD);
    }
    length = httpGetPacketLength(packet);
    assert(length <= HTTP2_MAX_FRAME_SIZE);

    /*
        Not yet supporting priority or weight
     */
    mprPutUint32ToBuf(buf, (((uint32) length) << 8 | type));
    mprPutCharToBuf(buf, flags);
    mprPutUint32ToBuf(buf, stream);

    typeStr = (type < HTTP2_MAX_FRAME) ? packetTypes[type] : "unknown";
    httpLog(net->trace, "tx.http2", "packet", "frame:%s, flags:%x, stream:%d, length:%zd,", typeStr, flags, stream, length);
    return packet;
}


/*
    Send a HTTP/2 packet downstream to the network
 */
static void sendFrame(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;

    net = q->net;
    if (packet && !net->sentGoaway && !net->eof && !net->error) {
        httpPutPacket(q->net->socketq, packet);
    }
}



/*
    Get or create a stream connection
 */
static HttpStream *getStream(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;
    HttpStream  *stream;
    HttpRx      *rx;
    HttpFrame   *frame;

    net = q->net;
    frame = packet->data;
    stream = frame->stream;
    frame = packet->data;

    if (!stream && httpIsServer(net)) {
        if (net->sentGoaway) {
            /*
                Ignore new streams as the network is going away. Don't send a reset, just ignore.
                sendReset(q, stream, HTTP2_REFUSED_STREAM, "Network is going away");
             */
            return 0;
        }
        if ((stream = httpCreateStream(net, 1)) == 0) {
            /* Memory error - centrally reported */
            return 0;
        }
        /*
            HTTP/2 does not use Content-Length or chunking. End of stream is detected by a frame with HTTP2_END_STREAM_FLAG
         */
        stream->rx->remainingContent = HTTP_UNLIMITED;
        stream->streamID = frame->streamID;
        frame->stream = stream;

        /*
            Servers create a new connection stream
         */
        if (mprGetListLength(net->streams) >= net->limits->streamsMax) {
            sendReset(q, stream, HTTP2_REFUSED_STREAM, "Too many streams for connection: %s %d/%d", net->ip,
                (int) mprGetListLength(net->streams), net->limits->streamsMax);
            return 0;
        }
    }
    if (frame->depend == frame->streamID) {
        sendReset(q, stream, HTTP2_PROTOCOL_ERROR, "Bad stream dependency");
        return 0;
    }
    if (frame->type == HTTP2_CONT_FRAME && (!stream->rx || !stream->rx->headerPacket)) {
        sendGoAway(q, HTTP2_PROTOCOL_ERROR, "Unexpected continue frame before headers frame");
        return 0;
    }
    rx = stream->rx;
    if (rx->headerPacket) {
        httpJoinPacket(rx->headerPacket, packet);
    } else {
        rx->headerPacket = packet;
    }
    packet->stream = stream;

    if (httpGetPacketLength(rx->headerPacket) > stream->limits->headerSize) {
        sendReset(q, stream, HTTP2_REFUSED_STREAM, "Header too big, length %ld, limit %ld",
            httpGetPacketLength(rx->headerPacket), stream->limits->headerSize);
        return 0;
    }
    return stream;
}


/*
    Find a HttpStream stream using the HTTP/2 stream ID
 */
static HttpStream *findStream(HttpNet *net, int streamID)
{
    HttpStream  *stream;
    int         next;

    for (ITERATE_ITEMS(net->streams, stream, next)) {
        if (stream->streamID == streamID) {
            stream->lastActivity = net->lastActivity;
            return stream;
        }
    }
    return 0;
}


/*
    Garbage collector callback.
 */
static void manageFrame(HttpFrame *frame, int flags)
{
    assert(frame);

    if (flags & MPR_MANAGE_MARK) {
        mprMark(frame->stream);
    }
}


static void restartSuspendedStreams(HttpNet *net) {
    HttpStream  *stream;
    HttpQueue   *q;
    int         next;

    for (ITERATE_ITEMS(net->streams, stream, next)) {
        q = stream->outputq;
        if (q->count && q->window > 0 && (q->flags & HTTP_QUEUE_SUSPENDED)) {
            httpResumeQueue(q, 0);
        }
    }
}


static void invalidState(HttpNet *net, HttpStream *stream, int event)
{
    cchar       *stateStr;

    stateStr = (stream->h2State < 0) ? "Error" : States[stream->h2State];
    sendGoAway(net->socketq, HTTP2_PROTOCOL_ERROR, "Invalid event on stream %d, \"%s\" (%d) in stream state \"%s\" (%d)",
        stream->streamID, Events[event], event, stateStr, stream->h2State);
}


/*
    Shutdown a network. This is not necessarily an error. Peer should open a new network.
    Continue processing current streams, but stop processing any new streams.
 */
static void sendGoAway(HttpQueue *q, int status, cchar *fmt, ...)
{
    HttpNet     *net;
    HttpPacket  *packet;
    HttpStream  *stream;
    MprBuf      *buf;
    va_list     ap;
    cchar       *msg;
    int         next;

    net = q->net;
    if (net->sentGoaway) {
        return;
    }
    if ((packet = httpCreatePacket(HTTP2_GOAWAY_SIZE)) == 0) {
        return;
    }
    va_start(ap, fmt);
    net->errorMsg = msg = sfmtv(fmt, ap);
    httpLog(net->trace, "tx.http2", "error", "Send network goAway, lastStream:%d, status:%d, msg:%s", net->lastStreamID, status, msg);
    va_end(ap);

    buf = packet->content;
    mprPutUint32ToBuf(buf, net->lastStreamID);
    mprPutUint32ToBuf(buf, status);
    if (msg && *msg) {
        mprPutStringToBuf(buf, msg);
    }
    sendFrame(q, defineFrame(q, packet, HTTP2_GOAWAY_FRAME, 0, 0));

    for (ITERATE_ITEMS(q->net->streams, stream, next)) {
        if (stream->streamID > net->lastStreamID) {
            resetStream(stream, "Stream terminated", HTTP2_REFUSED_STREAM);
            setState(stream, E2_GOAWAY);
        }
    }
    net->parsingHeaders = 0;
    net->sentGoaway = 1;
}


static int setState(HttpStream *stream, int event)
{
    HttpNet     *net;
    cchar       *stateStr, *type;
    int         state;

    net = stream->net;
    if (event < 0 || event >= E2_MAX || stream->h2State < 0) {
        return H2_ERR;
    }
    state = StateMatrix[event][stream->h2State];

    type = (state == H2_ERR) ? "error" : "packet";

    if (state != H2_SAME) {
        stateStr = (state < 0) ? "Error" : States[state];
        httpLog(net->trace, "rx.http2", type, "msg:State change for stream %d from \"%s\" (%d) to \"%s\" (%d) via event \"%s\" (%d)",
            stream->streamID, States[stream->h2State], stream->h2State, stateStr, state, Events[event], event);
        stream->h2State = state;
    }
    return state;
}

#endif /* CONFIG_HTTP_HTTP2 */




