#include "libhttp.h"

/*
    stream.c -- Request / Response Stream module

    Streams are multiplexed on top of HttpNet connections.

 */
/********************************* Includes ***********************************/



/********************************** Locals ************************************/
/*
    Invocation structure for httpCreateEvent
 */
typedef struct HttpInvoke {
    HttpEventProc   callback;
    uint64          seqno;          // Stream sequence number
    void            *data;          //  User data - caller must free if required in callback
    int             hasRun;
} HttpInvoke;

/***************************** Forward Declarations ***************************/

static void prepareStream(HttpStream *stream, MprHash *headers);
static void createInvokeEvent(HttpStream *stream, void *data);
static void manageInvoke(HttpInvoke *event, int flags);
static void manageStream(HttpStream *stream, int flags);
static void resetQueues(HttpStream *stream);

/*********************************** Code *************************************/
/*
    Create a new stream object. These are multiplexed onto network objects.

    Use httpCreateNet() to create a network object.
 */

PUBLIC HttpStream *httpCreateStream(HttpNet *net, bool peerCreated)
{
    Http        *http;
    HttpStream  *stream;
    HttpLimits  *limits;
    HttpHost    *host;
    HttpRoute   *route;

    assert(net);
    http = HTTP;
    if ((stream = mprAllocObj(HttpStream, manageStream)) == 0) {
        return 0;
    }
    stream->http = http;
    stream->port = -1;
    stream->started = http->now;
    stream->lastActivity = http->now;
    stream->net = net;
    stream->endpoint = net->endpoint;
    stream->sock = net->sock;
    stream->port = net->port;
    stream->ip = net->ip;
    stream->secure = net->secure;
    stream->peerCreated = peerCreated;
    if (stream->endpoint) {
        stream->notifier = net->endpoint->notifier;
    }
    if (net->endpoint) {
        host = mprGetFirstItem(net->endpoint->hosts);
        if (host && (route = host->defaultRoute) != 0) {
            stream->limits = route->limits;
            stream->trace = route->trace;
        } else {
            stream->limits = http->serverLimits;
            stream->trace = http->trace;
        }
    } else {
        stream->limits = http->clientLimits;
        stream->trace = http->trace;
    }
    limits = stream->limits;

    stream->keepAliveCount = (net->protocol >= 2) ? 0 : stream->limits->keepAliveMax;
    stream->dispatcher = net->dispatcher;

    resetQueues(stream);

#if CONFIG_HTTP_HTTP2
    stream->h2State = HTTP2_STATE_IDLE;
    if (!peerCreated && ((net->ownStreams >= limits->txStreamsMax) || (net->ownStreams >= limits->streamsMax))) {
        httpNetError(net, "Attempting to create too many streams for network connection: %d/%d/%d", net->ownStreams,
            limits->txStreamsMax, limits->streamsMax);
        return 0;
    }
#endif
    httpSetState(stream, HTTP_STATE_BEGIN);

    lock(http);
    stream->seqno = ++http->totalStreams;
    unlock(http);

    httpAddStream(net, stream);
    if (!peerCreated) {
        net->ownStreams++;
    }
    return stream;
}


/*
    Destroy a stream. This removes the stream from the list of streams.
 */
PUBLIC void httpDestroyStream(HttpStream *stream)
{
    if (!stream->destroyed && !stream->net->borrowed) {
        HTTP_NOTIFY(stream, HTTP_EVENT_DESTROY, 0);
        if (stream->tx) {
            httpClosePipeline(stream);
        }
        if (!stream->peerCreated) {
            stream->net->ownStreams--;
        }
        if (stream->streamID == stream->net->lastStreamID) {
            stream->net->lastStreamID++;
        }
        if (stream->counted) {
            httpMonitorEvent(stream, HTTP_COUNTER_ACTIVE_REQUESTS, -1);
            stream->counted = 0;
        }
        httpRemoveStream(stream->net, stream);
        stream->state = HTTP_STATE_COMPLETE;
        stream->destroyed = 1;
    }
}


static void manageStream(HttpStream *stream, int flags)
{
    assert(stream);

    if (flags & MPR_MANAGE_MARK) {
        mprMark(stream->authType);
        mprMark(stream->authData);
        mprMark(stream->boundary);
        mprMark(stream->context);
        mprMark(stream->data);
        mprMark(stream->dispatcher);
        mprMark(stream->endpoint);
        mprMark(stream->errorMsg);
        mprMark(stream->grid);
        mprMark(stream->headersCallbackArg);
        mprMark(stream->http);
        mprMark(stream->host);
        mprMark(stream->inputq);
        mprMark(stream->ip);
        mprMark(stream->limits);
        mprMark(stream->mark);
        mprMark(stream->net);
        mprMark(stream->outputq);
        mprMark(stream->password);
        mprMark(stream->pool);
        mprMark(stream->protocols);
        mprMark(stream->readq);
        mprMark(stream->record);
        mprMark(stream->reqData);
        mprMark(stream->rx);
        mprMark(stream->rxHead);
        mprMark(stream->sock);
        mprMark(stream->timeoutEvent);
        mprMark(stream->trace);
        mprMark(stream->tx);
        mprMark(stream->txHead);
        mprMark(stream->user);
        mprMark(stream->username);
        mprMark(stream->writeq);
#if DEPRECATED
        mprMark(stream->ejs);
#endif
    }
}


/*
    Prepare for another request for server
 */
PUBLIC void httpResetServerStream(HttpStream *stream)
{
    assert(httpServerStream(stream));
    assert(stream->state == HTTP_STATE_COMPLETE);

    if (stream->net->borrowed) {
        return;
    }
    HTTP_NOTIFY(stream, HTTP_EVENT_DESTROY, 0);

    if (stream->tx) {
        stream->tx->stream = 0;
    }
    if (stream->rx) {
        stream->rx->stream = 0;
    }
    stream->authType = 0;
    stream->username = 0;
    stream->password = 0;
    stream->user = 0;
    stream->authData = 0;
    stream->encoded = 0;

    prepareStream(stream, NULL);
}


PUBLIC void httpResetClientStream(HttpStream *stream, bool keepHeaders)
{
    assert(stream);

    if (stream->net->protocol < 2) {
        if (stream->state > HTTP_STATE_BEGIN && stream->state < HTTP_STATE_COMPLETE &&
                stream->keepAliveCount > 0 && stream->sock && !httpIsEof(stream)) {
            /* Residual data from past request, cannot continue on this socket */
            stream->sock = 0;
        }
    }
    if (stream->tx) {
        stream->tx->stream = 0;
    }
    if (stream->rx) {
        stream->rx->stream = 0;
    }
    prepareStream(stream, (keepHeaders && stream->tx) ? stream->tx->headers: NULL);
}


static void prepareStream(HttpStream *stream, MprHash *headers)
{
    stream->tx = httpCreateTx(stream, headers);
    stream->rx = httpCreateRx(stream);

    if (stream->timeoutEvent) {
        mprRemoveEvent(stream->timeoutEvent);
        stream->timeoutEvent = 0;
    }
    stream->started = stream->http->now;
    stream->lastActivity = stream->http->now;
    stream->error = 0;
    stream->errorMsg = 0;
    stream->h2State = 0;
    stream->authRequested = 0;
    stream->completed = 0;
    stream->destroyed = 0;
    stream->streamID = 0;
    stream->state = 0;

    resetQueues(stream);
    httpSetState(stream, HTTP_STATE_BEGIN);
}


static void resetQueues(HttpStream *stream)
{
    HttpQueue   *q, *tailq;
    HttpNet     *net;
    HttpLimits  *limits;
    Http        *http;

    net = stream->net;
    http = net->http;
    limits = stream->limits;

    stream->rx = httpCreateRx(stream);
    stream->tx = httpCreateTx(stream, NULL);

    /*
        Create the queues with the default (mandatory) filters.
     */
    q = stream->rxHead = httpCreateQueue(net, stream, http->queueHead, HTTP_QUEUE_RX, NULL);
    q = tailq = httpCreateQueue(net, stream, http->tailFilter, HTTP_QUEUE_RX, q);
    if (net->protocol < 2) {
        q = httpCreateQueue(net, stream, http->chunkFilter, HTTP_QUEUE_RX, q);
    }
    stream->inputq = tailq;
    stream->readq = stream->rxHead;

    /*
        Create the TX queue head
     */
    q = stream->txHead = httpCreateQueue(net, stream, http->queueHead, HTTP_QUEUE_TX, NULL);
    if (net->protocol < 2) {
        q = httpCreateQueue(net, stream, http->chunkFilter, HTTP_QUEUE_TX, q);
    } else {
        stream->rx->remainingContent = HTTP_UNLIMITED;
    }
    q = tailq = httpCreateQueue(net, stream, http->tailFilter, HTTP_QUEUE_TX, q);
    stream->outputq = tailq;
    stream->writeq = stream->txHead;

#if CONFIG_HTTP_HTTP2
    /*
        The stream->outputq queue window limit is updated on receipt of the peer settings frame and this defines the maximum amount of
        data we can send without receipt of a window flow control update message.
        The stream->inputq window is defined by net->limits and will be
     */
    httpSetQueueLimits(stream->inputq, limits, -1, -1, -1, -1);
    httpSetQueueLimits(stream->outputq, limits, -1, -1, -1, net->window);
#endif
    httpOpenQueues(stream);
}


/*
    This will disconnect the socket for HTTP/1 only. HTTP/2 streams are destroyed for each request.
    For HTTP/1 they are reused via keep-alive.
 */
PUBLIC void httpDisconnectStream(HttpStream *stream)
{
    HttpTx      *tx;

    tx = stream->tx;
    stream->error++;
    if (tx) {
        //  Ensure state transitions to finalized
        tx->finalizedConnector = 1;
        httpFinalizeInput(stream);
        httpFinalize(stream);
    }
    if (stream->net->protocol < 2) {
        mprDisconnectSocket(stream->sock);
        stream->net->error = 1;
    }
}


static void streamTimeout(HttpStream *stream, MprEvent *mprEvent)
{
    HttpLimits  *limits;
    cchar       *event, *msg, *prefix;
    int         status;

    if (stream->destroyed) {
        return;
    }
    assert(stream->tx);
    assert(stream->rx);
    limits = stream->limits;

    msg = 0;
    event = 0;

    if (stream->timeoutCallback) {
        (stream->timeoutCallback)(stream);
        HTTP_NOTIFY(stream, HTTP_EVENT_TIMEOUT, 0);
    }

    prefix = (stream->state == HTTP_STATE_BEGIN) ? "Idle stream" : "Request";
    if (stream->timeout == HTTP_PARSE_TIMEOUT) {
        msg = sfmt("%s exceeded parse headers timeout of %lld sec", prefix, limits->requestParseTimeout  / 1000);
        event = "timeout.parse";

    } else if (stream->timeout == HTTP_INACTIVITY_TIMEOUT) {
        if (httpClientStream(stream) /* too noisy || (stream->rx && stream->rx->uri) */) {
            msg = sfmt("%s exceeded inactivity timeout of %lld sec", prefix, limits->inactivityTimeout / 1000);
            event = "timeout.inactivity";
        }

    } else if (stream->timeout == HTTP_REQUEST_TIMEOUT) {
        msg = sfmt("%s exceeded timeout %lld sec", prefix, limits->requestTimeout / 1000);
        event = "timeout.duration";
    }
    if (stream->state < HTTP_STATE_FIRST) {
        /*
            Connection is idle
         */
        if (msg && event) {
            httpLog(stream->trace, event, "error", "msg:%s", msg);
            stream->errorMsg = msg;
        }
        httpDisconnectStream(stream);
    } else {
        /*
            For HTTP/2, we error and complete the stream and keep the connection open
            For HTTP/1, we close the connection as the request is partially complete
         */
        status = HTTP_CODE_REQUEST_TIMEOUT | (stream->net->protocol < 2 ? HTTP_CLOSE : 0);
        httpError(stream, status, "%s", msg ? msg : "Timeout");
    }
}


PUBLIC void httpStreamTimeout(HttpStream *stream)
{
    if (!stream->timeoutEvent && !stream->destroyed) {
        /*
            Will run on the HttpStream dispatcher unless shutting down and it is destroyed already
         */
        if (stream->dispatcher && !(stream->dispatcher->flags & MPR_DISPATCHER_DESTROYED)) {
            stream->timeoutEvent = mprCreateLocalEvent(stream->dispatcher, "streamTimeout", 0, streamTimeout, stream, 0);
        }
    }
}


PUBLIC void httpFollowRedirects(HttpStream *stream, bool follow)
{
    stream->followRedirects = follow;
}


PUBLIC ssize httpGetChunkSize(HttpStream *stream)
{
    if (stream->tx) {
        return stream->tx->chunkSize;
    }
    return 0;
}


PUBLIC void *httpGetStreamContext(HttpStream *stream)
{
    return stream->context;
}


PUBLIC void *httpGetStreamHost(HttpStream *stream)
{
    return stream->host;
}


PUBLIC ssize httpGetWriteQueueCount(HttpStream *stream)
{
    return stream->writeq ? stream->writeq->count : 0;
}


PUBLIC void httpResetCredentials(HttpStream *stream)
{
    stream->authType = 0;
    stream->username = 0;
    stream->password = 0;
    httpRemoveHeader(stream, "Authorization");
}


PUBLIC void httpSetStreamNotifier(HttpStream *stream, HttpNotifier notifier)
{
    stream->notifier = notifier;
    /*
        Only issue a readable event if streaming or already routed
     */
    if (stream->readq->first && stream->rx->route) {
        HTTP_NOTIFY(stream, HTTP_EVENT_READABLE, 0);
    }
}


/*
    Password and authType can be null
    User may be a combined user:password
 */
PUBLIC void httpSetCredentials(HttpStream *stream, cchar *username, cchar *password, cchar *authType)
{
    char    *ptok;

    httpResetCredentials(stream);
    if (password == NULL && strchr(username, ':') != 0) {
        stream->username = ssplit(sclone(username), ":", &ptok);
        stream->password = sclone(ptok);
    } else {
        stream->username = sclone(username);
        stream->password = sclone(password);
    }
    if (authType) {
        stream->authType = sclone(authType);
    }
}


PUBLIC void httpSetKeepAliveCount(HttpStream *stream, int count)
{
    stream->keepAliveCount = count;
}


PUBLIC void httpSetChunkSize(HttpStream *stream, ssize size)
{
    if (stream->tx) {
        stream->tx->chunkSize = size;
    }
}


PUBLIC void httpSetHeadersCallback(HttpStream *stream, HttpHeadersCallback fn, void *arg)
{
    stream->headersCallback = fn;
    stream->headersCallbackArg = arg;
}


PUBLIC void httpSetStreamContext(HttpStream *stream, void *context)
{
    stream->context = context;
}


PUBLIC void httpSetStreamHost(HttpStream *stream, void *host)
{
    stream->host = host;
}


PUBLIC void httpNotify(HttpStream *stream, int event, int arg)
{
    if (stream->notifier) {
        (stream->notifier)(stream, event, arg);
    }
}


/*
    Set each timeout arg to -1 to skip. Set to zero for no timeout. Otherwise set to number of msecs.
 */
PUBLIC void httpSetTimeout(HttpStream *stream, MprTicks requestTimeout, MprTicks inactivityTimeout)
{
    if (requestTimeout >= 0) {
        if (requestTimeout == 0) {
            stream->limits->requestTimeout = HTTP_UNLIMITED;
        } else {
            stream->limits->requestTimeout = requestTimeout;
        }
    }
    if (inactivityTimeout >= 0) {
        if (inactivityTimeout == 0) {
            stream->limits->inactivityTimeout = HTTP_UNLIMITED;
            stream->net->limits->inactivityTimeout = HTTP_UNLIMITED;
        } else {
            stream->limits->inactivityTimeout = inactivityTimeout;
            stream->net->limits->inactivityTimeout = inactivityTimeout;
        }
    }
}


PUBLIC HttpLimits *httpSetUniqueStreamLimits(HttpStream *stream)
{
    HttpLimits      *limits;

    if ((limits = mprAllocStruct(HttpLimits)) != 0) {
        *limits = *stream->limits;
        stream->limits = limits;
    }
    return limits;
}


/*
    Test if a request has expired relative to the default inactivity and request timeout limits.
    Set timeout to a non-zero value to apply an overriding smaller timeout
    Set timeout to a value in msec. If timeout is zero, override default limits and wait forever.
    If timeout is < 0, use default inactivity and duration timeouts.
    If timeout is > 0, then use this timeout as an additional timeout.
 */
PUBLIC bool httpRequestExpired(HttpStream *stream, MprTicks timeout)
{
    HttpLimits  *limits;
    MprTicks    inactivityTimeout, requestTimeout;

    limits = stream->limits;
    if (mprGetDebugMode() || timeout == 0) {
        inactivityTimeout = requestTimeout = MPR_MAX_TIMEOUT;

    } else if (timeout < 0) {
        inactivityTimeout = limits->inactivityTimeout;
        requestTimeout = limits->requestTimeout;

    } else {
        inactivityTimeout = min(limits->inactivityTimeout, timeout);
        requestTimeout = min(limits->requestTimeout, timeout);
    }

    if (mprGetRemainingTicks(stream->started, requestTimeout) < 0) {
        if (requestTimeout != timeout) {
            httpLog(stream->trace, "timeout.duration", "error",
                "msg:Request cancelled exceeded max duration, timeout:%lld", requestTimeout / 1000);
        }
        return 1;
    }
    if (mprGetRemainingTicks(stream->lastActivity, inactivityTimeout) < 0) {
        if (inactivityTimeout != timeout) {
            httpLog(stream->trace, "timeout.inactivity", "error",
                "msg:Request cancelled due to inactivity, timeout:%lld", inactivityTimeout / 1000);
        }
        return 1;
    }
    return 0;
}


PUBLIC void httpSetStreamData(HttpStream *stream, void *data)
{
    stream->data = data;
}


PUBLIC void httpSetStreamReqData(HttpStream *stream, void *data)
{
    stream->reqData = data;
}


/*
    Invoke the callback. This routine is run on the streams dispatcher on an MPR thread.
    If the stream is destroyed, the event will be NULL.
 */
static void invokeWrapper(HttpInvoke *invoke, MprEvent *event)
{
    HttpStream  *stream;

    if (event && (stream = httpFindStream(invoke->seqno, NULL, NULL)) != NULL) {
        invoke->callback(stream, invoke->data);
    } else {
        invoke->callback(NULL, invoke->data);
    }
    invoke->hasRun = 1;
    mprRelease(invoke);
}


/*
    Create an event on a stream identified by its sequence number.
    This routine is foreign thread-safe.
 */
PUBLIC int httpCreateEvent(uint64 seqno, HttpEventProc callback, void *data)
{
    HttpInvoke  *invoke;

    /*
        Must allocate memory immune from GC. The hold is released in the invokeWrapper
        callback which is always invoked eventually.
     */
    if ((invoke = mprAllocMem(sizeof(HttpInvoke), MPR_ALLOC_ZERO | MPR_ALLOC_MANAGER | MPR_ALLOC_HOLD)) != 0) {
        mprSetManager(mprSetAllocName(invoke, "HttpInvoke@" MPR_LOC), (MprManager) manageInvoke);
        invoke->seqno = seqno;
        invoke->callback = callback;
        invoke->data = data;
    }
    /*
        Find the stream by "seqno" and then call createInvokeEvent() to create an MPR event.
     */
    if (httpFindStream(seqno, createInvokeEvent, invoke)) {
        //  invokeWrapper will release
        return 0;
    }
    mprRelease(invoke);
    return MPR_ERR_CANT_FIND;
}


/*
    Destructor for Invoke to make sure the callback is always called.
 */
static void manageInvoke(HttpInvoke *invoke, int flags)
{
    if (flags & MPR_MANAGE_FREE) {
        if (!invoke->hasRun) {
            invokeWrapper(invoke, NULL);
        }
    }
}


static void createInvokeEvent(HttpStream *stream, void *data)
{
    mprCreateLocalEvent(stream->dispatcher, "httpEvent", 0, (MprEventProc) invokeWrapper, data, MPR_EVENT_ALWAYS);
}


/*
    If invoking on a foreign thread, provide a callback proc and data so the stream can be safely accessed while locked.
    Do not use the returned value except to test for NULL in a foreign thread.
 */
PUBLIC HttpStream *httpFindStream(uint64 seqno, HttpEventProc proc, void *data)
{
    HttpNet     *net;
    HttpStream  *stream;
    int         nextNet, nextStream;

    if (!proc && !mprGetCurrentThread()) {
        assert(proc || mprGetCurrentThread());
        return NULL;
    }
    /*
        WARNING: GC can be running here in a foreign thread. The locks will ensure that networks and
        streams are not destroyed while locked. Event service lock is needed for the manageDispatcher
        which then marks networks.
     */
    stream = 0;
    lock(MPR->eventService);
    lock(HTTP->networks);
    for (ITERATE_ITEMS(HTTP->networks, net, nextNet)) {
        /*
            This lock prevents the stream being freed and from being removed from HttpNet.networks
         */
        lock(net->streams);
        for (ITERATE_ITEMS(net->streams, stream, nextStream)) {
            if (stream->seqno == seqno && !stream->destroyed) {
                if (proc) {
                    (proc)(stream, data);
                }
                nextNet = HTTP->networks->length;
                break;
            }
        }
        unlock(net->streams);
    }
    unlock(HTTP->networks);
    unlock(MPR->eventService);
    return stream;
}


PUBLIC void httpAddInputEndPacket(HttpStream *stream, HttpQueue *q)
{
    HttpRx  *rx;

    rx = stream->rx;
    if (!rx->inputEnded) {
        rx->inputEnded = 1;
        if (!q) {
            q = stream->inputq;
        }
        httpSetEof(stream);
        httpPutPacket(q, httpCreateEndPacket());
    }
}




