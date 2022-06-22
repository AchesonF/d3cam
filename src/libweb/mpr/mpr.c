#include "libmpr.h"

/*
    mpr.c - Multithreaded Portable Runtime (MPR). Initialization, start/stop and control of the MPR.

 */

/********************************** Includes **********************************/



/*********************************** Locals ***********************************/
/*
    Define an illegal exit status value
 */
#define NO_STATUS   0x100000

static int mprExitStatus;
static int mprState;

/**************************** Forward Declarations ****************************/

static int initStdio(Mpr *mpr, MprFileSystem *fs);
static void manageMpr(Mpr *mpr, int flags);
static void serviceEventsThread(void *data, MprThread *tp);
static void setArgs(Mpr *mpr, int argc, char **argv);

/************************************* Code ***********************************/
/*
    Create and initialize the MPR service.
 */
PUBLIC Mpr *mprCreate(int argc, char **argv, int flags)
{
    Mpr             *mpr;
    MprFileSystem   *fs;

    srand((uint) time(NULL));

    if (flags & MPR_DAEMON) {
        mprDaemon();
    }
    mprAtomicOpen();
    if ((mpr = mprCreateMemService((MprManager) manageMpr, flags)) == 0) {
        assert(mpr);
        return 0;
    }
    mpr->flags = flags;
    mpr->start = mprGetTime();
    mpr->exitStrategy = 0;
    mpr->emptyString = sclone("");
    mpr->oneString = sclone("1");
    mpr->idleCallback = mprServicesAreIdle;
    mpr->mimeTypes = mprCreateMimeTypes(NULL);
    mpr->terminators = mprCreateList(0, MPR_LIST_STATIC_VALUES);
    mpr->keys = mprCreateHash(0, 0);
    mpr->verifySsl = 1;
    mpr->fileSystems = mprCreateList(0, 0);

    fs = mprCreateDiskFileSystem("/");
    mprAddFileSystem(fs);
    initStdio(mpr, fs);

    setArgs(mpr, argc, argv);
    mprCreateOsService();
    mprCreateTimeService();
    mpr->mutex = mprCreateLock();
    mpr->spin = mprCreateSpinLock();

    mprCreateLogService();
    mprCreateCacheService();

    mpr->signalService = mprCreateSignalService();
    mpr->threadService = mprCreateThreadService();
    mpr->moduleService = mprCreateModuleService();
    mpr->eventService = mprCreateEventService();
    mpr->cmdService = mprCreateCmdService();
    mpr->workerService = mprCreateWorkerService();
    mpr->waitService = mprCreateWaitService();
    mpr->socketService = mprCreateSocketService();
    mpr->pathEnv = sclone(getenv("PATH"));
    mpr->cond = mprCreateCond();
    mpr->stopCond = mprCreateCond();

    mpr->dispatcher = mprCreateDispatcher("main", 0);
    mpr->nonBlock = mprCreateDispatcher("nonblock", 0);
    mprSetDispatcherImmediate(mpr->nonBlock);

    if (flags & MPR_USER_EVENTS_THREAD) {
        if (!(flags & MPR_NO_WINDOW)) {
            /* Used by apps that need to use FindWindow after calling mprCreate() (webappMonitor) */
            mprSetWindowsThread(0);
        }
    } else {
        mprStartEventsThread();
    }
    if (!(flags & MPR_DELAY_GC_THREAD)) {
        mprStartGCService();
    }
    mprState = MPR_CREATED;
    mprExitStatus = NO_STATUS;

    if (MPR->hasError || mprHasMemError()) {
        return 0;
    }
    return mpr;
}


static void manageMpr(Mpr *mpr, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(mpr->logFile);
        mprMark(mpr->mimeTypes);
        mprMark(mpr->timeTokens);
        mprMark(mpr->keys);
        mprMark(mpr->stdError);
        mprMark(mpr->stdInput);
        mprMark(mpr->stdOutput);
        mprMark(mpr->appPath);
        mprMark(mpr->appDir);
        /*
            Argv will do a single allocation into argv == argBuf. May reallocate the program name in argv[0]
         */
        mprMark(mpr->argv);
        if (mpr->argv) {
            mprMark(mpr->argv[0]);
        }
        mprMark(mpr->logPath);
        mprMark(mpr->pathEnv);
        mprMark(mpr->name);
        mprMark(mpr->title);
        mprMark(mpr->version);
        mprMark(mpr->domainName);
        mprMark(mpr->hostName);
        mprMark(mpr->ip);
        mprMark(mpr->serverName);
        mprMark(mpr->cmdService);
        mprMark(mpr->eventService);
        mprMark(mpr->fileSystems);
        mprMark(mpr->moduleService);
        mprMark(mpr->osService);
        mprMark(mpr->signalService);
        mprMark(mpr->socketService);
        mprMark(mpr->threadService);
        mprMark(mpr->workerService);
        mprMark(mpr->waitService);
        mprMark(mpr->dispatcher);
        mprMark(mpr->nonBlock);
        mprMark(mpr->webappService);
        mprMark(mpr->ediService);
        mprMark(mpr->ejsService);
        mprMark(mpr->espService);
        mprMark(mpr->httpService);
        mprMark(mpr->terminators);
        mprMark(mpr->mutex);
        mprMark(mpr->spin);
        mprMark(mpr->cond);
        mprMark(mpr->romfs);
        mprMark(mpr->stopCond);
        mprMark(mpr->emptyString);
        mprMark(mpr->oneString);
    }
}


static int initStdio(Mpr *mpr, MprFileSystem *fs)
{
    if ((mpr->stdError = mprAllocStruct(MprFile)) == 0) {
        return MPR_ERR_MEMORY;
    }
    mprSetName(mpr->stdError, "stderr");
    mpr->stdError->fd = 2;
    mpr->stdError->fileSystem = fs;
    mpr->stdError->mode = O_WRONLY;

    if ((mpr->stdInput = mprAllocStruct(MprFile)) == 0) {
        return MPR_ERR_MEMORY;
    }
    mprSetName(mpr->stdInput, "stdin");
    mpr->stdInput->fd = 0;
    mpr->stdInput->fileSystem = fs;
    mpr->stdInput->mode = O_RDONLY;

    if ((mpr->stdOutput = mprAllocStruct(MprFile)) == 0) {
        return MPR_ERR_MEMORY;
    }
    mprSetName(mpr->stdOutput, "stdout");
    mpr->stdOutput->fd = 1;
    mpr->stdOutput->fileSystem = fs;
    mpr->stdOutput->mode = O_WRONLY;
    return 0;
}


/*
    The monitor event is invoked from mprShutdown() for graceful shutdowns if the application has requests still running.
    This event monitors the application to see when it becomes is idle.
    WARNING: this races with other threads
 */
static void shutdownMonitor(void *data, MprEvent *event)
{
    MprTicks        remaining;

    if (mprIsIdle(1)) {
        if (mprState <= MPR_STOPPING) {
            mprLog("info mpr", 2, "Shutdown proceeding, system is idle");
            mprState = MPR_STOPPED;
        }
        return;
    }
    remaining = mprGetRemainingTicks(MPR->shutdownStarted, MPR->exitTimeout);
    if (remaining <= 0) {
        if (MPR->exitStrategy & MPR_EXIT_SAFE && mprCancelShutdown()) {
            mprLog("warn mpr", 2, "Shutdown cancelled due to continuing requests");
        } else {
            mprLog("warn mpr", 6, "Timeout while waiting for requests to complete");
            if (mprState <= MPR_STOPPING) {
                mprState = MPR_STOPPED;
            }
        }
    } else {
        if (!mprGetDebugMode()) {
            mprLog("info mpr", 2, "Waiting for requests to complete, %lld secs remaining ...", remaining / TPS);
        }
        mprRescheduleEvent(event, 1000);
    }
}


/*
    Start shutdown of the Mpr. This sets the state to stopping and invokes the shutdownMonitor. This is done for
    all shutdown strategies regardless. Immediate shutdowns must still give threads some time to exit.
    This routine does no destructive actions.
    WARNING: this races with other threads.
 */
PUBLIC void mprShutdown(int how, int exitStatus, MprTicks timeout)
{
    MprTerminator   terminator;
    int             next;

    mprGlobalLock();
    if (mprState >= MPR_STOPPING) {
        mprGlobalUnlock();
        return;
    }
    mprState = MPR_STOPPING;
    mprSignalMultiCond(MPR->stopCond);
    mprGlobalUnlock();

    MPR->exitStrategy = how;
    mprExitStatus = exitStatus;
    MPR->exitTimeout = (timeout >= 0) ? timeout : MPR->exitTimeout;
    if (mprGetDebugMode()) {
        MPR->exitTimeout = MPR_MAX_TIMEOUT;
    }
    MPR->shutdownStarted = mprGetTicks();

    if (how & MPR_EXIT_ABORT) {
        if (how & MPR_EXIT_RESTART) {
            mprLog("info mpr", 3, "Abort with restart.");
            mprRestart();
        } else {
            mprLog("info mpr", 3, "Abortive exit.");
            exit(exitStatus);
        }
        /* No continue */
    }

    if (!mprIsIdle(0)) {
        mprLog("info mpr", 6, "Application exit, waiting for existing requests to complete.");
        mprCreateTimerEvent(NULL, "shutdownMonitor", 0, shutdownMonitor, 0, MPR_EVENT_QUICK);
    }
    mprWakeDispatchers();
    mprWakeNotifier();

    /*
        Note: terminators must take not destructive actions for the MPR_STOPPED state
     */
    for (ITERATE_ITEMS(MPR->terminators, terminator, next)) {
        (terminator)(mprState, how, mprExitStatus & ~NO_STATUS);
    }
}


PUBLIC bool mprCancelShutdown()
{
    mprGlobalLock();
    if (mprState == MPR_STOPPING) {
        mprState = MPR_STARTED;
        mprGlobalUnlock();
        return 1;
    }
    mprGlobalUnlock();
    return 0;
}


/*
    Destroy the Mpr and all services
    If the application has a user events thread and mprShutdown was called, then we will come here when already idle.
    This routine will call service terminators to allow them to shutdown their services in an orderly manner
 */
PUBLIC bool mprDestroy()
{
    MprTerminator   terminator;
    MprTicks        timeout;
    int             i, next;

    if (mprState < MPR_STOPPING) {
        mprShutdown(MPR->exitStrategy, mprExitStatus, MPR->exitTimeout);
    }
    mprGC(MPR_GC_FORCE | MPR_GC_COMPLETE);

    timeout = MPR->exitTimeout;
    if (MPR->shutdownStarted) {
        timeout -= (mprGetTicks() - MPR->shutdownStarted);
    }
    /*
        Wait for events thread to exit and the app to become idle
     */
    while (!mprIsIdle(0) || MPR->eventing) {
        mprWakeNotifier();
        mprWaitForCond(MPR->cond, 10);
        if (mprGetRemainingTicks(MPR->shutdownStarted, timeout) <= 0) {
            break;
        }
    }
    if (!mprIsIdle(0) || MPR->eventing) {
        if (MPR->exitStrategy & MPR_EXIT_SAFE) {
            /* Note: Pending outside events will pause GC which will make mprIsIdle return false */
            mprLog("warn mpr", 2, "Cancel termination due to continuing requests, application resumed.");
            mprCancelShutdown();
        } else if (MPR->exitTimeout > 0) {
            /* If a non-zero graceful timeout applies, always exit with non-zero status */
            exit(mprExitStatus != NO_STATUS ? mprExitStatus : 1);
        } else {
            exit(mprExitStatus & ~NO_STATUS);
        }
        return 0;
    }
    mprGlobalLock();
    if (mprState == MPR_STARTED) {
        mprGlobalUnlock();
        /* User cancelled shutdown */
        return 0;
    }
    /*
        Point of no return
     */
    mprState = MPR_DESTROYING;
    mprGlobalUnlock();

    for (ITERATE_ITEMS(MPR->terminators, terminator, next)) {
        (terminator)(mprState, MPR->exitStrategy, mprExitStatus & ~NO_STATUS);
    }
    mprStopWorkers();
    mprStopCmdService();
    mprStopModuleService();
    mprStopEventService();
    mprStopThreadService();
    mprStopWaitService();

    /*
        Run GC to finalize all memory until we are not freeing any memory. This IS deterministic.
     */
    for (i = 0; i < 25; i++) {
        if (mprGC(MPR_GC_FORCE | MPR_GC_COMPLETE) == 0) {
            break;
        }
    }
    mprState = MPR_DESTROYED;

    if (MPR->exitStrategy & MPR_EXIT_RESTART) {
        mprLog("info mpr", 2, "Restarting");
    }
    mprStopModuleService();
    mprStopSignalService();
    mprStopGCService();
    mprStopOsService();

    if (MPR->exitStrategy & MPR_EXIT_RESTART) {
        mprRestart();
    }
    mprDestroyMemService();
    return 1;
}


static void setArgs(Mpr *mpr, int argc, char **argv)
{
    cchar   *appPath;

    if (argv) {
        mpr->argv = mprAllocZeroed(sizeof(void*) * (argc + 1));
        memcpy((char*) mpr->argv, argv, sizeof(void*) * argc);

        mpr->argc = argc;

        appPath = mprGetAppPath();
        if (smatch(appPath, ".")) {
            mpr->argv[0] = sclone(CONFIG_NAME);
        } else if (mprIsPathAbs(mpr->argv[0])) {
            mpr->argv[0] = sclone(mprGetAppPath());
        } else {
            mpr->argv[0] = mprGetAppPath();
        }
    } else {
        mpr->name = sclone(CONFIG_NAME);
        mpr->argv = mprAllocZeroed(2 * sizeof(void*));
        mpr->argv[0] = mpr->name;
        mpr->argc = 0;
    }
    mpr->name = mprTrimPathExt(mprGetPathBase(mpr->argv[0]));
    mpr->title = sfmt("%s %s", stitle(CONFIG_COMPANY), stitle(mpr->name));
    mpr->version = sclone(CONFIG_VERSION);
}


PUBLIC int mprGetExitStatus()
{
    return mprExitStatus & ~NO_STATUS;
}


PUBLIC void mprSetExitStatus(int status)
{
    mprExitStatus = status;
}


PUBLIC void mprAddTerminator(MprTerminator terminator)
{
    mprAddItem(MPR->terminators, terminator);
}


PUBLIC void mprRestart()
{
    int     i;

    for (i = 3; i < MPR_MAX_FILE; i++) {
        close(i);
    }
    execv(MPR->argv[0], (char*const*) MPR->argv);

    /*
        Last-ditch trace. Can only use stdout. Logging may be closed.
     */
    printf("Failed to exec errno %d: ", errno);
    for (i = 0; MPR->argv[i]; i++) {
        printf("%s ", MPR->argv[i]);
    }
    printf("\n");
}


PUBLIC int mprStart()
{
    int     rc;

    rc = mprStartOsService();
    rc += mprStartModuleService();
    rc += mprStartWorkerService();
    if (rc != 0) {
        mprLog("error mpr", 0, "Cannot start MPR services");
        return MPR_ERR_CANT_INITIALIZE;
    }
    mprState = MPR_STARTED;
    return 0;
}


PUBLIC int mprStartEventsThread()
{
    MprThread   *tp;
    MprTicks    timeout;

    if ((tp = mprCreateThread("events", serviceEventsThread, NULL, 0)) == 0) {
        MPR->hasError = 1;
    } else {
        MPR->threadService->eventsThread = tp;
        MPR->cond = mprCreateCond();
        mprStartThread(tp);
        timeout = mprGetDebugMode() ? MPR_MAX_TIMEOUT : MPR_TIMEOUT_START_TASK;
        mprWaitForCond(MPR->cond, timeout);
    }
    return 0;
}


static void serviceEventsThread(void *data, MprThread *tp)
{
    mprLog("info mpr", 2, "Service thread started");
    mprSetWindowsThread(tp);
    mprSignalCond(MPR->cond);
    mprServiceEvents(-1, 0);
    mprRescheduleDispatcher(MPR->dispatcher);
}


/*
    Services should call this to determine if they should accept new services
 */
PUBLIC bool mprShouldAbortRequests()
{
    return mprIsStopped();
}


PUBLIC bool mprShouldDenyNewRequests()
{
    return mprIsStopping();
}


PUBLIC bool mprIsStopping()
{
    return mprState >= MPR_STOPPING;
}


PUBLIC bool mprIsStopped()
{
    return mprState >= MPR_STOPPED;
}


PUBLIC bool mprIsDestroying()
{
    return mprState >= MPR_DESTROYING;
}


PUBLIC bool mprIsDestroyed()
{
    return mprState >= MPR_DESTROYED;
}


PUBLIC int mprGetState()
{
    return mprState;
}


PUBLIC void mprSetState(int state)
{
    // OPT - Don't need lock around simple assignment
    mprGlobalLock();
    mprState = state;
    mprGlobalUnlock();
}



/*
    Test if the Mpr services are idle. Use mprIsIdle to determine if the entire process is idle.
    Note: this counts worker threads but ignores other threads created via mprCreateThread
 */
PUBLIC bool mprServicesAreIdle(bool traceRequests)
{
    bool    idle;

    /*
        Only test top level services. Dispatchers may have timers scheduled, but that is okay. If not, users can install
        their own idleCallback.
     */
    idle = mprGetBusyWorkerCount() == 0 && mprGetActiveCmdCount() == 0;
    if (!idle && traceRequests) {
        mprDebug("mpr", 3, "Services are not idle: cmds %d, busy threads %d, eventing %d",
            mprGetListLength(MPR->cmdService->cmds), mprGetListLength(MPR->workerService->busyThreads), MPR->eventing);
    }
    return idle;
}


PUBLIC bool mprIsIdle(bool traceRequests)
{
    return (MPR->idleCallback)(traceRequests);
}


/*
    Parse the args and return the count of args. If argv is NULL, the args are parsed read-only. If argv is set,
    then the args will be extracted, back-quotes removed and argv will be set to point to all the args.
    NOTE: this routine does not allocate.
 */
PUBLIC int mprParseArgs(char *args, char **argv, int maxArgc)
{
    char    *dest, *src, *start;
    int     quote, argc;

    /*
        Example     "showColors" red 'light blue' "yellow white" 'Cannot \"render\"'
        Becomes:    ["showColors", "red", "light blue", "yellow white", "Cannot \"render\""]
     */
    for (argc = 0, src = args; src && *src != '\0' && argc < maxArgc; argc++) {
        while (isspace((uchar) *src)) {
            src++;
        }
        if (*src == '\0')  {
            break;
        }
        start = dest = src;
        if (*src == '"' || *src == '\'') {
            quote = *src;
            src++;
            dest++;
        } else {
            quote = 0;
        }
        if (argv) {
            argv[argc] = src;
        }
        while (*src) {
            if (*src == '\\' && src[1] && (src[1] == '\\' || src[1] == '"' || src[1] == '\'')) {
                src++;
            } else {
                if (quote) {
                    if (*src == quote && !(src > start && src[-1] == '\\')) {
                        break;
                    }
                } else if (*src == ' ') {
                    break;
                }
            }
            if (argv) {
                *dest++ = *src;
            }
            src++;
        }
        if (*src != '\0') {
            src++;
        }
        if (argv) {
            *dest++ = '\0';
        }
    }
    return argc;
}


/*
    Make an argv array. All args are in a single memory block of which argv points to the start.
    Set MPR_ARGV_ARGS_ONLY if not passing in a program name.
    Always returns and argv[0] reserved for the program name or empty string.  First arg starts at argv[1].
 */
PUBLIC int mprMakeArgv(cchar *command, cchar ***argvp, int flags)
{
    char    **argv, *vector, *args;
    ssize   len;
    int     argc;

    assert(command);
    if (!command) {
        return MPR_ERR_BAD_ARGS;
    }
    /*
        Allocate one vector for argv and the actual args themselves
     */
    len = slen(command) + 1;
    argc = mprParseArgs((char*) command, NULL, INT_MAX);
    if (flags & MPR_ARGV_ARGS_ONLY) {
        argc++;
    }
    if ((vector = (char*) mprAlloc(((argc + 1) * sizeof(char*)) + len)) == 0) {
        assert(!MPR_ERR_MEMORY);
        return MPR_ERR_MEMORY;
    }
    args = &vector[(argc + 1) * sizeof(char*)];
    strcpy(args, command);
    argv = (char**) vector;

    if (flags & MPR_ARGV_ARGS_ONLY) {
        mprParseArgs(args, &argv[1], argc);
        argv[0] = MPR->emptyString;
    } else {
        mprParseArgs(args, argv, argc);
    }
    argv[argc] = 0;
    *argvp = (cchar**) argv;
    return argc;
}


PUBLIC MprIdleCallback mprSetIdleCallback(MprIdleCallback idleCallback)
{
    MprIdleCallback old;

    old = MPR->idleCallback;
    MPR->idleCallback = idleCallback;
    return old;
}


PUBLIC int mprSetAppName(cchar *name, cchar *title, cchar *version)
{
    char    *cp;

    if (name) {
        if ((MPR->name = (char*) mprGetPathBase(name)) == 0) {
            return MPR_ERR_CANT_ALLOCATE;
        }
        if ((cp = strrchr(MPR->name, '.')) != 0) {
            *cp = '\0';
        }
    }
    if (title) {
        if ((MPR->title = sclone(title)) == 0) {
            return MPR_ERR_CANT_ALLOCATE;
        }
    }
    if (version) {
        if ((MPR->version = sclone(version)) == 0) {
            return MPR_ERR_CANT_ALLOCATE;
        }
    }
    return 0;
}


PUBLIC cchar *mprGetAppName()
{
    return MPR->name;
}


PUBLIC cchar *mprGetAppTitle()
{
    return MPR->title;
}


/*
    Full host name with domain. E.g. "server.domain.com"
 */
PUBLIC void mprSetHostName(cchar *s)
{
    MPR->hostName = sclone(s);
}


/*
    Return the fully qualified host name
 */
PUBLIC cchar *mprGetHostName()
{
    return MPR->hostName;
}


/*
    Server name portion (no domain name)
 */
PUBLIC void mprSetServerName(cchar *s)
{
    MPR->serverName = sclone(s);
}


PUBLIC cchar *mprGetServerName()
{
    return MPR->serverName;
}


PUBLIC void mprSetDomainName(cchar *s)
{
    MPR->domainName = sclone(s);
}


PUBLIC cchar *mprGetDomainName()
{
    return MPR->domainName;
}


/*
    Set the IP address
 */
PUBLIC void mprSetIpAddr(cchar *s)
{
    MPR->ip = sclone(s);
}


/*
    Return the IP address
 */
PUBLIC cchar *mprGetIpAddr()
{
    return MPR->ip;
}


PUBLIC cchar *mprGetAppVersion()
{
    return MPR->version;
}


PUBLIC bool mprGetDebugMode()
{
    return MPR->debugMode;
}


PUBLIC void mprSetDebugMode(bool on)
{
    MPR->debugMode = on;
}


PUBLIC MprDispatcher *mprGetDispatcher()
{
    return MPR->dispatcher;
}


PUBLIC MprDispatcher *mprGetNonBlockDispatcher()
{
    return MPR->nonBlock;
}


PUBLIC cchar *mprCopyright(void)
{
    return  "Copyright (c) ht. All Rights Reserved.";
}


PUBLIC int mprGetEndian()
{
    char    *probe;
    int     test;

    test = 1;
    probe = (char*) &test;
    return (*probe == 1) ? ME_LITTLE_ENDIAN : ME_BIG_ENDIAN;
}


PUBLIC char *mprEmptyString()
{
    return MPR->emptyString;
}


PUBLIC void mprSetEnv(cchar *key, cchar *value)
{
    setenv(key, value, 1);

    if (scaselessmatch(key, "PATH")) {
        MPR->pathEnv = sclone(value);
    }
}


PUBLIC void mprSetExitTimeout(MprTicks timeout)
{
    MPR->exitTimeout = timeout;
}


PUBLIC void mprNop(void *ptr) {
}


/*
    This should not be called after mprCreate() as it will orphan the GC and events threads.
 */
PUBLIC int mprDaemon()
{
    struct sigaction    act, old;
    int                 i, pid, status;

    /*
        Ignore child death signals
     */
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = (void (*)(int, siginfo_t*, void*)) SIG_DFL;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_NOCLDSTOP | SA_RESTART | SA_SIGINFO;

    if (sigaction(SIGCHLD, &act, &old) < 0) {
        fprintf(stderr, "Cannot initialize signals");
        return MPR_ERR_BAD_STATE;
    }
    /*
        Close stdio so shells won't hang
     */
    for (i = 0; i < 3; i++) {
        close(i);
    }
    /*
        Fork twice to get a free child with no parent
     */
    if ((pid = fork()) < 0) {
        fprintf(stderr, "Fork failed for background operation");
        return MPR_ERR;

    } else if (pid == 0) {
        /* Child of first fork */
        if ((pid = fork()) < 0) {
            fprintf(stderr, "Second fork failed");
            exit(127);

        } else if (pid > 0) {
            /* Parent of second child -- must exit. This is waited for below */
            exit(0);
        }

        /*
            This is the real child that will continue as a daemon
         */
        setsid();
        if (sigaction(SIGCHLD, &old, 0) < 0) {
            fprintf(stderr, "Cannot restore signals");
            return MPR_ERR_BAD_STATE;
        }
        return 0;
    }

    /*
        Original (parent) process waits for first child here. Must get child death notification with a successful exit status.
     */
    while (waitpid(pid, &status, 0) != pid) {
        if (errno == EINTR) {
            mprSleep(100);
            continue;
        }
        fprintf(stderr, "Cannot wait for daemon parent.");
        exit(0);
    }
    if (WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Daemon parent had bad exit status.");
        exit(0);
    }
    if (sigaction(SIGCHLD, &old, 0) < 0) {
        fprintf(stderr, "Cannot restore signals");
        return MPR_ERR_BAD_STATE;
    }
    exit(0);
}


PUBLIC void mprSetKey(cchar *key, void *value)
{
    mprAddKey(MPR->keys, key, value);
}


PUBLIC void *mprGetKey(cchar *key)
{
    return mprLookupKey(MPR->keys, key);
}




