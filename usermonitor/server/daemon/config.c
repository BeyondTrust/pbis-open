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
 *        User monitor service for local users and groups
 *
 *        Configuration API (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
UmnSrvRefreshConfiguration(
    )
{
    DWORD dwError = 0;
    BOOLEAN bUnlockConfigLock = FALSE;
    PUMN_SRV_API_CONFIG pAPIConfig = NULL;

    dwError = UmnSrvReadConfig(&pAPIConfig);
    BAIL_ON_UMN_ERROR(dwError);

    pthread_rwlock_wrlock(&gUmnConfigLock);
    bUnlockConfigLock = TRUE;

    UmnSrvFreeConfig(gpAPIConfig);
    gpAPIConfig = pAPIConfig;
    pAPIConfig = NULL;

    dwError = UmnSrvPollerRefresh();
    if (dwError == ESRCH)
    {
        // The thread may not be running yet
        dwError = 0;
    }
    BAIL_ON_UMN_ERROR(dwError);

cleanup:

    UmnSrvFreeConfig(pAPIConfig);

    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gUmnConfigLock);
    }

    return(dwError);

error:
    goto cleanup;
}

DWORD
UmnSrvInitConfig(
    PUMN_SRV_API_CONFIG *ppConfig
    )
{
    PUMN_SRV_API_CONFIG pConfig = NULL;
    DWORD dwError = 0;

    dwError = RTL_ALLOCATE(
                    &pConfig,
                    UMN_SRV_API_CONFIG,
                    sizeof(*pConfig));
    BAIL_ON_UMN_ERROR(dwError);

    *ppConfig = pConfig;

cleanup:
    return dwError;

error:
    *ppConfig = NULL;
    UmnSrvFreeConfig(pConfig);
    goto cleanup;
}

DWORD
UmnSrvReadConfig(
    PUMN_SRV_API_CONFIG *ppConfig
    )
{
    DWORD dwError = 0;
    PUMN_SRV_API_CONFIG pConfig = NULL;

    dwError = UmnSrvInitConfig(&pConfig);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = UmnReadEventFwdConfigSettings(pConfig);
    BAIL_ON_UMN_ERROR(dwError);
    
    UMN_LOG_VERBOSE("CheckInterval = %d seconds\n",
            UMN_SAFE_LOG_STRING(pConfig->CheckInterval));

    *ppConfig = pConfig;

cleanup:

    return dwError;

error:
    UmnSrvFreeConfig(pConfig);
    *ppConfig = NULL;

    goto cleanup;
}

VOID
UmnSrvFreeConfig(
    PUMN_SRV_API_CONFIG pConfig
    )
{
    if (pConfig)
    {
        RtlCStringFree(&pConfig->pszCollector);
        RtlCStringFree(&pConfig->pszServicePrincipal);
        LW_SAFE_FREE_MEMORY(pConfig);
    }
}

DWORD
UmnSrvGetCollectorAddress(
    HANDLE hServer,
    PSTR *ppszCollector
    )
{
    DWORD dwError = 0;
    PSTR pszCollector = NULL;
    BOOLEAN bUnlockConfigLock = FALSE;

    BAIL_ON_INVALID_POINTER(ppszCollector);

    pthread_rwlock_rdlock(&gUmnConfigLock);
    bUnlockConfigLock = TRUE;

    if (!gpAPIConfig->pszCollector)
    {
        dwError = LW_STATUS_NOT_FOUND;
        BAIL_ON_UMN_ERROR(dwError);
    }
    dwError = RtlCStringDuplicate(
                    &pszCollector,
                    gpAPIConfig->pszCollector);
    BAIL_ON_UMN_ERROR(dwError);

    *ppszCollector = pszCollector;

cleanup:
    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gUmnConfigLock);
    }

    return dwError;

error:

    *ppszCollector = NULL;

    RtlCStringFree(&pszCollector);

    goto cleanup;
}

DWORD
UmnSrvGetCollectorServicePrincipal(
    HANDLE hServer,
    PSTR *ppszPrincipal
    )
{
    DWORD dwError = 0;
    PSTR pszPrincipal = NULL;
    BOOLEAN bUnlockConfigLock = FALSE;

    BAIL_ON_INVALID_POINTER(ppszPrincipal);

    pthread_rwlock_rdlock(&gUmnConfigLock);
    bUnlockConfigLock = TRUE;

    if (!gpAPIConfig->pszServicePrincipal)
    {
        dwError = LW_STATUS_NOT_FOUND;
        BAIL_ON_UMN_ERROR(dwError);
    }
    dwError = RtlCStringDuplicate(
                    &pszPrincipal,
                    gpAPIConfig->pszServicePrincipal);
    BAIL_ON_UMN_ERROR(dwError);

    *ppszPrincipal = pszPrincipal;

cleanup:
    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gUmnConfigLock);
    }

    return dwError;

error:

    *ppszPrincipal = NULL;

    RtlCStringFree(&pszPrincipal);

    goto cleanup;
}
DWORD
UmnReadEventFwdConfigSettings(PUMN_SRV_API_CONFIG pConfig)
{
    DWORD dwError = 0;
    PSTR pszCollector = NULL;
    PSTR pszCollectorPrincipal = NULL;

    LWREG_CONFIG_ITEM ConfigDescription[] =
    {
        {
            "Collector",
            TRUE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pszCollector,
            NULL
        },
        {
            "CollectorPrincipal",
            TRUE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pszCollectorPrincipal,
            NULL
        }
    };

    UMN_LOG_INFO("Read Eventlog configuration settings");

    dwError = LwRegProcessConfig(
                "Services\\eventfwd\\Parameters",
                "Policy\\Services\\eventfwd\\Parameters",
                ConfigDescription,
                sizeof(ConfigDescription)/sizeof(ConfigDescription[0]));
    
    BAIL_ON_UMN_ERROR(dwError);
    
    if (pszCollector && pszCollector[0])
    {
        dwError = LwAllocateString(pszCollector, &pConfig->pszCollector);
        BAIL_ON_UMN_ERROR(dwError);
    }
    
    if (pszCollectorPrincipal && pszCollectorPrincipal[0])
    {
        dwError = LwAllocateString(pszCollectorPrincipal, &pConfig->pszServicePrincipal);
        BAIL_ON_UMN_ERROR(dwError);
    }

error:
    LW_SAFE_FREE_STRING(pszCollector);
    LW_SAFE_FREE_STRING(pszCollectorPrincipal);
    return dwError;
}

