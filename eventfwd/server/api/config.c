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
 *        Event forwarder from eventlogd to collector service
 *
 *        Configuration API (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
EfdSrvRefreshConfiguration(
    )
{
    DWORD dwError = 0;
    BOOLEAN bUnlockConfigLock = FALSE;
    PEFD_SRV_API_CONFIG pAPIConfig = NULL;

    dwError = EfdSrvApiReadConfig(&pAPIConfig);
    BAIL_ON_EFD_ERROR(dwError);

    pthread_rwlock_wrlock(&gEfdConfigLock);
    bUnlockConfigLock = TRUE;

    EfdSrvApiFreeConfig(gpAPIConfig);
    gpAPIConfig = pAPIConfig;
    pAPIConfig = NULL;
    dwError = EfdSrvSetLogByConfig();
    BAIL_ON_EFD_ERROR(dwError);

    dwError = EfdSrvPollerRefresh();
    if (dwError == ESRCH)
    {
        // The thread may not be running yet
        dwError = 0;
    }
    BAIL_ON_EFD_ERROR(dwError);

cleanup:

    EfdSrvApiFreeConfig(pAPIConfig);

    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gEfdConfigLock);
    }

    return(dwError);

error:
    goto cleanup;
}

DWORD
EfdSrvApiInitConfig(
    PEFD_SRV_API_CONFIG *ppConfig
    )
{
    PEFD_SRV_API_CONFIG pConfig = NULL;
    DWORD dwError = 0;

    dwError = RTL_ALLOCATE(
                    &pConfig,
                    EFD_SRV_API_CONFIG,
                    sizeof(*pConfig));
    BAIL_ON_EFD_ERROR(dwError);

    dwError = RTL_ALLOCATE(
            &pConfig->pLogInfo,
            EFD_LOG_INFO,
            sizeof(*pConfig->pLogInfo));
    BAIL_ON_EFD_ERROR(dwError);

    pConfig->pLogInfo->maxAllowedLogLevel = EFD_LOG_LEVEL_ERROR;
    pConfig->pLogInfo->logTarget = EFD_LOG_TARGET_CONSOLE;
    pConfig->pLogInfo->pszPath = NULL;

    *ppConfig = pConfig;

cleanup:
    return dwError;

error:
    *ppConfig = NULL;
    EfdSrvApiFreeConfig(pConfig);
    goto cleanup;
}

DWORD
EfdSrvApiReadConfig(
    PEFD_SRV_API_CONFIG *ppConfig
    )
{
    DWORD dwError = 0;
    PEFD_SRV_API_CONFIG pConfig = NULL;

    dwError = EfdSrvApiInitConfig(&pConfig);
    BAIL_ON_EFD_ERROR(dwError);

    dwError = EfdReadEventFwdConfigSettings(pConfig);
    BAIL_ON_EFD_ERROR(dwError);
    
    EFD_LOG_VERBOSE("Collector = %s\nServicePrincipal = %s\n", 
            pConfig->pszCollector, pConfig->pszServicePrincipal);

    *ppConfig = pConfig;

cleanup:

    return dwError;

error:
    EfdSrvApiFreeConfig(pConfig);
    *ppConfig = NULL;

    goto cleanup;
}

VOID
EfdSrvApiFreeConfig(
    PEFD_SRV_API_CONFIG pConfig
    )
{
    if (pConfig)
    {
        EfdFreeLogInfo(pConfig->pLogInfo);
        RtlCStringFree(&pConfig->pszCollector);
        RtlCStringFree(&pConfig->pszServicePrincipal);
        EFD_SAFE_FREE_MEMORY(pConfig);
    }
}

DWORD
EfdSrvGetCollectorAddress(
    HANDLE hServer,
    PSTR *ppszCollector
    )
{
    DWORD dwError = 0;
    PSTR pszCollector = NULL;
    BOOLEAN bUnlockConfigLock = FALSE;

    BAIL_ON_INVALID_POINTER(ppszCollector);

    pthread_rwlock_rdlock(&gEfdConfigLock);
    bUnlockConfigLock = TRUE;

    if (!gpAPIConfig->pszCollector)
    {
        dwError = LW_STATUS_NOT_FOUND;
        BAIL_ON_EFD_ERROR(dwError);
    }
    dwError = RtlCStringDuplicate(
                    &pszCollector,
                    gpAPIConfig->pszCollector);
    BAIL_ON_EFD_ERROR(dwError);

    *ppszCollector = pszCollector;

cleanup:
    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gEfdConfigLock);
    }

    return dwError;

error:

    *ppszCollector = NULL;

    RtlCStringFree(&pszCollector);

    goto cleanup;
}

DWORD
EfdSrvGetCollectorServicePrincipal(
    HANDLE hServer,
    PSTR *ppszPrincipal
    )
{
    DWORD dwError = 0;
    PSTR pszPrincipal = NULL;
    BOOLEAN bUnlockConfigLock = FALSE;

    BAIL_ON_INVALID_POINTER(ppszPrincipal);

    pthread_rwlock_rdlock(&gEfdConfigLock);
    bUnlockConfigLock = TRUE;

    if (!gpAPIConfig->pszServicePrincipal)
    {
        dwError = LW_STATUS_NOT_FOUND;
        BAIL_ON_EFD_ERROR(dwError);
    }
    dwError = RtlCStringDuplicate(
                    &pszPrincipal,
                    gpAPIConfig->pszServicePrincipal);
    BAIL_ON_EFD_ERROR(dwError);

    *ppszPrincipal = pszPrincipal;

cleanup:
    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gEfdConfigLock);
    }

    return dwError;

error:

    *ppszPrincipal = NULL;

    RtlCStringFree(&pszPrincipal);

    goto cleanup;
}
static PSTR pszCollector = NULL;
static PSTR pszCollectorPrincipal = NULL;

static EFD_CONFIG_SETTING gConfigDescription[] =
{
    {
        NULL,
        "Collector",
        String,
        0,
        -1,
        NULL,
        &pszCollector,
        1
    },
    {
        NULL,
        "CollectorPrincipal",
        String,
        0,
        -1,
        NULL,
        &pszCollectorPrincipal,
        1
    }
};

DWORD
EfdReadEventFwdConfigSettings(PEFD_SRV_API_CONFIG pConfig)
{
    DWORD dwError = 0;

    EFD_LOG_INFO("Read Eventlog configuration settings");

    dwError = EfdProcessConfig(
                "Services\\eventfwd\\Parameters",
                "Policy\\Services\\eventfwd\\Parameters",
                gConfigDescription,
                sizeof(gConfigDescription)/sizeof(gConfigDescription[0]));
    
    BAIL_ON_EFD_ERROR(dwError);
    
    if(pszCollector)
    {
        dwError = EfdAllocateString(pszCollector, &pConfig->pszCollector);
        BAIL_ON_EFD_ERROR(dwError);
    }
    
    if(pszCollectorPrincipal)
    {
        dwError = EfdAllocateString(pszCollectorPrincipal, &pConfig->pszServicePrincipal);
        BAIL_ON_EFD_ERROR(dwError);
    }

error:
    EFD_SAFE_FREE_STRING(pszCollector);
    EFD_SAFE_FREE_STRING(pszCollectorPrincipal);
    return dwError;
}

