/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

#include "rdr.h"

static
NTSTATUS
RdrCommonQueryVolumeInformation(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrQueryVolumeInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = RdrCommonQueryVolumeInformation(
        NULL,
        pIrp
        );
    BAIL_ON_NT_STATUS(ntStatus);

error:
    
    return ntStatus;
}


static
NTSTATUS
RdrCommonQueryVolumeInformation(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_CLIENT_FILE_HANDLE pFile = IoFileGetContext(pIrp->FileHandle);
    SMB_INFO_LEVEL infoLevel = 0;

    switch(pIrp->Args.QuerySetVolumeInformation.FsInformationClass)
    {
    case FileFsSizeInformation:
        infoLevel = SMB_INFO_ALLOCATION;
        break;
    default:
        ntStatus = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

    ntStatus = RdrTransactQueryFsInfo(
        pFile->pTree,
        infoLevel,
        pIrp->Args.QuerySetVolumeInformation.FsInformation,
        pIrp->Args.QuerySetVolumeInformation.Length,
        &pIrp->IoStatusBlock.BytesTransferred);
    BAIL_ON_NT_STATUS(ntStatus);
    
error:

    pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;
}
