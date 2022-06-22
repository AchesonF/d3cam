#include "libmpr.h"

/**
    fs.c - File system services.

    This module provides a simple cross platform file system abstraction. File systems provide a file system switch and
    underneath a file system provider that implements actual I/O.
    This module is not thread-safe.

 */

/********************************** Includes **********************************/



/************************************ Code ************************************/

PUBLIC void mprInitFileSystem(MprFileSystem *fs, cchar *path)
{
    fs->separators = sclone("/");
    fs->newline = sclone("\n");
    fs->caseSensitive = 1;
    fs->root = sclone(path);
}


static int sortFs(MprFileSystem **fs1, MprFileSystem **fs2)
{
    int     rc;

    rc = scmp((*fs1)->root, (*fs2)->root);
    return -rc;
}


PUBLIC void mprAddFileSystem(MprFileSystem *fs)
{
    assert(fs);

    /*
        Sort in reverse order. This ensures lower down roots (longer) will be examined first
     */
    mprAddItem(MPR->fileSystems, fs);
    mprSortList(MPR->fileSystems, (MprSortProc) sortFs, 0);
}


PUBLIC MprFileSystem *mprLookupFileSystem(cchar *path)
{
    MprFileSystem   *fs;
    cchar           *rp, *pp;
    int             next;

    if (!path || *path == 0) {
        path = "/";
    }
    for (ITERATE_ITEMS(MPR->fileSystems, fs, next)) {
        for (rp = fs->root, pp = path; *rp & *pp; rp++, pp++) {
            if ((*rp == fs->separators[0] || *rp == fs->separators[1]) &&
                    (*pp == fs->separators[0] || *pp == fs->separators[1])) {
                continue;
            } else if (*rp != *pp) {
                break;
            }
        }
        if (*rp == 0) {
            return fs;
        }
    }
    return mprGetLastItem(MPR->fileSystems);
}


PUBLIC cchar *mprGetPathNewline(cchar *path)
{
    MprFileSystem   *fs;

    assert(path);

    fs = mprLookupFileSystem(path);
    return fs->newline;
}


PUBLIC cchar *mprGetPathSeparators(cchar *path)
{
    MprFileSystem   *fs;

    assert(path);

    fs = mprLookupFileSystem(path);
    return fs->separators;
}


PUBLIC char mprGetPathSeparator(cchar *path)
{
    MprFileSystem   *fs;

    assert(path);
    fs = mprLookupFileSystem(path);
    return fs->separators[0];
}


PUBLIC void mprSetPathSeparators(cchar *path, cchar *separators)
{
    MprFileSystem   *fs;

    assert(path);
    assert(separators);

    fs = mprLookupFileSystem(path);
    fs->separators = sclone(separators);
}


PUBLIC void mprSetPathNewline(cchar *path, cchar *newline)
{
    MprFileSystem   *fs;

    assert(path);
    assert(newline);

    fs = mprLookupFileSystem(path);
    fs->newline = sclone(newline);
}




