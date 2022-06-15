#include "libmpr.h"

/**
    cond.c - Thread Conditional variables
 */



/***************************** Forward Declarations ***************************/

static void manageCond(MprCond *cp, int flags);

/************************************ Code ************************************/
/*
    Create a condition variable for use by single or multiple waiters
 */

PUBLIC MprCond *mprCreateCond()
{
    MprCond     *cp;

    if ((cp = mprAllocObjNoZero(MprCond, manageCond)) == 0) {
        return 0;
    }
    cp->triggered = 0;
    cp->mutex = mprCreateLock();

    pthread_cond_init(&cp->cv, NULL);

    return cp;
}


static void manageCond(MprCond *cp, int flags)
{
    assert(cp);
    if (!cp) {
        return;
    }
    if (flags & MPR_MANAGE_MARK) {
        mprMark(cp->mutex);

    } else if (flags & MPR_MANAGE_FREE) {
        pthread_cond_destroy(&cp->cv);
    }
}


/*
    Wait for the event to be triggered. Should only be used when there are single waiters. If the event is already
    triggered, then it will return immediately. Timeout of -1 means wait forever. Timeout of 0 means no wait.
    Returns 0 if the event was signalled. Returns < 0 for a timeout.

    WARNING: On unix, the pthread_cond_timedwait uses an absolute time (Ugh!). So time-warps for daylight-savings may
    cause waits to prematurely return.
 */
PUBLIC int mprWaitForCond(MprCond *cp, MprTicks timeout)
{
    MprTicks            now, expire;
    int                 rc;

    struct timespec     waitTill;
    struct timeval      current;
    int                 usec;

    /*
        Avoid doing a mprGetTicks() if timeout is < 0
     */
    rc = 0;
    if (timeout >= 0) {
        if (timeout > MAXINT) {
            timeout = MAXINT;
        }
        now = mprGetTicks();
        expire = now + timeout;
        if (expire < 0) {
            expire = MPR_MAX_TIMEOUT;
        }

        gettimeofday(&current, NULL);
        usec = current.tv_usec + ((int) (timeout % 1000)) * 1000;
        waitTill.tv_sec = current.tv_sec + ((int) (timeout / 1000)) + (usec / 1000000);
        waitTill.tv_nsec = (usec % 1000000) * 1000;
    } else {
        expire = -1;
        now = 0;
    }
    mprLock(cp->mutex);
    /*
        NOTE: The WaitForSingleObject and semTake APIs keeps state as to whether the object is signalled.
        WaitForSingleObject and semTake will not block if the object is already signalled. However, pthread_cond_
        is different and does not keep such state. If it is signalled before pthread_cond_wait, the thread will
        still block. Consequently we need to keep our own state in cp->triggered. This also protects against
        spurious wakeups which can happen (on windows).
     */
    do {
        /*
            The pthread_cond_wait routines will atomically unlock the mutex before sleeping and will relock on awakening.
            WARNING: pthreads may do spurious wakeups without being triggered
         */
        if (!cp->triggered) {
            do {
                if (now) {
                    rc = pthread_cond_timedwait(&cp->cv, &cp->mutex->cs,  &waitTill);
                } else {
                    rc = pthread_cond_wait(&cp->cv, &cp->mutex->cs);
                }
            } while ((rc == 0 || rc == EAGAIN) && !cp->triggered);
            if (rc == ETIMEDOUT) {
                rc = MPR_ERR_TIMEOUT;
            } else if (rc == EAGAIN) {
                rc = 0;
            } else if (rc != 0) {
                mprLog("error mpr thread", 0, "pthread_cond_timedwait error rc %d", rc);
                rc = MPR_ERR;
            }
        }
    } while (!cp->triggered && rc == 0 && (!now || (now = mprGetTicks()) < expire));

    if (cp->triggered) {
        cp->triggered = 0;
        rc = 0;
    } else if (rc == 0) {
        rc = MPR_ERR_TIMEOUT;
    }
    mprUnlock(cp->mutex);
    return rc;
}


/*
    Signal a condition and wakeup the waiter. Note: this may be called prior to the waiter waiting.
 */
PUBLIC void mprSignalCond(MprCond *cp)
{
    mprLock(cp->mutex);
    if (!cp->triggered) {
        cp->triggered = 1;

        pthread_cond_signal(&cp->cv);
    }
    mprUnlock(cp->mutex);
}


PUBLIC void mprResetCond(MprCond *cp)
{
    mprLock(cp->mutex);
    cp->triggered = 0;

    pthread_cond_destroy(&cp->cv);
    pthread_cond_init(&cp->cv, NULL);

    mprUnlock(cp->mutex);
}


/*
    Wait for the event to be triggered when there may be multiple waiters. This routine may return early due to
    other signals or events. The caller must verify if the signalled condition truly exists. If the event is already
    triggered, then it will return immediately. This call will not reset cp->triggered and must be reset manually.
    A timeout of -1 means wait forever. Timeout of 0 means no wait.  Returns 0 if the event was signalled.
    Returns < 0 for a timeout.

    WARNING: On unix, the pthread_cond_timedwait uses an absolute time (Ugh!). So time-warps for daylight-savings may
    cause waits to prematurely return.
 */
PUBLIC int mprWaitForMultiCond(MprCond *cp, MprTicks timeout)
{
    int         rc;

    struct timespec     waitTill;
    struct timeval      current;
    int                 usec;

    if (timeout < 0) {
        timeout = MAXINT;
    }

    gettimeofday(&current, NULL);
    usec = current.tv_usec + ((int) (timeout % 1000)) * 1000;
    waitTill.tv_sec = current.tv_sec + ((int) (timeout / 1000)) + (usec / 1000000);
    waitTill.tv_nsec = (usec % 1000000) * 1000;

    mprLock(cp->mutex);
    rc = pthread_cond_timedwait(&cp->cv, &cp->mutex->cs,  &waitTill);
    if (rc == ETIMEDOUT) {
        rc = MPR_ERR_TIMEOUT;
    } else if (rc != 0) {
        rc = MPR_ERR;
    }
    mprUnlock(cp->mutex);

    return rc;
}


/*
    Signal a condition and wakeup the all the waiters. Note: this may be called before or after to the waiter waiting.
 */
PUBLIC void mprSignalMultiCond(MprCond *cp)
{
    mprLock(cp->mutex);
    pthread_cond_broadcast(&cp->cv);
    mprUnlock(cp->mutex);
}



