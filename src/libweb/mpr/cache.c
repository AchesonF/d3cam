#include "libmpr.h"

/**
    cache.c - In-process caching

 */

/********************************** Includes **********************************/



/************************************ Locals **********************************/

static MprCache *shared;                /* Singleton shared cache */

typedef struct CacheItem
{
    char            *key;               /* Original key */
    char            *data;              /* Cache data */
    void            *link;              /* Linked managed reference */
    MprTicks        lifespan;           /* Lifespan after each access to key (msec) */
    MprTicks        lastAccessed;       /* Last accessed time */
    MprTicks        expires;            /* Fixed expiry date. If zero, key is imortal. */
    MprTime         lastModified;       /* Last update time. This is an MprTime and records world-time. */
    int64           version;
} CacheItem;

#define CACHE_TIMER_PERIOD      (60 * TPS)
#define CACHE_LIFESPAN          (86400 * TPS)
#define CACHE_HASH_SIZE         257

/*********************************** Forwards *********************************/

static void manageCache(MprCache *cache, int flags);
static void manageCacheItem(CacheItem *item, int flags);
static void pruneCache(MprCache *cache, MprEvent *event);
static void removeItem(MprCache *cache, CacheItem *item);

/************************************* Code ***********************************/

PUBLIC int mprCreateCacheService()
{
    shared = 0;
    return 0;
}


PUBLIC MprCache *mprCreateCache(int options)
{
    MprCache    *cache;
    int         wantShared;

    if ((cache = mprAllocObj(MprCache, manageCache)) == 0) {
        return 0;
    }
    wantShared = (options & MPR_CACHE_SHARED);
    if (wantShared && shared) {
        cache->shared = shared;
    } else {
        cache->mutex = mprCreateLock();
        cache->store = mprCreateHash(CACHE_HASH_SIZE, 0);
        cache->maxMem = MAXSSIZE;
        cache->maxKeys = MAXSSIZE;
        cache->resolution = CACHE_TIMER_PERIOD;
        cache->lifespan = CACHE_LIFESPAN;
        if (wantShared) {
            shared = cache;
        }
    }
    return cache;
}


PUBLIC void *mprDestroyCache(MprCache *cache)
{
    assert(cache);

    if (cache->timer && cache != shared) {
        mprRemoveEvent(cache->timer);
        cache->timer = 0;
    }
    if (cache == shared) {
        shared = 0;
    }
    return 0;
}


/*
    Set expires to zero to remove
 */
PUBLIC int mprExpireCacheItem(MprCache *cache, cchar *key, MprTicks expires)
{
    CacheItem   *item;

    assert(cache);
    assert(key && *key);

    if (cache->shared) {
        cache = cache->shared;
        assert(cache == shared);
    }
    lock(cache);
    if ((item = mprLookupKey(cache->store, key)) == 0) {
        unlock(cache);
        return MPR_ERR_CANT_FIND;
    }
    if (expires == 0) {
        removeItem(cache, item);
    } else {
        item->expires = expires;
    }
    unlock(cache);
    return 0;
}


PUBLIC int64 mprIncCache(MprCache *cache, cchar *key, int64 amount)
{
    CacheItem   *item;
    int64       value;

    assert(cache);
    assert(key && *key);

    if (cache->shared) {
        cache = cache->shared;
        assert(cache == shared);
    }
    value = amount;

    lock(cache);
    if ((item = mprLookupKey(cache->store, key)) == 0) {
        if ((item = mprAllocObj(CacheItem, manageCacheItem)) == 0) {
            return 0;
        }
    } else {
        value += stoi(item->data);
    }
    if (item->data) {
        cache->usedMem -= slen(item->data);
    }
    item->data = itos(value);
    cache->usedMem += slen(item->data);
    item->version++;
    item->lastAccessed = mprGetTicks();
    item->expires = item->lastAccessed + item->lifespan;
    unlock(cache);
    return value;
}


PUBLIC char *mprLookupCache(MprCache *cache, cchar *key, MprTime *modified, int64 *version)
{
    CacheItem   *item;
    char        *result;

    assert(cache);
    assert(key);

    if (cache->shared) {
        cache = cache->shared;
        assert(cache == shared);
    }
    lock(cache);
    if ((item = mprLookupKey(cache->store, key)) == 0) {
        unlock(cache);
        return 0;
    }
    if (item->expires && item->expires <= mprGetTicks()) {
        unlock(cache);
        return 0;
    }
    if (version) {
        *version = item->version;
    }
    if (modified) {
        *modified = item->lastModified;
    }
    result = item->data;
    unlock(cache);
    return result;
}


PUBLIC char *mprReadCache(MprCache *cache, cchar *key, MprTime *modified, int64 *version)
{
    CacheItem   *item;
    char        *result;

    assert(cache);
    assert(key);

    if (cache->shared) {
        cache = cache->shared;
        assert(cache == shared);
    }
    lock(cache);
    if ((item = mprLookupKey(cache->store, key)) == 0) {
        unlock(cache);
        return 0;
    }
    if (item->expires && item->expires <= mprGetTicks()) {
        removeItem(cache, item);
        unlock(cache);
        return 0;
    }
    if (version) {
        *version = item->version;
    }
    if (modified) {
        *modified = item->lastModified;
    }
    item->lastAccessed = mprGetTicks();
    item->expires = item->lastAccessed + item->lifespan;
    result = item->data;
    unlock(cache);
    return result;
}


PUBLIC bool mprRemoveCache(MprCache *cache, cchar *key)
{
    CacheItem   *item;
    bool        result;

    assert(cache);
    assert(key && *key);

    if (cache->shared) {
        cache = cache->shared;
        assert(cache == shared);
    }
    lock(cache);
    if (key) {
        if ((item = mprLookupKey(cache->store, key)) != 0) {
            cache->usedMem -= (slen(key) + slen(item->data));
            mprRemoveKey(cache->store, key);
            result = 1;
        } else {
            result = 0;
        }

    } else {
        /* Remove all keys */
        result = mprGetHashLength(cache->store) ? 1 : 0;
        cache->store = mprCreateHash(CACHE_HASH_SIZE, 0);
        cache->usedMem = 0;
    }
    unlock(cache);
    return result;
}


PUBLIC void mprSetCacheNotify(MprCache *cache, MprCacheProc notify)
{
    assert(cache);

    if (cache->shared) {
        cache = cache->shared;
        assert(cache == shared);
    }
    cache->notify = notify;
}


PUBLIC void mprSetCacheLimits(MprCache *cache, int64 keys, MprTicks lifespan, int64 memory, int resolution)
{
    assert(cache);

    if (cache->shared) {
        cache = cache->shared;
        assert(cache == shared);
    }
    if (keys > 0) {
        cache->maxKeys = (ssize) keys;
        if (cache->maxKeys <= 0) {
            cache->maxKeys = MAXSSIZE;
        }
    }
    if (lifespan > 0) {
        cache->lifespan = lifespan;
    }
    if (memory > 0) {
        cache->maxMem = (ssize) memory;
        if (cache->maxMem <= 0) {
            cache->maxMem = MAXSSIZE;
        }
    }
    if (resolution > 0) {
        cache->resolution = resolution;
        if (cache->resolution <= 0) {
            cache->resolution = CACHE_TIMER_PERIOD;
        }
    }
}


PUBLIC ssize mprWriteCache(MprCache *cache, cchar *key, cchar *value, MprTime modified, MprTicks lifespan,
    int64 version, int options)
{
    CacheItem   *item;
    MprKey      *kp;
    ssize       len, oldLen;
    int         exists, add, set, prepend, append, throw, event;

    assert(cache);
    assert(key && *key);
    assert(value);

    if (cache->shared) {
        cache = cache->shared;
        assert(cache == shared);
    }
    exists = add = prepend = append = throw = 0;
    add = options & MPR_CACHE_ADD;
    append = options & MPR_CACHE_APPEND;
    prepend = options & MPR_CACHE_PREPEND;
    set = options & MPR_CACHE_SET;
    if ((add + append + prepend) == 0) {
        set = 1;
    }
    lock(cache);
    if ((kp = mprLookupKeyEntry(cache->store, key)) != 0) {
        exists++;
        item = (CacheItem*) kp->data;
        if (version) {
            if (item->version != version) {
                unlock(cache);
                return MPR_ERR_BAD_STATE;
            }
        }
    } else {
        if ((item = mprAllocObj(CacheItem, manageCacheItem)) == 0) {
            unlock(cache);
            return 0;
        }
        mprAddKey(cache->store, key, item);
        item->key = sclone(key);
        set = 1;
    }
    oldLen = (item->data) ? (slen(item->key) + slen(item->data)) : 0;
    if (set) {
        item->data = sclone(value);
    } else if (add) {
        if (exists) {
            return 0;
        }
        item->data = sclone(value);
    } else if (append) {
        item->data = sjoin(item->data, value, NULL);
    } else if (prepend) {
        item->data = sjoin(value, item->data, NULL);
    }
    if (lifespan >= 0) {
        item->lifespan = lifespan;
    }
    item->lastModified = modified ? modified : mprGetTime();
    item->lastAccessed = mprGetTicks();
    item->expires = item->lastAccessed + item->lifespan;
    item->version++;
    len = slen(item->key) + slen(item->data);
    cache->usedMem += (len - oldLen);

    if (cache->timer == 0) {
        cache->timer = mprCreateTimerEvent(MPR->dispatcher, "localCacheTimer", cache->resolution, pruneCache, cache,
            MPR_EVENT_STATIC_DATA);
    }
    if (cache->notify) {
        if (exists) {
            event = MPR_CACHE_NOTIFY_CREATE;
        } else {
            event = MPR_CACHE_NOTIFY_UPDATE;
        }
        (cache->notify)(cache, item->key, item->data, event);
    }
    unlock(cache);
    return len;
}


PUBLIC void *mprGetCacheLink(MprCache *cache, cchar *key)
{
    CacheItem   *item;
    MprKey      *kp;
    void        *result;

    assert(cache);
    assert(key && *key);

    if (cache->shared) {
        cache = cache->shared;
        assert(cache == shared);
    }
    result = 0;
    lock(cache);
    if ((kp = mprLookupKeyEntry(cache->store, key)) != 0) {
        item = (CacheItem*) kp->data;
        result = item->link;
    }
    unlock(cache);
    return result;
}


PUBLIC int mprSetCacheLink(MprCache *cache, cchar *key, void *link)
{
    CacheItem   *item;
    MprKey      *kp;

    assert(cache);
    assert(key && *key);

    if (cache->shared) {
        cache = cache->shared;
        assert(cache == shared);
    }
    lock(cache);
    if ((kp = mprLookupKeyEntry(cache->store, key)) != 0) {
        item = (CacheItem*) kp->data;
        item->link = link;
    }
    unlock(cache);
    return kp ? 0 : MPR_ERR_CANT_FIND;
}


static void removeItem(MprCache *cache, CacheItem *item)
{
    assert(cache);
    assert(item);

    lock(cache);
    if (cache->notify) {
        (cache->notify)(cache, item->key, item->data, MPR_CACHE_NOTIFY_REMOVE);
    }
    mprRemoveKey(cache->store, item->key);
    cache->usedMem -= (slen(item->key) + slen(item->data));
    unlock(cache);
}


static void pruneCache(MprCache *cache, MprEvent *event)
{
    MprTicks        when, factor;
    MprKey          *kp;
    CacheItem       *item;
    ssize           excessKeys;

    if (!cache) {
        cache = shared;
        if (!cache) {
            return;
        }
    }
    if (event) {
        when = mprGetTicks();
    } else {
        /* Expire all items by setting event to NULL */
        when = MPR_MAX_TIMEOUT;
    }
    if (mprTryLock(cache->mutex)) {
        /*
            Check for expired items
         */
        for (kp = 0; (kp = mprGetNextKey(cache->store, kp)) != 0; ) {
            item = (CacheItem*) kp->data;
            if (item->expires && item->expires <= when) {
                mprDebug("debug mpr cache", 5, "Prune expired key %s", kp->key);
                removeItem(cache, item);
            }
        }
        assert(cache->usedMem >= 0);

        /*
            If too many keys or too much memory used, prune keys that expire soonest.
         */
        if (cache->maxKeys < MAXSSIZE || cache->maxMem < MAXSSIZE) {
            /*
                Look for those expiring in the next 5 minutes, then 20 mins, then 80 ...
             */
            excessKeys = mprGetHashLength(cache->store) - cache->maxKeys;
            if (excessKeys < 0) {
                excessKeys = 0;
            }
            factor = 5 * 60 * TPS;
            when += factor;
            while (excessKeys > 0 || cache->usedMem > cache->maxMem) {
                for (kp = 0; (kp = mprGetNextKey(cache->store, kp)) != 0; ) {
                    item = (CacheItem*) kp->data;
                    if (item->expires && item->expires <= when) {
                        mprDebug("debug mpr cache", 3, "Cache too big, execess keys %zd, mem %zd, prune key %s",
                            excessKeys, (cache->maxMem - cache->usedMem), kp->key);
                        removeItem(cache, item);
                        excessKeys--;
                    }
                }
                factor *= 4;
                when += factor;
            }
        }
        assert(cache->usedMem >= 0);

        if (mprGetHashLength(cache->store) == 0) {
            if (event) {
                mprRemoveEvent(event);
                cache->timer = 0;
            }
        }
        unlock(cache);
    }
}


PUBLIC void mprPruneCache(MprCache *cache)
{
    pruneCache(cache, NULL);
}


static void manageCache(MprCache *cache, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(cache->store);
        mprMark(cache->mutex);
        mprMark(cache->timer);
        mprMark(cache->shared);

    } else if (flags & MPR_MANAGE_FREE) {
        if (cache == shared) {
            shared = 0;
        }
    }
}


static void manageCacheItem(CacheItem *item, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(item->key);
        mprMark(item->data);
        mprMark(item->link);
    }
}


PUBLIC void mprGetCacheStats(MprCache *cache, int *numKeys, ssize *mem)
{
    if (numKeys) {
        *numKeys = mprGetHashLength(cache->store);
    }
    if (mem) {
        *mem = cache->usedMem;
    }
}




