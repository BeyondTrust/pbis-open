/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_CONFIG_REG pReg = NULL;

    ntStatus = LwIoOpenConfig(
                   LWIO_CONF_REGISTRY_LOCAL,
                   LWIO_CONF_REGISTRY_POLICY,
                   &pReg);
    if (ntStatus)
    {
        LWIO_LOG_ERROR(
            "Failed to access device configuration [error code: %l]",
            ntStatus);

        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pReg)
    {
        LwIoCloseConfig(pReg);
    }

    return ntStatus;

error:

    goto cleanup;
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
