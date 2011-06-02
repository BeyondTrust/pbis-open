/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        config.c
 *
 * Abstract:
 *
 *        Reaper for syslog
 *
 *        Configuration API (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
RSysSrvRefreshConfiguration(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bUnlockConfigLock = FALSE;
    PRSYS_SRV_API_CONFIG pAPIConfig = NULL;

    dwError = RSysSrvApiReadConfig(&pAPIConfig);
    BAIL_ON_RSYS_ERROR(dwError);

    pthread_rwlock_wrlock(&gRSysConfigLock);
    bUnlockConfigLock = TRUE;

    RSysSrvApiFreeConfig(gpAPIConfig);
    gpAPIConfig = pAPIConfig;
    pAPIConfig = NULL;

cleanup:

    RSysSrvApiFreeConfig(pAPIConfig);

    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gRSysConfigLock);
    }

    return(dwError);

error:
    goto cleanup;
}

DWORD
RSysSrvApiInitConfig(
    PRSYS_SRV_API_CONFIG *ppConfig
    )
{
    PRSYS_SRV_API_CONFIG pConfig = NULL;
    DWORD dwError = 0;

    dwError = RTL_ALLOCATE(
                    &pConfig,
                    RSYS_SRV_API_CONFIG,
                    sizeof(*pConfig));
    BAIL_ON_RSYS_ERROR(dwError);

    pConfig->dwEscrowTime = 1 * 1000000;
    pConfig->bLogUnmatchedErrorEvents = FALSE;
    pConfig->bLogUnmatchedWarningEvents = FALSE;
    pConfig->bLogUnmatchedInfoEvents = FALSE;

    *ppConfig = pConfig;

cleanup:
    return dwError;

error:
    *ppConfig = NULL;
    RSysSrvApiFreeConfig(pConfig);
    goto cleanup;
}

DWORD
RSysSrvApiReadConfig(
    PRSYS_SRV_API_CONFIG *ppConfig
    )
{
    DWORD dwError = 0;
    PRSYS_SRV_API_CONFIG pConfig = NULL;

    dwError = RSysSrvApiInitConfig(&pConfig);
    BAIL_ON_RSYS_ERROR(dwError);

    {
        RSYS_CONFIG_SETTING settingsArray[] = {
            {
                "global",
                "EscrowMicroseconds",
                TRUE,
                Dword,
                0,
                -1,
                NULL,
                &pConfig->dwEscrowTime,
            },
            {
                "global",
                "LogUnmatchedErrorEvents",
                TRUE,
                Boolean,
                0,
                1,
                NULL,
                &pConfig->bLogUnmatchedErrorEvents,
            },
            {
                "global",
                "LogUnmatchedWarningEvents",
                TRUE,
                Boolean,
                0,
                1,
                NULL,
                &pConfig->bLogUnmatchedWarningEvents,
            },
            {
                "global",
                "LogUnmatchedInfoEvents",
                TRUE,
                Boolean,
                0,
                1,
                NULL,
                &pConfig->bLogUnmatchedInfoEvents,
            },
        };

        dwError = RSysProcessConfig(
                    "Services\\reapsysl\\Parameters",
                    "Policy\\Services\\reapsysl\\Parameters",
                    settingsArray,
                    sizeof(settingsArray)/sizeof(settingsArray[0]));
        BAIL_ON_RSYS_ERROR(dwError);

        RSYS_CONFIG_SETTINGS settings = {
            sizeof(settingsArray)/sizeof(settingsArray[0]),
            settingsArray,
            pConfig,
        };

        dwError = GetPatternListFromRegistry(
                    "Services\\reapsysl\\Parameters\\Pattern",
                    &settings);
        BAIL_ON_RSYS_ERROR(dwError);
    }

    *ppConfig = pConfig;

cleanup:

    return dwError;

error:
    RSysSrvApiFreeConfig(pConfig);
    *ppConfig = NULL;

    goto cleanup;
}

VOID
RSysSrvApiFreeConfig(
    PRSYS_SRV_API_CONFIG pConfig
    )
{
    if (pConfig)
    {
        while (pConfig->pPatternHead)
        {
            PDLINKEDLIST pToDelete = pConfig->pPatternHead;
            RSYS_MESSAGE_PATTERN* pPattern = pToDelete->pItem;

            pConfig->pPatternHead = pConfig->pPatternHead->pNext;

            if (pPattern->bCompiled)
            {
                regfree(&pPattern->compiledRegEx);
            }
            RtlCStringFree(&pPattern->pszEventType);
            RtlCStringFree(&pPattern->pszRawMessageRegEx);
            LW_RTL_FREE(&pToDelete);
        }

        RSYS_SAFE_FREE_MEMORY(pConfig);
    }
}

DWORD
RSysSrvGetEscrowTime(
    HANDLE hServer,
    DWORD* pdwEscrowTime
    )
{
    DWORD dwError = 0;
    BOOLEAN bUnlockConfigLock = FALSE;

    BAIL_ON_INVALID_POINTER(pdwEscrowTime);

    pthread_rwlock_rdlock(&gRSysConfigLock);
    bUnlockConfigLock = TRUE;

    *pdwEscrowTime = gpAPIConfig->dwEscrowTime;

cleanup:
    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gRSysConfigLock);
    }

    return dwError;

error:

    *pdwEscrowTime = 0;

    goto cleanup;
}

DWORD
RSysSrvGetLogUnmatchedEvents(
    HANDLE hServer,
    BOOLEAN* pbLogUnmatchedErrorEvents,
    BOOLEAN* pbLogUnmatchedWarningEvents,
    BOOLEAN* pbLogUnmatchedInfoEvents
    )
{
    DWORD dwError = 0;
    BOOLEAN bUnlockConfigLock = FALSE;

    BAIL_ON_INVALID_POINTER(pbLogUnmatchedErrorEvents);
    BAIL_ON_INVALID_POINTER(pbLogUnmatchedWarningEvents);
    BAIL_ON_INVALID_POINTER(pbLogUnmatchedInfoEvents);

    pthread_rwlock_rdlock(&gRSysConfigLock);
    bUnlockConfigLock = TRUE;

    *pbLogUnmatchedErrorEvents = gpAPIConfig->bLogUnmatchedErrorEvents;
    *pbLogUnmatchedWarningEvents = gpAPIConfig->bLogUnmatchedWarningEvents;
    *pbLogUnmatchedInfoEvents = gpAPIConfig->bLogUnmatchedInfoEvents;

cleanup:
    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gRSysConfigLock);
    }

    return dwError;

error:

    *pbLogUnmatchedErrorEvents = FALSE;
    *pbLogUnmatchedWarningEvents = FALSE;
    *pbLogUnmatchedInfoEvents = FALSE;

    goto cleanup;
}

DWORD
RSysSrvLockPatternList(
    HANDLE hServer,
    PDLINKEDLIST* ppPatternList
    )
{
    DWORD dwError = 0;
    BOOLEAN bUnlockConfigLock = FALSE;

    BAIL_ON_INVALID_POINTER(ppPatternList);

    pthread_rwlock_rdlock(&gRSysConfigLock);
    bUnlockConfigLock = TRUE;

    *ppPatternList = gpAPIConfig->pPatternHead;

cleanup:
    return dwError;

error:
    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gRSysConfigLock);
    }
    if (ppPatternList)
    {
        *ppPatternList = 0;
    }

    goto cleanup;
}

DWORD
RSysSrvUnlockPatternList(
    HANDLE hServer,
    PDLINKEDLIST pPatternList
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pPatternList);

    gpAPIConfig->pPatternHead = pPatternList;

    dwError = pthread_rwlock_unlock(&gRSysConfigLock);
    BAIL_ON_RSYS_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
GetPatternListFromRegistry(
    PSTR pszKeyPath,
    RSYS_CONFIG_SETTINGS* pConfigSettings
    )
{
    DWORD dwError = 0;
    HANDLE hReg = (HANDLE)NULL;
    HKEY pRootKey = NULL;
    HKEY pNewKey = NULL;
    DWORD dwSubKeyCount = 0;
    DWORD dwSubKeyLen = 0;
    DWORD dwValuesCount = 0;
    DWORD dwMaxSubKeyLen = 0;
    PSTR pszKeyName = NULL;
    PSTR pszFullKeyName = NULL;
    int i =0;
    PRSYS_SRV_API_CONFIG pConfig = (PRSYS_SRV_API_CONFIG)pConfigSettings->pvUserData;
    RSYS_MESSAGE_PATTERN* pPattern = NULL;
    PSTR pszParentKeyPath = NULL;

    dwError = RegOpenServer(&hReg);
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = RegOpenKeyExA(
                (HANDLE) hReg,
                NULL,
                HKEY_THIS_MACHINE,
                (DWORD) 0,
                (REGSAM) KEY_ALL_ACCESS,
                (PHKEY) &pRootKey
                );
    if (dwError)
    {
        RSYS_LOG_ERROR( "Unable to open registry root key %s",HKEY_THIS_MACHINE);
        goto error;
    }

    dwError = RegOpenKeyExA(
                (HANDLE) hReg,
                (HKEY) pRootKey,
                pszKeyPath,
                (DWORD) 0,
                (REGSAM) KEY_ALL_ACCESS,
                (PHKEY) &pNewKey
                );

    if (dwError)
    {
        RSYS_LOG_ERROR( "Unable to open registry key %s",pszKeyPath);
        goto error;
    }

    dwError = RegQueryInfoKeyA(
                hReg,
                pNewKey,
                NULL,
                NULL,
                NULL,
                &dwSubKeyCount,
                &dwMaxSubKeyLen,
                NULL,
                &dwValuesCount,
                NULL,
                NULL,
                NULL,
                NULL);
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszParentKeyPath,
                    "%s\\%s",
                    HKEY_THIS_MACHINE,
                    pszKeyPath);
    BAIL_ON_RSYS_ERROR(dwError);

    for(i=0; i<dwSubKeyCount; i++)
    {
        dwSubKeyLen = dwMaxSubKeyLen+1;

        dwError = LwAllocateMemory(dwSubKeyLen*sizeof(*pszKeyName),
                                   (PVOID)&pszKeyName);
        BAIL_ON_RSYS_ERROR(dwError);

        dwError = RegEnumKeyExA((HANDLE)hReg,
                                pNewKey,
                                i,
                                pszKeyName,
                                &dwSubKeyLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        BAIL_ON_RSYS_ERROR(dwError);

        dwError = LwAllocateStringPrintf(
                        &pszFullKeyName,
                        "%s\\%s",
                        pszParentKeyPath,
                        pszKeyName);
        BAIL_ON_RSYS_ERROR(dwError);

        dwError = GetValuesFromRegistry(pszFullKeyName,&pPattern);
        BAIL_ON_RSYS_ERROR(dwError);

        dwError = RSysDLinkedListAppend(&pConfig->pPatternTail, pPattern);
        BAIL_ON_RSYS_ERROR(dwError);

        if (!pConfig->pPatternHead)
        {
            pConfig->pPatternHead = pConfig->pPatternTail;
        }
        else
        {
            pConfig->pPatternTail = pConfig->pPatternTail->pNext;
        }

        // The pattern is now owned by the list
        pPattern = NULL;
        LW_SAFE_FREE_STRING(pszKeyName);
        pszKeyName = NULL;
        LW_SAFE_FREE_STRING(pszFullKeyName);
        pszFullKeyName = NULL;
    }

cleanup:

    RegCloseKey(hReg, pNewKey);
    pNewKey = NULL;

    RegCloseKey(hReg, pRootKey);
    pRootKey = NULL;

    RegCloseServer(hReg);
    hReg = NULL;

    LW_SAFE_FREE_STRING(pszKeyName);
    LW_SAFE_FREE_STRING(pszFullKeyName);
    LW_SAFE_FREE_STRING(pszParentKeyPath);

    return(dwError);

error:
    goto cleanup;
}

DWORD
GetValuesFromRegistry( 
    PSTR pszKeyName,
    RSYS_MESSAGE_PATTERN** ppPattern
    )
{
    DWORD dwError = 0;
    PSTR pszToken = NULL;
    PSTR pszPath = NULL;
    PSTR pszAbsoluteKey = NULL;
    RSYS_MESSAGE_PATTERN* pPattern = NULL;

    dwError = LwNtStatusToWin32Error(
                  LW_RTL_ALLOCATE(
                      &pPattern,
                      RSYS_MESSAGE_PATTERN,
                      sizeof(*pPattern)));
    BAIL_ON_RSYS_ERROR(dwError);

    //Now read all the parameters and store it accordingly
    RSYS_CONFIG_SETTING ConfigDescription[] =
    {
        {
            "Pattern",
            "Type",
            FALSE,
            String,
            0,
            -1,
            NULL,
            &(pPattern->pszEventType)
        },
        {
            "Pattern",
            "Regex",
            FALSE,
            String,
            0,
            -1,
            NULL,
            &(pPattern->pszRawMessageRegEx)
        },
        {
            "Pattern",
            "UserNameIndex",
            FALSE,
            Dword,
            0,
            -1,
            NULL,
            &(pPattern->ulUserMatchIndex)
        },
        {
            "Pattern",
            "UserType",
            FALSE,
            Dword,
            0,
            -1,
            NULL,
            &(pPattern->filter)
        },
        {
            "Pattern",
            "Event",
            FALSE,
            Dword,
            0,
            -1,
            NULL,
            &(pPattern->ulId)
        },
    };

    //Read from registry
    dwError = LwAllocateStringPrintf(&pszAbsoluteKey, "%s", pszKeyName);
    BAIL_ON_RSYS_ERROR(dwError);

    pszToken = (char*)strtok_r(pszAbsoluteKey,"\\",&pszPath);

    dwError = RSysProcessConfig(
                pszPath,
                pszPath,
                ConfigDescription,
                sizeof(ConfigDescription)/sizeof(ConfigDescription[0]));

    BAIL_ON_RSYS_ERROR(dwError);

    *ppPattern = pPattern;

cleanup:
    LW_SAFE_FREE_STRING(pszAbsoluteKey);
    return dwError;
error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
