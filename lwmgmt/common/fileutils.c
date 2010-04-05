/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog file utilities
 *
 */
#include "includes.h"


DWORD
LWMGMTRemoveFile(
    PSTR pszPath
    )
{
    DWORD dwError = 0;

    while (1) {
        if (unlink(pszPath) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = errno;
            BAIL_ON_LWMGMT_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
LWMGMTCheckFileExists(
    PSTR pszPath,
    PBOOLEAN pbFileExists
    )
{
    DWORD dwError = 0;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1) {
        if (stat(pszPath, &statbuf) < 0) {
            if (errno == EINTR) {
                continue;
            } else if (errno == ENOENT) {
             *pbFileExists = 0;
             break;
            }
            dwError = errno;
            BAIL_ON_LWMGMT_ERROR(dwError);
        } else {
            *pbFileExists = 1;
            break;
        }
    }

error:

    return dwError;
}

DWORD
LWMGMTMoveFile(
    PSTR pszSrcPath,
    PSTR pszDstPath
    )
{
    DWORD dwError = 0;

    if (rename(pszSrcPath, pszDstPath) < 0) {
        dwError = errno;
    }

    return dwError;
}

DWORD
LWMGMTChangePermissions(
    PSTR pszPath,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;

    while (1) {
        if (chmod(pszPath, dwFileMode) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = errno;
            BAIL_ON_LWMGMT_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
LWMGMTChangeOwner(
    PSTR pszPath,
    uid_t uid,
    gid_t gid
    )
{
    DWORD dwError = 0;

    while (1) {
        if (chown(pszPath, uid, gid) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = errno;
            BAIL_ON_LWMGMT_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
LWMGMTChangeOwnerAndPermissions(
    PSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;

    dwError = LWMGMTChangeOwner(pszPath, uid, gid);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LWMGMTChangePermissions(pszPath, dwFileMode);
    BAIL_ON_LWMGMT_ERROR(dwError);

error:

    return dwError;
}

DWORD
LWMGMTChangeDirectory(
    PSTR pszPath
    )
{
    if (pszPath == NULL || *pszPath == '\0')
        return EINVAL;

    if (chdir(pszPath) < 0)
        return errno;

    return 0;
}

/*
// TODO: Check access and removability before actual deletion
*/
DWORD
LWMGMTRemoveDirectory(
    PSTR pszPath
    )
{
    DWORD dwError = 0;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[PATH_MAX+1];

    if ((pDir = opendir(pszPath)) == NULL) {
        dwError = errno;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0) {
            dwError = errno;
            BAIL_ON_LWMGMT_ERROR(dwError);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
            dwError = LWMGMTRemoveDirectory(szBuf);
            BAIL_ON_LWMGMT_ERROR(dwError);

            if (rmdir(szBuf) < 0) {
                dwError = errno;
                BAIL_ON_LWMGMT_ERROR(dwError);
            }

        } else {

            dwError = LWMGMTRemoveFile(szBuf);
            BAIL_ON_LWMGMT_ERROR(dwError);

        }
    }

error:

    if (pDir)
        closedir(pDir);

    return dwError;
}

DWORD
LWMGMTCheckDirectoryExists(
    PSTR pszPath,
    PBOOLEAN pbDirExists
    )
{
    DWORD dwError = 0;

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
            dwError = errno;
            BAIL_ON_LWMGMT_ERROR(dwError);

        }

        /*
            The path exists. Is it a directory?
         */

        *pbDirExists = (((statbuf.st_mode & S_IFMT) == S_IFDIR) ? TRUE : FALSE);
        break;
    }

error:

    return dwError;
}

DWORD
LWMGMTGetCurrentDirectoryPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    CHAR szBuf[PATH_MAX+1];
    PSTR pszPath = NULL;

    if (getcwd(szBuf, PATH_MAX) == NULL) {
        dwError = errno;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    dwError = LWMGMTAllocateString(szBuf, &pszPath);
    BAIL_ON_LWMGMT_ERROR(dwError);

    *ppszPath = pszPath;

    return dwError;

error:

    if (pszPath) {
        LWMGMTFreeString(pszPath);
    }

    return dwError;
}

static
DWORD
LWMGMTCreateDirectoryRecursive(
    PSTR pszCurDirPath,
    PSTR pszTmpPath,
    PSTR *ppszTmp,
    DWORD dwFileMode,
    DWORD dwWorkingFileMode,
    int  iPart
    )
{
    DWORD dwError = 0;
    PSTR pszDirPath = NULL;
    BOOLEAN bDirCreated = FALSE;
    BOOLEAN bDirExists = FALSE;
    CHAR szDelimiters[] = "/";

    PSTR pszToken = strtok_r((iPart ? NULL : pszTmpPath), szDelimiters, ppszTmp);

    if (pszToken != NULL) {

        dwError = LWMGMTAllocateMemory(strlen(pszCurDirPath)+strlen(pszToken)+2,
                                    (PVOID*)&pszDirPath);
        BAIL_ON_LWMGMT_ERROR(dwError);

        sprintf(pszDirPath,
                "%s/%s",
                (!strcmp(pszCurDirPath, "/") ? "" : pszCurDirPath),
                pszToken);

        dwError = LWMGMTCheckDirectoryExists(pszDirPath, &bDirExists);
        BAIL_ON_LWMGMT_ERROR(dwError);

        if (!bDirExists) {
            if (mkdir(pszDirPath, dwWorkingFileMode) < 0) {
                dwError = errno;
                BAIL_ON_LWMGMT_ERROR(dwError);
            }
            bDirCreated = TRUE;
        }

        dwError = LWMGMTChangeDirectory(pszDirPath);
        BAIL_ON_LWMGMT_ERROR(dwError);

        dwError = LWMGMTCreateDirectoryRecursive(
            pszDirPath,
            pszTmpPath,
            ppszTmp,
            dwFileMode,
            dwWorkingFileMode,
            iPart+1
            );
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    if (bDirCreated && (dwFileMode != dwWorkingFileMode)) {
        dwError = LWMGMTChangePermissions(pszDirPath, dwFileMode);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    if (pszDirPath) {
        LWMGMTFreeMemory(pszDirPath);
    }

    return dwError;

error:

    if (bDirCreated) {
        LWMGMTRemoveDirectory(pszDirPath);
    }

    if (pszDirPath) {
        LWMGMTFreeMemory(pszDirPath);
    }

    return dwError;
}

DWORD
LWMGMTCreateDirectory(
    PSTR pszPath,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;
    PSTR pszCurDirPath = NULL;
    PSTR pszTmpPath = NULL;
    PSTR pszTmp = NULL;
    mode_t dwWorkingFileMode;

    if (pszPath == NULL || *pszPath == '\0') {
        dwError = EINVAL;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    dwWorkingFileMode = dwFileMode;
    if (!(dwFileMode & S_IXUSR)) {
        /*
         * This is so that we can navigate the folders
         * when we are creating the subfolders
         */
        dwWorkingFileMode |= S_IXUSR;
    }

    dwError = LWMGMTGetCurrentDirectoryPath(&pszCurDirPath);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LWMGMTAllocateString(pszPath, &pszTmpPath);
    BAIL_ON_LWMGMT_ERROR(dwError);

    if (*pszPath == '/') {
        dwError = LWMGMTChangeDirectory("/");
        BAIL_ON_LWMGMT_ERROR(dwError);

        dwError = LWMGMTCreateDirectoryRecursive("/", pszTmpPath, &pszTmp, dwFileMode, dwWorkingFileMode, 0);
        BAIL_ON_LWMGMT_ERROR(dwError);

    } else {

        dwError = LWMGMTCreateDirectoryRecursive(pszCurDirPath, pszTmpPath, &pszTmp, dwFileMode, dwWorkingFileMode, 0);
        BAIL_ON_LWMGMT_ERROR(dwError);

    }

error:

    if (pszCurDirPath) {

        LWMGMTChangeDirectory(pszCurDirPath);

        LWMGMTFreeMemory(pszCurDirPath);

    }

    if (pszTmpPath) {
        LWMGMTFreeMemory(pszTmpPath);
    }

    return dwError;
}

