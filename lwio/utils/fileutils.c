/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        fileutils.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "includes.h"

DWORD
SMBRemoveFile(
    PCSTR pszPath
    )
{
    DWORD dwError = 0;

    while (1) {
        if (unlink(pszPath) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = errno;
            BAIL_ON_LWIO_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
SMBCheckFileExists(
    PCSTR pszPath,
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
           BAIL_ON_LWIO_ERROR(dwError);
        } else {
          *pbFileExists = 1;
          break;
        }
    }

error:

    return dwError;
}

DWORD
SMBCheckSockExists(
    PSTR pszPath,
    PBOOLEAN pbSockExists
    )
{
    DWORD dwError = 0;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1) {
        if (stat(pszPath, &statbuf) < 0) {
           if (errno == EINTR) {
              continue;
           } else if (errno == ENOENT || errno == ENOTDIR) {
             *pbSockExists = 0;
             break;
           }
           dwError = errno;
           BAIL_ON_LWIO_ERROR(dwError);
        } else {
          *pbSockExists = (((statbuf.st_mode & S_IFMT) == S_IFSOCK) ? TRUE : FALSE);
          break;
        }
    }

error:

    return dwError;
}

DWORD
SMBMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    )
{
    DWORD dwError = 0;

    if (rename(pszSrcPath, pszDstPath) < 0) {
        dwError = errno;
    }

    return dwError;
}

DWORD
SMBChangePermissions(
    PCSTR pszPath,
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
            BAIL_ON_LWIO_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
SMBChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    )
{
    DWORD dwError = 0;
    struct stat statbuf = {0};

    if (lstat(pszPath, &statbuf) < 0) {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    while (1) {

        if (S_ISLNK(statbuf.st_mode)) {

            if (lchown(pszPath, uid, gid) < 0) {
                if (errno == EINTR) {
                    continue;
                }
                dwError = errno;
                BAIL_ON_LWIO_ERROR(dwError);
            } else {
                break;
            }

        } else {

            if (chown(pszPath, uid, gid) < 0) {
                if (errno == EINTR) {
                    continue;
                }
                dwError = errno;
                BAIL_ON_LWIO_ERROR(dwError);
            } else {
                break;
            }
        }
    }

error:

    return dwError;
}

DWORD
SMBChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;

    dwError = SMBChangeOwner(pszPath, uid, gid);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = SMBChangePermissions(pszPath, dwFileMode);
    BAIL_ON_LWIO_ERROR(dwError);

error:

    return dwError;
}

DWORD
SMBChangeDirectory(
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
SMBRemoveDirectory(
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
        BAIL_ON_LWIO_ERROR(dwError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0) {
            dwError = errno;
            BAIL_ON_LWIO_ERROR(dwError);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
            dwError = SMBRemoveDirectory(szBuf);
            BAIL_ON_LWIO_ERROR(dwError);

            if (rmdir(szBuf) < 0) {
                dwError = errno;
                BAIL_ON_LWIO_ERROR(dwError);
            }

        } else {

            dwError = SMBRemoveFile(szBuf);
            BAIL_ON_LWIO_ERROR(dwError);

        }
    }

error:

    if (pDir)
        closedir(pDir);

    return dwError;
}

DWORD
SMBCheckDirectoryExists(
    PCSTR pszPath,
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
            BAIL_ON_LWIO_ERROR(dwError);

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
SMBGetCurrentDirectoryPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    CHAR szBuf[PATH_MAX+1];
    PSTR pszPath = NULL;

    if (getcwd(szBuf, PATH_MAX) == NULL) {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    dwError = SMBAllocateString(szBuf, &pszPath);
    BAIL_ON_LWIO_ERROR(dwError);

    *ppszPath = pszPath;

    return dwError;

error:

    if (pszPath) {
        SMBFreeString(pszPath);
    }

    return dwError;
}

static
DWORD
SMBCreateDirectoryRecursive(
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

        dwError = LwIoAllocateMemory(strlen(pszCurDirPath)+strlen(pszToken)+2,
                                   (PVOID*)&pszDirPath);
        BAIL_ON_LWIO_ERROR(dwError);

        sprintf(pszDirPath,
                "%s/%s",
                (!strcmp(pszCurDirPath, "/") ? "" : pszCurDirPath),
                pszToken);


        dwError = SMBCheckDirectoryExists(pszDirPath, &bDirExists);
        BAIL_ON_LWIO_ERROR(dwError);

        if (!bDirExists) {
            if (mkdir(pszDirPath, dwWorkingFileMode) < 0) {
                dwError = errno;
                BAIL_ON_LWIO_ERROR(dwError);
            }
            bDirCreated = TRUE;
        }

        dwError = SMBChangeDirectory(pszDirPath);
        BAIL_ON_LWIO_ERROR(dwError);

        dwError = SMBCreateDirectoryRecursive(
            pszDirPath,
            pszTmpPath,
            ppszTmp,
            dwFileMode,
            dwWorkingFileMode,
            iPart+1
            );
        BAIL_ON_LWIO_ERROR(dwError);
    }

    if (bDirCreated && (dwFileMode != dwWorkingFileMode)) {
        dwError = SMBChangePermissions(pszDirPath, dwFileMode);
        BAIL_ON_LWIO_ERROR(dwError);
    }
    if (pszDirPath) {
        LwIoFreeMemory(pszDirPath);
    }

    return dwError;

error:

    if (bDirCreated) {
        SMBRemoveDirectory(pszDirPath);
    }

    if (pszDirPath) {
        LwIoFreeMemory(pszDirPath);
    }

    return dwError;
}

DWORD
SMBCreateDirectory(
    PCSTR pszPath,
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
        BAIL_ON_LWIO_ERROR(dwError);
    }

    dwWorkingFileMode = dwFileMode;
    if (!(dwFileMode & S_IXUSR)) {
        /*
         * This is so that we can navigate the folders
         * when we are creating the subfolders
         */
        dwWorkingFileMode |= S_IXUSR;
    }

    dwError = SMBGetCurrentDirectoryPath(&pszCurDirPath);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = SMBAllocateString(pszPath, &pszTmpPath);
    BAIL_ON_LWIO_ERROR(dwError);

    if (*pszPath == '/') {
        dwError = SMBChangeDirectory("/");
        BAIL_ON_LWIO_ERROR(dwError);

        dwError = SMBCreateDirectoryRecursive("/", pszTmpPath, &pszTmp, dwFileMode, dwWorkingFileMode, 0);
        BAIL_ON_LWIO_ERROR(dwError);

    } else {

        dwError = SMBCreateDirectoryRecursive(pszCurDirPath, pszTmpPath, &pszTmp, dwFileMode, dwWorkingFileMode, 0);
        BAIL_ON_LWIO_ERROR(dwError);

    }

error:

    if (pszCurDirPath) {

        SMBChangeDirectory(pszCurDirPath);

        LwIoFreeMemory(pszCurDirPath);

    }

    if (pszTmpPath) {
        LwIoFreeMemory(pszTmpPath);
    }

    return dwError;
}

DWORD
SMBGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    )
{
    DWORD dwError = 0;
    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszSrcPath, &statbuf) < 0) {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    *uid = statbuf.st_uid;
    *gid = statbuf.st_gid;
    *mode = statbuf.st_mode;

error:

    return dwError;
}

DWORD
SMBCreateSymlink(
   PCSTR pszOldPath,
   PCSTR pszNewPath
   )
{
    return ((symlink(pszOldPath, pszNewPath) < 0) ? errno : 0);
}

