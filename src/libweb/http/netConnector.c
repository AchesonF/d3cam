#include "libhttp.h"

/*
    netConnector.c -- General network connector.

    The Network connector handles I/O from upstream handlers and filters. It uses vectored writes to
    aggregate output packets into fewer actual I/O requests to the O/S.

 */

/********************************* Includes ***********************************/



/**************************** Forward Declarations ****************************/

static void addPacketForNet(HttpQueue *q, HttpPacket *packet);
static void addToNetVector(HttpNet *net, char *ptr, ssize bytes);
static void adjustNetVec(HttpNet *net, ssize written);
static MprOff buildNetVec(HttpQueue *q);
static void closeStreams(HttpNet *net);
static void freeNetPackets(HttpQueue *q, ssize written);
static HttpPacket *getPacket(HttpNet *net, ssize *size);
static void netOutgoing(HttpQueue *q, HttpPacket *packet);
static void netOutgoingService(HttpQueue *q);
static HttpPacket *readPacket(HttpNet *net);
static void resumeEvents(HttpNet *net, MprEvent *event);
static int sleuthProtocol(HttpNet *net, HttpPacket *packet);

/*********************************** Code *************************************/
/*
    Initialize the net connector
 */
PUBLIC int httpOpenNetConnector()
{
    HttpStage     *stage;

    if ((stage = httpCreateConnector("netConnector", NULL)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    stage->outgoing = netOutgoing;
    stage->outgoingService = netOutgoingService;
    HTTP->netConnector = stage;
    return 0;
}


/*
    Accept a new client connection on a new socket. This is invoked from acceptNet in endpoint.c
    and will come arrive on a worker thread with a new dispatcher dedicated to this connection.
 */
PUBLIC HttpNet *httpAccept(HttpEndpoint *endpoint, MprEvent *event)
{
    Http        *http;
    HttpNet     *net;
    HttpLimits  *limits;
    MprSocket   *sock;
    int64       value;
#if CONFIG_HTTP_DEFENSE
    HttpAddress *address;
#endif

    assert(event);
    assert(event->dispatcher);
    assert(endpoint);

    if (mprShouldDenyNewRequests()) {
        return 0;
    }
    sock = event->sock;

    if ((net = httpCreateNet(event->dispatcher, endpoint, -1, HTTP_NET_ASYNC)) == 0) {
        mprCloseSocket(sock, 0);
        return 0;
    }
    httpBindSocket(net, sock);
    limits = net->limits;
    http = net->http;

    mprLog("net info", 5, "Connection received for IP %s:%d", net->ip, net->port);

#if KEEP
    //  Useful for debugging and simulating multiple clients
    net->ip = mprGetRandomString(16);
#endif

#if CONFIG_HTTP_DEFENSE
    if ((address = httpMonitorAddress(net, 0)) == 0) {
        mprLog("net info", 1, "Connection denied for IP %s. Too many concurrent clients, active: %d, max:%d",
            net->ip, mprGetHashLength(http->addresses), net->limits->clientMax);
        //  Increment here because DestroyNet always will decrement
        httpMonitorNetEvent(net, HTTP_COUNTER_ACTIVE_CONNECTIONS, 1);
        httpDestroyNet(net);
        return 0;
    }
#endif

    net->servicing = 1;
    if ((value = httpMonitorNetEvent(net, HTTP_COUNTER_ACTIVE_CONNECTIONS, 1)) > limits->connectionsPerClientMax) {
        mprLog("net info", 1, "Connection denied for IP %s. Too many concurrent connections for client, active: %d, max: %d",
            net->ip, (int) (value - 1), limits->connectionsPerClientMax);
        httpDestroyNet(net);
        return 0;
    }
    if (mprGetListLength(http->networks) > limits->connectionsMax) {
        mprLog("net info", 1, "Connection denied for IP %s. Too many concurrent connections for the server, active: %d, max: %d",
            net->ip, mprGetListLength(http->networks), limits->connectionsMax);
        httpDestroyNet(net);
        return 0;
    }

#if CONFIG_HTTP_DEFENSE
    address = net->address;
    if (address && address->banUntil) {
        if (address->banUntil < net->http->now) {
            mprLog("net info", 1, "Remove ban for client IP %s", net->ip);
            address->banUntil = 0;
        } else {
            mprLog("net info", 1, "Network connection refused for client IP %s, client banned: %s", net->ip,
                address->banMsg ? address->banMsg : "");
            httpDestroyNet(net);
            return 0;
        }
    }
#endif

#if CONFIG_COM_SSL
    if (endpoint->ssl) {
        if (mprUpgradeSocket(sock, endpoint->ssl, 0) < 0) {
            httpMonitorNetEvent(net, HTTP_COUNTER_SSL_ERRORS, 1);
            mprLog("net error", 0, "Cannot upgrade socket, %s", sock->errorMsg);
            httpDestroyNet(net);
            return 0;
        }
    }
#endif

    event->mask = MPR_READABLE;
    event->timestamp = net->http->now;
    if (net->http->netCallback) {
        net->http->netCallback(net, HTTP_NET_ACCEPT);
    }
    (net->ioCallback)(net, event);
    return net;
}


PUBLIC bool httpReadIO(HttpNet *net)
{
    HttpPacket  *packet;
    HttpStream  *stream;

    stream = net->stream;
    if ((packet = readPacket(net)) != NULL) {
        if (stream) {
            if (stream->state >= HTTP_STATE_COMPLETE) {
                if (stream->keepAliveCount <= 0) {
                    //  Client should have closed first
                    mprLog("net info", 5, "Client sent request after keep-alive expired");
                    net->error = 1;
                    return 0;
                }
                if (httpServerStream(stream)) {
                    httpResetServerStream(stream);
                }
            }
        }
        if (net->protocol < 0 && strstr(packet->content->start, "\r\n") != NULL) {
            if (sleuthProtocol(net, packet) < 0) {
                net->error = 1;
                return 0;
            }
        }
        httpPutPacket(net->inputq, packet);
        return 1;
    }
    return 0;
}


/*
    Handle IO on the network. Initially the dispatcher will be set to the server->dispatcher and the first
    I/O event will be handled on the server thread (or main thread). A request handler may create a new
    net->dispatcher and transfer execution to a worker thread if required.
 */
PUBLIC void httpIOEvent(HttpNet *net, MprEvent *event)
{
    if (net->destroyed) {
        return;
    }
    net->lastActivity = net->http->now;

    if (event->mask & MPR_WRITABLE && (net->socketq->count + net->ioCount) > 0) {
        httpResumeQueue(net->socketq, 1);
    }
    if (event->mask & MPR_READABLE) {
        if (!mprIsSocketEof(net->sock)) {
            httpReadIO(net);
        } else {
            httpSetNetEof(net);
        }
    }

    /*
        Process the packet. This will propagate the packet through configured queues for the net.
    */
    httpServiceNet(net);

    if (net->callback) {
        (net->callback)(net, HTTP_NET_IO);
    }
    if (mprNeedYield()) {
        mprYield(0);
    }
}


/*
    Service the primary network. This will propagate the packet through configured queues for the net.
    Also service associated networks (with same dispatcher) incase packets transferred between networks (proxy).
*/
PUBLIC void httpServiceNet(HttpNet *net)
{
    net->active = 1;
    httpServiceNetQueues(net, HTTP_BLOCK);

    if (net->error || net->eof || (net->sentGoaway && !net->socketq->first)) {
        closeStreams(net);
        if (net->autoDestroy) {
            httpDestroyNet(net);
        }
    } else if (net->async && !net->delay) {
        httpEnableNetEvents(net);
    }
    net->active = 0;
}


/*
    Read data from the peer. This will use an existing packet on the inputq or allocate a new packet if required to
    hold the data. Socket error messages are stored in net->errorMsg.
 */
static HttpPacket *readPacket(HttpNet *net)
{
    HttpPacket  *packet;
    ssize       size, lastRead;

    if ((packet = getPacket(net, &size)) != 0) {
        lastRead = mprReadSocket(net->sock, mprGetBufEnd(packet->content), size);
        if (mprIsSocketEof(net->sock)) {
            httpSetNetEof(net);
        }
#if CONFIG_COM_SSL
        if (net->sock->secured && !net->secure && net->sock->cipher) {
            MprSocket   *sock;
            net->secure = 1;
            sock = net->sock;
            if (sock->peerCert) {
                httpLog(net->trace, "rx.net.ssl", "context",
                    "msg:Connection secured, cipher:%s, peerName:%s, subject:%s, issuer:%s, session:%s",
                    sock->cipher, sock->peerName, sock->peerCert, sock->peerCertIssuer, sock->session);
            } else {
                httpLog(net->trace, "rx.net.ssl", "context", "msg:Connection secured, cipher:%s, session:%s", sock->cipher, sock->session);
            }
            if (mprGetLogLevel() >= 5) {
                mprLog("info http ssl", 6, "SSL State: %s", mprGetSocketState(sock));
            }
        }
#endif
        if (lastRead > 0) {
            if (net->tracing) {
                httpLogRxPacket(net, packet->content->end, lastRead);
            }
            mprAdjustBufEnd(packet->content, lastRead);
            mprAddNullToBuf(packet->content);
            return packet;
        }
        if (lastRead < 0 && net->eof) {
            if (net->sock->flags & (MPR_SOCKET_ERROR | MPR_SOCKET_CERT_ERROR)) {
                httpLog(net->trace, "rx.net", "error", "msg:Connection error %s", net->sock->errorMsg);
            }
            return 0;
        }
    }
    return 0;
}


static void netOutgoing(HttpQueue *q, HttpPacket *packet)
{
    assert(q == q->net->socketq);

    if (q->net->socketq) {
        httpPutForService(q->net->socketq, packet, HTTP_SCHEDULE_QUEUE);
    }
}


static void netOutgoingService(HttpQueue *q)
{
    HttpNet     *net;
    ssize       written;
    int         errCode;

    net = q->net;
    net->writeBlocked = 0;
    written = 0;

    while (q->first || net->ioIndex) {
        if (net->ioIndex == 0 && buildNetVec(q) <= (MprOff) 0) {
            freeNetPackets(q, 0);
            break;
        }
        if (net->ioFile) {
            written = mprSendFileToSocket(net->sock, net->ioFile, net->ioPos, net->ioCount, net->iovec, net->ioIndex, NULL, 0);
        } else {
            written = mprWriteSocketVector(net->sock, net->iovec, net->ioIndex);
        }
        if (written < 0) {
            errCode = mprGetError();
            if (errCode == EAGAIN || errCode == EWOULDBLOCK) {
                /*  Socket full, wait for an I/O event */
                net->writeBlocked = 1;
                break;
            }
            if (errCode == EPROTO && net->secure) {
                httpNetError(net, "Cannot negotiate SSL with server: %s", net->sock->errorMsg);
            } else {
                httpNetError(net, "netConnector: Cannot write. errno %d", errCode);
            }
            freeNetPackets(q, MAXINT);
            httpEnableNetEvents(net);
            break;

        } else if (written > 0) {
            if (net->tracing) {
                httpLogTxPacket(net, written);
            }
            freeNetPackets(q, written);
            adjustNetVec(net, written);

        } else {
            // Socket full or SSL negotiate
            net->writeBlocked = 1;
            break;
        }
    }
    if (net->writeBlocked) {
        if ((q->first || net->ioIndex) && !(net->eventMask & MPR_WRITABLE)) {
            httpEnableNetEvents(net);
        }
    } else if (q->count <= q->low && (net->outputq->flags & HTTP_QUEUE_SUSPENDED)) {
        httpResumeQueue(net->outputq, 0);
    }
}


/*
    Build the IO vector. Return the count of bytes to be written. Return -1 for EOF.
 */
static MprOff buildNetVec(HttpQueue *q)
{
    HttpNet     *net;
    HttpPacket  *packet;

    net = q->net;
    /*
        Examine each packet and accumulate as many packets into the I/O vector as possible. Leave the packets on
        the queue for now, they are removed after the IO is complete for the entire packet. mprWriteSocketVector will
        use O/S vectored writes or aggregate packets into a single write where appropriate.
     */
     net->ioCount = 0;
     for (packet = q->first; packet; packet = packet->next) {
        if (net->ioIndex >= (ME_MAX_IOVEC - 2)) {
            break;
        }
        if (httpGetPacketLength(packet) > 0 || packet->prefix || packet->esize > 0) {
            addPacketForNet(q, packet);
        }
    }
    return net->ioCount;
}



/*
    Add a packet to the io vector. Return the number of bytes added to the vector.
 */
static void addPacketForNet(HttpQueue *q, HttpPacket *packet)
{
    HttpNet     *net;
    HttpStream  *stream;

    net = q->net;
    stream = packet->stream;

    assert(q->count >= 0);
    assert(net->ioIndex < (ME_MAX_IOVEC - 2));

    if (packet->prefix && mprGetBufLength(packet->prefix) > 0) {
        addToNetVector(net, mprGetBufStart(packet->prefix), mprGetBufLength(packet->prefix));
    }
    if (packet->content && mprGetBufLength(packet->content) > 0) {
        addToNetVector(net, mprGetBufStart(packet->content), mprGetBufLength(packet->content));

    } else if (packet->esize > 0) {
        net->ioFile = stream->tx->file;
        net->ioCount += packet->esize;
    }
    if (stream) {
        stream->lastActivity = net->http->now;
    }
}


/*
    Add one entry to the io vector
 */
static void addToNetVector(HttpNet *net, char *ptr, ssize bytes)
{
    assert(bytes > 0);

    net->iovec[net->ioIndex].start = ptr;
    net->iovec[net->ioIndex].len = bytes;
    net->ioCount += bytes;
    net->ioIndex++;
}


static void freeNetPackets(HttpQueue *q, ssize bytes)
{
    HttpNet     *net;
    HttpPacket  *packet;
    HttpStream  *stream;
    ssize       len;

    net = q->net;
    assert(q->count >= 0);
    assert(bytes >= 0);

    while ((packet = q->first) != 0) {
        stream = packet->stream;
        if (packet->flags & HTTP_PACKET_END) {
            if (packet->prefix) {
                len = mprGetBufLength(packet->prefix);
                len = min(len, bytes);
                mprAdjustBufStart(packet->prefix, len);
                bytes -= len;
                // Prefixes don't count in the q->count. No need to adjust
                if (mprGetBufLength(packet->prefix) == 0) {
                    // Ensure the prefix is not resent if all the content is not sent
                    packet->prefix = 0;
                }
            }
            if (stream) {
                httpFinalizeConnector(stream);
            }
        } else if (bytes > 0) {
            if (packet->prefix) {
                len = mprGetBufLength(packet->prefix);
                len = min(len, bytes);
                mprAdjustBufStart(packet->prefix, len);
                bytes -= len;
                // Prefixes don't count in the q->count. No need to adjust
                if (mprGetBufLength(packet->prefix) == 0) {
                    // Ensure the prefix is not resent if all the content is not sent
                    packet->prefix = 0;
                }
            }
            if (packet->content) {
                len = mprGetBufLength(packet->content);
                len = min(len, bytes);
                mprAdjustBufStart(packet->content, len);
                bytes -= len;
                net->bytesWritten += len;
                q->count -= len;
                assert(q->count >= 0);


            } else if (packet->esize > 0) {
                len = min(packet->esize, bytes);
                bytes -= len;
                packet->esize -= len;
                packet->epos += len;
                net->ioPos += len;
                net->bytesWritten += len;
                assert(packet->esize >= 0);
            }
        }
        if ((packet->flags & HTTP_PACKET_END) || (httpGetPacketLength(packet) == 0 && !packet->prefix && packet->esize == 0)) {
            // Done with this packet - consume it
            httpGetPacket(q);
        } else {
            // Packet still has data to be written
            break;
        }
    }
    if (stream) {
        stream->lastActivity = net->http->now;
    }
}


/*
    Clear entries from the IO vector that have actually been transmitted. Support partial writes.
 */
static void adjustNetVec(HttpNet *net, ssize written)
{
    MprIOVec    *iovec;
    ssize       len;
    int         i, j;

    /*
        Cleanup the IO vector
     */
    if (written == net->ioCount) {
        /*
            Entire vector written. Just reset.
         */
        net->ioIndex = 0;
        net->ioCount = 0;
        net->ioPos = 0;
        net->ioFile = 0;

    } else {
        /*
            Partial write of an vector entry. Need to copy down the unwritten vector entries.
         */
        net->ioCount -= written;
        assert(net->ioCount >= 0);
        iovec = net->iovec;
        for (i = 0; i < net->ioIndex; i++) {
            len = iovec[i].len;
            if (written < len) {
                iovec[i].start += written;
                iovec[i].len -= written;
                break;
            } else {
                written -= len;
            }
        }
        /*
            Compact the vector
         */
        for (j = 0; i < net->ioIndex; ) {
            iovec[j++] = iovec[i++];
        }
        net->ioIndex = j;
    }
}


static int sleuthProtocol(HttpNet *net, HttpPacket *packet)
{
    MprBuf      *buf;
    ssize       len;
    cchar       *eol, *start, *ver;
    int         protocol;

    buf = packet->content;
    start = buf->start;
    protocol = 0;

    if ((eol = strstr(start, "\r\n")) == 0) {
        return MPR_ERR_BAD_STATE;
    }
    ver = strstr(start, "HTTP/");
    if (ver == NULL || ver > eol) {
        return MPR_ERR_BAD_STATE;
    }
    ver += 5;
    if (strncmp(ver, "2.", 2) == 0) {
        protocol = 2;
    } else if (strncmp(ver, "1.1", 3) == 0) {
        protocol = 1;
    } else {
        protocol = 0;
    }
    if (protocol == 2) {
        if (!net->http2 || CONFIG_HTTP_HTTP2 == 0) {
            return MPR_ERR_BAD_STATE;
        }
        if ((len = mprGetBufLength(buf)) < (sizeof(HTTP2_PREFACE) - 1)) {
            // Insufficient data
            return 0;
        }
        if (memcmp(buf->start, HTTP2_PREFACE, sizeof(HTTP2_PREFACE) - 1) != 0) {
            protocol = MPR_ERR_BAD_STATE;
        }
        mprAdjustBufStart(buf, strlen(HTTP2_PREFACE));
        httpLog(net->trace, "rx.net", "context", "msg:Detected HTTP/2 preface");
    }
    httpSetNetProtocol(net, protocol);
    return protocol;
}


/*
    Get the packet into which to read data. Return in *size the length of data to attempt to read.
 */
static HttpPacket *getPacket(HttpNet *net, ssize *lenp)
{
    HttpPacket  *packet;
    MprBuf      *buf;
    ssize       size;

    packet = 0;

#if CONFIG_HTTP_HTTP2
    if (net->protocol < 2) {
        size = net->inputq ? net->inputq->packetSize : ME_PACKET_SIZE;
    } else {
        size = (net->inputq ? net->inputq->packetSize : HTTP2_MIN_FRAME_SIZE) + HTTP2_FRAME_OVERHEAD;
    }
#else
    size = net->inputq ? net->inputq->packetSize : ME_PACKET_SIZE;
#endif
    /*
        Must use queued buffer and append if we haven't yet sleuthed the protocol
    */
    if (net->protocol < 0 && net->inputq) {
        //  This will remove the packet from the queue. Should not be any other packets on the queue.
        packet = httpGetPacket(net->inputq);
        assert(net->inputq->first == NULL);
    }
    if (packet) {
        buf = packet->content;
        if (mprGetBufSpace(buf) < size && mprGrowBuf(buf, size) < 0) {
            //  Already reported
            return 0;
        }
    } else {
        packet = httpCreateDataPacket(size);
    }
    *lenp = mprGetBufSpace(packet->content);
    assert(*lenp > 0);
    return packet;
}


static bool netBanned(HttpNet *net)
{
    HttpAddress     *address;

    if ((address = net->address) != 0 && address->delay) {
        if (address->delayUntil > net->http->now) {
            /*
                Defensive counter measure - go slow
             */
            mprCreateLocalEvent(net->dispatcher, "delayConn", net->delay, resumeEvents, net, 0);
            httpLog(net->trace, "monitor.delay.stop", "context", "msg:Suspend I/O, client:%s", net->ip);
            return 1;
        } else {
            address->delay = 0;
            httpLog(net->trace, "monitor.delay.stop", "context", "msg:Resume I/O, client:%s", net->ip);
        }
    }
    return 0;
}


/*
    Defensive countermesasure - resume output after a delay
 */
static void resumeEvents(HttpNet *net, MprEvent *event)
{
    net->delay = 0;
    mprCreateLocalEvent(net->dispatcher, "resumeConn", 0, httpEnableNetEvents, net, 0);
}


/*
    Get the event mask to enable events.
    Important that this work for networks with net->error set.
 */
PUBLIC int httpGetNetEventMask(HttpNet *net)
{
    MprSocket   *sock;
    HttpStream  *stream;
    int         eventMask;

    if ((sock = net->sock) == 0) {
        return 0;
    }
    stream = net->stream;
    eventMask = 0;

    if (!mprSocketHandshaking(sock) && !(net->error || net->eof)) {
        if (httpQueuesNeedService(net) || mprSocketHasBufferedWrite(sock) || (net->socketq->count + net->ioCount) > 0) {
            // Must wait to write until handshaking is complete
            eventMask |= MPR_WRITABLE;
        }
    }
    if (net->protocol >= 2 || !stream || stream->state < HTTP_STATE_READY || stream->state == HTTP_STATE_COMPLETE ||
            mprSocketHasBufferedRead(sock) || mprSocketHandshaking(sock) || net->error) {
        if (! (net->inputq->flags & HTTP_QUEUE_SUSPENDED)) {
            eventMask |= MPR_READABLE;
        }
    }
    return eventMask;
}


/*
    Important that this work for networks with net->error set.
 */
PUBLIC void httpEnableNetEvents(HttpNet *net)
{
    if (mprShouldAbortRequests() || net->destroyed || net->borrowed || netBanned(net)) {
        return;
    }
    httpSetupWaitHandler(net, httpGetNetEventMask(net));
}


PUBLIC void httpSetupWaitHandler(HttpNet *net, int eventMask)
{
    MprSocket   *sp;

    if ((sp = net->sock) == 0) {
        return;
    }
    if (eventMask) {
        if (sp->handler == 0) {
            mprAddSocketHandler(sp, eventMask, net->dispatcher, net->ioCallback, net, 0);
        } else {
            mprSetSocketDispatcher(sp, net->dispatcher);
            mprEnableSocketEvents(sp, eventMask);
        }
        if (sp->flags & (MPR_SOCKET_BUFFERED_READ | MPR_SOCKET_BUFFERED_WRITE)) {
            mprRecallWaitHandler(sp->handler);
        }
    } else if (sp->handler) {
        mprWaitOn(sp->handler, eventMask);
    }
    net->eventMask = eventMask;
}


static void closeStreams(HttpNet *net)
{
    HttpStream  *stream;
    HttpRx      *rx;
    HttpTx      *tx;
    int         next;

    for (ITERATE_ITEMS(net->streams, stream, next)) {
        rx = stream->rx;
        tx = stream->tx;
        httpSetEof(stream);

        if (stream->state > HTTP_STATE_BEGIN) {
            if (stream->state < HTTP_STATE_PARSED) {
                httpError(stream, 0, "Peer closed connection before receiving response");

            } else if (tx) {
                if (!tx->finalizedOutput) {
                    httpError(stream, 0, "Peer closed connection before transmitting full response");

                } else if (!tx->finalizedInput) {
                    httpError(stream, 0, "Peer closed connection before receiving full response");
                }
            }
            if (stream->state < HTTP_STATE_COMPLETE) {
                httpSetState(stream, HTTP_STATE_COMPLETE);
            }
        }
    }
}




