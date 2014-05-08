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
 *        Registry File Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "includes.h"

static
DWORD
RegCreateDirectoryRecursive(
    PSTR pszCurDirPath,
    PSTR pszTmpPath,
    PSTR *ppszTmp,
    DWORD dwFileMode,
    DWORD dwWorkingFileMode,
    int  iPart
    );

DWORD
RegRemoveFile(
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
            BAIL_ON_REG_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
RegCheckFileExists(
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
           BAIL_ON_REG_ERROR(dwError);
        } else {
          *pbFileExists = 1;
          break;
        }
    }

error:

    return dwError;
}

DWORD
RegCheckSockExists(
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
           BAIL_ON_REG_ERROR(dwError);
        } else {
          *pbSockExists = (((statbuf.st_mode & S_IFMT) == S_IFSOCK) ? TRUE : FALSE);
          break;
        }
    }

error:

    return dwError;
}

DWORD
RegMoveFile(
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
RegChangePermissions(
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
            BAIL_ON_REG_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
RegChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    )
{
    DWORD dwError = 0;
    struct stat statbuf = {0};

    if (lstat(pszPath, &statbuf) < 0) {
        dwError = errno;
        BAIL_ON_REG_ERROR(dwError);
    }

    while (1) {

        if (S_ISLNK(statbuf.st_mode)) {

            if (lchown(pszPath, uid, gid) < 0) {
                if (errno == EINTR) {
                    continue;
                }
                dwError = errno;
                BAIL_ON_REG_ERROR(dwError);
            } else {
                break;
            }

        } else {

            if (chown(pszPath, uid, gid) < 0) {
                if (errno == EINTR) {
                    continue;
                }
                dwError = errno;
                BAIL_ON_REG_ERROR(dwError);
            } else {
                break;
            }
        }
    }

error:

    return dwError;
}

DWORD
RegChangeDirectory(
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
RegRemoveDirectory(
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
        BAIL_ON_REG_ERROR(dwError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0) {
            dwError = errno;
            BAIL_ON_REG_ERROR(dwError);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
            dwError = RegRemoveDirectory(szBuf);
            BAIL_ON_REG_ERROR(dwError);

            if (rmdir(szBuf) < 0) {
                dwError = errno;
                BAIL_ON_REG_ERROR(dwError);
            }

        } else {

            dwError = RegRemoveFile(szBuf);
            BAIL_ON_REG_ERROR(dwError);

        }
    }

error:

    if (pDir)
        closedir(pDir);

    return dwError;
}

DWORD
RegCheckDirectoryExists(
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
            BAIL_ON_REG_ERROR(dwError);

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
RegGetCurrentDirectoryPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    CHAR szBuf[PATH_MAX+1];
    PSTR pszPath = NULL;

    if (getcwd(szBuf, PATH_MAX) == NULL) {
        dwError = errno;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegCStringDuplicate(&pszPath, szBuf);
    BAIL_ON_REG_ERROR(dwError);

    *ppszPath = pszPath;

    return dwError;

error:

    LWREG_SAFE_FREE_STRING(pszPath);

    return dwError;
}

DWORD
RegCreateDirectory(
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
        BAIL_ON_REG_ERROR(dwError);
    }

    dwWorkingFileMode = dwFileMode;
    if (!(dwFileMode & S_IXUSR)) {
        /*
         * This is so that we can navigate the folders
         * when we are creating the subfolders
         */
        dwWorkingFileMode |= S_IXUSR;
    }

    dwError = RegGetCurrentDirectoryPath(&pszCurDirPath);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegCStringDuplicate(&pszTmpPath, pszPath);
    BAIL_ON_REG_ERROR(dwError);

    if (*pszPath == '/') {
        dwError = RegChangeDirectory("/");
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegCreateDirectoryRecursive("/", pszTmpPath, &pszTmp, dwFileMode, dwWorkingFileMode, 0);
        BAIL_ON_REG_ERROR(dwError);

    } else {

        dwError = RegCreateDirectoryRecursive(pszCurDirPath, pszTmpPath, &pszTmp, dwFileMode, dwWorkingFileMode, 0);
        BAIL_ON_REG_ERROR(dwError);

    }

error:

    if (pszCurDirPath) {

        RegChangeDirectory(pszCurDirPath);

        RegMemoryFree(pszCurDirPath);

    }

    if (pszTmpPath) {
        RegMemoryFree(pszTmpPath);
    }

    return dwError;
}

DWORD
RegGetDirectoryFromPath(
    IN PCSTR pszPath,
    OUT PSTR* ppszDir
    )
{
    PCSTR pszLastSlash = NULL;
    PSTR pszDir = NULL;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pszPath);

    pszLastSlash = strrchr(pszPath, '/');
    if (pszLastSlash == pszPath)
    {
        //Include the trailing / since this is the root directory ( / )
        pszLastSlash++;
    }

    if (pszLastSlash == NULL)
    {
        dwError = RegCStringDuplicate(
                        &pszDir, ".");
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = RegStrndup(
                        pszPath,
                        pszLastSlash - pszPath,
                        &pszDir);
        BAIL_ON_REG_ERROR(dwError);
    }

    *ppszDir = pszDir;

cleanup:

    return dwError;

error:

    LWREG_SAFE_FREE_STRING(pszDir);
    *ppszDir = NULL;
    goto cleanup;
}

DWORD
RegGetOwnerAndPermissions(
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
        BAIL_ON_REG_ERROR(dwError);
    }

    *uid = statbuf.st_uid;
    *gid = statbuf.st_gid;
    *mode = statbuf.st_mode;

error:

    return dwError;
}

DWORD
RegChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;

    dwError = RegChangeOwner(pszPath, uid, gid);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegChangePermissions(pszPath, dwFileMode);
    BAIL_ON_REG_ERROR(dwError);

error:

    return dwError;
}

DWORD
RegGetSymlinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   )
{
    DWORD dwError = 0;
    CHAR szBuf[PATH_MAX+1];
    PSTR pszTargetPath = NULL;

    memset(szBuf, 0, sizeof(szBuf));

    while (1) {

       if (readlink(pszPath, szBuf, PATH_MAX) < 0) {
          if (errno == EINTR)
             continue;

          dwError = errno;
          BAIL_ON_REG_ERROR(dwError);
       }

       break;
    }

    dwError = RegCStringDuplicate(
                    &pszTargetPath,
                    szBuf);
    BAIL_ON_REG_ERROR(dwError);

    *ppszTargetPath = pszTargetPath;

cleanup:

    return dwError;

error:

    *ppszTargetPath = NULL;

    LWREG_SAFE_FREE_STRING(pszTargetPath);

    goto cleanup;
}

DWORD
RegCreateSymlink(
   PCSTR pszOldPath,
   PCSTR pszNewPath
   )
{
    return ((symlink(pszOldPath, pszNewPath) < 0) ? errno : 0);
}

DWORD
RegGetMatchingFilePathsInFolder(
    PCSTR pszDirPath,
    PCSTR pszFileNameRegExp,
    PSTR** pppszHostFilePaths,
    PDWORD pdwNPaths
    )
{
    typedef struct __PATHNODE
    {
        PSTR pszPath;
        struct __PATHNODE *pNext;
    } PATHNODE, *PPATHNODE;

    DWORD dwError = 0;
    DIR* pDir = NULL;
    struct dirent*  pDirEntry = NULL;
    regex_t rx;
    BOOLEAN rxAllocated = FALSE;
    regmatch_t* pResult = NULL;
    size_t nMatch = 1;
    DWORD  dwNPaths = 0;
    DWORD  iPath = 0;
    PSTR*  ppszHostFilePaths = NULL;
    CHAR   szBuf[PATH_MAX+1];
    struct stat statbuf;
    PPATHNODE pPathList = NULL;
    PPATHNODE pPathNode = NULL;
    BOOLEAN bDirExists = FALSE;

    dwError = RegCheckDirectoryExists(pszDirPath, &bDirExists);
    BAIL_ON_REG_ERROR(dwError);

    if(!bDirExists) {
        dwError = ENOENT;
        BAIL_ON_REG_ERROR(dwError);
    }

    if (regcomp(&rx, pszFileNameRegExp, REG_EXTENDED) != 0) {
        dwError = LWREG_ERROR_REGEX_COMPILE_FAILED;
        BAIL_ON_REG_ERROR(dwError);
    }
    rxAllocated = TRUE;

    dwError = RegAllocateMemory(sizeof(*pResult), (PVOID*)&pResult);
    BAIL_ON_REG_ERROR(dwError);

    pDir = opendir(pszDirPath);
    if (!pDir) {
        dwError = errno;
        BAIL_ON_REG_ERROR(dwError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        int copied = snprintf(
                            szBuf,
                            sizeof(szBuf),
                            "%s/%s",
                            pszDirPath,
                            pDirEntry->d_name);
        if (copied >= sizeof(szBuf))
        {
            //Skip pathnames that are too long
            continue;
        }
        memset(&statbuf, 0, sizeof(struct stat));
        if (lstat(szBuf, &statbuf) < 0) {
            if(errno == ENOENT)
            {
                //This occurs when there is a symbolic link pointing to a
                //location that doesn't exist, because stat looks at the final
                //file, not the link. Since this file doesn't exist anyway,
                //just skip it.
                continue;
            }
            dwError = errno;
            BAIL_ON_REG_ERROR(dwError);
        }

        /*
         * For now, we are searching only for regular files
         * This may be enhanced in the future to support additional
         * file system entry types
         */
        if (((statbuf.st_mode & S_IFMT) == S_IFREG) &&
            (regexec(&rx, pDirEntry->d_name, nMatch, pResult, 0) == 0)) {
            dwNPaths++;

            dwError = RegAllocateMemory(sizeof(*pPathNode), (PVOID*)&pPathNode);
            BAIL_ON_REG_ERROR(dwError);

            dwError = RegCStringDuplicate(&pPathNode->pszPath, szBuf);
            BAIL_ON_REG_ERROR(dwError);

            pPathNode->pNext = pPathList;
            pPathList = pPathNode;
            pPathNode = NULL;
        }
    }

    if (pPathList) {

        dwError = RegAllocateMemory(sizeof(*ppszHostFilePaths)*dwNPaths, (PVOID*)&ppszHostFilePaths);
        BAIL_ON_REG_ERROR(dwError);
        /*
         *  The linked list is in reverse.
         *  Assign values in reverse to get it right
         */
        iPath = dwNPaths-1;
        pPathNode = pPathList;
        while (pPathNode) {
            *(ppszHostFilePaths+iPath) = pPathNode->pszPath;
            pPathNode->pszPath = NULL;
            pPathNode = pPathNode->pNext;
            iPath--;
        }
    }

    *pppszHostFilePaths = ppszHostFilePaths;
    *pdwNPaths = dwNPaths;

cleanup:

    if (pPathNode) {
    	LWREG_SAFE_FREE_STRING(pPathNode->pszPath);
        RegMemoryFree(pPathNode);
    }

    while(pPathList) {
        pPathNode = pPathList;
        pPathList = pPathList->pNext;
        LWREG_SAFE_FREE_STRING(pPathNode->pszPath);
        RegMemoryFree(pPathNode);
    }

    if (rxAllocated) {
        regfree(&rx);
    }

    LWREG_SAFE_FREE_MEMORY(pResult);

    if (pDir) {
        closedir(pDir);
    }

    return dwError;

error:

    if (ppszHostFilePaths) {
    	RegFreeStringArray(ppszHostFilePaths, dwNPaths);
    }

    goto cleanup;
}

DWORD
RegCheckLinkExists(
    PSTR pszPath,
    PBOOLEAN pbLinkExists
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1)
    {
        if (stat(pszPath, &statbuf) < 0)
        {
           if (errno == EINTR)
           {
              continue;
           }
           else if (errno == ENOENT || errno == ENOTDIR)
           {
             *pbLinkExists = 0;
             break;
           }

           dwError = errno;
           BAIL_ON_REG_ERROR(dwError);
        }
        else
        {
           if (((statbuf.st_mode & S_IFMT) == S_IFLNK))
           {
               bExists = TRUE;
           }
          break;
        }
    }

error:

    *pbLinkExists = bExists;

    return dwError;
}

DWORD
RegCheckFileOrLinkExists(
    PSTR pszPath,
    PBOOLEAN pbExists
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1)
    {
        if (stat(pszPath, &statbuf) < 0)
        {
           if (errno == EINTR)
           {
              continue;
           }
           else if (errno == ENOENT || errno == ENOTDIR)
           {
             break;
           }

           dwError = errno;
           BAIL_ON_REG_ERROR(dwError);
        }
        else
        {
          if (((statbuf.st_mode & S_IFMT) == S_IFLNK) ||
              ((statbuf.st_mode & S_IFMT) == S_IFREG))
          {
              bExists = TRUE;
          }
          break;
        }
    }

error:

    *pbExists = bExists;

    return dwError;
}

static
DWORD
RegCreateDirectoryRecursive(
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

        dwError = RegAllocateMemory(strlen(pszCurDirPath)+strlen(pszToken)+2, (PVOID*)&pszDirPath);
        BAIL_ON_REG_ERROR(dwError);

        sprintf(pszDirPath,
                "%s/%s",
                (!strcmp(pszCurDirPath, "/") ? "" : pszCurDirPath),
                pszToken);


        dwError = RegCheckDirectoryExists(pszDirPath, &bDirExists);
        BAIL_ON_REG_ERROR(dwError);

        if (!bDirExists) {
            if (mkdir(pszDirPath, dwWorkingFileMode) < 0) {
                dwError = errno;
                BAIL_ON_REG_ERROR(dwError);
            }
            bDirCreated = TRUE;
        }

        dwError = RegChangeDirectory(pszDirPath);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegCreateDirectoryRecursive(
            pszDirPath,
            pszTmpPath,
            ppszTmp,
            dwFileMode,
            dwWorkingFileMode,
            iPart+1
            );
        BAIL_ON_REG_ERROR(dwError);
    }

    if (bDirCreated && (dwFileMode != dwWorkingFileMode)) {
        dwError = RegChangePermissions(pszDirPath, dwFileMode);
        BAIL_ON_REG_ERROR(dwError);
    }
    if (pszDirPath) {
        RegMemoryFree(pszDirPath);
    }

    return dwError;

error:

    if (bDirCreated) {
        RegRemoveDirectory(pszDirPath);
    }

    if (pszDirPath) {
        RegMemoryFree(pszDirPath);
    }

    return dwError;
}
