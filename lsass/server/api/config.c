/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Configuration API (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "api.h"


DWORD
LsaSrvRefreshConfiguration(
    HANDLE hServer
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    BOOLEAN bUnlockConfigLock = FALSE;
    LSA_SRV_API_CONFIG apiConfig;

    if (pServerState->peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvApiInitConfig(&apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiReadRegistry(&apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_lock(&gAPIConfigLock);
    bUnlockConfigLock = TRUE;

    dwError = LsaSrvApiTransferConfigContents(
                    &apiConfig,
                    &gAPIConfig);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_unlock(&gAPIConfigLock);
    bUnlockConfigLock = FALSE;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      NULL,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnRefreshConfiguration(
                                        hProvider);
        if (dwError)
        {
            LSA_LOG_ERROR("Refreshing provider %s failed.",
                          pProvider->pszName ? pProvider->pszName : "");
            dwError = 0;
        }

        LsaSrvCloseProvider(pProvider, hProvider);
        hProvider = (HANDLE)NULL;
    }

cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LsaSrvApiFreeConfigContents(&apiConfig);

    if (bUnlockConfigLock)
    {
        pthread_mutex_unlock(&gAPIConfigLock);
    }

    return(dwError);

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "refresh configuration");

    goto cleanup;

}

DWORD
LsaSrvApiInitConfig(
    PLSA_SRV_API_CONFIG pConfig
    )
{
    LsaSrvApiFreeConfigContents(pConfig);

    pConfig->bEnableEventLog = FALSE;
    pConfig->cDomainSeparator = '\\';
    pConfig->cSpaceReplacement = '^';

    return 0;
}

DWORD
LsaSrvApiReadRegistry(
    PLSA_SRV_API_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    LSA_SRV_API_CONFIG StagingConfig;
    PSTR pszDomainSeparator = NULL;
    PSTR pszSpaceReplacement = NULL;
    LWREG_CONFIG_ITEM Config[] =
    {
        {
           "EnableEventlog",
           TRUE,
           LsaTypeBoolean,
           0,
           MAXDWORD,
           NULL,
           &StagingConfig.bEnableEventLog,
           NULL
        },
        {
           "DomainSeparator",
           TRUE,
           LsaTypeString,
           0,
           MAXDWORD,
           NULL,
           &pszDomainSeparator
        },
        {
           "SpaceReplacement",
           TRUE,
           LsaTypeString,
           0,
           MAXDWORD,
           NULL,
           &pszSpaceReplacement
        },
    };

    memset(&StagingConfig, 0, sizeof(StagingConfig));
    dwError = LsaSrvApiInitConfig(&StagingConfig);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = RegProcessConfig(
                "Services\\lsass\\Parameters",
                "Policy\\Services\\lsass\\Parameters",
                Config,
                sizeof(Config)/sizeof(Config[0]));
    BAIL_ON_LSA_ERROR(dwError);

    if (pszDomainSeparator && strlen(pszDomainSeparator) == 1)
    {
        StagingConfig.cDomainSeparator = pszDomainSeparator[0];
    }
    if (pszSpaceReplacement && strlen(pszSpaceReplacement) == 1)
    {
        StagingConfig.cSpaceReplacement = pszSpaceReplacement[0];
    }

    dwError = LsaSrvApiTransferConfigContents(
                    &StagingConfig,
                    pConfig);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LsaSrvApiFreeConfigContents(&StagingConfig);
    LW_SAFE_FREE_STRING(pszDomainSeparator);
    LW_SAFE_FREE_STRING(pszSpaceReplacement);

    return dwError;

error:

    goto cleanup;
}

#if 0
static
DWORD
ValidateAndSetLogLevel(
    PCSTR               pszName,
    PCSTR               pszValue
    )
{
    DWORD dwError = 0;

    LSA_LOG_INFO LogInfo = {};
    
    if (!strcasecmp(pszValue, "error"))
    {
        LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_ERROR;
    }
    else if (!strcasecmp(pszValue, "warning"))
    {
        LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_WARNING;
    }
    else if (!strcasecmp(pszValue, "info"))
    {
        LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_INFO;
    }
    else if (!strcasecmp(pszValue, "verbose"))
    {
        LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_VERBOSE;
    }
    else if (!strcasecmp(pszValue, "debug"))
    {
        LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_DEBUG;
    }
    else if (!strcasecmp(pszValue, "trace"))
    {
        LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_TRACE;
    }
    else
    {
        LSA_LOG_ERROR("Invalid value for %s.",
                      pszName);
        dwError = LW_ERROR_INVALID_LOG_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLogSetInfo_r(&LogInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}
#endif

DWORD
LsaSrvApiTransferConfigContents(
    PLSA_SRV_API_CONFIG pSrc,
    PLSA_SRV_API_CONFIG pDest
    )
{
    LsaSrvApiFreeConfigContents(pDest);

    *pDest = *pSrc;

    LsaSrvApiFreeConfigContents(pSrc);

    return 0;
}

VOID
LsaSrvApiFreeConfigContents(
    PLSA_SRV_API_CONFIG pConfig
    )
{
    // Nothing to free right now
    memset(pConfig, 0, sizeof(*pConfig));
}

BOOLEAN
LsaSrvEventlogEnabled(
    VOID
    )
{
    BOOLEAN bResult = FALSE;

    pthread_mutex_lock(&gAPIConfigLock);

    bResult = gAPIConfig.bEnableEventLog;

    pthread_mutex_unlock(&gAPIConfigLock);

    return bResult;
}

VOID
LsaSrvEnableEventlog(
    BOOLEAN bValue
    )
{
    pthread_mutex_lock(&gAPIConfigLock);

    gAPIConfig.bEnableEventLog = bValue;

    pthread_mutex_unlock(&gAPIConfigLock);
}

char
LsaSrvSpaceReplacement(
    VOID
    )
{
    char cResult = 0;

    pthread_mutex_lock(&gAPIConfigLock);

    cResult = gAPIConfig.cSpaceReplacement;

    pthread_mutex_unlock(&gAPIConfigLock);

    return cResult;
}

char
LsaSrvDomainSeparator(
    VOID
    )
{
    char cResult = 0;

    pthread_mutex_lock(&gAPIConfigLock);

    cResult = gAPIConfig.cDomainSeparator;

    pthread_mutex_unlock(&gAPIConfigLock);

    return cResult;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
