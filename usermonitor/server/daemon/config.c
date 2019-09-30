/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
    pConfig->SkipNoLogin = FALSE;

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
    UMN_LOG_VERBOSE("SkipNoLogin = %d\n",
            pConfig->SkipNoLogin);

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
UmnSrvGetSkipNoLogin(
    PBOOLEAN pValue
    )
{
    DWORD dwError = 0;
    BOOLEAN bUnlockConfigLock = FALSE;

    BAIL_ON_INVALID_POINTER(pValue);

    pthread_rwlock_rdlock(&gUmnConfigLock);
    bUnlockConfigLock = TRUE;

    *pValue = gpAPIConfig->SkipNoLogin;

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
        {
            "SkipNoLogin",
            TRUE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &pConfig->SkipNoLogin,
            NULL
        },
    };

    UMN_LOG_INFO("Read user monitor configuration settings");

    dwError = LwRegProcessConfigUsingAttributeRanges(
                "Services\\" SERVICE_NAME "\\Parameters",
                "Policy\\Services\\" SERVICE_NAME "\\Parameters",
                ConfigDescription,
                sizeof(ConfigDescription)/sizeof(ConfigDescription[0]));
    BAIL_ON_UMN_ERROR(dwError);

error:
    return dwError;
}

