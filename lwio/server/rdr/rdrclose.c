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
 *        close.c
 *
 * Abstract:
 *
 *        Likewise SMB Redirector File System Driver (RDR)
 *
 *        Close Dispatch Function
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "rdr.h"

static
NTSTATUS
RdrCommonClose(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrClose(
    IO_DEVICE_HANDLE DeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = RdrCommonClose(
        NULL,
        pIrp
        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
VOID
RdrCloseWorkItem(
    PVOID pContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIRP pIrp = (PIRP) pContext;
    PSMB_CLIENT_FILE_HANDLE pFile = NULL;

    pFile = IoFileGetContext(pIrp->FileHandle);

    if (pFile->fid)
    {
        ntStatus = RdrTransactCloseFile(
            pFile->pTree,
            pFile->fid
            );
        BAIL_ON_NT_STATUS(ntStatus);

        pFile->fid = 0;
    }

cleanup:

    RdrReleaseFile(pFile);
    
    pIrp->IoStatusBlock.Status = ntStatus;
    
    IoIrpComplete(pIrp);
    
    return;
    
error:
    
    /* We discard any errors on close and proceed to
       release all local resources */
    ntStatus = STATUS_SUCCESS;

    goto cleanup;
}

static
VOID
RdrCancelClose(
    PIRP pIrp,
    PVOID pContext
    )
{
    return;
}

static
NTSTATUS
RdrCommonClose(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    IoIrpMarkPending(pIrp, RdrCancelClose, NULL);

    ntStatus = LwRtlQueueWorkItem(gRdrRuntime.pThreadPool, RdrCloseWorkItem, pIrp, 0);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = STATUS_PENDING;
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;
    
error:
    
    goto cleanup;
}
