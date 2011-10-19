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
 *        rdrquerydir.c
 *
 * Abstract:
 *
 *        Likewise IO Redirector (RDR)
 *
 *        Query Directory Information
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "rdr.h"

#define MAX_FIND_BUFFER 4096

static
NTSTATUS
RdrCommonQueryDirectory(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

static
NTSTATUS
RdrFileSpecToSearchPattern(
    PCWSTR pwszPath,
    PIO_MATCH_FILE_SPEC pFileSpec,
    PWSTR* ppwszSearchPattern
    );

static
NTSTATUS
RdrUnmarshalFindResults(
    PSMB_CLIENT_FILE_HANDLE pHandle,
    BOOLEAN bReturnSingleEntry,
    PVOID pFileInformation,
    ULONG ulLength,
    FILE_INFORMATION_CLASS fileInformationClass,
    PULONG pulLengthUsed
    );

static
NTSTATUS
RdrUnmarshalFileBothDirectoryInformation(
    PSMB_CLIENT_FILE_HANDLE pHandle,
    PVOID pFileInformation,
    ULONG ulLength,
    PULONG pulLengthUsed,
    PULONG* ppulNextOffset
    );

NTSTATUS
RdrQueryDirectory(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = RdrCommonQueryDirectory(
        NULL,
        pIrp
        );
    BAIL_ON_NT_STATUS(ntStatus);

error:
    
    return ntStatus;
}


static
NTSTATUS
RdrCommonQueryDirectory(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_CLIENT_FILE_HANDLE pHandle = NULL;
    SMB_INFO_LEVEL infoLevel = 0;
    PWSTR pwszPattern = NULL;

    switch (pIrp->Args.QueryDirectory.FileInformationClass)
    {
    case FileBothDirectoryInformation:
        infoLevel = SMB_FIND_FILE_BOTH_DIRECTORY_INFO;
        break;
    default:
        ntStatus = STATUS_NOT_IMPLEMENTED;
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

    pHandle = IoFileGetContext(pIrp->FileHandle);

    if (pHandle->find.pBuffer && pHandle->find.usSearchCount == 0)
    {
        if (pHandle->find.usEndOfSearch)
        {
            /* We are out of of buffered entries and
               the server has no more results for us */
            ntStatus = STATUS_NO_MORE_MATCHES;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            /* Perform a find next */
            ntStatus = RdrTransactFindNext2(
                pHandle->pTree,
                pHandle->find.usSearchId,
                512, /* Search count */
                infoLevel,
                0, /* ulResumeKey */
                0x2, /* Search flags */
                NULL, /* Filename */
                &pHandle->find.usSearchCount,
                &pHandle->find.usEndOfSearch,
                NULL, /* EA error offest */
                &pHandle->find.usLastNameOffset, /* Last name offset */
                pHandle->find.pBuffer,
                pHandle->find.ulBufferCapacity,
                &pHandle->find.ulBufferLength);
            BAIL_ON_NT_STATUS(ntStatus);

            pHandle->find.pCursor = pHandle->find.pBuffer;
        }
    }
    else if (!pHandle->find.pBuffer)
    {
        /* This is the first query, so we start a find */
        pHandle->find.ulBufferCapacity = MAX_FIND_BUFFER;

        ntStatus = RTL_ALLOCATE(&pHandle->find.pBuffer,
                                BYTE,
                                pHandle->find.ulBufferCapacity);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RdrFileSpecToSearchPattern(
            pHandle->pwszPath,
            pIrp->Args.QueryDirectory.FileSpec,
            &pwszPattern);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RdrTransactFindFirst2(
            pHandle->pTree,
            0x7f, /* Search attributes */
            512, /* Search count */
            0x2, /* Search flags */
            infoLevel, /* Info level */
            0, /* Search storage type */
            pwszPattern, /* Search pattern */
            &pHandle->find.usSearchId,
            &pHandle->find.usSearchCount,
            &pHandle->find.usEndOfSearch,
            NULL,
            &pHandle->find.usLastNameOffset,
            pHandle->find.pBuffer,
            pHandle->find.ulBufferCapacity,
            &pHandle->find.ulBufferLength);
        BAIL_ON_NT_STATUS(ntStatus);

        pHandle->find.pCursor = pHandle->find.pBuffer;
    }

    if (pHandle->find.usSearchCount == 0)
    {
        ntStatus = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = RdrUnmarshalFindResults(
        pHandle,
        pIrp->Args.QueryDirectory.ReturnSingleEntry,
        pIrp->Args.QueryDirectory.FileInformation,
        pIrp->Args.QueryDirectory.Length,
        pIrp->Args.QueryDirectory.FileInformationClass,
        &pIrp->IoStatusBlock.BytesTransferred
        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;
}

static
NTSTATUS
RdrFileSpecToSearchPattern(
    PCWSTR pwszPath,
    PIO_MATCH_FILE_SPEC pFileSpec,
    PWSTR* ppwszSearchPattern
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszPattern = NULL;
    size_t pathLength = 0;

    if (pFileSpec)
    {
        ntStatus = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        pathLength = LwRtlWC16StringNumChars(pwszPath);

        ntStatus = RTL_ALLOCATE(
            &pwszPattern,
            WCHAR,
            (pathLength + 1 + 1 + 1) * sizeof(WCHAR));
        BAIL_ON_NT_STATUS(ntStatus);
        memcpy(pwszPattern, pwszPath, pathLength * sizeof(WCHAR));
        pwszPattern[pathLength] = '\\';
        pwszPattern[pathLength+1] = '*';
        pwszPattern[pathLength+2] = '\0';
    }

    *ppwszSearchPattern = pwszPattern;

cleanup:

    return ntStatus;

error:

    RTL_FREE(&pwszPattern);

    goto cleanup;
}

static
NTSTATUS
RdrUnmarshalFindResults(
    PSMB_CLIENT_FILE_HANDLE pHandle,
    BOOLEAN bReturnSingleEntry,
    PVOID pFileInformation,
    ULONG ulLength,
    FILE_INFORMATION_CLASS fileInformationClass,
    PULONG pulLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulLengthUsed = 0;
    ULONG ulTotalLengthUsed = 0;
    PULONG pulNextOffset = NULL;

    do
    {
        switch (fileInformationClass)
        {
        case FileBothDirectoryInformation:
            ntStatus = RdrUnmarshalFileBothDirectoryInformation(
                pHandle,
                pFileInformation,
                ulLength,
                &ulLengthUsed,
                &pulNextOffset
                );
            BAIL_ON_NT_STATUS(ntStatus);
            break;
        default:
            ntStatus = STATUS_NOT_SUPPORTED;
            BAIL_ON_NT_STATUS(ntStatus);
            break;
        }

        if (ulLengthUsed)
        {
            ulTotalLengthUsed += ulLengthUsed;
            pFileInformation = ((PBYTE) pFileInformation) + ulLengthUsed;
            ulLength -= ulLengthUsed;
            *pulNextOffset = ulLengthUsed;
        }
    } while (!bReturnSingleEntry && ulLengthUsed);

    if (pulNextOffset)
    {
        *pulNextOffset = 0;
    }

    *pulLengthUsed = ulTotalLengthUsed;

error:

    return ntStatus;
}

static
NTSTATUS
RdrUnmarshalFileBothDirectoryInformation(
    PSMB_CLIENT_FILE_HANDLE pHandle,
    PVOID pFileInformation,
    ULONG ulLength,
    PULONG pulLengthUsed,
    PULONG* ppulNextOffset
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PFILE_BOTH_DIR_INFORMATION pBothInfo = pFileInformation;
    PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER pBothInfoPacked = (PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER) pHandle->find.pCursor;
    ULONG ulFileNameLength = SMB_LTOH32(pBothInfoPacked->FileNameLength);
    ULONG ulLengthUsed = 0;

    if ((ulFileNameLength + 1) * sizeof(WCHAR) + sizeof(*pBothInfo) <= ulLength &&
        pHandle->find.usSearchCount)
    {
        pBothInfo->FileIndex         = SMB_LTOH32(pBothInfoPacked->FileIndex);
        pBothInfo->CreationTime      = SMB_LTOH64(pBothInfoPacked->CreationTime);
        pBothInfo->LastAccessTime    = SMB_LTOH64(pBothInfoPacked->LastAccessTime);
        pBothInfo->LastWriteTime     = SMB_LTOH64(pBothInfoPacked->LastWriteTime);
        pBothInfo->ChangeTime        = SMB_LTOH64(pBothInfoPacked->ChangeTime);
        pBothInfo->EndOfFile         = SMB_LTOH64(pBothInfoPacked->EndOfFile);
        pBothInfo->AllocationSize    = SMB_LTOH64(pBothInfoPacked->AllocationSize);
        pBothInfo->FileAttributes    = SMB_LTOH32(pBothInfoPacked->FileAttributes);
        pBothInfo->FileNameLength    = ulFileNameLength;
        pBothInfo->EaSize            = SMB_LTOH32(pBothInfoPacked->EaSize);
        pBothInfo->ShortNameLength   = SMB_LTOH8(pBothInfoPacked->ShortNameLength);

        SMB_LTOHWSTR(pBothInfo->ShortName, pBothInfoPacked->ShortName, sizeof(pBothInfo->ShortName) / sizeof(WCHAR) - 1);
        SMB_LTOHWSTR(pBothInfo->FileName, pBothInfoPacked->FileName, ulFileNameLength / sizeof(WCHAR));
        
        pHandle->find.pCursor += SMB_LTOH32(pBothInfoPacked->NextEntryOffset);
        pHandle->find.usSearchCount--;

        ulLengthUsed = (ulFileNameLength + 1) * sizeof(WCHAR) + sizeof(*pBothInfo);

        /* Align next entry to 8 byte boundary */
        if (ulLengthUsed % 8)
        {
            ulLengthUsed += (8 - ulLengthUsed % 8);
        }

        *ppulNextOffset = &pBothInfo->NextEntryOffset;
        *pulLengthUsed = ulLengthUsed;
    }
    else
    {
        *pulLengthUsed = 0;
    }

    return status;
}
