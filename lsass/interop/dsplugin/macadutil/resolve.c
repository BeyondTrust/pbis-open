/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "../includes.h"

#define BAIL_ON_ERROR(dwError) { if (dwError) goto error; }

//#define BAIL_ON_KRB_ERROR(ctx, ret) { if (ret) dwError = LW_ERROR_KRB5_CALL_FAILED;}

/* Given a UNC path, converts slashes to Unix/Web standard. */
static
DWORD
ConvertSlashes(
    PCSTR pszPath,
    PSTR *ppszConvertedPath
    )
{
    DWORD dwError = 0;
    PSTR pszConvertedPath = NULL;
    size_t i;

    dwError = LwAllocateString(pszPath, &pszConvertedPath);
    BAIL_ON_ERROR(dwError);

    for (i = 0; pszConvertedPath[i]; i++)
    {
        if (pszConvertedPath[i] == '\\')
        {
            pszConvertedPath[i] = '/';
        }
    }

    *ppszConvertedPath = pszConvertedPath;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszConvertedPath);
    goto cleanup;
}

/* Given a UNC path with forward slashes, returns a resolved UNC address.with
  forward slashes.
 */ 
static
DWORD
ResolveRemotePath(
    PCSTR pszPath,
    PSTR *ppszPhysicalPath
    )
{
    DWORD dwError = 0;
    PWSTR pwszRemotePath = NULL;
    PSTR pszPhysicalPathWrongSlashes = NULL;
    PSTR pszPhysicalPathRightSlashes = NULL;
    PSTR pszPhysicalPath = NULL;
    PWSTR pwszPhysicalPath = NULL;
    IO_FILE_NAME filename = {0};
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatus;

    /* Handle one or two leading '/' */
    if (pszPath[0] == '/')
    {
        if (pszPath[1] == '/')
        {
            dwError = LwAllocateWc16sPrintfW(
                            &pwszRemotePath,
                            L"/rdr%s",
                            pszPath + 1);
            BAIL_ON_ERROR(dwError);
        }
        else
        {
            dwError = LwAllocateWc16sPrintfW(
                            &pwszRemotePath,
                            L"/rdr%s",
                            pszPath);
            BAIL_ON_ERROR(dwError);
        }
    }

    LwRtlUnicodeStringInit(&filename.Name, pwszRemotePath);

    dwError = LwNtStatusToWin32Error(
                LwNtCreateFile(
                    &hFile,
                    NULL,
                    &ioStatus,
                    &filename,
                    NULL,
                    NULL,
                    READ_CONTROL | FILE_READ_ATTRIBUTES,
                    0,
                    0,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    FILE_DIRECTORY_FILE,
                    NULL,
                    0,
                    NULL,
                    NULL));
    BAIL_ON_ERROR(dwError);


    dwError = LwNtStatusToWin32Error(
                LwIoRdrGetPhysicalPath(
                    hFile,
                    &pwszPhysicalPath));
    BAIL_ON_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszPhysicalPath, &pszPhysicalPathWrongSlashes);
    BAIL_ON_ERROR(dwError);

    dwError = ConvertSlashes(
            pszPhysicalPathWrongSlashes,
            &pszPhysicalPathRightSlashes);
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                &pszPhysicalPath,
                "/%s",
                pszPhysicalPathRightSlashes);
    BAIL_ON_ERROR(dwError);

    LwNtCloseFile(hFile);
    hFile = NULL;

    *ppszPhysicalPath = pszPhysicalPath;
    pszPhysicalPath = NULL;

cleanup:

    LW_SAFE_FREE_STRING(pszPhysicalPathRightSlashes);
    LW_SAFE_FREE_STRING(pszPhysicalPathWrongSlashes);
    LW_SAFE_FREE_MEMORY(pwszPhysicalPath);
    LW_SAFE_FREE_MEMORY(pwszRemotePath);

    if (hFile)
    {
        LwNtCloseFile(hFile);
        hFile = NULL;
    }
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszPhysicalPath);
    goto cleanup;
}


static
DWORD
SplitUncPath(
    PCSTR pszUncPath,
    PSTR *ppszServerName,
    PSTR *ppszShare,
    PSTR *ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszServerName = NULL;
    PSTR pszServerNameIter = NULL;
    PSTR pszShare = NULL;
    PSTR pszShareIter = NULL;
    PSTR pszPath = NULL;

    while (*pszUncPath == '/')
    {
        pszUncPath++;
    }

    if (!*pszUncPath)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwAllocateString(pszUncPath, &pszServerName);
    BAIL_ON_ERROR(dwError);

    pszServerNameIter = pszServerName;
    while (*pszServerNameIter && *pszServerNameIter != '/')
        pszServerNameIter++;

    if (*pszServerNameIter)
    {
        *pszServerNameIter = '\0';

        dwError = LwAllocateString(pszServerNameIter + 1, &pszShare);
        BAIL_ON_ERROR(dwError);
    }

    pszShareIter = pszShare;
    while (*pszShareIter && *pszShareIter != '/')
        pszShareIter++;

    if (*pszShareIter)
    {
        *pszShareIter = '\0';

        dwError = LwAllocateString(pszShareIter + 1, &pszPath);
        BAIL_ON_ERROR(dwError);
    }


    if (ppszServerName)
    {
        *ppszServerName = pszServerName;
        pszServerName = NULL;
    }

    if (ppszShare)
    {
        *ppszShare = pszShare;
        pszShare = NULL;
    }

    if (ppszPath)
    {
        *ppszPath = pszPath;
        pszPath = NULL;
    }

error:
    LW_SAFE_FREE_STRING(pszServerName);
    LW_SAFE_FREE_STRING(pszShare);
    LW_SAFE_FREE_STRING(pszPath);
    return dwError;
}

static
DWORD
ConvertPathInternal(
    IN PCSTR pszOriginalUncPath,
    OUT PSTR *ppszServerName,
    OUT PSTR *ppszShare,
    OUT PSTR *ppszPath)
{
    DWORD dwError = 0;
    PSTR pszUncPathForwardSlashes = NULL;
    PSTR pszUncResolvedPath = NULL;
    PSTR pszServerName = NULL;
    PSTR pszShare = NULL;
    PSTR pszPath = NULL;
    PCSTR pszUncPath = NULL;

    dwError = ConvertSlashes(pszOriginalUncPath, &pszUncPathForwardSlashes);
    BAIL_ON_ERROR(dwError);

    dwError = ResolveRemotePath(pszUncPathForwardSlashes, &pszUncResolvedPath);
    dwError = 0;

    pszUncPath = pszUncPathForwardSlashes;
    if (pszUncResolvedPath)
        pszUncPath = pszUncResolvedPath;

    dwError = SplitUncPath(pszUncPath, &pszServerName, &pszShare, &pszPath);
    BAIL_ON_ERROR(dwError);

    *ppszServerName = pszServerName;
    *ppszShare = pszShare;
    *ppszPath = pszPath;

cleanup:
    LW_SAFE_FREE_STRING(pszUncResolvedPath);
    LW_SAFE_FREE_STRING(pszUncPathForwardSlashes);
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszPath);
    LW_SAFE_FREE_STRING(pszShare);
    LW_SAFE_FREE_STRING(pszServerName);
    goto cleanup;
}

VOID
ConvertPath(
    IN uid_t uid,
    IN PCSTR pszOriginalUncPath,
    OUT PSTR *ppszServer,
    OUT PSTR *ppszShare,
    OUT PSTR *ppszPath
    )
{
    PSTR pszServer = NULL;
    PSTR pszShare = NULL;
    PSTR pszPath = NULL;

    ConvertPathInternal(pszOriginalUncPath, &pszServer, &pszShare, &pszPath);

    if (ppszServer)
    {
        *ppszServer = pszServer;
        pszServer = NULL;
    }

    if (ppszShare)
    {
        *ppszShare = pszShare;
        pszShare = NULL;
    }

    if (ppszPath)
    {
        *ppszPath = pszPath;
        pszPath = NULL;
    }

    LW_SAFE_FREE_STRING(pszServer);
    LW_SAFE_FREE_STRING(pszShare);
    LW_SAFE_FREE_STRING(pszPath);
}

VOID
FreePaths(
    PSTR pszServer,
    PSTR pszShare,
    PSTR pszPath
    )
{
    LW_SAFE_FREE_STRING(pszServer);
    LW_SAFE_FREE_STRING(pszShare);
    LW_SAFE_FREE_STRING(pszPath);
}
