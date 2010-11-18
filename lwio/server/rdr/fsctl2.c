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
 *        fsctl2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        SMB2 FSCTL
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 */

#include "rdr.h"

static
NTSTATUS
RdrGetSessionKey2(
    PRDR_CCB2 pFile,
    PVOID pBuffer,
    ULONG ulLength,
    PULONG pulLengthUsed
    );

NTSTATUS
RdrFsctl2(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID pOutBuffer = pIrp->Args.IoFsControl.OutputBuffer;
    ULONG OutLength = pIrp->Args.IoFsControl.OutputBufferLength;
    PRDR_CCB2 pFile = IoFileGetContext(pIrp->FileHandle);

    switch (pIrp->Args.IoFsControl.ControlCode)
    {
    case IO_FSCTL_SMB_GET_SESSION_KEY:
        status = RdrGetSessionKey2(
            pFile,
            pOutBuffer,
            OutLength,
            &pIrp->IoStatusBlock.BytesTransferred);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        status = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(status);
    }

error:

    pIrp->IoStatusBlock.Status = status;

    return status;
}

static
NTSTATUS
RdrGetSessionKey2(
    PRDR_CCB2 pFile,
    PVOID pBuffer,
    ULONG ulLength,
    PULONG pulLengthUsed
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SESSION2 pSession = pFile->pTree->pSession;

    if (pSession->dwSessionKeyLength > ulLength)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }
    
    memcpy(pBuffer, pSession->pSessionKey, pSession->dwSessionKeyLength);

    *pulLengthUsed = pSession->dwSessionKeyLength;

error:

    return status;
}
