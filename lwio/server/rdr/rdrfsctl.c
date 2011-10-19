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
 *        driver.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (RDR)
 *
 *        Device I/O Function
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "rdr.h"

NTSTATUS
RdrFsctl(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PRDR_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = RdrAllocateIrpContext(
                        pIrp,
                        &pIrpContext
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RdrCommonFsctl(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}


NTSTATUS
RdrCommonFsctl(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pOutBuffer = pIrp->Args.IoFsControl.OutputBuffer;
    ULONG OutLength = pIrp->Args.IoFsControl.OutputBufferLength;
    HANDLE hFile = NULL;
    DWORD dwSessionKeyLength = 0;
    PBYTE pSessionKeyBytes = NULL;

    hFile = IoFileGetContext(pIrp->FileHandle);

    ntStatus = RdrGetSessionKey(
                    hFile,
                    &dwSessionKeyLength,
                    &pSessionKeyBytes
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    /* Ensure there is enough space in the output buffer for the
       session key structure */
    if (dwSessionKeyLength > OutLength)
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pSessionKeyBytes, dwSessionKeyLength);
    
    pIrp->IoStatusBlock.BytesTransferred = dwSessionKeyLength;

error:

    IO_SAFE_FREE_MEMORY(pSessionKeyBytes);

    pIrp->IoStatusBlock.Status = ntStatus;
    return(ntStatus);
}

