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
    )
{
    DWORD dwError = 0;
    PSTR pszConfigFilePath = NULL;
    BOOLEAN bUnlockConfigLock = FALSE;
    PRSYS_SRV_API_CONFIG pAPIConfig = NULL;

    dwError = RSysSrvApiGetConfigFilePath(&pszConfigFilePath);
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = RSysSrvApiReadConfig(
                    pszConfigFilePath,
                    &pAPIConfig);
    if (dwError == ENOENT)
    {
        RSYS_LOG_ERROR("Unable to find configuration file [%s]", pszConfigFilePath);
    }
    BAIL_ON_RSYS_ERROR(dwError);

    pthread_rwlock_wrlock(&gRSysConfigLock);
    bUnlockConfigLock = TRUE;

    RSysSrvApiFreeConfig(gpAPIConfig);
    gpAPIConfig = pAPIConfig;
    pAPIConfig = NULL;
    dwError = RSysSrvSetLogByConfig();
    BAIL_ON_RSYS_ERROR(dwError);

cleanup:

    RtlCStringFree(&pszConfigFilePath);

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
RSysSrvApiGetConfigFilePath(
    PSTR* ppszConfigFilePath
    )
{
    DWORD dwError = 0;
    PSTR  pszConfigFilePath = NULL;
    BOOLEAN bUnlockConfigLock = FALSE;

    pthread_rwlock_rdlock(&gRSysConfigLock);
    bUnlockConfigLock = TRUE;

    BAIL_ON_INVALID_STRING(gpszConfigFilePath);

    dwError = RtlCStringDuplicate(
                    &pszConfigFilePath,
                    gpszConfigFilePath);
    BAIL_ON_RSYS_ERROR(dwError);

    *ppszConfigFilePath = pszConfigFilePath;

cleanup:

    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gRSysConfigLock);
    }

    return dwError;

error:

    *ppszConfigFilePath = NULL;

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

    dwError = RTL_ALLOCATE(
            &pConfig->pLogInfo,
            RSYS_LOG_INFO,
            sizeof(*pConfig->pLogInfo));
    BAIL_ON_RSYS_ERROR(dwError);

    pConfig->pLogInfo->maxAllowedLogLevel = RSYS_LOG_LEVEL_ERROR;
    pConfig->pLogInfo->logTarget = RSYS_LOG_TARGET_CONSOLE;
    pConfig->pLogInfo->pszPath = NULL;

    pConfig->dwEscrowTime = 1 * 1000000;

    *ppConfig = pConfig;

cleanup:
    return dwError;

error:
    *ppConfig = NULL;
    RSysSrvApiFreeConfig(pConfig);
    goto cleanup;
}

DWORD
RSysSrvApiNewPattern(
    PRSYS_CONFIG_SETTINGS pSettings,
    PCSTR pszSectionName
    )
{
    DWORD dwError = 0;
    PRSYS_SRV_API_CONFIG pConfig = (PRSYS_SRV_API_CONFIG)pSettings->pvUserData;
    RSYS_MESSAGE_PATTERN* pPattern = NULL;

    if (!strcmp(pszSectionName, "pattern"))
    {
        pPattern = RtlMemoryAllocate(sizeof(*pPattern));
        if (pPattern == NULL)
        {
            dwError = ENOMEM;
            BAIL_ON_RSYS_ERROR(dwError);
        }

        dwError = RSysDLinkedListAppend(
                        &pConfig->pPatternTail,
                        pPattern);
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
    }

cleanup:
    return dwError;

error:
    LW_RTL_FREE(&pPattern);
    goto cleanup;
}

DWORD
RSysSrvApiPatternValue(
    PRSYS_CONFIG_SETTINGS pSettings,
    PCSTR pszName,
    PCSTR pszValue
    )
{
    DWORD dwError = 0;
    // do not free
    LW_ANSI_STRING strValue;
    PRSYS_SRV_API_CONFIG pConfig = (PRSYS_SRV_API_CONFIG)pSettings->pvUserData;
    RSYS_MESSAGE_PATTERN* pPattern = NULL;

    if (strcmp(pSettings->pszCurrentSection, "pattern"))
    {
        dwError = EINVAL;
        BAIL_ON_RSYS_ERROR(dwError);
    }

    pPattern = pConfig->pPatternTail->pItem;

    strValue.Length = strlen(pszValue);
    strValue.MaximumLength = strValue.Length;
    strValue.Buffer = (PSTR)pszValue;

    if (!strcmp(pszName, "id"))
    {
        dwError = LwRtlAnsiStringParseULONG(
                        &pPattern->ulId,
                        &strValue,
                        &strValue);
        BAIL_ON_RSYS_ERROR(dwError);
    }
    else if (!strcmp(pszName, "type"))
    {
        dwError = RtlCStringDuplicate(
                        &pPattern->pszEventType,
                        pszValue);
        BAIL_ON_RSYS_ERROR(dwError);
    }
    else if (!strcmp(pszName, "regex"))
    {
        dwError = RtlCStringDuplicate(
                        &pPattern->pszRawMessageRegEx,
                        pszValue);
        BAIL_ON_RSYS_ERROR(dwError);
    }
    else if (!strcmp(pszName, "username-index"))
    {
        dwError = LwRtlAnsiStringParseULONG(
                        &pPattern->ulUserMatchIndex,
                        &strValue,
                        &strValue);
        BAIL_ON_RSYS_ERROR(dwError);
    }
    else if (!strcmp(pszName, "user-type"))
    {
        if (!strcasecmp(pszValue, "ad"))
        {
            pPattern->filter = RSYS_AD_USER;
        }
        else if (!strcasecmp(pszValue, "local"))
        {
            pPattern->filter = RSYS_LOCAL_USER;
        }
        else
        {
            pPattern->filter = RSYS_ANY_USER;
        }
    }
    else
    {
        errno = EINVAL;
        BAIL_ON_RSYS_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
RSysSrvApiReadConfig(
    PCSTR pszConfigFilePath,
    PRSYS_SRV_API_CONFIG *ppConfig
    )
{
    DWORD dwError = 0;
    PRSYS_SRV_API_CONFIG pConfig = NULL;

    BAIL_ON_INVALID_STRING(pszConfigFilePath);

    dwError = RSysSrvApiInitConfig(&pConfig);
    BAIL_ON_RSYS_ERROR(dwError);

    {
        const PCSTR ppszLogLevelNames[] = {
            "always",
            "error",
            "warning",
            "info",
            "verbose",
            "debug"
        };
        const PCSTR ppszLogTargetNames[] = {
            "disabled",
            "console",
            "file",
            "syslog",
        };
        RSYS_CONFIG_SETTING settingsArray[] = {
            {
                "logging",
                "log-level",
                Enum,
                RSYS_LOG_LEVEL_ALWAYS,
                RSYS_LOG_LEVEL_DEBUG,
                ppszLogLevelNames,
                &pConfig->pLogInfo->maxAllowedLogLevel,
            },
            {
                "logging",
                "log-target",
                Enum,
                RSYS_LOG_TARGET_DISABLED,
                RSYS_LOG_TARGET_SYSLOG,
                ppszLogTargetNames,
                &pConfig->pLogInfo->logTarget,
            },
            {
                "logging",
                "log-path",
                String,
                0,
                -1,
                NULL,
                &pConfig->pLogInfo->pszPath,
            },
            {
                "global",
                "escrow-microseconds",
                Dword,
                0,
                -1,
                NULL,
                &pConfig->dwEscrowTime,
            },
        };
        RSYS_CONFIG_SETTINGS settings = {
            sizeof(settingsArray)/sizeof(settingsArray[0]),
            settingsArray,
            RSysSrvApiNewPattern,
            RSysSrvApiPatternValue,
            pConfig
        };

        dwError = RSysParseConfigFile(
                        pszConfigFilePath,
                        RSYS_CFG_OPTION_STRIP_ALL,
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
        RSysFreeLogInfo(pConfig->pLogInfo);

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
