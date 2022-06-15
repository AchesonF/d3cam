#include "libmpr.h"

/*
    dispatcher.c - Event dispatch services

    This module is thread-safe.

 */

/********************************** Includes **********************************/



/*********************************** Locals *************************************/

#define RESERVED_DISPATCHER     ((MprOsThread) 0x1)

/***************************** Forward Declarations ***************************/

static bool claimDispatcher(MprDispatcher *dispatcher, MprOsThread thread);
static MprDispatcher *createQhead(cchar *name);
static void dequeueDispatcher(MprDispatcher *dispatcher);
static int dispatchEvents(MprDispatcher *dispatcher);
static void dispatchEventsHelper(MprDispatcher *dispatcher);
static MprTicks getDispatcherIdleTicks(MprDispatcher *dispatcher, MprTicks timeout);
static MprTicks getIdleTicks(MprEventService *es, MprTicks timeout);
static MprDispatcher *getNextReadyDispatcher(MprEventService *es);
static bool hasPendingDispatchers(void);
static void initDispatcher(MprDispatcher *q);
static void manageDispatcher(MprDispatcher *dispatcher, int flags);
static void manageEventService(MprEventService *es, int flags);
static bool ownedDispatcher(MprDispatcher *dispatcher);
static void queueDispatcher(MprDispatcher *prior, MprDispatcher *dispatcher);
static bool reclaimDispatcher(MprDispatcher *dispatcher);
static void releaseDispatcher(MprDispatcher *dispatcher);

#define isIdle(dispatcher) (dispatcher->parent == dispatcher->service->idleQ)
#define isRunning(dispatcher) (dispatcher->parent == dispatcher->service->runQ)
#define isReady(dispatcher) (dispatcher->parent == dispatcher->service->readyQ)
#define isWaiting(dispatcher) (dispatcher->parent == dispatcher->service->waitQ)
#define isEmpty(dispatcher) (dispatcher->eventQ->next == dispatcher->eventQ)

#if CONFIG_DEBUG
static bool isReservedDispatcher(MprDispatcher *dispatcher);
#endif

/************************************* Code ***********************************/
/*
    Create the overall dispatch service. There may be many event dispatchers.
 */
PUBLIC MprEventService *mprCreateEventService()
{
    MprEventService     *es;

    if ((es = mprAllocObj(MprEventService, manageEventService)) == 0) {
        return 0;
    }
    MPR->eventService = es;
    es->now = mprGetTicks();
    es->mutex = mprCreateLock();
    es->waitCond = mprCreateCond();
    es->runQ = createQhead("running");
    es->readyQ = createQhead("ready");
    es->idleQ = createQhead("idle");
    es->pendingQ = createQhead("pending");
    es->waitQ = createQhead("waiting");
    return es;
}


static void manageEventService(MprEventService *es, int flags)
{
    MprDispatcher   *dp;

    if (flags & MPR_MANAGE_MARK) {
        mprMark(es->runQ);
        mprMark(es->readyQ);
        mprMark(es->waitQ);
        mprMark(es->idleQ);
        mprMark(es->pendingQ);
        mprMark(es->waitCond);
        mprMark(es->mutex);

        /*
            Special case: must lock because mprCreateEvent may queue events while marking
         */
        lock(es);
        for (dp = es->runQ->next; dp != es->runQ; dp = dp->next) {
            mprMark(dp);
        }
        for (dp = es->readyQ->next; dp != es->readyQ; dp = dp->next) {
            mprMark(dp);
        }
        for (dp = es->waitQ->next; dp != es->waitQ; dp = dp->next) {
            mprMark(dp);
        }
        for (dp = es->idleQ->next; dp != es->idleQ; dp = dp->next) {
            mprMark(dp);
        }
        for (dp = es->pendingQ->next; dp != es->pendingQ; dp = dp->next) {
            mprMark(dp);
        }
        unlock(es);
    }
}


static void destroyDispatcherQueue(MprDispatcher *q)
{
    MprDispatcher   *dp, *next;

    for (dp = q->next; dp != q; dp = next) {
        next = dp->next;
        mprDestroyDispatcher(dp);
        if (next == dp->next) {
            break;
        }
    }
}


PUBLIC void mprStopEventService()
{
    MprEventService     *es;

    es = MPR->eventService;
    destroyDispatcherQueue(es->runQ);
    destroyDispatcherQueue(es->readyQ);
    destroyDispatcherQueue(es->waitQ);
    destroyDispatcherQueue(es->idleQ);
    destroyDispatcherQueue(es->pendingQ);
    es->mutex = 0;
}


PUBLIC void mprSetDispatcherImmediate(MprDispatcher *dispatcher)
{
    dispatcher->flags |= MPR_DISPATCHER_IMMEDIATE;
}


static MprDispatcher *createQhead(cchar *name)
{
    MprDispatcher       *dispatcher;

    if ((dispatcher = mprAllocObj(MprDispatcher, manageDispatcher)) == 0) {
        return 0;
    }
    dispatcher->service = MPR->eventService;
    dispatcher->name = name;
    initDispatcher(dispatcher);
    return dispatcher;
}


PUBLIC MprDispatcher *mprCreateDispatcher(cchar *name, int flags)
{
    MprEventService     *es;
    MprDispatcher       *dispatcher;

    es = MPR->eventService;
    if ((dispatcher = mprAllocObj(MprDispatcher, manageDispatcher)) == 0) {
        return 0;
    }
    dispatcher->flags = flags;
    dispatcher->service = es;
    dispatcher->name = name;
    dispatcher->cond = mprCreateCond();
    dispatcher->eventQ = mprCreateEventQueue();
    dispatcher->currentQ = mprCreateEventQueue();

    queueDispatcher(es->idleQ, dispatcher);
    return dispatcher;
}


static void freeEvents(MprEvent *q)
{
    MprEvent    *event, *next;

    if (q) {
        for (event = q->next; event != q; event = next) {
            next = event->next;
            if (event->cond) {
                mprSignalCond(event->cond);
            }
            if (event->dispatcher) {
                mprRemoveEvent(event);
            }
        }
    }
}


PUBLIC void mprDestroyDispatcher(MprDispatcher *dispatcher)
{
    MprEventService     *es;

    if (dispatcher && !(dispatcher->flags & MPR_DISPATCHER_DESTROYED)) {
        es = dispatcher->service;
        assert(es == MPR->eventService);
        lock(es);
        //  Must not free events in currentQ incase running in dispatchEvents -- current event must not be freed
        freeEvents(dispatcher->eventQ);
        if (!isRunning(dispatcher)) {
            //  Must not dequeue otherwise GC may claim dispatcher while running dispatchEvents
            dequeueDispatcher(dispatcher);
        }
        dispatcher->flags |= MPR_DISPATCHER_DESTROYED;
        unlock(es);
    }
}


static void manageDispatcher(MprDispatcher *dispatcher, int flags)
{
    MprEvent    *q, *event, *next;

    if (flags & MPR_MANAGE_MARK) {
        mprMark(dispatcher->currentQ);
        mprMark(dispatcher->eventQ);
        mprMark(dispatcher->cond);
        mprMark(dispatcher->parent);
        mprMark(dispatcher->service);

        if ((q = dispatcher->currentQ) != 0) {
            for (event = q->next; event != q; event = next) {
                next = event->next;
                mprMark(event);
            }
        }
        if ((q = dispatcher->eventQ) != 0) {
            for (event = q->next; event != q; event = next) {
                next = event->next;
                mprMark(event);
            }
        }
    } else if (flags & MPR_MANAGE_FREE) {
        if (!(dispatcher->flags & MPR_DISPATCHER_DESTROYED)) {
            freeEvents(dispatcher->currentQ);
            freeEvents(dispatcher->eventQ);
        }
    }
}


/*
    Schedule events.
    This routine will service events until the timeout expires or if MPR_SERVICE_NO_BLOCK is specified in flags,
    until there are no more events to service. This routine will also return when the MPR is stopping. This will
    service all enabled non-running dispatcher queues and pending I/O events.
    An app should dedicate only one thread to be an event service thread.
    @param timeout Time in milliseconds to wait. Set to zero for no wait. Set to -1 to wait forever.
    @param flags Set to MPR_SERVICE_NO_BLOCK for non-blocking.
    @returns Number of events serviced.
 */
PUBLIC int mprServiceEvents(MprTicks timeout, int flags)
{
    MprEventService     *es;
    MprDispatcher       *dp;
    MprTicks            expires, delay;
    int                 beginEventCount, eventCount;

    if (MPR->eventing) {
        mprLog("warn mpr event", 0, "mprServiceEvents called reentrantly");
        return 0;
    }
    if (mprIsDestroying()) {
        return 0;
    }
    MPR->eventing = 1;
    es = MPR->eventService;
    beginEventCount = eventCount = es->eventCount;
    es->now = mprGetTicks();
    expires = timeout < 0 ? MPR_MAX_TIMEOUT : (es->now + timeout);
    if (expires < 0) {
        expires = MPR_MAX_TIMEOUT;
    }
    mprSetWindowsThread(0);

    while (es->now <= expires) {
        eventCount = es->eventCount;
        mprServiceSignals();

        while ((dp = getNextReadyDispatcher(es)) != NULL) {
            assert(isReservedDispatcher(dp));
            assert(!isRunning(dp));
            queueDispatcher(es->runQ, dp);
            assert(isRunning(dp));

            /*
                dispatchEventsHelper will dispatch events and release the claim
             */
            if (dp->flags & MPR_DISPATCHER_IMMEDIATE) {
                dispatchEventsHelper(dp);

            } else if (mprStartWorker((MprWorkerProc) dispatchEventsHelper, dp) < 0) {
                releaseDispatcher(dp);
                queueDispatcher(es->pendingQ, dp);
                break;
            }
        }
        if (flags & MPR_SERVICE_NO_BLOCK) {
            expires = 0;
            /* But still service I/O events below */
        }
        if (es->eventCount == eventCount) {
            lock(es);
            delay = getIdleTicks(es, expires - es->now);
            es->willAwake = es->now + delay;
            es->waiting = 1;
            if (hasPendingDispatchers() && mprAvailableWorkers()) {
                delay = 0;
            }
            unlock(es);
            /*
                Service IO events. Will Yield.
             */
            mprWaitForIO(MPR->waitService, delay);
        }
        es->now = mprGetTicks();
        if (flags & MPR_SERVICE_NO_BLOCK) {
            break;
        }
        if (mprIsStopping()) {
            break;
        }
    }
    MPR->eventing = 0;
    mprSignalCond(MPR->cond);
    return abs(es->eventCount - beginEventCount);
}


PUBLIC void mprSuspendThread(MprTicks timeout)
{
    mprWaitForMultiCond(MPR->stopCond, timeout);
}


PUBLIC int64 mprGetEventMark(MprDispatcher *dispatcher)
{
    int64   result;

    /*
        Ensure all writes are flushed so user state will be valid across all threads
     */
    result = dispatcher->mark;
    mprAtomicBarrier(MPR_ATOMIC_SEQUENTIAL);
    return result;
}


/*
    Wait for an event to occur on the dispatcher and service the event. This is not called by mprServiceEvents.
    The dispatcher may be "started" and owned by the thread, or it may be unowned.
    WARNING: the event may have already happened by the time this API is invoked.
    WARNING: this will enable GC while sleeping.
 */
PUBLIC int mprWaitForEvent(MprDispatcher *dispatcher, MprTicks timeout, int64 mark)
{
    MprEventService     *es;
    MprTicks            delay;
    int                 changed;

    es = MPR->eventService;
    if (dispatcher == NULL) {
        dispatcher = MPR->dispatcher;
    }
    if (dispatcher->flags & MPR_DISPATCHER_DESTROYED) {
        return 0;
    }
    if (ownedDispatcher(dispatcher) && dispatchEvents(dispatcher)) {
        return 0;
    }

    lock(es);
    delay = getDispatcherIdleTicks(dispatcher, timeout);
    dispatcher->flags |= MPR_DISPATCHER_WAITING;
    changed = dispatcher->mark != mark && mark != -1;
    unlock(es);

    if (changed) {
        return 0;
    }
    if (!ownedDispatcher(dispatcher) || dispatchEvents(dispatcher) == 0) {
        mprYield(MPR_YIELD_STICKY);
        mprWaitForCond(dispatcher->cond, delay);
        mprResetYield();
        es->now = mprGetTicks();
    }
    lock(es);
    dispatcher->flags &= ~MPR_DISPATCHER_WAITING;
    unlock(es);
    return 0;
}


PUBLIC void mprSignalCompletion(MprDispatcher *dispatcher)
{
    if (dispatcher == 0) {
        dispatcher = MPR->dispatcher;
    }
    dispatcher->flags |= MPR_DISPATCHER_COMPLETE;
    mprSignalDispatcher(dispatcher);
}


/*
    Wait for an event to complete signified by the 'completion' flag being set.
    This will wait for events on the dispatcher.
    The completion flag will be reset on return.
 */
PUBLIC bool mprWaitForCompletion(MprDispatcher *dispatcher, MprTicks timeout)
{
    MprTicks    mark;
    bool        success;

    assert(timeout >= 0);

    if (dispatcher == 0) {
        dispatcher = MPR->dispatcher;
    }
    if (mprGetDebugMode()) {
        timeout *= 100;
    }
    for (mark = mprGetTicks(); !(dispatcher->flags & MPR_DISPATCHER_COMPLETE) && mprGetElapsedTicks(mark) < timeout; ) {
        mprWaitForEvent(dispatcher, 10, -1);
    }
    success = (dispatcher->flags & MPR_DISPATCHER_COMPLETE) ? 1 : 0;
    dispatcher->flags &= ~MPR_DISPATCHER_COMPLETE;
    return success;
}


PUBLIC void mprClearWaiting()
{
    MPR->eventService->waiting = 0;
}


PUBLIC void mprWakeEventService()
{
    if (MPR->eventService->waiting) {
        mprWakeNotifier();
    }
}


PUBLIC void mprWakeDispatchers()
{
    MprEventService     *es;
    MprDispatcher       *runQ, *dp;

    if ((es = MPR->eventService) == 0) {
        return;
    }
    lock(es);
    runQ = es->runQ;
    for (dp = runQ->next; dp != runQ; dp = dp->next) {
        mprSignalCond(dp->cond);
    }
    unlock(es);
}


PUBLIC int mprDispatchersAreIdle()
{
    MprEventService     *es;
    MprDispatcher       *runQ, *dispatcher;
    int                 idle;

    es = MPR->eventService;
    runQ = es->runQ;
    lock(es);
    dispatcher = runQ->next;
    idle = (dispatcher == runQ) ? 1 : (dispatcher->eventQ == dispatcher->eventQ->next);
    unlock(es);
    return idle;
}


/*
    Manually start the dispatcher by putting it on the runQ. This prevents the event service from
    starting any events in parallel. The invoking thread should service events directly by
    calling mprServiceEvents or mprWaitForEvent. This is often called by utility programs.
 */
PUBLIC int mprStartDispatcher(MprDispatcher *dispatcher)
{
    if (!claimDispatcher(dispatcher, 0)) {
        mprLog("error mpr event", 0, "Cannot start dispatcher - owned by another thread");
        return MPR_ERR_BAD_STATE;
    }
    if (!isRunning(dispatcher)) {
        queueDispatcher(dispatcher->service->runQ, dispatcher);
    }
    return 0;
}


PUBLIC int mprStopDispatcher(MprDispatcher *dispatcher)
{
    if (!ownedDispatcher(dispatcher)) {
        mprLog("error mpr event", 0, "Cannot stop dispatcher - owned by another thread");
        return MPR_ERR_BAD_STATE;
    }
    if (!isRunning(dispatcher)) {
        assert(isRunning(dispatcher));
        return MPR_ERR_BAD_STATE;
    }
    dequeueDispatcher(dispatcher);
    releaseDispatcher(dispatcher);
    mprScheduleDispatcher(dispatcher);
    return 0;
}


/*
    Schedule a dispatcher to run but don't disturb an already running dispatcher. If the event queue is empty,
    the dispatcher is moved to the idleQ. If there is a past-due event, it is moved to the readyQ. If there is a future
    event pending, it is put on the waitQ.
 */
PUBLIC void mprScheduleDispatcher(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    MprEvent            *event;
    int                 mustWakeWaitService, mustWakeCond;

    assert(dispatcher);
    if (dispatcher->flags & MPR_DISPATCHER_DESTROYED) {
        return;
    }
    if ((es = dispatcher->service) == 0) {
        return;
    }
    lock(es);
    es->now = mprGetTicks();
    mustWakeWaitService = es->waiting;

    if (isRunning(dispatcher)) {
        mustWakeCond = dispatcher->flags & MPR_DISPATCHER_WAITING;

    } else if (isEmpty(dispatcher)) {
        queueDispatcher(es->idleQ, dispatcher);
        mustWakeCond = dispatcher->flags & MPR_DISPATCHER_WAITING;

    } else {
        event = dispatcher->eventQ->next;
        mustWakeWaitService = mustWakeCond = 0;
        if (event->due > es->now) {
            queueDispatcher(es->waitQ, dispatcher);
            if (event->due < es->willAwake) {
                mustWakeWaitService = 1;
                mustWakeCond = dispatcher->flags & MPR_DISPATCHER_WAITING;
            }
        } else {
            queueDispatcher(es->readyQ, dispatcher);
            mustWakeWaitService = es->waiting;
            mustWakeCond = dispatcher->flags & MPR_DISPATCHER_WAITING;
        }
    }
    unlock(es);
    if (mustWakeCond) {
        mprSignalDispatcher(dispatcher);
    }
    if (mustWakeWaitService) {
        mprWakeEventService();
    }
}


PUBLIC void mprRescheduleDispatcher(MprDispatcher *dispatcher)
{
    if (dispatcher) {
        dequeueDispatcher(dispatcher);
        mprScheduleDispatcher(dispatcher);
    }
}


/*
    Run events for a dispatcher
    WARNING: may yield
 */
static int dispatchEvents(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    MprEvent            *event;
    int                 count;

    if (mprIsStopped()) {
        return 0;
    }
    assert(isRunning(dispatcher));
    assert(ownedDispatcher(dispatcher));

    es = dispatcher->service;

    /*
        Events are serviced from the dispatcher queue. When serviced, they are removed.
        If the callback calls mprRemoveEvent, it will not remove it from the queue, but will clear the continuous flag.
     */
    for (count = 0; (event = mprGetNextEvent(dispatcher)) != 0; count++) {
        mprAtomicAdd64(&dispatcher->mark, 1);
        mprLinkEvent(dispatcher->currentQ, event);

        //  WARNING: may yield
        //  WARNING: may destroy dispatcher (memory still present)
        (event->proc)(event->data, event);

        mprUnlinkEvent(event);
        event->hasRun = 1;

        if (event->cond) {
            mprSignalCond(event->cond);
        }
        if (dispatcher->flags & MPR_DISPATCHER_DESTROYED) {
            break;
        }
        if (event->flags & MPR_EVENT_CONTINUOUS && !event->next) {
            event->timestamp = dispatcher->service->now;
            event->due = event->timestamp + (event->period ? event->period : 1);
            mprQueueEvent(dispatcher, event);
        }
        lock(es);
        es->eventCount++;
        unlock(es);
    }
    return count;
}


/*
    Run events for a dispatcher. When complete, reschedule the dispatcher as required.
 */
static void dispatchEventsHelper(MprDispatcher *dispatcher)
{
    if (dispatcher->flags & MPR_DISPATCHER_DESTROYED) {
        /* Dispatcher destroyed after worker started */
        return;
    }
    if (!reclaimDispatcher(dispatcher)) {
        return;
    }
    dispatchEvents(dispatcher);

    releaseDispatcher(dispatcher);
    dequeueDispatcher(dispatcher);

    if (!(dispatcher->flags & MPR_DISPATCHER_DESTROYED)) {
        mprScheduleDispatcher(dispatcher);
    }
}


static bool hasPendingDispatchers()
{
    MprEventService *es;
    bool            hasPending;

    if ((es = MPR->eventService) == 0) {
        return 0;
    }
    lock(es);
    hasPending = es->pendingQ->next != es->pendingQ;
    unlock(es);
    return hasPending;
}


PUBLIC void mprWakePendingDispatchers()
{
    if (hasPendingDispatchers()) {
        mprWakeEventService();
    }
}


/*
    Get the next (ready) dispatcher off given runQ and move onto the runQ
 */
static MprDispatcher *getNextReadyDispatcher(MprEventService *es)
{
    MprDispatcher   *dp, *next, *pendingQ, *readyQ, *waitQ, *dispatcher;
    MprEvent        *event;

    waitQ = es->waitQ;
    readyQ = es->readyQ;
    pendingQ = es->pendingQ;
    dispatcher = 0;

    lock(es);
    if (pendingQ->next != pendingQ && mprAvailableWorkers() > 0) {
        dispatcher = pendingQ->next;

    } else if (readyQ->next == readyQ) {
        /*
            ReadyQ is empty, try to transfer a dispatcher with due events onto the readyQ
         */
        for (dp = waitQ->next; dp != waitQ; dp = next) {
            next = dp->next;
            event = dp->eventQ->next;
            if (event->due <= es->now) {
                queueDispatcher(es->readyQ, dp);
                break;
            }
        }
    }
    if (!dispatcher && readyQ->next != readyQ) {
        dispatcher = readyQ->next;
    }
    if (dispatcher && !claimDispatcher(dispatcher, RESERVED_DISPATCHER)) {
        dispatcher = 0;
    }
    unlock(es);
    return dispatcher;
}


/*
    Get the time to sleep till the next pending event. Must be called locked.
 */
static MprTicks getIdleTicks(MprEventService *es, MprTicks timeout)
{
    MprDispatcher   *readyQ, *waitQ, *dp;
    MprEvent        *event;
    MprTicks        delay;

    waitQ = es->waitQ;
    readyQ = es->readyQ;

    if (readyQ->next != readyQ) {
        delay = 0;
    } else if (mprIsStopping()) {
        delay = 10;
    } else {
        /*
            Examine all the dispatchers on the waitQ
         */
        delay = es->delay ? es->delay : MPR_MAX_TIMEOUT;
        for (dp = waitQ->next; dp != waitQ; dp = dp->next) {
            event = dp->eventQ->next;
            if (event != dp->eventQ) {
                delay = min(delay, (event->due - es->now));
                if (delay <= 0) {
                    break;
                }
            }
        }
        delay = min(delay, timeout);
        es->delay = 0;
    }
    return delay < 0 ? 0 : delay;
}


PUBLIC void mprSetEventServiceSleep(MprTicks delay)
{
    MPR->eventService->delay = delay;
}


static MprTicks getDispatcherIdleTicks(MprDispatcher *dispatcher, MprTicks timeout)
{
    MprEventService *es;
    MprEvent        *next;
    MprTicks        delay, expires;

    es = MPR->eventService;
    es->now = mprGetTicks();

    if (timeout < 0) {
        timeout = 0;
    } else {
        next = dispatcher->eventQ->next;
        delay = MPR_MAX_TIMEOUT;
        if (next != dispatcher->eventQ) {
            delay = (next->due - dispatcher->service->now);
            if (delay < 0) {
                delay = 0;
            }
        }
        expires = timeout < 0 ? MPR_MAX_TIMEOUT : (es->now + timeout);
        if (expires < 0) {
            expires = MPR_MAX_TIMEOUT;
        }
        timeout = expires - es->now;
        timeout = min(delay, timeout);
    }
    return timeout;
}


static void initDispatcher(MprDispatcher *dispatcher)
{
    dispatcher->next = dispatcher;
    dispatcher->prev = dispatcher;
    dispatcher->parent = dispatcher;
}


static void queueDispatcher(MprDispatcher *prior, MprDispatcher *dispatcher)
{
    assert(dispatcher->service == MPR->eventService);
    lock(dispatcher->service);

    if (dispatcher->parent) {
        dequeueDispatcher(dispatcher);
    }
    dispatcher->parent = prior->parent;
    dispatcher->prev = prior;
    dispatcher->next = prior->next;
    prior->next->prev = dispatcher;
    prior->next = dispatcher;
    unlock(dispatcher->service);
}


static void dequeueDispatcher(MprDispatcher *dispatcher)
{
    lock(dispatcher->service);
    if (dispatcher->next) {
        dispatcher->next->prev = dispatcher->prev;
        dispatcher->prev->next = dispatcher->next;
        dispatcher->next = dispatcher;
        dispatcher->prev = dispatcher;
        dispatcher->parent = dispatcher;
    } else {
        assert(dispatcher->parent == dispatcher);
        assert(dispatcher->next == dispatcher);
        assert(dispatcher->prev == dispatcher);
    }
    unlock(dispatcher->service);
}


static bool claimDispatcher(MprDispatcher *dispatcher, MprOsThread thread)
{
    MprEventService *es;

    if (dispatcher->flags & MPR_DISPATCHER_DESTROYED) {
        return 0;
    }
    es = MPR->eventService;
    lock(es);
    if (dispatcher->owner && dispatcher->owner != mprGetCurrentOsThread()) {
        unlock(es);
        return 0;
    }
    dispatcher->owner = (thread ? thread : mprGetCurrentOsThread());
    unlock(es);
    return 1;
}


/*
    Used by workers to change the owner
 */
static bool reclaimDispatcher(MprDispatcher *dispatcher)
{
    MprEventService *es;

    if (dispatcher->flags & MPR_DISPATCHER_DESTROYED) {
        return 0;
    }
    es = MPR->eventService;
    lock(es);
    if (dispatcher->owner != RESERVED_DISPATCHER) {
        assert(dispatcher->owner == RESERVED_DISPATCHER);
        unlock(es);
        return 0;
    }
    dispatcher->owner = mprGetCurrentOsThread();
    unlock(es);
    return 1;
}


static void releaseDispatcher(MprDispatcher *dispatcher)
{
    dispatcher->owner = 0;
}


static bool ownedDispatcher(MprDispatcher *dispatcher)
{
    if (dispatcher->flags & MPR_DISPATCHER_DESTROYED) {
        return 0;
    }
    return (dispatcher->owner == mprGetCurrentOsThread()) ? 1 : 0;
}


#if CONFIG_DEBUG
static bool isReservedDispatcher(MprDispatcher *dispatcher)
{
    if (dispatcher->flags & MPR_DISPATCHER_DESTROYED) {
        return 0;
    }
    return (dispatcher->owner == RESERVED_DISPATCHER) ? 1 : 0;
}
#endif


PUBLIC void mprSignalDispatcher(MprDispatcher *dispatcher)
{
    if (dispatcher == NULL) {
        dispatcher = MPR->dispatcher;
    }
    mprSignalCond(dispatcher->cond);
}


PUBLIC bool mprDispatcherHasEvents(MprDispatcher *dispatcher)
{
    if (dispatcher == 0) {
        return 0;
    }
    return !isEmpty(dispatcher);
}




