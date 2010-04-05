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
 *        Likewise I/O (LWIO) - NFS
 *
 *        Configuration
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
NTSTATUS
NfsTransferConfigContents(
    PLWIO_NFS_CONFIG pSrc,
    PLWIO_NFS_CONFIG pDest
    );


NTSTATUS
NfsReadConfig(
    PLWIO_NFS_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWIO_NFS_CONFIG nfsConfig = {0};

    ntStatus = NfsInitConfig(&nfsConfig);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsTransferConfigContents(&nfsConfig, pConfig);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    NfsFreeConfigContents(&nfsConfig);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NfsInitConfig(
    PLWIO_NFS_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    NfsFreeConfigContents(pConfig);

    pConfig->ulMaxNumPackets          = LWIO_NFS_DEFAULT_NUM_MAX_PACKETS;
    pConfig->ulNumWorkers             = LWIO_NFS_DEFAULT_NUM_WORKERS;
    pConfig->ulMaxNumWorkItemsInQueue = LWIO_NFS_DEFAULT_NUM_MAX_QUEUE_ITEMS;

    return ntStatus;
}

static
NTSTATUS
NfsTransferConfigContents(
    PLWIO_NFS_CONFIG pSrc,
    PLWIO_NFS_CONFIG pDest
    )
{
    NfsFreeConfigContents(pDest);

    *pDest = *pSrc;

    NfsFreeConfigContents(pSrc);

    return 0;
}

VOID
NfsFreeConfigContents(
    PLWIO_NFS_CONFIG pConfig
    )
{
    // Nothing to free right now
    memset(pConfig, 0, sizeof(*pConfig));
}
