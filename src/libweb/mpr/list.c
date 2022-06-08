#include "libmpr.h"

/**
    list.c - Simple list type.

    The list supports two modes of operation. Compact mode where the list is compacted after removing list items,
    and no-compact mode where removed items are zeroed. No-compact mode implies that all valid list entries must
    be non-zero.

    This module is not thread-safe. It is the callers responsibility to perform all thread synchronization.
 */

/********************************** Includes **********************************/



/********************************** Defines ***********************************/

#ifndef ME_MAX_LIST
    #define ME_MAX_LIST   8
#endif

/********************************** Forwards **********************************/

static int growList(MprList *lp, int incr);
static void manageList(MprList *lp, int flags);

/************************************ Code ************************************/
/*
    Create a general growable list structure
 */
PUBLIC MprList *mprCreateList(int size, int flags)
{
    MprList     *lp;

    if ((lp = mprAllocObjNoZero(MprList, manageList)) == 0) {
        return 0;
    }
    lp->flags = flags | MPR_OBJ_LIST;
    lp->size = 0;
    lp->length = 0;
    lp->maxSize = MAXINT;
    if (!(flags & MPR_LIST_STABLE)) {
        lp->mutex = mprCreateLock();
    } else {
        lp->mutex = 0;
    }
    lp->items = 0;
    if (size != 0) {
        mprSetListLimits(lp, size, -1);
    }
    return lp;
}


static void manageList(MprList *lp, int flags)
{
    int     i;

    if (flags & MPR_MANAGE_MARK) {
        mprMark(lp->mutex);
        if (lp->items) {
            /* Lock incase foreign threads queueing events. Also for markRoots. */
            lock(lp);
            mprMark(lp->items);
            if (!(lp->flags & MPR_LIST_STATIC_VALUES)) {
                for (i = 0; i < lp->length; i++) {
#if CONFIG_DEBUG
                    assert(lp->items[i] == 0 || mprIsValid(lp->items[i]));
#endif
                    mprMark(lp->items[i]);
                }
            }
            unlock(lp);
        }
    }
}

/*
    Initialize a list which may not be a memory context.
 */
PUBLIC void mprInitList(MprList *lp, int flags)
{
    lp->flags = 0;
    lp->size = 0;
    lp->length = 0;
    lp->maxSize = MAXINT;
    lp->items = 0;
    lp->mutex = (flags & MPR_LIST_STABLE) ? 0 : mprCreateLock();
}


/*
    Define the list maximum size. If the list has not yet been written to, the initialSize will be observed.
 */
PUBLIC int mprSetListLimits(MprList *lp, int initialSize, int maxSize)
{
    ssize   size;

    if (initialSize <= 0) {
        initialSize = ME_MAX_LIST;
    }
    if (maxSize <= 0) {
        maxSize = MAXINT;
    }
    size = initialSize * sizeof(void*);

    lock(lp);
    if (lp->items == 0) {
        if ((lp->items = mprAlloc(size)) == 0) {
            assert(!MPR_ERR_MEMORY);
            unlock(lp);
            return MPR_ERR_MEMORY;
        }
        memset(lp->items, 0, size);
        lp->size = initialSize;
    }
    lp->maxSize = maxSize;
    unlock(lp);
    return 0;
}


PUBLIC int mprCopyListContents(MprList *dest, MprList *src)
{
    void        *item;
    int         next;

    mprClearList(dest);

    lock(src);
    if (mprSetListLimits(dest, src->size, src->maxSize) < 0) {
        assert(!MPR_ERR_MEMORY);
        unlock(src);
        return MPR_ERR_MEMORY;
    }
    for (next = 0; (item = mprGetNextItem(src, &next)) != 0; ) {
        if (mprAddItem(dest, item) < 0) {
            assert(!MPR_ERR_MEMORY);
            unlock(src);
            return MPR_ERR_MEMORY;
        }
    }
    unlock(src);
    return 0;
}


PUBLIC MprList *mprCloneList(MprList *src)
{
    MprList     *lp;

    if ((lp = mprCreateList(src->size, src->flags)) == 0) {
        return 0;
    }
    if (mprCopyListContents(lp, src) < 0) {
        return 0;
    }
    return lp;
}


PUBLIC MprList *mprCreateListFromWords(cchar *str)
{
    MprList     *list;
    char        *word, *next;

    list = mprCreateList(0, 0);
    word = stok(sclone(str), ", \t\n\r", &next);
    while (word) {
        mprAddItem(list, word);
        word = stok(NULL, ", \t\n\r", &next);
    }
    return list;
}


PUBLIC MprList *mprAppendList(MprList *lp, MprList *add)
{
    void        *item;
    int         next;

    assert(lp);

    for (next = 0; ((item = mprGetNextItem(add, &next)) != 0); ) {
        if (mprAddItem(lp, item) < 0) {
            return 0;
        }
    }
    return lp;
}


/*
    Change the item in the list at index. Return the old item.
 */
PUBLIC void *mprSetItem(MprList *lp, int index, cvoid *item)
{
    void    *old;
    int     length;

    assert(lp);
    if (!lp) {
        return 0;
    }
    assert(lp->size >= 0);
    assert(lp->length >= 0);
    assert(index >= 0);

    length = lp->length;

    if (index >= length) {
        length = index + 1;
    }
    lock(lp);
    if (length > lp->size) {
        if (growList(lp, length - lp->size) < 0) {
            unlock(lp);
            return 0;
        }
    }
    old = lp->items[index];
    lp->items[index] = (void*) item;
    lp->length = length;
    unlock(lp);
    return old;
}



/*
    Add an item to the list and return the item index.
 */
PUBLIC int mprAddItem(MprList *lp, cvoid *item)
{
    int     index;

    assert(lp);
    if (!lp) {
        return MPR_ERR_BAD_ARGS;
    }
    assert(lp->size >= 0);
    assert(lp->length >= 0);

    lock(lp);
    if (lp->length >= lp->size) {
        if (growList(lp, 1) < 0) {
            unlock(lp);
            return MPR_ERR_TOO_MANY;
        }
    }
    index = lp->length++;
    lp->items[index] = (void*) item;
    unlock(lp);
    return index;
}


PUBLIC int mprAddNullItem(MprList *lp)
{
    int     index;

    assert(lp);
    if (!lp) {
        return MPR_ERR_BAD_ARGS;
    }
    assert(lp->size >= 0);
    assert(lp->length >= 0);

    lock(lp);
    if (lp->length != 0 && lp->items[lp->length - 1] == 0) {
        index = lp->length - 1;
    } else {
        if (lp->length >= lp->size) {
            if (growList(lp, 1) < 0) {
                unlock(lp);
                return MPR_ERR_TOO_MANY;
            }
        }
        index = lp->length;
        lp->items[index] = 0;
    }
    unlock(lp);
    return index;
}


/*
    Insert an item to the list at a specified position. We insert before the item at "index".
    ie. The inserted item will go into the "index" location and the other elements will be moved up.
 */
PUBLIC int mprInsertItemAtPos(MprList *lp, int index, cvoid *item)
{
    void    **items;
    int     i;

    assert(lp);
    if (!lp) {
        return MPR_ERR_BAD_ARGS;
    }
    assert(lp->size >= 0);
    assert(lp->length >= 0);
    assert(index >= 0);

    if (index < 0) {
        index = 0;
    }
    lock(lp);
    if (index >= lp->size) {
        if (growList(lp, index - lp->size + 1) < 0) {
            unlock(lp);
            return MPR_ERR_TOO_MANY;
        }

    } else if (lp->length >= lp->size) {
        if (growList(lp, 1) < 0) {
            unlock(lp);
            return MPR_ERR_TOO_MANY;
        }
    }
    if (index >= lp->length) {
        lp->length = index + 1;
    } else {
        /*
            Copy up items to make room to insert
         */
        items = lp->items;
        for (i = lp->length; i > index; i--) {
            items[i] = items[i - 1];
        }
        lp->length++;
    }
    lp->items[index] = (void*) item;
    unlock(lp);
    return index;
}


/*
    Remove an item from the list. Return the index where the item resided.
 */
PUBLIC int mprRemoveItem(MprList *lp, cvoid *item)
{
    int     index;

    if (!lp) {
        return -1;
    }
    lock(lp);
    if ((index = mprLookupItem(lp, item)) < 0) {
        unlock(lp);
        return index;
    }
    index = mprRemoveItemAtPos(lp, index);
    assert(index >= 0);
    unlock(lp);
    return index;
}


PUBLIC int mprRemoveLastItem(MprList *lp)
{
    assert(lp);
    if (!lp) {
        return MPR_ERR_BAD_ARGS;
    }
    assert(lp->size > 0);
    assert(lp->length > 0);

    if (lp->length <= 0) {
        return MPR_ERR_CANT_FIND;
    }
    return mprRemoveItemAtPos(lp, lp->length - 1);
}


/*
    Remove an index from the list. Return the index where the item resided.
    The list is compacted.
 */
PUBLIC int mprRemoveItemAtPos(MprList *lp, int index)
{
    void    **items;

    assert(lp);
    assert(lp->size > 0);
    assert(index >= 0 && index < lp->size);
    assert(lp->length > 0);

    if (index < 0 || index >= lp->length) {
        return MPR_ERR_CANT_FIND;
    }
    lock(lp);
    items = lp->items;
    memmove(&items[index], &items[index + 1], (lp->length - index - 1) * sizeof(void*));
    lp->length--;
    lp->items[lp->length] = 0;
    assert(lp->length >= 0);
    unlock(lp);
    return index;
}


/*
    Remove a set of items. Return 0 if successful.
 */
PUBLIC int mprRemoveRangeOfItems(MprList *lp, int start, int end)
{
    void    **items;
    int     i, count;

    assert(lp);
    if (!lp) {
        return MPR_ERR_BAD_ARGS;
    }
    assert(lp->size > 0);
    assert(lp->length > 0);
    assert(start > end);

    if (start < 0 || start >= lp->length) {
        return MPR_ERR_CANT_FIND;
    }
    if (end < 0 || end >= lp->length) {
        return MPR_ERR_CANT_FIND;
    }
    if (start > end) {
        return MPR_ERR_BAD_ARGS;
    }
    /*
        Copy down to compress
     */
    items = lp->items;
    count = end - start;
    lock(lp);
    for (i = start; i < (lp->length - count); i++) {
        items[i] = items[i + count];
    }
    lp->length -= count;
    for (i = lp->length; i < lp->size; i++) {
        items[i] = 0;
    }
    unlock(lp);
    return 0;
}


/*
    Remove a string item from the list. Return the index where the item resided.
 */
PUBLIC int mprRemoveStringItem(MprList *lp, cchar *str)
{
    int     index;

    assert(lp);

    lock(lp);
    index = mprLookupStringItem(lp, str);
    if (index < 0) {
        unlock(lp);
        return index;
    }
    index = mprRemoveItemAtPos(lp, index);
    assert(index >= 0);
    unlock(lp);
    return index;
}


PUBLIC void *mprGetItem(MprList *lp, int index)
{
    assert(lp);

    if (index < 0 || index >= lp->length) {
        return 0;
    }
    return lp->items[index];
}


PUBLIC void *mprGetFirstItem(MprList *lp)
{
    assert(lp);

    if (lp == 0) {
        return 0;
    }
    if (lp->length == 0) {
        return 0;
    }
    return lp->items[0];
}


PUBLIC void *mprGetLastItem(MprList *lp)
{
    assert(lp);

    if (lp == 0) {
        return 0;
    }
    if (lp->length == 0) {
        return 0;
    }
    return lp->items[lp->length - 1];
}


PUBLIC void *mprGetNextItem(MprList *lp, int *next)
{
    void    *item;
    int     index;

    assert(next);
    assert(*next >= 0);

    if (lp == 0 || lp->length == 0) {
        return 0;
    }
    lock(lp);
    index = *next;
    if (index < lp->length) {
        item = lp->items[index];
        *next = ++index;
        unlock(lp);
        return item;
    }
    unlock(lp);
    return 0;
}


PUBLIC void *mprGetNextStableItem(MprList *lp, int *next)
{
    void    *item;
    int     index;

    assert(next);
    if (!lp || lp->length == 0 || !next) {
        return 0;
    }
    assert(*next >= 0);

    assert(lp->flags & MPR_LIST_STABLE);
    index = *next;
    if (index < lp->length) {
        item = lp->items[index];
        *next = ++index;
        return item;
    }
    return 0;
}


PUBLIC void *mprGetPrevItem(MprList *lp, int *next)
{
    void    *item;
    int     index;

    assert(next);

    if (lp == 0 || lp->length == 0) {
        return 0;
    }
    lock(lp);
    if (*next < 0) {
        *next = lp->length;
    }
    index = *next;
    if (--index < lp->length && index >= 0) {
        *next = index;
        item = lp->items[index];
        unlock(lp);
        return item;
    }
    unlock(lp);
    return 0;
}


PUBLIC int mprPushItem(MprList *lp, cvoid *item)
{
    return mprAddItem(lp, item);
}


PUBLIC void *mprPopItem(MprList *lp)
{
    void    *item;
    int     index;

    item = NULL;
    if (lp->length > 0) {
        lock(lp);
        index = lp->length - 1;
        if (index >= 0) {
            item = mprGetItem(lp, index);
            mprRemoveItemAtPos(lp, index);
        }
        unlock(lp);
    }
    return item;
}


#ifndef mprGetListLength
PUBLIC int mprGetListLength(MprList *lp)
{
    if (lp == 0) {
        return 0;
    }
    return lp->length;
}
#endif


PUBLIC int mprGetListCapacity(MprList *lp)
{
    assert(lp);

    if (lp == 0) {
        return 0;
    }
    return lp->size;
}


PUBLIC void mprClearList(MprList *lp)
{
    int     i;

    assert(lp);

    lock(lp);
    for (i = 0; i < lp->length; i++) {
        lp->items[i] = 0;
    }
    lp->length = 0;
    unlock(lp);
}


PUBLIC int mprLookupItem(MprList *lp, cvoid *item)
{
    int     i;

    assert(lp);

    lock(lp);
    for (i = 0; i < lp->length; i++) {
        if (lp->items[i] == item) {
            unlock(lp);
            return i;
        }
    }
    unlock(lp);
    return MPR_ERR_CANT_FIND;
}


PUBLIC int mprLookupStringItem(MprList *lp, cchar *str)
{
    int     i;

    assert(lp);

    lock(lp);
    for (i = 0; i < lp->length; i++) {
        if (smatch(lp->items[i], str)) {
            unlock(lp);
            return i;
        }
    }
    unlock(lp);
    return MPR_ERR_CANT_FIND;
}


/*
    Grow the list by the requried increment
 */
static int growList(MprList *lp, int incr)
{
    ssize       memsize;
    int         len;

    if (lp->maxSize <= 0) {
        lp->maxSize = MAXINT;
    }
    /*
        Need to grow the list
     */
    if (lp->size >= lp->maxSize) {
        assert(lp->size < lp->maxSize);
        return MPR_ERR_TOO_MANY;
    }
    /*
        If growing by 1, then use the default increment which exponentially grows. Otherwise, assume the caller knows exactly
        how much the list needs to grow.
     */
    if (incr <= 1) {
        len = ME_MAX_LIST + (lp->size * 2);
    } else {
        len = lp->size + incr;
    }
    memsize = len * sizeof(void*);

    if ((lp->items = mprRealloc(lp->items, memsize)) == NULL) {
        assert(!MPR_ERR_MEMORY);
        return MPR_ERR_MEMORY;
    }
    memset(&lp->items[lp->size], 0, (len - lp->size) * sizeof(void*));
    lp->size = len;
    return 0;
}


static int defaultSort(char **q1, char **q2, void *ctx)
{
    return scmp(*q1, *q2);
}


PUBLIC MprList *mprSortList(MprList *lp, MprSortProc cmp, void *ctx)
{
    if (!lp) {
        return 0;
    }
    lock(lp);
    mprSort(lp->items, lp->length, sizeof(void*), cmp, ctx);
    unlock(lp);
    return lp;
}


static void manageKeyValue(MprKeyValue *pair, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(pair->key);
        mprMark(pair->value);
    }
}


PUBLIC MprKeyValue *mprCreateKeyPair(cchar *key, cchar *value, int flags)
{
    MprKeyValue     *pair;

    if ((pair = mprAllocObjNoZero(MprKeyValue, manageKeyValue)) == 0) {
        return 0;
    }
    pair->key = sclone(key);
    pair->value = sclone(value);
    pair->flags = flags;
    return pair;
}


static void swapElt(char *a, char *b, ssize width)
{
    char    tmp;

    if (a == b) {
        return;
    }
    while (width--) {
        tmp = *a;
        *a++ = *b;
        *b++ = tmp;
    }
}


PUBLIC void *mprSort(void *base, ssize nelt, ssize esize, MprSortProc cmp, void *ctx)
{
    char    *array, *pivot, *left, *right, *end;

    if (nelt < 2 || esize <= 0) {
        return base;
    }
    if (!cmp) {
        cmp = (MprSortProc) defaultSort;
    }
    array = base;
    end = array + (nelt * esize);
    left = array;
    right = array + ((nelt - 1) * esize);
    pivot = array;

    while (left < right) {
        while (left < end && cmp(left, pivot, ctx) <= 0) {
            left += esize;
        }
        while (cmp(right, pivot, ctx) > 0) {
            right -= esize;
        }
        if (left < right) {
            swapElt(left, right, esize);
        }
    }
    swapElt(pivot, right, esize);
    mprSort(array, (right - array) / esize, esize, cmp, ctx);
    mprSort(left, nelt - ((left - array) / esize), esize, cmp, ctx);
    return base;
}


PUBLIC char *mprListToString(MprList *list, cchar *join)
{
    MprBuf  *buf;
    cchar   *s;
    int     next;

    if (!join) {
        join = ",";
    }
    buf = mprCreateBuf(0, 0);
    for (ITERATE_ITEMS(list, s, next)) {
        mprPutStringToBuf(buf, s);
        mprPutStringToBuf(buf, join);
    }
    if (next > 0) {
        mprAdjustBufEnd(buf, -1);
    }
    mprAddNullToBuf(buf);
    return mprGetBufStart(buf);
}





