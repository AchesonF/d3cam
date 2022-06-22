#include "libmpr.h"

/*
    cmd.c - Run external commands

 */

/********************************** Includes **********************************/



/******************************* Forward Declarations *************************/

static int blendEnv(MprCmd *cmd, cchar **env, int flags);
static void closeFiles(MprCmd *cmd);
static void defaultCmdCallback(MprCmd *cmd, int channel, void *data);
static int makeChannel(MprCmd *cmd, int index);
static int makeCmdIO(MprCmd *cmd);
static void manageCmdService(MprCmdService *cmd, int flags);
static void manageCmd(MprCmd *cmd, int flags);
static void prepWinCommand(MprCmd *cmd);
static void prepWinProgram(MprCmd *cmd);
static bool reapCmd(MprCmd *cmd);
static void resetCmd(MprCmd *cmd, bool finalizing);
static int startProcess(MprCmd *cmd);
static void stdinCallback(MprCmd *cmd, MprEvent *event);
static void stdoutCallback(MprCmd *cmd, MprEvent *event);
static void stderrCallback(MprCmd *cmd, MprEvent *event);


/************************************* Code ***********************************/

PUBLIC MprCmdService *mprCreateCmdService()
{
    MprCmdService   *cs;

    if ((cs = (MprCmdService*) mprAllocObj(MprCmd, manageCmdService)) == 0) {
        return 0;
    }
    cs->cmds = mprCreateList(0, 0);
    cs->mutex = mprCreateLock();
    return cs;
}


PUBLIC void mprStopCmdService()
{
    mprClearList(MPR->cmdService->cmds);
}


static void manageCmdService(MprCmdService *cs, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(cs->cmds);
        mprMark(cs->mutex);
    }
}


PUBLIC MprCmd *mprCreateCmd(MprDispatcher *dispatcher)
{
    MprCmd          *cmd;
    MprCmdFile      *files;
    int             i;

    if ((cmd = mprAllocObj(MprCmd, manageCmd)) == 0) {
        return 0;
    }
    cmd->forkCallback = (MprForkCallback) closeFiles;
    cmd->dispatcher = dispatcher ? dispatcher : MPR->dispatcher;
    cmd->status = -1;
    cmd->searchPath = MPR->pathEnv;

    files = cmd->files;
    for (i = 0; i < MPR_CMD_MAX_PIPE; i++) {
        files[i].clientFd = -1;
        files[i].fd = -1;
    }
    cmd->mutex = mprCreateLock();
    return cmd;
}


PUBLIC ssize mprGetActiveCmdCount()
{
    return mprGetListLength(MPR->cmdService->cmds);
}


static void manageCmd(MprCmd *cmd, int flags)
{
    int             i;

    if (flags & MPR_MANAGE_MARK) {
        mprMark(cmd->program);
        mprMark(cmd->makeArgv);
        mprMark(cmd->defaultEnv);
        mprMark(cmd->dir);
        mprMark(cmd->env);
        for (i = 0; i < MPR_CMD_MAX_PIPE; i++) {
            mprMark(cmd->files[i].name);
        }
        for (i = 0; i < MPR_CMD_MAX_PIPE; i++) {
            mprMark(cmd->handlers[i]);
        }
        mprMark(cmd->dispatcher);
        mprMark(cmd->callbackData);
        mprMark(cmd->signal);
        mprMark(cmd->forkData);
        mprMark(cmd->stdoutBuf);
        mprMark(cmd->stderrBuf);
        mprMark(cmd->userData);
        mprMark(cmd->mutex);
        mprMark(cmd->searchPath);
        mprMark(cmd->argv);
        for (i = 0; cmd->argv && i < cmd->argc; i++) {
            mprMark(cmd->argv[i]);
        }

    } else if (flags & MPR_MANAGE_FREE) {
        resetCmd(cmd, 1);
    }
}


static void resetCmd(MprCmd *cmd, bool finalizing)
{
    MprCmdFile  *files;
    MprTicks    mark, timeout;
    int         i;

    assert(cmd);
    files = cmd->files;
    for (i = 0; i < MPR_CMD_MAX_PIPE; i++) {
        if (cmd->handlers[i]) {
            mprDestroyWaitHandler(cmd->handlers[i]);
            cmd->handlers[i] = 0;
        }
        if (files[i].clientFd >= 0) {
            close(files[i].clientFd);
            files[i].clientFd = -1;
        }
        if (files[i].fd >= 0) {
            close(files[i].fd);
            files[i].fd = -1;
        }
    }
    cmd->eofCount = 0;
    cmd->complete = 0;
    cmd->status = -1;

    if (cmd->pid && (!(cmd->flags & MPR_CMD_DETACH) || finalizing)) {
        mprStopCmd(cmd, -1);
        mark = mprGetTicks();
        timeout = 20 * TPS;
        do {
            if (reapCmd(cmd)) {
                break;
            }
            mprYield(0);
            mprNap(1);
        } while (mprGetRemainingTicks(mark, timeout) > 0);
    }
    if (cmd->pid) {
        mprLog("error cmd", 0, "Could not reap command pid %d", cmd->pid);
    }
    if (cmd->signal) {
        mprRemoveSignalHandler(cmd->signal);
        cmd->signal = 0;
    }
}


PUBLIC void mprDestroyCmd(MprCmd *cmd)
{
    assert(cmd);
    resetCmd(cmd, 0);
    mprRemoveItem(MPR->cmdService->cmds, cmd);
}


static void completeCommand(MprCmd *cmd)
{
    /*
        After removing the command from the cmds list, it can be garbage collected if no other reference is retained
     */
    cmd->complete = 1;
    mprDisconnectCmd(cmd);
    mprRemoveItem(MPR->cmdService->cmds, cmd);
}


PUBLIC void mprDisconnectCmd(MprCmd *cmd)
{
    int     i;

    assert(cmd);

    for (i = 0; i < MPR_CMD_MAX_PIPE; i++) {
        if (cmd->handlers[i]) {
            mprDestroyWaitHandler(cmd->handlers[i]);
            cmd->handlers[i] = 0;
        }
    }
}


/*
    Close a command channel. Must be able to be called redundantly.
 */
PUBLIC void mprCloseCmdFd(MprCmd *cmd, int channel)
{
    assert(cmd);
    assert(0 <= channel && channel <= MPR_CMD_MAX_PIPE);

    if (cmd->handlers[channel]) {
        assert(cmd->handlers[channel]->fd >= 0);
        mprDestroyWaitHandler(cmd->handlers[channel]);
        cmd->handlers[channel] = 0;
    }
    if (cmd->files[channel].fd != -1) {
        close(cmd->files[channel].fd);
        cmd->files[channel].fd = -1;

        if (channel != MPR_CMD_STDIN) {
            cmd->eofCount++;
            if (cmd->eofCount >= cmd->requiredEof) {
                if (cmd->pid == 0) {
                    completeCommand(cmd);
                }
            }
        }
    }
}


PUBLIC void mprFinalizeCmd(MprCmd *cmd)
{
    assert(cmd);
    mprCloseCmdFd(cmd, MPR_CMD_STDIN);
}


PUBLIC int mprIsCmdComplete(MprCmd *cmd)
{
    return cmd->complete;
}


PUBLIC int mprRun(MprDispatcher *dispatcher, cchar *command, cchar *input, char **output, char **error, MprTicks timeout)
{
    MprCmd  *cmd;

    cmd = mprCreateCmd(dispatcher);
    return mprRunCmd(cmd, command, NULL, input, output, error, timeout, MPR_CMD_IN  | MPR_CMD_OUT | MPR_CMD_ERR);
}


/*
    Run a simple blocking command. See arg usage below in mprRunCmdV.
 */
PUBLIC int mprRunCmd(MprCmd *cmd, cchar *command, cchar **envp, cchar *in, char **out, char **err, MprTicks timeout,
    int flags)
{
    cchar   **argv;
    int     argc;

    if (cmd == 0 && (cmd = mprCreateCmd(0)) == 0) {
        return MPR_ERR_BAD_STATE;
    }
    if ((argc = mprMakeArgv(command, &argv, 0)) < 0 || argv == 0) {
        return MPR_ERR_BAD_ARGS;
    }
    cmd->makeArgv = argv;
    return mprRunCmdV(cmd, argc, argv, envp, in, out, err, timeout, flags);
}


/*
    This routine runs a command and waits for its completion. Stdoutput and Stderr are returned in *out and *err
    respectively. The command returns the exit status of the command.
    Valid flags are:
        MPR_CMD_NEW_SESSION     Create a new session on Unix
        MPR_CMD_SHOW            Show the commands window on Windows
        MPR_CMD_IN              Connect to stdin
 */
PUBLIC int mprRunCmdV(MprCmd *cmd, int argc, cchar **argv, cchar **envp, cchar *in, char **out, char **err,
    MprTicks timeout, int flags)
{
    ssize   len;
    int     rc, status;

    assert(cmd);
    if (in) {
        flags |= MPR_CMD_IN;
    }
    if (err) {
        *err = 0;
        flags |= MPR_CMD_ERR;
    } else {
        flags &= ~MPR_CMD_ERR;
    }
    if (out) {
        *out = 0;
        flags |= MPR_CMD_OUT;
    } else {
        flags &= ~MPR_CMD_OUT;
    }
    if (flags & MPR_CMD_OUT) {
        cmd->stdoutBuf = mprCreateBuf(ME_BUFSIZE, -1);
    }
    if (flags & MPR_CMD_ERR) {
        cmd->stderrBuf = mprCreateBuf(ME_BUFSIZE, -1);
    }
    mprSetCmdCallback(cmd, defaultCmdCallback, NULL);
    rc = mprStartCmd(cmd, argc, argv, envp, flags);

    if (in) {
        len = slen(in);
        if (mprWriteCmdBlock(cmd, MPR_CMD_STDIN, in, len) != len) {
            *err = sfmt("Cannot write to command %s", cmd->program);
            return MPR_ERR_CANT_WRITE;
        }
    }
    if (cmd->files[MPR_CMD_STDIN].fd >= 0) {
        mprFinalizeCmd(cmd);
    }
    if (rc < 0) {
        if (err) {
            if (!cmd->program && argv && argc) {
                cmd->program = argv[0];
            }
            if (rc == MPR_ERR_CANT_ACCESS) {
                *err = sfmt("Cannot access command %s", cmd->program);
            } else if (rc == MPR_ERR_CANT_OPEN) {
                *err = sfmt("Cannot open standard I/O for command %s", cmd->program);
            } else if (rc == MPR_ERR_CANT_CREATE) {
                *err = sfmt("Cannot create process for %s", cmd->program);
            }
        }
        return rc;
    }
    if (cmd->flags & MPR_CMD_DETACH) {
        return 0;
    }
    if (mprWaitForCmd(cmd, timeout) < 0) {
        mprRemoveItem(MPR->cmdService->cmds, cmd);
        return MPR_ERR_NOT_READY;
    }
    if ((status = mprGetCmdExitStatus(cmd)) < 0) {
        mprRemoveItem(MPR->cmdService->cmds, cmd);
        return MPR_ERR;
    }
    if (err && flags & MPR_CMD_ERR) {
        *err = mprGetBufStart(cmd->stderrBuf);
    }
    if (out && flags & MPR_CMD_OUT) {
        *out = mprGetBufStart(cmd->stdoutBuf);
    }
    return status;
}


static int addCmdHandlers(MprCmd *cmd)
{
    int     stdinFd, stdoutFd, stderrFd;

    stdinFd = cmd->files[MPR_CMD_STDIN].fd;
    stdoutFd = cmd->files[MPR_CMD_STDOUT].fd;
    stderrFd = cmd->files[MPR_CMD_STDERR].fd;

    if (stdinFd >= 0 && cmd->handlers[MPR_CMD_STDIN] == 0) {
        if ((cmd->handlers[MPR_CMD_STDIN] = mprCreateWaitHandler(stdinFd, MPR_WRITABLE, cmd->dispatcher,
                stdinCallback, cmd, MPR_WAIT_NOT_SOCKET)) == 0) {
            return MPR_ERR_CANT_OPEN;
        }
    }
    if (stdoutFd >= 0 && cmd->handlers[MPR_CMD_STDOUT] == 0) {
        if ((cmd->handlers[MPR_CMD_STDOUT] = mprCreateWaitHandler(stdoutFd, MPR_READABLE, cmd->dispatcher,
                stdoutCallback, cmd, MPR_WAIT_NOT_SOCKET)) == 0) {
            return MPR_ERR_CANT_OPEN;
        }
    }
    if (stderrFd >= 0 && cmd->handlers[MPR_CMD_STDERR] == 0) {
        if ((cmd->handlers[MPR_CMD_STDERR] = mprCreateWaitHandler(stderrFd, MPR_READABLE, cmd->dispatcher,
                stderrCallback, cmd, MPR_WAIT_NOT_SOCKET)) == 0) {
            return MPR_ERR_CANT_OPEN;
        }
    }
    return 0;
}


/*
    Env is an array of "KEY=VALUE" strings. Null terminated
    The user must preserve the environment. This module does not clone the environment and uses the supplied reference.
 */
PUBLIC void mprSetCmdDefaultEnv(MprCmd *cmd, cchar **env)
{
    /* WARNING: defaultEnv is not cloned, but is marked */
    cmd->defaultEnv = env;
}


PUBLIC void mprSetCmdSearchPath(MprCmd *cmd, cchar *search)
{
    cmd->searchPath = sclone(search);
}


/*
    Start the command to run (stdIn and stdOut are named from the client's perspective). This is the lower-level way to
    run a command. The caller needs to do code like mprRunCmd() themselves to wait for completion and to send/receive data.
    The routine does not wait. Callers must call mprWaitForCmd to wait for the command to complete.
 */
PUBLIC int mprStartCmd(MprCmd *cmd, int argc, cchar **argv, cchar **envp, int flags)
{
    MprPath     info;
    cchar       *pair;
    int         rc, next, i;

    assert(cmd);
    assert(argv);

    if (argc <= 0 || argv == NULL || argv[0] == NULL) {
        return MPR_ERR_BAD_ARGS;
    }
    resetCmd(cmd, 0);

    cmd->flags = flags;
    cmd->argc = argc;
    cmd->argv = mprAlloc((argc + 1) * sizeof(char*));
    for (i = 0; i < argc; i++) {
        cmd->argv[i] = sclone(argv[i]);
    }
    cmd->argv[i] = 0;

    prepWinProgram(cmd);

    if ((cmd->program = mprSearchPath(cmd->argv[0], MPR_SEARCH_EXE, cmd->searchPath, NULL)) == 0) {
        mprLog("error mpr cmd", 0, "Cannot access %s, errno %d", cmd->argv[0], mprGetOsError());
        return MPR_ERR_CANT_ACCESS;
    }
    if (mprGetPathInfo(cmd->program, &info) == 0 && info.isDir) {
        mprLog("error mpr cmd", 0, "Program \"%s\", is a directory", cmd->program);
        return MPR_ERR_CANT_ACCESS;
    }
    mprLog("info mpr cmd", 6, "Program: %s", cmd->program);
    cmd->argv[0] = cmd->program;

    prepWinCommand(cmd);

    if (envp == 0) {
        envp = cmd->defaultEnv;
    }
    if (blendEnv(cmd, envp, flags) < 0) {
        return MPR_ERR_MEMORY;
    }
    for (i = 0; i < cmd->argc; i++) {
        mprLog("info mpr cmd", 6, "    arg[%d]: %s", i, cmd->argv[i]);
    }
    for (ITERATE_ITEMS(cmd->env, pair, next)) {
        mprLog("info mpr cmd", 6, "    env[%d]: %s", next, pair);
    }

    if (makeCmdIO(cmd) < 0) {
        return MPR_ERR_CANT_OPEN;
    }
    /*
        Determine how many end-of-files will be seen when the child dies
     */
    cmd->requiredEof = 0;
    if (cmd->flags & MPR_CMD_OUT) {
        cmd->requiredEof++;
    }
    if (cmd->flags & MPR_CMD_ERR) {
        cmd->requiredEof++;
    }
    if (addCmdHandlers(cmd) < 0) {
        mprLog("error mpr cmd", 0, "Cannot open command handlers - insufficient I/O handles");
        return MPR_ERR_CANT_OPEN;
    }
    rc = startProcess(cmd);

    cmd->originalPid = cmd->pid;
    mprAddItem(MPR->cmdService->cmds, cmd);

    return rc;
}


static int makeCmdIO(MprCmd *cmd)
{
    int     rc;

    rc = 0;
    if (cmd->flags & MPR_CMD_IN) {
        rc += makeChannel(cmd, MPR_CMD_STDIN);
    }
    if (cmd->flags & MPR_CMD_OUT) {
        rc += makeChannel(cmd, MPR_CMD_STDOUT);
    }
    if (cmd->flags & MPR_CMD_ERR) {
        rc += makeChannel(cmd, MPR_CMD_STDERR);
    }
    return rc;
}


/*
    Stop the command
    WARNING: Called from the finalizer. Must not block or lock.
 */
PUBLIC int mprStopCmd(MprCmd *cmd, int signal)
{
    mprDebug("mpr cmd", 6, "cmd: stop");
    if (signal < 0) {
        signal = SIGTERM;
    }
    cmd->stopped = 1;
    if (cmd->pid) {
        return kill(cmd->pid, signal);
    }
    return 0;
}


/*
    Do non-blocking I/O - except on windows - will block
 */
PUBLIC ssize mprReadCmd(MprCmd *cmd, int channel, char *buf, ssize bufsize)
{
    assert(cmd->files[channel].fd >= 0);
    return read(cmd->files[channel].fd, buf, bufsize);
}


/*
    Do non-blocking I/O - except on windows - will block
 */
PUBLIC ssize mprWriteCmd(MprCmd *cmd, int channel, cchar *buf, ssize bufsize)
{
    if (bufsize <= 0) {
        bufsize = slen(buf);
    }
    return write(cmd->files[channel].fd, (char*) buf, (wsize) bufsize);
}


/*
    Do blocking I/O
 */
PUBLIC ssize mprWriteCmdBlock(MprCmd *cmd, int channel, cchar *buf, ssize bufsize)
{
    MprCmdFile  *file;
    ssize       total, wrote;

    file = &cmd->files[channel];
    /*
        Workaround for Mac OSX that seems to sometimes not full block on writes
     */
    fcntl(file->fd, F_SETFL, fcntl(file->fd, F_GETFL) & ~O_NONBLOCK);
    total = 0;
    while (bufsize > 0) {
        if ((wrote = mprWriteCmd(cmd, channel, buf, bufsize)) < 0) {
            return wrote;
        }
        buf += wrote;
        bufsize -= wrote;
        total += wrote;
    }
    fcntl(file->fd, F_SETFL, fcntl(file->fd, F_GETFL) | O_NONBLOCK);
    return total;
}


PUBLIC bool mprAreCmdEventsEnabled(MprCmd *cmd, int channel)
{
    MprWaitHandler  *wp;

    int mask = (channel == MPR_CMD_STDIN) ? MPR_WRITABLE : MPR_READABLE;
    if (cmd == 0) {
        return 0;
    }
    return ((wp = cmd->handlers[channel]) != 0) && (wp->desiredMask & mask);
}


PUBLIC void mprEnableCmdOutputEvents(MprCmd *cmd, bool on)
{
    int     mask;

    if (cmd == 0) {
        return;
    }
    mask = on ? MPR_READABLE : 0;
    if (cmd->handlers[MPR_CMD_STDOUT]) {
        mprWaitOn(cmd->handlers[MPR_CMD_STDOUT], mask);
    }
    if (cmd->handlers[MPR_CMD_STDERR]) {
        mprWaitOn(cmd->handlers[MPR_CMD_STDERR], mask);
    }
}


PUBLIC void mprEnableCmdEvents(MprCmd *cmd, int channel)
{
    if (cmd == 0) {
        return;
    }
    int mask = (channel == MPR_CMD_STDIN) ? MPR_WRITABLE : MPR_READABLE;
    if (cmd->handlers[channel]) {
        mprWaitOn(cmd->handlers[channel], mask);
    }
}


PUBLIC void mprDisableCmdEvents(MprCmd *cmd, int channel)
{
    if (cmd == 0) {
        return;
    }
    if (cmd->handlers[channel]) {
        mprWaitOn(cmd->handlers[channel], 0);
    }
}

/*
    Wait for a command to complete. Return 0 if the command completed, otherwise it will return MPR_ERR_TIMEOUT.
 */
PUBLIC int mprWaitForCmd(MprCmd *cmd, MprTicks timeout)
{
    MprTicks    expires, remaining, delay;
    int64       dispatcherMark;

    assert(cmd);
    if (timeout < 0) {
        timeout = MAXINT;
    }
    if (mprGetDebugMode()) {
        timeout = MAXINT;
    }
    if (cmd->stopped) {
        timeout = 0;
    }
    expires = mprGetTicks() + timeout;
    remaining = timeout;

    /* Add root to allow callers to use mprRunCmd without first managing the cmd */
    mprAddRoot(cmd);
    dispatcherMark = mprGetEventMark(cmd->dispatcher);

    while (!cmd->complete && remaining > 0) {
        if (mprShouldAbortRequests()) {
            break;
        }
        delay = (cmd->eofCount >= cmd->requiredEof) ? 10 : remaining;
        if (!MPR->eventing) {
            mprServiceEvents(delay, MPR_SERVICE_NO_BLOCK);
            delay = 0;
        }
        mprWaitForEvent(cmd->dispatcher, delay, dispatcherMark);
        remaining = (expires - mprGetTicks());
        dispatcherMark = mprGetEventMark(cmd->dispatcher);
    }
    mprRemoveRoot(cmd);
    if (cmd->pid) {
        return MPR_ERR_TIMEOUT;
    }
    return 0;
}


/*
    Gather the child's exit status.
    WARNING: this may be called with a false-positive, ie. SIGCHLD will get invoked for all process deaths and not just
    when this cmd has completed.
 */
static bool reapCmd(MprCmd *cmd)
{
    if (cmd->pid == 0) {
        return 0;
    }

    int     status, rc;

    status = 0;
    if ((rc = waitpid(cmd->pid, &status, WNOHANG | __WALL)) < 0) {
        mprLog("error mpr cmd", 0, "Waitpid failed for pid %d, errno %d", cmd->pid, errno);

    } else if (rc == cmd->pid) {
        if (!WIFSTOPPED(status)) {
            if (WIFEXITED(status)) {
                cmd->status = WEXITSTATUS(status);
                mprDebug("mpr cmd", 6, "Process exited pid %d, status %d", cmd->pid, cmd->status);
            } else if (WIFSIGNALED(status)) {
                cmd->status = WTERMSIG(status);
            }
            cmd->pid = 0;
            if (cmd->signal) {
                mprRemoveSignalHandler(cmd->signal);
                cmd->signal = 0;
            }
        }
    } else {
        mprDebug("mpr cmd", 6, "Still running pid %d, thread %s", cmd->pid, mprGetCurrentThreadName());
    }

    if (cmd->pid == 0) {
        if (cmd->eofCount >= cmd->requiredEof) {
            completeCommand(cmd);
        }
        mprDebug("mpr cmd", 6, "Process reaped: status %d, pid %d, eof %d / %d", cmd->status, cmd->pid,
                cmd->eofCount, cmd->requiredEof);
        if (cmd->callback) {
            (cmd->callback)(cmd, -1, cmd->callbackData);
        }
    }
    return cmd->pid == 0 ? 1 : 0;
}


/*
    Default callback routine for the mprRunCmd routines. Uses may supply their own callback instead of this routine.
    The callback is run whenever there is I/O to read/write to the CGI gateway.
 */
static void defaultCmdCallback(MprCmd *cmd, int channel, void *data)
{
    MprBuf      *buf;
    ssize       len, space;
    int         errCode;

    /*
        Note: stdin, stdout and stderr are named from the client's perspective
     */
    buf = 0;
    switch (channel) {
    case MPR_CMD_STDIN:
        return;
    case MPR_CMD_STDOUT:
        buf = cmd->stdoutBuf;
        break;
    case MPR_CMD_STDERR:
        buf = cmd->stderrBuf;
        break;
    default:
        /* Child death notification */
        return;
    }
    /*
        Read and aggregate the result into a single string
     */
    space = mprGetBufSpace(buf);
    if (space < (ME_BUFSIZE / 4)) {
        if (mprGrowBuf(buf, ME_BUFSIZE) < 0) {
            mprCloseCmdFd(cmd, channel);
            return;
        }
        space = mprGetBufSpace(buf);
    }
    len = mprReadCmd(cmd, channel, mprGetBufEnd(buf), space);
    errCode = mprGetError();
    if (len <= 0) {
        if (len == 0 || (len < 0 && !(errCode == EAGAIN || errCode == EWOULDBLOCK))) {
            mprCloseCmdFd(cmd, channel);
            return;
        }
    } else {
        mprAdjustBufEnd(buf, len);
    }
    mprAddNullToBuf(buf);
    mprEnableCmdEvents(cmd, channel);
}


static void stdinCallback(MprCmd *cmd, MprEvent *event)
{
    if (cmd->callback && cmd->files[MPR_CMD_STDIN].fd >= 0) {
        (cmd->callback)(cmd, MPR_CMD_STDIN, cmd->callbackData);
    }
}


static void stdoutCallback(MprCmd *cmd, MprEvent *event)
{
    if (cmd->callback && cmd->files[MPR_CMD_STDOUT].fd >= 0) {
        (cmd->callback)(cmd, MPR_CMD_STDOUT, cmd->callbackData);
    }
}


static void stderrCallback(MprCmd *cmd, MprEvent *event)
{
    if (cmd->callback && cmd->files[MPR_CMD_STDERR].fd >= 0) {
        (cmd->callback)(cmd, MPR_CMD_STDERR, cmd->callbackData);
    }
}


PUBLIC void mprSetCmdCallback(MprCmd *cmd, MprCmdProc proc, void *data)
{
    cmd->callback = proc;
    cmd->callbackData = data;
}


PUBLIC int mprGetCmdExitStatus(MprCmd *cmd)
{
    assert(cmd);

    if (cmd->pid == 0) {
        return cmd->status;
    }
    return MPR_ERR_NOT_READY;
}


PUBLIC bool mprIsCmdRunning(MprCmd *cmd)
{
    return cmd->pid > 0;
}


PUBLIC int mprGetCmdFd(MprCmd *cmd, int channel)
{
    return cmd->files[channel].fd;
}


PUBLIC MprBuf *mprGetCmdBuf(MprCmd *cmd, int channel)
{
    return (channel == MPR_CMD_STDOUT) ? cmd->stdoutBuf : cmd->stderrBuf;
}


PUBLIC void mprSetCmdDir(MprCmd *cmd, cchar *dir)
{
    assert(dir && *dir);
    cmd->dir = sclone(dir);
}

/*
    Match two environment keys up to the '='
 */
static bool matchEnvKey(cchar *s1, cchar *s2)
{
    for (; *s1 && *s2; s1++, s2++) {
        if (*s1 != *s2) {
            break;
        } else if (*s1 == '=') {
            return 1;
        }
    }
    return 0;
}


static int blendEnv(MprCmd *cmd, cchar **env, int flags)
{
    cchar       **ep, *prior;
    int         next;

    cmd->env = 0;

    if ((cmd->env = mprCreateList(128, MPR_LIST_STATIC_VALUES | MPR_LIST_STABLE)) == 0) {
        return MPR_ERR_MEMORY;
    }

    /*
        Add prior environment to the list
     */
    if (!(flags & MPR_CMD_EXACT_ENV)) {
        for (ep = (cchar**) environ; ep && *ep; ep++) {
            mprAddItem(cmd->env, *ep);
        }
    }

    /*
        Add new env keys. Detect and overwrite duplicates
     */
    for (ep = env; ep && *ep; ep++) {
        prior = 0;
        for (ITERATE_ITEMS(cmd->env, prior, next)) {
            if (matchEnvKey(*ep, prior)) {
                mprSetItem(cmd->env, next - 1, *ep);
                break;
            }
        }
        if (prior == 0) {
            mprAddItem(cmd->env, *ep);
        }
    }

    mprAddItem(cmd->env, NULL);
    return 0;
}

/*
    Determine the windows program to invoke.
    Support UNIX style "#!/program" bang directives on windows.
    Also supports ".cmd" and ".bat" alternatives.
 */
static void prepWinProgram(MprCmd *cmd)
{
}


/*
    Create a single string command to invoke - windows doesn't support exec(argv)
 */
static void prepWinCommand(MprCmd *cmd)
{
}

static int makeChannel(MprCmd *cmd, int index)
{
    MprCmdFile      *file;
    int             fds[2];

    file = &cmd->files[index];

    if (pipe(fds) < 0) {
        mprLog("error mpr cmd", 0, "Cannot create stdio pipes. Err %d", mprGetOsError());
        return MPR_ERR_CANT_CREATE;
    }
    if (index == MPR_CMD_STDIN) {
        file->clientFd = fds[0];        /* read fd */
        file->fd = fds[1];              /* write fd */
    } else {
        file->clientFd = fds[1];        /* write fd */
        file->fd = fds[0];              /* read fd */
    }
    fcntl(file->fd, F_SETFL, fcntl(file->fd, F_GETFL) | O_NONBLOCK);
    return 0;
}

/*
    Called on the cmd dispatcher in response to a child death
 */
static void cmdChildDeath(MprCmd *cmd, MprSignal *sp)
{
    reapCmd(cmd);
}


static int startProcess(MprCmd *cmd)
{
    MprCmdFile      *files;
    int             i;

    files = cmd->files;
    if (!cmd->signal) {
        cmd->signal = mprAddSignalHandler(SIGCHLD, cmdChildDeath, cmd, cmd->dispatcher, MPR_SIGNAL_BEFORE);
    }
    /*
        Create the child
     */
    cmd->pid = fork();

    if (cmd->pid < 0) {
        mprLog("error mpr cmd", 0, "Cannot fork a new process to run %s, errno %d", cmd->program, mprGetOsError());
        return MPR_ERR_CANT_INITIALIZE;

    } else if (cmd->pid == 0) {
        /*
            Child
         */
        umask(022);
        if (cmd->flags & MPR_CMD_NEW_SESSION) {
            setsid();
        }
        if (cmd->dir) {
            if (chdir(cmd->dir) < 0) {
                mprLog("error mpr cmd", 0, "Cannot change directory to %s", cmd->dir);
                return MPR_ERR_CANT_INITIALIZE;
            }
        }
        if (cmd->flags & MPR_CMD_IN) {
            if (files[MPR_CMD_STDIN].clientFd >= 0) {
                dup2(files[MPR_CMD_STDIN].clientFd, 0);
                close(files[MPR_CMD_STDIN].fd);
            } else {
                close(0);
            }
        }
        if (cmd->flags & MPR_CMD_OUT) {
            if (files[MPR_CMD_STDOUT].clientFd >= 0) {
                dup2(files[MPR_CMD_STDOUT].clientFd, 1);
                close(files[MPR_CMD_STDOUT].fd);
            } else {
                close(1);
            }
        }
        if (cmd->flags & MPR_CMD_ERR) {
            if (files[MPR_CMD_STDERR].clientFd >= 0) {
                dup2(files[MPR_CMD_STDERR].clientFd, 2);
                close(files[MPR_CMD_STDERR].fd);
            } else {
                close(2);
            }
        }
        cmd->forkCallback(cmd->forkData);
        if (cmd->env) {
            (void) execve(cmd->program, (char**) cmd->argv, (char**) &cmd->env->items[0]);
        } else {
            (void) execv(cmd->program, (char**) cmd->argv);
        }
        /*
            Use _exit to avoid flushing I/O any other I/O.
         */
        _exit(-(MPR_ERR_CANT_INITIALIZE));

    } else {
        /*
            Close the client handles
         */
        for (i = 0; i < MPR_CMD_MAX_PIPE; i++) {
            if (files[i].clientFd >= 0) {
                close(files[i].clientFd);
                files[i].clientFd = -1;
            }
        }
    }
    return 0;
}


static void closeFiles(MprCmd *cmd)
{
    int     i;
    for (i = 3; i < MPR_MAX_FILE; i++) {
        close(i);
    }
}



