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
 *        read.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (RDR)
 *
 *       Read Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "rdr.h"

/* Offset from the beginning off the SMB header to
   the data in the Read&X response */

#define READ_DATA_OFFSET     60

static
NTSTATUS
RdrCommonRead(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrRead(
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

    ntStatus = RdrCommonRead(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
RdrCommonRead(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pBuffer = NULL;
    ULONG ulLength = 0;
    ULONG ulReadMax = 0;
    ULONG ulReadLength = 0;
    LONG64 llByteOffset = 0;
    USHORT usBytesRead = 0;
    LONG64 llBufferOffset = 0;
    ULONG ulTotalBytesRead = 0;
    PSMB_CLIENT_FILE_HANDLE pFile = IoFileGetContext(pIrp->FileHandle);

    pBuffer = pIrp->Args.ReadWrite.Buffer;
    ulLength = pIrp->Args.ReadWrite.Length;

    if (pIrp->Args.ReadWrite.ByteOffset)
    {
        llByteOffset = *pIrp->Args.ReadWrite.ByteOffset;
    }
    else
    {
        llByteOffset = pFile->llOffset;
    }

    /* Resp can only contain "MaxTransmitBufferSize - (SMB_HEADER + READX_HEADER)" */

    ulReadMax = pFile->pTree->pSession->pSocket->maxBufferSize - READ_DATA_OFFSET;

    while (ulLength)
    {
        ulReadLength = ulReadMax;

        if (ulReadLength > UINT16_MAX)
        {
            ulReadLength = UINT16_MAX;
        }

        if (ulReadLength > ulLength)
        {
            ulReadLength = ulLength;
        }

        ntStatus = RdrTransactReadFile(
            pFile->pTree,
            pFile->fid,
            (ULONG64) llByteOffset,
            pBuffer + llBufferOffset,
            (USHORT) ulReadLength,
            /* If we already have some data, use MinCount = 0 to avoid
               accidental blocks on named pipes */
            ulTotalBytesRead == 0 ? (USHORT) ulReadLength : 0,
            &usBytesRead);
        BAIL_ON_NT_STATUS(ntStatus);

        ulTotalBytesRead += usBytesRead;
        ulLength -= usBytesRead;
        llByteOffset += usBytesRead;
        llBufferOffset += usBytesRead;

        if (usBytesRead < ulReadLength)
        {
            if (ulTotalBytesRead == 0)
            {
                ntStatus = STATUS_END_OF_FILE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                break;
            }
        }
    }

    pIrp->IoStatusBlock.BytesTransferred = ulTotalBytesRead;

cleanup:

    pFile->llOffset = llByteOffset;

    pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;

error:

    goto cleanup;
}

