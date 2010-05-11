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

#include "config.h"
#include "ctbase.h"
#include "ctarray.h"
#include <unistd.h>

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

CENTERROR
CTRemoveFile(
    PCSTR pszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    while (1) {
        if (unlink(pszPath) < 0) {
            if (errno == EINTR) {
                continue;
            }
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            break;
        }
    }

error:

    return ceError;
}

/*
// TODO: Check access and removability before actual deletion
*/
CENTERROR
CTRemoveDirectory(
    PCSTR pszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[PATH_MAX+1];

    if ((pDir = opendir(pszPath)) == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0) {
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
            ceError = CTRemoveDirectory(szBuf);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (rmdir(szBuf) < 0) {
                ceError = CTMapSystemError(ceError);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

        } else {

            ceError = CTRemoveFile(szBuf);
            BAIL_ON_CENTERIS_ERROR(ceError);

        }
    }
    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        ceError = CTMapSystemError(ceError);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    pDir = NULL;

    if (rmdir(pszPath) < 0) {
        ceError = CTMapSystemError(ceError);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (pDir)
        closedir(pDir);

    return ceError;
}

CENTERROR
CTCheckLinkExists(
    PCSTR pszPath,
    PBOOLEAN pbLinkExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1) {
        if (lstat(pszPath, &statbuf) < 0) {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == ENOENT || errno == ENOTDIR)
            {
                *pbLinkExists = FALSE;
                break;
            }
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            *pbLinkExists = (((statbuf.st_mode & S_IFMT) == S_IFLNK) ? TRUE : FALSE);
            break;
        }
    }

error:

    return ceError;
}

CENTERROR
CTCheckSockExists(
    PCSTR pszPath,
    PBOOLEAN pbSockExists
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
                *pbSockExists = FALSE;
                break;
            }
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            *pbSockExists = (((statbuf.st_mode & S_IFMT) == S_IFSOCK) ? TRUE : FALSE);
            break;
        }
    }

error:

    return ceError;
}

CENTERROR
CTCheckFileExists(
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
            ceError = CTMapSystemError(errno);
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
CTCheckDirectoryExists(
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
            ceError = CTMapSystemError(errno);
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

CENTERROR
CTMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (rename(pszSrcPath, pszDstPath) < 0) {
        ceError = CTMapSystemError(errno);
    }

    return ceError;
}

CENTERROR
CTCopyFileWithPerms(
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

    ceError = CTAllocateMemory(strlen(pszDstPath)+strlen(pszTmpSuffix)+2,
                               (PVOID*)&pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    strcpy(pszTmpPath, pszDstPath);
    strcat(pszTmpPath, pszTmpSuffix);

    if ((iFd = open(pszSrcPath, O_RDONLY, S_IRUSR)) < 0) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if ((oFd = open(pszTmpPath, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR)) < 0) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    bRemoveFile = TRUE;

    while (1) {

        if ((dwBytesRead = read(iFd, szBuf, 1024)) < 0) {

            if (errno == EINTR)
                continue;

            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if (dwBytesRead == 0)
            break;

        if (write(oFd, szBuf, dwBytesRead) != dwBytesRead) {

            if (errno == EINTR)
                continue;

            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

    }

    close(iFd); iFd = -1;
    close(oFd); oFd = -1;

    ceError = CTMoveFile(pszTmpPath, pszDstPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = FALSE;

    ceError = CTChangePermissions(pszDstPath, dwPerms);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if (iFd >= 0)
        close(iFd);

    if (oFd >= 0)
        close(oFd);

    if (bRemoveFile) {
        CTRemoveFile(pszTmpPath);
    }

    if (pszTmpPath)
        CTFreeString(pszTmpPath);

    return ceError;
}

CENTERROR
CTGetOwnerAndPermissions(
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
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *uid = statbuf.st_uid;
    *gid = statbuf.st_gid;
    *mode = statbuf.st_mode;

error:

    return ceError;
}

CENTERROR
CTGetOwnerUID(
    PCSTR pszFilePath,
    uid_t* pUid
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszFilePath, &statbuf) < 0) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *pUid = statbuf.st_uid;
    
error:

    return ceError;
}

CENTERROR
CTCheckFileOrLinkExists(
    PCSTR pszPath,
    PBOOLEAN pbExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BAIL_ON_CENTERIS_ERROR(ceError = CTCheckFileExists(pszPath, pbExists));
    if(*pbExists == TRUE)
        goto error;

    BAIL_ON_CENTERIS_ERROR(ceError = CTCheckLinkExists(pszPath, pbExists));

error:
    return ceError;
}

CENTERROR
CTGetFileTimeStamps(
    PCSTR pszFilePath,
    time_t *patime,   /* time of last access */
    time_t *pmtime,   /* time of last modification */
    time_t *pctime )  /* time of last status change */
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszFilePath, &statbuf) < 0) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if ( patime ) {
        *patime = statbuf.st_atime;
    }

    if ( pmtime ) {
        *pmtime = statbuf.st_mtime;
    }

    if ( pctime ) {
        *pctime = statbuf.st_ctime;
    }
error:

    return ceError;
}


CENTERROR
CTCopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    uid_t uid;
    gid_t gid;
    mode_t mode;

    ceError = CTGetOwnerAndPermissions(pszSrcPath, &uid, &gid, &mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTCopyFileWithPerms(pszSrcPath, pszDstPath, mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTChangeOwnerAndPermissions(pszDstPath, uid, gid, mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

CENTERROR
CTChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    while (1) {
        if (chmod(pszPath, dwFileMode) < 0) {
            if (errno == EINTR) {
                continue;
            }
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            break;
        }
    }

error:

    return ceError;
}

CENTERROR
CTChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    while (1) {
        if (chown(pszPath, uid, gid) < 0) {
            if (errno == EINTR) {
                continue;
            }
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            break;
        }
    }

error:

    return ceError;
}

CENTERROR
CTChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = CTChangeOwner(pszPath, uid, gid);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTChangePermissions(pszPath, dwFileMode);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

CENTERROR
CTGetCurrentDirectoryPath(
    PSTR* ppszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];
    PSTR pszPath = NULL;

    if (getcwd(szBuf, PATH_MAX) == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTAllocateString(szBuf, &pszPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszPath = pszPath;

    return ceError;

error:

    if (pszPath) {
        CTFreeString(pszPath);
    }

    return ceError;
}

CENTERROR
CTChangeDirectory(
    PSTR pszPath
    )
{
    if (pszPath == NULL || *pszPath == '\0')
        return CENTERROR_INVALID_PARAMETER;

    if (chdir(pszPath) < 0)
        return CTMapSystemError(errno);

    return CENTERROR_SUCCESS;
}

CENTERROR
CTCreateTempDirectory(
    PSTR *pszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR tmpDir;
    PSTR template = NULL;

    if (pszPath == NULL)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_INVALID_PARAMETER);
    }
    *pszPath = NULL;

    tmpDir = getenv("TMPDIR");
    if(tmpDir == NULL)
        tmpDir = "/tmp";

    ceError = CTAllocateStringPrintf(&template, "%s/likewisetmpXXXXXX", tmpDir);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (mkdtemp(template) == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *pszPath = template;
    template = NULL;

error:
    CT_SAFE_FREE_STRING(template);
    return ceError;
}

static CENTERROR
CTCreateDirectoryInternal(
    PSTR pszPath,
    PSTR pszLastSlash,
    mode_t dwFileMode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszSlash = NULL;
    BOOLEAN bDirExists = FALSE;
    BOOLEAN bDirCreated = FALSE;

    pszSlash = pszLastSlash ? strchr(pszLastSlash + 1, '/') : strchr(pszPath, '/');

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (pszPath[0])
    {
        ceError = CTCheckDirectoryExists(pszPath, &bDirExists);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if (!bDirExists)
        {
            if (mkdir(pszPath, S_IRWXU) != 0)
            {
                ceError = CTMapSystemError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
            bDirCreated = TRUE;
        }
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }

    if (pszSlash)
    {
        ceError = CTCreateDirectoryInternal(pszPath, pszSlash, dwFileMode);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (bDirCreated)
    {
        ceError = CTChangePermissions(pszPath, dwFileMode);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }
    
cleanup:

    return ceError;

error:

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (bDirCreated)
    {
        CTRemoveDirectory(pszPath);
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }

    goto cleanup;
}

CENTERROR
CTCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszTmpPath = NULL;

    if (pszPath == NULL)
    {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    ceError = CTAllocateString(pszPath, &pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTCreateDirectoryInternal(pszTmpPath, NULL, dwFileMode);
    BAIL_ON_CENTERIS_ERROR(ceError);

cleanup:

    CT_SAFE_FREE_STRING(pszTmpPath);

    return ceError;

error:

    goto cleanup;
}

static
CENTERROR
CTGetMatchingDirEntryPathsInFolder(PCSTR pszDirPath,
                                   PCSTR pszFileNameRegExp,
                                   PSTR** pppszHostFilePaths,
                                   PDWORD pdwNPaths,
                                   DWORD dirEntryType)
{
    typedef struct __PATHNODE
    {
        PSTR pszPath;
        struct __PATHNODE *pNext;
    } PATHNODE, *PPATHNODE;

    CENTERROR ceError = CENTERROR_SUCCESS;
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
    BOOLEAN isDirPathDir;

    *pppszHostFilePaths = NULL;
    *pdwNPaths = 0;

    ceError = CTCheckDirectoryExists(pszDirPath, &isDirPathDir);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if(!isDirPathDir)
        BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_INVALID_DIRECTORY);

    if (regcomp(&rx, pszFileNameRegExp, REG_EXTENDED) != 0) {
        ceError = CENTERROR_REGEX_COMPILE_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    rxAllocated = TRUE;

    ceError = CTAllocateMemory(sizeof(regmatch_t), (PVOID*)&pResult);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pDir = opendir(pszDirPath);
    if (pDir == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        int copied = snprintf(szBuf, sizeof(szBuf), "%s/%s", pszDirPath, pDirEntry->d_name);
        if (copied >= sizeof(szBuf))
        {
            //Skip pathnames that are too long
            continue;
        }
        memset(&statbuf, 0, sizeof(struct stat));
        if (stat(szBuf, &statbuf) < 0) {
            if(errno == ENOENT)
            {
                //This occurs when there is a symbolic link pointing to a
                //location that doesn't exist, because stat looks at the final
                //file, not the link. Since this file doesn't exist anyway,
                //just skip it.
                continue;
            }
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        /*
         * For now, we are searching only for regular files
         * This may be enhanced in the future to support additional
         * file system entry types
         */
        if (((statbuf.st_mode & S_IFMT) == dirEntryType) &&
            (regexec(&rx, pDirEntry->d_name, nMatch, pResult, 0) == 0)) {
            dwNPaths++;

            ceError = CTAllocateMemory(sizeof(PATHNODE), (PVOID*)&pPathNode);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = CTAllocateString(szBuf, &pPathNode->pszPath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pPathNode->pNext = pPathList;
            pPathList = pPathNode;
            pPathNode = NULL;
        }
    }

    if (pPathList) {
        ceError = CTAllocateMemory(sizeof(PSTR)*dwNPaths,
                                   (PVOID*)&ppszHostFilePaths);
        BAIL_ON_CENTERIS_ERROR(ceError);
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
    ppszHostFilePaths = NULL;

    *pdwNPaths = dwNPaths;

error:

    if (ppszHostFilePaths) {
        CTFreeStringArray(ppszHostFilePaths, dwNPaths);
    }

    if (pPathNode) {
        pPathNode->pNext = pPathList;
        pPathList = pPathNode;
    }

    while(pPathList) {
        pPathNode = pPathList;
        pPathList = pPathList->pNext;
        if (pPathNode->pszPath)
            CTFreeString(pPathNode->pszPath);
        CTFreeMemory(pPathNode);
    }

    if(rxAllocated)
        regfree(&rx);

    if (pResult) {
        CTFreeMemory(pResult);
    }

    if (pDir)
        closedir(pDir);

    return ceError;
}

CENTERROR
CTGetMatchingFilePathsInFolder(PCSTR pszDirPath,
                               PCSTR pszFileNameRegExp,
                               PSTR** pppszHostFilePaths,
                               PDWORD pdwNPaths)
{
    return CTGetMatchingDirEntryPathsInFolder(pszDirPath,
                                              pszFileNameRegExp,
                                              pppszHostFilePaths,
                                              pdwNPaths,
                                              S_IFREG);
}

CENTERROR
CTGetMatchingDirPathsInFolder(PCSTR pszDirPath,
                              PCSTR pszDirNameRegExp,
                              PSTR** pppszHostDirPaths,
                              PDWORD pdwNPaths)
{
    return CTGetMatchingDirEntryPathsInFolder(pszDirPath,
                                              pszDirNameRegExp,
                                              pppszHostDirPaths,
                                              pdwNPaths,
                                              S_IFDIR);
}

CENTERROR
CTCheckFileHoldsPattern(
    PCSTR pszFilePath,
    PCSTR pszPattern,
    PBOOLEAN pbPatternExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE* fp = NULL;
    CHAR szBuf[1024+1];
    regex_t rx;
    regmatch_t* pResult = NULL;
    size_t nMatch = 1;
    BOOLEAN bPatternExists = FALSE;

    memset(&rx, 0, sizeof(regex_t));

    if (regcomp(&rx, pszPattern, REG_EXTENDED) != 0) {
        ceError = CENTERROR_REGEX_COMPILE_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTAllocateMemory(sizeof(regmatch_t), (PVOID*)&pResult);
    BAIL_ON_CENTERIS_ERROR(ceError);

    fp = fopen(pszFilePath, "r");
    if (fp == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    while (!feof(fp)) {
        if (fgets(szBuf, 1024, fp)) {
            if (regexec(&rx, szBuf, nMatch, pResult, 0) == 0) {
                bPatternExists = TRUE;
                break;
            }
        } else if (!feof(fp)) {
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    *pbPatternExists = bPatternExists;

error:

    regfree(&rx);

    if (pResult)
        CTFreeMemory(pResult);

    if (fp != NULL)
        fclose(fp);

    return ceError;
}

CENTERROR
CTGetAbsolutePath(
    PSTR pszRelativePath,
    PSTR* ppszAbsolutePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[PATH_MAX+1];

    if (realpath(pszRelativePath, szBuf) == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTAllocateString(szBuf, ppszAbsolutePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

CENTERROR
CTRemoveFiles(
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
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return(ceError);
}

CENTERROR
CTGetSymLinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   )
{
   CENTERROR ceError = CENTERROR_SUCCESS;
   CHAR szBuf[PATH_MAX+1];

   memset(szBuf, 0, PATH_MAX);

   while (1) {

      if (readlink(pszPath, szBuf, PATH_MAX) < 0) {
         if (errno == EINTR)
            continue;

         ceError = CTMapSystemError(errno);
         BAIL_ON_CENTERIS_ERROR(ceError);
      }

      break;
   }

   ceError = CTAllocateString(szBuf, ppszTargetPath);
   BAIL_ON_CENTERIS_ERROR(ceError);

  error:

   return ceError;
}

CENTERROR
CTCreateSymLink(
    PCSTR pszOldPath,
    PCSTR pszNewPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (IsNullOrEmptyString(pszOldPath) ||
        IsNullOrEmptyString(pszNewPath)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (symlink(pszOldPath, pszNewPath) < 0) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;
}

CENTERROR
CTBackupFile(
    PCSTR path
    )
{
    CENTERROR ceError;
    PSTR backupPath = NULL;
    BOOLEAN exists;

    ceError = CTCheckFileExists(path, &exists);
    CLEANUP_ON_CENTERROR(ceError);
    if (!exists)
    {
        /* Do not need to backup, since the file does not yet exist. */
        goto cleanup;
    }

    ceError = CTAllocateStringPrintf(&backupPath, "%s.lwidentity.orig", path);
    CLEANUP_ON_CENTERROR(ceError);

    ceError = CTCheckFileExists(backupPath, &exists);
    CLEANUP_ON_CENTERROR(ceError);

    if (exists)
    {
        CTFreeMemory(backupPath);
        backupPath = NULL;

        ceError = CTAllocateStringPrintf(&backupPath, "%s.lwidentity.bak", path);
        CLEANUP_ON_CENTERROR(ceError);
    }

    ceError = CTCopyFileWithOriginalPerms(path, backupPath);
    CLEANUP_ON_CENTERROR(ceError);

cleanup:
    if (backupPath)
    {
        CTFreeMemory(backupPath);
        backupPath = NULL;
    }
    return ceError;
}

/*
// On some unix systems, you may not be allowed to
// move files across devices. So, we copy the file to a
// tmp file and then move it to the target location -
// and remove the original file
*/
CENTERROR
CTMoveFileAcrossDevices(
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

    ceError = CTGetOwnerAndPermissions(pszSrcPath, &uid, &gid, &mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTCopyFileWithPerms(pszSrcPath, szTmpPath, mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = TRUE;

    ceError = CTMoveFile(szTmpPath, pszDstPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = FALSE;

    ceError = CTRemoveFile(pszSrcPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTChangeOwnerAndPermissions(pszDstPath, uid, gid, mode);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if (bRemoveFile && !IsNullOrEmptyString(szTmpPath))
       CTRemoveFile(szTmpPath);

    return ceError;
}

/*
// Do not use this function on very large files since it reads the
// entire file into memory.
*/
CENTERROR
CTReadFile(
    PCSTR pszFilePath,
    PSTR *ppBuffer,
    PLONG pSize )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct stat statbuf;
    FILE *fp = NULL;

    *ppBuffer = NULL;
    if(pSize != NULL)
        *pSize = 0;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszFilePath, &statbuf) < 0) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* allocate one additional byte of memory and set it to NULL to
       allow for functions like strtok() to work properly */
    ceError = CTAllocateMemory(statbuf.st_size + 1, (VOID*)ppBuffer);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (statbuf.st_size > 0)
    {
        fp = fopen( pszFilePath, "r" );

        if ( fp == NULL ){
            ceError = CENTERROR_GP_FILE_OPEN_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if ( fread( *ppBuffer, statbuf.st_size, 1, fp ) != 1 ) {
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        fclose(fp);
        fp = NULL;

    }
    if (pSize != NULL)
    {
        *pSize = statbuf.st_size;
    }

    return ceError;

error:
    if (*ppBuffer) {
           CTFreeMemory( *ppBuffer );
           *ppBuffer = NULL;
    }

    if (fp) {
        fclose( fp );
    }

    return ceError;
}

CENTERROR
CTOpenFile(
    PCSTR path,
    PCSTR mode,
    FILE** handle)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    *handle = fopen(path, mode);

    if (!*handle)
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
error:

    return ceError;
}

CENTERROR
CTFileStreamWrite(
    FILE* handle,
    PCSTR data,
    unsigned int size)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    unsigned int written = 0;

    while (written < size)
    {
        int amount = fwrite(data+written, 1, size-written, handle);
        if (amount < 0)
        {
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        written += amount;
    }
error:

    return ceError;
}

CENTERROR
CTFilePrintf(
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
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    return ceError;
}

CENTERROR
CTCloseFile(
    FILE* handle
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (fclose(handle))
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;
}

CENTERROR
CTSafeCloseFile(
    FILE** handle
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if(*handle == NULL)
        goto cleanup;

    if (fclose(*handle))
    {
        ceError = CTMapSystemError(errno);
        GCE(ceError);
    }

cleanup:
    *handle = NULL;
    return ceError;
}

CENTERROR
CTFileContentsSame(
    PCSTR pszFilePath1,
    PCSTR pszFilePath2,
    PBOOLEAN pbSame
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE* fp1 = NULL;
    FILE* fp2 = NULL;
    BOOLEAN f1IsFile, f1IsLink;
    BOOLEAN f2IsFile, f2IsLink;

    ceError = CTCheckFileExists(pszFilePath1, &f1IsFile);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTCheckLinkExists(pszFilePath1, &f1IsLink);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTCheckFileExists(pszFilePath2, &f2IsFile);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTCheckLinkExists(pszFilePath2, &f2IsLink);
    BAIL_ON_CENTERIS_ERROR(ceError);

    f1IsFile |= f1IsLink;
    f2IsFile |= f2IsLink;

    if(!f1IsFile && !f2IsFile)
    {
        *pbSame = TRUE;
        goto error;
    }

    if(!f1IsFile || !f2IsFile)
    {
        *pbSame = FALSE;
        goto error;
    }

    ceError = CTOpenFile(pszFilePath1, "r", &fp1);
    BAIL_ON_CENTERIS_ERROR(ceError);
    ceError = CTOpenFile(pszFilePath2, "r", &fp2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTStreamContentsSame(fp1, fp2, pbSame);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:
    if (fp1 != NULL)
        fclose(fp1);
    if (fp2 != NULL)
        fclose(fp2);

    return ceError;
}

CENTERROR
CTStreamContentsSame(
    FILE *fp1,
    FILE *fp2,
    PBOOLEAN pbSame
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    unsigned char buffer1[1024], buffer2[1024];
    size_t read1, read2;

    while (1) {
        read1 = fread(buffer1, 1, sizeof(buffer1), fp1);
        if(read1 < sizeof(buffer1) && ferror(fp1))
        {
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        read2 = fread(buffer2, 1, sizeof(buffer2), fp2);
        if(read2 < sizeof(buffer2) && ferror(fp2))
        {
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if(read1 != read2 || memcmp(buffer1, buffer2, read1) != 0)
        {
            *pbSame = FALSE;
            goto error;
        }

        if(read1 == 0)
            break;
    }

    *pbSame = TRUE;

error:
    return ceError;
}

CENTERROR
CTFindSed(
        PSTR *sedPath
        )
{
    CENTERROR ceError = CTFindFileInPath("sed", "/bin:/usr/bin", sedPath);
    if(ceError == CENTERROR_FILE_NOT_FOUND)
        ceError = CENTERROR_SED_NOT_FOUND;
    return ceError;
}

CENTERROR
CTWillSedChangeFile(
    PCSTR pszSrcPath,
    PCSTR pszExpression,
    BOOLEAN *changes
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCSTR  ppszArgs[] =
        { NULL,
          NULL,
          NULL,
          NULL,
        };
    FILE *srcFile = NULL;
    FILE *sedOut = NULL;
    int duppedFdout = -1;
    int argPos = 0;
    PPROCINFO pProcInfo = NULL;
    LONG status = 0;
    PSTR sedPath = NULL;
    BOOLEAN same = FALSE;

    BAIL_ON_CENTERIS_ERROR(ceError = CTFindSed(&sedPath));
    ppszArgs[argPos++] = sedPath;

    ppszArgs[argPos++] = pszExpression;

    ceError = CTOpenFile(pszSrcPath, "r", &srcFile);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTSpawnProcessWithFds(ppszArgs[0], (const PSTR *)ppszArgs, fileno(srcFile), -1, 2, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Get a FILE * for the stdout file descriptor */
    duppedFdout = dup(pProcInfo->fdout);
    if (duppedFdout < 0)
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    sedOut = fdopen(duppedFdout, "r");
    if(sedOut == NULL)
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    duppedFdout = -1;

    // Reopen the source file so that we don't share the file offset with
    // sed (and thus we can reread the same data).
    ceError = CTCloseFile(srcFile);
    srcFile = NULL;
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTOpenFile(pszSrcPath, "r", &srcFile);
    BAIL_ON_CENTERIS_ERROR(ceError);

    BAIL_ON_CENTERIS_ERROR(ceError = CTStreamContentsSame(srcFile, sedOut, &same));

    ceError = CTGetExitStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_COMMAND_FAILED);
    }

    ceError = CTCloseFile(srcFile);
    srcFile = NULL;
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTCloseFile(sedOut);
    sedOut = NULL;
    BAIL_ON_CENTERIS_ERROR(ceError);

    *changes = !same;

error:
    if(srcFile != NULL)
        CTCloseFile(srcFile);
    if(sedOut != NULL)
        CTCloseFile(sedOut);
    if(duppedFdout != -1)
        close(duppedFdout);
    if (pProcInfo)
        CTFreeProcInfo(pProcInfo);
    CT_SAFE_FREE_STRING(sedPath);

    return ceError;
}

CENTERROR
CTRunSedOnFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    BOOLEAN bDashN,
    PCSTR pszExpression
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCSTR  ppszArgs[] =
        { NULL,
          NULL,
          NULL,
          NULL,
        };
    int dwFdIn = -1, dwFdOut = -1;
    int argPos = 0;
    PPROCINFO pProcInfo = NULL;
    LONG status = 0;
    PSTR tempPath = NULL;
    uid_t uid;
    gid_t gid;
    mode_t mode;
    PSTR sedPath = NULL;
    BOOLEAN isSame;
    PSTR pszFinalPath = NULL;

    BAIL_ON_CENTERIS_ERROR(ceError = CTFindSed(&sedPath));
    ppszArgs[argPos++] = sedPath;

    if(bDashN)
        ppszArgs[argPos++] = "-n";
    ppszArgs[argPos++] = pszExpression;

    ceError = CTGetFileTempPath(
                        pszDstPath,
                        &pszFinalPath,
                        &tempPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    dwFdIn = open(pszSrcPath, O_RDONLY);
    if (dwFdIn < 0)
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    dwFdOut = open(tempPath, O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR);
    if (dwFdOut < 0)
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    BAIL_ON_CENTERIS_ERROR(ceError = CTGetOwnerAndPermissions(pszSrcPath, &uid, &gid, &mode));
    BAIL_ON_CENTERIS_ERROR(ceError = CTChangeOwnerAndPermissions(tempPath, uid, gid, mode));

    ceError = CTSpawnProcessWithFds(ppszArgs[0], (const PSTR *)ppszArgs, dwFdIn, dwFdOut, 2, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTGetExitStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_COMMAND_FAILED);
    }

    BAIL_ON_CENTERIS_ERROR(ceError = CTFileContentsSame(tempPath, pszFinalPath, &isSame));
    if (isSame)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTRemoveFile(tempPath));
    }
    else
    {
        ceError = CTSafeReplaceFile(pszFinalPath, tempPath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    if(dwFdIn != -1)
        close(dwFdIn);
    if(dwFdOut != -1)
    {
        close(dwFdOut);
        if(!CENTERROR_IS_OK(ceError))
        {
            CTRemoveFile(tempPath);
        }
    }
    if (pProcInfo)
        CTFreeProcInfo(pProcInfo);
    CT_SAFE_FREE_STRING(tempPath);
    CT_SAFE_FREE_STRING(pszFinalPath);
    CT_SAFE_FREE_STRING(sedPath);

    return ceError;
}

CENTERROR
CTCloneFilePerms(
    PCSTR pszTemplatePath,
    PCSTR pszDstPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    uid_t uid;
    gid_t gid;
    mode_t mode;

    ceError = CTGetOwnerAndPermissions(pszTemplatePath, &uid, &gid, &mode);
    CLEANUP_ON_CENTERROR(ceError);

    ceError = CTChangeOwnerAndPermissions(pszDstPath, uid, gid, mode);
    CLEANUP_ON_CENTERROR(ceError);

cleanup:
    return ceError;
}

CENTERROR
CTFindFileInPath(
    PCSTR filename,
    PCSTR searchPath,
    PSTR* foundPath
    )
{
    return CTFindInPath(NULL, filename, searchPath, foundPath);
}

CENTERROR
CTFindInPath(
    PCSTR rootPrefix,
    PCSTR filename,
    PCSTR searchPath,
    PSTR* foundPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    //Copy the search path so that strtok can be run on it
    PSTR mySearchPath = NULL;
    PSTR strtokSavePtr = NULL;
    PSTR currentDir = NULL;
    PSTR testPath = NULL;
    BOOLEAN exists = FALSE;

    if(foundPath != NULL)
        *foundPath = NULL;

    if(rootPrefix == NULL)
        rootPrefix = "";

    GCE(ceError = CTStrdup(searchPath, &mySearchPath));

    currentDir = strtok_r(mySearchPath, ":", &strtokSavePtr);
    while(TRUE)
    {
        CT_SAFE_FREE_STRING(testPath);
        GCE(ceError = CTAllocateStringPrintf(&testPath, "%s%s/%s", rootPrefix, currentDir, filename));
        GCE(ceError = CTCheckFileOrLinkExists(testPath, &exists));
        if(exists)
        {
            if(foundPath != NULL)
            {

                GCE(ceError = CTAllocateStringPrintf(foundPath, "%s/%s", currentDir, filename));
            }
            break;
        }
        currentDir = strtok_r(NULL, ":", &strtokSavePtr);
        if(currentDir == NULL)
        {
            GCE(ceError = CENTERROR_FILE_NOT_FOUND);
        }
    }

cleanup:
    CT_SAFE_FREE_STRING(mySearchPath);
    CT_SAFE_FREE_STRING(testPath);
    return ceError;
}

CENTERROR
CTGetFileDiff(
    PCSTR first,
    PCSTR second,
    PSTR *diff,
    BOOLEAN failIfNoDiff
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR diffPath = NULL;
    PCSTR args[4] = { NULL, first, second, NULL };
    int exitCode;

    *diff = NULL;
    ceError = CTFindFileInPath("diff",
            "/bin:/usr/bin:/sbin:/usr/sbin", &diffPath);
    if(ceError == CENTERROR_FILE_NOT_FOUND && !failIfNoDiff)
    {
        GCE(ceError = CTStrdup("Diff command not found", diff));
        goto cleanup;
    }
    GCE(ceError);
    args[0] = diffPath;

    GCE(ceError = CTCaptureOutputWithStderrEx(args[0], args, FALSE, diff, 
                &exitCode));
    //Diff returns 1 when the files are different
    if(exitCode == 1)
    {
        exitCode = 0;
    }
    if(exitCode != 0)
        GCE(ceError = CENTERROR_COMMAND_FAILED);

cleanup:
    CT_SAFE_FREE_STRING(diffPath);
    return ceError;
}

CENTERROR
CTGetFileTempPath(
        PCSTR unresolvedSrcPath,
        PSTR* resolvedSrcPath,
        PSTR* tempPath)
{
    PSTR symTarget = NULL;
    PSTR newPath = NULL;
    PSTR currentPath = NULL;
    CENTERROR ceError = CENTERROR_SUCCESS;
    // do not free
    PSTR separator = NULL;

    if (resolvedSrcPath)
    {
        *resolvedSrcPath = NULL;
    }
    if (tempPath)
    {
        *tempPath = NULL;
    }

    ceError = CTAllocateString(unresolvedSrcPath, &currentPath);
    GCE(ceError);

    while (1)
    {
        ceError = CTGetSymLinkTarget(
                        currentPath,
                        &symTarget);
        if (ceError == CTMapSystemError(EINVAL))
        {
            // The last symlink component was resolved
            ceError = 0;
            break;
        }
        else if (ceError == CTMapSystemError(ENOENT))
        {
            // The target does not exist yet. The caller should create this path
            ceError = 0;
            break;
        }
        GCE(ceError);

        // Strip off the file component
        separator = strrchr(currentPath, '/');
        if (separator != NULL)
        {
            separator[1] = 0;
        }

        if (symTarget[0] == '/')
        {
            GCE(ceError = CTAllocateStringPrintf(&newPath, "%s", symTarget));
        }
        else
        {
            GCE(ceError = CTAllocateStringPrintf(&newPath, "%s%s", currentPath,
                        symTarget));
        }

        CT_SAFE_FREE_STRING(currentPath);
        CT_SAFE_FREE_STRING(symTarget);
        currentPath = newPath;
        newPath = NULL;
    }

    if (tempPath)
    {
        GCE(ceError = CTAllocateStringPrintf(tempPath, "%s.lwidentity.new", currentPath));
    }

    if (resolvedSrcPath)
    {
        *resolvedSrcPath = currentPath;
        currentPath = NULL;
    }

cleanup:
    CT_SAFE_FREE_STRING(currentPath);
    CT_SAFE_FREE_STRING(newPath);
    CT_SAFE_FREE_STRING(symTarget);
    return ceError;
}

CENTERROR
CTSafeReplaceFile(
        PCSTR finalName,
        PCSTR replaceWith)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    GCE(ceError = CTCloneFilePerms(finalName, replaceWith));
    GCE(ceError = CTBackupFile(finalName));
    GCE(ceError = CTMoveFile(replaceWith, finalName));
    
cleanup:
    return ceError;
}

CENTERROR
CTCopyDirectory(
    PCSTR source,
    PCSTR dest
    )
{
    uid_t uid;
    gid_t gid;
    mode_t mode;
    CENTERROR ceError = CENTERROR_SUCCESS;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    PSTR srcPath = NULL;
    PSTR destPath = NULL;

    GCE(ceError = CTGetOwnerAndPermissions(source, &uid, &gid, &mode));
    GCE(ceError = CTCreateDirectory(dest, mode));
    GCE(ceError = CTChangeOwnerAndPermissions(dest, uid, gid, mode));


    if ((pDir = opendir(source)) == NULL) {
        GCE(ceError = CTMapSystemError(errno));
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        GCE(ceError = CTAllocateStringPrintf(&srcPath, "%s/%s", source,
                    pDirEntry->d_name));
        GCE(ceError = CTAllocateStringPrintf(&destPath, "%s/%s", dest,
                    pDirEntry->d_name));

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(srcPath, &statbuf) < 0) {
            GCE(ceError = CTMapSystemError(errno));
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
            GCE(ceError = CTCopyDirectory(srcPath, destPath));
        } else {
            GCE(ceError = CTCopyFileWithOriginalPerms(srcPath, destPath));
        }
    }
    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        GCE(ceError = CTMapSystemError(ceError));
    }
    pDir = NULL;

cleanup:
    if (pDir)
        closedir(pDir);
    CT_SAFE_FREE_STRING(srcPath);
    CT_SAFE_FREE_STRING(destPath);
    return ceError;
}

CENTERROR
CTReadNextLine(
    FILE* fp,
    PSTR *output,
    PBOOLEAN pbEndOfFile
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DynamicArray buffer;
    const char nullTerm = '\0';

    *pbEndOfFile = 0;
    *output = NULL;
    memset(&buffer, 0, sizeof(buffer));
    GCE(ceError = CTSetCapacity(&buffer, 1, 100));

    while(1)
    {
        if(fgets(buffer.data + buffer.size,
                buffer.capacity - buffer.size, fp) ==  NULL)
        {
            if (feof(fp)) {
                *pbEndOfFile = 1;
            } else {
                ceError = CTMapSystemError(errno);
                GCE(ceError);
            }
        }
        buffer.size += strlen(buffer.data + buffer.size);

        if(*pbEndOfFile)
            break;
        if(buffer.size == buffer.capacity - 1 &&
                ((char *)buffer.data)[buffer.size - 1] != '\n')
            GCE(ceError = CTSetCapacity(&buffer, 1, buffer.capacity * 2));
        else
            break;
    }

    GCE(ceError = CTArrayAppend(&buffer, 1, &nullTerm, 1));
    *output = buffer.data;
    buffer.data = NULL;

cleanup:
    CTArrayFree(&buffer);
    return ceError;
}

//The dynamic array must be initialized (at least zeroed out) beforehand
CENTERROR
CTReadLines(FILE *file, DynamicArray *dest)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    BOOLEAN eof = FALSE;
    PSTR readLine = NULL;

    CTArrayFree(dest);
    while(!eof)
    {
        GCE(ceError = CTReadNextLine(file, &readLine, &eof));
        GCE(ceError = CTArrayAppend(dest, sizeof(readLine), &readLine, 1));
        readLine = NULL;
    }

cleanup:
    CT_SAFE_FREE_STRING(readLine);
    return ceError;
}

CENTERROR
CTWriteLines(FILE *file, const DynamicArray *lines)
{
    size_t i;
    CENTERROR ceError = CENTERROR_SUCCESS;

    for(i = 0; i < lines->size; i++)
    {
        PCSTR line = *(PCSTR *)CTArrayGetItem((DynamicArray*)lines, i,
                sizeof(PCSTR));
        GCE(ceError = CTFileStreamWrite(file, line, strlen(line)));
    }

cleanup:
    return ceError;
}

void
CTFreeLines(DynamicArray *lines)
{
    size_t i;

    for(i = 0; i < lines->size; i++)
    {
        CT_SAFE_FREE_STRING(*(PSTR *)CTArrayGetItem(
                    (DynamicArray*)lines, i, sizeof(PSTR)));
    }
    CTArrayFree(lines);
}

CENTERROR
CTSetCloseOnExec(
    int fd)
{
    long flags = fcntl(fd, F_GETFD);
    if(flags < 0)
        return CTMapSystemError(errno);

    flags |= FD_CLOEXEC;
    if(fcntl(fd, F_SETFD, flags) < 0)
        return CTMapSystemError(errno);

    return CENTERROR_SUCCESS;
}
