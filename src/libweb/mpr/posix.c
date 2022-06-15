#include "libmpr.h"

/**
    posix.c - Posix specific adaptions

 */

/********************************* Includes ***********************************/


/*********************************** Code *************************************/

PUBLIC int mprCreateOsService()
{
    umask(022);

    /*
        Cleanup the environment. IFS is often a security hole
     */
    putenv("IFS=\t ");
    return 0;
}


PUBLIC int mprStartOsService()
{
    /*
        Open a syslog connection
     */
#if SOLARIS
    openlog(mprGetAppName(), LOG_LOCAL0);
#else
    openlog(mprGetAppName(), 0, LOG_LOCAL0);
#endif
    return 0;
}


PUBLIC void mprStopOsService()
{
    closelog();
}


PUBLIC int mprGetRandomBytes(char *buf, ssize length, bool block)
{
    ssize   sofar, rc;
    int     fd;

    if ((fd = open((block) ? "/dev/random" : "/dev/urandom", O_RDONLY, 0666)) < 0) {
        return MPR_ERR_CANT_OPEN;
    }
    sofar = 0;
    do {
        rc = read(fd, &buf[sofar], length);
        if (rc < 0) {
            close(fd);
            return MPR_ERR_CANT_READ;
        }
        length -= rc;
        sofar += rc;
    } while (length > 0);
    close(fd);
    return 0;
}


#if CONFIG_COMPILER_HAS_DYN_LOAD
PUBLIC int mprLoadNativeModule(MprModule *mp)
{
    MprModuleEntry  fn;
    void            *handle;

    assert(mp);

    /*
        Search the image incase the module has been statically linked
     */
#ifdef RTLD_DEFAULT
    handle = RTLD_DEFAULT;
#else
#ifdef RTLD_MAIN_ONLY
    handle = RTLD_MAIN_ONLY;
#else
    handle = 0;
#endif
#endif
    if (!mp->entry || !dlsym(handle, mp->entry)) {
#if ME_STATIC
        mprLog("error mpr", 0, "Cannot load module %s, product built static", mp->name);
        return MPR_ERR_BAD_STATE;
#else
        MprPath info;
        char    *at;
        if ((at = mprSearchForModule(mp->path)) == 0) {
            mprLog("error mpr", 0, "Cannot find module \"%s\", cwd: \"%s\", search path \"%s\"", mp->path,
                mprGetCurrentPath(), mprGetModuleSearchPath());
            return MPR_ERR_CANT_ACCESS;
        }
        mp->path = at;
        mprGetPathInfo(mp->path, &info);
        mp->modified = info.mtime;
        mprLog("info mpr", 5, "Loading native module %s", mprGetPathBase(mp->path));
        if ((handle = dlopen(mp->path, RTLD_LAZY | RTLD_GLOBAL)) == 0) {
            mprLog("error mpr", 0, "Cannot load module %s, reason: \"%s\"", mp->path, dlerror());
            return MPR_ERR_CANT_OPEN;
        }
        mp->handle = handle;
        mp->flags |= MPR_MODULE_LOADED;
#endif /* !ME_STATIC */

    } else if (mp->entry) {
        mprLog("info mpr", 4, "Activating native module %s", mp->name);
    }
    if (mp->entry) {
        if ((fn = (MprModuleEntry) dlsym(handle, mp->entry)) != 0) {
            if ((fn)(mp->moduleData, mp) < 0) {
                mprLog("error mpr", 0, "Initialization for module %s failed", mp->name);
                mprUnloadNativeModule(mp);
                return MPR_ERR_CANT_INITIALIZE;
            }
        } else {
            mprLog("error mpr", 0, "Cannot load module %s, reason: cannot find function \"%s\"", mp->path, mp->entry);
            mprUnloadNativeModule(mp);
            return MPR_ERR_CANT_READ;
        }
    }
    return 0;
}


PUBLIC int mprUnloadNativeModule(MprModule *mp)
{
    if (mp->flags & MPR_MODULE_LOADED) {
        mp->flags &= ~MPR_MODULE_LOADED;
        return dlclose(mp->handle);
    }
    return MPR_ERR_BAD_STATE;
}
#endif


/*
    This routine does not yield
 */
PUBLIC void mprNap(MprTicks timeout)
{
    MprTicks        remaining, mark;
    struct timespec t;
    int             rc;

    assert(timeout >= 0);

    mark = mprGetTicks();
    if (timeout < 0 || timeout > MAXINT) {
        timeout = MAXINT;
    }
    remaining = timeout;
    do {
        /* MAC OS X corrupts the timeout if using the 2nd paramater, so recalc each time */
        t.tv_sec = ((int) (remaining / 1000));
        t.tv_nsec = ((int) ((remaining % 1000) * 1000000));
        rc = nanosleep(&t, NULL);
        remaining = mprGetRemainingTicks(mark, timeout);
    } while (rc < 0 && errno == EINTR && remaining > 0);
}


/*
    This routine yields
 */
PUBLIC void mprSleep(MprTicks timeout)
{
    mprYield(MPR_YIELD_STICKY);
    mprNap(timeout);
    mprResetYield();
}


/*
    Write a message in the O/S native log (syslog in the case of linux)
 */
PUBLIC void mprWriteToOsLog(cchar *message, int level)
{
    syslog((level == 0) ? LOG_ERR : LOG_WARNING, "%s", message);
}


PUBLIC void mprSetFilesLimit(int limit)
{
    struct rlimit r;

    if (limit == 0 || limit == MAXINT) {
        /*
            We need to determine a reasonable maximum possible limit value.
            There is no #define we can use for this, so we test to determine it empirically.
         */
        for (limit = 0x40000000; limit > 0; limit >>= 1) {
            r.rlim_cur = r.rlim_max = limit;
            if (setrlimit(RLIMIT_NOFILE, &r) == 0) {
                break;
            }
        }
    } else {
        r.rlim_cur = r.rlim_max = limit;
        if (setrlimit(RLIMIT_NOFILE, &r) < 0) {
            mprLog("error mpr", 0, "Cannot set file limit to %d", limit);
        }
    }
    getrlimit(RLIMIT_NOFILE, &r);
}




