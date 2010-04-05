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
 *        config.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols
 *
 *        Configuration
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
NfsProtocolTransferConfigContents(
    PNFS_PROTOCOL_CONFIG pSrc,
    PNFS_PROTOCOL_CONFIG pDest
    );


NTSTATUS
NfsProtocolReadConfig(
    PNFS_PROTOCOL_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NFS_PROTOCOL_CONFIG config = { 0 };
    PLWIO_CONFIG_REG pReg = NULL;

    ntStatus = NfsProtocolInitConfig(&config);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwIoOpenConfig(
                "Services\\lwio\\Parameters\\Drivers\\nfs",
                "Policy\\Services\\lwio\\Parameters\\Drivers\\nfs",
                &pReg);
    if (ntStatus)
    {
        LWIO_LOG_ERROR("Failed to access device configuration [error code: %u]",
                       ntStatus);

        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    /* Ignore error as it may not exist; we can still use default. */
    LwIoReadConfigBoolean(
            pReg,
            "SupportSmb2",
            NFS_PROTOCOL_CONFIG_DEFAULT_ENABLE_SMB2,
            &config.bEnableSmb2);

    LwIoReadConfigBoolean(
            pReg,
            "EnableSecuritySignatures",
            NFS_PROTOCOL_CONFIG_DEFAULT_ENABLE_SIGNING,
            &config.bEnableSigning);

    LwIoReadConfigBoolean(
            pReg,
            "RequireSecuritySignatures",
            NFS_PROTOCOL_CONFIG_DEFAULT_REQUIRE_SIGNING,
            &config.bRequireSigning);

    ntStatus = NfsProtocolTransferConfigContents(&config, pConfig);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pReg)
    {
        LwIoCloseConfig(pReg);
    }

    NfsProtocolFreeConfigContents(&config);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NfsProtocolInitConfig(
    PNFS_PROTOCOL_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    NfsProtocolFreeConfigContents(pConfig);

    pConfig->bEnableSmb2 = NFS_PROTOCOL_CONFIG_DEFAULT_ENABLE_SMB2;
    pConfig->bEnableSigning = NFS_PROTOCOL_CONFIG_DEFAULT_ENABLE_SIGNING;
    pConfig->bRequireSigning = NFS_PROTOCOL_CONFIG_DEFAULT_REQUIRE_SIGNING;

    return ntStatus;
}

static
NTSTATUS
NfsProtocolTransferConfigContents(
    PNFS_PROTOCOL_CONFIG pSrc,
    PNFS_PROTOCOL_CONFIG pDest
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    NfsProtocolFreeConfigContents(pDest);

    *pDest = *pSrc;

    NfsProtocolFreeConfigContents(pSrc);

    return ntStatus;
}

VOID
NfsProtocolFreeConfigContents(
    PNFS_PROTOCOL_CONFIG pConfig
    )
{
    // Nothing to free right now
    memset(pConfig, 0, sizeof(*pConfig));
}

BOOLEAN
NfsProtocolConfigIsSigningEnabled(
    VOID
    )
{
    BOOLEAN bEnabled = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gProtocolApiGlobals.mutex);

    bEnabled = gProtocolApiGlobals.config.bEnableSigning;

    LWIO_UNLOCK_MUTEX(bInLock, &gProtocolApiGlobals.mutex);

    return bEnabled;
}

BOOLEAN
NfsProtocolConfigIsSigningRequired(
    VOID
    )
{
    BOOLEAN bRequired = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gProtocolApiGlobals.mutex);

    bRequired = gProtocolApiGlobals.config.bRequireSigning;

    LWIO_UNLOCK_MUTEX(bInLock, &gProtocolApiGlobals.mutex);

    return bRequired;
}

BOOLEAN
NfsProtocolConfigIsSmb2Enabled(
    VOID
    )
{
    BOOLEAN bEnabled = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gProtocolApiGlobals.mutex);

    bEnabled = gProtocolApiGlobals.config.bEnableSmb2;

    LWIO_UNLOCK_MUTEX(bInLock, &gProtocolApiGlobals.mutex);

    return bEnabled;
}
