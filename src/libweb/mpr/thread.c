#include "libmpr.h"

/**
    thread.c - Primitive multi-threading support for Windows

    This module provides threading, mutex and condition variable APIs.

 */

/********************************* Includes **********************************/



/*************************** Forward Declarations ****************************/

static void changeState(MprWorker *worker, int state);
static MprWorker *createWorker(MprWorkerService *ws, ssize stackSize);
static int getNextThreadNum(MprWorkerService *ws);
static void manageThreadService(MprThreadService *ts, int flags);
static void manageThread(MprThread *tp, int flags);
static void manageWorker(MprWorker *worker, int flags);
static void manageWorkerService(MprWorkerService *ws, int flags);
static void pruneWorkers(MprWorkerService *ws, MprEvent *timer);
static void threadProc(MprThread *tp);
static void workerMain(MprWorker *worker, MprThread *tp);

/************************************ Code ***********************************/

PUBLIC MprThreadService *mprCreateThreadService()
{
    MprThreadService    *ts;

    if ((ts = mprAllocObj(MprThreadService, manageThreadService)) == 0) {
        return 0;
    }
    if ((ts->pauseThreads = mprCreateCond()) == 0) {
        return 0;
    }
    if ((ts->threads = mprCreateList(-1, 0)) == 0) {
        return 0;
    }
    MPR->mainOsThread = mprGetCurrentOsThread();
    MPR->threadService = ts;
    ts->stackSize = ME_STACK_SIZE;
    /*
        Don't actually create the thread. Just create a thread object for this main thread.
     */
    if ((ts->mainThread = mprCreateThread("main", NULL, NULL, 0)) == 0) {
        return 0;
    }
    ts->mainThread->isMain = 1;
    ts->mainThread->osThread = mprGetCurrentOsThread();
    return ts;
}


PUBLIC void mprStopThreadService()
{
}


static void manageThreadService(MprThreadService *ts, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ts->threads);
        mprMark(ts->mainThread);
        mprMark(ts->eventsThread);
        mprMark(ts->pauseThreads);
    }
}


PUBLIC void mprSetThreadStackSize(ssize size)
{
    MPR->threadService->stackSize = size;
}


PUBLIC MprThread *mprGetCurrentThread()
{
    MprThreadService    *ts;
    MprThread           *tp;
    MprOsThread         id;
    int                 i;

    ts = MPR->threadService;
    if (ts && ts->threads) {
        id = mprGetCurrentOsThread();
        lock(ts->threads);
        for (i = 0; i < ts->threads->length; i++) {
            tp = mprGetItem(ts->threads, i);
            if (tp->osThread == id) {
                unlock(ts->threads);
                return tp;
            }
        }
        unlock(ts->threads);
    }
    return 0;
}


PUBLIC cchar *mprGetCurrentThreadName()
{
    MprThread       *tp;

    if ((tp = mprGetCurrentThread()) == 0) {
        return 0;
    }
    return tp->name;
}


/*
    Create a main thread
 */
PUBLIC MprThread *mprCreateThread(cchar *name, void *entry, void *data, ssize stackSize)
{
    MprThreadService    *ts;
    MprThread           *tp;

    if ((ts = MPR->threadService) == 0) {
        return 0;
    }
    tp = mprAllocObj(MprThread, manageThread);
    if (tp == 0) {
        return 0;
    }
    tp->data = data;
    tp->entry = entry;
    tp->name = sclone(name);
    tp->mutex = mprCreateLock();
    tp->cond = mprCreateCond();
    tp->pid = getpid();
    tp->priority = MPR_NORMAL_PRIORITY;

    if (stackSize == 0) {
        tp->stackSize = ts->stackSize;
    } else {
        tp->stackSize = stackSize;
    }

    assert(ts);
    assert(ts->threads);
    if (mprAddItem(ts->threads, tp) < 0) {
        return 0;
    }
    return tp;
}


static void manageThread(MprThread *tp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(tp->mutex);
        mprMark(tp->cond);
        mprMark(tp->data);
        mprMark(tp->name);

    } else if (flags & MPR_MANAGE_FREE) {

    }
}


/*
    Entry thread function
 */
PUBLIC void *threadProcWrapper(void *data)
{
    threadProc((MprThread*) data);
    return 0;
}


/*
    Thread entry
 */
static void threadProc(MprThread *tp)
{
    assert(tp);

    tp->osThread = mprGetCurrentOsThread();

    tp->pid = getpid();

    (tp->entry)(tp->data, tp);
    mprRemoveItem(MPR->threadService->threads, tp);
    tp->pid = 0;
}


PUBLIC int mprStartThread(MprThread *tp)
{
    if (!tp) {
        assert(tp);
        return MPR_ERR_BAD_ARGS;
    }
    lock(tp);
    if (mprStartOsThread(tp->name, threadProcWrapper, tp, tp) < 0) {
        mprLog("error mpr thread", 0, "Cannot create thread %s", tp->name);
        unlock(tp);
        return MPR_ERR_CANT_INITIALIZE;
    }
    unlock(tp);
    return 0;
}


PUBLIC int mprStartOsThread(cchar *name, void *proc, void *data, MprThread *tp)
{
    ssize       stackSize;

    stackSize = tp ? tp->stackSize : ME_STACK_SIZE;

    pthread_attr_t  attr;
    pthread_t       h;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, stackSize);

    if (pthread_create(&h, &attr, proc, (void*) data) != 0) {
        pthread_attr_destroy(&attr);
        return MPR_ERR_CANT_CREATE;
    }
    pthread_attr_destroy(&attr);

    return 0;
}


PUBLIC MprOsThread mprGetCurrentOsThread()
{
    return (MprOsThread) pthread_self();
}


static void manageThreadLocal(MprThreadLocal *tls, int flags)
{
    if (flags & MPR_MANAGE_FREE) {
        if (tls->key) {
            pthread_key_delete(tls->key);
        }
    }
}


PUBLIC MprThreadLocal *mprCreateThreadLocal()
{
    MprThreadLocal      *tls;

    if ((tls = mprAllocObj(MprThreadLocal, manageThreadLocal)) == 0) {
        return 0;
    }

    if (pthread_key_create(&tls->key, NULL) != 0) {
        tls->key = 0;
        return 0;
    }

    return tls;
}


PUBLIC int mprSetThreadData(MprThreadLocal *tls, void *value)
{
    bool    err;

    err = pthread_setspecific(tls->key, value) != 0;

    return (err) ? MPR_ERR_CANT_WRITE: 0;
}


PUBLIC void *mprGetThreadData(MprThreadLocal *tls)
{
    return pthread_getspecific(tls->key);
}

/*
    Map MR priority to linux native priority. Unix priorities range from -19 to +19. Linux does -20 to +19.
 */
PUBLIC int mprMapMprPriorityToOs(int mprPriority)
{
    assert(mprPriority >= 0 && mprPriority < 100);

    if (mprPriority <= MPR_BACKGROUND_PRIORITY) {
        return 19;
    } else if (mprPriority <= MPR_LOW_PRIORITY) {
        return 10;
    } else if (mprPriority <= MPR_NORMAL_PRIORITY) {
        return 0;
    } else if (mprPriority <= MPR_HIGH_PRIORITY) {
        return -8;
    } else {
        return -19;
    }
    return 0;
}


/*
    Map O/S priority to Mpr priority.
 */
PUBLIC int mprMapOsPriorityToMpr(int nativePriority)
{
    int     priority;

    priority = (nativePriority + 19) * (100 / 40);
    if (priority < 0) {
        priority = 0;
    }
    if (priority >= 100) {
        priority = 99;
    }
    return priority;
}

PUBLIC MprWorkerService *mprCreateWorkerService()
{
    MprWorkerService      *ws;

    if ((ws = mprAllocObj(MprWorkerService, manageWorkerService)) == 0) {
        return 0;
    }
    ws->mutex = mprCreateLock();
    ws->minThreads = MPR_DEFAULT_MIN_THREADS;
    ws->maxThreads = MPR_DEFAULT_MAX_THREADS;

    /*
        Presize the lists so they cannot get memory allocation failures later on.
     */
    ws->idleThreads = mprCreateList(0, 0);
    mprSetListLimits(ws->idleThreads, ws->maxThreads, -1);
    ws->busyThreads = mprCreateList(0, 0);
    mprSetListLimits(ws->busyThreads, ws->maxThreads, -1);
    return ws;
}


static void manageWorkerService(MprWorkerService *ws, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ws->busyThreads);
        mprMark(ws->idleThreads);
        mprMark(ws->mutex);
        mprMark(ws->pruneTimer);
    }
}


PUBLIC int mprStartWorkerService()
{
    MprWorkerService    *ws;

    ws = MPR->workerService;
    mprSetMinWorkers(ws->minThreads);
    return 0;
}


PUBLIC void mprStopWorkers()
{
    MprWorkerService    *ws;
    MprWorker           *worker;
    int                 next;

    if ((ws = MPR->workerService) == 0) {
        return;
    }
    lock(ws);
    if (ws->pruneTimer) {
        mprRemoveEvent(ws->pruneTimer);
        ws->pruneTimer = 0;
    }
    /*
        Wake up all idle workers. Busy workers take care of themselves. An idle thread will wakeup, exit and be
        removed from the busy list and then delete the thread. We progressively remove the last thread in the idle
        list. ChangeState will move the workers to the busy queue.
     */
    for (next = -1; (worker = (MprWorker*) mprGetPrevItem(ws->idleThreads, &next)) != 0; ) {
        changeState(worker, MPR_WORKER_BUSY);
    }
    unlock(ws);
}


/*
    Define the new minimum number of workers. Pre-allocate the minimum.
 */
PUBLIC void mprSetMinWorkers(int n)
{
    MprWorker           *worker;
    MprWorkerService    *ws;

    ws = MPR->workerService;
    lock(ws);
    ws->minThreads = n;
    if (n > 0) {
        mprLog("info mpr thread", 5, "Pre-start %d workers", ws->minThreads);
    }
    while (ws->numThreads < ws->minThreads) {
        worker = createWorker(ws, ws->stackSize);
        ws->numThreads++;
        ws->maxUsedThreads = max(ws->numThreads, ws->maxUsedThreads);
        changeState(worker, MPR_WORKER_BUSY);
        mprStartThread(worker->thread);
    }
    unlock(ws);
}


/*
    Define a new maximum number of theads. Prune if currently over the max.
 */
PUBLIC void mprSetMaxWorkers(int n)
{
    MprWorkerService  *ws;

    if ((ws = MPR->workerService) == 0) {
        return;
    }
    lock(ws);
    ws->maxThreads = n;
    if (ws->numThreads > ws->maxThreads) {
        pruneWorkers(ws, NULL);
    }
    if (ws->minThreads > ws->maxThreads) {
        ws->minThreads = ws->maxThreads;
    }
    unlock(ws);
}


PUBLIC int mprGetMaxWorkers()
{
    return MPR->workerService->maxThreads;
}


/*
    Return the current worker thread object
 */
PUBLIC MprWorker *mprGetCurrentWorker()
{
    MprWorkerService    *ws;
    MprWorker           *worker;
    MprThread           *thread;
    int                 next;

    if ((ws = MPR->workerService) == 0) {
        return 0;
    }
    lock(ws);
    thread = mprGetCurrentThread();
    for (next = -1; (worker = (MprWorker*) mprGetPrevItem(ws->busyThreads, &next)) != 0; ) {
        if (worker->thread == thread) {
            unlock(ws);
            return worker;
        }
    }
    unlock(ws);
    return 0;
}


PUBLIC void mprActivateWorker(MprWorker *worker, MprWorkerProc proc, void *data)
{
    MprWorkerService    *ws;

    ws = worker->workerService;

    lock(ws);
    worker->proc = proc;
    worker->data = data;
    changeState(worker, MPR_WORKER_BUSY);
    unlock(ws);
}


PUBLIC void mprSetWorkerStartCallback(MprWorkerProc start)
{
    MPR->workerService->startWorker = start;
}


PUBLIC void mprGetWorkerStats(MprWorkerStats *stats)
{
    MprWorkerService    *ws;
    MprWorker           *wp;
    int                 next;

    if ((ws = MPR->workerService) == 0) {
        return;
    }
    lock(ws);
    stats->max = ws->maxThreads;
    stats->min = ws->minThreads;
    stats->maxUsed = ws->maxUsedThreads;

    stats->idle = (int) ws->idleThreads->length;
    stats->busy = (int) ws->busyThreads->length;

    stats->yielded = 0;
    for (ITERATE_ITEMS(ws->busyThreads, wp, next)) {
        /*
            Only count as yielded, those workers who call yield from their user code
            This excludes the yield in worker main
         */
        stats->yielded += (wp->thread->yielded && wp->running);
    }
    unlock(ws);
}


PUBLIC int mprAvailableWorkers()
{
    MprWorkerStats  wstats;
    int             spareThreads, result;

    memset(&wstats, 0, sizeof(wstats));
    mprGetWorkerStats(&wstats);
    /*
        SpareThreads    == Threads that can be created up to max threads
        ActiveWorkers   == Worker threads actively servicing requests
        SpareCores      == Cores available on the system
        Result          == Idle workers + lesser of SpareCores|SpareThreads
     */
    result = spareThreads = wstats.max - wstats.busy - wstats.idle;
#if CONFIG_MPR_THREAD_LIMIT_BY_CORES
{
    int     activeWorkers, spareCores;

    activeWorkers = wstats.busy - wstats.yielded;
    spareCores = MPR->heap->stats.cpuCores - activeWorkers;
    if (spareCores <= 0) {
        return 0;
    }
    result = wstats.idle + min(spareThreads, spareCores);
}
#endif
#if DEBUG_TRACE
    printf("Avail %d, busy %d, yielded %d, idle %d, spare-threads %d, spare-cores %d, mustYield %d\n", result, wstats.busy,
        wstats.yielded, wstats.idle, spareThreads, spareCores, MPR->heap->mustYield);
#endif
    return result;
}


PUBLIC int mprStartWorker(MprWorkerProc proc, void *data)
{
    MprWorkerService    *ws;
    MprWorker           *worker;

    if ((ws = MPR->workerService) == 0) {
        return MPR_ERR_BAD_ARGS;
    }
    lock(ws);
    if (mprIsStopped()) {
        unlock(ws);
        return MPR_ERR_BAD_STATE;
    }
    /*
        Try to find an idle thread and wake it up. It will wakeup in workerMain(). If not any available, then add
        another thread to the worker. Must account for workers we've already created but have not yet gone to work
        and inserted themselves in the idle/busy queues. Get most recently used idle worker so we tend to reuse
        active threads. This lets the pruner trim idle workers.
     */
    worker = mprGetLastItem(ws->idleThreads);
    if (worker) {
        worker->data = data;
        worker->proc = proc;
        changeState(worker, MPR_WORKER_BUSY);

    } else if (ws->numThreads < ws->maxThreads) {
        if (mprAvailableWorkers() == 0) {
            unlock(ws);
            return MPR_ERR_BUSY;
        }
        worker = createWorker(ws, ws->stackSize);
        ws->numThreads++;
        ws->maxUsedThreads = max(ws->numThreads, ws->maxUsedThreads);
        worker->data = data;
        worker->proc = proc;
        changeState(worker, MPR_WORKER_BUSY);
        mprStartThread(worker->thread);

    } else {
        unlock(ws);
        return MPR_ERR_BUSY;
    }
    if (!ws->pruneTimer && (ws->numThreads > ws->minThreads)) {
        ws->pruneTimer = mprCreateTimerEvent(NULL, "pruneWorkers", MPR_TIMEOUT_PRUNER, pruneWorkers, ws, MPR_EVENT_QUICK);
    }
    unlock(ws);
    return 0;
}


/*
    Trim idle workers
 */
static void pruneWorkers(MprWorkerService *ws, MprEvent *timer)
{
    MprWorker     *worker;
    int           index, pruned;

    if (mprGetDebugMode()) {
        mprRescheduleEvent(timer, 15 * 60 * TPS);
        return;
    }
    lock(ws);
    pruned = 0;
    for (index = 0; index < ws->idleThreads->length; index++) {
        if ((ws->numThreads - pruned) <= ws->minThreads) {
            break;
        }
        worker = mprGetItem(ws->idleThreads, index);
        if ((worker->lastActivity + MPR_TIMEOUT_WORKER) < MPR->eventService->now) {
            changeState(worker, MPR_WORKER_PRUNED);
            pruned++;
            index--;
        }
    }
    if (pruned) {
        mprLog("info mpr thread", 4, "Pruned %d workers, pool has %d workers. Limits %d-%d.",
            pruned, ws->numThreads - pruned, ws->minThreads, ws->maxThreads);
    }
    if (timer && (ws->numThreads < ws->minThreads)) {
        mprRemoveEvent(ws->pruneTimer);
        ws->pruneTimer = 0;
    }
    unlock(ws);
}


static int getNextThreadNum(MprWorkerService *ws)
{
    int     rc;

    lock(ws);
    rc = ws->nextThreadNum++;
    unlock(ws);
    return rc;
}


/*
    Define a new stack size for new workers. Existing workers unaffected.
 */
PUBLIC void mprSetWorkerStackSize(int n)
{
    MPR->workerService->stackSize = n;
}


/*
    Create a new thread for the task
 */
static MprWorker *createWorker(MprWorkerService *ws, ssize stackSize)
{
    MprWorker   *worker;

    char    name[16];

    if ((worker = mprAllocObj(MprWorker, manageWorker)) == 0) {
        return 0;
    }
    worker->workerService = ws;
    worker->idleCond = mprCreateCond();

    fmt(name, sizeof(name), "worker.%u", getNextThreadNum(ws));
    mprLog("info mpr thread", 5, "Create %s, pool has %d workers. Limits %d-%d.", name, ws->numThreads + 1,
        ws->minThreads, ws->maxThreads);
    worker->thread = mprCreateThread(name, (MprThreadProc) workerMain, worker, stackSize);
    worker->thread->isWorker = 1;
    return worker;
}


static void manageWorker(MprWorker *worker, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(worker->data);
        mprMark(worker->thread);
        mprMark(worker->workerService);
        mprMark(worker->idleCond);
    }
}


static void workerMain(MprWorker *worker, MprThread *tp)
{
    MprWorkerService    *ws;

    ws = MPR->workerService;
    assert(worker->state == MPR_WORKER_BUSY);
    assert(!worker->idleCond->triggered);

    if (ws->startWorker) {
        (*ws->startWorker)(worker->data, worker);
    }
    /*
        Very important for performance to elimminate to locking the WorkerService
     */
    while (!(worker->state & MPR_WORKER_PRUNED)) {
        if (worker->proc) {
            worker->running = 1;
            (*worker->proc)(worker->data, worker);
            worker->running = 0;
        }
        worker->lastActivity = MPR->eventService->now;
        if (mprIsStopping()) {
            break;
        }
        assert(worker->cleanup == 0);
        if (worker->cleanup) {
            (*worker->cleanup)(worker->data, worker);
            worker->cleanup = NULL;
        }
        worker->proc = 0;
        worker->data = 0;
        changeState(worker, MPR_WORKER_IDLE);

        /*
            Sleep till there is more work to do. Yield for GC first.
         */
        mprYield(MPR_YIELD_STICKY);
        mprWaitForCond(worker->idleCond, -1);
        mprResetYield();
    }
    lock(ws);
    changeState(worker, 0);
    worker->thread = 0;
    ws->numThreads--;
    unlock(ws);
    if (ws->numThreads) {
        mprLog("info mpr thread", 6, "Worker exiting with %d workers in the pool.", ws->numThreads);
    }
}


static void changeState(MprWorker *worker, int state)
{
    MprWorkerService    *ws;
    MprList             *lp;
    int                 wakeIdle, wakeDispatchers;

    if (state == worker->state) {
        return;
    }
    wakeIdle = wakeDispatchers = 0;
    lp = 0;
    ws = worker->workerService;
    if (!ws) {
        return;
    }
    lock(ws);

    switch (worker->state) {
    case MPR_WORKER_BUSY:
        lp = ws->busyThreads;
        break;

    case MPR_WORKER_IDLE:
        lp = ws->idleThreads;
        wakeIdle = 1;
        break;

    case MPR_WORKER_PRUNED:
        break;
    }

    /*
        Reassign the worker to the appropriate queue
     */
    if (lp) {
        mprRemoveItem(lp, worker);
    }
    lp = 0;
    switch (state) {
    case MPR_WORKER_BUSY:
        lp = ws->busyThreads;
        break;

    case MPR_WORKER_IDLE:
        lp = ws->idleThreads;
        wakeDispatchers = 1;
        break;

    case MPR_WORKER_PRUNED:
        /* Don't put on a queue and the thread will exit */
        wakeDispatchers = 1;
        break;
    }
    worker->state = state;

    if (lp) {
        if (mprAddItem(lp, worker) < 0) {
            unlock(ws);
            assert(!MPR_ERR_MEMORY);
            return;
        }
    }
    unlock(ws);
    if (wakeDispatchers) {
        mprWakePendingDispatchers();
    }
    if (wakeIdle) {
        mprSignalCond(worker->idleCond);
    }
}


PUBLIC ssize mprGetBusyWorkerCount()
{
    MprWorkerService    *ws;

    if ((ws = MPR->workerService) == 0) {
        return 0;
    }
    return mprGetListLength(MPR->workerService->busyThreads);
}


PUBLIC bool mprSetThreadYield(MprThread *tp, bool on)
{
    bool    prior;

    if (!tp) {
        tp = mprGetCurrentThread();
    }
    prior = !tp->noyield;
    tp->noyield = !on;
    return prior;
}




