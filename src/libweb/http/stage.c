#include "libhttp.h"

/*
    stage.c -- Stages are the building blocks of the Http request pipeline.

    Stages support the extensible and modular processing of HTTP requests. Handlers are a kind of stage that are the
    first line processing of a request. Connectors are the last stage in a chain to send/receive data over a network.

 */

/********************************* Includes ***********************************/



/********************************* Forwards ***********************************/

static void manageStage(HttpStage *stage, int flags);

/*********************************** Code *************************************/

PUBLIC HttpStage *httpCreateStage(cchar *name, int flags, MprModule *module)
{
    HttpStage     *stage;

    assert(name && *name);

    if ((stage = httpLookupStage(name)) != 0) {
        if (!(stage->flags & HTTP_STAGE_UNLOADED)) {
            mprLog("error http", 0, "Stage %s already exists", name);
            return 0;
        }
    } else if ((stage = mprAllocObj(HttpStage, manageStage)) == 0) {
        return 0;
    }
    stage->flags = flags;
    stage->name = sclone(name);
    stage->incoming = httpDefaultIncoming;
    stage->outgoing = httpDefaultOutgoing;
    stage->module = module;
    httpAddStage(stage);
    return stage;
}


static void manageStage(HttpStage *stage, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(stage->name);
        mprMark(stage->path);
        mprMark(stage->stageData);
        mprMark(stage->module);
        mprMark(stage->extensions);
    }
}


PUBLIC HttpStage *httpCloneStage(HttpStage *stage)
{
    HttpStage   *clone;

    if ((clone = mprAllocObj(HttpStage, manageStage)) == 0) {
        return 0;
    }
    *clone = *stage;
    return clone;
}


PUBLIC HttpStage *httpCreateHandler(cchar *name, MprModule *module)
{
    return httpCreateStage(name, HTTP_STAGE_HANDLER, module);
}


/*
    Put packets on the service queue.
 */
PUBLIC void httpDefaultOutgoing(HttpQueue *q, HttpPacket *packet)
{
    if (q->service) {
        httpPutForService(q, packet, HTTP_SCHEDULE_QUEUE);
    } else {
        httpPutPacketToNext(q, packet);
    }
}


/*
    Default incoming data routine for all stages. Transfer the data upstream to the next stage.
    This will join incoming packets at the queue head. Most handlers which are also queue heads use this routine.
 */
PUBLIC void httpDefaultIncoming(HttpQueue *q, HttpPacket *packet)
{
    HttpStream  *stream;

    assert(q);
    assert(packet);
    stream = q->stream;

    if (q->flags & HTTP_QUEUE_HEAD) {
        /*
            End of the pipeline
         */
        if (packet->flags & HTTP_PACKET_END) {
            httpPutForService(q, packet, HTTP_SCHEDULE_QUEUE);
            httpFinalizeInput(stream);

        } else if (packet->flags & HTTP_PACKET_SOLO) {
            httpPutForService(q, packet, HTTP_SCHEDULE_QUEUE);

        } else {
            httpJoinPacketForService(q, packet, HTTP_SCHEDULE_QUEUE);
        }
        if (stream->state < HTTP_STATE_COMPLETE) {
            HTTP_NOTIFY(stream, HTTP_EVENT_READABLE, 0);
        }

    } else {
        if (q->service) {
            httpPutForService(q, packet, HTTP_SCHEDULE_QUEUE);
        } else {
            httpPutPacketToNext(q, packet);
        }
    }
}


/*
    Default service callback. Copy packets to the next stage if flow-control permits.
    This will resume upstream queues and suspend the "q" if required.
 */
PUBLIC void httpDefaultService(HttpQueue *q)
{
    HttpPacket    *packet;

    if (q->direction == HTTP_QUEUE_RX && q->flags & HTTP_QUEUE_HEAD) {
        return;
    }
    for (packet = httpGetPacket(q); packet; packet = httpGetPacket(q)) {
        if (!httpWillNextQueueAcceptPacket(q, packet)) {
            httpPutBackPacket(q, packet);
            break;
        }
        httpPutPacketToNext(q, packet);
    }
}


PUBLIC void httpDiscardService(HttpQueue *q)
{
    HttpPacket  *packet;

    for (packet = httpGetPacket(q); packet; packet = httpGetPacket(q)) { }
}


/*
    The Queue head stage accepts incoming body data and initiates routing for form requests
 */
PUBLIC int httpOpenQueueHead()
{
    HttpStage   *stage;

    if ((stage = httpCreateStage("QueueHead", 0, NULL)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    HTTP->queueHead = stage;
    stage->flags |= HTTP_STAGE_QHEAD | HTTP_STAGE_INTERNAL;
    stage->incoming = httpDefaultIncoming;
    return 0;
}


PUBLIC HttpStage *httpCreateFilter(cchar *name, MprModule *module)
{
    return httpCreateStage(name, HTTP_STAGE_FILTER, module);
}


PUBLIC HttpStage *httpCreateConnector(cchar *name, MprModule *module)
{
    return httpCreateStage(name, HTTP_STAGE_CONNECTOR, module);
}




