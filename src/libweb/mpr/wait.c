#include "libmpr.h"

/*
    wait.c - Wait for I/O service.

    This module provides wait management for sockets and other file descriptors and allows users to create wait
    handlers which will be called when I/O events are detected. Multiple backends (one at a time) are supported.

    This module is thread-safe.

 */

/********************************* Includes ***********************************/



/***************************** Forward Declarations ***************************/

static void ioEvent(void *data, MprEvent *event);
static void manageWaitService(MprWaitService *ws, int flags);
static void manageWaitHandler(MprWaitHandler *wp, int flags);

/************************************ Code ************************************/
/*
    Initialize the service
 */
PUBLIC MprWaitService *mprCreateWaitService()
{
    MprWaitService  *ws;

    ws = mprAllocObj(MprWaitService, manageWaitService);
    if (ws == 0) {
        return 0;
    }
    MPR->waitService = ws;
    ws->handlers = mprCreateList(-1, 0);
    ws->mutex = mprCreateLock();
    ws->spin = mprCreateSpinLock();
    mprCreateNotifierService(ws);
    return ws;
}


static void manageWaitService(MprWaitService *ws, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ws->handlers);
        mprMark(ws->handlerMap);
        mprMark(ws->mutex);
        mprMark(ws->spin);
    }
#if ME_EVENT_NOTIFIER == MPR_EVENT_ASYNC
    mprManageAsync(ws, flags);
#endif
#if ME_EVENT_NOTIFIER == MPR_EVENT_EPOLL
    mprManageEpoll(ws, flags);
#endif
#if ME_EVENT_NOTIFIER == MPR_EVENT_KQUEUE
    mprManageKqueue(ws, flags);
#endif
#if ME_EVENT_NOTIFIER == MPR_EVENT_SELECT
    mprManageSelect(ws, flags);
#endif
}


PUBLIC void mprStopWaitService()
{
    MPR->waitService = 0;
}


static MprWaitHandler *initWaitHandler(MprWaitHandler *wp, int fd, int mask, MprDispatcher *dispatcher, void *proc,
    void *data, int flags)
{
    MprWaitService  *ws;

    assert(fd >= 0);
    ws = MPR->waitService;

#if CONFIG_DEBUG
    {
        MprWaitHandler  *op;
        static int      warnOnce = 0;
        int             index;

        for (ITERATE_ITEMS(ws->handlers, op, index)) {
            if (op->fd == fd) {
                if (!warnOnce) {
                    mprLog("error mpr event", 0, "Duplicate fd in wait handlers");
                    warnOnce = 1;
                }
            }
        }
    }
#endif
    wp->fd              = fd;
    wp->notifierIndex   = -1;
    wp->dispatcher      = dispatcher;
    wp->proc            = proc;
    wp->flags           = 0;
    wp->handlerData     = data;
    wp->service         = ws;
    wp->flags           = flags;

    if (mprGetListLength(ws->handlers) >= FD_SETSIZE) {
        mprLog("error mpr event", 1,
            "Too many io handlers: FD_SETSIZE %d, increase FD_SETSIZE or reduce limits", FD_SETSIZE);
        return 0;
    }

#if ME_EVENT_NOTIFIER == MPR_EVENT_SELECT
    if (fd >= FD_SETSIZE) {
        mprLog("error mpr event", 1, "File descriptor %d exceeds max FD_SETSIZE of %d, increase FD_SETSIZE or reduce limits",
            fd, FD_SETSIZE);
    }
#endif

    if (mask) {
        if (mprAddItem(ws->handlers, wp) < 0) {
            return 0;
        }
        mprNotifyOn(wp, mask);
    }
    return wp;
}


PUBLIC MprWaitHandler *mprCreateWaitHandler(int fd, int mask, MprDispatcher *dispatcher, void *proc, void *data, int flags)
{
    MprWaitHandler  *wp;

    assert(fd >= 0);

    if ((wp = mprAllocObj(MprWaitHandler, manageWaitHandler)) == 0) {
        return 0;
    }
    return initWaitHandler(wp, fd, mask, dispatcher, proc, data, flags);
}


static void manageWaitHandler(MprWaitHandler *wp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(wp->handlerData);
        mprMark(wp->dispatcher);
        mprMark(wp->requiredWorker);
        mprMark(wp->thread);
        mprMark(wp->callbackComplete);
    }
}


PUBLIC void mprRemoveWaitHandler(MprWaitHandler *wp)
{
    if (wp && wp->service) {
        if (!mprIsStopped()) {
            /*
                It needs special handling for the shutdown case when the locks have been removed
             */
            if (wp->fd >= 0 && wp->desiredMask) {
                mprNotifyOn(wp, 0);
            }
            mprRemoveItem(wp->service->handlers, wp);
        }
        wp->fd = INVALID_SOCKET;
    }
}


PUBLIC void mprDestroyWaitHandler(MprWaitHandler *wp)
{
    MprWaitService      *ws;

    if (wp == 0) {
        return;
    }
    ws = wp->service;
    lock(ws);
    if (wp->fd >= 0) {
        mprRemoveWaitHandler(wp);
        wp->fd = INVALID_SOCKET;
    }
    wp->dispatcher = 0;
    unlock(ws);
}


PUBLIC void mprQueueIOEvent(MprWaitHandler *wp)
{
    MprDispatcher   *dispatcher;

    if (wp->flags & MPR_WAIT_NEW_DISPATCHER) {
        dispatcher = mprCreateDispatcher("IO", MPR_DISPATCHER_AUTO);
    } else if (wp->dispatcher) {
        dispatcher = wp->dispatcher;
    } else {
        dispatcher = mprGetDispatcher();
    }
    mprCreateIOEvent(dispatcher, ioEvent, wp, wp, NULL);
}


static void ioEvent(void *data, MprEvent *event)
{
    assert(event);
    if (!event) {
        return;
    }
    assert(event->handler);

    event->handler->proc(data, event);
}


PUBLIC void mprWaitOn(MprWaitHandler *wp, int mask)
{
    if (!wp->service) {
        return;
    }
    lock(wp->service);
    if (mask != wp->desiredMask) {
        if (wp->flags & MPR_WAIT_RECALL_HANDLER) {
            wp->service->needRecall = 1;
        }
        mprNotifyOn(wp, mask);
    }
    unlock(wp->service);
}


/*
    Set a handler to be recalled without further I/O
 */
PUBLIC void mprRecallWaitHandlerByFd(Socket fd)
{
    MprWaitService  *ws;
    MprWaitHandler  *wp;
    int             index;

    assert(fd >= 0);

    if ((ws = MPR->waitService) == 0) {
        return;
    }
    lock(ws);
    for (index = 0; (wp = (MprWaitHandler*) mprGetNextItem(ws->handlers, &index)) != 0; ) {
        if (wp->fd == fd) {
            wp->flags |= MPR_WAIT_RECALL_HANDLER;
            ws->needRecall = 1;
            mprWakeEventService();
            break;
        }
    }
    unlock(ws);
}


PUBLIC void mprRecallWaitHandler(MprWaitHandler *wp)
{
    MprWaitService  *ws;

    if (wp) {
        ws = MPR->waitService;
        if (ws) {
            lock(ws);
            wp->flags |= MPR_WAIT_RECALL_HANDLER;
            ws->needRecall = 1;
            mprWakeEventService();
            unlock(ws);
        }
    }
}


/*
    Recall a handler which may have buffered data. Only called by notifiers.
 */
PUBLIC int mprDoWaitRecall(MprWaitService *ws)
{
    MprWaitHandler      *wp;
    int                 count, index;

    if (!ws) {
        return 0;
    }
    lock(ws);
    ws->needRecall = 0;
    count = 0;
    for (index = 0; (wp = (MprWaitHandler*) mprGetNextItem(ws->handlers, &index)) != 0; ) {
        if ((wp->flags & MPR_WAIT_RECALL_HANDLER) && (wp->desiredMask & MPR_READABLE)) {
            wp->presentMask |= MPR_READABLE;
            wp->flags &= ~MPR_WAIT_RECALL_HANDLER;
            mprNotifyOn(wp, 0);
            mprQueueIOEvent(wp);
            count++;
        }
    }
    unlock(ws);
    return count;
}





