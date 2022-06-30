#include "libhttp.h"

/*
    pipeline.c -- HTTP pipeline processing.
 */

/********************************* Includes ***********************************/



/********************************** Forward ***********************************/

static int loadQueue(HttpQueue *q, ssize chunkSize);
static int matchFilter(HttpStream *stream, HttpStage *filter, HttpRoute *route, int dir);
static void openPipeQueues(HttpStream *stream, HttpQueue *qhead);
static void pairQueues(HttpQueue *head1, HttpQueue *head2);

/*********************************** Code *************************************/
/*
    Called after routing the request (httpRouteRequest)
 */
PUBLIC void httpCreatePipeline(HttpStream *stream)
{
    HttpRx      *rx;
    HttpRoute   *route;

    rx = stream->rx;
    route = rx->route;
    if (httpClientStream(stream) && !route) {
        route = stream->http->clientRoute;
    }
    httpCreateRxPipeline(stream, route);
    httpCreateTxPipeline(stream, route);
}


PUBLIC void httpCreateRxPipeline(HttpStream *stream, HttpRoute *route)
{
    HttpNet     *net;
    HttpTx      *tx;
    HttpRx      *rx;
    HttpQueue   *q;
    HttpStage   *stage, *filter;
    int         next;

    assert(stream);
    assert(route);

    net = stream->net;
    rx = stream->rx;
    tx = stream->tx;

    rx->inputPipeline = mprCreateList(-1, MPR_LIST_STABLE);
    if (route) {
        for (next = 0; (filter = mprGetNextItem(route->inputStages, &next)) != 0; ) {
            if (filter->flags & HTTP_STAGE_INTERNAL) {
                continue;
            }
            if (matchFilter(stream, filter, route, HTTP_STAGE_RX) == HTTP_ROUTE_OK) {
                mprAddItem(rx->inputPipeline, filter);
            }
        }
    }
    /*
        Convert the queue head to the tx handler
     */
    if (tx->handler) {
        httpAssignQueueCallbacks(stream->rxHead, tx->handler, HTTP_QUEUE_RX);
        stream->rxHead->name = tx->handler->name;
    }
    /*
        Insert the pipeline before the RxHead and after the existing stages (chunkFilter)
     */
    q = stream->rxHead->prevQ;
    stream->transferq = stream->rxHead;
    for (next = 0; (stage = mprGetNextItem(rx->inputPipeline, &next)) != 0; ) {
        q = httpCreateQueue(net, stream, stage, HTTP_QUEUE_RX, q);
        q->flags |= HTTP_QUEUE_REQUEST;
        if (!stream->transferq) {
            stream->transferq = q;
        }
    }
    stream->readq = stream->rxHead;

    if (httpClientStream(stream)) {
        pairQueues(stream->rxHead, stream->txHead);
        httpOpenQueues(stream);

    } else if (!rx->streaming) {
        q->max = stream->limits->rxFormSize;
    }
    if (net->protocol < 2) {
        net->inputq->stream = stream;
        net->inputq->pair->stream = stream;
    }
}


PUBLIC void httpCreateTxPipeline(HttpStream *stream, HttpRoute *route)
{
    Http        *http;
    HttpNet     *net;
    HttpTx      *tx;
    HttpRx      *rx;
    HttpQueue   *q;
    HttpStage   *stage, *filter;
    cchar       *pattern;
    int         next;//, simple;

    assert(stream);
    if (!route) {
        if (httpServerStream(stream)) {
            mprLog("error http", 0, "Missing route");
            return;
        }
        route = stream->http->clientRoute;
    }
    http = stream->http;
    net = stream->net;
    rx = stream->rx;
    tx = stream->tx;

    tx->charSet = route->charSet;

    tx->outputPipeline = mprCreateList(-1, MPR_LIST_STABLE);
    if (httpServerStream(stream)) {
        if (tx->handler == 0 || tx->finalized) {
            tx->handler = http->passHandler;
        }
        httpAssignQueueCallbacks(stream->txHead, tx->handler, HTTP_QUEUE_TX);
        stream->txHead->name = tx->handler->name;
    } else {
        //  No handler callbacks needed for the client side
        tx->started = 1;
    }
    //simple = 1;
    if (route->outputStages) {
        for (next = 0; (filter = mprGetNextItem(route->outputStages, &next)) != 0; ) {
            if (filter->flags & HTTP_STAGE_INTERNAL) {
                continue;
            }
            if (matchFilter(stream, filter, route, HTTP_STAGE_TX) == HTTP_ROUTE_OK) {
                mprAddItem(tx->outputPipeline, filter);
                tx->flags |= HTTP_TX_HAS_FILTERS;
            }
        }
    }
    /*
        Create the outgoing queues linked from the tx queue head
     */
    q = stream->txHead;
    for (ITERATE_ITEMS(tx->outputPipeline, stage, next)) {
        q = httpCreateQueue(stream->net, stream, stage, HTTP_QUEUE_TX, q);
        q->flags |= HTTP_QUEUE_REQUEST;
    }
    stream->writeq = stream->txHead;
    pairQueues(stream->txHead, stream->rxHead);
    pairQueues(stream->rxHead, stream->txHead);
    tx->connector = http->netConnector;
    tx->simplePipeline = (net->protocol < 2 && !net->secure && !(tx->flags & HTTP_TX_HAS_FILTERS) &&
        tx->chunkSize < 0 && !stream->error);
    httpTraceQueues(stream);

    /*
        Open the pipeline stages. This calls the open entry points on all stages.
     */
    tx->flags |= HTTP_TX_PIPELINE;
    httpOpenQueues(stream);

    /*
        Incase we got an error in opening queues, need to now revert to the pass handler
     */
    if (httpServerStream(stream)) {
        if (stream->error && tx->handler != http->passHandler) {
            tx->handler = http->passHandler;
            httpAssignQueueCallbacks(stream->writeq, tx->handler, HTTP_QUEUE_TX);
            stream->txHead->name = tx->handler->name;
        }
        if (tx->handler && tx->handler == http->passHandler) {
            httpServiceQueue(stream->writeq);
        }
    }
    if (net->endpoint) {
        pattern = rx->route->pattern;
        httpLog(stream->trace, "pipeline", "context",
            "route:%s, handler:%s, target:%s, endpoint:%s:%d, host:%s, filename:%s",
            pattern && *pattern ? pattern : "*",
            tx->handler->name, rx->route->targetRule, net->endpoint->ip, net->endpoint->port,
            stream->host->name ? stream->host->name : "default", tx->filename ? tx->filename : "");
    }
}


static void pairQueues(HttpQueue *head1, HttpQueue *head2)
{
    HttpQueue   *q, *rq;

    q = head1;
    do {
        if (q->pair == 0) {
            rq = head2;
            do {
                if (q->stage == rq->stage) {
                    httpPairQueues(q, rq);
                }
                rq = rq->nextQ;
            } while (rq != head2);
        }
        q = q->nextQ;
    } while (q != head1);
}


PUBLIC void httpOpenQueues(HttpStream *stream)
{
    openPipeQueues(stream, stream->rxHead);
    openPipeQueues(stream, stream->txHead);
}


static void openPipeQueues(HttpStream *stream, HttpQueue *qhead)
{
    HttpTx      *tx;
    HttpQueue   *q;

    tx = stream->tx;
    q = qhead;
    do {
        if (q->open && !(q->flags & (HTTP_QUEUE_OPEN_TRIED))) {
            if (q->pair == 0 || !(q->pair->flags & HTTP_QUEUE_OPEN_TRIED)) {
                loadQueue(q, tx->chunkSize);
                if (q->open) {
                    q->flags |= HTTP_QUEUE_OPEN_TRIED;
                    if (q->stage->open(q) == 0) {
                        q->flags |= HTTP_QUEUE_OPENED;
                    } else if (!stream->error) {
                        httpError(stream, HTTP_CODE_INTERNAL_SERVER_ERROR, "Cannot open stage %s", q->stage->name);
                    }
                }
            }
        }
        q = q->nextQ;
    } while (q != qhead);
}


static int loadQueue(HttpQueue *q, ssize chunkSize)
{
    Http        *http;
    HttpStream  *stream;
    HttpStage   *stage;
    MprModule   *module;

    stage = q->stage;
    stream = q->stream;
    http = q->stream->http;

    if (chunkSize > 0) {
        q->packetSize = min(q->packetSize, chunkSize);
    }
    if (stage->flags & HTTP_STAGE_UNLOADED && stage->module) {
        module = stage->module;
        module = mprCreateModule(module->name, module->path, module->entry, http);
        if (mprLoadModule(module) < 0) {
            httpError(stream, HTTP_CODE_INTERNAL_SERVER_ERROR, "Cannot load module %s", module->name);
            return MPR_ERR_CANT_READ;
        }
        stage->module = module;
    }
    if (stage->module) {
        stage->module->lastActivity = http->now;
    }
    return 0;
}


/*
    Set the fileHandler as the selected handler for the request and invoke the open and start fileHandler callbacks.
    Called by ESP to render a document.
 */
PUBLIC void httpSetFileHandler(HttpStream *stream, cchar *path)
{
    HttpStage   *fp;

    HttpTx      *tx;

    tx = stream->tx;
    if (path && path != tx->filename) {
        httpSetFilename(stream, path, 0);
    }
    tx->entityLength = tx->fileInfo.size;
    fp = tx->handler = HTTP->fileHandler;
    stream->writeq->service = fp->outgoingService;
    stream->readq->put = fp->incoming;
    fp->open(stream->writeq);
    fp->start(stream->writeq);
    httpLog(stream->trace, "pipeline", "context", "Relay to file handler");
}


PUBLIC void httpClosePipeline(HttpStream *stream)
{
    HttpQueue   *q;

    q = stream->txHead;
    do {
        if (q->close && q->flags & HTTP_QUEUE_OPENED) {
            q->flags &= ~HTTP_QUEUE_OPENED;
            q->stage->close(q);
        }
         q = q->nextQ;
    } while (q != stream->txHead);

    q = stream->rxHead;
    do {
        if (q->close && q->flags & HTTP_QUEUE_OPENED) {
            q->flags &= ~HTTP_QUEUE_OPENED;
            q->stage->close(q);
        }
        q = q->nextQ;
    } while (q != stream->rxHead);
}


/*
    Start all queues, but do not start the handler
 */
PUBLIC void httpStartPipeline(HttpStream *stream)
{
    HttpQueue   *q, *prevQ, *nextQ;
    HttpRx      *rx;

    rx = stream->rx;
    assert(stream->net->endpoint);

    q = stream->txHead;
    do {
        prevQ = q->prevQ;
        if (q != stream->writeq && q->start && !(q->flags & HTTP_QUEUE_STARTED)) {
            q->flags |= HTTP_QUEUE_STARTED;
            q->stage->start(q);
        }
        q = prevQ;
    } while (q != stream->txHead);

    if (rx->needInputPipeline) {
        q = stream->rxHead;
        do {
            nextQ = q->nextQ;
            if (q != stream->readq && q->start && !(q->flags & HTTP_QUEUE_STARTED)) {
                /* Don't start if tx side already started */
                if (q->pair == 0 || !(q->pair->flags & HTTP_QUEUE_STARTED)) {
                    q->flags |= HTTP_QUEUE_STARTED;
                    q->stage->start(q);
                }
            }
            q = nextQ;
        } while (q != stream->rxHead);
    }
}


/*
    Called when all input data has been received
 */
PUBLIC void httpReadyHandler(HttpStream *stream)
{
    HttpQueue   *q;

    q = stream->writeq;
    if (q->stage && q->stage->ready && !(q->flags & HTTP_QUEUE_READY)) {
        q->flags |= HTTP_QUEUE_READY;
        q->stage->ready(q);
    }
}


PUBLIC void httpStartHandler(HttpStream *stream)
{
    HttpQueue   *q;
    HttpTx      *tx;

    tx = stream->tx;
    if (!tx->started) {
        tx->started = 1;
        q = stream->writeq;
        if (q->stage->start && !(q->flags & HTTP_QUEUE_STARTED)) {
            q->flags |= HTTP_QUEUE_STARTED;
            q->stage->start(q);
        }
        if (tx->pendingFinalize) {
            tx->finalizedOutput = 0;
            httpFinalizeOutput(stream);
        }
    }
}


PUBLIC bool httpQueuesNeedService(HttpNet *net)
{
    HttpQueue   *q;

    q = net->serviceq;
    return (q->scheduleNext != q);
}


PUBLIC void httpDiscardData(HttpStream *stream, int dir)
{
    HttpTx      *tx;
    HttpQueue   *q, *qhead;

    tx = stream->tx;
    if (tx == 0) {
        return;
    }
    qhead = (dir == HTTP_QUEUE_TX) ? stream->txHead : stream->rxHead;
    httpDiscardQueueData(qhead, 1);
    for (q = qhead->nextQ; q != qhead; q = q->nextQ) {
        httpDiscardQueueData(q, 1);
    }
}


static int matchFilter(HttpStream *stream, HttpStage *filter, HttpRoute *route, int dir)
{
    HttpTx      *tx;

    tx = stream->tx;
    if (filter->match) {
        return filter->match(stream, route, dir);
    }
    if (filter->extensions && tx->ext) {
        if (mprLookupKey(filter->extensions, tx->ext)) {
            return HTTP_ROUTE_OK;
        }
        if (mprLookupKey(filter->extensions, "")) {
            return HTTP_ROUTE_OK;
        }
        return HTTP_ROUTE_OMIT_FILTER;
    }
    return HTTP_ROUTE_OK;
}


PUBLIC void httpRemoveChunkFilter(HttpQueue *head)
{
    HttpQueue   *q;
    HttpStream  *stream;

    stream = head->stream;

    for (q = head->nextQ; q != head; q = q->nextQ) {
        if (q->stage == stream->http->chunkFilter) {
            httpRemoveQueue(q);
            httpTraceQueues(stream);
            return;
        }
    }
}




