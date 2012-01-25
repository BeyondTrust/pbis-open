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

    pConfig->CheckInterval = 60 * 30;

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

    dwError = UmnSrvReadAllocatedConfig(pConfig);
    BAIL_ON_UMN_ERROR(dwError);
    
    UMN_LOG_VERBOSE("CheckInterval = %d seconds\n",
            pConfig->CheckInterval);

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
        LW_SAFE_FREE_MEMORY(pConfig);
    }
}

DWORD
UmnSrvGetCheckInterval(
    HANDLE hServer,
    PDWORD pValue
    )
{
    DWORD dwError = 0;
    BOOLEAN bUnlockConfigLock = FALSE;

    BAIL_ON_INVALID_POINTER(pValue);

    pthread_rwlock_rdlock(&gUmnConfigLock);
    bUnlockConfigLock = TRUE;

    *pValue = gpAPIConfig->CheckInterval;

cleanup:
    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gUmnConfigLock);
    }

    return dwError;

error:

    *pValue = 0;
    goto cleanup;
}

DWORD
UmnSrvReadAllocatedConfig(
    PUMN_SRV_API_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    LWREG_CONFIG_ITEM ConfigDescription[] =
    {
        {
            "CheckInterval",
            TRUE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &pConfig->CheckInterval,
            NULL
        },
    };

    UMN_LOG_INFO("Read user monitor configuration settings");

    dwError = LwRegProcessConfig(
                "Services\\usermonitor\\Parameters",
                "Policy\\Services\\usermonitor\\Parameters",
                ConfigDescription,
                sizeof(ConfigDescription)/sizeof(ConfigDescription[0]));
    BAIL_ON_UMN_ERROR(dwError);

error:
    return dwError;
}

