#include "libmpr.h"

/**
    lock.c - Thread Locking Support
 */

/*********************************** Includes *********************************/



/***************************** Forward Declarations ***************************/

static void manageLock(MprMutex *lock, int flags);
static void manageSpinLock(MprSpin *lock, int flags);

/************************************ Code ************************************/

PUBLIC MprMutex *mprCreateLock()
{
    MprMutex    *lock;

    if ((lock = mprAllocObjNoZero(MprMutex, manageLock)) == 0) {
        return 0;
    }
    return mprInitLock(lock);
}


static void manageLock(MprMutex *lock, int flags)
{
    if (flags & MPR_MANAGE_FREE) {
        assert(lock);
        pthread_mutex_destroy(&lock->cs);
    }
}


PUBLIC MprMutex *mprInitLock(MprMutex *lock)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&lock->cs, &attr);
    pthread_mutexattr_destroy(&attr);

    return lock;
}


/*
    Try to attain a lock. Do not block! Returns true if the lock was attained.
 */
PUBLIC bool mprTryLock(MprMutex *lock)
{
    int     rc;

    if (lock == 0) return 0;

    rc = pthread_mutex_trylock(&lock->cs) != 0;

#if CONFIG_DEBUG
    lock->owner = mprGetCurrentOsThread();
#endif
    return (rc) ? 0 : 1;
}


PUBLIC MprSpin *mprCreateSpinLock()
{
    MprSpin    *lock;

    if ((lock = mprAllocObjNoZero(MprSpin, manageSpinLock)) == 0) {
        return 0;
    }
    return mprInitSpinLock(lock);
}


static void manageSpinLock(MprSpin *lock, int flags)
{
    if (flags & MPR_MANAGE_FREE) {
        assert(lock);
#if ME_COMPILER_HAS_SPINLOCK
        pthread_spin_destroy(&lock->cs);
#else
        pthread_mutex_destroy(&lock->cs);
        semDelete(lock->cs);
#endif
    }
}


/*
    Static version just for mprAlloc which needs locks that don't allocate memory.
 */
PUBLIC MprSpin *mprInitSpinLock(MprSpin *lock)
{
#if ME_COMPILER_HAS_SPINLOCK
    pthread_spin_init(&lock->cs, 0);

#else
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&lock->cs, &attr);
    pthread_mutexattr_destroy(&attr);
#endif

#if CONFIG_DEBUG
    lock->owner = 0;
#endif
    return lock;
}


/*
    Try to attain a lock. Do not block! Returns true if the lock was attained.
 */
PUBLIC bool mprTrySpinLock(MprSpin *lock)
{
    int     rc;

    if (lock == 0) return 0;

#if ME_COMPILER_HAS_SPINLOCK
    rc = pthread_spin_trylock(&lock->cs) != 0;
#else
    rc = pthread_mutex_trylock(&lock->cs) != 0;
#endif

#if CONFIG_DEBUG && COSTLY
    if (rc == 0) {
        assert(lock->owner != mprGetCurrentOsThread());
        lock->owner = mprGetCurrentOsThread();
    }
#endif
    return (rc) ? 0 : 1;
}


/*
    Big global lock. Avoid using this.
 */
PUBLIC void mprGlobalLock()
{
    if (MPR && MPR->mutex) {
        mprLock(MPR->mutex);
    }
}


PUBLIC void mprGlobalUnlock()
{
    if (MPR && MPR->mutex) {
        mprUnlock(MPR->mutex);
    }
}


#if ME_USE_LOCK_MACROS
/*
    Still define these even if using macros to make linking with *.def export files easier
 */
#undef mprLock
#undef mprUnlock
#undef mprSpinLock
#undef mprSpinUnlock
#endif

/*
    Lock a mutex
 */
PUBLIC void mprLock(MprMutex *lock)
{
    if (lock == 0) return;

    pthread_mutex_lock(&lock->cs);

#if CONFIG_DEBUG
    /* Store last locker only */
    lock->owner = mprGetCurrentOsThread();
#endif
}


PUBLIC void mprUnlock(MprMutex *lock)
{
    if (lock == 0) return;

    pthread_mutex_unlock(&lock->cs);
}


/*
    Use functions for debug mode. Production release uses macros
 */
/*
    Lock a mutex
 */
PUBLIC void mprSpinLock(MprSpin *lock)
{
    if (lock == 0) return;

#if CONFIG_DEBUG
    /*
        Spin locks don't support recursive locking on all operating systems.
     */
    assert(lock->owner != mprGetCurrentOsThread());
#endif

#if ME_COMPILER_HAS_SPINLOCK
    pthread_spin_lock(&lock->cs);
#else
    pthread_mutex_lock(&lock->cs);
#endif

#if CONFIG_DEBUG
    assert(lock->owner != mprGetCurrentOsThread());
    lock->owner = mprGetCurrentOsThread();
#endif
}


PUBLIC void mprSpinUnlock(MprSpin *lock)
{
    if (lock == 0) return;

#if CONFIG_DEBUG
    lock->owner = 0;
#endif

#if ME_COMPILER_HAS_SPINLOCK
    pthread_spin_unlock(&lock->cs);
#else
    pthread_mutex_unlock(&lock->cs);
#endif
}




