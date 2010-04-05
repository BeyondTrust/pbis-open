/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

#include "includes.h"

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

CENTERROR
GPARemoveFiles(
    PSTR pszPath,
    BOOLEAN fDirectory,
    BOOLEAN fRecursive
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR  szCommand[2 * PATH_MAX + 1];

    sprintf(szCommand, "/bin/rm -f %s %s %s",
            fDirectory ? "-d" : "",
            fRecursive ? "-r" : "",
            pszPath);

    if (system(szCommand) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return(ceError);
}

CENTERROR
GPACheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1) {
        if (stat(pszPath, &statbuf) < 0) {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == ENOENT || errno == ENOTDIR)
            {
                *pbFileExists = FALSE;
                break;
            }
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            *pbFileExists = (((statbuf.st_mode & S_IFMT) == S_IFREG) ? TRUE : FALSE);
            break;
        }
    }

error:

    return ceError;
}

CENTERROR
GPACopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCSTR pszTmpSuffix = ".tmp_likewise";
    PSTR pszTmpPath = NULL;
    BOOLEAN bRemoveFile = FALSE;
    CHAR szBuf[1024+1];
    int  iFd = -1;
    int  oFd = -1;
    DWORD dwBytesRead = 0;

    if (IsNullOrEmptyString(pszSrcPath) ||
        IsNullOrEmptyString(pszDstPath)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwAllocateMemory(strlen(pszDstPath)+strlen(pszTmpSuffix)+2,
                               (PVOID*)&pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    strcpy(pszTmpPath, pszDstPath);
    strcat(pszTmpPath, pszTmpSuffix);

    if ((iFd = open(pszSrcPath, O_RDONLY, S_IRUSR)) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if ((oFd = open(pszTmpPath, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR)) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    bRemoveFile = TRUE;

    while (1) {

        if ((dwBytesRead = read(iFd, szBuf, 1024)) < 0) {

            if (errno == EINTR)
                continue;

            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if (dwBytesRead == 0)
            break;

        if (write(oFd, szBuf, dwBytesRead) != dwBytesRead) {

            if (errno == EINTR)
                continue;

            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

    }

    close(iFd); iFd = -1;
    close(oFd); oFd = -1;

    ceError = LwMoveFile(pszTmpPath, pszDstPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = FALSE;

    ceError = GPAChangePermissions(pszDstPath, dwPerms);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if (iFd >= 0)
        close(iFd);

    if (oFd >= 0)
        close(oFd);

    if (bRemoveFile) {
        LwRemoveFile(pszTmpPath);
    }

    if (pszTmpPath)
        LwFreeString(pszTmpPath);

    return ceError;
}

CENTERROR
GPAGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszSrcPath, &statbuf) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *uid = statbuf.st_uid;
    *gid = statbuf.st_gid;
    *mode = statbuf.st_mode;

error:

    return ceError;
}

CENTERROR
GPACopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    uid_t uid;
    gid_t gid;
    mode_t mode;

    ceError = LwGetOwnerAndPermissions(pszSrcPath, &uid, &gid, &mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACopyFileWithPerms(pszSrcPath, pszDstPath, mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwChangeOwnerAndPermissions(pszDstPath, uid, gid, mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

CENTERROR
GPAFilePrintf(
    FILE* handle,
    PCSTR format,
    ...
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    va_list args;
    int count;

    va_start(args, format);
    count = vfprintf(handle, format, args);
    va_end(args);

    if (count < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    return ceError;
}

CENTERROR
GPAGetCurrentDirectoryPath(
    PSTR* ppszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    PSTR pszPath = NULL;

    if (getcwd(szBuf, PATH_MAX) == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwAllocateString(szBuf, &pszPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszPath = pszPath;

    return ceError;

error:

    if (pszPath) {
        LwFreeString(pszPath);
    }

    return ceError;
}

CENTERROR
GPARemoveDirectory(
    PCSTR pszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[PATH_MAX+1];

    if ((pDir = opendir(pszPath)) == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0) {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
            ceError = GPARemoveDirectory(szBuf);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (rmdir(szBuf) < 0) {
                ceError = LwMapErrnoToLwError(ceError);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

        } else {

            ceError = LwRemoveFile(szBuf);
            BAIL_ON_CENTERIS_ERROR(ceError);

        }
    }
    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        ceError = LwMapErrnoToLwError(ceError);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    pDir = NULL;

    if (rmdir(pszPath) < 0) {
        ceError = LwMapErrnoToLwError(ceError);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (pDir)
        closedir(pDir);

    return ceError;
}


CENTERROR
GPACheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    struct stat statbuf;

    while (1) {

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(pszPath, &statbuf) < 0) {

            if (errno == EINTR) {
                continue;
            }
            else if (errno == ENOENT || errno == ENOTDIR) {
                *pbDirExists = FALSE;
                break;
            }
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);

        }

        /*
           The path exists. Is it a directory?
         */

        *pbDirExists = (((statbuf.st_mode & S_IFMT) == S_IFDIR) ? TRUE : FALSE);
        break;
    }

error:

    return ceError;
}
/*
// On some unix systems, you may not be allowed to
// move files across devices. So, we copy the file to a
// tmp file and then move it to the target location -
// and remove the original file
*/
CENTERROR
GPAMoveFileAcrossDevices(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szTmpPath[PATH_MAX+1] = "";
    BOOLEAN bRemoveFile = FALSE;
    uid_t uid;
    gid_t gid;
    mode_t mode;

    sprintf(szTmpPath, "%s_lwi_.tmp", pszDstPath);

    ceError = LwGetOwnerAndPermissions(pszSrcPath, &uid, &gid, &mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACopyFileWithPerms(pszSrcPath, szTmpPath, mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = TRUE;

    ceError = LwMoveFile(szTmpPath, pszDstPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = FALSE;

    ceError = LwRemoveFile(pszSrcPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwChangeOwnerAndPermissions(pszDstPath, uid, gid, mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if (bRemoveFile && !IsNullOrEmptyString(szTmpPath))
       LwRemoveFile(szTmpPath);

    return ceError;
}

CENTERROR
GPAOpenFile(
    PCSTR path,
    PCSTR mode,
    FILE** handle)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    *handle = fopen(path, mode);

    if (!*handle)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
error:

    return ceError;
}

CENTERROR
GPACloseFile(
    FILE* handle
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (fclose(handle))
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;
}


CENTERROR
GPAGetOwnerUID(
    PCSTR pszFilePath,
    uid_t* pUid
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszFilePath, &statbuf) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *pUid = statbuf.st_uid;
    
error:

    return ceError;
}
