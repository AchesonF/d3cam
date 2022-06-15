#include "libmpr.h"

/**
    disk.c - File services for systems with a (disk) based file system.

    This module is not thread safe.

 */

/********************************** Includes **********************************/



/*********************************** Defines **********************************/

#define MASK_PERMS(perms)   perms

/********************************** Forwards **********************************/

static void manageDiskFile(MprFile *file, int flags);
static int disk_getPathInfo(MprDiskFileSystem *fs, cchar *path, MprPath *info);

/************************************ Code ************************************/

static void manageDiskFile(MprFile *file, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(file->path);
        mprMark(file->fileSystem);
        mprMark(file->buf);

    } else if (flags & MPR_MANAGE_FREE) {
        if (file->fd >= 0) {
            close(file->fd);
            file->fd = -1;
        }
    }
}


static bool disk_accessPath(MprDiskFileSystem *fs, cchar *path, int omode)
{
    return access(path, omode) == 0;
}


/*
    WARNING: this may be called by finalizers, so no blocking or locking
 */
static int disk_closeFile(MprFile *file)
{
    MprBuf  *bp;

    assert(file);

    if (file == 0) {
        return 0;
    }
    bp = file->buf;
    if (bp && (file->mode & (O_WRONLY | O_RDWR))) {
        mprFlushFile(file);
    }
    if (file->fd >= 0) {
        close(file->fd);
        file->fd = -1;
    }
    return 0;
}


static int disk_deletePath(MprDiskFileSystem *fs, cchar *path)
{
    MprPath     info;

    if (disk_getPathInfo(fs, path, &info) == 0 && info.isDir && !info.isLink) {
        return rmdir((char*) path);
    }

    return unlink((char*) path);
}


static MprFile *disk_openFile(MprFileSystem *fs, cchar *path, int omode, int perms)
{
    MprFile     *file;

    assert(path);

    if ((file = mprAllocObj(MprFile, manageDiskFile)) == 0) {
        return NULL;
    }
    file->path = sclone(path);
    file->fd = open(path, omode, MASK_PERMS(perms));
    if (file->fd < 0) {
        file = NULL;
    }
    return file;
}


static void manageDirEntry(MprDirEntry *dp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(dp->name);
    }
}


/*
    This returns a list of MprDirEntry objects
 */
static MprList *disk_listDir(MprDiskFileSystem *fs, cchar *path)
{
    DIR             *dir;
    MprPath         fileInfo;
    MprList         *list;
    struct dirent   *dirent;
    MprDirEntry     *dp;
    char            *fileName;
    int             rc;

    list = mprCreateList(256, 0);
    if ((dir = opendir((char*) path)) == 0) {
        return list;
    }
    while ((dirent = readdir(dir)) != 0) {
        if (dirent->d_name[0] == '.' && (dirent->d_name[1] == '\0' || dirent->d_name[1] == '.')) {
            continue;
        }
        fileName = mprJoinPath(path, dirent->d_name);
        /* workaround for if target of symlink does not exist */
        fileInfo.isLink = 0;
        fileInfo.isDir = 0;
        rc = mprGetPathInfo(fileName, &fileInfo);
        if ((dp = mprAllocObj(MprDirEntry, manageDirEntry)) == 0) {
            closedir(dir);
            return list;
        }
        dp->name = sclone(dirent->d_name);
        if (dp->name == 0) {
            closedir(dir);
            return list;
        }
        if (rc == 0 || fileInfo.isLink) {
            dp->lastModified = fileInfo.mtime;
            dp->size = fileInfo.size;
            dp->isDir = fileInfo.isDir;
            dp->isLink = fileInfo.isLink;
        } else {
            dp->lastModified = 0;
            dp->size = 0;
            dp->isDir = 0;
            dp->isLink = 0;
        }
        mprAddItem(list, dp);
    }
    closedir(dir);
    return list;
}

static int disk_makeDir(MprDiskFileSystem *fs, cchar *path, int perms, int owner, int group)
{
    int     rc;

    rc = mkdir(path, perms);
    if (rc < 0) {
        return MPR_ERR_CANT_CREATE;
    }

    if ((owner != -1 || group != -1) && chown(path, owner, group) < 0) {
        rmdir(path);
        return MPR_ERR_CANT_COMPLETE;
    }

    return 0;
}


static int disk_makeLink(MprDiskFileSystem *fs, cchar *path, cchar *target, int hard)
{
    if (hard) {
        return link(path, target);
    } else {
        return symlink(path, target);
    }
}


static int disk_getPathInfo(MprDiskFileSystem *fs, cchar *path, MprPath *info)
{
    struct stat s;
    info->valid = 0;
    info->isReg = 0;
    info->isDir = 0;
    info->size = 0;
    info->checked = 1;
    info->atime = 0;
    info->ctime = 0;
    info->mtime = 0;
    if (lstat((char*) path, &s) < 0) {
        return MPR_ERR_CANT_ACCESS;
    }
    #ifdef S_ISLNK
        info->isLink = S_ISLNK(s.st_mode);
        if (info->isLink) {
            if (stat((char*) path, &s) < 0) {
                return MPR_ERR_CANT_ACCESS;
            }
        }
    #endif
    info->valid = 1;
    info->size = s.st_size;
    info->atime = s.st_atime;
    info->ctime = s.st_ctime;
    info->mtime = s.st_mtime;
    info->inode = s.st_ino;
    info->isDir = S_ISDIR(s.st_mode);
    info->isReg = S_ISREG(s.st_mode);
    info->perms = s.st_mode & 07777;
    info->owner = s.st_uid;
    info->group = s.st_gid;
    if (strcmp(path, "/dev/null") == 0) {
        info->isReg = 0;
    }

    return 0;
}


static char *disk_getPathLink(MprDiskFileSystem *fs, cchar *path)
{
    char    pbuf[ME_MAX_PATH];
    ssize   len;

    if ((len = readlink(path, pbuf, sizeof(pbuf) - 1)) < 0) {
        return NULL;
    }
    pbuf[len] = '\0';
    return sclone(pbuf);
}


/*
    These functions are supported regardles
 */
static ssize disk_readFile(MprFile *file, void *buf, ssize size)
{
    assert(file);
    assert(buf);

    return read(file->fd, buf, (uint) size);
}


static MprOff disk_seekFile(MprFile *file, int seekType, MprOff distance)
{
    assert(file);

    if (file == 0) {
        return MPR_ERR_BAD_HANDLE;
    }
#if ME_COMPILER_HAS_OFF64
    return (MprOff) lseek64(file->fd, (off64_t) distance, seekType);
#else
    return (MprOff) lseek(file->fd, (off_t) distance, seekType);
#endif
}


static int disk_truncateFile(MprDiskFileSystem *fs, cchar *path, MprOff size)
{
    if (!mprPathExists(path, F_OK)) {
        return MPR_ERR_CANT_ACCESS;
    }

    if (truncate(path, size) < 0) {
        return MPR_ERR_CANT_WRITE;
    }

    return 0;
}


static void manageDiskFileSystem(MprDiskFileSystem *dfs, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(dfs->separators);
        mprMark(dfs->newline);
    }
}


static ssize disk_writeFile(MprFile *file, cvoid *buf, ssize count)
{
    assert(file);
    assert(buf);

    return write(file->fd, buf, (uint) count);
}


PUBLIC MprDiskFileSystem *mprCreateDiskFileSystem(cchar *path)
{
    MprFileSystem       *fs;
    MprDiskFileSystem   *dfs;

    if ((dfs = mprAllocObj(MprDiskFileSystem, manageDiskFileSystem)) == 0) {
        return 0;
    }
    fs = (MprFileSystem*) dfs;
    mprInitFileSystem(fs, path);

    dfs->accessPath = disk_accessPath;
    dfs->deletePath = disk_deletePath;
    dfs->getPathInfo = disk_getPathInfo;
    dfs->getPathLink = disk_getPathLink;
    dfs->listDir = disk_listDir;
    dfs->makeDir = disk_makeDir;
    dfs->makeLink = disk_makeLink;
    dfs->openFile = disk_openFile;
    dfs->closeFile = disk_closeFile;
    dfs->readFile = disk_readFile;
    dfs->seekFile = disk_seekFile;
    dfs->truncateFile = disk_truncateFile;
    dfs->writeFile = disk_writeFile;
    return dfs;
}






