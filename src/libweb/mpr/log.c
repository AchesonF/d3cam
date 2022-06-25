#include "libmpr.h"

/**
    log.c - Multithreaded Portable Runtime (MPR) Logging and error reporting.
 */

/********************************** Includes **********************************/



/********************************** Defines ***********************************/

#ifndef ME_MAX_LOGLINE
    #define ME_MAX_LOGLINE 8192           /* Max size of a log line */
#endif

/********************************** Forwards **********************************/

static void logOutput(cchar *tags, int level, cchar *msg);

/************************************ Code ************************************/
/*
    Put first in file so it is easy to locate in a debugger
 */
PUBLIC void mprBreakpoint()
{
#if DEBUG_PAUSE
    {
        static int  paused = 1;
        int         i;
        printf("Paused to permit debugger to attach - will awake in 2 minutes\n");
        fflush(stdout);
        for (i = 0; i < 120 && paused; i++) {
            mprNap(1000);
        }
    }
#endif
}


PUBLIC void mprCreateLogService()
{
    MPR->logFile = MPR->stdError;
}


PUBLIC int mprStartLogging(cchar *logSpec, int flags)
{
    MprFile     *file;
    char        *levelSpec, *path;
    int         level;

    if (logSpec == 0 || strcmp(logSpec, "none") == 0) {
        return 0;
    }
    level = -1;
    file = 0;
    MPR->logPath = path = sclone(logSpec);
    if ((levelSpec = strrchr(path, ':')) != 0 && isdigit((uchar) levelSpec[1])) {
        *levelSpec++ = '\0';
        level = atoi(levelSpec);
    }
    if (strcmp(path, "stdout") == 0) {
        file = MPR->stdOutput;
    } else if (strcmp(path, "stderr") == 0) {
        file = MPR->stdError;
#if ME_MPR_DISK
    } else {
        MprPath     info;
        int         mode;
        mode = (flags & MPR_LOG_ANEW) ? O_TRUNC : O_APPEND;
        mode |= O_CREAT | O_WRONLY | O_TEXT;
        if (MPR->logBackup > 0) {
            mprGetPathInfo(path, &info);
            if (MPR->logSize <= 0 || (info.valid && info.size > MPR->logSize) || (flags & MPR_LOG_ANEW)) {
                if (mprBackupLog(path, MPR->logBackup) < 0) {
                    mprPrintf("Cannot backup log %s, errno=%d\n", path, errno);
                }
            }
        }
        if ((file = mprOpenFile(path, mode, 0664)) == 0) {
            mprPrintf("Cannot open log file %s, errno=%d", path, errno);
            return MPR_ERR_CANT_OPEN;
        }
#endif
    }
    MPR->flags |= (flags & (MPR_LOG_DETAILED | MPR_LOG_ANEW | MPR_LOG_CONFIG | MPR_LOG_CMDLINE | MPR_LOG_TAGGED));

    if (level >= 0) {
        mprSetLogLevel(level);
    }
    if (file) {
        mprSetLogFile(file);
    }
    if (flags & MPR_LOG_CONFIG) {
        mprLogConfig();
    }
    return 0;
}


PUBLIC void mprLogConfig()
{
    cchar   *name;

    name = MPR->name;
    mprLog(name, 2, "Configuration for %s", mprGetAppTitle());
    mprLog(name, 2, "----------------------------------");
    mprLog(name, 2, "Version:            %s", CONFIG_VERSION);
    mprLog(name, 2, "BuildType:          %s", CONFIG_DEBUG ? "Debug" : "Release");
    mprLog(name, 2, "CPU:                %s", ME_CPU);
    mprLog(name, 2, "OS:                 %s", CONFIG_OS);
    mprLog(name, 2, "Host:               %s", mprGetHostName());
    mprLog(name, 2, "PID:                %d", getpid());
    mprLog(name, 2, "----------------------------------");
}


PUBLIC int mprBackupLog(cchar *path, int count)
{
    char    *from, *to;
    int     i;

    for (i = count - 1; i > 0; i--) {
        from = sfmt("%s.%d", path, i - 1);
        to = sfmt("%s.%d", path, i);
        unlink(to);
        rename(from, to);
    }
    from = sfmt("%s", path);
    to = sfmt("%s.0", path);
    unlink(to);
    if (rename(from, to) < 0) {
        return MPR_ERR_CANT_CREATE;
    }
    return 0;
}


PUBLIC void mprSetLogBackup(ssize size, int backup, int flags)
{
    MPR->logBackup = backup;
    MPR->logSize = size;
    MPR->flags |= (flags & MPR_LOG_ANEW);
}


/*
    Legacy error messages
 */
PUBLIC void mprError(cchar *format, ...)
{
    va_list     args;
    char        buf[ME_MAX_LOGLINE], tagbuf[128];

    va_start(args, format);
    fmt(tagbuf, sizeof(tagbuf), "%s error", MPR->name);
    logOutput(tagbuf, 0, fmtv(buf, sizeof(buf), format, args));
    va_end(args);
}


PUBLIC void mprLogProc(cchar *tags, int level, cchar *fmt, ...)
{
    va_list     args;
    char        buf[ME_MAX_LOGLINE];

    va_start(args, fmt);
    logOutput(tags, level, fmtv(buf, sizeof(buf), fmt, args));
    va_end(args);
}


PUBLIC void mprAssert(cchar *loc, cchar *msg)
{
#if ME_MPR_DEBUG_LOGGING
    char    buf[ME_MAX_LOGLINE];

    if (loc) {
        snprintf(buf, sizeof(buf), "Assertion %s, failed at %s", msg, loc);
    }
    mprLogProc("debug assert", 0, "%s", buf);
#if ME_DEBUG_WATSON
    fprintf(stdout, "Pause for debugger to attach\n");
    mprSleep(24 * 3600 * 1000);
#endif
#endif
}


/*
    Output a log message to the log handler
 */
static void logOutput(cchar *tags, int level, cchar *msg)
{
    MprLogHandler   handler;

    if (level < 0 || level > mprGetLogLevel()) {
        return;
    }
    handler = MPR->logHandler;
    if (handler != 0) {
        (handler)(tags, level, msg);
        return;
    }
    mprDefaultLogHandler(tags, level, msg);
}


static void backupLog()
{
#if ME_MPR_DISK
    MprPath     info;
    MprFile     *file;
    int         mode;

    mprGetPathInfo(MPR->logPath, &info);
    if (info.valid && info.size > MPR->logSize) {
        lock(MPR);
        mprSetLogFile(0);
        if (mprBackupLog(MPR->logPath, MPR->logBackup) < 0) {
            mprPrintf("Cannot backup log %s, errno=%d\n", MPR->logPath, errno);
        }
        mode = O_CREAT | O_WRONLY | O_TEXT;
        if ((file = mprOpenFile(MPR->logPath, mode, 0664)) == 0) {
            mprLog("error mpr log", 0, "Cannot open log file %s, errno=%d", MPR->logPath, errno);
            MPR->logSize = MAXINT;
            unlock(MPR);
            return;
        }
        mprSetLogFile(file);
        unlock(MPR);
    }
#endif
}


/*
    If MPR_LOG_DETAILED with tags, the format is:
        MM/DD/YY HH:MM:SS LEVEL TAGS, Message
    Otherwise just the message is output
 */
PUBLIC void mprDefaultLogHandler(cchar *tags, int level, cchar *msg)
{
    char            tbuf[128];
    static ssize    length = 0;

    if (MPR->logFile == 0 || msg == 0 || *msg == '\0') {
        return;
    }
    if (MPR->logBackup && MPR->logSize && length >= MPR->logSize) {
        backupLog();
    }
    if (tags && *tags) {
        if (MPR->flags & MPR_LOG_DETAILED) {
            fmt(tbuf, sizeof(tbuf), "%s %d %s, ", mprGetDate(MPR_LOG_DATE), level, tags);
            length += mprWriteFileString(MPR->logFile, tbuf);
        } else if (MPR->flags & MPR_LOG_TAGGED) {
            if (schr(tags, ' ')) {
                tags = ssplit(sclone(tags), " ", NULL);
            }
            if (!isupper((uchar) *tags)) {
                tags = stitle(tags);
            }
            length += mprWriteFileFmt(MPR->logFile, "%12s ", sfmt("[%s]", tags));
        }
    }
    length += mprWriteFileString(MPR->logFile, msg);
    if (*msg && msg[slen(msg) - 1] != '\n') {
        length += mprWriteFileString(MPR->logFile, "\n");
    }
#if CONFIG_MPR_OSLOG
    if (level == 0) {
        mprWriteToOsLog(sfmt("%s: %d %s: %s", MPR->name, level, tags, msg), level);
    }
#endif
}


/*
    Return the raw O/S error code
 */
PUBLIC int mprGetOsError()
{
    return errno;
}


PUBLIC void mprSetOsError(int error)
{
    errno = error;
}


/*
    Return the mapped (portable, Posix) error code
 */
PUBLIC int mprGetError()
{
    return mprGetOsError();
}


/*
    Set the mapped (portable, Posix) error code
 */
PUBLIC void mprSetError(int error)
{
    mprSetOsError(error);
    return;
}


PUBLIC int mprGetLogLevel()
{
    Mpr     *mpr;

    /* Leave the code like this so debuggers can patch logLevel before returning */
    mpr = MPR;
    return mpr->logLevel;
}


PUBLIC MprLogHandler mprGetLogHandler()
{
    return MPR->logHandler;
}


PUBLIC int mprUsingDefaultLogHandler()
{
    return MPR->logHandler == mprDefaultLogHandler;
}


PUBLIC MprFile *mprGetLogFile()
{
    return MPR->logFile;
}


PUBLIC MprLogHandler mprSetLogHandler(MprLogHandler handler)
{
    MprLogHandler   priorHandler;

    priorHandler = MPR->logHandler;
    MPR->logHandler = handler;
    return priorHandler;
}


PUBLIC void mprSetLogFile(MprFile *file)
{
    if (file != MPR->logFile && MPR->logFile != MPR->stdOutput && MPR->logFile != MPR->stdError) {
        mprCloseFile(MPR->logFile);
    }
    MPR->logFile = file;
}


PUBLIC void mprSetLogLevel(int level)
{
    MPR->logLevel = level;
}


PUBLIC bool mprSetCmdlineLogging(bool on)
{
    bool    wasLogging;

    wasLogging = MPR->flags & MPR_LOG_CMDLINE;
    MPR->flags &= ~MPR_LOG_CMDLINE;
    if (on) {
        MPR->flags |= MPR_LOG_CMDLINE;
    }
    return wasLogging;
}


PUBLIC bool mprGetCmdlineLogging()
{
    return MPR->flags & MPR_LOG_CMDLINE ? 1 : 0;
}




