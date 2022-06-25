#include "libmpr.h"

/**
    signal.c - Signal handling for Unix systems

 */

/*********************************** Includes *********************************/



/*********************************** Forwards *********************************/

static void manageSignal(MprSignal *sp, int flags);
static void manageSignalService(MprSignalService *ssp, int flags);
static void signalEvent(MprSignal *sp, MprEvent *event);
static void signalHandler(int signo, siginfo_t *info, void *arg);
static void standardSignalHandler(void *ignored, MprSignal *sp);
static void unhookSignal(int signo);

/************************************ Code ************************************/

PUBLIC MprSignalService *mprCreateSignalService()
{
    MprSignalService    *ssp;

    if ((ssp = mprAllocObj(MprSignalService, manageSignalService)) == 0) {
        return 0;
    }
    ssp->mutex = mprCreateLock();
    ssp->signals = mprAllocZeroed(sizeof(MprSignal*) * MPR_MAX_SIGNALS);
    ssp->standard = mprCreateList(-1, 0);
    return ssp;
}


static void manageSignalService(MprSignalService *ssp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ssp->signals);
        mprMark(ssp->standard);
        mprMark(ssp->mutex);
        /* Don't mark signals elements as it will prevent signal handlers being reclaimed */
    }
}


PUBLIC void mprStopSignalService()
{
    int     i;

    for (i = 1; i < MPR_MAX_SIGNALS; i++) {
        unhookSignal(i);
    }
}


/*
    Signals are hooked on demand and remain till the Mpr is destroyed
 */
static void hookSignal(int signo, MprSignal *sp)
{
    MprSignalService    *ssp;
    struct sigaction    act, old;
    int                 rc;

    assert(0 < signo && signo < MPR_MAX_SIGNALS);
    if ((ssp = MPR->signalService) == 0) {
        return;
    }
    lock(ssp);
    rc = sigaction(signo, 0, &old);
    if (rc == 0 && old.sa_sigaction != signalHandler) {
        sp->sigaction = old.sa_sigaction;
        ssp->prior[signo] = old;
        memset(&act, 0, sizeof(act));
        act.sa_sigaction = signalHandler;
        act.sa_flags |= SA_SIGINFO | SA_RESTART | SA_NOCLDSTOP;
        act.sa_flags &= ~SA_NODEFER;
        sigemptyset(&act.sa_mask);
        if (sigaction(signo, &act, 0) != 0) {
            mprLog("error mpr", 0, "Cannot hook signal %d, errno %d", signo, mprGetOsError());
        }
    }
    unlock(ssp);
}


static void unhookSignal(int signo)
{
    MprSignalService    *ssp;
    struct sigaction    act;
    int                 rc;

    ssp = MPR->signalService;
    lock(ssp);
    rc = sigaction(signo, 0, &act);
    if (rc == 0 && act.sa_sigaction == signalHandler) {
        if (sigaction(signo, &ssp->prior[signo], 0) != 0) {
            mprLog("error mpr", 0, "Cannot unhook signal %d, errno %d", signo, mprGetOsError());
        }
    }
    unlock(ssp);
}


/*
    Actual signal handler - must be async-safe. Do very, very little here. Just set a global flag and wakeup the wait
    service (mprWakeEventService is async-safe). WARNING: Don't put memory allocation, logging or printf here.

    NOTES: The problems here are several fold. The signalHandler may be invoked re-entrantly for different threads for
    the same signal (SIGCHLD). Masked signals are blocked by a single bit and so siginfo will only store one such instance,
    so you cannot use siginfo to get the pid for SIGCHLD. So you really cannot save state here, only set an indication that
    a signal has occurred. MprServiceSignals will then process. Signal handlers must then all be invoked and they must
    test if the signal is valid for them.
 */
static void signalHandler(int signo, siginfo_t *info, void *arg)
{
    MprSignalService    *ssp;
    MprSignalInfo       *ip;
    int                 saveErrno;

    if (signo == SIGINT) {
        /* Fixes command line recall to complete the line */
        printf("\n");
        exit(1);
    }
    if (signo <= 0 || signo >= MPR_MAX_SIGNALS || MPR == 0 || mprIsStopped()) {
        return;
    }
    /*
        Cannot save siginfo, because there is no reliable and scalable way to save siginfo state for multiple threads.
     */
    ssp = MPR->signalService;
    ip = &ssp->info[signo];
    ip->triggered = 1;
    ssp->hasSignals = 1;
    saveErrno = errno;
    mprWakeNotifier();
    errno = saveErrno;
}

/*
    Called by mprServiceEvents after a signal has been received. Create an event and queue on the appropriate dispatcher.
    Run on an MPR thread.
 */
PUBLIC void mprServiceSignals()
{
    MprSignalService    *ssp;
    MprSignal           *sp;
    MprSignalInfo       *ip;
    int                 signo;

    ssp = MPR->signalService;
    if (ssp->hasSignals) {
        lock(ssp);
        ssp->hasSignals = 0;
        for (ip = ssp->info; ip < &ssp->info[MPR_MAX_SIGNALS]; ip++) {
            if (ip->triggered) {
                ip->triggered = 0;
                /*
                    Create events for all registered handlers
                 */
                signo = (int) (ip - ssp->info);
                for (sp = ssp->signals[signo]; sp; sp = sp->next) {
                    mprCreateLocalEvent(sp->dispatcher, "signalEvent", 0, signalEvent, sp, 0);
                }
            }
        }
        unlock(ssp);
    }
}


/*
    Process the signal event. Runs from the dispatcher so signal handlers don't have to be async-safe.
 */
static void signalEvent(MprSignal *sp, MprEvent *event)
{
    assert(sp);
    assert(event);

    if (!sp) {
        return;
    }
    mprDebug("mpr signal", 5, "Received signal %d, flags %x", sp->signo, sp->flags);

    /*
        Return if the handler has been removed since it the event was created
     */
    if (sp->signo == 0) {
        return;
    }
    if (sp->flags & MPR_SIGNAL_BEFORE) {
        (sp->handler)(sp->data, sp);
    }
    if (sp->sigaction && ((void*) sp->sigaction != SIG_IGN && (void*) sp->sigaction != SIG_DFL)) {
        /*
            Call the original (foreign) action handler. Cannot pass on siginfo, because there is no reliable and scalable
            way to save siginfo state when the signalHandler is reentrant across multiple threads.
         */
        (sp->sigaction)(sp->signo, NULL, NULL);
    }
    if (sp->flags & MPR_SIGNAL_AFTER) {
        (sp->handler)(sp->data, sp);
    }
}


static void linkSignalHandler(MprSignal *sp)
{
    MprSignalService    *ssp;

    if ((ssp = MPR->signalService) == 0) {
        return;
    }
    lock(ssp);
    sp->next = ssp->signals[sp->signo];
    ssp->signals[sp->signo] = sp;
    unlock(ssp);
}


static void unlinkSignalHandler(MprSignal *sp)
{
    MprSignalService    *ssp;
    MprSignal           *np, *prev;

    if ((ssp = MPR->signalService) == 0) {
        return;
    }
    lock(ssp);
    for (prev = 0, np = ssp->signals[sp->signo]; np; np = np->next) {
        if (sp == np) {
            if (prev) {
                prev->next = sp->next;
            } else {
                ssp->signals[sp->signo] = sp->next;
            }
            sp->signo = 0;
            break;
        }
        prev = np;
    }
    unlock(ssp);
}


/*
    Add a safe-signal handler. This creates a signal handler that will run from a dispatcher without the
    normal async-safe strictures of normal signal handlers. This manages a next of signal handlers and ensures
    that prior handlers will be called appropriately.
 */
PUBLIC MprSignal *mprAddSignalHandler(int signo, void *handler, void *data, MprDispatcher *dispatcher, int flags)
{
    MprSignal           *sp;

    if (signo <= 0 || signo >= MPR_MAX_SIGNALS) {
        mprLog("error mpr", 0, "Bad signal: %d", signo);
        return 0;
    }
    if (!(flags & MPR_SIGNAL_BEFORE)) {
        flags |= MPR_SIGNAL_AFTER;
    }
    if ((sp = mprAllocObj(MprSignal, manageSignal)) == 0) {
        return 0;
    }
    sp->signo = signo;
    sp->flags = flags;
    sp->handler = handler;
    sp->dispatcher = dispatcher;
    sp->data = data;
    linkSignalHandler(sp);
    hookSignal(signo, sp);
    return sp;
}


static void manageSignal(MprSignal *sp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        /* Don't mark next as it will prevent other signal handlers being reclaimed */
        mprMark(sp->data);
        mprMark(sp->dispatcher);

    } else if (flags & MPR_MANAGE_FREE) {
        if (sp->signo) {
            unlinkSignalHandler(sp);
        }
    }
}


PUBLIC void mprRemoveSignalHandler(MprSignal *sp)
{
    if (sp && sp->signo) {
        unlinkSignalHandler(sp);
    }
}


/*
    Standard signal handler. The following signals are handled:
        SIGINT - immediate exit
        SIGTERM - graceful shutdown
        SIGPIPE - ignore
        SIGXFZ - ignore
        SIGUSR1 - graceful shutdown, then restart
        SIGUSR2 - toggle trace level (Webapp)
        All others - default exit
 */
PUBLIC void mprAddStandardSignals()
{
    MprSignalService    *ssp;

    ssp = MPR->signalService;
    mprAddItem(ssp->standard, mprAddSignalHandler(SIGINT,  standardSignalHandler, 0, 0, MPR_SIGNAL_AFTER));
    mprAddItem(ssp->standard, mprAddSignalHandler(SIGQUIT, standardSignalHandler, 0, 0, MPR_SIGNAL_AFTER));
    mprAddItem(ssp->standard, mprAddSignalHandler(SIGTERM, standardSignalHandler, 0, 0, MPR_SIGNAL_AFTER));
    mprAddItem(ssp->standard, mprAddSignalHandler(SIGPIPE, standardSignalHandler, 0, 0, MPR_SIGNAL_AFTER));
    mprAddItem(ssp->standard, mprAddSignalHandler(SIGUSR1, standardSignalHandler, 0, 0, MPR_SIGNAL_AFTER));
#if SIGXFSZ
    mprAddItem(ssp->standard, mprAddSignalHandler(SIGXFSZ, standardSignalHandler, 0, 0, MPR_SIGNAL_AFTER));
#endif
}


static void standardSignalHandler(void *ignored, MprSignal *sp)
{
    if (sp->signo == SIGTERM) {
        mprShutdown(MPR_EXIT_NORMAL, -1, MPR_EXIT_TIMEOUT);

    } else if (sp->signo == SIGINT || sp->signo == SIGQUIT) {
        /*  Ensure shell input goes to a new line */
        if (isatty(1)) {
            if (write(1, "\n", 1) < 0) {}
        }

        mprShutdown(MPR_EXIT_ABORT, -1, 0);

    } else if (sp->signo == SIGUSR1) {
        /* Graceful shutdown */
        mprShutdown(MPR_EXIT_RESTART, 0, -1);

    } else if (sp->signo == SIGPIPE || sp->signo == SIGXFSZ) {
        /* Ignore */

#if MANUAL_DEBUG
    } else if (sp->signo == SIGSEGV || sp->signo == SIGBUS) {
#if DEBUG_PAUSE
        printf("PAUSED for watson to debug\n");
        sleep(120);
#else
        exit(255);
#endif
#endif
    } else {
        mprShutdown(MPR_EXIT_ABORT, -1, 0);
    }
}





