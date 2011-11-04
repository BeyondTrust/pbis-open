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
 *        pam-config.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Pluggable Authentication Module
 *
 *        Configuration API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

#define LSA_IGNORE_LIST_UPDATE_INTERVAL (5*60)
#define LSA_USER_IGNORE_LIST_PATH CONFIGDIR "/user-ignore"
#define LSA_GROUP_IGNORE_LIST_PATH CONFIGDIR "/group-ignore"

static
DWORD
LsaPamGetConfigFromServer(
    OUT PLSA_PAM_CONFIG *ppConfig
    );

DWORD
LsaPamGetConfig(
    OUT PLSA_PAM_CONFIG* ppConfig
    )
{
    DWORD dwError = 0;
    PLSA_PAM_CONFIG pConfig = NULL;

    dwError = LsaPamGetConfigFromServer(&pConfig);
    if (dwError)
    {
        dwError = LsaUtilAllocatePamConfig(&pConfig);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppConfig = pConfig;

    return dwError;

error:
    if ( pConfig )
    {
        LsaPamFreeConfig(pConfig);
        pConfig = NULL;
    }

    goto cleanup;
}

VOID
LsaPamFreeConfig(
    IN PLSA_PAM_CONFIG pConfig
    )
{
    LsaUtilFreePamConfig(pConfig);
}

static
DWORD
LsaPamGetConfigFromServer(
    OUT PLSA_PAM_CONFIG *ppConfig
    )
{
    DWORD dwError = 0;
    PLSA_PAM_CONFIG pConfig = NULL;
    HANDLE hLsaConnection = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetPamConfig(hLsaConnection, &pConfig);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (hLsaConnection != NULL)
    {
        LsaCloseServer(hLsaConnection);
        hLsaConnection = NULL;
    }

    *ppConfig = pConfig;

    return dwError;

error:
    if (pConfig)
    {
        LsaPamFreeConfig(pConfig);
        pConfig = NULL;
    }

    goto cleanup;
}

DWORD
LsaReadIgnoreLists()
{
    DWORD dwError = 0;
    time_t tCurrentTime = 0;
    PSTR pszUserIgnoreList = NULL;
    PSTR pszGroupIgnoreList = NULL;
    int iListFd = -1;
    size_t sOffset = 0;
    ssize_t ssRead = 0;
    struct stat fileStat = {0};

    if (time(&tCurrentTime) == (time_t)-1)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (tCurrentTime < gtIgnoreListLastUpdated +
            LSA_IGNORE_LIST_UPDATE_INTERVAL)
    {
        goto cleanup;
    }

    if ((iListFd = open(LSA_USER_IGNORE_LIST_PATH, O_RDONLY, 0)) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        if (dwError == LwMapErrnoToLwError(ENOENT) ||
            dwError == LwMapErrnoToLwError(ENOTDIR))
        {
            dwError = 0;
            goto cleanup;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (fstat(iListFd, &fileStat) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
        fileStat.st_size + 1,
        (PVOID*)&pszUserIgnoreList);
    BAIL_ON_LSA_ERROR(dwError);

    sOffset = 0;
    while (sOffset < fileStat.st_size)
    {
        ssRead = read(
            iListFd,
            pszUserIgnoreList,
            fileStat.st_size - sOffset);
        if (ssRead < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            if (dwError == LwMapErrnoToLwError(EINTR))
            {
                dwError = 0;
                ssRead = 0;
            }
            BAIL_ON_LSA_ERROR(dwError);
        }
        sOffset += ssRead;
    }
    pszUserIgnoreList[sOffset] = 0;

    if (iListFd != -1)
    {
        close(iListFd);
        iListFd = -1;
    }
    if (stat(LSA_GROUP_IGNORE_LIST_PATH, &fileStat) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        if (dwError == LwMapErrnoToLwError(ENOENT) ||
            dwError == LwMapErrnoToLwError(ENOTDIR))
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        if ((iListFd = open(LSA_GROUP_IGNORE_LIST_PATH, O_RDONLY, 0)) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }
        dwError = LwAllocateMemory(
                        fileStat.st_size + 1,
                        (PVOID*)&pszGroupIgnoreList);
        BAIL_ON_LSA_ERROR(dwError);

        sOffset = 0;
        while (sOffset < fileStat.st_size)
        {
            ssRead = read(
                        iListFd,
                        pszGroupIgnoreList,
                        fileStat.st_size - sOffset);
            if (ssRead < 0)
            {
                dwError = LwMapErrnoToLwError(errno);
                if (dwError == LwMapErrnoToLwError(EINTR))
                {
                    dwError = 0;
                    ssRead = 0;
                }
                BAIL_ON_LSA_ERROR(dwError);
            }
            sOffset += ssRead;
        }
        pszGroupIgnoreList[sOffset] = 0;
    }

    LW_SAFE_FREE_MEMORY(gpszUserIgnoreList);
    gpszUserIgnoreList = pszUserIgnoreList;
    LW_SAFE_FREE_MEMORY(gpszGroupIgnoreList);
    gpszGroupIgnoreList = pszGroupIgnoreList;

    gtIgnoreListLastUpdated = tCurrentTime;

cleanup:
    if (iListFd != -1)
    {
        close(iListFd);
        iListFd = -1;
    }
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pszUserIgnoreList);
    LW_SAFE_FREE_MEMORY(pszGroupIgnoreList);
    goto cleanup;
}

BOOLEAN
LsaShouldIgnoreGroup(
    PCSTR pszName
    )
{
    PCSTR pszEntryStart = NULL;
    PCSTR pszEntryEnd = NULL;
    PCSTR pszNextEntry = NULL;
    // Ignore errors
    LsaReadIgnoreLists();

    pszEntryStart = gpszGroupIgnoreList;
    while (pszEntryStart && pszEntryStart[0])
    {
        pszNextEntry = strchr(pszEntryStart, '\n');
        if (!pszNextEntry)
        {
            pszEntryEnd = pszEntryStart + strlen(pszEntryStart);
        }
        else if (pszNextEntry > pszEntryStart && pszNextEntry[-1] == '\r')
        {
            pszEntryEnd = pszNextEntry - 1;
            pszNextEntry++;
        }
        else
        {
            pszEntryEnd = pszNextEntry;
            pszNextEntry++;
        }

        if (pszEntryEnd - pszEntryStart == strlen(pszName) &&
                !strncmp(pszName, pszEntryStart, pszEntryEnd - pszEntryStart))
        {
            return 1;
        }
        pszEntryStart = pszNextEntry;
    }

    return 0;
}

BOOLEAN
LsaShouldIgnoreUser(
    PCSTR pszName
    )
{
    PCSTR pszEntryStart = NULL;
    PCSTR pszEntryEnd = NULL;
    PCSTR pszNextEntry = NULL;
    // Ignore errors
    LsaReadIgnoreLists();

    pszEntryStart = gpszUserIgnoreList;
    while (pszEntryStart && pszEntryStart[0])
    {
        pszNextEntry = strchr(pszEntryStart, '\n');
        if (!pszNextEntry)
        {
            pszEntryEnd = pszEntryStart + strlen(pszEntryStart);
        }
        else if (pszNextEntry > pszEntryStart && pszNextEntry[-1] == '\r')
        {
            pszEntryEnd = pszNextEntry - 1;
            pszNextEntry++;
        }
        else
        {
            pszEntryEnd = pszNextEntry;
            pszNextEntry++;
        }

        if (pszEntryEnd - pszEntryStart == strlen(pszName) &&
                !strncmp(pszName, pszEntryStart, pszEntryEnd - pszEntryStart))
        {
            return 1;
        }
        pszEntryStart = pszNextEntry;
    }

    return 0;
}

VOID
LsaFreeIgnoreLists(VOID)
{
    LW_SAFE_FREE_MEMORY(gpszUserIgnoreList);
    LW_SAFE_FREE_MEMORY(gpszGroupIgnoreList);
}
