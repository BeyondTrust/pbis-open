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
 *        Driver Entry Function
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "rdr.h"

static
NTSTATUS
RdrCommonSetInformation(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

static
NTSTATUS
RdrCommonRename(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

static
NTSTATUS
RdrGetFilePath(
    PCWSTR pwszFullPath,
    PWSTR* ppwszFilePath
    );

NTSTATUS
RdrSetInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = RdrCommonSetInformation(
        NULL,
        pIrp
        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
RdrCommonSetInformation(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_INFO_LEVEL infoLevel = 0;
    PSMB_CLIENT_FILE_HANDLE pFile = NULL;

    switch (pIrp->Args.QuerySetInformation.FileInformationClass)
    {
    case FileEndOfFileInformation:
        infoLevel = SMB_SET_FILE_END_OF_FILE_INFO;
        break;
    case FileRenameInformation:
        /* Handle this as a special case */
        ntStatus = RdrCommonRename(
            pIrpContext,
            pIrp);
        goto error;
        break;
    default:
        ntStatus = STATUS_NOT_IMPLEMENTED;
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

    pFile = IoFileGetContext(pIrp->FileHandle);
    
    ntStatus = RdrTransactSetInfoFile(
        pFile->pTree,
        pFile->fid,
        infoLevel,
        pIrp->Args.QuerySetInformation.FileInformation,
        pIrp->Args.QuerySetInformation.Length);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;
}

static
NTSTATUS
RdrCommonRename(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_CLIENT_FILE_HANDLE pFile = NULL;
    PFILE_RENAME_INFORMATION pRenameInfo = NULL;
    PWSTR pwszNewFilePath = NULL;

    pFile = IoFileGetContext(pIrp->FileHandle);

    pRenameInfo = pIrp->Args.QuerySetInformation.FileInformation;

    if (pFile->fid)
    {
        ntStatus = RdrTransactCloseFile(
            pFile->pTree,
            pFile->fid
            );
        BAIL_ON_NT_STATUS(ntStatus);

        pFile->fid = 0;
    }

    ntStatus = RdrGetFilePath(
        pRenameInfo->FileName,
        &pwszNewFilePath);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RdrTransactRenameFile(
        pFile->pTree,
        0x7f, /* FIXME: magic constant */
        pFile->pwszPath,
        pwszNewFilePath);

    RTL_FREE(&pFile->pwszPath);
    pFile->pwszPath = pwszNewFilePath;
    pwszNewFilePath = NULL;
    
error:

    RTL_FREE(&pwszNewFilePath);

    return ntStatus;
}

static
NTSTATUS
RdrGetFilePath(
    PCWSTR pwszFullPath,
    PWSTR* ppwszFilePath
    )
{
    NTSTATUS ntStatus = 0;
    PCWSTR pwszIndex = NULL;
    USHORT usNumSlashes = 0;
    PWSTR pwszFilePath = NULL;
    size_t i = 0;

    for (pwszIndex = pwszFullPath; *pwszIndex; pwszIndex++)
    {
        if (*pwszIndex == '/')
        {
            usNumSlashes++;
        }

        if (usNumSlashes == 3)
        {
            break;
        }
    }

    if (usNumSlashes != 3)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = RTL_ALLOCATE(
        &pwszFilePath,
        WCHAR,
        (LwRtlWC16StringNumChars(pwszIndex) + 1) * sizeof(WCHAR));
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; pwszIndex[i]; i++)
    {
        switch (pwszIndex[i])
        {
        case '/':
            pwszFilePath[i] = '\\';
            break;
        default:
            pwszFilePath[i] = pwszIndex[i];
            break;
        }
    }

    pwszFilePath[i] = '\0';

    *ppwszFilePath = pwszFilePath;

cleanup:

    return ntStatus;

error:

    *ppwszFilePath = NULL;

    RTL_FREE(&pwszFilePath);

    goto cleanup;
}
