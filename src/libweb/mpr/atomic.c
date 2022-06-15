#include "libmpr.h"

/**
    atomic.c - Atomic operations

 */

/*********************************** Includes *********************************/



/*********************************** Local ************************************/

static MprSpin  atomicSpinLock;
static MprSpin  *atomicSpin = &atomicSpinLock;

/************************************ Code ************************************/

PUBLIC void mprAtomicOpen()
{
    mprInitSpinLock(atomicSpin);
}

/*
    Full memory barrier
 */
PUBLIC void mprAtomicBarrier(int model)
{
    #if defined(VX_MEM_BARRIER_RW)
        VX_MEM_BARRIER_RW();

    #elif CONFIG_COMPILER_HAS_SYNC
        __sync_synchronize();

    #elif __GNUC__ && (ME_CPU_ARCH == ME_CPU_X86 || ME_CPU_ARCH == ME_CPU_X64)
        asm volatile ("mfence" : : : "memory");

    #elif __GNUC__ && (ME_CPU_ARCH == ME_CPU_PPC)
        asm volatile ("sync" : : : "memory");

    #elif __GNUC__ && (ME_CPU_ARCH == ME_CPU_ARM) && FUTURE
        asm volatile ("isync" : : : "memory");

    #else
        if (mprTrySpinLock(atomicSpin)) {
            mprSpinUnlock(atomicSpin);
        }
    #endif
}


/*
    Atomic compare and swap a pointer.
 */
PUBLIC int mprAtomicCas(void * volatile *addr, void *expected, cvoid *value)
{
    #if CONFIG_COMPILER_HAS_SYNC_CAS
        return __sync_bool_compare_and_swap(addr, expected, (void*) value);

    #elif __GNUC__ && (ME_CPU_ARCH == ME_CPU_X86)
        void *prev;
        asm volatile ("lock; cmpxchgl %2, %1"
            : "=a" (prev), "=m" (*addr)
            : "r" (value), "m" (*addr), "0" (expected));
        return expected == prev;

    #elif __GNUC__ && (ME_CPU_ARCH == ME_CPU_X64)
        void *prev;
        asm volatile ("lock; cmpxchgq %q2, %1"
            : "=a" (prev), "=m" (*addr)
            : "r" (value), "m" (*addr),
              "0" (expected));
            return expected == prev;

    #elif __GNUC__ && (ME_CPU_ARCH == ME_CPU_ARM) && FUTURE

    #else
        mprSpinLock(atomicSpin);
        if (*addr == expected) {
            *addr = (void*) value;
            mprSpinUnlock(atomicSpin);
            return 1;
        }
        mprSpinUnlock(atomicSpin);
        return 0;
    #endif
}


/*
    Atomic add of a signed value. Used for add, subtract, inc, dec
 */
PUBLIC void mprAtomicAdd(volatile int *ptr, int value)
{
    #if CONFIG_COMPILER_HAS_SYNC_CAS
        __sync_add_and_fetch(ptr, value);

    #elif __GNUC__ && (ME_CPU_ARCH == ME_CPU_X86 || ME_CPU_ARCH == ME_CPU_X64)
        asm volatile("lock; addl %1,%0"
             : "+m" (*ptr)
             : "ir" (value));
    #else
        mprSpinLock(atomicSpin);
        *ptr += value;
        mprSpinUnlock(atomicSpin);

    #endif
}


/*
    On some platforms, this operation is only atomic with respect to other calls to mprAtomicAdd64
 */
PUBLIC void mprAtomicAdd64(volatile int64 *ptr, int64 value)
{
    #if CONFIG_COMPILER_HAS_SYNC64 && (ME_64 || ME_CPU_ARCH == ME_CPU_X86 || ME_CPU_ARCH == ME_CPU_X64)
        __sync_add_and_fetch(ptr, value);

    #elif __GNUC__ && (ME_CPU_ARCH == ME_CPU_X86)
        asm volatile ("lock; xaddl %0,%1"
            : "=r" (value), "=m" (*ptr)
            : "0" (value), "m" (*ptr)
            : "memory", "cc");

    #elif __GNUC__ && (ME_CPU_ARCH == ME_CPU_X64)
        asm volatile("lock; addq %1,%0"
             : "=m" (*ptr)
             : "er" (value), "m" (*ptr));

    #else
        mprSpinLock(atomicSpin);
        *ptr += value;
        mprSpinUnlock(atomicSpin);

    #endif
}


#if FUTURE
PUBLIC void *mprAtomicExchange(void *volatile *addr, cvoid *value)
{
    #if CONFIG_COMPILER_HAS_SYNC
        return __sync_lock_test_and_set(addr, (void*) value);

    #else
        void    *old;
        mprSpinLock(atomicSpin);
        old = *(void**) addr;
        *addr = (void*) value;
        mprSpinUnlock(atomicSpin);
        return old;
    #endif
}
#endif


/*
    Atomic list insertion. Inserts "item" at the "head" of the list. The "link" field is the next field in item.
 */
PUBLIC void mprAtomicListInsert(void **head, void **link, void *item)
{
    do {
        *link = *head;
    } while (!mprAtomicCas(head, (void*) *link, item));
}


