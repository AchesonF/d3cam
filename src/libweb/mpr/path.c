#include "libmpr.h"

/**
    path.c - Path (filename) services.

    This modules provides cross platform path name services.

 */

/********************************** Includes **********************************/



/********************************** Defines ***********************************/
/*
    Find the first separator in the path
 */
#define firstSep(fs, path)  strchr(path, fs->separators[0])


#define defaultSep(fs)          (fs->separators[0])

/*********************************** Forwards *********************************/

static MprList *globPathFiles(MprList *results, cchar *path, cchar *pattern, cchar *relativeTo, cchar *exclude, int flags);
static bool matchFile(cchar *filename, cchar *pattern);
static char *ptok(char *str, cchar *delim, char **last);
static cchar *rewritePattern(cchar *pattern, int flags);

/************************************* Code ***********************************/

static ME_INLINE bool isSep(MprFileSystem *fs, int c)
{
    char    *separators;

    assert(fs);
    for (separators = fs->separators; *separators; separators++) {
        if (*separators == c)
            return 1;
    }
    return 0;
}


static ME_INLINE bool hasDrive(MprFileSystem *fs, cchar *path)
{
    char    *cp, *endDrive;

    assert(fs);
    assert(path);

    if (fs->hasDriveSpecs) {
        cp = firstSep(fs, path);
        endDrive = strchr(path, ':');
        if (endDrive && (cp == NULL || endDrive < cp)) {
            return 1;
        }
    }
    return 0;
}


/*
    Return true if the path is absolute.
    This means the path portion after an optional drive specifier must begin with a directory speparator charcter.
    Cygwin returns true for "/abc" and "C:/abc".
 */
static ME_INLINE bool isAbsPath(MprFileSystem *fs, cchar *path)
{
    char    *cp, *endDrive;

    assert(fs);
    assert(path);

    if (path == NULL || *path == '\0') {
        return 0;
    }
    if (fs->hasDriveSpecs) {
        if ((cp = firstSep(fs, path)) != 0) {
            if ((endDrive = strchr(path, ':')) != 0) {
                if (&endDrive[1] == cp) {
                    return 1;
                }
            }
            if (cp == path) {
                return 1;
            }
        }
    } else {
        if (isSep(fs, path[0])) {
            return 1;
        }
    }
    return 0;
}


/*
    Return true if the path is a fully qualified absolute path.
    On windows, this means it must have a drive specifier.
    On cygwin, this means it must not have a drive specifier.
 */
static ME_INLINE bool isFullPath(MprFileSystem *fs, cchar *path)
{
    if (!fs || !path) {
        return 0;
    }

    if (isSep(fs, path[0])) {
        return 1;
    }
    return 0;
}


/*
    Return true if the directory is the root directory on a file system
 */
static ME_INLINE bool isRoot(MprFileSystem *fs, cchar *path)
{
    char    *cp;

    if (isAbsPath(fs, path)) {
        cp = firstSep(fs, path);
        if (cp && cp[1] == '\0') {
            return 1;
        }
    }
    return 0;
}


static ME_INLINE char *lastSep(MprFileSystem *fs, cchar *path)
{
    char    *cp;

    for (cp = (char*) &path[slen(path)] - 1; cp >= path; cp--) {
        if (isSep(fs, *cp)) {
            return cp;
        }
    }
    return 0;
}

/************************************ Code ************************************/
/*
    This copies a file.
 */
PUBLIC int mprCopyPath(cchar *fromName, cchar *toName, int mode)
{
    MprFile     *from, *to;
    ssize       count;
    char        buf[ME_BUFSIZE];

    if ((from = mprOpenFile(fromName, O_RDONLY | O_BINARY, 0)) == 0) {
        return MPR_ERR_CANT_OPEN;
    }
    if ((to = mprOpenFile(toName, O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, mode)) == 0) {
        return MPR_ERR_CANT_OPEN;
    }
    while ((count = mprReadFile(from, buf, sizeof(buf))) > 0) {
        mprWriteFile(to, buf, count);
    }
    mprCloseFile(from);
    mprCloseFile(to);
    return 0;
}


PUBLIC int mprDeletePath(cchar *path)
{
    MprFileSystem   *fs;

    if (path == NULL || *path == '\0') {
        return MPR_ERR_CANT_ACCESS;
    }
    fs = mprLookupFileSystem(path);
    return fs->deletePath(fs, path);
}


/*
    Return an absolute (normalized) path.
    On CYGWIN, this is a cygwin path with forward-slashes and without drive specs.
    Use mprGetWinPath for a windows style path with a drive specifier and back-slashes.

    Get an absolute (canonical) equivalent representation of a path. On windows this path will have
    back-slash directory separators and will have a drive specifier. On Cygwin, the path will be a Cygwin style
    path with forward-slash directory specifiers and without a drive specifier. If the path is outside the
    cygwin filesystem (outside c:/cygwin), the path will have a /cygdrive/DRIVE prefix. To get a windows style
    path on *NIX, use mprGetWinPath.
 */
PUBLIC char *mprGetAbsPath(cchar *path)
{
    MprFileSystem   *fs;
    cchar           *dir;
    char            *result;

    if (path == 0 || *path == '\0') {
        path = ".";
    }
    fs = mprLookupFileSystem(path);

    if (isFullPath(fs, path)) {
        /* Already absolute. On windows, must contain a drive specifier */
        result = mprNormalizePath(path);
        mprMapSeparators(result, defaultSep(fs));
        return result;
    }
    if (fs == MPR->romfs) {
        dir = mprGetCurrentPath();
        result = mprJoinPath(dir, path);
    } else {
        dir = mprGetCurrentPath();
        result = mprJoinPath(dir, path);
    }
    return result;
}


/*
    Get the directory containing the application executable. Tries to return an absolute path.
 */
PUBLIC cchar *mprGetAppDir()
{
    if (MPR->appDir == 0) {
        MPR->appDir = mprGetPathDir(mprGetAppPath());
    }
    return sclone(MPR->appDir);
}


/*
    Get the path for the application executable. Tries to return an absolute path.
 */
PUBLIC cchar *mprGetAppPath()
{
    if (MPR->appPath) {
        return sclone(MPR->appPath);
    }

    MprPath info;
    char    pbuf[ME_MAX_PATH], *path;
    int     len;

    path = sfmt("/proc/%i/exe", getpid());
    if (mprGetPathInfo(path, &info) == 0 && info.isLink) {
        len = readlink(path, pbuf, sizeof(pbuf) - 1);
        if (len > 0) {
            pbuf[len] = '\0';
            MPR->appPath = mprGetAbsPath(pbuf);
            MPR->appPath = mprJoinPath(mprGetPathDir(path), pbuf);
        } else {
            MPR->appPath = mprGetAbsPath(path);
        }
    } else {
        MPR->appPath = mprGetAbsPath(path);
    }

    return MPR->appPath;
}


/*
    This will return a fully qualified absolute path for the current working directory.
 */
PUBLIC cchar *mprGetCurrentPath()
{
    char    dir[ME_MAX_PATH];

    if (getcwd(dir, sizeof(dir)) == 0) {
        return mprGetAbsPath("/");
    }

    return sclone(dir);
}


PUBLIC cchar *mprGetFirstPathSeparator(cchar *path)
{
    MprFileSystem   *fs;

    fs = mprLookupFileSystem(path);
    return firstSep(fs, path);
}


/*
    Return a pointer into the path at the last path separator or null if none found
 */
PUBLIC cchar *mprGetLastPathSeparator(cchar *path)
{
    MprFileSystem   *fs;

    fs = mprLookupFileSystem(path);
    return lastSep(fs, path);
}


/*
    Return a path with native separators. This means "\\" on windows and cygwin
 */
PUBLIC char *mprGetNativePath(cchar *path)
{
    return mprTransformPath(path, MPR_PATH_NATIVE_SEP);
}


/*
    Return the last portion of a pathname. The separators are not mapped and the path is not cleaned.
 */
PUBLIC char *mprGetPathBase(cchar *path)
{
    MprFileSystem   *fs;
    char            *cp;

    if (path == 0) {
        return sclone("");
    }
    fs = mprLookupFileSystem(path);
    cp = (char*) lastSep(fs, path);
    if (cp == 0) {
        return sclone(path);
    }
    if (cp == path) {
        if (cp[1] == '\0') {
            return sclone(path);
        }
    } else if (cp[1] == '\0') {
        return sclone("");
    }
    return sclone(&cp[1]);
}


/*
    Return the last portion of a pathname. The separators are not mapped and the path is not cleaned.
    Caller must not free.
 */
PUBLIC cchar *mprGetPathBaseRef(cchar *path)
{
    MprFileSystem   *fs;
    char            *cp;

    if (path == 0) {
        return "";
    }
    fs = mprLookupFileSystem(path);
    if ((cp = (char*) lastSep(fs, path)) == 0) {
        return path;
    }
    if (cp == path) {
        if (cp[1] == '\0') {
            return path;
        }
    }
    return &cp[1];
}


/*
    Return the directory portion of a pathname.
 */
PUBLIC char *mprGetPathDir(cchar *path)
{
    MprFileSystem   *fs;
    cchar           *cp, *start;
    char            *result;
    ssize          len;

    assert(path);

    if (path == 0 || *path == '\0') {
        return sclone(path);
    }

    fs = mprLookupFileSystem(path);
    len = slen(path);
    cp = &path[len - 1];
    start = hasDrive(fs, path) ? strchr(path, ':') + 1 : path;

    /*
        Step back over trailing slashes
     */
    while (cp > start && isSep(fs, *cp)) {
        cp--;
    }
    for (; cp > start && !isSep(fs, *cp); cp--) { }

    if (cp == start) {
        if (!isSep(fs, *cp)) {
            /* No slashes found, parent is current dir */
            return sclone(".");
        }
        cp++;
    }
    len = (cp - path);
    result = mprAlloc(len + 1);
    mprMemcpy(result, len + 1, path, len);
    result[len] = '\0';
    return result;
}


/*
    Return the extension portion of a pathname. Returns the extension without the ".". Returns null if no extension.
 */
PUBLIC char *mprGetPathExt(cchar *path)
{
    MprFileSystem  *fs;
    char            *cp;

    if ((cp = srchr(path, '.')) != NULL) {
        fs = mprLookupFileSystem(path);
        /*
            If there is no separator ("/") after the extension, then use it.
         */
        if (firstSep(fs, cp) == 0) {
            return sclone(++cp);
        }
    }
    return 0;
}


static int sortFiles(MprDirEntry **dp1, MprDirEntry **dp2)
{
    return strcmp((*dp1)->name, (*dp2)->name);
}


/*
    Find files in the directory "dir". If base is set, use that as the prefix for returned files.
    Returns a list of MprDirEntry objects.
 */
static MprList *findFiles(MprList *list, cchar *dir, cchar *base, int flags)
{
    MprDirEntry     *dp;
    MprList         *files;
    char            *name;
    int             next;

    if ((files = mprGetDirList(dir)) == 0) {
        return 0;
    }
    for (next = 0; (dp = mprGetNextItem(files, &next)) != 0; ) {
        if (dp->name[0] == '.') {
            if (dp->name[1] == '\0' || (dp->name[1] == '.' && dp->name[2] == '\0')) {
                continue;
            }
            if (!(flags & MPR_PATH_INC_HIDDEN)) {
                continue;
            }
        }
        name = dp->name;
        dp->name = mprJoinPath(base, name);

        if (!(flags & MPR_PATH_DEPTH_FIRST) && !(dp->isDir && flags & MPR_PATH_NO_DIRS)) {
            mprAddItem(list, dp);
        }
        if (dp->isDir) {
            if (flags & MPR_PATH_DESCEND) {
                findFiles(list, mprJoinPath(dir, name), mprJoinPath(base, name), flags);
            }
        }
        if ((flags & MPR_PATH_DEPTH_FIRST) && (!(dp->isDir && flags & MPR_PATH_NO_DIRS))) {
            mprAddItem(list, dp);
        }
    }

    /* Linux returns directories not sorted */
    mprSortList(list, (MprSortProc) sortFiles, 0);

    return list;
}


/*
    Get the files in a directory. Returns a list of MprDirEntry objects.

    MPR_PATH_DESCEND        to traverse subdirectories
    MPR_PATH_DEPTH_FIRST    to do a depth-first traversal
    MPR_PATH_INC_HIDDEN     to include hidden files
    MPR_PATH_NO_DIRS        to exclude subdirectories
    MPR_PATH_RELATIVE       to return paths relative to the initial directory
 */
PUBLIC MprList *mprGetPathFiles(cchar *dir, int flags)
{
    MprList *list;
    cchar   *base;

    if (dir == 0 || *dir == '\0') {
        dir = ".";
    }
    base = (flags & MPR_PATH_RELATIVE) ? 0 : dir;
    if ((list = findFiles(mprCreateList(-1, 0), dir, base, flags)) == 0) {
        return 0;
    }
    return list;
}


/*
    Skip over double wilds to the next non-double wild segment
    Return the first pattern segment as a result.
    Return in reference arg the following pattern and set *dwild if a double wild was skipped.
    This routine clones the original pattern to preserve it.
 */
static cchar *getNextPattern(cchar *pattern, cchar **nextPat, bool *dwild)
{
    MprFileSystem   *fs;
    char            *pp, *thisPat;

    pp = sclone(pattern);
    fs = mprLookupFileSystem(pp);
    *dwild = 0;

    while (1) {
        thisPat = ptok(pp, fs->separators, &pp);
        if (smatch(thisPat, "**") == 0) {
            break;
        }
        *dwild = 1;
    }
    if (nextPat) {
        *nextPat = pp;
    }
    return thisPat;
}


/*
    Glob a full multi-segment path and return a list of matching files

    relativeTo  - Relative files are relative to this directory.
    path        - Directory to search. Will be a physical directory path.
    pattern     - Search pattern with optional wildcards.
    exclude     - Exclusion pattern

    As this routine recurses, 'relativeTo' does not change, but path and pattern will.
 */
static MprList *globPathFiles(MprList *results, cchar *path, cchar *pattern, cchar *relativeTo, cchar *exclude, int flags)
{
    MprDirEntry     *dp;
    MprList         *list;
    cchar           *filename, *nextPat, *thisPat;
    bool            dwild;
    int             add, matched, next;

    if ((list = mprGetPathFiles(path, (flags & ~MPR_PATH_NO_DIRS) | MPR_PATH_RELATIVE)) == 0) {
        return results;
    }
    thisPat = getNextPattern(pattern, &nextPat, &dwild);

    for (next = 0; (dp = mprGetNextItem(list, &next)) != 0; ) {
        if (flags & MPR_PATH_RELATIVE) {
            filename = mprGetRelPath(mprJoinPath(path, dp->name), relativeTo);
        } else {
            filename = mprJoinPath(path, dp->name);
        }
        if ((matched = matchFile(dp->name, thisPat)) == 0) {
            if (dwild) {
                if (thisPat == 0) {
                    matched = 1;
                } else {
                    /* Match failed, so backup the pattern and try the double wild for this filename (only) */
                    globPathFiles(results, mprJoinPath(path, dp->name), pattern, relativeTo, exclude, flags);
                    continue;
                }
            }
        }
        add = (matched && (!nextPat || smatch(nextPat, "**")));
        if (add && exclude && matchFile(filename, exclude)) {
            continue;
        }
        if (add && dp->isDir && (flags & MPR_PATH_NO_DIRS)) {
            add = 0;
        }
        if (add && !(flags & MPR_PATH_DEPTH_FIRST)) {
            mprAddItem(results, filename);
        }
        if (dp->isDir) {
            if (dwild) {
                globPathFiles(results, mprJoinPath(path, dp->name), pattern, relativeTo, exclude, flags);
            } else if (matched && nextPat) {
                globPathFiles(results, mprJoinPath(path, dp->name), nextPat, relativeTo, exclude, flags);
            }
        }
        if (add && (flags & MPR_PATH_DEPTH_FIRST)) {
            mprAddItem(results, filename);
        }
    }
    return results;
}


/*
    Get the files in a directory and subdirectories using glob-style matching
 */
PUBLIC MprList *mprGlobPathFiles(cchar *path, cchar *pattern, int flags)
{
    MprFileSystem   *fs;
    MprList         *result;
    cchar           *exclude, *relativeTo;
    char            *pat, *special, *start;

    result = mprCreateList(0, 0);
    if (path && pattern) {
        fs = mprLookupFileSystem(path);
        exclude = 0;
        pat = 0;
        relativeTo = (flags & MPR_PATH_RELATIVE) ? path : 0;
        /*
            Adjust path to include any fixed segements from the pattern
         */
        start = sclone(pattern);
        if ((special = strpbrk(start, "*?")) != 0) {
            if (special > start) {
                for (pat = special; pat > start && !strchr(fs->separators, *pat); pat--) { }
                if (pat > start) {
                    *pat++ = '\0';
                    path = mprJoinPath(path, start);
                }
                pattern = pat;
            }
        } else {
            pat = (char*) mprGetPathBaseRef(start);
            if (pat > start) {
                pat[-1] = '\0';
                path = mprJoinPath(path, start);
            }
            pattern = pat;
        }
        if (*pattern == '!') {
            exclude = &pattern[1];
        }
        globPathFiles(result, path, rewritePattern(pattern, flags), relativeTo, exclude, flags);
        if (!(flags & (MPR_PATH_DEPTH_FIRST))) {
            mprSortList(result, NULL, NULL);
        }
    }
    return result;
}


/*
    Special version of stok that does not skip leading delimiters.
    Need this to handle leading "/path". This is handled as an empty "" filename segment
    This then works (automagically) for windows drives "C:/"

    Warning: this modifies the "str"
 */
static char *ptok(char *str, cchar *delim, char **last)
{
    char    *start, *end;
    ssize   i;

    assert(delim);
    start = (str || !last) ? str : *last;
    if (start == 0) {
        if (last) {
            *last = 0;
        }
        return 0;
    }
    /* Don't skip delimiters at the start
    i = strspn(start, delim);
    start += i;
    */
    if (*start == '\0') {
        if (last) {
            *last = 0;
        }
        return 0;
    }
    end = strpbrk(start, delim);
    if (end) {
        *end++ = '\0';
        i = strspn(end, delim);
        end += i;
    }
    if (last) {
        *last = end;
    }
    return start;
}


/*
    Convert pattern to canonical form:
    abc** => abc* / **
    **abc => ** / *abc
 */
static cchar *rewritePattern(cchar *pattern, int flags)
{
    MprFileSystem   *fs;
    MprBuf          *buf;
    cchar           *cp;

    fs = mprLookupFileSystem(pattern);
    if (!scontains(pattern, "**") && !(flags & MPR_PATH_DESCEND)) {
        return pattern;
    }
    if (flags & MPR_PATH_DESCEND) {
        pattern = mprJoinPath(pattern, "**");
    }
    buf = mprCreateBuf(0, 0);
    for (cp = pattern; *cp; cp++) {
        if (cp[0] == '*' && cp[1] == '*') {
            if (isSep(fs, cp[2]) && cp[3] == '*' && cp[4] == '*') {
                /* Remove redundant ** */
                cp += 3;
            }
            if (cp > pattern && !isSep(fs, cp[-1])) {
                // abc** => abc*/**
                mprPutCharToBuf(buf, '*');
                mprPutCharToBuf(buf, fs->separators[0]);
            }
            mprPutCharToBuf(buf, '*');
            mprPutCharToBuf(buf, '*');
            if (cp[2] && !isSep(fs, cp[2])) {
                // **abc  => **/*abc
                mprPutCharToBuf(buf, fs->separators[0]);
                mprPutCharToBuf(buf, '*');
            }
            cp++;
        } else {
            mprPutCharToBuf(buf, *cp);
        }
    }
    mprAddNullToBuf(buf);
    return mprGetBufStart(buf);
}



/*
    Match a single filename (without separators) to a pattern (without separators).
    This supports the wildcards '?' and '*'. This routine does not handle double wild.
    If filename or pattern are null, returns false.
    Pattern may be an empty string -- will only match an empty filename. Used for matching leading "/".
 */
static bool matchFile(cchar *filename, cchar *pattern)
{
    MprFileSystem   *fs;
    cchar           *fp, *pp;

    if (filename == pattern) {
        return 1;
    }
    if (!filename || !pattern) {
        return 0;
    }
    fs = mprLookupFileSystem(filename);
    for (fp = filename, pp = pattern; *fp && *pp; fp++, pp++) {
        if (*pp == '?') {
            continue;
        } else if (*pp == '*') {
            if (matchFile(&fp[1], pp)) {
                return 1;
            }
            fp--;
            continue;
        } else {
            if (fs->caseSensitive) {
                if (*fp != *pp) {
                    return 0;
                }
            } else if (tolower((uchar) *fp) != tolower((uchar) *pp)) {
                return 0;
            }
        }
    }
    if (*fp) {
        return 0;
    }
    if (*pp) {
        /* Trailing '*' or '**' */
        if (!((pp[0] == '*' && pp[1] == '\0') || (pp[0] == '*' && pp[1] == '*' && pp[2] == '\0'))) {
            return 0;
        }
    }
    return 1;
}


/*
    Pattern is in canonical form where "**" is always a segment by itself
 */
static bool matchPath(MprFileSystem *fs, cchar *path, cchar *pattern)
{
    cchar   *nextPat, *thisFile, *thisPat;
    char    *pp;
    bool    dwild;

    pp = sclone(path);
    while (pattern && pp) {
        thisFile = ptok(pp, fs->separators, &pp);
        thisPat = getNextPattern(pattern, &nextPat, &dwild);
        if (matchFile(thisFile, thisPat)) {
            if (dwild) {
                /*
                    Matching '**' but more pattern to come. Must support back-tracking
                    So recurse and match the next pattern and if that fails, continue with '**'
                 */
                if (matchPath(fs, pp, nextPat)) {
                    return 1;
                }
                nextPat = pattern;
            }
        } else {
            if (dwild) {
                if (pp) {
                    return matchPath(fs, pp, pattern);
                } else {
                    return thisPat ? 0 : 1;
                }
            }
            return 0;
        }
        pattern = nextPat;
    }
    return ((pattern && *pattern) || (pp && *pp)) ? 0 : 1;
}


PUBLIC bool mprMatchPath(cchar *path, cchar *pattern)
{
    MprFileSystem   *fs;

    if (!path || !pattern) {
        return 0;
    }
    fs = mprLookupFileSystem(path);
    return matchPath(fs, path, rewritePattern(pattern, 0));
}



/*
    Return the first directory of a pathname
 */
PUBLIC char *mprGetPathFirstDir(cchar *path)
{
    MprFileSystem   *fs;
    cchar           *cp;
    int             len;

    assert(path);

    fs = mprLookupFileSystem(path);
    if (isAbsPath(fs, path)) {
        len = hasDrive(fs, path) ? 2 : 1;
        return snclone(path, len);
    } else {
        if ((cp = firstSep(fs, path)) != 0) {
            return snclone(path, cp - path);
        }
        return sclone(path);
    }
}


PUBLIC int mprGetPathInfo(cchar *path, MprPath *info)
{
    MprFileSystem  *fs;

    if (!info) {
        return MPR_ERR_BAD_ARGS;
    }
    if (!path) {
        info->valid = 0;
        return MPR_ERR_BAD_ARGS;
    }
    fs = mprLookupFileSystem(path);
    return fs->getPathInfo(fs, path, info);
}


PUBLIC char *mprGetPathLink(cchar *path)
{
    MprFileSystem  *fs;

    fs = mprLookupFileSystem(path);
    return fs->getPathLink(fs, path);
}


/*
    GetPathParent is smarter than GetPathDir which operates purely textually on the path. GetPathParent will convert
    relative paths to absolute to determine the parent directory.
 */
PUBLIC char *mprGetPathParent(cchar *path)
{
    MprFileSystem   *fs;
    char            *dir;

    fs = mprLookupFileSystem(path);

    if (path == 0 || path[0] == '\0') {
        return mprGetAbsPath(".");
    }
    if (firstSep(fs, path) == NULL) {
        /*
            No parents in the path, so convert to absolute
         */
        dir = mprGetAbsPath(path);
        return mprGetPathDir(dir);
    }
    return mprGetPathDir(path);
}


PUBLIC char *mprGetPortablePath(cchar *path)
{
    char    *result, *cp;

    result = mprTransformPath(path, 0);
    for (cp = result; *cp; cp++) {
        if (*cp == '\\') {
            *cp = '/';
        }
    }
    return result;
}


/*
    Get a relative path from an origin path to a destination. If a relative path cannot be obtained,
    an absolute path to the destination will be returned. This happens if the paths cross drives.
    Returns the supplied destArg modified to be relative to originArg.
 */
PUBLIC char *mprGetRelPath(cchar *destArg, cchar *originArg)
{
    MprFileSystem   *fs;
    cchar           *dp, *lastdp, *lastop, *op, *origin;
    char            *result, *dest, *rp;
    int             originSegments, i, commonSegments, sep;

    fs = mprLookupFileSystem(destArg);

    if (destArg == 0 || *destArg == '\0') {
        return sclone(".");
    }
    dest = mprNormalizePath(destArg);

    if (!isAbsPath(fs, dest) && (originArg == 0 || *originArg == '\0')) {
        return dest;
    }
    sep = (dp = firstSep(fs, dest)) ? *dp : defaultSep(fs);

    if (originArg == 0 || *originArg == '\0') {
        /*
            Get the working directory. Ensure it is null terminated and leave room to append a trailing separators
            On cygwin, this will be a cygwin style path (starts with "/" and no drive specifier).
         */
        origin = mprGetCurrentPath();
    } else {
        origin = mprGetAbsPath(originArg);
    }
    dest = mprGetAbsPath(dest);

    /*
        Count segments in origin working directory. Ignore trailing separators.
     */
    for (originSegments = 0, dp = origin; *dp; dp++) {
        if (isSep(fs, *dp) && dp[1]) {
            originSegments++;
        }
    }

    /*
        Find portion of dest that matches the origin directory, if any. Start at -1 because matching root doesn't count.
     */
    commonSegments = -1;
    for (lastop = op = origin, lastdp = dp = dest; *op && *dp; op++, dp++) {
        if (isSep(fs, *op)) {
            lastop = op + 1;
            if (isSep(fs, *dp)) {
                lastdp = dp + 1;
                commonSegments++;
            }
        } else if (fs->caseSensitive) {
            if (*op != *dp) {
                break;
            }
        } else if (*op != *dp && tolower((uchar) *op) != tolower((uchar) *dp)) {
            break;
        }
    }
    if (commonSegments < 0) {
        /* Different drives - must return absolute path */
        return dest;
    }

    if ((*op && *dp) || (*op && *dp && !isSep(fs, *op) && !isSep(fs, *dp))) {
        /*
            Cases:
            /seg/abc>   Path('/seg/xyz').relative       # differing trailing segment
            /seg/abc>   Path('/seg/abcd)                # common last segment prefix, dest longer
            /seg/abc>   Path('/seg/ab')                 # common last segment prefix, origin longer
        */
        op = lastop;
        dp = lastdp;
    }

    /*
        Add one more segment if the last segment matches. Handle trailing separators.
     */
    if ((isSep(fs, *op) || *op == '\0') && (isSep(fs, *dp) || *dp == '\0')) {
        commonSegments++;
    }
    if (isSep(fs, *dp)) {
        dp++;
    }
    rp = result = mprAlloc(originSegments * 3 + slen(dest) + 2);
    for (i = commonSegments; i < originSegments; i++) {
        *rp++ = '.';
        *rp++ = '.';
        *rp++ = defaultSep(fs);
    }
    if (*dp) {
        strcpy(rp, dp);
    } else if (rp > result) {
        /*
            Cleanup trailing separators ("../" is the end of the new path)
         */
        rp[-1] = '\0';
    } else {
        strcpy(result, ".");
    }
    mprMapSeparators(result, sep);
    return result;
}


/*
    Get a temporary file name. The file is created in the system temp location.
 */
PUBLIC char *mprGetTempPath(cchar *tempDir)
{
    MprFile         *file;
    cchar           *name;
    char            *dir, *path;
    int             i, now;
    static int      tempSeed = 0;

    if (tempDir == 0 || *tempDir == '\0') {
        dir = sclone("/tmp");
    } else {
        dir = sclone(tempDir);
    }
    now = ((int) mprGetTime() & 0xFFFF) % 64000;
    file = 0;
    path = 0;
    name = MPR->name ? MPR->name : "MPR";
    for (i = 0; i < 128; i++) {
        path = sfmt("%s/%s-%d-%d-%d.tmp", dir, mprGetPathBase(name), getpid(), now, ++tempSeed);
        file = mprOpenFile(path, O_CREAT | O_EXCL | O_BINARY, 0664);
        if (file) {
            mprCloseFile(file);
            break;
        }
    }
    if (file == 0) {
        return 0;
    }
    return path;
}



PUBLIC char *mprGetWinPath(cchar *path)
{
    char            *result;

    if (path == 0 || *path == '\0') {
        path = ".";
    }

    result = mprNormalizePath(path);
    mprMapSeparators(result, '\\');

    return result;
}


PUBLIC bool mprIsPathContained(cchar *path, cchar *dir)
{
    ssize   len;
    char    *base;

    dir = mprGetAbsPath(dir);
    path = mprGetAbsPath(path);
    len = slen(dir);
    if (len <= slen(path)) {
        base = sclone(path);
        base[len] = '\0';
        if (mprSamePath(dir, base)) {
            return 1;
        }
    }
    return 0;
}


PUBLIC bool mprIsAbsPathContained(cchar *path, cchar *dir)
{
    MprFileSystem   *fs;
    ssize            len;

    assert(mprIsPathAbs(path));
    assert(mprIsPathAbs(dir));
    len = slen(dir);
    if (len <= slen(path)) {
        fs = mprLookupFileSystem(path);
        if (mprSamePathCount(dir, path, len) && (path[len] == '\0' || isSep(fs, path[len]))) {
            return 1;
        }
    }
    return 0;
}


PUBLIC bool mprIsPathAbs(cchar *path)
{
    MprFileSystem   *fs;

    fs = mprLookupFileSystem(path);
    return isAbsPath(fs, path);
}


PUBLIC bool mprIsPathDir(cchar *path)
{
    MprPath     info;

    return (mprGetPathInfo(path, &info) == 0 && info.isDir);
}


PUBLIC bool mprIsPathRel(cchar *path)
{
    MprFileSystem   *fs;

    fs = mprLookupFileSystem(path);
    return !isAbsPath(fs, path);
}


PUBLIC bool mprIsPathSeparator(cchar *path, cchar c)
{
    MprFileSystem   *fs;

    fs = mprLookupFileSystem(path);
    return isSep(fs, c);
}


/*
    Join paths. Returns a joined (normalized) path.
    If other is absolute, then return other. If other is null, empty or "." then return path.
    The separator is chosen to match the first separator found in either path. If none, it uses the default separator.
 */
PUBLIC char *mprJoinPath(cchar *path, cchar *other)
{
    MprFileSystem   *pfs, *ofs;
    char            *join, *drive, *cp;
    int             sep;

    if (other == NULL || *other == '\0' || strcmp(other, ".") == 0) {
        return sclone(path);
    }
    pfs = mprLookupFileSystem(path);
    ofs = mprLookupFileSystem(other);

    if (isAbsPath(ofs, other)) {
        if (ofs->hasDriveSpecs && !isFullPath(ofs, other) && isFullPath(pfs, path)) {
            /*
                Other is absolute, but without a drive. Use the drive from path.
             */
            drive = sclone(path);
            if ((cp = strchr(drive, ':')) != 0) {
                *++cp = '\0';
            }
            return sjoin(drive, other, NULL);
        } else {
            return mprNormalizePath(other);
        }
    }
    if (path == NULL || *path == '\0') {
        return mprNormalizePath(other);
    }
    if ((cp = firstSep(pfs, path)) != 0) {
        sep = *cp;
    } else if ((cp = firstSep(ofs, other)) != 0) {
        sep = *cp;
    } else {
        sep = defaultSep(ofs);
    }
    if ((join = sfmt("%s%c%s", path, sep, other)) == 0) {
        return 0;
    }
    return mprNormalizePath(join);
}


PUBLIC char *mprJoinPaths(cchar *base, ...)
{
    va_list     args;
    cchar       *path;

    va_start(args, base);
    while ((path = va_arg(args, char*)) != 0) {
        base = mprJoinPath(base, path);
    }
    va_end(args);
    return (char*) base;
}


/*
    Join an extension to a path. If path already has an extension, this call does nothing.
    The extension should not have a ".", but this routine is tolerant if it does.
 */
PUBLIC char *mprJoinPathExt(cchar *path, cchar *ext)
{
    MprFileSystem   *fs;
    char            *cp;

    fs = mprLookupFileSystem(path);
    if (ext == NULL || *ext == '\0') {
        return sclone(path);
    }
    cp = srchr(path, '.');
    if (cp && firstSep(fs, cp) == 0) {
        return sclone(path);
    }
    if (ext[0] == '.') {
        return sjoin(path, ext, NULL);
    } else {
        return sjoin(path, ".", ext, NULL);
    }
}


/*
    Make a directory with all necessary intervening directories.
 */
PUBLIC int mprMakeDir(cchar *path, int perms, int owner, int group, bool makeMissing)
{
    MprFileSystem   *fs;
    char            *parent;
    int             rc;

    fs = mprLookupFileSystem(path);

    if (mprPathExists(path, X_OK)) {
        return 0;
    }
    if (fs->makeDir(fs, path, perms, owner, group) == 0) {
        return 0;
    }
    if (makeMissing && !isRoot(fs, path)) {
        parent = mprGetPathParent(path);
        if (!mprPathExists(parent, X_OK)) {
            if ((rc = mprMakeDir(parent, perms, owner, group, makeMissing)) < 0) {
                return rc;
            }
        }
        return fs->makeDir(fs, path, perms, owner, group);
    }
    return MPR_ERR_CANT_CREATE;
}


PUBLIC int mprMakeLink(cchar *path, cchar *target, bool hard)
{
    MprFileSystem   *fs;

    if (mprPathExists(path, X_OK)) {
        return 0;
    }
    fs = mprLookupFileSystem(target);
    return fs->makeLink(fs, path, target, hard);
}


/*
    Normalize a path to remove redundant "./" and cleanup "../" and make separator uniform. Does not make an abs path.
    It does not map separators, change case, nor add drive specifiers.
 */
PUBLIC char *mprNormalizePath(cchar *pathArg)
{
    MprFileSystem   *fs;
    char            *path, *sp, *dp, *mark, **segments;
    ssize           len;
    int             addSep, i, segmentCount, hasDot, last, sep;

    if (pathArg == 0 || *pathArg == '\0') {
        return sclone("");
    }
    fs = mprLookupFileSystem(pathArg);

    /*
        Allocate one spare byte incase we need to break into segments. If so, will add a trailing "/" to make
        parsing easier later.
     */
    len = slen(pathArg);
    if ((path = mprAlloc(len + 2)) == 0) {
        return NULL;
    }
    strcpy(path, pathArg);
    sep = (sp = firstSep(fs, path)) ? *sp : defaultSep(fs);

    /*
        Remove multiple path separators. Check if we have any "." characters and count the number of path segments
        Map separators to the first separator found
     */
    hasDot = segmentCount = 0;
    for (sp = dp = path; *sp; ) {
        if (isSep(fs, *sp)) {
            *sp = sep;
            segmentCount++;
            while (isSep(fs, sp[1])) {
                sp++;
            }
        }
        if (*sp == '.') {
            hasDot++;
        }
        *dp++ = *sp++;
    }
    *dp = '\0';
    if (!sep) {
        sep = defaultSep(fs);
    }
    if (!hasDot && segmentCount == 0) {
        if (fs->hasDriveSpecs) {
            last = path[slen(path) - 1];
            if (last == ':') {
                path = sjoin(path, ".", NULL);
            }
        }
        return path;
    }

    if (dp > path && !isSep(fs, dp[-1])) {
        *dp++ = sep;
        *dp = '\0';
        segmentCount++;
    }

    /*
        Have dots to process so break into path segments. Add one incase we need have an absolute path with a drive-spec.
     */
    assert(segmentCount > 0);
    if ((segments = mprAlloc(sizeof(char*) * (segmentCount + 1))) == 0) {
        return NULL;
    }

    /*
        NOTE: The root "/" for absolute paths will be stored as empty.
     */
    len = 0;
    for (i = 0, mark = sp = path; *sp; sp++) {
        if (isSep(fs, *sp)) {
            *sp = '\0';
            if (*mark == '.' && mark[1] == '\0' && segmentCount > 1) {
                /* Remove "."  However, preserve lone "." */
                mark = sp + 1;
                segmentCount--;
                continue;
            }
            if (*mark == '.' && mark[1] == '.' && mark[2] == '\0' && i > 0 && strcmp(segments[i-1], "..") != 0) {
                /* Erase ".." and previous segment */
                if (*segments[i - 1] == '\0' ) {
                    assert(i == 1);
                    /* Previous segement is "/". Prevent escape from root */
                    segmentCount--;
                } else {
                    i--;
                    segmentCount -= 2;
                }
                assert(segmentCount >= 0);
                mark = sp + 1;
                continue;
            }
            segments[i++] = mark;
            len += (sp - mark);
#if DISABLE
            if (i == 1 && segmentCount == 1 && fs->hasDriveSpecs && strchr(mark, ':') != 0) {
                /*
                    Normally we truncate a trailing "/", but this is an absolute path with a drive spec (c:/).
                 */
                segments[i++] = "";
                segmentCount++;
            }
#endif
            mark = sp + 1;
        }
    }

    if (--sp > mark) {
        segments[i++] = mark;
        len += (sp - mark);
    }
    assert(i <= segmentCount);
    segmentCount = i;

    if (segmentCount <= 0) {
        return sclone(".");
    }

    addSep = 0;
    sp = segments[0];
    if (fs->hasDriveSpecs && *sp != '\0') {
        last = sp[slen(sp) - 1];
        if (last == ':') {
            /* This matches an original path of: "c:/" but not "c:filename" */
            addSep++;
        }
    }

    if ((path = mprAlloc(len + segmentCount + 1)) == 0) {
        return NULL;
    }
    assert(segmentCount > 0);

    /*
        First segment requires special treatment due to drive specs
     */
    dp = path;
    strcpy(dp, segments[0]);
    dp += slen(segments[0]);

    if (segmentCount == 1 && (addSep || (*segments[0] == '\0'))) {
        *dp++ = sep;
    }

    for (i = 1; i < segmentCount; i++) {
        *dp++ = sep;
        strcpy(dp, segments[i]);
        dp += slen(segments[i]);
    }
    *dp = '\0';
    return path;
}


PUBLIC void mprMapSeparators(char *path, int separator)
{
    MprFileSystem   *fs;
    char            *cp;

    if ((fs = mprLookupFileSystem(path)) == 0) {
        return;
    }
    if (separator == 0) {
        separator = fs->separators[0];
    }
    for (cp = path; *cp; cp++) {
        if (isSep(fs, *cp)) {
            *cp = separator;
        }
    }
}


PUBLIC bool mprPathExists(cchar *path, int omode)
{
    MprFileSystem  *fs;

    if (path == 0 || *path == '\0') {
        return 0;
    }
    fs = mprLookupFileSystem(path);
    return fs->accessPath(fs, path, omode);
}


PUBLIC char *mprReadPathContents(cchar *path, ssize *lenp)
{
    MprFile     *file;
    MprPath     info;
    ssize       len;
    char        *buf;

    if (!path) {
        return 0;
    }
    if ((file = mprOpenFile(path, O_RDONLY | O_BINARY, 0)) == 0) {
        return 0;
    }
    if (mprGetPathInfo(path, &info) < 0) {
        mprCloseFile(file);
        return 0;
    }
    len = (ssize) info.size;
    if ((buf = mprAlloc(len + 1)) == 0) {
        mprCloseFile(file);
        return 0;
    }
    if (mprReadFile(file, buf, len) != len) {
        mprCloseFile(file);
        return 0;
    }
    buf[len] = '\0';
    if (lenp) {
        *lenp = len;
    }
    mprCloseFile(file);
    return buf;
}


PUBLIC int mprRenamePath(cchar *from, cchar *to)
{
    return rename(from, to);
}


PUBLIC char *mprReplacePathExt(cchar *path, cchar *ext)
{
    if (ext == NULL || *ext == '\0') {
        return sclone(path);
    }
    path = mprTrimPathExt(path);
    /*
        Don't use mprJoinPathExt incase path has an embedded "."
     */
    if (ext[0] == '.') {
        return sjoin(path, ext, NULL);
    } else {
        return sjoin(path, ".", ext, NULL);
    }
}


/*
    Resolve paths in the neighborhood of a base path. Resolve operates like join, except that it joins the
    given paths to the directory portion of the current (base) path. For example:
    mprResolvePath("/usr/bin/mpr/bin", "lib") will return "/usr/lib/mpr/lib". i.e. it will return the
    sibling directory "lib".

    Resolve operates by determining a virtual current directory for this Path object. It then successively
    joins the given paths to the directory portion of the current result. If the next path is an absolute path,
    it is used unmodified.  The effect is to find the given paths with a virtual current directory set to the
    directory containing the prior path.

    Resolve is useful for creating paths in the region of the current path and gracefully handles both
    absolute and relative path segments.

    Returns a joined (normalized) path.
    If path is absolute, then return path. If path is null, empty or "." then return path.
 */
PUBLIC char *mprResolvePath(cchar *base, cchar *path)
{
    MprFileSystem   *fs;
    char            *join, *drive, *cp, *dir;

    fs = mprLookupFileSystem(base);
    if (path == NULL || *path == '\0' || strcmp(path, ".") == 0) {
        return sclone(base);
    }
    if (isAbsPath(fs, path)) {
        if (fs->hasDriveSpecs && !isFullPath(fs, path) && isFullPath(fs, base)) {
            /*
                Other is absolute, but without a drive. Use the drive from base.
             */
            drive = sclone(base);
            if ((cp = strchr(drive, ':')) != 0) {
                *++cp = '\0';
            }
            return sjoin(drive, path, NULL);
        }
        return mprNormalizePath(path);
    }
    if (base == NULL || *base == '\0') {
        return mprNormalizePath(path);
    }
    dir = mprGetPathDir(base);
    if ((join = sfmt("%s/%s", dir, path)) == 0) {
        return 0;
    }
    return mprNormalizePath(join);
}


/*
    Compare two file path to determine if they point to the same file.
 */
PUBLIC int mprSamePath(cchar *path1, cchar *path2)
{
    MprFileSystem   *fs1, *fs2;
    cchar           *p1, *p2;

    if (!path1 || !path2) {
        return 0;
    }
    fs1 = mprLookupFileSystem(path1);
    fs2 = mprLookupFileSystem(path2);
    if (fs1 != fs2) {
        return 0;
    }

    /*
        Convert to absolute (normalized) paths to compare.
     */
    if (!isFullPath(fs1, path1)) {
        path1 = mprGetAbsPath(path1);
    } else {
        path1 = mprNormalizePath(path1);
    }
    if (!isFullPath(fs2, path2)) {
        path2 = mprGetAbsPath(path2);
    } else {
        path2 = mprNormalizePath(path2);
    }
    if (fs1->caseSensitive) {
        for (p1 = path1, p2 = path2; *p1 && *p2; p1++, p2++) {
            if (*p1 != *p2 && !(isSep(fs1, *p1) && isSep(fs2, *p2))) {
                break;
            }
        }
    } else {
        for (p1 = path1, p2 = path2; *p1 && *p2; p1++, p2++) {
            if (tolower((uchar) *p1) != tolower((uchar) *p2) && !(isSep(fs1, *p1) && isSep(fs2, *p2))) {
                break;
            }
        }
    }
    return (*p1 == *p2);
}


/*
    Compare two file path to determine if they point to the same file.
 */
PUBLIC int mprSamePathCount(cchar *path1, cchar *path2, ssize len)
{
    MprFileSystem   *fs1, *fs2;
    cchar           *p1, *p2;

    if (!path1 || !path2) {
        return 0;
    }
    fs1 = mprLookupFileSystem(path1);
    fs2 = mprLookupFileSystem(path2);

    if (fs1 != fs2) {
        return 0;
    }
    /*
        Convert to absolute paths to compare.
     */
    if (!isFullPath(fs1, path1)) {
        path1 = mprGetAbsPath(path1);
    }
    if (!isFullPath(fs2, path2)) {
        path2 = mprGetAbsPath(path2);
    }
    if (fs1->caseSensitive) {
        for (p1 = path1, p2 = path2; *p1 && *p2 && len > 0; p1++, p2++, len--) {
            if (*p1 != *p2 && !(isSep(fs1, *p1) && isSep(fs2, *p2))) {
                break;
            }
        }
    } else {
        for (p1 = path1, p2 = path2; *p1 && *p2 && len > 0; p1++, p2++, len--) {
            if (tolower((uchar) *p1) != tolower((uchar) *p2) && !(isSep(fs1, *p1) && isSep(fs2, *p2))) {
                break;
            }
        }
    }
    return len == 0;
}


PUBLIC void mprSetAppPath(cchar *path)
{
    MPR->appPath = sclone(path);
    MPR->appDir = mprGetPathDir(MPR->appPath);
}


static char *checkPath(cchar *path, int flags)
{
    MprPath     info;
    int         access;

    access = (flags & (MPR_SEARCH_EXE | MPR_SEARCH_DIR)) ? X_OK : R_OK;

    if (mprPathExists(path, access)) {
        mprGetPathInfo(path, &info);
        if (flags & MPR_SEARCH_DIR && info.isDir) {
            return sclone(path);
        }
        if (info.isReg) {
            return sclone(path);
        }
    }
    return 0;
}


PUBLIC char *mprSearchPath(cchar *file, int flags, cchar *search, ...)
{
    va_list     args;
    char        *result, *path, *dir, *nextDir, *tok;

    va_start(args, search);

    if ((result = checkPath(file, flags)) != 0) {
        return result;
    }

    for (nextDir = (char*) search; nextDir; nextDir = va_arg(args, char*)) {
        tok = NULL;
        nextDir = sclone(nextDir);
        dir = stok(nextDir, MPR_SEARCH_SEP, &tok);
        while (dir && *dir) {
            path = mprJoinPath(dir, file);
            if ((result = checkPath(path, flags)) != 0) {
                va_end(args);
                return mprNormalizePath(result);
            }

            dir = stok(0, MPR_SEARCH_SEP, &tok);
        }
    }
    va_end(args);
    return 0;
}


/*
    This normalizes a path. Returns a normalized path according to flags. Default is absolute.
    if MPR_PATH_NATIVE_SEP is specified in the flags, map separators to the native format.
 */
PUBLIC char *mprTransformPath(cchar *path, int flags)
{
    char    *result;

    if (flags & MPR_PATH_ABS) {
        result = mprGetAbsPath(path);

    } else if (flags & MPR_PATH_REL) {
        result = mprGetRelPath(path, 0);

    } else {
        result = mprNormalizePath(path);
    }

    if (flags & MPR_PATH_NATIVE_SEP) {
    }
    return result;
}


PUBLIC char *mprTrimPathComponents(cchar *path, int count)
{
    MprFileSystem   *fs;
    cchar           *cp;
    int             sep;

    fs = mprLookupFileSystem(path);

    if (count == 0) {
        return sclone(path);

    } else if (count > 0) {
        do {
            if ((path = firstSep(fs, path)) == 0) {
                return sclone("");
            }
            path++;
        } while (--count > 0);
        return sclone(path);

    } else {
        sep = (cp = firstSep(fs, path)) ? *cp : defaultSep(fs);
        for (cp = &path[slen(path) - 1]; cp >= path && count < 0; cp--) {
            if (*cp == sep) {
                count++;
            }
        }
        if (count == 0) {
            return snclone(path, cp - path + 1);
        }
    }
    return sclone("");
}


PUBLIC char *mprTrimPathExt(cchar *path)
{
    MprFileSystem   *fs;
    char            *cp, *result;

    fs = mprLookupFileSystem(path);
    result = sclone(path);
    if ((cp = srchr(result, '.')) != NULL) {
        if (firstSep(fs, cp) == 0) {
            *cp = '\0';
        }
    }
    return result;
}


PUBLIC char *mprTrimPathDrive(cchar *path)
{
    MprFileSystem   *fs;
    char            *cp, *endDrive;

    fs = mprLookupFileSystem(path);
    if (fs->hasDriveSpecs) {
        cp = firstSep(fs, path);
        endDrive = strchr(path, ':');
        if (endDrive && (cp == NULL || endDrive < cp)) {
            return sclone(++endDrive);
        }
    }
    return sclone(path);
}


PUBLIC ssize mprWritePathContents(cchar *path, cchar *buf, ssize len, int mode)
{
    MprFile     *file;

    if (mode == 0) {
        mode = 0644;
    }
    if (len < 0) {
        len = slen(buf);
    }
    if ((file = mprOpenFile(path, O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, mode)) == 0) {
        return MPR_ERR_CANT_OPEN;
    }
    if (mprWriteFile(file, buf, len) != len) {
        mprCloseFile(file);
        return MPR_ERR_CANT_WRITE;
    }
    mprCloseFile(file);
    return len;
}





