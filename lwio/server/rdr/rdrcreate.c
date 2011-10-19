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
 *        create.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *       Create Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "rdr.h"

NTSTATUS
RdrCreate(
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

    ntStatus = RdrCommonCreate(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}



NTSTATUS
RdrAllocateIrpContext(
    PIRP pIrp,
    PRDR_IRP_CONTEXT * ppIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PRDR_IRP_CONTEXT pIrpContext = NULL;

    /*ntStatus = IoMemoryAllocate(
                    sizeof(RDR_IRP_CONTEXT),
                    &pIrpContext
                    );*/
    BAIL_ON_NT_STATUS(ntStatus);

    *ppIrpContext = pIrpContext;

    return(ntStatus);

error:

    *ppIrpContext = NULL;
    return(ntStatus);
}


NTSTATUS
RdrAllocateCCB(
    PRDR_CCB *ppCCB
    )
{
    NTSTATUS ntStatus = 0;
    PRDR_CCB pCCB = NULL;

   /* ntStatus = IoMemoryAllocate(
                    sizeof(RDR_CCB),
                    &pCCB
                    );*/
    BAIL_ON_NT_STATUS(ntStatus);

    *ppCCB = pCCB;

    return(ntStatus);

error:

    *ppCCB = NULL;

    return(ntStatus);
}



NTSTATUS
RdrCommonCreate(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE FileHandle;
    PIO_FILE_NAME pFileName;
    ACCESS_MASK DesiredAccess;
    LONG64 AllocationSize;
    FILE_SHARE_FLAGS ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition;
    FILE_CREATE_OPTIONS CreateOptions;
    FILE_ATTRIBUTES FileAttributes;
    HANDLE hFile = NULL;
    PIO_CREDS pSecurityToken = IoSecurityGetCredentials(pIrp->Args.Create.SecurityContext);
    PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcessInfo = IoSecurityGetProcessInfo(pIrp->Args.Create.SecurityContext);

    FileHandle = pIrp->FileHandle;
    DesiredAccess = pIrp->Args.Create.DesiredAccess;
    AllocationSize = pIrp->Args.Create.AllocationSize;
    FileAttributes = pIrp->Args.Create.FileAttributes;
    ShareAccess = pIrp->Args.Create.ShareAccess;
    CreateDisposition = pIrp->Args.Create.CreateDisposition;
    CreateOptions = pIrp->Args.Create.CreateOptions;
    pFileName = &pIrp->Args.Create.FileName;

    ntStatus = RdrCreateFileEx(
        pSecurityToken,
        pProcessInfo,
        pFileName->FileName,
        DesiredAccess,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        &hFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(FileHandle, hFile);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;
}

