/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#include "ctbase.h"
#include <unistd.h>
#include "Utilities.h"
#include "PlugInShell.h"
#include <DirectoryService/DirServicesTypes.h>

long
CTRemoveFile(
    PCSTR pszPath
    )
{
    long macError = eDSNoErr;

    while (1) {
        if (unlink(pszPath) < 0) {
            if (errno == EINTR) {
                continue;
            }
            macError = CTMapSystemError(errno);
            GOTO_CLEANUP_ON_MACERROR(macError);
        } else {
            break;
        }
    }

cleanup:

    return macError;
}

/*
// TODO: Check access and removability before actual deletion
*/
long
CTRemoveDirectory(
    PCSTR pszPath
    )
{
    long macError = eDSNoErr;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[PATH_MAX+1];

    if ((pDir = opendir(pszPath)) == NULL) {
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0) {
            macError = CTMapSystemError(errno);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
            macError = CTRemoveDirectory(szBuf);
            GOTO_CLEANUP_ON_MACERROR(macError);

            if (rmdir(szBuf) < 0) {
                macError = CTMapSystemError(macError);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }

        } else {

            macError = CTRemoveFile(szBuf);
            GOTO_CLEANUP_ON_MACERROR(macError);

        }
    }
    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        macError = CTMapSystemError(macError);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    pDir = NULL;

    if (rmdir(pszPath) < 0) {
        macError = CTMapSystemError(macError);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    if (pDir)
        closedir(pDir);

    return macError;
}

long
CTCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    )
{
    long macError = eDSNoErr;

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
            macError = CTMapSystemError(errno);
            GOTO_CLEANUP_ON_MACERROR(macError);

        }

        /*
           The path exists. Is it a directory?
         */

        *pbDirExists = (((statbuf.st_mode & S_IFMT) == S_IFDIR) ? TRUE : FALSE);
        break;
    }

cleanup:

    return macError;
}

long
CTChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    )
{
    long macError = eDSNoErr;

    while (1) {
        if (chmod(pszPath, dwFileMode) < 0) {
            if (errno == EINTR) {
                continue;
            }
            macError = CTMapSystemError(errno);
            GOTO_CLEANUP_ON_MACERROR(macError);
        } else {
            break;
        }
    }

cleanup:

    return macError;
}

long
CTGetCurrentDirectoryPath(
    PSTR* ppszPath
    )
{
    long macError = eDSNoErr;
    CHAR szBuf[PATH_MAX+1];
    PSTR pszPath = NULL;

    if (getcwd(szBuf, PATH_MAX) == NULL) {
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = LWAllocateString(szBuf, &pszPath);
    GOTO_CLEANUP_ON_MACERROR(macError);

    *ppszPath = pszPath;

    return macError;

cleanup:

    if (pszPath) {
        LWFreeString(pszPath);
    }

    return macError;
}

long
CTChangeDirectory(
    PSTR pszPath
    )
{
    if (pszPath == NULL || *pszPath == '\0')
        return eParameterError;

    if (chdir(pszPath) < 0)
        return CTMapSystemError(errno);

    return eDSNoErr;
}

long
CTCreateDirectoryRecursive(
    PSTR pszCurDirPath,
    PSTR pszTmpPath,
    PSTR *ppszTmp,
    DWORD dwFileMode,
    DWORD dwWorkingFileMode,
    int  iPart
    )
{
    long macError = eDSNoErr;
    PSTR pszDirPath = NULL;
    BOOLEAN bDirCreated = FALSE;
    BOOLEAN bDirExists = FALSE;
    CHAR szDelimiters[] = "/";

    PSTR pszToken = strtok_r((iPart ? NULL : pszTmpPath), szDelimiters, ppszTmp);

    if (pszToken != NULL) {

        macError = LWAllocateMemory(strlen(pszCurDirPath)+strlen(pszToken)+2,
                                   (PVOID*)&pszDirPath);
        GOTO_CLEANUP_ON_MACERROR(macError);

        sprintf(pszDirPath,
                "%s/%s",
                (!strcmp(pszCurDirPath, "/") ? "" : pszCurDirPath),
                pszToken);

        macError = CTCheckDirectoryExists(pszDirPath, &bDirExists);
        GOTO_CLEANUP_ON_MACERROR(macError);

        if (!bDirExists) {
            if (mkdir(pszDirPath, dwWorkingFileMode) < 0) {
                macError = CTMapSystemError(errno);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
            bDirCreated = TRUE;
        }

        macError = CTChangeDirectory(pszDirPath);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = CTCreateDirectoryRecursive(
            pszDirPath,
            pszTmpPath,
            ppszTmp,
            dwFileMode,
            dwWorkingFileMode,
            iPart+1
            );
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (bDirCreated && (dwFileMode != dwWorkingFileMode)) {
        macError = CTChangePermissions(pszDirPath, dwFileMode);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszDirPath) {
        LWFreeMemory(pszDirPath);
    }

    return macError;

cleanup:

    if (bDirCreated) {
        CTRemoveDirectory(pszDirPath);
    }

    if (pszDirPath) {
        LWFreeMemory(pszDirPath);
    }

    return macError;
}

long
CTCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    )
{
    long macError = eDSNoErr;
    PSTR pszCurDirPath = NULL;
    PSTR pszTmpPath = NULL;
    PSTR pszTmp = NULL;
    mode_t dwWorkingFileMode;

    if (pszPath == NULL || *pszPath == '\0') {
        macError = eParameterError;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    dwWorkingFileMode = dwFileMode;
    if (!(dwFileMode & S_IXUSR)) {
        /*
         * This is so that we can navigate the folders
         * when we are creating the subfolders
         */
        dwWorkingFileMode |= S_IXUSR;
    }

    macError = CTGetCurrentDirectoryPath(&pszCurDirPath);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWAllocateString(pszPath, &pszTmpPath);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (*pszPath == '/') {
        macError = CTChangeDirectory((char*)"/");
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = CTCreateDirectoryRecursive((char*)"/", pszTmpPath, &pszTmp, dwFileMode, dwWorkingFileMode, 0);
        GOTO_CLEANUP_ON_MACERROR(macError);

    } else {

        macError = CTCreateDirectoryRecursive(pszCurDirPath, pszTmpPath, &pszTmp, dwFileMode, dwWorkingFileMode, 0);
        GOTO_CLEANUP_ON_MACERROR(macError);

    }

cleanup:

    if (pszCurDirPath) {

        CTChangeDirectory(pszCurDirPath);

        LWFreeMemory(pszCurDirPath);

    }

    if (pszTmpPath) {
        LWFreeMemory(pszTmpPath);
    }

    return macError;
}
