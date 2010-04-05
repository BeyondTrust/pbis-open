/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        wnet_addconnection2.c
 *
 * Abstract:
 *
 *        Networking client interface
 *
 *        WNetAddConnection2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

extern
DWORD
ResourceToLwIoPathPrefix(
    PCWSTR pwszRemote,
    PWSTR* ppwszPath
    );

DWORD
ResourceToLwIoPathPrefix(
    PCWSTR pwszRemote,
    PWSTR* ppwszPath
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszRemote = NULL;
    PWSTR pwszPath = NULL;
    PWSTR pwszIn = NULL;
    PWSTR pwszOut = NULL;

    dwError = LwWc16sToMbs(pwszRemote, &pszRemote);
    BAIL_ON_WIN_ERROR(dwError);

    pwszPath = asw16printfw(L"/rdr/%s/", pszRemote);
    BAIL_ON_WIN_ERROR(dwError);

    for (pwszIn = pwszOut = pwszPath; *pwszIn; pwszIn++)
    {
        switch (*pwszIn)
        {
        case '\\':
        case '/':
            *(pwszOut++) = '/';
            while (pwszIn[1] == '\\' ||
                   pwszIn[1] == '/')
            {
                pwszIn++;
            }
            break;
        default:
            *(pwszOut++) = *pwszIn;
            break;
        }
    }

    *pwszOut = '\0';

    *ppwszPath = pwszPath;

error:

    LW_SAFE_FREE_MEMORY(pszRemote);

    return dwError;
}

static
DWORD
CrackUsername(
    PCWSTR pwszUsername,
    PWSTR* ppwszUser,
    PWSTR* ppwszDomain
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PCWSTR pwszSlash = NULL;
    PCWSTR pwszIndex = NULL;
    PWSTR pwszUser = NULL;
    PWSTR pwszDomain = NULL;

    for (pwszIndex = pwszUsername; *pwszIndex; pwszIndex++)
    {
        if (*pwszIndex == '\\')
        {
            pwszSlash = pwszIndex;
            break;
        }
    }
 
    if (pwszSlash)
    {
        dwError = LwAllocateWc16String(&pwszDomain, pwszUsername);
        BAIL_ON_WIN_ERROR(dwError);

        pwszDomain[pwszSlash - pwszUsername] = '\0';

        dwError = LwAllocateWc16String(&pwszUser, pwszSlash + 1);
        BAIL_ON_WIN_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateWc16String(&pwszUser, pwszUsername);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwMbsToWc16s("WORKGROUP", &pwszDomain);
        BAIL_ON_WIN_ERROR(dwError);
    }

    *ppwszUser = pwszUser;
    *ppwszDomain = pwszDomain;

cleanup:
    
    return dwError;

error:

    *ppwszUser = NULL;
    *ppwszDomain = NULL;

    LW_SAFE_FREE_MEMORY(pwszUser);
    LW_SAFE_FREE_MEMORY(pwszDomain);

    goto cleanup;
}

static
DWORD
TestConnection(
    IN PCWSTR pwszPath
    )
{
    DWORD dwError = 0;
    PIO_CREDS pThreadCreds = NULL;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatus = {0};
    IO_FILE_NAME filename = {0};

    /* Save current thread-local credentials */
    dwError = LwNtStatusToWin32Error(LwIoGetThreadCreds(&pThreadCreds));
    BAIL_ON_WIN_ERROR(dwError);

    /* Disable thread-local credentials so we use path-based creds */
    dwError = LwNtStatusToWin32Error(LwIoSetThreadCreds(NULL));
    BAIL_ON_WIN_ERROR(dwError);

    filename.FileName = (PWSTR) pwszPath;

    dwError = LwNtStatusToWin32Error(NtCreateFile(
        &hFile,                                  /* Created handle */
        NULL,                                    /* Async control block */
        &ioStatus,                               /* Status block */
        &filename,                               /* Filename */
        NULL,                                    /* Security descriptor */
        NULL,                                    /* Security QOS */
        GENERIC_READ,                            /* Access mode */
        0,                                       /* Allocation size */
        0,                                       /* File attributes */
        FILE_SHARE_READ | FILE_SHARE_WRITE,      /* Sharing mode */
        FILE_OPEN,                               /* Create disposition */
        FILE_DIRECTORY_FILE,                     /* Create options */
        NULL,                                    /* EA buffer */
        0,                                       /* EA buffer length */
        NULL));                                  /* ECP List */
    BAIL_ON_WIN_ERROR(dwError);

error:

    if (pThreadCreds)
    {
        LwIoSetThreadCreds(pThreadCreds);
        LwIoDeleteCreds(pThreadCreds);
    }

    if (hFile)
    {
        NtCloseFile(hFile);
    }

    return dwError;
}

DWORD
WNetAddConnection2(
    IN PNETRESOURCE pResource,
    IN PCWSTR       pwszPassword,
    IN PCWSTR       pwszUsername,
    IN DWORD        dwFlags
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszUser = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszPath = NULL;
    PIO_CREDS pCreds = NULL;

    dwError = ResourceToLwIoPathPrefix(pResource->pwszRemoteName, &pwszPath);
    BAIL_ON_WIN_ERROR(dwError);

    if (pwszUsername && pwszPassword)
    {
        dwError = CrackUsername(pwszUsername, &pwszUser, &pwszDomain);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwNtStatusToWin32Error(LwIoCreatePlainCredsW(pwszUser, pwszDomain, pwszPassword, &pCreds));
        BAIL_ON_WIN_ERROR(dwError);
    }
        
    dwError = LwNtStatusToWin32Error(LwIoSetPathCreds(pwszPath, pCreds));
    BAIL_ON_WIN_ERROR(dwError);

    dwError = TestConnection(pwszPath);
    BAIL_ON_WIN_ERROR(dwError);

error:

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    LW_SAFE_FREE_MEMORY(pwszUser);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszPath);

    return dwError;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
