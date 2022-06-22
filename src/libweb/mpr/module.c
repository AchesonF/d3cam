#include "libmpr.h"

/**
    module.c - Dynamic module loading support.
 */

/********************************* Includes ***********************************/



/********************************** Forwards **********************************/

static void manageModule(MprModule *mp, int flags);
static void manageModuleService(MprModuleService *ms, int flags);

/************************************* Code ***********************************/
/*
    Open the module service
 */
PUBLIC MprModuleService *mprCreateModuleService()
{
    MprModuleService    *ms;

    if ((ms = mprAllocObj(MprModuleService, manageModuleService)) == 0) {
        return 0;
    }
    ms->modules = mprCreateList(-1, 0);
    ms->mutex = mprCreateLock();
    MPR->moduleService = ms;
    mprSetModuleSearchPath(NULL);
    return ms;
}


static void manageModuleService(MprModuleService *ms, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ms->modules);
        mprMark(ms->searchPath);
        mprMark(ms->mutex);
    }
}


/*
    Call the start routine for each module
 */
PUBLIC int mprStartModuleService()
{
    MprModuleService    *ms;
    MprModule           *mp;
    int                 next;

    ms = MPR->moduleService;
    assert(ms);

    for (next = 0; (mp = mprGetNextItem(ms->modules, &next)) != 0; ) {
        if (mprStartModule(mp) < 0) {
            return MPR_ERR_CANT_INITIALIZE;
        }
    }

    return 0;
}


PUBLIC void mprStopModuleService()
{
    MprModuleService    *ms;
    MprModule           *mp;
    int                 next;

    ms = MPR->moduleService;
    assert(ms);
    mprLock(ms->mutex);
    for (next = 0; (mp = mprGetNextItem(ms->modules, &next)) != 0; ) {
        mprStopModule(mp);
    }
    mprUnlock(ms->mutex);
}


PUBLIC MprModule *mprCreateModule(cchar *name, cchar *path, cchar *entry, void *data)
{
    MprModuleService    *ms;
    MprModule           *mp;
    int                 index;

    ms = MPR->moduleService;
    assert(ms);

    if ((mp = mprAllocObj(MprModule, manageModule)) == 0) {
        return 0;
    }
    mp->name = sclone(name);
    if (path && *path) {
        mp->path = sclone(path);
    }
    if (entry && *entry) {
        mp->entry = sclone(entry);
    }
    /*
        Not managed by default unless MPR_MODULE_DATA_MANAGED is set
     */
    mp->moduleData = data;
    mp->lastActivity = mprGetTicks();
    index = mprAddItem(ms->modules, mp);
    if (index < 0 || mp->name == 0) {
        return 0;
    }
    return mp;
}


static void manageModule(MprModule *mp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(mp->name);
        mprMark(mp->path);
        mprMark(mp->entry);
        if (mp->flags & MPR_MODULE_DATA_MANAGED) {
            mprMark(mp->moduleData);
        }
    }
}


PUBLIC int mprStartModule(MprModule *mp)
{
    assert(mp);

    if (mp->start && !(mp->flags & MPR_MODULE_STARTED)) {
        if (mp->start(mp) < 0) {
            return MPR_ERR_CANT_INITIALIZE;
        }
    }
    mp->flags |= MPR_MODULE_STARTED;
    return 0;
}


PUBLIC int mprStopModule(MprModule *mp)
{
    assert(mp);

    if (mp->stop && (mp->flags & MPR_MODULE_STARTED) && !(mp->flags & MPR_MODULE_STOPPED)) {
        if (mp->stop(mp) < 0) {
            return MPR_ERR_NOT_READY;
        }
        mp->flags |= MPR_MODULE_STOPPED;
    }
    return 0;
}


/*
    See if a module is already loaded
 */
PUBLIC MprModule *mprLookupModule(cchar *name)
{
    MprModuleService    *ms;
    MprModule           *mp;
    int                 next;

    assert(name && name);
    if (!name) {
        return 0;
    }
    ms = MPR->moduleService;
    assert(ms);

    for (next = 0; (mp = mprGetNextItem(ms->modules, &next)) != 0; ) {
        assert(mp->name);
        if (mp && mp->name && strcmp(mp->name, name) == 0) {
            return mp;
        }
    }
    return 0;
}


PUBLIC void *mprLookupModuleData(cchar *name)
{
    MprModule   *module;

    if ((module = mprLookupModule(name)) == NULL) {
        return NULL;
    }
    return module->moduleData;
}


PUBLIC void mprSetModuleTimeout(MprModule *module, MprTicks timeout)
{
    assert(module);
    if (module) {
        module->timeout = timeout;
    }
}


PUBLIC void mprSetModuleFinalizer(MprModule *module, MprModuleProc stop)
{
    assert(module);
    if (module) {
        module->stop = stop;
    }
}


PUBLIC void mprSetModuleSearchPath(char *searchPath)
{
    MprModuleService    *ms;

    ms = MPR->moduleService;
    if (searchPath == 0) {
#ifdef CONFIG_VAPP_PREFIX
        ms->searchPath = sjoin(mprGetAppDir(), MPR_SEARCH_SEP, mprGetAppDir(), MPR_SEARCH_SEP, CONFIG_VAPP_PREFIX "/bin", NULL);
#else
        ms->searchPath = sjoin(mprGetAppDir(), MPR_SEARCH_SEP, mprGetAppDir(), NULL);
#endif
    } else {
        ms->searchPath = sclone(searchPath);
    }
}


PUBLIC cchar *mprGetModuleSearchPath()
{
    return MPR->moduleService->searchPath;
}


/*
    Load a module. The module is located by searching for the filename by optionally using the module search path.
 */
PUBLIC int mprLoadModule(MprModule *mp)
{
#if CONFIG_COMPILER_HAS_DYN_LOAD
    assert(mp);

    if (mprLoadNativeModule(mp) < 0) {
        return MPR_ERR_CANT_READ;
    }
    mprStartModule(mp);
    return 0;
#else
    mprLog("error mpr", 0, "mprLoadModule: %s failed", mp->name);
    mprLog("error mpr", 0, "Product built without the ability to load modules dynamically");
    return MPR_ERR_BAD_STATE;
#endif
}


PUBLIC int mprUnloadModule(MprModule *mp)
{
    mprDebug("mpr", 5, "Unloading native module %s from %s", mp->name, mp->path);
    if (mprStopModule(mp) < 0) {
        return MPR_ERR_NOT_READY;
    }
#if CONFIG_COMPILER_HAS_DYN_LOAD
    if (mp->flags & MPR_MODULE_LOADED) {
        if (mprUnloadNativeModule(mp) != 0) {
            mprLog("error mpr", 0, "Cannot unload module %s", mp->name);
        }
    }
#endif
    mprRemoveItem(MPR->moduleService->modules, mp);
    return 0;
}


#if CONFIG_COMPILER_HAS_DYN_LOAD
/*
    Return true if the shared library in "file" can be found. Return the actual path in *path. The filename
    may not have a shared library extension which is typical so calling code can be cross platform.
 */
static char *probe(cchar *filename)
{
    char    *path;

    assert(filename && *filename);

    if (mprPathExists(filename, R_OK)) {
        return sclone(filename);
    }
    if (strstr(filename, CONFIG_SHOBJ) == 0) {
        path = sjoin(filename, CONFIG_SHOBJ, NULL);
        if (mprPathExists(path, R_OK)) {
            return path;
        }
    }
    return 0;
}
#endif


/*
    Search for a module "filename" in the modulePath. Return the result in "result"
 */
PUBLIC char *mprSearchForModule(cchar *filename)
{
#if CONFIG_COMPILER_HAS_DYN_LOAD
    char    *path, *f, *searchPath, *dir, *tok;

    filename = mprNormalizePath(filename);

    /*
        Search for the path directly
     */
    if ((path = probe(filename)) != 0) {
        return path;
    }

    /*
        Search in the searchPath
     */
    searchPath = sclone(mprGetModuleSearchPath());
    tok = 0;
    dir = stok(searchPath, MPR_SEARCH_SEP, &tok);
    while (dir && *dir) {
        f = mprJoinPath(dir, filename);
        if ((path = probe(f)) != 0) {
            return path;
        }
        dir = stok(0, MPR_SEARCH_SEP, &tok);
    }
#endif /* CONFIG_COMPILER_HAS_DYN_LOAD */
    return 0;
}




