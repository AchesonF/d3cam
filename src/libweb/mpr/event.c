#include "libmpr.h"

/*
    event.c - Event and dispatch services

    This module is thread-safe.

 */

/********************************** Includes **********************************/



/***************************** Forward Declarations ***************************/

static MprEvent *createEvent(MprDispatcher *dispatcher, cchar *name, MprTicks period, void *proc, void *data, int flags);
static void initEventQ(MprEvent *q, cchar *name);
static void manageEvent(MprEvent *event, int flags);

/************************************* Code ***********************************/
/*
    Create and queue a new event for service. Period is used as the delay before running the event and as the period
    between events for continuous events.
 */
PUBLIC MprEvent *mprCreateEventQueue()
{
    MprEvent    *queue;

    if ((queue = mprAllocObj(MprEvent, manageEvent)) == 0) {
        return 0;
    }
    initEventQ(queue, "eventq");
    return queue;
}


PUBLIC MprEvent *mprCreateLocalEvent(MprDispatcher *dispatcher, cchar *name, MprTicks period, void *proc, void *data, int flags)
{
    return createEvent(dispatcher, name, period, proc, data, flags | MPR_EVENT_LOCAL);
}


/*
    Must only be called from an Webapp thread owning this dispatcher
 */
PUBLIC void mprCreateIOEvent(MprDispatcher *dispatcher, void *proc, void *data, MprWaitHandler *wp, MprSocket *sock)
{
    MprEvent    *event;

    assert(proc);
    assert(wp);
    if ((event = createEvent(dispatcher, "IOEvent", 0, proc, wp->handlerData, MPR_EVENT_LOCAL | MPR_EVENT_DONT_QUEUE)) != 0) {
        event->mask = wp->presentMask;
        event->handler = wp;
        event->sock = sock;
        mprQueueEvent(dispatcher, event);
    }
}


/*
    Create an interval timer
 */
PUBLIC MprEvent *mprCreateTimerEvent(MprDispatcher *dispatcher, cchar *name, MprTicks period, void *proc, void *data, int flags)
{
    return createEvent(dispatcher, name, period, proc, data, MPR_EVENT_CONTINUOUS | MPR_EVENT_LOCAL | flags);
}


/*
    Create and queue an event for execution by a dispatcher.
    May be called by foreign threads if:
    - The dispatcher and data are held or null.
    - The caller is responsible for the data memory
    - The caller is responsibile to either set MPR_EVENT_LOCAL or calling mprRelase on the event.
 */
PUBLIC MprEvent *mprCreateEvent(MprDispatcher *dispatcher, cchar *name, MprTicks period, void *proc, void *data, int flags)
{
    return createEvent(dispatcher, name, period, proc, data, flags);
}


/*
    Create a new event. The period is used as the delay before running the event and
    as the period between events for continuous events.
    This routine is foreign thread-safe provided the dispatcher and data are held or null.
 */
static MprEvent *createEvent(MprDispatcher *dispatcher, cchar *name, MprTicks period, void *proc, void *data, int flags)
{
    MprEvent    *event;
    int         aflags;

    if (dispatcher == 0) {
        dispatcher = (flags & MPR_EVENT_QUICK) ? MPR->nonBlock : MPR->dispatcher;
    }
    if (dispatcher && dispatcher->flags & MPR_DISPATCHER_DESTROYED) {
        dispatcher = (flags & MPR_EVENT_QUICK) ? MPR->nonBlock : MPR->dispatcher;
    }
    /*
        The hold is for allocations via foreign threads which retains the event until it is queued.
        The hold is released after the event is queued.
     */
    aflags = MPR_ALLOC_MANAGER | MPR_ALLOC_ZERO;
    if (!(flags & MPR_EVENT_LOCAL)) {
        aflags |= MPR_ALLOC_HOLD;
    }
    if ((event = mprAllocMem(sizeof(MprEvent), aflags)) == 0) {
        return 0;
    }
    mprSetManager(event, (MprManager) manageEvent);
    event->name = name;
    event->proc = proc;
    event->data = data;
    event->dispatcher = dispatcher;
    event->next = event->prev = 0;
    event->flags = flags;
    event->timestamp = mprGetTicks();
    if (period < 0) {
        period = -period;
    } else if (period > MPR_EVENT_MAX_PERIOD) {
        period = MPR_EVENT_MAX_PERIOD;
    }
    event->period = period;
    event->due = event->timestamp + period;

    if (!(flags & MPR_EVENT_DONT_QUEUE)) {
        mprQueueEvent(dispatcher, event);
        if (aflags & MPR_ALLOC_HOLD) {
            mprRelease(event);
        }
    }
    return event;
}


static void manageEvent(MprEvent *event, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        //  Don't mark the dispatcher as it will be marked elsewhere by owners of the dispatcher
        mprMark(event->handler);
        if (!(event->flags & MPR_EVENT_STATIC_DATA)) {
            mprMark(event->data);
        }
        mprMark(event->sock);
        mprMark(event->cond);

    } else if (flags & MPR_MANAGE_FREE) {
        if (!event->hasRun && (event->flags & MPR_EVENT_ALWAYS)) {
            (event->proc)(event->data, NULL);
        }
    }
}


PUBLIC void mprQueueEvent(MprDispatcher *dispatcher, MprEvent *event)
{
    MprEventService     *es;
    MprEvent            *prior, *q;

    assert(dispatcher);
    assert(event);
    assert(event->timestamp);
    assert(event->next == NULL);

    es = dispatcher->service;
    lock(es);
    if (!(dispatcher->flags & MPR_DISPATCHER_DESTROYED)) {
        q = dispatcher->eventQ;
        for (prior = q->prev; prior != q; prior = prior->prev) {
            if (event->due > prior->due) {
                break;
            } else if (event->due == prior->due) {
                break;
            }
        }
        assert(prior->next);
        assert(prior->prev);

        mprLinkEvent(prior, event);
        event->dispatcher = dispatcher;
        es->eventCount++;
        mprScheduleDispatcher(dispatcher);
    }
    unlock(es);
}


PUBLIC void mprRemoveEvent(MprEvent *event)
{
    MprEventService     *es;
    MprDispatcher       *dispatcher;

    if (!event) {
        return;
    }
    dispatcher = event->dispatcher;
    if (dispatcher) {
        es = dispatcher->service;
        lock(es);
        if (event->next) {
            mprUnlinkEvent(event);
        }
        event->dispatcher = 0;
        event->flags &= ~MPR_EVENT_CONTINUOUS;
        if (event->due == es->willAwake && dispatcher->eventQ && dispatcher->eventQ->next != dispatcher->eventQ) {
            mprScheduleDispatcher(dispatcher);
        }
        if (event->cond) {
            mprSignalCond(event->cond);
        }
        unlock(es);
    }
}


PUBLIC void mprRescheduleEvent(MprEvent *event, MprTicks period)
{
    MprEventService     *es;
    MprDispatcher       *dispatcher;
    int                 continuous;

    dispatcher = event->dispatcher;

    if ((es = dispatcher->service) == 0) {
        return;
    }
    lock(es);
    event->period = period;
    event->timestamp = es->now;
    event->due = event->timestamp + period;
    if (event->next) {
        continuous = event->flags & MPR_EVENT_CONTINUOUS;
        mprRemoveEvent(event);
        event->flags |= continuous;
    }
    mprQueueEvent(dispatcher, event);
    unlock(es);
}


PUBLIC void mprStopContinuousEvent(MprEvent *event)
{
    lock(event->dispatcher->service);
    event->flags &= ~MPR_EVENT_CONTINUOUS;
    unlock(event->dispatcher->service);
}


PUBLIC void mprRestartContinuousEvent(MprEvent *event)
{
    lock(event->dispatcher->service);
    event->flags |= MPR_EVENT_CONTINUOUS;
    unlock(event->dispatcher->service);
    mprRescheduleEvent(event, event->period);
}


PUBLIC void mprEnableContinuousEvent(MprEvent *event, int enable)
{
    lock(event->dispatcher->service);
    event->flags &= ~MPR_EVENT_CONTINUOUS;
    if (enable) {
        event->flags |= MPR_EVENT_CONTINUOUS;
    }
    unlock(event->dispatcher->service);
}


/*
    Get the next due event from the front of the event queue and dequeue it.
    Internal: only called by the dispatcher
 */
PUBLIC MprEvent *mprGetNextEvent(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    MprEvent            *event, *next;

    if ((es = dispatcher->service) == 0) {
        return 0;
    }
    event = 0;
    lock(es);
    next = dispatcher->eventQ->next;
    if (next != dispatcher->eventQ) {
        if (next->due <= es->now) {
            event = next;
            mprUnlinkEvent(event);
        }
    }
    unlock(es);
    return event;
}


PUBLIC int mprGetEventCount(MprDispatcher *dispatcher)
{
    MprEventService     *es;
    MprEvent            *event;
    int                 count;

    es = dispatcher->service;

    lock(es);
    count = 0;
    for (event = dispatcher->eventQ->next; event != dispatcher->eventQ; event = event->next) {
        count++;
    }
    unlock(es);
    return count;
}


static void initEventQ(MprEvent *q, cchar *name)
{
    assert(q);

    q->next = q;
    q->prev = q;
    q->name = name;
}


/*
    Append a new event. Must be locked when called.
 */
PUBLIC void mprLinkEvent(MprEvent *prior, MprEvent *event)
{
    assert(prior);
    if (!event) {
        return;
    }
    if (!prior) {
        return;
    }
    assert(event);
    assert(prior->next);

    if (event->next) {
        mprUnlinkEvent(event);
    }
    event->prev = prior;
    event->next = prior->next;
    prior->next->prev = event;
    prior->next = event;
}


/*
    Remove an event. Must be locked when called.
 */
PUBLIC void mprUnlinkEvent(MprEvent *event)
{
    assert(event);

    /* If a continuous event is removed, next may already be null */
    if (event->next) {
        event->next->prev = event->prev;
        event->prev->next = event->next;
        event->next = 0;
        event->prev = 0;
    }
}




