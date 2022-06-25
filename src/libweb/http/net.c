#include "libhttp.h"

/*
    net.c -- Network I/O.

 */

/********************************* Includes ***********************************/



/***************************** Forward Declarations ***************************/

static void manageNet(HttpNet *net, int flags);
static void netTimeout(HttpNet *net, MprEvent *mprEvent);
static void secureNet(HttpNet *net, MprSsl *ssl, cchar *peerName);

#if CONFIG_HTTP_HTTP2
static HttpHeaderTable *createHeaderTable(int maxSize);
static void manageHeaderTable(HttpHeaderTable *table, int flags);
#endif

/*********************************** Code *************************************/

PUBLIC HttpNet *httpCreateNet(MprDispatcher *dispatcher, HttpEndpoint *endpoint, int protocol, int flags)
{
    Http        *http;
    HttpNet     *net;
    HttpHost    *host;
    HttpRoute   *route;
    int         level;

    http = HTTP;

    if ((net = mprAllocObj(HttpNet, manageNet)) == 0) {
        return 0;
    }
    net->http = http;
    net->callback = http->netCallback;
    net->endpoint = endpoint;
    net->lastActivity = http->now = mprGetTicks();
    net->ioCallback = httpIOEvent;
    net->protocol = -1;

    if (endpoint) {
        net->async = endpoint->async;
        net->endpoint = endpoint;
        net->autoDestroy = 1;
        host = mprGetFirstItem(endpoint->hosts);
        if (host && (route = host->defaultRoute) != 0) {
            net->trace = route->trace;
            net->limits = route->limits;
        } else {
            net->limits = http->serverLimits;
            net->trace = http->trace;
        }
    } else {
        net->limits = http->clientLimits;
        net->trace = http->trace;
        net->nextStreamID = 1;
    }

    level = PTOI(mprLookupKey(net->trace->events, "packet"));
    net->tracing = (net->trace->level >= level) ? 1 : 0;

    net->port = -1;
    net->async = (flags & HTTP_NET_ASYNC) ? 1 : 0;

    net->socketq = httpCreateQueue(net, NULL, http->netConnector, HTTP_QUEUE_TX, NULL);
    net->socketq->name = sclone("socket-tx");

#if CONFIG_HTTP_HTTP2
{
    /*
        The socket queue will typically send and accept packets of HTTP2_MIN_FRAME_SIZE plus the frame size overhead.
        Set the max to fit four packets. Note that HTTP/2 flow control happens on the http filters and not on the socketq.
        Other queues created in netConnector after the protocol is known.
     */
    ssize packetSize = max(HTTP2_MIN_FRAME_SIZE + HTTP2_FRAME_OVERHEAD, net->limits->packetSize);
    httpSetQueueLimits(net->socketq, net->limits, packetSize, -1, -1, -1);
    net->rxHeaders = createHeaderTable(HTTP2_TABLE_SIZE);
    net->txHeaders = createHeaderTable(HTTP2_TABLE_SIZE);
    net->http2 = HTTP->http2;
    net->window = HTTP2_MIN_WINDOW;
}
#endif

    /*
        Create queue of queues that require servicing
     */
    net->serviceq = httpCreateQueueHead(net, NULL, "serviceq", 0);
    httpInitSchedulerQueue(net->serviceq);

    if (dispatcher) {
        net->dispatcher = dispatcher;
    } else if (endpoint) {
        net->dispatcher = endpoint->dispatcher;
    } else {
        net->dispatcher = mprGetDispatcher();
    }
    net->streams = mprCreateList(0, 0);

    net->inputq = httpCreateQueueHead(net, NULL, "http", HTTP_QUEUE_RX);
    net->outputq = httpCreateQueueHead(net, NULL, "http", HTTP_QUEUE_TX);
    httpPairQueues(net->inputq, net->outputq);
    httpAppendQueue(net->socketq, net->outputq);

    if (protocol >= 0) {
        httpSetNetProtocol(net, protocol);
    }
    lock(http);
    net->seqno = ++http->totalConnections;
    unlock(http);
    httpAddNet(net);
    return net;
}


/*
    Destroy a network. This removes the network from the list of networks.
 */
PUBLIC void httpDestroyNet(HttpNet *net)
{
    HttpStream  *stream;
    int         next;

    if (net->callback) {
        net->callback(net, HTTP_NET_DESTROY);
    }
    if (!net->destroyed && !net->borrowed) {
        if (httpIsServer(net)) {
            for (ITERATE_ITEMS(net->streams, stream, next)) {
                mprRemoveItem(net->streams, stream);
                if (HTTP_STATE_BEGIN < stream->state && stream->state < HTTP_STATE_COMPLETE && !stream->destroyed) {
                    httpSetState(stream, HTTP_STATE_COMPLETE);
                }
                httpDestroyStream(stream);
                next--;
            }
            if (net->servicing) {
                httpMonitorNetEvent(net, HTTP_COUNTER_ACTIVE_CONNECTIONS, -1);
                net->servicing = 0;
            }
        }
        httpRemoveNet(net);
        if (net->sock) {
            mprCloseSocket(net->sock, 0);
            mprLog("net info", 5, "Close connection for IP %s:%d", net->ip, net->port);
            // Don't zero just incase another thread (in error) uses net->sock
        }
        if (net->dispatcher && !(net->sharedDispatcher) && net->dispatcher->flags & MPR_DISPATCHER_AUTO) {
            assert(net->streams->length == 0);
            mprDestroyDispatcher(net->dispatcher);
            // Don't NULL net->dispatcher just incase another thread (in error) uses net->dispatcher
        }
        net->destroyed = 1;
    }
}


static void manageNet(HttpNet *net, int flags)
{
    assert(net);

    if (flags & MPR_MANAGE_MARK) {
        mprMark(net->address);
        mprMark(net->streams);
        mprMark(net->stream);
        mprMark(net->context);
        mprMark(net->data);
        mprMark(net->dispatcher);
        mprMark(net->endpoint);
        mprMark(net->errorMsg);
        mprMark(net->holdq);
        mprMark(net->http);
        mprMark(net->inputq);
        mprMark(net->ip);
        mprMark(net->limits);
        mprMark(net->newDispatcher);
        mprMark(net->oldDispatcher);
        mprMark(net->outputq);
        mprMark(net->serviceq);
        mprMark(net->sock);
        mprMark(net->socketq);
        mprMark(net->trace);
        mprMark(net->timeoutEvent);
        mprMark(net->workerEvent);

#if CONFIG_HTTP_HTTP2
        mprMark(net->frame);
        mprMark(net->rxHeaders);
        mprMark(net->txHeaders);
#endif
#if DEPRECATED
        mprMark(net->pool);
        mprMark(net->ejs);
#endif
    }
}


PUBLIC void httpBindSocket(HttpNet *net, MprSocket *sock)
{
    assert(net);
    if (sock) {
        sock->data = net;
        net->sock = sock;
        net->port = sock->port;
        net->ip = sclone(sock->ip);
    }
}


/*
    Client connect the network to a new peer.
    Existing streams are closed when the socket is closed.
 */
PUBLIC int httpConnectNet(HttpNet *net, cchar *ip, int port, MprSsl *ssl)
{
    MprSocket   *sp;

    assert(net);

    if (net->sock) {
        mprCloseSocket(net->sock, 0);
        net->sock = 0;
    }
    if ((sp = mprCreateSocket()) == 0) {
        mprLog("net error", 0, "Cannot create socket");
        return MPR_ERR_CANT_ALLOCATE;
    }
    net->error = 0;
    if (mprConnectSocket(sp, ip, port, MPR_SOCKET_REUSE_PORT) < 0) {
        return MPR_ERR_CANT_CONNECT;
    }
    net->sock = sp;
    net->ip = sclone(ip);
    net->port = port;

    if (ssl) {
        secureNet(net, ssl, ip);
    }
    httpLog(net->trace, "tx.net.peer", "context", "peer:%s:%d", net->ip, net->port);
    if (net->callback) {
        (net->callback)(net, HTTP_NET_CONNECT);
    }
    return 0;
}


static void secureNet(HttpNet *net, MprSsl *ssl, cchar *peerName)
{
#if CONFIG_COM_SSL
    MprSocket   *sock;

    sock = net->sock;
    if (mprUpgradeSocket(sock, ssl, peerName) < 0) {
        httpNetError(net, "Cannot perform SSL upgrade. %s", sock->errorMsg);

    } else if (sock->peerCert) {
        httpLog(net->trace, "tx.net.ssl", "context",
            "msg:Connection secured with peer certificate, " \
            "secure:true, cipher:%s, peerName:%s, subject:%s, issuer:'%s'",
            sock->cipher, sock->peerName, sock->peerCert, sock->peerCertIssuer);
    }
#endif
}


PUBLIC void httpSetNetProtocol(HttpNet *net, int protocol)
{
    Http        *http;
    HttpStage   *stage;

    http = net->http;
    protocol = net->protocol = protocol;

#if CONFIG_HTTP_HTTP2
    stage = protocol < 2 ? http->http1Filter : http->http2Filter;
    /*
        The input queue window is defined by the network limits value. Default of HTTP2_MIN_WINDOW but revised by config.
     */
    httpSetQueueLimits(net->inputq, net->limits, HTTP2_MIN_FRAME_SIZE, -1, -1, -1);
    /*
        The packetSize and window size will always be revised in Http2:parseSettingsFrame
     */
    httpSetQueueLimits(net->outputq, net->limits, HTTP2_MIN_FRAME_SIZE, -1, -1, -1);
#else
    stage = http->http1Filter;
#endif

    httpAssignQueueCallbacks(net->inputq, stage, HTTP_QUEUE_RX);
    httpAssignQueueCallbacks(net->outputq, stage, HTTP_QUEUE_TX);
    net->inputq->name = stage->name;
    net->outputq->name = stage->name;
}


#if CONFIG_HTTP_HTTP2
static HttpHeaderTable *createHeaderTable(int maxsize)
{
    HttpHeaderTable     *table;

    table = mprAllocObj(HttpHeaderTable, manageHeaderTable);
    table->list = mprCreateList(256, 0);
    table->size = 0;
    table->max = maxsize;
    return table;
}


static void manageHeaderTable(HttpHeaderTable *table, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(table->list);
    }
}
#endif


PUBLIC void httpAddStream(HttpNet *net, HttpStream *stream)
{
    assert(!(net->dispatcher->flags & MPR_DISPATCHER_DESTROYED));
    if (mprLookupItem(net->streams, stream) < 0) {
        mprAddItem(net->streams, stream);
    }
    stream->net = net;
    if (net->protocol < 2) {
        net->stream = stream;
    }
}


PUBLIC void httpRemoveStream(HttpNet *net, HttpStream *stream)
{
    mprRemoveItem(net->streams, stream);
    if (net->protocol < 2) {
        net->stream = NULL;
    }
}


PUBLIC void httpNetTimeout(HttpNet *net)
{
    if (!net->timeoutEvent && !net->destroyed) {
        /*
            Will run on the HttpNet dispatcher unless shutting down and it is destroyed already
         */
        if (net->dispatcher && !(net->dispatcher->flags & MPR_DISPATCHER_DESTROYED)) {
            net->timeoutEvent = mprCreateLocalEvent(net->dispatcher, "netTimeout", 0, netTimeout, net, 0);
        }
    }
}


static void netTimeout(HttpNet *net, MprEvent *mprEvent)
{
    if (net->destroyed) {
        return;
    }
    /* This will trigger an I/O event which will then destroy the network */
    mprDisconnectSocket(net->sock);
    httpEnableNetEvents(net);
}


PUBLIC bool httpGetAsync(HttpNet *net)
{
    return net->async;
}


PUBLIC void httpSetAsync(HttpNet *net, bool async)
{
    net->async = async;
}


PUBLIC void httpSetIOCallback(HttpNet *net, HttpIOCallback fn)
{
    net->ioCallback = fn;
}


PUBLIC void httpSetNetCallback(HttpNet *net, HttpNetCallback callback)
{
    if (net) {
        net->callback = callback;
    } else if (HTTP) {
        HTTP->netCallback = callback;
    }
}


PUBLIC void httpSetNetContext(HttpNet *net, void *context)
{
    net->context = context;
}


#if DEPRECATED
/*
    Used by ejs
 */
PUBLIC void httpUseWorker(HttpNet *net, MprDispatcher *dispatcher, MprEvent *event)
{
    lock(net->http);
    net->oldDispatcher = net->dispatcher;
    net->dispatcher = dispatcher;
    net->worker = 1;
    assert(!net->workerEvent);
    net->workerEvent = event;
    unlock(net->http);
}


PUBLIC void httpUsePrimary(HttpNet *net)
{
    lock(net->http);
    assert(net->worker);
    assert(net->oldDispatcher && net->dispatcher != net->oldDispatcher);
    net->dispatcher = net->oldDispatcher;
    net->oldDispatcher = 0;
    net->worker = 0;
    unlock(net->http);
}
#endif


#if DEPRECATED
PUBLIC void httpBorrowNet(HttpNet *net)
{
    assert(!net->borrowed);
    if (!net->borrowed) {
        mprAddRoot(net);
        net->borrowed = 1;
    }
}


PUBLIC void httpReturnNet(HttpNet *net)
{
    assert(net->borrowed);
    if (net->borrowed) {
        net->borrowed = 0;
        mprRemoveRoot(net);
        httpEnableNetEvents(net);
    }
}
#endif


#if DEPRECATE || 1
/*
    Steal the socket object from a network. This disconnects the socket from management by the Http service.
    It is the callers responsibility to call mprCloseSocket when required.
    Harder than it looks. We clone the socket, steal the socket handle and set the socket handle to invalid.
    This preserves the HttpNet.sock object for the network and returns a new MprSocket for the caller.
 */
PUBLIC MprSocket *httpStealSocket(HttpNet *net)
{
    MprSocket   *sock;
    HttpStream  *stream;
    int         next;

    assert(net->sock);
    assert(!net->destroyed);

    if (!net->destroyed && !net->borrowed) {
        lock(net->http);
        for (ITERATE_ITEMS(net->streams, stream, next)) {
            httpDestroyStream(stream);
            next--;
        }
        if (net->servicing) {
            httpMonitorNetEvent(net, HTTP_COUNTER_ACTIVE_CONNECTIONS, -1);
            net->servicing = 0;
        }
        sock = mprCloneSocket(net->sock);
        (void) mprStealSocketHandle(net->sock);
        mprRemoveSocketHandler(net->sock);
        httpRemoveNet(net);

        net->endpoint = 0;
        net->async = 0;
        unlock(net->http);
        return sock;
    }
    return 0;
}
#endif


/*
    Steal the O/S socket handle. This disconnects the socket handle from management by the network.
    It is the callers responsibility to call close() on the Socket when required.
    Note: this does not change the state of the network.
 */
PUBLIC Socket httpStealSocketHandle(HttpNet *net)
{
    return mprStealSocketHandle(net->sock);
}


PUBLIC cchar *httpGetProtocol(HttpNet *net)
{
    if (net->protocol == 0) {
        return "HTTP/1.0";
    } else if (net->protocol >= 2) {
        return "HTTP/2.0";
    } else {
        return "HTTP/1.1";
    }
}


PUBLIC void httpSetNetEof(HttpNet *net)
{
    net->eof = 1;
    if (net->callback) {
        (net->callback)(net, HTTP_NET_EOF);
    }
}


PUBLIC void httpSetNetError(HttpNet *net)
{
    net->eof = 1;
    net->error = 1;
    if (net->callback) {
        (net->callback)(net, HTTP_NET_ERROR);
    }
}




