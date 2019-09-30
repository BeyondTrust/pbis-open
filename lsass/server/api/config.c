/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        config.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
    pConfig->dwSaslMaxBufSize = 16777215;  // 16MB
    pConfig->cDomainSeparator = '\\';
    pConfig->cSpaceReplacement = '^';
    pConfig->bEnableSmartCard = FALSE;
    pConfig->bEnableRemoteSmartCard = FALSE;

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
           LwRegTypeBoolean,
           0,
           MAXDWORD,
           NULL,
           &StagingConfig.bEnableEventLog,
           NULL
        },
        {
           "DomainSeparator",
           TRUE,
           LwRegTypeString,
           0,
           MAXDWORD,
           NULL,
           &pszDomainSeparator
        },
        {
           "SpaceReplacement",
           TRUE,
           LwRegTypeString,
           0,
           MAXDWORD,
           NULL,
           &pszSpaceReplacement
        },
        {
           "SaslMaxBufSize",
           TRUE,
           LwRegTypeDword,
           1048575,   /* Minimum is 1MB */
           16777215,  /* Maximum is 16MB bytes which is the default value in OpenLDAP. */
           NULL,
           &StagingConfig.dwSaslMaxBufSize,
           NULL
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
    BOOLEAN bEnableSmartCard = pDest->bEnableSmartCard;
    BOOLEAN bEnableRemoteSmartCard = pDest->bEnableRemoteSmartCard;
    
    LsaSrvApiFreeConfigContents(pDest);

    *pDest = *pSrc;

    LsaSrvApiFreeConfigContents(pSrc);

    pDest->bEnableSmartCard = bEnableSmartCard;
    pDest->bEnableRemoteSmartCard = bEnableRemoteSmartCard;

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

DWORD
LsaSrvSaslMaxBufSize(VOID)
{
    DWORD dwResult = 16777215;    // 16MB

    pthread_mutex_lock(&gAPIConfigLock);

    dwResult = gAPIConfig.dwSaslMaxBufSize;

    pthread_mutex_unlock(&gAPIConfigLock);

    return dwResult;
}

BOOLEAN
LsaSrvSmartCardEnabled(VOID)
{
    BOOLEAN rv = FALSE;

    pthread_mutex_lock(&gAPIConfigLock);

    rv = gAPIConfig.bEnableSmartCard;

    pthread_mutex_unlock(&gAPIConfigLock);

    return rv;
}

VOID
LsaSrvSmartCardEnable(BOOLEAN v)
{
    pthread_mutex_lock(&gAPIConfigLock);

    gAPIConfig.bEnableSmartCard = v;

    pthread_mutex_unlock(&gAPIConfigLock);
}

BOOLEAN
LsaSrvRemoteSmartCardEnabled(VOID)
{
    BOOLEAN rv = FALSE;

    pthread_mutex_lock(&gAPIConfigLock);

    rv = gAPIConfig.bEnableRemoteSmartCard;

    pthread_mutex_unlock(&gAPIConfigLock);

    return rv;
}

VOID
LsaSrvRemoteSmartCardEnable(BOOLEAN v)
{
    pthread_mutex_lock(&gAPIConfigLock);

    gAPIConfig.bEnableRemoteSmartCard = v;

    pthread_mutex_unlock(&gAPIConfigLock);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
