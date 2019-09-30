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
 * Abstract:
 *
 * Authors: Scott Salley <ssalley@likewise.com>
 *          Gerald Carter Mgcarter@likewise.com>
 *
 */

#include "includes.h"

static
NTSTATUS
LwioSrvReadRegistry(
    IN OUT PLWIO_CONFIG pConfig
    );

static
NTSTATUS
LwioSrvTransferConfigContents(
    PLWIO_CONFIG pDstConfig,
    PLWIO_CONFIG pSrcConfig
    );


/***********************************************************************
 **********************************************************************/

NTSTATUS
LwioSrvRefreshConfig(
    PLWIO_CONFIG pCurrentConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWIO_CONFIG newConfig;
    BOOLEAN bConfigLocked = FALSE;

    ntStatus = LwioSrvInitializeConfig(&newConfig);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwioSrvReadRegistry(&newConfig);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bConfigLocked, &newConfig.RwLock);

    ntStatus = LwioSrvTransferConfigContents(
                   pCurrentConfig,
                   &newConfig);

    LWIO_UNLOCK_RWMUTEX(bConfigLocked, &newConfig.RwLock);

    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoMgrRefreshConfig();
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LwioSrvFreeConfigContents(&newConfig);

    return ntStatus;

error:

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
LwioSrvReadRegistry(
    IN OUT PLWIO_CONFIG pConfig
    )
{
    return STATUS_SUCCESS;
}


/***********************************************************************
 **********************************************************************/

VOID
LwioSrvFreeConfig(
    IN OUT PLWIO_CONFIG pConfig
    )
{
    if (pConfig)
    {
        LwioSrvFreeConfigContents(pConfig);

        LwIoFreeMemory(pConfig);
    }
}


/***********************************************************************
 **********************************************************************/

VOID
LwioSrvFreeConfigContents(
    IN OUT PLWIO_CONFIG pConfig
    )
{
    if (pConfig->pRwLock)
    {
        pthread_rwlock_destroy(&pConfig->RwLock);
    }
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
LwioSrvInitializeConfig(
    IN OUT PLWIO_CONFIG pConfig
    )
{
    pthread_rwlock_init(&pConfig->RwLock, NULL);
    pConfig->pRwLock = &pConfig->RwLock;

    return STATUS_SUCCESS;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
LwioSrvTransferConfigContents(
    PLWIO_CONFIG pDstConfig,
    PLWIO_CONFIG pSrcConfig
    )
{
    return STATUS_SUCCESS;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
