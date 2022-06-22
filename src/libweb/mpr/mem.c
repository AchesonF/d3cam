#include "libmpr.h"


/**
    mem.c - Memory Allocator and Garbage Collector.

    This is the MPR memory allocation service. It provides an application specific memory allocator to use instead
    of malloc. This allocator is tailored to the needs of embedded applications and is faster than most general
    purpose malloc allocators. It is deterministic and allocates and frees in constant time O(1). It exhibits very
    low fragmentation and accurate coalescing.

    The allocator uses a garbage collector for freeing unused memory. The collector is a cooperative, non-compacting,
    parallel collector.  The allocator is optimized for frequent allocations of small blocks (< 4K) and uses a
    scheme of free queues for fast allocation.

    The allocator handles memory allocation errors globally. The application may configure a memory limit so that
    memory depletion can be proactively detected and handled before memory allocations actually fail.

    A memory block that is being used must be marked as active to prevent the garbage collector from reclaiming it.
    To mark a block as active, #mprMarkBlock must be called during each garbage collection cycle. When allocating
    non-temporal memory blocks, a manager callback can be specified via #mprAllocObj. This manager routine will be
    called by the collector so that dependent memory blocks can be marked as active.

    The collector performs the marking phase by invoking the manager routines for a set of root blocks. A block can be
    added to the set of roots by calling #mprAddRoot. Each root's manager routine will mark other blocks which will cause
    their manager routines to run and so on, until all active blocks have been marked. Non-marked blocks can then safely
    be reclaimed as garbage. A block may alternatively be permanently marked as active by calling #mprHold.

    The mark phase begins when all threads explicitly "yield" to the garbage collector. This cooperative approach ensures
    that user threads will not inadvertendly loose allocated blocks to the collector. Once all active blocks are marked,
    user threads are resumed and the garbage sweeper frees unused blocks in parallel with user threads.

 */

/********************************* Includes ***********************************/



/********************************** Defines ***********************************/

#undef GET_MEM
#undef GET_PTR
#define GET_MEM(ptr)                ((MprMem*) (((char*) (ptr)) - sizeof(MprMem)))
#define GET_PTR(mp)                 ((char*) (((char*) mp) + sizeof(MprMem)))
#define GET_USIZE(mp)               ((size_t) (mp->size - sizeof(MprMem) - (mp->hasManager * sizeof(void*))))

/*
    These routines are stable and will work, lock-free regardless of block splitting or joining.
    There is be a race where GET_NEXT will skip a block if the allocator is splits mp.
 */
#define GET_NEXT(mp)                ((MprMem*) ((char*) mp + mp->size))
#define GET_REGION(mp)              ((MprRegion*) (((char*) mp) - MPR_ALLOC_ALIGN(sizeof(MprRegion))))

/*
    Memory checking and breakpoints
    ME_MPR_ALLOC_DEBUG checks that blocks are valid and keeps track of the location where memory is allocated from.
 */
#if ME_MPR_ALLOC_DEBUG
    /*
        Set this address to break when this address is allocated or freed
        Only used for debug, but defined regardless so we can have constant exports.
     */
    static MprMem *stopAlloc = 0;
    static int stopSeqno = -1;

    #define BREAKPOINT(mp)          breakpoint(mp)
    #define CHECK(mp)               if (mp) { mprCheckBlock((MprMem*) mp); } else
    #define CHECK_PTR(ptr)          CHECK(GET_MEM(ptr))
    #define SCRIBBLE(mp)            if (heap->scribble && mp != GET_MEM(MPR)) { \
                                        memset((char*) mp + MPR_ALLOC_MIN_BLOCK, 0xFE, mp->size - MPR_ALLOC_MIN_BLOCK); \
                                    } else
    #define SCRIBBLE_RANGE(ptr, size) if (heap->scribble) { \
                                        memset((char*) ptr, 0xFE, size); \
                                    } else
    #define SET_MAGIC(mp)           mp->magic = MPR_ALLOC_MAGIC
    #define SET_SEQ(mp)             mp->seqno = heap->nextSeqno++
    #define VALID_BLK(mp)           validBlk(mp)
    #define SET_NAME(mp, value)     mp->name = value

#else
    #define BREAKPOINT(mp)
    #define CHECK(mp)
    #define CHECK_PTR(mp)
    #define SCRIBBLE(mp)
    #define SCRIBBLE_RANGE(ptr, size)
    #define SET_NAME(mp, value)
    #define SET_MAGIC(mp)
    #define SET_SEQ(mp)
    #define VALID_BLK(mp) 1
#endif

#define ATOMIC_ADD(field, adj) mprAtomicAdd64((int64*) &heap->stats.field, adj)

#if ME_MPR_ALLOC_STATS
    #define ATOMIC_INC(field) mprAtomicAdd64((int64*) &heap->stats.field, 1)
    #define INC(field) heap->stats.field++
#else
    #define ATOMIC_INC(field)
    #define INC(field)
#endif

#if ME_64
    #define findFirstBit(word) ffsl((long) word)
#else
    #define findFirstBit(word) ffs((int) word)
#endif

#ifndef findFirstBit
    static ME_INLINE int findFirstBit(size_t word);
#endif
#ifndef findLastBit
    static ME_INLINE int findLastBit(size_t word);
#endif

#define YIELDED_THREADS     0x1         /* Resume threads that are yielded (only) */
#define WAITING_THREADS     0x2         /* Resume threads that are waiting for GC sweep to complete */

/********************************** Data **************************************/

#undef              MPR
PUBLIC Mpr          *MPR;
static MprHeap      *heap;
static MprMemStats  memStats;
static int          padding[] = { 0, MPR_MANAGER_SIZE };

/***************************** Forward Declarations ***************************/

static ME_INLINE bool acquire(MprFreeQueue *freeq);
static void allocException(int cause, size_t size);
static MprMem *allocMem(size_t size);
static ME_INLINE int cas(size_t *target, size_t expected, size_t value);
static ME_INLINE bool claim(MprMem *mp);
static ME_INLINE void clearbitmap(size_t *bitmap, int bindex);
static void dummyManager(void *ptr, int flags);
static void freeBlock(MprMem *mp);
static void getSystemInfo(void);
static MprMem *growHeap(size_t size);
static void invokeAllDestructors(void);
static ME_INLINE size_t qtosize(int qindex);
static ME_INLINE bool linkBlock(MprMem *mp);
static ME_INLINE void linkSpareBlock(char *ptr, size_t size);
static ME_INLINE void initBlock(MprMem *mp, size_t size, int first);
static int initQueues(void);
static void invokeDestructors(void);
static void markAndSweep(void);
static void markRoots(void);
static ME_INLINE bool needGC(MprHeap *heap);
static int pauseThreads(void);
static void printMemReport(void);
static ME_INLINE void release(MprFreeQueue *freeq);
static void resumeThreads(int flags);
static ME_INLINE void setbitmap(size_t *bitmap, int bindex);
static ME_INLINE int sizetoq(size_t size);
static void dontBusyWait(void);
static void sweep(void);
static void sweeperThread(void *unused, MprThread *tp);
static ME_INLINE void triggerGC(int always);
static ME_INLINE void unlinkBlock(MprMem *mp);
static void *vmalloc(size_t size, int mode);
static void vmfree(void *ptr, size_t size);


#if ME_MPR_ALLOC_DEBUG
    static void breakpoint(MprMem *mp);
    static int validBlk(MprMem *mp);
    static void freeLocation(MprMem *mp);
#else
    #define freeLocation(mp)
#endif
#if ME_MPR_ALLOC_STATS
    static void printQueueStats(void);
    static void printGCStats(void);
#endif
#if ME_MPR_ALLOC_STACK
    static void monitorStack(void);
#else
    #define monitorStack()
#endif

/************************************* Code ***********************************/

PUBLIC Mpr *mprCreateMemService(MprManager manager, int flags)
{
    MprMem      *mp;
    MprRegion   *region;
    size_t      size, mprSize, spareSize, regionSize;

    getSystemInfo();
    size = MPR_PAGE_ALIGN(sizeof(MprHeap), memStats.pageSize);
    if ((heap = vmalloc(size, MPR_MAP_READ | MPR_MAP_WRITE)) == NULL) {
        return NULL;
    }
    memset(heap, 0, sizeof(MprHeap));
    heap->stats.cpuCores = memStats.cpuCores;
    heap->stats.pageSize = memStats.pageSize;
    heap->stats.maxHeap = (size_t) -1;
    heap->stats.warnHeap = ((size_t) -1) / 100 * 95;

    /*
        Hand-craft the Mpr structure from the first region. Free the remainder below.
     */
    mprSize = MPR_ALLOC_ALIGN(sizeof(MprMem) + sizeof(Mpr) + (MPR_MANAGER_SIZE * sizeof(void*)));
    regionSize = MPR_ALLOC_ALIGN(sizeof(MprRegion));
    size = max(mprSize + regionSize, ME_MPR_ALLOC_REGION_SIZE);
    if ((region = mprVirtAlloc(size, MPR_MAP_READ | MPR_MAP_WRITE)) == NULL) {
        return NULL;
    }
    mp = region->start = (MprMem*) (((char*) region) + regionSize);
    region->end = (MprMem*) (((char*) region) + size);
    region->size = size;

    MPR = (Mpr*) GET_PTR(mp);
    initBlock(mp, mprSize, 1);
    SET_MANAGER(mp, manager);
    mprSetName(MPR, "Mpr");
    MPR->heap = heap;

    heap->flags = flags;
    heap->nextSeqno = 1;
    heap->regionSize = ME_MPR_ALLOC_REGION_SIZE;
    heap->stats.maxHeap = (size_t) -1;
    heap->stats.warnHeap = ((size_t) -1) / 100 * 95;
    heap->stats.cacheHeap = ME_MPR_ALLOC_CACHE;
    heap->stats.lowHeap = max(ME_MPR_ALLOC_CACHE / 8, ME_MPR_ALLOC_REGION_SIZE);
    heap->workQuota = ME_MPR_ALLOC_QUOTA;
    heap->gcEnabled = !(heap->flags & MPR_DISABLE_GC);

    /* Internal testing use only */
    if (scmp(getenv("MPR_DISABLE_GC"), "1") == 0) {
        heap->gcEnabled = 0;
    }
#if ME_MPR_ALLOC_DEBUG
    if (scmp(getenv("MPR_SCRIBBLE_MEM"), "1") == 0) {
        heap->scribble = 1;
    }
    if (scmp(getenv("MPR_VERIFY_MEM"), "1") == 0) {
        heap->verify = 1;
    }
    if (scmp(getenv("MPR_TRACK_MEM"), "1") == 0) {
        heap->track = 1;
    }
#endif
    heap->stats.bytesAllocated += size;
    heap->stats.bytesAllocatedPeak = heap->stats.bytesAllocated;
    INC(allocs);
    initQueues();

    /*
        Free the remaining memory after MPR
     */
    spareSize = size - regionSize - mprSize;
    if (spareSize > 0) {
        linkSpareBlock(((char*) mp) + mprSize, spareSize);
        heap->regions = region;
    }
    heap->gcCond = mprCreateCond();

    heap->roots = mprCreateList(-1, 0);
    mprAddRoot(MPR);
    return MPR;
}


/*
    Destroy all allocated memory including the MPR itself
 */
PUBLIC void mprDestroyMemService()
{
    MprRegion   *region, *next;
    ssize       size;

    for (region = heap->regions; region; ) {
        next = region->next;
        mprVirtFree(region, region->size);
        region = next;
    }
    size = MPR_PAGE_ALIGN(sizeof(MprHeap), memStats.pageSize);
    mprVirtFree(heap, size);
    MPR = 0;
    heap = 0;
}


static ME_INLINE void initBlock(MprMem *mp, size_t size, int first)
{
    static MprMem empty = {0};

    *mp = empty;
    /* Implicit:  mp->free = 0; */
    mp->first = first;
    mp->mark = heap->mark;
    mp->size = (MprMemSize) size;
    SET_MAGIC(mp);
    SET_SEQ(mp);
    SET_NAME(mp, NULL);
    CHECK(mp);
}


PUBLIC void *mprAllocMem(size_t usize, int flags)
{
    MprMem      *mp;
    void        *ptr;
    size_t      size;
    int         padWords;

    padWords = padding[flags & MPR_ALLOC_PAD_MASK];
    size = usize + sizeof(MprMem) + (padWords * sizeof(void*));
    size = max(size, MPR_ALLOC_MIN_BLOCK);
    size = MPR_ALLOC_ALIGN(size);

    if ((mp = allocMem(size)) == NULL) {
        return NULL;
    }
    mp->hasManager = (flags & MPR_ALLOC_MANAGER) ? 1 : 0;
    ptr = GET_PTR(mp);
    if (flags & MPR_ALLOC_ZERO && !mp->fullRegion) {
        /* Regions are zeroed by vmalloc */
        memset(ptr, 0, GET_USIZE(mp));
    }
    if (!(flags & MPR_ALLOC_HOLD)) {
        mp->eternal = 0;
    }
    CHECK(mp);
    monitorStack();
    return ptr;
}


/*
    Optimized allocation for blocks without managers or zeroing
 */
PUBLIC void *mprAllocFast(size_t usize)
{
    MprMem  *mp;
    size_t  size;

    size = usize + sizeof(MprMem);
    size = max(size, MPR_ALLOC_MIN_BLOCK);
    size = MPR_ALLOC_ALIGN(size);
    if ((mp = allocMem(size)) == NULL) {
        return NULL;
    }
    mp->eternal = 0;
    return GET_PTR(mp);
}


/*
    This routine is defined to not zero. However, we zero for some legacy applications.
    This is not guaranteed by the API definition.
 */
PUBLIC void *mprReallocMem(void *ptr, size_t usize)
{
    MprMem      *mp, *newb;
    void        *newptr;
    size_t      oldSize, oldUsize;

    assert(usize > 0);
    if (ptr == 0) {
        return mprAllocZeroed(usize);
    }
    mp = GET_MEM(ptr);
    CHECK(mp);

    oldUsize = GET_USIZE(mp);
    if (usize <= oldUsize) {
        return ptr;
    }
    if ((newptr = mprAllocMem(usize, mp->hasManager ? MPR_ALLOC_MANAGER : 0)) == NULL) {
        return 0;
    }
    newb = GET_MEM(newptr);
    if (mp->hasManager) {
        SET_MANAGER(newb, GET_MANAGER(mp));
    }
    oldSize = mp->size;
    memcpy(newptr, ptr, oldSize - sizeof(MprMem));
    return newptr;
}


PUBLIC void *mprMemdupMem(cvoid *ptr, size_t usize)
{
    char    *newp;

    if ((newp = mprAllocMem(usize, 0)) != 0) {
        memcpy(newp, ptr, usize);
    }
    return newp;
}


PUBLIC int mprMemcmp(cvoid *s1, size_t s1Len, cvoid *s2, size_t s2Len)
{
    int         rc;

    assert(s1);
    assert(s2);
    assert(s1Len >= 0);
    assert(s2Len >= 0);

    if ((rc = memcmp(s1, s2, min(s1Len, s2Len))) == 0) {
        if (s1Len < s2Len) {
            return -1;
        } else if (s1Len > s2Len) {
            return 1;
        }
    }
    return rc;
}


/*
    mprMemcpy will support insitu copy where src and destination overlap
 */
PUBLIC size_t mprMemcpy(void *dest, size_t destMax, cvoid *src, size_t nbytes)
{
    assert(dest);
    assert(destMax <= 0 || destMax >= nbytes);
    assert(src);
    assert(nbytes >= 0);
    if (!dest || !src) {
        return 0;
    }
    if (destMax > 0 && nbytes > destMax) {
        assert(!MPR_ERR_WONT_FIT);
        return 0;
    }
    if (nbytes > 0) {
        memmove(dest, src, nbytes);
        return nbytes;
    } else {
        return 0;
    }
}

/*************************** Allocator *************************/

static int initQueues()
{
    MprFreeQueue    *freeq;
    int             qindex;

    for (freeq = heap->freeq, qindex = 0; freeq < &heap->freeq[MPR_ALLOC_NUM_QUEUES]; freeq++, qindex++) {
        /* Size includes MprMem header */
        freeq->minSize = (MprMemSize) qtosize(qindex);
#if (ME_MPR_ALLOC_STATS && ME_MPR_ALLOC_DEBUG) && MPR_ALLOC_TRACE
        printf("Queue: %d, usize %u  size %u\n",
            (int) (freeq - heap->freeq), (int) freeq->minSize - (int) sizeof(MprMem), (int) freeq->minSize);
#endif
        assert(sizetoq(freeq->minSize) == qindex);
        freeq->next = freeq->prev = (MprFreeMem*) freeq;
        mprInitSpinLock(&freeq->lock);
    }
    return 0;
}


/*
    Lock free memory allocator. This routine races with the sweeper and if invoked from a foreign thread, the marker as well.
 */
static MprMem *allocMem(size_t required)
{
    MprFreeQueue    *freeq;
    MprFreeMem      *fp;
    MprMem          *mp;
    size_t          *bitmap, localMap;
    int             baseBindex, bindex, qindex, baseQindex, retryIndex;

    if (!heap) {
        return 0;
    }
    ATOMIC_INC(requests);

    if ((qindex = sizetoq(required)) >= 0) {
        /*
            Check if the requested size is the smallest possible size in a queue. If not the smallest, must look at the
            next queue higher up to guarantee a block of sufficient size. This implements a Good-fit strategy.
         */
        freeq = &heap->freeq[qindex];
        if (required > freeq->minSize) {
            if (++qindex >= MPR_ALLOC_NUM_QUEUES) {
                qindex = -1;
            } else {
                assert(required < heap->freeq[qindex].minSize);
            }
        }
    }
    baseQindex = qindex;

    if (qindex >= 0) {
        heap->workDone += required;
    retry:
        retryIndex = -1;
        baseBindex = qindex / MPR_ALLOC_BITMAP_BITS;
        bitmap = &heap->bitmap[baseBindex];

        /*
            Non-blocking search for a free block. If contention of any kind, simply skip the queue and try the next queue.
            This races with the sweeper. Must clear the fp->blk.free bit last after fully allocating the block.
            If invoked from a foreign thread, we are also racing the marker and so heap->mark may change at any time.
            That is why we always create eternal blocks here so that foreign threads can release the eternal hold when safe.
         */
        for (bindex = baseBindex; bindex < MPR_ALLOC_NUM_BITMAPS; bitmap++, bindex++) {
            /* Mask queues lower than the base queue */
            localMap = *bitmap & ((size_t) ((uint64) -1 << max(0, (qindex - (MPR_ALLOC_BITMAP_BITS * bindex)))));

            while (localMap) {
                qindex = (bindex * MPR_ALLOC_BITMAP_BITS) + findFirstBit(localMap) - 1;
                freeq = &heap->freeq[qindex];
                ATOMIC_INC(trys);
                /* Acquire exclusive access to this queue */
                if (acquire(freeq)) {
                    if (freeq->next != (MprFreeMem*) freeq) {
                        /* Inline unlinkBlock for speed */
                        fp = freeq->next;
                        fp->prev->next = fp->next;
                        fp->next->prev = fp->prev;
                        fp->blk.qindex = 0;
                        /*
                            We've secured the free queue, so no one else can allocate this block. But racing with the sweeper.
                            But we must fully configure before clearing the "free" flag last.
                         */
                        fp->blk.eternal = 1;
                        fp->blk.mark = heap->mark;
                        assert(fp->blk.free == 1);
                        fp->blk.free = 0;
                        if (--freeq->count == 0) {
                            clearbitmap(bitmap, qindex % MPR_ALLOC_BITMAP_BITS);
                        }
                        assert(freeq->count >= 0);
                        mp = (MprMem*) fp;
                        release(freeq);
                        mprAtomicAdd64((int64*) &heap->stats.bytesFree, -(int64) mp->size);

                        if (mp->size >= (size_t) (required + MPR_ALLOC_MIN_SPLIT)) {
                            linkSpareBlock(((char*) mp) + required, mp->size - required);
                            mp->size = (MprMemSize) required;
                            ATOMIC_INC(splits);
                        }
                        if (needGC(heap)) {
                            triggerGC(0);
                        }
                        ATOMIC_INC(reuse);
                        assert(mp->size >= required);
                        assert(fp->blk.free == 0);
                        return mp;
                    } else {
                        /* Another thread raced for the last block */
                        ATOMIC_INC(race);
                        if (freeq->count == 0) {
                            clearbitmap(bitmap, qindex % MPR_ALLOC_BITMAP_BITS);
                        }
                        release(freeq);
                    }
                } else {
                    /* Contention on this queue */
                    ATOMIC_INC(tryFails);
                    if (freeq->count > 0 && retryIndex < 0) {
                        retryIndex = qindex;
                    }
                }
                /*
                    Refresh the bitmap incase threads have split or depleted suitable queues.
                    +1 to step past the current queue.
                 */
                localMap = *bitmap & ((size_t) ((uint64) -1 << max(0, (qindex + 1 - (MPR_ALLOC_BITMAP_BITS * bindex)))));
                ATOMIC_INC(qrace);
            }
        }
        /*
            Avoid growing the heap if there is a suitable block in the heap.
         */
        if (retryIndex >= 0) {
            /* Contention on a suitable queue - retry that */
            ATOMIC_INC(retries);
            qindex = retryIndex;
            dontBusyWait();
            goto retry;
        }
        if (heap->stats.bytesFree > heap->stats.lowHeap) {
            /* A suitable block may be available - try again */
            bitmap = &heap->bitmap[baseBindex];
            for (bindex = baseBindex; bindex < MPR_ALLOC_NUM_BITMAPS; bitmap++, bindex++) {
                if (*bitmap & ((size_t) ((uint64) -1 << max(0, (baseQindex - (MPR_ALLOC_BITMAP_BITS * bindex)))))) {
                    qindex = baseQindex;
                    dontBusyWait();
                    goto retry;
                }
            }
        }
    }
    return growHeap(required);
}


/*
    Grow the heap and return a block of the required size (unqueued)
 */
static MprMem *growHeap(size_t required)
{
    MprRegion   *region;
    MprMem      *mp;
    size_t      size, rsize, spareLen;

    if (required < MPR_ALLOC_MAX_BLOCK && needGC(heap)) {
        triggerGC(1);
    }
    if (required >= MPR_ALLOC_MAX) {
        allocException(MPR_MEM_TOO_BIG, required);
        return 0;
    }
    rsize = MPR_ALLOC_ALIGN(sizeof(MprRegion));
    size = max((size_t) required + rsize, (size_t) heap->regionSize);
    if ((region = mprVirtAlloc(size, MPR_MAP_READ | MPR_MAP_WRITE)) == NULL) {
        allocException(MPR_MEM_TOO_BIG, size);
        return 0;
    }
    region->size = size;
    region->start = (MprMem*) (((char*) region) + rsize);
    region->end = (MprMem*) ((char*) region + size);
    region->freeable = 0;
    mp = (MprMem*) region->start;
    spareLen = size - required - rsize;

    /*
        If a block is big, don't split the block. This improves the chances it will be unpinned.
     */
    if (spareLen < MPR_ALLOC_MIN_BLOCK || required >= MPR_ALLOC_MAX_BLOCK) {
        required = size - rsize;
        spareLen = 0;
    }
    initBlock(mp, required, 1);
    mp->eternal = 1;
    if (spareLen > 0) {
        assert(spareLen >= MPR_ALLOC_MIN_BLOCK);
        linkSpareBlock(((char*) mp) + required, spareLen);
    } else {
        mp->fullRegion = 1;
    }
    mprAtomicListInsert((void**) &heap->regions, (void**) &region->next, region);
    ATOMIC_ADD(bytesAllocated, size);
    /*
        Compute peak heap stats. Not an accurate stat - tolerate races.
     */
    if (heap->stats.bytesAllocated > heap->stats.bytesAllocatedPeak) {
        heap->stats.bytesAllocatedPeak = heap->stats.bytesAllocated;
#if (ME_MPR_ALLOC_STATS && ME_MPR_ALLOC_DEBUG) && ME_MPR_ALLOC_TRACE
        printf("MPR: Heap new max %lld request %lu\n", heap->stats.bytesAllocatedPeak, required);
#endif
    }
    CHECK(mp);
    ATOMIC_INC(allocs);
    return mp;
}


/*
    Free a memory block back onto the freelists
 */
static void freeBlock(MprMem *mp)
{
    MprRegion   *region;

    assert(!mp->free);
    SCRIBBLE(mp);
#if CONFIG_DEBUG || ME_MPR_ALLOC_STATS
    heap->stats.swept++;
    heap->stats.sweptBytes += mp->size;
#endif
    heap->freedBlocks = 1;
#if ME_MPR_ALLOC_STATS
    heap->stats.freed += mp->size;
#endif
    /*
        If memory block is first in the region, check if the entire region is free
     */
    if (mp->first) {
        region = GET_REGION(mp);
        if (GET_NEXT(mp) >= region->end) {
            if (mp->fullRegion || heap->stats.bytesFree >= heap->stats.cacheHeap) {
                region->freeable = 1;
                return;
            }
        }
    }
    linkBlock(mp);
}


/*
    Map a queue index to a block size. This size includes the MprMem header.
 */
static ME_INLINE size_t qtosize(int qindex)
{
    size_t  size;
    int     high, low;

    high = qindex / MPR_ALLOC_NUM_QBITS;
    low = qindex % MPR_ALLOC_NUM_QBITS;
    if (high) {
        low += MPR_ALLOC_NUM_QBITS;
    }
    high = max(0, high - 1);
    size = (low << high) << ME_MPR_ALLOC_ALIGN_SHIFT;
    size += sizeof(MprMem);
    return size;
}


/*
    Map a block size to a queue index. The block size includes the MprMem header. However, determine the free queue
    based on user sizes (sans header). This permits block searches to avoid scanning the next highest queue for
    common block sizes: eg. 1K.
 */
static ME_INLINE int sizetoq(size_t size)
{
    size_t      asize;
    int         msb, shift, high, low, qindex;

    assert(MPR_ALLOC_ALIGN(size) == size);

    if (size > MPR_ALLOC_MAX_BLOCK) {
        /* Large block, don't put on queues */
        return -1;
    }
    size -= sizeof(MprMem);
    asize = (size >> ME_MPR_ALLOC_ALIGN_SHIFT);
    msb = findLastBit(asize) - 1;
    high = max(0, msb - MPR_ALLOC_QBITS_SHIFT + 1);
    shift = max(0, high - 1);
    low = (asize >> shift) & (MPR_ALLOC_NUM_QBITS - 1);
    qindex = (high * MPR_ALLOC_NUM_QBITS) + low;
    assert(qindex < MPR_ALLOC_NUM_QUEUES);
    return qindex;
}


/*
    Add a block to a free q. Called by user threads from allocMem and by sweeper from freeBlock.
    WARNING: Must be called with the freelist not acquired. This is the opposite of unlinkBlock.
 */
static ME_INLINE bool linkBlock(MprMem *mp)
{
    MprFreeQueue    *freeq;
    MprFreeMem      *fp;
    ssize           size;
    int             qindex;

    CHECK(mp);

    size = mp->size;
    qindex = sizetoq(size);
    assert(qindex >= 0);
    freeq = &heap->freeq[qindex];

    /*
        Acquire the free queue. Racing with multiple-threads in allocMem(). If we fail to acquire, the sweeper
        will retry next time. Note: the bitmap is updated with the queue acquired to safeguard the integrity of
        this queue's free bit.
     */
    ATOMIC_INC(trys);
    if (!acquire(freeq)) {
        ATOMIC_INC(tryFails);
        mp->mark = !mp->mark;
        assert(!mp->free);
        return 0;
    }
    assert(qindex >= 0);
    mp->qindex = qindex;
    mp->hasManager = 0;
    fp = (MprFreeMem*) mp;
    fp->next = freeq->next;
    fp->prev = (MprFreeMem*) freeq;
    freeq->next->prev = fp;
    freeq->next = fp;
    freeq->count++;
    setbitmap(&heap->bitmap[mp->qindex / MPR_ALLOC_BITMAP_BITS], mp->qindex % MPR_ALLOC_BITMAP_BITS);
    mp->free = 1;
    release(freeq);
    mprAtomicAdd64((int64*) &heap->stats.bytesFree, size);
    return 1;
}


/*
    Remove a block from a free q.
    WARNING: Must be called with the freelist acquired.
 */
static ME_INLINE void unlinkBlock(MprMem *mp)
{
    MprFreeQueue    *freeq;
    MprFreeMem      *fp;

    fp = (MprFreeMem*) mp;
    fp->prev->next = fp->next;
    fp->next->prev = fp->prev;
    assert(mp->qindex);
    freeq = &heap->freeq[mp->qindex];
    freeq->count--;
    mp->qindex = 0;
#if ME_MPR_ALLOC_DEBUG
    fp->next = fp->prev = NULL;
#endif
    mprAtomicAdd64((int64*) &heap->stats.bytesFree, -(int64) mp->size);
}


/*
    This must be robust. i.e. the spare block memory must end up on the freeq.
 */
static ME_INLINE void linkSpareBlock(char *ptr, size_t size)
{
    MprMem  *mp;
    size_t  len;

    assert(size >= MPR_ALLOC_MIN_BLOCK);
    mp = (MprMem*) ptr;
    len = size;

    while (size > 0) {
        initBlock(mp, len, 0);
        if (!linkBlock(mp)) {
            /* Cannot acquire queue. Break into pieces and try lesser queue */
            if (len >= (MPR_ALLOC_MIN_BLOCK * 8)) {
                len = MPR_ALLOC_ALIGN(len / 2);
                len = min(size, len);
            } else {
                dontBusyWait();
            }
        } else {
            size -= len;
            mp = (MprMem*) ((char*) mp + len);
            len = size;
        }
    }
    assert(size == 0);
}


/*
    Allocate virtual memory and check a memory allocation request against configured maximums and redlines.
    An application-wide memory allocation failure routine can be invoked from here when a memory redline is exceeded.
    It is the application's responsibility to set the red-line value suitable for the system.
    Memory is zereod on all platforms.
 */
PUBLIC void *mprVirtAlloc(size_t size, int mode)
{
    size_t      used;
    void        *ptr;

    used = mprGetMem();
    if (memStats.pageSize) {
        size = MPR_PAGE_ALIGN(size, memStats.pageSize);
    }
    if ((size + used) > heap->stats.maxHeap) {
        allocException(MPR_MEM_LIMIT, size);

    } else if ((size + used) > heap->stats.warnHeap) {
        allocException(MPR_MEM_WARNING, size);
    }
    if ((ptr = vmalloc(size, mode)) == 0) {
        allocException(MPR_MEM_FAIL, size);
        return 0;
    }
    return ptr;
}


PUBLIC void mprVirtFree(void *ptr, size_t size)
{
    vmfree(ptr, size);
}


static void *vmalloc(size_t size, int mode)
{
    void    *ptr;

#if ME_MPR_ALLOC_VIRTUAL
    if ((ptr = mmap(0, size, mode, MAP_PRIVATE | MAP_ANON, -1, 0)) == (void*) -1) {
        return 0;
    }
#else
    if ((ptr = malloc(size)) != 0) {
        memset(ptr, 0, size);
    }
#endif
    return ptr;
}


static void vmfree(void *ptr, size_t size)
{
#if ME_MPR_ALLOC_VIRTUAL
    munmap(ptr, size);
#else
    free(ptr);
#endif
}


/***************************************************** Garbage Colllector *************************************************/

PUBLIC void mprStartGCService()
{
    if (heap->gcEnabled) {
        if ((heap->sweeper = mprCreateThread("sweeper", sweeperThread, NULL, 0)) == 0) {
            mprLog("critical mpr memory", 0, "Cannot create sweeper thread");
            MPR->hasError = 1;
        } else {
            mprStartThread(heap->sweeper);
        }
    }
}


PUBLIC void mprStopGCService()
{
    int     i;

    mprWakeGCService();
    for (i = 0; heap->sweeper && i < MPR_TIMEOUT_STOP; i++) {
        mprNap(1);
    }
    invokeAllDestructors();
}


PUBLIC void mprWakeGCService()
{
    mprSignalCond(heap->gcCond);
}


static ME_INLINE void triggerGC(int always)
{
    if (always || (!heap->gcRequested && heap->gcEnabled)) {
        heap->gcRequested = 1;
        heap->mustYield = 1;
        mprSignalCond(heap->gcCond);
    }
}


/*
    Trigger a GC collection worthwhile. If MPR_GC_FORCE is set, force the collection regardless. Flags:

    MPR_CG_DEFAULT      Run GC if necessary. Will yield and block for GC
    MPR_GC_FORCE        Force a GC whether it is required or not
    MPR_GC_NO_BLOCK     Run GC if necessary and return without yielding
    MPR_GC_COMPLETE     Force a GC and wait until all threads yield and GC completes including sweeper
 */
PUBLIC int mprGC(int flags)
{
    heap->freedBlocks = 0;
    if (!(flags & MPR_GC_NO_BLOCK)) {
        /*
            Yield here, so the sweeper can operate now
         */
        mprYield(MPR_YIELD_STICKY);
    }
    if ((flags & (MPR_GC_FORCE | MPR_GC_COMPLETE)) || needGC(heap)) {
        triggerGC(flags & (MPR_GC_FORCE | MPR_GC_COMPLETE));
    }
    if (!(flags & MPR_GC_NO_BLOCK)) {
        mprResetYield();
        mprYield((flags & MPR_GC_COMPLETE) ? MPR_YIELD_COMPLETE : 0);
    }
    return MPR->heap->freedBlocks;
}


/*
    Called by user code to signify the thread is ready for GC and all object references are saved.  Flags:

    MPR_YIELD_DEFAULT   If GC is required, yield and wait for mark phase to coplete, otherwise return without blocking.
    MPR_YIELD_COMPLETE  Yield and wait until the GC entirely completes including sweeper.
    MPR_YIELD_STICKY    Yield and remain yielded until reset. Does not block.

    A yielding thread may block for up to MPR_TIMEOUT_GC_SYNC (1/10th sec) for other threads to also yield. If one more
    more threads do not yield, the marker will resume all yielded threads. If all threads yield, they will wait until
    the mark phase has completed and then be resumed by the marker.
 */
PUBLIC void mprYield(int flags)
{
    MprThreadService    *ts;
    MprThread           *tp;

    ts = MPR->threadService;
    if ((tp = mprGetCurrentThread()) == 0) {
        mprLog("error mpr memory", 0, "Yield called from an unknown thread");
        return;
    }
    assert(!tp->waiting);
    assert(!tp->yielded);
    assert(!tp->stickyYield);

    if (tp->noyield) {
        return;
    }
    if (flags & MPR_YIELD_STICKY) {
        tp->stickyYield = 1;
        tp->yielded = 1;
    }
    /*
        Double test to be lock free for the common case
     */
    if (heap->mustYield && heap->sweeper) {
        lock(ts->threads);
        tp->waitForSweeper = (flags & MPR_YIELD_COMPLETE);
        while (heap->mustYield) {
            tp->yielded = 1;
            tp->waiting = 1;
            unlock(ts->threads);

            mprSignalCond(ts->pauseThreads);
            if (tp->stickyYield) {
                tp->waiting = 0;
                return;
            }
            mprWaitForCond(tp->cond, -1);
            lock(ts->threads);
            tp->waiting = 0;
            if (tp->yielded && !tp->stickyYield) {
                /*
                    WARNING: this wait above may return without tp->yielded having been cleared.
                    This can happen because the cond may have already been triggered by a
                    previous sticky yield. i.e. it did not wait.
                 */
                tp->yielded = 0;
            }
        }
        unlock(ts->threads);
    }
    if (!tp->stickyYield) {
        assert(!tp->yielded);
        assert(!heap->marking);
    }
    assert(!tp->waiting);
}


#ifndef mprNeedYield
PUBLIC bool mprNeedYield()
{
    return heap->mustYield;
}
#endif


PUBLIC void mprResetYield()
{
    MprThreadService    *ts;
    MprThread           *tp;

    ts = MPR->threadService;
    if ((tp = mprGetCurrentThread()) == 0) {
        mprLog("error mpr memory", 0, "Yield called from an unknown thread");
        return;
    }
    assert(tp->stickyYield || tp->noyield);
    if (tp->stickyYield) {
        /*
            Marking could have started again while sticky yielded. So must yield here regardless.
         */
        lock(ts->threads);
        tp->stickyYield = 0;
        if (heap->marking) {
            tp->yielded = 0;
            unlock(ts->threads);

            mprYield(0);
            assert(!tp->yielded);
        } else {
            tp->yielded = 0;
            unlock(ts->threads);
        }
    }
    assert(!tp->yielded);
}


/*
    Pause until all threads have yielded. Called by the GC marker only.
 */
static int pauseThreads()
{
    MprThreadService    *ts;
    MprThread           *tp;
    MprTicks            start;
    int                 i, allYielded, timeout, noyield;

    /*
        Short timeout wait for all threads to yield. Typically set to 1/10 sec
     */
    heap->mustYield = 1;
    timeout = MPR_TIMEOUT_GC_SYNC;
    if ((ts = MPR->threadService) == 0 || !ts->threads) {
        return 0;
    }
    start = mprGetTicks();
    if (mprGetDebugMode()) {
        timeout = timeout * 500;
    }
    do {
        lock(ts->threads);
        allYielded = 1;
        noyield = 0;
        for (i = 0; i < ts->threads->length; i++) {
            tp = (MprThread*) mprGetItem(ts->threads, i);
            if (!tp->yielded) {
                allYielded = 0;
                if (tp->noyield) {
                    noyield = 1;
                }
                break;
            }
        }
        if (allYielded) {
            heap->marking = 1;
            unlock(ts->threads);
            break;
        }
        unlock(ts->threads);
        if (noyield || mprGetState() >= MPR_DESTROYING) {
            /* Do not wait for paused threads if shutting down */
            break;
        }
        mprWaitForCond(ts->pauseThreads, 20);

    } while (mprGetElapsedTicks(start) < timeout);

    return (allYielded) ? 1 : 0;
}


static void resumeThreads(int flags)
{
    MprThreadService    *ts;
    MprThread           *tp;
    int                 i;

    if ((ts = MPR->threadService) == 0 || !ts->threads) {
        return;
    }
    lock(ts->threads);
    heap->mustYield = 0;
    for (i = 0; i < ts->threads->length; i++) {
        tp = (MprThread*) mprGetItem(ts->threads, i);
        if (tp && tp->yielded) {
            if (flags == WAITING_THREADS && !tp->waitForSweeper) {
                continue;
            }
            if (flags == YIELDED_THREADS && tp->waitForSweeper) {
                continue;
            }
            if (!tp->stickyYield) {
                tp->yielded = 0;
            }
            tp->waitForSweeper = 0;
            if (tp->waiting) {
                assert(tp->stickyYield || !tp->yielded);
                mprSignalCond(tp->cond);
            }
        }
    }
    unlock(ts->threads);
}


/*
    Garbage collector main thread
 */
static void sweeperThread(void *unused, MprThread *tp)
{
    tp->stickyYield = 1;
    tp->yielded = 1;

    while (!mprIsDestroyed()) {
        if (!heap->mustYield) {
            heap->gcRequested = 0;
            mprWaitForCond(heap->gcCond, -1);
        }
        if (mprIsDestroyed()) {
            heap->mustYield = 0;
            continue;
        }
        markAndSweep();
    }
    invokeDestructors();
    resumeThreads(YIELDED_THREADS | WAITING_THREADS);
    heap->sweeper = 0;
}


/*
    The mark phase will run with all user threads yielded. The sweep phase then runs in parallel.
    The mark phase is relatively quick.
 */
static void markAndSweep()
{
    MprThreadService    *ts;
    int                 threadCount;

    ts = MPR->threadService;
    threadCount = ts->threads->length;

    if (!pauseThreads()) {
#if ME_MPR_ALLOC_STATS && ME_MPR_ALLOC_DEBUG && MPR_ALLOC_TRACE
        static int warnOnce = 0;
        if (warnOnce == 0 && !mprGetDebugMode() && !mprIsStopping()) {
            warnOnce = 1;
            mprLog("error mpr memory", 6, "GC synchronization timed out, some threads did not yield.");
            mprLog("error mpr memory", 6, "If debugging, run the process with -D to enable debug mode.");
        }
#endif
        resumeThreads(YIELDED_THREADS | WAITING_THREADS);
        return;
    }

    /*
        Mark used memory. Toggle the in-use heap->mark for each collection.
        Assert global lock around marking and changing heap->mark so that routines in foreign threads (like httpCreateEvent)
        can stop GC when creating events.
     */
    mprGlobalLock();
    heap->mark = !heap->mark;
    markRoots();
    heap->marking = 0;
    heap->priorWorkDone = heap->workDone;
    heap->workDone = 0;
    mprGlobalUnlock();

    /*
        Sweep unused memory. If less than 4 threds, sweeping goes on in parallel with running threads.
        Otherwise, with high thread loads, the sweeper can be starved which leads to memory growth.
     */
    heap->sweeping = 1;

    if (threadCount < 4) {
        resumeThreads(YIELDED_THREADS);
    }
    sweep();
    heap->sweeping = 0;

    /*
        Resume threads waiting for the sweeper to complete
     */
    if (threadCount >= 4) {
        resumeThreads(YIELDED_THREADS);
    }
    resumeThreads(WAITING_THREADS);
}


static void markRoots()
{
#if ME_MPR_ALLOC_STATS
    heap->stats.markVisited = 0;
    heap->stats.marked = 0;
#endif
    mprMark(heap->roots);
    mprMark(heap->gcCond);
}


static void invokeDestructors()
{
    MprRegion   *region;
    MprMem      *mp;
    MprManager  mgr;
    uchar       eternal, mark;

    for (region = heap->regions; region; region = region->next) {
        for (mp = region->start; mp < region->end; mp = GET_NEXT(mp)) {
            /*
                Examine all freeable (allocated and not marked with by a reference) memory with destructors.
                Order matters: racing with allocator. The allocator sets free last. mprRelease sets
                eternal last and uses mprAtomicStore to ensure mp->mark is committed.
             */
            mprAtomicLoad(&mp->eternal, &eternal, MPR_ATOMIC_ACQUIRE);
            if (mp->hasManager && !mp->free && !mp->eternal) {
                mprAtomicLoad(&mp->mark, &mark, MPR_ATOMIC_ACQUIRE);
                if (mark != heap->mark) {
                    mgr = GET_MANAGER(mp);
                    if (mgr) {
                        assert(!mp->eternal);
                        assert(!mp->free);
                        assert(mp->mark != heap->mark);
                        (mgr)(GET_PTR(mp), MPR_MANAGE_FREE);
                        // Retest incase the manager routine revived the object
                        if (mp->mark != heap->mark) {
                            mp->hasManager = 0;
                        }
                    }
                }
            }
        }
    }
}


static void invokeAllDestructors()
{
#if FUTURE
    MprRegion   *region;
    MprMem      *mp;
    MprManager  mgr;

    if (MPR->flags & MPR_NOT_ALL) {
        return;
    }
    for (region = heap->regions; region; region = region->next) {
        for (mp = region->start; mp < region->end; mp = GET_NEXT(mp)) {
            if (!mp->free && mp->hasManager) {
                mgr = GET_MANAGER(mp);
                if (mgr) {
                    (mgr)(GET_PTR(mp), MPR_MANAGE_FREE);
                    /* Retest incase the manager routine revied the object */
                    if (mp->mark != heap->mark) {
                        mp->hasManager = 0;
                    }
                }
            }
        }
    }
#endif
}


/*
    Claim a block from its freeq for the sweeper. This removes the block from the freeq and clears the "free" bit.
 */
static ME_INLINE bool claim(MprMem *mp)
{
    MprFreeQueue    *freeq;
    int             qindex;

    if ((qindex = mp->qindex) == 0) {
        /* allocator won the race */
        return 0;
    }
    freeq = &heap->freeq[qindex];
    ATOMIC_INC(trys);
    if (!acquire(freeq)) {
        ATOMIC_INC(tryFails);
        return 0;
    }
    if (mp->qindex != qindex) {
        /* No on this queue. Allocator must have claimed this block */
        release(freeq);
        return 0;
    }
    unlinkBlock(mp);
    assert(mp->free);
    mp->free = 0;
    release(freeq);
    return 1;
}


/*
    Sweep up the garbage. The sweeper runs in parallel with the program. Dead blocks will have (MprMem.mark != heap->mark).
*/
static void sweep()
{
    MprRegion   *region, *nextRegion, *prior, *rp;
    MprMem      *mp, *next;
    int         joinBlocks, rcount;

    if (!heap->gcEnabled) {
        return;
    }
#if ME_MPR_ALLOC_STATS
    heap->priorFree = heap->stats.bytesFree;
    heap->stats.sweepVisited = 0;
    heap->stats.freed = 0;
    heap->stats.collections++;
#endif
#if CONFIG_DEBUG || ME_MPR_ALLOC_STATS
    heap->stats.swept = 0;
    heap->stats.sweptBytes = 0;
#endif
    /*
        First run managers so that dependant memory blocks will still exist when the manager executes.
        Actually free the memory in a 2nd pass below.
     */
    invokeDestructors();

    /*
        RACE: Racing with growHeap. This traverses the region list lock-free. growHeap() will insert new regions to
        the front of heap->regions. This code is the only code that frees regions.
     */
    prior = NULL;
    rcount = 0;
    for (region = heap->regions; region; region = nextRegion) {
        nextRegion = region->next;
        joinBlocks = heap->stats.bytesFree >= heap->stats.cacheHeap;

        for (mp = region->start; mp < region->end; mp = next) {
            assert(mp->size > 0);
            next = GET_NEXT(mp);
            assert(next != mp);
            CHECK(mp);
            INC(sweepVisited);

            /*
                Racing with the allocator. Be conservative. The sweeper is the only place that mp->free is set.
                The allocator is the only place that clears mp->free. If mp->free is zero, we can be sure the block is
                not free and not on a freeq. If mp->free is set, we could be racing with the allocator for the block.
             */
            if (mp->eternal) {
                assert(!region->freeable);
                continue;
            }
            if (mp->free && joinBlocks) {
                /*
                    Coalesce already free blocks if the next is unreferenced and we can claim the block (racing with allocator).
                    Claim the block and then mark it as unreferenced. Then the code below will join the blocks.
                 */
                if (next < region->end && !next->free && next->mark != heap->mark && claim(mp)) {
                    mp->mark = !heap->mark;
                    INC(compacted);
                }
            }
            /*
                Test that the block was not marked on the current mark phase
             */
            if (!mp->free && mp->mark != heap->mark) {
                freeLocation(mp);
                if (joinBlocks) {
                    /*
                        Try to join this block with successors
                     */
                    while (next < region->end && !next->eternal) {
                        if (next->free) {
                            /*
                                Block is free and on a freeq - must claim as racing with the allocator for the block
                             */
                            if (!claim(next)) {
                                break;
                            }
                            mp->size += next->size;
                            freeLocation(next);
                            assert(!next->free);
                            SCRIBBLE_RANGE(next, MPR_ALLOC_MIN_BLOCK);
                            INC(joins);

                        } else if (next->mark != heap->mark) {
                            /*
                                Block not in use and NOT on a freeq - no need to claim
                             */
                            assert(!next->free);
                            assert(next->qindex == 0);
                            mp->size += next->size;
                            freeLocation(next);
                            SCRIBBLE_RANGE(next, MPR_ALLOC_MIN_BLOCK);
                            INC(joins);

                        } else {
                            break;
                        }
                        next = GET_NEXT(mp);
                    }
                }
                freeBlock(mp);
            }
        }
        if (region->freeable) {
            if (prior) {
                prior->next = nextRegion;
            } else {
                if (!mprAtomicCas((void**) &heap->regions, region, nextRegion)) {
                    prior = 0;
                    for (rp = heap->regions; rp != region; prior = rp, rp = rp->next) { }
                    assert(prior);
                    if (prior) {
                        prior->next = nextRegion;
                    }
                }
            }
            ATOMIC_ADD(bytesAllocated, - (int64) region->size);
            mprVirtFree(region, region->size);
            INC(unpins);
        } else {
            prior = region;
            rcount++;
        }
    }
    heap->stats.heapRegions = rcount;
    heap->stats.sweeps++;
#if ME_MPR_ALLOC_STATS && ME_MPR_ALLOC_DEBUG && MPR_ALLOC_TRACE
    printf("GC: Marked %lld / %lld, Swept %lld / %lld, freed %lld, bytesFree %lld (prior %lld)\n"
                 "    WeightedCount %d / %d, allocated blocks %lld allocated bytes %lld\n"
                 "    Unpins %lld, Collections %lld\n",
        heap->stats.marked, heap->stats.markVisited, heap->stats.swept, heap->stats.sweepVisited,
        heap->stats.freed, heap->stats.bytesFree, heap->priorFree, heap->priorWorkDone, heap->workQuota,
        heap->stats.sweepVisited - heap->stats.swept, heap->stats.bytesAllocated, heap->stats.unpins,
        heap->stats.collections);
    printf("Swept blocks %lld bytes %lld, workDone %d\n", heap->stats.swept, heap->stats.sweptBytes, heap->priorWorkDone);
#endif
    if (heap->printStats) {
        printMemReport();
        heap->printStats = 0;
    }
}


/*
    Permanent allocation. Immune to garbage collector.
 */
void *palloc(size_t size)
{
    return mprAllocMem(size, MPR_ALLOC_HOLD | MPR_ALLOC_ZERO);
}


/*
    Normal free. Note: this must not be called with a block allocated via "malloc".
    No harm in calling this on a block allocated with mprAlloc and not "palloc".
 */
PUBLIC void pfree(void *ptr)
{
    if (ptr) {
        mprRelease(ptr);
        /* Do not access ptr here - async sweeper() may have already run */
    }
}


PUBLIC void *prealloc(void *ptr, size_t size)
{
    void    *mem;
    size_t  oldSize;

    oldSize = psize(ptr);
    if (size <= oldSize) {
        return ptr;
    }
    if ((mem = mprAllocMem(size, MPR_ALLOC_HOLD | MPR_ALLOC_ZERO)) == 0) {
        return 0;
    }
    if (ptr) {
        memcpy(mem, ptr, oldSize);
        mprRelease(ptr);
    }
    return mem;
}


PUBLIC size_t psize(void *ptr)
{
    return mprGetBlockSize(ptr);
}


/*
    WARNING: this does not mark component members. If that is required, use mprAddRoot.
    WARNING: this should only ever be used by MPR threads that are not yielded when this API is called.
 */
PUBLIC void mprHold(cvoid *ptr)
{
    MprMem  *mp;
    uchar   one;

    if (ptr) {
        mp = GET_MEM(ptr);
        assert(!mp->free);
        assert(!mp->eternal);
        assert(mp->mark == MPR->heap->mark);

        if (!mp->free && VALID_BLK(mp)) {
            // mp->eternal = 1;
            one = 1;
            mprAtomicStore(&mp->eternal, &one, MPR_ATOMIC_RELEASE);
            assert(mp->eternal);
        }
    }
}


PUBLIC void mprRelease(cvoid *ptr)
{
    MprMem       *mp;
    static uchar zero = 0;

    if (ptr) {
        mp = GET_MEM(ptr);
        assert(mp->eternal);
        assert(!mp->free);

        if (!mp->free && VALID_BLK(mp)) {
            /*
                For memory allocated in foreign threads, there could be a race where the release missed the GC mark phase
                and the sweeper is or is about to run. We simulate a GC mark here to prevent the sweeper from collecting
                the block on this sweep. The block will be collected on the next sweep if there is no other reference.
                Note: this races with the sweeper (invokeDestructors) so must set the mark first and clear eternal
                after that with an ATOMIC_RELEASE barrier to ensure the mark change is committed.
             */
            mprAtomicStore(&mp->mark, &heap->mark, MPR_ATOMIC_RELEASE);
            mprAtomicStore(&mp->eternal, &zero, MPR_ATOMIC_RELEASE);
        }
    }
}


/*
    WARNING: this does not mark component members. If that is required, use mprAddRoot.
 */
PUBLIC void mprHoldBlocks(cvoid *ptr, ...)
{
    va_list args;

    if (ptr) {
        mprHold(ptr);
        va_start(args, ptr);
        while ((ptr = va_arg(args, char*)) != 0) {
            mprHold(ptr);
        }
        va_end(args);
    }
}


PUBLIC void mprReleaseBlocks(cvoid *ptr, ...)
{
    va_list     args;

    if (ptr) {
        va_start(args, ptr);
        mprRelease(ptr);
        while ((ptr = va_arg(args, char*)) != 0) {
            mprRelease(ptr);
        }
        va_end(args);
    }
}


PUBLIC bool mprEnableGC(bool on)
{
    bool    old;

    old = heap->gcEnabled;
    heap->gcEnabled = on;
    return old;
}


PUBLIC void mprAddRoot(cvoid *root)
{
    if (root) {
        mprAddItem(heap->roots, root);
    }
}


PUBLIC void mprRemoveRoot(cvoid *root)
{
    mprRemoveItem(heap->roots, root);
}


/****************************************************** Debug *************************************************************/

#if ME_MPR_ALLOC_STATS
static void printQueueStats()
{
    MprFreeQueue    *freeq;
    double          mb;
    int             i;

    mb = 1024.0 * 1024;
    /*
        Note the total size is a minimum as blocks may be larger than minSize
     */
    printf("\nFree Queue Stats\n  Queue           Usize         Count          Total\n");
    for (i = 0, freeq = heap->freeq; freeq < &heap->freeq[MPR_ALLOC_NUM_QUEUES]; freeq++, i++) {
        if (freeq->count) {
            printf("%7d %14d %14d %14d\n", i, freeq->minSize - (int) sizeof(MprMem), freeq->count,
                freeq->minSize * freeq->count);
        }
    }
    printf("\n");
    printf("Heap-used    %8.1f MB\n", (heap->stats.bytesAllocated - heap->stats.bytesFree) / mb);
}


#if ME_MPR_ALLOC_DEBUG
static MprLocationStats sortLocations[MPR_TRACK_HASH];

static int sortLocation(cvoid *l1, cvoid *l2)
{
    MprLocationStats    *lp1, *lp2;

    lp1 = (MprLocationStats*) l1;
    lp2 = (MprLocationStats*) l2;
    if (lp1->total < lp2->total) {
        return -1;
    } else if (lp1->total == lp2->total) {
        return 0;
    }
    return 1;
}


static void printTracking()
{
    MprLocationStats    *lp;
    double              mb;
    size_t              total;
    cchar               **np;

    printf("\nAllocation Stats\n     Size Location\n");
    memcpy(sortLocations, heap->stats.locations, sizeof(sortLocations));
    qsort(sortLocations, MPR_TRACK_HASH, sizeof(MprLocationStats), sortLocation);

    total = 0;
    for (lp = sortLocations; lp < &sortLocations[MPR_TRACK_HASH]; lp++) {
        if (lp->total) {
            for (np = &lp->names[0]; *np && np < &lp->names[MPR_TRACK_NAMES]; np++) {
                if (*np) {
                    if (np == lp->names) {
                        printf("%10d %-24s %d\n", (int) lp->total, *np, lp->count);
                    } else {
                        printf("           %-24s\n", *np);
                    }
                }
            }
            total += lp->total;
        }
    }
    mb = 1024.0 * 1024;
    printf("Total:    %8.1f MB\n", total / (1024.0 * 1024));
    printf("Heap-used %8.1f MB\n", (MPR->heap->stats.bytesAllocated - MPR->heap->stats.bytesFree) / mb);
}
#endif /* ME_MPR_ALLOC_DEBUG */


static void printGCStats()
{
    MprRegion   *region;
    MprMem      *mp;
    uint64      freeBytes, activeBytes, eternalBytes, regionBytes, available;
    double      mb;
    char        *tag;
    int         regions, freeCount, activeCount, eternalCount, regionCount;

    mb = 1024.0 * 1024;
    printf("\nRegion Stats:\n");
    regions = 0;
    activeBytes = eternalBytes = freeBytes = 0;
    activeCount = eternalCount = freeCount = 0;

    for (region = heap->regions; region; region = region->next, regions++) {
        regionCount = 0;
        regionBytes = 0;

        for (mp = region->start; mp < region->end; mp = GET_NEXT(mp)) {
            assert(mp->size > 0);
            if (mp->free) {
                freeBytes += mp->size;
                freeCount++;

            } else if (mp->eternal) {
                eternalBytes += mp->size;
                eternalCount++;
                regionCount++;
                regionBytes += mp->size;

            } else {
                activeBytes += mp->size;
                activeCount++;
                regionCount++;
                regionBytes += mp->size;
            }
        }
        available = region->size - regionBytes - MPR_ALLOC_ALIGN(sizeof(MprRegion));
        if (available == 0) {
            tag = "(fully used)";
        } else if (regionBytes == 0) {
            tag = "(empty)";
        } else {
            tag = "";
        }
        printf("  Region %2d size %d, allocated %4d blocks, %7d bytes free %s\n", regions, (int) region->size,
            regionCount, (int) available, tag);
    }
    printf("\nGC Stats:\n");
    printf("  Active:  %8d blocks, %6.1f MB\n", activeCount, activeBytes / mb);
    printf("  Eternal: %8d blocks, %6.1f MB\n", eternalCount, eternalBytes / mb);
    printf("  Free:    %8d blocks, %6.1f MB\n", freeCount, freeBytes / mb);
}
#endif /* ME_MPR_ALLOC_STATS */


PUBLIC void mprPrintMem(cchar *msg, int flags)
{
    printf("%s:\n\n", msg);
    heap->printStats = (flags & MPR_MEM_DETAIL) ? 2 : 1;
    mprGC(MPR_GC_FORCE | MPR_GC_COMPLETE);
}


static void printMemReport()
{
    MprMemStats     *ap;
    double          mb;
    int             fd;

    ap = mprGetMemStats();
    mb = 1024.0 * 1024;

    fd = open("/dev/null", O_RDONLY);
    close(fd);

    printf("Memory Stats:\n");
    printf("  Memory          %12.1f MB\n", mprGetMem() / mb);
    printf("  Heap            %12.1f MB\n", ap->bytesAllocated / mb);
    printf("  Heap-peak       %12.1f MB\n", ap->bytesAllocatedPeak / mb);
    printf("  Heap-used       %12.1f MB\n", (ap->bytesAllocated - ap->bytesFree) / mb);
    printf("  Heap-free       %12.1f MB\n", ap->bytesFree / mb);
    printf("  Heap cache      %12.1f MB (%.2f %%)\n", ap->cacheHeap / mb, ap->cacheHeap * 100.0 / ap->maxHeap);

    if (ap->maxHeap == (size_t) -1) {
        printf("  Heap limit         unlimited\n");
        printf("  Heap readline      unlimited\n");
    } else {
        printf("  Heap limit      %12.1f MB\n", ap->maxHeap / mb);
        printf("  Heap redline    %12.1f MB\n", ap->warnHeap / mb);
    }
    printf("  Errors          %12d\n", (int) ap->errors);
    printf("  CPU cores       %12d\n", (int) ap->cpuCores);
    printf("  Next free fd    %12d\n", (int) fd);
    printf("\n");

#if ME_MPR_ALLOC_STATS
    printf("Allocator Stats:\n");
    printf("  Memory requests %12d\n",                (int) ap->requests);
    printf("  Region allocs   %12.2f %% (%d)\n",      ap->allocs * 100.0 / ap->requests, (int) ap->allocs);
    printf("  Region unpins   %12.2f %% (%d)\n",      ap->unpins * 100.0 / ap->requests, (int) ap->unpins);
    printf("  Reuse           %12.2f %%\n",           ap->reuse * 100.0 / ap->requests);
    printf("  Joins           %12.2f %% (%d)\n",      ap->joins * 100.0 / ap->requests, (int) ap->joins);
    printf("  Splits          %12.2f %% (%d)\n",      ap->splits * 100.0 / ap->requests, (int) ap->splits);
    printf("  Q races         %12.2f %% (%d)\n",      ap->qrace * 100.0 / ap->requests, (int) ap->qrace);
    printf("  Q contention    %12.2f %% (%d / %d)\n", ap->tryFails * 100.0 / ap->trys, (int) ap->tryFails, (int) ap->trys);
    printf("  Alloc retries   %12.2f %% (%d / %d)\n", ap->retries * 100.0 / ap->requests, (int) ap->retries, (int) ap->requests);
    printf("  GC collections  %12.2f %% (%d)\n",      ap->collections * 100.0 / ap->requests, (int) ap->collections);
    printf("  Compact next    %12.2f %% (%d)\n",      ap->compacted * 100.0 / ap->requests, (int) ap->compacted);
    printf("  MprMem size     %12d\n",                (int) sizeof(MprMem));
    printf("  MprFreeMem size %12d\n",                (int) sizeof(MprFreeMem));

    printGCStats();
    if (heap->printStats > 1) {
        printQueueStats();
#if ME_MPR_ALLOC_DEBUG
        if (heap->track) {
            printTracking();
        }
#endif
    }
#endif /* ME_MPR_ALLOC_STATS */
}


#if ME_MPR_ALLOC_DEBUG
static int validBlk(MprMem *mp)
{
    assert(mp->magic == MPR_ALLOC_MAGIC);
    assert(mp->size > 0);
    return (mp->magic == MPR_ALLOC_MAGIC) && (mp->size > 0);
}


PUBLIC void mprCheckBlock(MprMem *mp)
{
    BREAKPOINT(mp);
    if (mp->magic != MPR_ALLOC_MAGIC || mp->size == 0) {
        mprLog("critical mpr memory", 0, "Memory corruption in memory block %p (MprBlk %p, seqno %d). " \
            "This most likely happend earlier in the program execution.", GET_PTR(mp), mp, mp->seqno);
    }
}


static void breakpoint(MprMem *mp)
{
    if (mp == stopAlloc || mp->seqno == stopSeqno) {
        mprBreakpoint();
    }
}


#if ME_MPR_ALLOC_DEBUG
/*
    Called to set the memory block name when doing an allocation
 */
PUBLIC void *mprSetAllocName(void *ptr, cchar *name)
{
    MprMem  *mp;

    assert(name && *name);

    mp = GET_MEM(ptr);
    mp->name = name;

    if (heap->track) {
        MprLocationStats    *lp;
        cchar               **np, *n;
        int                 index;

        if (name == 0) {
            name = "";
        }
        index = shash(name, strlen(name)) % MPR_TRACK_HASH;
        lp = &heap->stats.locations[index];
        for (np = lp->names; np <= &lp->names[MPR_TRACK_NAMES]; np++) {
            n = *np;
            if (n == 0 || n == name || strcmp(n, name) == 0) {
                break;
            }
            /* Collision */
        }
        if (np < &lp->names[MPR_TRACK_NAMES]) {
            *np = (char*) name;
        }
        mprAtomicAdd64((int64*) &lp->total, mp->size);
        mprAtomicAdd(&lp->count, 1);
    }
    return ptr;
}


/*
    Mark this allocation address as freed (debug only)
 */
static void freeLocation(MprMem *mp)
{
    MprLocationStats    *lp;
    cchar               *name;
    int                 index;

    if (!heap->track || mp->eternal) {
        return;
    }
    name = mp->name;
    if (name == 0) {
        return;
    }
    index = shash(name, strlen(name)) % MPR_TRACK_HASH;
    lp = &heap->stats.locations[index];
    mprAtomicAdd(&lp->count, -1);
    if (lp->total >= mp->size) {
        mprAtomicAdd64((int64*) &lp->total, - (int64) mp->size);
    } else {
        lp->total = 0;
    }
    SET_NAME(mp, NULL);
}
#endif


PUBLIC void *mprSetName(void *ptr, cchar *name)
{
    MprMem  *mp;

    assert(name && *name);
    mp = GET_MEM(ptr);
    if (mp->name) {
        freeLocation(mp);
    }
    mprSetAllocName(ptr, name);
    return ptr;
}


PUBLIC void *mprCopyName(void *dest, void *src)
{
    return mprSetName(dest, mprGetName(src));
}
#endif

/********************************************* Misc ***************************************************/

static void printMemWarn(size_t used, bool critical)
{
    static int once = 0;

    if (once++ == 0 || critical) {
        mprLog("warn mpr memory", 0, "Memory used %'d, redline %'d, limit %'d.", (int) used, (int) heap->stats.warnHeap,
            (int) heap->stats.maxHeap);
    }
}


static void allocException(int cause, size_t size)
{
    size_t      used;
    static int  once = 0;

    INC(errors);
    if (heap->stats.inMemException || mprIsStopping()) {
        return;
    }
    heap->stats.inMemException = 1;
    used = mprGetMem();

    if (cause == MPR_MEM_FAIL) {
        heap->hasError = 1;
        mprLog("error mpr memory", 0, "Cannot allocate memory block of size %'zd bytes.", size);
        printMemWarn(used, 1);

    } else if (cause == MPR_MEM_TOO_BIG) {
        heap->hasError = 1;
        mprLog("error mpr memory", 0, "Cannot allocate memory block of size %'zd bytes.", size);
        printMemWarn(used, 1);

    } else if (cause == MPR_MEM_WARNING) {
        if (once++ == 0) {
            mprLog("error mpr memory", 0, "Memory request for %'zd bytes exceeds memory red-line.", size);
        }
        mprPruneCache(NULL);
        printMemWarn(used, 0);

    } else if (cause == MPR_MEM_LIMIT) {
        mprLog("error mpr memory", 0, "Memory request for %'zd bytes exceeds memory limit.", size);
        printMemWarn(used, 1);
    }

    if (heap->notifier) {
        (heap->notifier)(cause, heap->allocPolicy,  size, used);
    }
    if (cause & (MPR_MEM_TOO_BIG | MPR_MEM_FAIL)) {
        /*
            Allocation failed
         */
        if (heap->allocPolicy == MPR_ALLOC_POLICY_ABORT) {
            abort();
        } else {
            mprLog("critical mpr memory", 0, "Application exiting immediately due to memory depletion.");
            mprShutdown(MPR_EXIT_ABORT, -1, 0);
        }

    } else if (cause & MPR_MEM_LIMIT) {
        /*
            Over memory max limit
         */
        if (heap->allocPolicy == MPR_ALLOC_POLICY_RESTART) {
            mprLog("critical mpr memory", 0, "Application restarting due to low memory condition.");
            mprShutdown(MPR_EXIT_RESTART, -1, 0);

        } else if (heap->allocPolicy == MPR_ALLOC_POLICY_EXIT) {
            mprLog("critical mpr memory", 0, "Application exiting due to memory depletion.");
            mprShutdown(MPR_EXIT_NORMAL, -1, MPR_EXIT_TIMEOUT);

        } else if (heap->allocPolicy == MPR_ALLOC_POLICY_ABORT) {
            //  kill(getpid(), SIGSEGV);
            abort();
        }
    }
    heap->stats.inMemException = 0;
}


static void getSystemInfo()
{
    memStats.cpuCores = 1;


    static const char processor[] = "processor\t:";
    char    c;
    int     fd, col, match;
 
    fd = open("/proc/cpuinfo", O_RDONLY);
    if (fd < 0) {
        return;
    }
    match = 1;
    memStats.cpuCores = 0;
    for (col = 0; read(fd, &c, 1) == 1; ) {
        if (c == '\n') {
            col = 0;
            match = 1;
        } else {
            if (match && col < (sizeof(processor) - 1)) {
                if (c != processor[col]) {
                    match = 0;
                }
                col++;
 
            } else if (match) {
                memStats.cpuCores++;
                match = 0;
            }
        }
    }
    if (memStats.cpuCores <= 0) {
        memStats.cpuCores = 1;
    }
    close(fd);
    memStats.pageSize = sysconf(_SC_PAGESIZE);

    if (memStats.pageSize <= 0 || memStats.pageSize >= (16 * 1024)) {
        memStats.pageSize = 4096;
    }
}

PUBLIC MprMemStats *mprGetMemStats()
{
    char            buf[1024], *cp;
    size_t          len;
    int             fd;

    heap->stats.ram = MAXSSIZE;
    if ((fd = open("/proc/meminfo", O_RDONLY)) >= 0) {
        if ((len = read(fd, buf, sizeof(buf) - 1)) > 0) {
            buf[len] = '\0';
            if ((cp = strstr(buf, "MemTotal:")) != 0) {
                for (; *cp && !isdigit((uchar) *cp); cp++) {}
                heap->stats.ram = ((size_t) atoi(cp) * 1024);
            }
        }
        close(fd);
    }

    heap->stats.rss = mprGetMem();
    heap->stats.cpuUsage = mprGetCPU();
    return &heap->stats;
}


/*
    Return the amount of memory currently in use. This routine may open files and thus is not very quick on some
    platforms. On FREEBDS it returns the peak resident set size using getrusage. If a suitable O/S API is not available,
    the amount of heap memory allocated by the MPR is returned.
 */
PUBLIC size_t mprGetMem()
{
    size_t      size = 0;

    static int  procfd = -1;
    char        buf[ME_BUFSIZE], *cp;
    int         nbytes;

    if (procfd < 0) {
        procfd = open("/proc/self/statm", O_RDONLY);
    }
    if (procfd >= 0) {
        lseek(procfd, 0, 0);
        if ((nbytes = read(procfd, buf, sizeof(buf) - 1)) > 0) {
            buf[nbytes] = '\0';
            for (cp = buf; *cp && *cp != ' '; cp++) {}
            for (; *cp == ' '; cp++) {}
            size = stoi(cp) * memStats.pageSize;
        }
    }
    if (size == 0) {
        struct rusage rusage;
        getrusage(RUSAGE_SELF, &rusage);
        size = rusage.ru_maxrss * 1024;
    }

    if (size == 0) {
        size = (size_t) heap->stats.bytesAllocated;
    }
    return size;
}


PUBLIC uint64 mprGetCPU()
{
    uint64     ticks;

    ticks = 0;

    int fd;
    char path[ME_MAX_PATH];
    sprintf(path, "/proc/%d/stat", getpid());
    if ((fd = open(path, O_RDONLY)) >= 0) {
        char buf[ME_BUFSIZE];
        int nbytes = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (nbytes > 0) {
            ulong utime, stime;
            buf[nbytes] = '\0';
            sscanf(buf, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu", &utime, &stime);
            ticks = (utime + stime) * TPS / sysconf(_SC_CLK_TCK);
        }
    }

    return ticks;
}


#ifndef findFirstBit
static ME_INLINE int findFirstBit(size_t word)
{
    int     b;
    for (b = 0; word; word >>= 1, b++) {
        if (word & 0x1) {
            b++;
            break;
        }
    }
    return b;
}
#endif


#ifndef findLastBit
static ME_INLINE int findLastBit(size_t word)
{
    int     b;

    for (b = 0; word; word >>= 1, b++) ;
    return b;
}
#endif


/*
    Acquire the freeq. Note: this is only ever used by non-blocking algorithms.
 */
static ME_INLINE bool acquire(MprFreeQueue *freeq)
{
#if ME_COMPILER_HAS_SPINLOCK
    return pthread_spin_trylock(&freeq->lock.cs) == 0;
#else
    return pthread_mutex_trylock(&freeq->lock.cs) == 0;
#endif
}


static ME_INLINE void release(MprFreeQueue *freeq)
{
#if ME_COMPILER_HAS_SPINLOCK
    pthread_spin_unlock(&freeq->lock.cs);
#else
    pthread_mutex_unlock(&freeq->lock.cs);
#endif
}


static ME_INLINE int cas(size_t *target, size_t expected, size_t value)
{
    return mprAtomicCas((void**) target, (void*) expected, (cvoid*) value);
}


static ME_INLINE void clearbitmap(size_t *bitmap, int index)
{
    size_t  bit, prior;

    bit = (((size_t) 1) << index);
    do {
        prior = *bitmap;
        if (!(prior & bit)) {
            break;
        }
    } while (!cas(bitmap, prior, prior & ~bit));
}


static ME_INLINE void setbitmap(size_t *bitmap, int index)
{
    size_t  bit, prior;

    bit = (((size_t) 1) << index);
    do {
        prior = *bitmap;
        if (prior & bit) {
            break;
        }
    } while (!cas(bitmap, prior, prior | bit));
}

PUBLIC int mprGetPageSize()
{
    return memStats.pageSize;
}


PUBLIC size_t mprGetBlockSize(cvoid *ptr)
{
    MprMem      *mp;

    mp = GET_MEM(ptr);
    if (ptr == 0 || !VALID_BLK(mp)) {
        return 0;
    }
    CHECK(mp);
    return GET_USIZE(mp);
}


PUBLIC int mprGetHeapFlags(void)
{
    return heap->flags;
}


PUBLIC void mprSetMemNotifier(MprMemNotifier cback)
{
    heap->notifier = cback;
}


PUBLIC void mprSetMemLimits(ssize warnHeap, ssize maxHeap, ssize cacheHeap)
{
    if (warnHeap > 0) {
        heap->stats.warnHeap = warnHeap;
    }
    if (maxHeap > 0) {
        heap->stats.maxHeap = maxHeap;
    }
    if (cacheHeap >= 0) {
        heap->stats.cacheHeap = cacheHeap;
        heap->stats.lowHeap = cacheHeap ? cacheHeap / 8 : ME_MPR_ALLOC_REGION_SIZE;
    }
}


PUBLIC void mprSetMemPolicy(int policy)
{
    heap->allocPolicy = policy;
}


PUBLIC void mprSetMemError()
{
    heap->hasError = 1;
}


PUBLIC bool mprHasMemError()
{
    return heap->hasError;
}


PUBLIC void mprResetMemError()
{
    heap->hasError = 0;
}


PUBLIC int mprIsValid(cvoid *ptr)
{
    MprMem      *mp;

    if (ptr == NULL) {
        return 0;
    }
    mp = GET_MEM(ptr);
    if (!mp || mp->free) {
        return 0;
    }
#if ME_WIN
    if (isBadWritePtr(mp, sizeof(MprMem))) {
        return 0;
    }
    if (!VALID_BLK(GET_MEM(ptr)) {
        return 0;
    }
    if (isBadWritePtr(ptr, mp->size)) {
        return 0;
    }
    return 0;
#else
#if ME_MPR_ALLOC_DEBUG
    return ptr && mp->magic == MPR_ALLOC_MAGIC && mp->size > 0;
#else
    return ptr && mp->size > 0;
#endif
#endif
}


/*
    Yield the thread timeslice to allow contention on queues to be relieved
    This stops busy waiting which can be a problem on VxWorks if higher priority threads outside the MPR
    can starve the MPR of cpu while contenting for memory via mprCreateEvent.
 */
static void dontBusyWait()
{
    struct timespec nap;
    nap.tv_sec = 0;
    nap.tv_nsec = 1;
    nanosleep(&nap, NULL);
}


static void dummyManager(void *ptr, int flags)
{
}


PUBLIC void *mprSetManager(void *ptr, MprManager manager)
{
    MprMem      *mp;

    mp = GET_MEM(ptr);
    if (mp->hasManager) {
        if (!manager) {
            manager = dummyManager;
        }
        SET_MANAGER(mp, manager);
    }
    return ptr;
}


#if ME_MPR_ALLOC_STACK
static void monitorStack()
{
    MprThread   *tp;
    int         diff;

    if (MPR->threadService && (tp = mprGetCurrentThread()) != 0) {
        if (tp->stackBase == 0) {
            tp->stackBase = &tp;
        }
        diff = (int) ((char*) tp->stackBase - (char*) &diff);
        if (diff < 0) {
            tp->peakStack -= diff;
            tp->stackBase = (void*) &diff;
            diff = 0;
        }
        if (diff > tp->peakStack) {
            tp->peakStack = diff;
        }
    }
}
#endif


static ME_INLINE bool needGC(MprHeap *heap)
{
    if (!heap->gcRequested && heap->workDone > (heap->workQuota * (mprGetBusyWorkerCount() / 2 + 1))) {
        return 1;
    }
    return 0;
}


#if !ME_MPR_ALLOC_DEBUG
#undef mprSetName
#undef mprCopyName
#undef mprSetAllocName

/*
    Re-instate defines for combo releases, where source will be appended below here
 */
#define mprCopyName(dest, src)
#define mprGetName(ptr) ""
#define mprSetAllocName(ptr, name) ptr
#define mprSetName(ptr, name)
#endif



