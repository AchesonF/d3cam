#include "libhttp.h"

/*
    queue.c -- Queue support routines. Queues are the bi-directional data flow channels for the pipeline.
 */

/********************************* Includes ***********************************/



/********************************** Forwards **********************************/

static void initQueue(HttpNet *net, HttpStream *stream, HttpQueue *q, cchar *name, int dir, int flags);
static void manageQueue(HttpQueue *q, int flags);

/************************************ Code ************************************/
/*
    Create a queue head that has no processing callbacks
 */
PUBLIC HttpQueue *httpCreateQueueHead(HttpNet *net, HttpStream *stream, cchar *name, int dir)
{
    HttpQueue   *q;

    assert(net);

    if ((q = mprAllocObj(HttpQueue, manageQueue)) == 0) {
        return 0;
    }
    initQueue(net, stream, q, name, dir, HTTP_QUEUE_HEAD);
    httpInitSchedulerQueue(q);
    return q;
}


/*
    If prev is supplied, this queue is appended after the given "prev" queue.
    Returns the new queue.
 */
PUBLIC HttpQueue *httpCreateQueue(HttpNet *net, HttpStream *stream, HttpStage *stage, int dir, HttpQueue *prev)
{
    HttpQueue   *q;
    int         flags;

    if ((q = mprAllocObj(HttpQueue, manageQueue)) == 0) {
        return 0;
    }
    if (stage) {
        flags = (stage->flags & HTTP_STAGE_QHEAD) ? HTTP_QUEUE_HEAD : 0;
        initQueue(net, stream, q, stage->name, dir, flags);
        httpAssignQueueCallbacks(q, stage, dir);
    }
    httpInitSchedulerQueue(q);
    if (prev) {
        httpAppendQueue(q, prev);
    }
    return q;
}


static void initQueue(HttpNet *net, HttpStream *stream, HttpQueue *q, cchar *name, int dir, int flags)
{
    if (dir == HTTP_QUEUE_TX) {
        flags |= HTTP_QUEUE_OUTGOING;
    }
    q->net = net;
    q->stream = stream;
    q->nextQ = q;
    q->prevQ = q;
    q->name = sclone(name);
    if (stream && stream->tx && stream->tx->chunkSize > 0) {
        q->packetSize = stream->tx->chunkSize;
    } else {
        q->packetSize = net->limits->packetSize;
    }
    q->max = q->packetSize * ME_QUEUE_MAX_FACTOR;
    q->low = q->packetSize;
    q->flags = flags;
}


static void manageQueue(HttpQueue *q, int flags)
{
    HttpPacket      *packet;

    if (flags & MPR_MANAGE_MARK) {
        mprMark(q->name);
        mprMark(q->nextQ);
        for (packet = q->first; packet; packet = packet->next) {
            mprMark(packet);
        }
        mprMark(q->stream);
        mprMark(q->last);
        mprMark(q->prevQ);
        mprMark(q->stage);
        mprMark(q->scheduleNext);
        mprMark(q->schedulePrev);
        mprMark(q->pair);
        mprMark(q->queueData);
        if (q->nextQ && q->nextQ->stage) {
            /* Not a queue head */
            mprMark(q->nextQ);
        }
    }
}


/*
    Assign stage callbacks to a queue
 */
PUBLIC void httpAssignQueueCallbacks(HttpQueue *q, HttpStage *stage, int dir)
{
    q->stage = stage;
    q->close = stage->close;
    q->open = stage->open;
    q->start = stage->start;
    if (dir == HTTP_QUEUE_TX) {
        q->put = stage->outgoing;
        q->service = stage->outgoingService;
    } else {
        q->put = stage->incoming;
        q->service = stage->incomingService;
    }
}


PUBLIC void httpSetQueueLimits(HttpQueue *q, HttpLimits *limits, ssize packetSize, ssize low, ssize max, ssize window)
{
    if (packetSize < 0) {
        if (limits) {
            packetSize = limits->packetSize;
        } else {
            packetSize = ME_PACKET_SIZE;
        }
    }
    if (max < 0) {
        max = q->packetSize * ME_QUEUE_MAX_FACTOR;
    }
    if (low < 0) {
        low = q->packetSize;
    }
    q->packetSize = packetSize;
    q->max = max;
    q->low = low;

#if CONFIG_HTTP_HTTP2
    if (window < 0) {
        if (limits) {
            window = limits->window;
        } else {
            window = HTTP2_MIN_WINDOW;
        }
    }
    q->window = window;
#endif
}


PUBLIC void httpPairQueues(HttpQueue *q1, HttpQueue *q2)
{
    q1->pair = q2;
    q2->pair = q1;
}


PUBLIC bool httpIsQueueSuspended(HttpQueue *q)
{
    return (q->flags & HTTP_QUEUE_SUSPENDED) ? 1 : 0;
}


PUBLIC void httpSuspendQueue(HttpQueue *q)
{
    q->flags |= HTTP_QUEUE_SUSPENDED;
}


PUBLIC bool httpIsSuspendQueue(HttpQueue *q)
{
    return q->flags & HTTP_QUEUE_SUSPENDED;
}


/*
    Remove all data in the queue. If removePackets is true, actually remove the packet too.
    This preserves the header and EOT packets.
 */
PUBLIC void httpDiscardQueueData(HttpQueue *q, bool removePackets)
{
    HttpPacket  *packet, *prev, *next;
    ssize       len;

    if (q == 0) {
        return;
    }
    for (prev = 0, packet = q->first; packet; packet = next) {
        next = packet->next;
        if (packet->flags & (HTTP_PACKET_RANGE | HTTP_PACKET_DATA)) {
            if (removePackets) {
                if (prev) {
                    prev->next = next;
                } else {
                    q->first = next;
                }
                if (packet == q->last) {
                    q->last = prev;
                }
                q->count -= httpGetPacketLength(packet);
                assert(q->count >= 0);
                if (q->flags & HTTP_QUEUE_SUSPENDED && q->count < q->max) {
                    httpResumeQueue(q, 1);
                }
                continue;
            } else {
                len = httpGetPacketLength(packet);
                if (q->stream && q->stream->tx && q->stream->tx->length > 0) {
                    q->stream->tx->length -= len;
                }
                q->count -= len;
                assert(q->count >= 0);
                if (packet->content) {
                    mprFlushBuf(packet->content);
                } else if (packet->esize > 0) {
                    packet->esize = 0;
                }
                if (q->flags & HTTP_QUEUE_SUSPENDED && q->count < q->max) {
                    httpResumeQueue(q, 1);
                }
            }
        }
        prev = packet;
    }
}


/*
    Flush queue data toward the network connector by scheduling the queue and servicing all scheduled queues.
    Return true if there is room for more data. If blocking is requested, the call will block until
    the queue count falls below the queue max. NOTE: may return early if the inactivityTimeout expires.
    WARNING: may yield.
 */
PUBLIC bool httpFlushQueue(HttpQueue *q, int flags)
{
    HttpNet     *net;
    HttpStream  *stream;
    MprTicks    timeout;
    int         events, mask;

    net = q->net;
    stream = q->stream;

    /*
        Initiate flushing.
     */
    if (!(q->flags & HTTP_QUEUE_SUSPENDED)) {
        httpScheduleQueue(q);
    }
    httpServiceNetQueues(net, flags);

    /*
        For HTTP/2 we must process incoming window update frames, so run any pending IO events.
     */
    if (net->protocol == HTTP_2) {
        //  WARNING: may yield
        mprWaitForEvent(stream->dispatcher, 0, mprGetEventMark(stream->dispatcher));
    }
    if (net->error) {
        return 1;
    }

    while (q->count > 0 && !stream->error && !net->error) {
        timeout = (flags & HTTP_BLOCK) ? stream->limits->inactivityTimeout : 0;
        mask = MPR_READABLE;
        if (net->socketq->count > 0 || net->ioCount > 0) {
            mask = MPR_WRITABLE;
        }
        if ((events = mprWaitForSingleIO((int) net->sock->fd, mask, timeout)) != 0) {
            stream->lastActivity = net->lastActivity = net->http->now;
            if (events & MPR_READABLE) {
                httpReadIO(net);
            }
            if ((net->socketq->count > 0 || net->ioCount > 0) && (events & MPR_WRITABLE)) {
                net->lastActivity = net->http->now;
                httpResumeQueue(net->socketq, 1);
                httpServiceNetQueues(net, flags);
            }
            if (net->protocol == HTTP_2) {
                /*
                    Process HTTP/2 window update messages for flow control. WARNING: may yield.
                 */
                mprWaitForEvent(stream->dispatcher, 0, mprGetEventMark(stream->dispatcher));
            }
        }
        httpServiceNetQueues(net, flags);
        if (!(flags & HTTP_BLOCK)) {
            break;
        }
    }
    return (q->count < q->max) ? 1 : 0;
}


PUBLIC void httpFlush(HttpStream *stream)
{
    httpFlushQueue(stream->writeq, HTTP_NON_BLOCK);
}


/*
    Flush the write queue. In sync mode, this call may yield.
 */
PUBLIC void httpFlushAll(HttpStream *stream)
{
    httpFlushQueue(stream->writeq, stream->net->async ? HTTP_NON_BLOCK : HTTP_BLOCK);
}


PUBLIC bool httpResumeQueue(HttpQueue *q, bool schedule)
{
    HttpQueue   *prevQ;

    if (q) {
        if (q->flags & HTTP_QUEUE_SUSPENDED || schedule) {
            q->flags &= ~HTTP_QUEUE_SUSPENDED;
            httpScheduleQueue(q);
        }
        prevQ = httpFindPreviousQueue(q);
        if (q->count == 0 && prevQ && prevQ->flags & HTTP_QUEUE_SUSPENDED) {
            httpResumeQueue(prevQ, schedule);
            return 1;
        }
    }
    return 0;
}


PUBLIC HttpQueue *httpFindNextQueue(HttpQueue *q)
{
    for (q = q->nextQ; q; q = q->nextQ) {
        //  FUTURE - should really be service && count
        if (q->service || q->count) {
            return q;
        }
        if (q->flags & HTTP_QUEUE_HEAD) {
            return q;
        }
    }
    return 0;
}


/*
    Find previous queue that has buffered data
*/
PUBLIC HttpQueue *httpFindPreviousQueue(HttpQueue *q)
{
    for (q = q->prevQ; q; q = q->prevQ) {
        //  FUTURE - should really be service && count
        if (q->service || q->count) {
            return q;
        }
        if (q->flags & HTTP_QUEUE_HEAD) {
            return q;
        }
    }
    return 0;
}


PUBLIC HttpQueue *httpGetNextQueueForService(HttpQueue *q)
{
    HttpQueue     *next;

    if (q->scheduleNext != q) {
        next = q->scheduleNext;
        next->schedulePrev->scheduleNext = next->scheduleNext;
        next->scheduleNext->schedulePrev = next->schedulePrev;
        next->schedulePrev = next->scheduleNext = next;
        return next;
    }
    return 0;
}


/*
    Return the number of bytes the queue will accept. Always positive.
 */
PUBLIC ssize httpGetQueueRoom(HttpQueue *q)
{
    assert(q->max > 0);
    assert(q->count >= 0);

    if (q->count >= q->max) {
        return 0;
    }
    return q->max - q->count;
}


PUBLIC void httpInitSchedulerQueue(HttpQueue *q)
{
    q->scheduleNext = q;
    q->schedulePrev = q;
}


/*
    Append a queue after the previous element
 */
PUBLIC HttpQueue *httpAppendQueue(HttpQueue *q, HttpQueue *prev)
{
    q->nextQ = prev->nextQ;
    q->prevQ = prev;
    prev->nextQ->prevQ = q;
    prev->nextQ = q;
    return q;
}


PUBLIC bool httpIsQueueEmpty(HttpQueue *q)
{
    return q->first == 0;
}


PUBLIC void httpRemoveQueue(HttpQueue *q)
{
    q->prevQ->nextQ = q->nextQ;
    q->nextQ->prevQ = q->prevQ;
    q->prevQ = q->nextQ = q;
    q->flags |= HTTP_QUEUE_REMOVED;
}


#if KEEP
PUBLIC void httpCheckQueues(HttpQueue *q)
{
    HttpQueue     *head;
    HttpQueue     *next;

    head = q->net->serviceq;

    for (next = head->scheduleNext; next != head; next = next->scheduleNext) {
        //  Some assert here
    }
}
#endif


PUBLIC void httpScheduleQueue(HttpQueue *q)
{
    HttpQueue     *head;

    head = q->net->serviceq;

    assert(q != head);
    if (q->service && q->scheduleNext == q && !(q->flags & HTTP_QUEUE_SUSPENDED)) {
        assert(q->service);
        q->scheduleNext = head;
        q->schedulePrev = head->schedulePrev;
        head->schedulePrev->scheduleNext = q;
        head->schedulePrev = q;
    }
}


PUBLIC void httpServiceQueue(HttpQueue *q)
{
    if (q->servicing) {
        q->flags |= HTTP_QUEUE_RESERVICE;
        return;
    }

    /*
        Hold the queue for GC while scheduling.
        Not typically required as the queue is typically linked into a pipeline.
     */
    if (q->net) {
        q->net->holdq = q;
    }
    /*
        Since we are servicing this "q" now, we can remove from the schedule queue if it is already queued.
     */
    if (q->net->serviceq->scheduleNext == q) {
        httpGetNextQueueForService(q->net->serviceq);
    }
    if (q->flags & HTTP_QUEUE_REMOVED) {
        return;
    }
    if (q->flags & HTTP_QUEUE_SUSPENDED) {
        return;
    }
    q->servicing = 1;
    q->service(q);

    if (q->flags & HTTP_QUEUE_RESERVICE) {
        q->flags &= ~HTTP_QUEUE_RESERVICE;
        httpScheduleQueue(q);
    }
    q->flags |= HTTP_QUEUE_SERVICED;
    q->servicing = 0;
}


PUBLIC void httpServiceQueues(HttpStream *stream, int flags)
{
    httpServiceNetQueues(stream->net, flags);
}


/*
    Run the queue service routines until there is no more work to be done.
    If flags & HTTP_BLOCK, this routine may block while yielding.  Return true if actual work was done.
 */
PUBLIC void httpServiceNetQueues(HttpNet *net, int flags)
{
    HttpQueue   *q, *lastq;
    int         pingPong = 0;

    lastq = net->serviceq->schedulePrev;
    pingPong = 0;

    while ((q = httpGetNextQueueForService(net->serviceq)) != NULL) {
        /*
            When assigning queue callbacks in pipeline, may cause an already queued handler to have
            no service callback when it is actually invoked here.
         */
        if (q->service) {
            httpServiceQueue(q);
        }
        if ((flags & HTTP_BLOCK) && mprNeedYield()) mprYield(0);

        /*
            Stop servicing if all queues that were scheduled at the start of the call have been serviced.
        */
        if ((flags & HTTP_CURRENT) && q == lastq) {
            break;
        }
        if (pingPong++ > 500) {
            //  Safety: Can get degenerate cases were badly written filters interact and recursively flow control each other.
            break;
        }
    }
    /*
        Always do a yield if requested even if there are no queues to service
     */
    if ((flags & HTTP_BLOCK) && mprNeedYield()) mprYield(0);
}


/*
    Return true if the next queue will accept this packet. If not, then disable the queue's service procedure.
    This may split the packet if it exceeds the downstreams maximum packet size.
    If the nextQ does not have a service routine, use the next stage that does.
 */
PUBLIC bool httpWillQueueAcceptPacket(HttpQueue *q, HttpQueue *nextQ, HttpPacket *packet)
{
    ssize       room, size;

    size = httpGetPacketLength(packet);
    if (nextQ->service == 0) {
        nextQ = httpFindNextQueue(nextQ);
    }
    if (nextQ == 0) {
        return 1;
    }
    room = nextQ->max - nextQ->count;
    if (size <= room || (size - room) < HTTP_QUEUE_ALLOW) {
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
        The downstream queue cannot accept this packet, so disable queue and mark the downstream queue as full and service
     */
    httpSuspendQueue(q);
    if (!(nextQ->flags & HTTP_QUEUE_SUSPENDED)) {
        httpScheduleQueue(nextQ);
    }
    return 0;
}


PUBLIC bool httpWillNextQueueAcceptPacket(HttpQueue *q, HttpPacket *packet)
{
    HttpQueue   *nextQ;

    if ((nextQ = httpFindNextQueue(q)) == 0) {
        return 1;
    }
    return httpWillQueueAcceptPacket(q, nextQ, packet);
}


PUBLIC bool httpIsNextQueueSuspended(HttpQueue *q)
{
    HttpQueue   *nextQ;

    if ((nextQ = httpFindNextQueue(q)) != 0 && nextQ->flags & HTTP_QUEUE_SUSPENDED) {
        return 1;
    }
    return 0;
}

/*
    Return true if the next queue will accept a certain amount of data. If not, then disable the queue's service procedure.
    Will not split the packet.
 */
PUBLIC bool httpWillNextQueueAcceptSize(HttpQueue *q, ssize size)
{
    HttpQueue   *nextQ;

    nextQ = q->nextQ;
    if (size <= nextQ->packetSize && (size + nextQ->count) <= nextQ->max) {
        return 1;
    }
    httpSuspendQueue(q);
    if (!(nextQ->flags & HTTP_QUEUE_SUSPENDED)) {
        httpScheduleQueue(nextQ);
    }
    return 0;
}


PUBLIC void httpTransferPackets(HttpQueue *inq, HttpQueue *outq)
{
    HttpPacket  *packet;

    if (inq != outq) {
        while ((packet = httpGetPacket(inq)) != 0) {
            httpPutPacket(outq, packet);
        }
    }
}


/*
    Replay packets through the untraversed portions of the pipeline.
    If the in and out queues are the same, the packes are replayed through the put() routine.
 */
PUBLIC void httpReplayPackets(HttpQueue *inq, HttpQueue *outq)
{
    HttpPacket  *chain, *next, *packet;

    if (inq != outq) {
        //  Different queues, can simply transfer
        httpTransferPackets(inq, outq);

    } else {
        //  Unlink the chain and then put back to the queue.
        chain = inq->first;
        inq->first = NULL;
        inq->count = 0;
        for (packet = chain; packet; packet = next) {
            next = packet->next;
            packet->next = 0;
            httpPutPacket(outq, packet);
        }
    }
}


#if CONFIG_DEBUG
PUBLIC bool httpVerifyQueue(HttpQueue *q)
{
    HttpPacket  *packet;
    ssize       count;

    count = 0;
    for (packet = q->first; packet; packet = packet->next) {
        if (packet->next == 0) {
            assert(packet == q->last);
        }
        count += httpGetPacketLength(packet);
    }
    assert(count == q->count);
    return count <= q->count;
}
#endif





