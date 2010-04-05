/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        find.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Find/Search
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
NfsFindIdBothDirInformation(
    PNFS_EXEC_CONTEXT         pExecContext,
    PLWIO_NFS_FILE_2          pFile,
    PSMB2_FIND_REQUEST_HEADER pRequestHeader,
    PBYTE                     pDataBuffer,
    ULONG                     ulDataOffset,
    ULONG                     ulMaxDataLength,
    PULONG                    pulDataLength
    );

static
NTSTATUS
NfsMarshalIdBothDirInfoSearchResults(
    PLWIO_NFS_SEARCH_SPACE_2            pSearchSpace,
    PBYTE                               pBuffer,
    ULONG                               ulBytesAvailable,
    ULONG                               ulOffset,
    PULONG                              pulBytesUsed,
    PULONG                              pulSearchCount,
    PSMB2_FILE_ID_BOTH_DIR_INFO_HEADER* ppLastInfoHeader
    );

static
NTSTATUS
NfsFindBothDirInformation(
    PNFS_EXEC_CONTEXT         pExecContext,
    PLWIO_NFS_FILE_2          pFile,
    PSMB2_FIND_REQUEST_HEADER pRequestHeader,
    PBYTE                     pDataBuffer,
    ULONG                     ulDataOffset,
    ULONG                     ulMaxDataLength,
    PULONG                    pulDataLength
    );

static
NTSTATUS
NfsMarshalBothDirInfoSearchResults(
    PLWIO_NFS_SEARCH_SPACE_2         pSearchSpace,
    PBYTE                            pBuffer,
    ULONG                            ulBytesAvailable,
    ULONG                            ulOffset,
    PULONG                           pulBytesUsed,
    PULONG                           pulSearchCount,
    PSMB2_FILE_BOTH_DIR_INFO_HEADER* ppLastInfoHeader
    );

static
NTSTATUS
NfsFindIdFullDirInformation(
    PNFS_EXEC_CONTEXT         pExecContext,
    PLWIO_NFS_FILE_2          pFile,
    PSMB2_FIND_REQUEST_HEADER pRequestHeader,
    PBYTE                     pDataBuffer,
    ULONG                     ulDataOffset,
    ULONG                     ulMaxDataLength,
    PULONG                    pulDataLength
    );

static
NTSTATUS
NfsMarshalIdFullDirInfoSearchResults(
    PLWIO_NFS_SEARCH_SPACE_2            pSearchSpace,
    PBYTE                               pBuffer,
    ULONG                               ulBytesAvailable,
    ULONG                               ulOffset,
    PULONG                              pulBytesUsed,
    PULONG                              pulSearchCount,
    PSMB2_FILE_ID_FULL_DIR_INFO_HEADER* ppLastInfoHeader
    );

static
NTSTATUS
NfsFindFullDirInformation(
    PNFS_EXEC_CONTEXT         pExecContext,
    PLWIO_NFS_FILE_2          pFile,
    PSMB2_FIND_REQUEST_HEADER pRequestHeader,
    PBYTE                     pDataBuffer,
    ULONG                     ulDataOffset,
    ULONG                     ulMaxDataLength,
    PULONG                    pulDataLength
    );

static
NTSTATUS
NfsMarshalFullDirInfoSearchResults(
    PLWIO_NFS_SEARCH_SPACE_2         pSearchSpace,
    PBYTE                            pBuffer,
    ULONG                            ulBytesAvailable,
    ULONG                            ulOffset,
    PULONG                           pulBytesUsed,
    PULONG                           pulSearchCount,
    PSMB2_FILE_FULL_DIR_INFO_HEADER* ppLastInfoHeader
    );

static
NTSTATUS
NfsFindDirectoryInformation(
    PNFS_EXEC_CONTEXT         pExecContext,
    PLWIO_NFS_FILE_2          pFile,
    PSMB2_FIND_REQUEST_HEADER pRequestHeader,
    PBYTE                     pDataBuffer,
    ULONG                     ulDataOffset,
    ULONG                     ulMaxDataLength,
    PULONG                    pulDataLength
    );

static
NTSTATUS
NfsMarshalDirectoryInfoSearchResults(
    PLWIO_NFS_SEARCH_SPACE_2          pSearchSpace,
    PBYTE                             pBuffer,
    ULONG                             ulBytesAvailable,
    ULONG                             ulOffset,
    PULONG                            pulBytesUsed,
    PULONG                            pulSearchCount,
    PSMB2_FILE_DIRECTORY_INFO_HEADER* ppLastInfoHeader
    );

static
NTSTATUS
NfsFindNamesInformation(
    PNFS_EXEC_CONTEXT         pExecContext,
    PLWIO_NFS_FILE_2          pFile,
    PSMB2_FIND_REQUEST_HEADER pRequestHeader,
    PBYTE                     pDataBuffer,
    ULONG                     ulDataOffset,
    ULONG                     ulMaxDataLength,
    PULONG                    pulDataLength
    );

static
NTSTATUS
NfsMarshalNamesInfoSearchResults(
    PLWIO_NFS_SEARCH_SPACE_2      pSearchSpace,
    PBYTE                         pBuffer,
    ULONG                         ulBytesAvailable,
    ULONG                         ulOffset,
    PULONG                        pulBytesUsed,
    PULONG                        pulSearchCount,
    PSMB2_FILE_NAMES_INFO_HEADER* ppLastInfoHeader
    );

NTSTATUS
NfsProcessFind_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus                = STATUS_SUCCESS;
    PLWIO_NFS_CONNECTION       pConnection   = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PLWIO_NFS_SESSION_2 pSession     = NULL;
    PLWIO_NFS_TREE_2    pTree        = NULL;
    PLWIO_NFS_FILE_2    pFile        = NULL;
    IO_FILE_HANDLE      hFile2       = NULL;
    PSMB2_FIND_REQUEST_HEADER  pRequestHeader  = NULL; // Do not free
    PSMB2_FIND_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    UNICODE_STRING             wszFilename     = {0};
    PWSTR    pwszFilePath       = NULL;
    PWSTR    pwszFilesystemPath = NULL;
    BOOLEAN  bInLock           = FALSE;
    BOOLEAN  bShareInLock       = FALSE;
    PBYTE    pData             = NULL; // Do not free
    ULONG    ulMaxDataLength   = 0;
    ULONG    ulDataLength      = 0;
    BOOLEAN  bReopenSearch     = FALSE;
    BOOLEAN  bRestartScan      = FALSE;
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulDataOffset     = 0;
    ULONG ulBytesUsed      = 0;
    ULONG ulTotalBytesUsed = 0;
    PIO_ECP_LIST pEcpList = NULL;

    ntStatus = NfsConnection2FindSession_SMB_V2(
                    pCtxSmb2,
                    pConnection,
                    pSmbRequest->pHeader->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsSession2FindTree_SMB_V2(
                    pCtxSmb2,
                    pSession,
                    pSmbRequest->pHeader->ulTid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2UnmarshalFindRequest(
                    pSmbRequest,
                    &pRequestHeader,
                    &wszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsTree2FindFile_SMB_V2(
                    pCtxSmb2,
                    pTree,
                    &pRequestHeader->fid,
                    LwIsSetFlag(pSmbRequest->pHeader->ulFlags,
                                SMB2_FLAGS_RELATED_OPERATION),
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    if (pFile->pSearchSpace)
    {
        if (LwIsSetFlag(pRequestHeader->ucSearchFlags,SMB2_SEARCH_FLAGS_REOPEN))
        {
            bReopenSearch = TRUE;
        }
        else if (LwIsSetFlag(pRequestHeader->ucSearchFlags,SMB2_SEARCH_FLAGS_RESTART_SCAN))
        {
            bRestartScan = TRUE;

            if (wszFilename.Length &&
                (pFile->searchSpace.ulSearchPatternLength == wszFilename.Length) &&
                !memcmp((PBYTE)pFile->searchSpace.pwszSearchPatternRaw,
                        (PBYTE)wszFilename.Buffer,
                        wszFilename.Length))
            {
                // Search pattern has not changed
                bReopenSearch = TRUE;
            }
        }
    }

    if (bReopenSearch)
    {
        IO_STATUS_BLOCK ioStatusBlock = {0};

        ntStatus = NfsIoCreateFile(
                        pTree->pShareInfo,
                        &hFile2,
                        NULL,
                        &ioStatusBlock,
                        pSession->pIoSecurityContext,
                        pFile->pFilename,
                        NULL, /* pFile->pSecurityDescriptor */
                        NULL, /* pFile->pSecurityQOS */
                        pFile->desiredAccess,
                        0LL,
                        pFile->fileAttributes,
                        pFile->shareAccess,
                        pFile->createDisposition,
                        pFile->createOptions,
                        NULL, /* EA Buffer */
                        0,    /* EA Length */
                        &pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        if (pFile->hFile)
        {
            IoCloseFile(pFile->hFile);
            pFile->hFile = NULL;
        }

        pFile->hFile = hFile2;
        hFile2 = NULL;

        if (pFile->pSearchSpace)
        {
            if (pFile->searchSpace.pwszSearchPattern)
            {
                NfsFreeMemory(pFile->searchSpace.pwszSearchPattern);
                pFile->searchSpace.pwszSearchPattern = NULL;
            }

            if (pFile->searchSpace.pwszSearchPatternRaw)
            {
                NfsFreeMemory(pFile->searchSpace.pwszSearchPatternRaw);
                pFile->searchSpace.pwszSearchPatternRaw = NULL;
                pFile->searchSpace.pwszSearchPatternRef = NULL;
            }

            if (pFile->searchSpace.pFileInfo)
            {
                NfsFreeMemory(pFile->searchSpace.pFileInfo);
                pFile->searchSpace.pFileInfo = NULL;
            }

            memset(&pFile->searchSpace, 0, sizeof(pFile->searchSpace));

            pFile->pSearchSpace = NULL;
        }
    }
    else if (bRestartScan)
    {
        if (pFile->pSearchSpace &&
            (pFile->searchSpace.ucInfoClass != pRequestHeader->ucInfoClass))
        {
            pFile->searchSpace.pFileInfoCursor = NULL;
            pFile->searchSpace.ucInfoClass = pRequestHeader->ucInfoClass;
        }
    }

    if (!pFile->pSearchSpace)
    {
        pFile->searchSpace.ucInfoClass = pRequestHeader->ucInfoClass;
        pFile->searchSpace.ucSearchFlags = pRequestHeader->ucSearchFlags;
        pFile->searchSpace.ulFileIndex = pRequestHeader->ulFileIndex;
        pFile->searchSpace.bUseLongFilenames = TRUE;

        pFile->pSearchSpace = &pFile->searchSpace;

        if (wszFilename.Length)
        {
            wchar16_t wszBackSlash[] = {'\\', 0};

            ntStatus = NfsAllocateMemory(
                            wszFilename.Length + sizeof(wchar16_t),
                            (PVOID*)&pFile->searchSpace.pwszSearchPatternRaw);
            BAIL_ON_NT_STATUS(ntStatus);

            memcpy((PBYTE)pFile->searchSpace.pwszSearchPatternRaw,
                   (PBYTE)wszFilename.Buffer,
                   wszFilename.Length);

            pFile->searchSpace.ulSearchPatternLength = wszFilename.Length;

            pFile->searchSpace.pwszSearchPatternRef =
                            pFile->searchSpace.pwszSearchPatternRaw;

            if (pFile->searchSpace.pwszSearchPatternRef &&
                *pFile->searchSpace.pwszSearchPatternRef == wszBackSlash[0])
            {
                pFile->searchSpace.pwszSearchPatternRef++;
            }
        }

        LWIO_LOCK_RWMUTEX_SHARED(bShareInLock, &pTree->pShareInfo->mutex);

        ntStatus = NfsBuildFilePath(
                        pTree->pShareInfo->pwszPath,
                        pFile->pwszFilename,
                        &pwszFilePath);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_RWMUTEX(bShareInLock, &pTree->pShareInfo->mutex);

        ntStatus = NfsFinderBuildSearchPath(
                        pwszFilePath,
                        pFile->searchSpace.pwszSearchPatternRef,
                        &pwszFilesystemPath,
                        &pFile->searchSpace.pwszSearchPattern,
                        NULL);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_FIND,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pTree->ulTid,
                    pSession->ullUid,
                    0LL, /* Async Id */
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(pSmbRequest->pHeader->ulFlags,
                                SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    ntStatus = SMB2MarshalFindResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    NULL, /* No data */
                    0,
                    &ulDataOffset,
                    &pResponseHeader,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += ulBytesUsed;
    ulOffset         += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;
    ulTotalBytesUsed += ulBytesUsed;

    pData = pSmbResponse->pBuffer + ulDataOffset;

    ulMaxDataLength = SMB_MIN(pRequestHeader->ulOutBufferLength,
                              ulBytesAvailable);

    switch (pRequestHeader->ucInfoClass)
    {
		case SMB2_FILE_INFO_CLASS_ID_BOTH_DIR:

            ntStatus = NfsFindIdBothDirInformation(
                            pExecContext,
                            pFile,
                            pRequestHeader,
                            pData,
                            ulDataOffset,
                            ulMaxDataLength,
                            &ulDataLength);

            break;

		case SMB2_FILE_INFO_CLASS_ID_FULL_DIR:

            ntStatus = NfsFindIdFullDirInformation(
                            pExecContext,
                            pFile,
                            pRequestHeader,
                            pData,
                            ulDataOffset,
                            ulMaxDataLength,
                            &ulDataLength);

		    break;

        case SMB2_FILE_INFO_CLASS_BOTH_DIR:

            ntStatus = NfsFindBothDirInformation(
                            pExecContext,
                            pFile,
                            pRequestHeader,
                            pData,
                            ulDataOffset,
                            ulMaxDataLength,
                            &ulDataLength);

            break;

        case SMB2_FILE_INFO_CLASS_FULL_DIR:

            ntStatus = NfsFindFullDirInformation(
                            pExecContext,
                            pFile,
                            pRequestHeader,
                            pData,
                            ulDataOffset,
                            ulMaxDataLength,
                            &ulDataLength);

            break;

        case SMB2_FILE_INFO_CLASS_DIR:

            ntStatus = NfsFindDirectoryInformation(
                            pExecContext,
                            pFile,
                            pRequestHeader,
                            pData,
                            ulDataOffset,
                            ulMaxDataLength,
                            &ulDataLength);

            break;

        case SMB2_FILE_INFO_CLASS_NAMES:

            ntStatus = NfsFindNamesInformation(
                            pExecContext,
                            pFile,
                            pRequestHeader,
                            pData,
                            ulDataOffset,
                            ulMaxDataLength,
                            &ulDataLength);

            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER;

            break;
    }

    switch (ntStatus)
    {
        case STATUS_NO_MORE_MATCHES:

            pSmbResponse->pHeader->error = STATUS_NO_MORE_FILES;
            pResponseHeader->usOutBufferOffset = 0;
            pResponseHeader->ulOutBufferLength = 0;

            if (ulBytesAvailable < 1)
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ulDataLength = 1;

            *pData = 0xFF;

            if (ulDataLength % 8)
            {
                USHORT usAlign = 8 - (ulDataLength % 8);

                if (ulBytesAvailable < usAlign)
                {
                    ntStatus = STATUS_INVALID_BUFFER_SIZE;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                ulDataLength     += usAlign;
                ulOffset         += usAlign;
                ulBytesAvailable -= usAlign;
                ulTotalBytesUsed += usAlign;
            }

            ntStatus = STATUS_SUCCESS;

            break;

        case STATUS_SUCCESS:

            pOutBuffer       += ulDataLength;
            ulOffset         += ulDataLength;
            ulBytesAvailable -= ulDataLength;
            ulTotalBytesUsed += ulDataLength;

            pResponseHeader->usOutBufferOffset = ulDataOffset;
            pResponseHeader->ulOutBufferLength = ulDataLength;

            break;

        default:

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    if (pEcpList)
    {
        IoRtlEcpListFree(&pEcpList);
    }

    if (pwszFilesystemPath)
    {
        NfsFreeMemory(pwszFilesystemPath);
    }
    if (pwszFilePath)
    {
        NfsFreeMemory(pwszFilePath);
    }

    if (pFile)
    {
        NfsFile2Release(pFile);
    }

    if (pTree)
    {
        LWIO_UNLOCK_RWMUTEX(bShareInLock, &pTree->pShareInfo->mutex);

        NfsTree2Release(pTree);
    }

    if (pSession)
    {
        NfsSession2Release(pSession);
    }

    if (hFile2)
    {
        IoCloseFile(hFile2);
    }

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
NfsFindIdBothDirInformation(
    PNFS_EXEC_CONTEXT         pExecContext,
    PLWIO_NFS_FILE_2          pFile,
    PSMB2_FIND_REQUEST_HEADER pRequestHeader,
    PBYTE                     pDataBuffer,
    ULONG                     ulDataOffset,
    ULONG                     ulMaxDataLength,
    PULONG                    pulDataLength
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    PBYTE    pData = pDataBuffer;
    ULONG    ulDataLength = 0;
    ULONG    ulSearchResultCount = 0;
    ULONG    ulBytesUsed = 0;
    ULONG    ulOffset = ulDataOffset;
    ULONG    ulBytesAvailable = ulMaxDataLength;
    BOOLEAN  bEndOfSearch = FALSE;
    BOOLEAN  bRestartScan = LwIsSetFlag(
                                pRequestHeader->ucSearchFlags,
                                SMB2_SEARCH_FLAGS_RESTART_SCAN);
    BOOLEAN  bReturnSingleEntry = LwIsSetFlag(
                                    pRequestHeader->ucSearchFlags,
                                    SMB2_SEARCH_FLAGS_RETURN_SINGLE_ENTRY);
    PFILE_ID_BOTH_DIR_INFORMATION pFileInfoCursor = NULL;
    PSMB2_FILE_ID_BOTH_DIR_INFO_HEADER pLastInfoHeader = NULL; // Do not free
    PLWIO_NFS_SEARCH_SPACE_2 pSearchSpace = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    pSearchSpace = pFile->pSearchSpace;

    if (!pSearchSpace->pFileInfo)
    {
        // Keep space for 10 items
        USHORT usBytesAllocated = (sizeof(FILE_ID_BOTH_DIR_INFORMATION) +
                                   256 * sizeof(wchar16_t)) * 10;

        ntStatus = NfsAllocateMemory(
                        usBytesAllocated,
                        (PVOID*)&pSearchSpace->pFileInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pSearchSpace->usFileInfoLen = usBytesAllocated;
    }

    while (!bEndOfSearch && ulBytesAvailable)
    {
        ULONG ulIterSearchCount = 0;

        pFileInfoCursor = (PFILE_ID_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;
        if (!pFileInfoCursor)
        {
            IO_MATCH_FILE_SPEC ioFileSpec = {0};
            IO_STATUS_BLOCK ioStatusBlock = {0};

            ioFileSpec.Type = IO_MATCH_FILE_SPEC_TYPE_UNKNOWN;
            // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
            RtlUnicodeStringInit(
                            &ioFileSpec.Pattern,
                            pSearchSpace->pwszSearchPattern);

            pSearchSpace->pFileInfoCursor = NULL;

            do
            {
                memset(pSearchSpace->pFileInfo, 0x0, pSearchSpace->usFileInfoLen);

                ntStatus = IoQueryDirectoryFile(
                                pFile->hFile,
                                NULL,
                                &ioStatusBlock,
                                pSearchSpace->pFileInfo,
                                pSearchSpace->usFileInfoLen,
                                FileIdBothDirectoryInformation,
                                bReturnSingleEntry,
                                &ioFileSpec,
                                bRestartScan);
                if (ntStatus == STATUS_SUCCESS)
                {
                    pSearchSpace->pFileInfoCursor = pSearchSpace->pFileInfo;

                    break;
                }
                else if (ntStatus == STATUS_NO_MORE_MATCHES)
                {
                    bEndOfSearch = TRUE;
                    pSearchSpace->pFileInfoCursor = NULL;
                    ntStatus = STATUS_SUCCESS;

                    break;
                }
                else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
                {
                    USHORT usNewSize = pSearchSpace->usFileInfoLen + 256 * sizeof(wchar16_t);

                    ntStatus = SMBReallocMemory(
                                    pSearchSpace->pFileInfo,
                                    (PVOID*)&pSearchSpace->pFileInfo,
                                    usNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    memset(pSearchSpace->pFileInfo + pSearchSpace->usFileInfoLen,
                           0,
                           usNewSize - pSearchSpace->usFileInfoLen);

                    pSearchSpace->usFileInfoLen = usNewSize;

                    continue;
                }
                BAIL_ON_NT_STATUS(ntStatus);

            } while (TRUE);
        }

        if (pSearchSpace->pFileInfoCursor)
        {
            ntStatus = NfsMarshalIdBothDirInfoSearchResults(
                            pSearchSpace,
                            pDataBuffer + ulDataLength,
                            ulBytesAvailable,
                            ulOffset,
                            &ulBytesUsed,
                            &ulIterSearchCount,
                            &pLastInfoHeader);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!ulIterSearchCount)
            {
                break;
            }
            else
            {
                pData            += ulBytesUsed;
                ulDataLength     += ulBytesUsed;
                ulOffset         += ulBytesUsed;
                ulBytesAvailable -= ulBytesUsed;
            }

            ulSearchResultCount += ulIterSearchCount;

            if (bReturnSingleEntry && ulSearchResultCount)
            {
                break;
            }
        }
    }

    if (!ulSearchResultCount)
    {
        ntStatus = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pLastInfoHeader)
    {
        pLastInfoHeader->ulNextEntryOffset = 0;
    }

    *pulDataLength = ulDataLength;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return ntStatus;

error:

    *pulDataLength = 0;

    goto cleanup;
}

static
NTSTATUS
NfsMarshalIdBothDirInfoSearchResults(
    PLWIO_NFS_SEARCH_SPACE_2            pSearchSpace,
    PBYTE                               pBuffer,
    ULONG                               ulBytesAvailable,
    ULONG                               ulOffset,
    PULONG                              pulBytesUsed,
    PULONG                              pulSearchCount,
    PSMB2_FILE_ID_BOTH_DIR_INFO_HEADER* ppLastInfoHeader
    )
{
    NTSTATUS ntStatus = 0;
    PSMB2_FILE_ID_BOTH_DIR_INFO_HEADER pInfoHeader = *ppLastInfoHeader;
    PFILE_ID_BOTH_DIR_INFORMATION pFileInfoCursor = NULL;
    PBYTE pDataCursor = pBuffer;
    ULONG ulBytesUsed = 0;
    ULONG ulSearchCount = 0;
    ULONG iSearchCount = 0;
    ULONG ulDataOffset = ulOffset;

    pFileInfoCursor =
                   (PFILE_ID_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

    while (pFileInfoCursor && (ulBytesAvailable > 0))
    {
        ULONG ulInfoBytesRequired = 0;

        ulInfoBytesRequired = sizeof(SMB2_FILE_ID_BOTH_DIR_INFO_HEADER);
        ulDataOffset += sizeof(SMB2_FILE_ID_BOTH_DIR_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames)
        {
            ulInfoBytesRequired += pFileInfoCursor->FileNameLength;
            ulDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            ulInfoBytesRequired += sizeof(wchar16_t);
            ulDataOffset += sizeof(wchar16_t);
        }

        if (ulDataOffset % 8)
        {
            USHORT usAlignment = (8 - (ulDataOffset % 8));

            ulInfoBytesRequired += usAlignment;
            ulDataOffset += usAlignment;
        }

        if (ulBytesAvailable < ulInfoBytesRequired)
        {
            break;
        }

        ulSearchCount++;
        ulBytesAvailable -= ulInfoBytesRequired;
        ulBytesUsed += ulInfoBytesRequired;

        if (pFileInfoCursor->NextEntryOffset)
        {
            pFileInfoCursor =
                (PFILE_ID_BOTH_DIR_INFORMATION)(((PBYTE)pFileInfoCursor) +
                                              pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileInfoCursor = NULL;
        }
    }

    if (pInfoHeader)
    {
        ulDataOffset = pBuffer - (PBYTE)pInfoHeader;
    }

    for (; iSearchCount < ulSearchCount; iSearchCount++)
    {
        pFileInfoCursor = (PFILE_ID_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

        if (pInfoHeader)
        {
            pInfoHeader->ulNextEntryOffset = ulDataOffset;
        }

        pInfoHeader = (PSMB2_FILE_ID_BOTH_DIR_INFO_HEADER)pDataCursor;

        ulDataOffset = 0;

        pInfoHeader->ulFileIndex       = pFileInfoCursor->FileIndex;
        pInfoHeader->ulEaSize          = pFileInfoCursor->EaSize;
        pInfoHeader->ullCreationTime   = pFileInfoCursor->CreationTime;
        pInfoHeader->ullLastAccessTime = pFileInfoCursor->LastAccessTime;
        pInfoHeader->ullLastWriteTime  = pFileInfoCursor->LastWriteTime;
        pInfoHeader->ullChangeTime     = pFileInfoCursor->ChangeTime;
        pInfoHeader->ullEndOfFile      = pFileInfoCursor->EndOfFile;
        pInfoHeader->ullAllocationSize = pFileInfoCursor->AllocationSize;
        pInfoHeader->llFileId          = pFileInfoCursor->FileId;
        pInfoHeader->ulFileAttributes  = pFileInfoCursor->FileAttributes;
        pInfoHeader->ulFileNameLength  = pFileInfoCursor->FileNameLength;

        if (!pSearchSpace->bUseLongFilenames)
        {
            memcpy((PBYTE)&pInfoHeader->wszShortName[0],
                   (PBYTE)pFileInfoCursor->ShortName,
                   sizeof(pInfoHeader->wszShortName));

            pInfoHeader->ucShortNameLength = pFileInfoCursor->ShortNameLength;
        }
        else
        {
            memset((PBYTE)&pInfoHeader->wszShortName[0],
                   0x0,
                   sizeof(pInfoHeader->wszShortName));

            pInfoHeader->ucShortNameLength = 0;
        }

        pDataCursor += sizeof(SMB2_FILE_ID_BOTH_DIR_INFO_HEADER);
        ulDataOffset += sizeof(SMB2_FILE_ID_BOTH_DIR_INFO_HEADER);
        ulOffset += sizeof(SMB2_FILE_ID_BOTH_DIR_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames && pFileInfoCursor->FileNameLength)
        {
            memcpy(pDataCursor, (PBYTE)pFileInfoCursor->FileName, pFileInfoCursor->FileNameLength);

            pDataCursor += pFileInfoCursor->FileNameLength;
            ulOffset += pFileInfoCursor->FileNameLength;
            ulDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            pDataCursor += sizeof(wchar16_t);
            ulOffset += sizeof(wchar16_t);
            ulDataOffset += sizeof(wchar16_t);
        }

        if (ulOffset % 8)
        {
            USHORT usAlignment = (8 - (ulOffset % 8));

            pDataCursor += usAlignment;
            ulOffset += usAlignment;
            ulDataOffset += usAlignment;
        }

        if (pFileInfoCursor->NextEntryOffset)
        {
            pSearchSpace->pFileInfoCursor = (((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pSearchSpace->pFileInfoCursor = NULL;
        }
    }

    *pulSearchCount = ulSearchCount;
    *pulBytesUsed = ulBytesUsed;
    *ppLastInfoHeader = pInfoHeader;

    return ntStatus;
}

static
NTSTATUS
NfsFindBothDirInformation(
    PNFS_EXEC_CONTEXT         pExecContext,
    PLWIO_NFS_FILE_2          pFile,
    PSMB2_FIND_REQUEST_HEADER pRequestHeader,
    PBYTE                     pDataBuffer,
    ULONG                     ulDataOffset,
    ULONG                     ulMaxDataLength,
    PULONG                    pulDataLength
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    PBYTE    pData = pDataBuffer;
    ULONG    ulDataLength = 0;
    ULONG    ulSearchResultCount = 0;
    ULONG    ulBytesUsed = 0;
    ULONG    ulOffset = ulDataOffset;
    ULONG    ulBytesAvailable = ulMaxDataLength;
    BOOLEAN  bEndOfSearch = FALSE;
    BOOLEAN  bRestartScan = LwIsSetFlag(
                                pRequestHeader->ucSearchFlags,
                                SMB2_SEARCH_FLAGS_RESTART_SCAN);
    BOOLEAN  bReturnSingleEntry = LwIsSetFlag(
                                       pRequestHeader->ucSearchFlags,
                                       SMB2_SEARCH_FLAGS_RETURN_SINGLE_ENTRY);
    PFILE_BOTH_DIR_INFORMATION pFileInfoCursor = NULL;
    PSMB2_FILE_BOTH_DIR_INFO_HEADER pLastInfoHeader = NULL; // Do not free
    PLWIO_NFS_SEARCH_SPACE_2 pSearchSpace = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    pSearchSpace = pFile->pSearchSpace;

    if (!pSearchSpace->pFileInfo)
    {
        // Keep space for 10 items
        USHORT usBytesAllocated = (sizeof(FILE_BOTH_DIR_INFORMATION) +
                                   256 * sizeof(wchar16_t)) * 10;

        ntStatus = NfsAllocateMemory(
                        usBytesAllocated,
                        (PVOID*)&pSearchSpace->pFileInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pSearchSpace->usFileInfoLen = usBytesAllocated;
    }

    while (!bEndOfSearch && ulBytesAvailable)
    {
        ULONG ulIterSearchCount = 0;

        pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;
        if (!pFileInfoCursor)
        {
            IO_MATCH_FILE_SPEC ioFileSpec = {0};
            IO_STATUS_BLOCK ioStatusBlock = {0};

            ioFileSpec.Type = IO_MATCH_FILE_SPEC_TYPE_UNKNOWN;
            // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
            RtlUnicodeStringInit(
                            &ioFileSpec.Pattern,
                            pSearchSpace->pwszSearchPattern);

            pSearchSpace->pFileInfoCursor = NULL;

            do
            {
                memset(pSearchSpace->pFileInfo, 0x0, pSearchSpace->usFileInfoLen);

                ntStatus = IoQueryDirectoryFile(
                                pFile->hFile,
                                NULL,
                                &ioStatusBlock,
                                pSearchSpace->pFileInfo,
                                pSearchSpace->usFileInfoLen,
                                FileBothDirectoryInformation,
                                bReturnSingleEntry,
                                &ioFileSpec,
                                bRestartScan);
                if (ntStatus == STATUS_SUCCESS)
                {
                    pSearchSpace->pFileInfoCursor = pSearchSpace->pFileInfo;

                    break;
                }
                else if (ntStatus == STATUS_NO_MORE_MATCHES)
                {
                    bEndOfSearch = TRUE;
                    ntStatus = STATUS_SUCCESS;

                    break;
                }
                else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
                {
                    USHORT usNewSize = pSearchSpace->usFileInfoLen + 256 * sizeof(wchar16_t);

                    ntStatus = SMBReallocMemory(
                                    pSearchSpace->pFileInfo,
                                    (PVOID*)&pSearchSpace->pFileInfo,
                                    usNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    memset(pSearchSpace->pFileInfo + pSearchSpace->usFileInfoLen,
                           0,
                           usNewSize - pSearchSpace->usFileInfoLen);

                    pSearchSpace->usFileInfoLen = usNewSize;

                    continue;
                }
                BAIL_ON_NT_STATUS(ntStatus);

            } while (TRUE);
        }

        if (pSearchSpace->pFileInfoCursor)
        {
            ntStatus = NfsMarshalBothDirInfoSearchResults(
                            pSearchSpace,
                            pDataBuffer + ulDataLength,
                            ulBytesAvailable,
                            ulOffset,
                            &ulBytesUsed,
                            &ulIterSearchCount,
                            &pLastInfoHeader);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!ulIterSearchCount)
            {
                break;
            }
            else
            {
                pData            += ulBytesUsed;
                ulDataLength     += ulBytesUsed;
                ulOffset         += ulBytesUsed;
                ulBytesAvailable -= ulBytesUsed;
            }

            ulSearchResultCount += ulIterSearchCount;

            if (bReturnSingleEntry && ulSearchResultCount)
            {
                break;
            }
        }
    }

    if (!ulSearchResultCount)
    {
        ntStatus = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pLastInfoHeader)
    {
        pLastInfoHeader->ulNextEntryOffset = 0;
    }

    *pulDataLength = ulDataLength;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return ntStatus;

error:

    *pulDataLength = 0;

    goto cleanup;
}

static
NTSTATUS
NfsMarshalBothDirInfoSearchResults(
    PLWIO_NFS_SEARCH_SPACE_2         pSearchSpace,
    PBYTE                            pBuffer,
    ULONG                            ulBytesAvailable,
    ULONG                            ulOffset,
    PULONG                           pulBytesUsed,
    PULONG                           pulSearchCount,
    PSMB2_FILE_BOTH_DIR_INFO_HEADER* ppLastInfoHeader
    )
{
    NTSTATUS ntStatus = 0;
    PSMB2_FILE_BOTH_DIR_INFO_HEADER pInfoHeader = *ppLastInfoHeader;
    PFILE_BOTH_DIR_INFORMATION pFileInfoCursor = NULL;
    PBYTE pDataCursor = pBuffer;
    ULONG ulBytesUsed = 0;
    ULONG ulSearchCount = 0;
    ULONG iSearchCount = 0;
    ULONG ulDataOffset = ulOffset;

    pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

    while (pFileInfoCursor && (ulBytesAvailable > 0))
    {
        ULONG ulInfoBytesRequired = 0;

        ulInfoBytesRequired = sizeof(SMB2_FILE_BOTH_DIR_INFO_HEADER);
        ulDataOffset += sizeof(SMB2_FILE_BOTH_DIR_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames)
        {
            ulInfoBytesRequired += pFileInfoCursor->FileNameLength;
            ulDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            ulInfoBytesRequired += sizeof(wchar16_t);
            ulDataOffset += sizeof(wchar16_t);
        }

        if (ulDataOffset % 8)
        {
            USHORT usAlignment = (8 - (ulDataOffset % 8));

            ulInfoBytesRequired += usAlignment;
            ulDataOffset += usAlignment;
        }

        if (ulBytesAvailable < ulInfoBytesRequired)
        {
            break;
        }

        ulSearchCount++;
        ulBytesAvailable -= ulInfoBytesRequired;
        ulBytesUsed += ulInfoBytesRequired;

        if (pFileInfoCursor->NextEntryOffset)
        {
            pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)(((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileInfoCursor = NULL;
        }
    }

    if (pInfoHeader)
    {
        ulDataOffset = pBuffer - (PBYTE)pInfoHeader;
    }

    for (; iSearchCount < ulSearchCount; iSearchCount++)
    {
        pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

        if (pInfoHeader)
        {
            pInfoHeader->ulNextEntryOffset = ulDataOffset;
        }

        pInfoHeader = (PSMB2_FILE_BOTH_DIR_INFO_HEADER)pDataCursor;

        ulDataOffset = 0;

        pInfoHeader->ulFileIndex = pFileInfoCursor->FileIndex;
        pInfoHeader->ulEaSize = pFileInfoCursor->EaSize;
        pInfoHeader->ullCreationTime = pFileInfoCursor->CreationTime;
        pInfoHeader->ullLastAccessTime = pFileInfoCursor->LastAccessTime;
        pInfoHeader->ullLastWriteTime = pFileInfoCursor->LastWriteTime;
        pInfoHeader->ullChangeTime = pFileInfoCursor->ChangeTime;
        pInfoHeader->ullEndOfFile = pFileInfoCursor->EndOfFile;
        pInfoHeader->ullAllocationSize = pFileInfoCursor->AllocationSize;
        pInfoHeader->ulFileAttributes = pFileInfoCursor->FileAttributes;
        pInfoHeader->ulFileNameLength = pFileInfoCursor->FileNameLength;

        if (!pSearchSpace->bUseLongFilenames)
        {
            memcpy((PBYTE)&pInfoHeader->wszShortName[0],
                   (PBYTE)pFileInfoCursor->ShortName,
                   sizeof(pInfoHeader->wszShortName));

            pInfoHeader->usShortNameLength = pFileInfoCursor->ShortNameLength;
        }
        else
        {
            memset((PBYTE)&pInfoHeader->wszShortName[0],
                   0x0,
                   sizeof(pInfoHeader->wszShortName));

            pInfoHeader->usShortNameLength = 0;
        }

        pDataCursor += sizeof(SMB2_FILE_BOTH_DIR_INFO_HEADER);
        ulDataOffset += sizeof(SMB2_FILE_BOTH_DIR_INFO_HEADER);
        ulOffset += sizeof(SMB2_FILE_BOTH_DIR_INFO_HEADER);

        if (pSearchSpace->bUseLongFilenames && pFileInfoCursor->FileNameLength)
        {
            memcpy(pDataCursor, (PBYTE)pFileInfoCursor->FileName, pFileInfoCursor->FileNameLength);

            pDataCursor += pFileInfoCursor->FileNameLength;
            ulOffset += pFileInfoCursor->FileNameLength;
            ulDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (!pFileInfoCursor->FileNameLength)
        {
            pDataCursor += sizeof(wchar16_t);
            ulOffset += sizeof(wchar16_t);
            ulDataOffset += sizeof(wchar16_t);
        }

        if (ulOffset % 8)
        {
            USHORT usAlignment = (8 - (ulOffset % 8));

            pDataCursor += usAlignment;
            ulOffset += usAlignment;
            ulDataOffset += usAlignment;
        }

        if (pFileInfoCursor->NextEntryOffset)
        {
            pSearchSpace->pFileInfoCursor = (((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pSearchSpace->pFileInfoCursor = NULL;
        }
    }

    *pulSearchCount = ulSearchCount;
    *pulBytesUsed = ulBytesUsed;
    *ppLastInfoHeader = pInfoHeader;

    return ntStatus;
}

static
NTSTATUS
NfsFindIdFullDirInformation(
    PNFS_EXEC_CONTEXT         pExecContext,
    PLWIO_NFS_FILE_2          pFile,
    PSMB2_FIND_REQUEST_HEADER pRequestHeader,
    PBYTE                     pDataBuffer,
    ULONG                     ulDataOffset,
    ULONG                     ulMaxDataLength,
    PULONG                    pulDataLength
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    PBYTE    pData = pDataBuffer;
    ULONG    ulDataLength = 0;
    ULONG    ulSearchResultCount = 0;
    ULONG    ulBytesUsed = 0;
    ULONG    ulOffset = ulDataOffset;
    ULONG    ulBytesAvailable = ulMaxDataLength;
    BOOLEAN  bEndOfSearch = FALSE;
    BOOLEAN  bRestartScan = LwIsSetFlag(
                                pRequestHeader->ucSearchFlags,
                                SMB2_SEARCH_FLAGS_RESTART_SCAN);
    BOOLEAN  bReturnSingleEntry = LwIsSetFlag(
                                       pRequestHeader->ucSearchFlags,
                                       SMB2_SEARCH_FLAGS_RETURN_SINGLE_ENTRY);
    PFILE_ID_FULL_DIR_INFORMATION pFileInfoCursor = NULL;
    PSMB2_FILE_ID_FULL_DIR_INFO_HEADER pLastInfoHeader = NULL; // Do not free
    PLWIO_NFS_SEARCH_SPACE_2 pSearchSpace = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    pSearchSpace = pFile->pSearchSpace;

    if (!pSearchSpace->pFileInfo)
    {
        // Keep space for 10 items
        USHORT usBytesAllocated = (sizeof(FILE_ID_FULL_DIR_INFORMATION) +
                                   256 * sizeof(wchar16_t)) * 10;

        ntStatus = NfsAllocateMemory(
                        usBytesAllocated,
                        (PVOID*)&pSearchSpace->pFileInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pSearchSpace->usFileInfoLen = usBytesAllocated;
    }

    while (!bEndOfSearch && ulBytesAvailable)
    {
        ULONG ulIterSearchCount = 0;

        pFileInfoCursor = (PFILE_ID_FULL_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;
        if (!pFileInfoCursor)
        {
            IO_MATCH_FILE_SPEC ioFileSpec = {0};
            IO_STATUS_BLOCK ioStatusBlock = {0};

            ioFileSpec.Type = IO_MATCH_FILE_SPEC_TYPE_UNKNOWN;
            // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
            RtlUnicodeStringInit(
                            &ioFileSpec.Pattern,
                            pSearchSpace->pwszSearchPattern);

            pSearchSpace->pFileInfoCursor = NULL;

            do
            {
                memset(pSearchSpace->pFileInfo, 0x0, pSearchSpace->usFileInfoLen);

                ntStatus = IoQueryDirectoryFile(
                                pFile->hFile,
                                NULL,
                                &ioStatusBlock,
                                pSearchSpace->pFileInfo,
                                pSearchSpace->usFileInfoLen,
                                FileIdFullDirectoryInformation,
                                bReturnSingleEntry,
                                &ioFileSpec,
                                bRestartScan);
                if (ntStatus == STATUS_SUCCESS)
                {
                    pSearchSpace->pFileInfoCursor = pSearchSpace->pFileInfo;

                    break;
                }
                else if (ntStatus == STATUS_NO_MORE_MATCHES)
                {
                    bEndOfSearch = TRUE;
                    ntStatus = STATUS_SUCCESS;

                    break;
                }
                else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
                {
                    USHORT usNewSize = pSearchSpace->usFileInfoLen + 256 * sizeof(wchar16_t);

                    ntStatus = SMBReallocMemory(
                                    pSearchSpace->pFileInfo,
                                    (PVOID*)&pSearchSpace->pFileInfo,
                                    usNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    memset(pSearchSpace->pFileInfo + pSearchSpace->usFileInfoLen,
                           0,
                           usNewSize - pSearchSpace->usFileInfoLen);

                    pSearchSpace->usFileInfoLen = usNewSize;

                    continue;
                }
                BAIL_ON_NT_STATUS(ntStatus);

            } while (TRUE);
        }

        if (pSearchSpace->pFileInfoCursor)
        {
            ntStatus = NfsMarshalIdFullDirInfoSearchResults(
                            pSearchSpace,
                            pDataBuffer + ulDataLength,
                            ulBytesAvailable,
                            ulOffset,
                            &ulBytesUsed,
                            &ulIterSearchCount,
                            &pLastInfoHeader);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!ulIterSearchCount)
            {
                break;
            }
            else
            {
                pData            += ulBytesUsed;
                ulDataLength     += ulBytesUsed;
                ulOffset         += ulBytesUsed;
                ulBytesAvailable -= ulBytesUsed;
            }

            ulSearchResultCount += ulIterSearchCount;

            if (bReturnSingleEntry && ulSearchResultCount)
            {
                break;
            }
        }
    }

    if (!ulSearchResultCount)
    {
        ntStatus = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pLastInfoHeader)
    {
        pLastInfoHeader->ulNextEntryOffset = 0;
    }

    *pulDataLength = ulDataLength;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return ntStatus;

error:

    *pulDataLength = 0;

    goto cleanup;
}

static
NTSTATUS
NfsMarshalIdFullDirInfoSearchResults(
    PLWIO_NFS_SEARCH_SPACE_2            pSearchSpace,
    PBYTE                               pBuffer,
    ULONG                               ulBytesAvailable,
    ULONG                               ulOffset,
    PULONG                              pulBytesUsed,
    PULONG                              pulSearchCount,
    PSMB2_FILE_ID_FULL_DIR_INFO_HEADER* ppLastInfoHeader
    )
{
    NTSTATUS ntStatus = 0;
    PSMB2_FILE_ID_FULL_DIR_INFO_HEADER pInfoHeader = *ppLastInfoHeader;
    PFILE_ID_FULL_DIR_INFORMATION pFileInfoCursor = NULL;
    PBYTE pDataCursor = pBuffer;
    ULONG ulBytesUsed = 0;
    ULONG ulSearchCount = 0;
    ULONG iSearchCount = 0;
    ULONG ulDataOffset = ulOffset;

    pFileInfoCursor = (PFILE_ID_FULL_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

    while (pFileInfoCursor && (ulBytesAvailable > 0))
    {
        ULONG ulInfoBytesRequired = 0;

        ulInfoBytesRequired = sizeof(SMB2_FILE_ID_FULL_DIR_INFO_HEADER);
        ulDataOffset += sizeof(SMB2_FILE_ID_FULL_DIR_INFO_HEADER);

        ulInfoBytesRequired += pFileInfoCursor->FileNameLength;
        ulDataOffset += pFileInfoCursor->FileNameLength;

        if (!pFileInfoCursor->FileNameLength)
        {
            ulInfoBytesRequired += sizeof(wchar16_t);
            ulDataOffset += sizeof(wchar16_t);
        }

        if (ulDataOffset % 8)
        {
            USHORT usAlignment = (8 - (ulDataOffset % 8));

            ulInfoBytesRequired += usAlignment;
            ulDataOffset += usAlignment;
        }

        if (ulBytesAvailable < ulInfoBytesRequired)
        {
            break;
        }

        ulSearchCount++;
        ulBytesAvailable -= ulInfoBytesRequired;
        ulBytesUsed += ulInfoBytesRequired;

        if (pFileInfoCursor->NextEntryOffset)
        {
            pFileInfoCursor = (PFILE_ID_FULL_DIR_INFORMATION)(((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileInfoCursor = NULL;
        }
    }

    if (pInfoHeader)
    {
        ulDataOffset = pBuffer - (PBYTE)pInfoHeader;
    }

    for (; iSearchCount < ulSearchCount; iSearchCount++)
    {
        pFileInfoCursor = (PFILE_ID_FULL_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

        if (pInfoHeader)
        {
            pInfoHeader->ulNextEntryOffset = ulDataOffset;
        }

        pInfoHeader = (PSMB2_FILE_ID_FULL_DIR_INFO_HEADER)pDataCursor;

        ulDataOffset = 0;

        pInfoHeader->ulFileIndex       = pFileInfoCursor->FileIndex;
        pInfoHeader->ulEaSize          = pFileInfoCursor->EaSize;
        pInfoHeader->ullCreationTime   = pFileInfoCursor->CreationTime;
        pInfoHeader->ullLastAccessTime = pFileInfoCursor->LastAccessTime;
        pInfoHeader->ullLastWriteTime  = pFileInfoCursor->LastWriteTime;
        pInfoHeader->ullChangeTime     = pFileInfoCursor->ChangeTime;
        pInfoHeader->ullEndOfFile      = pFileInfoCursor->EndOfFile;
        pInfoHeader->ullAllocationSize = pFileInfoCursor->AllocationSize;
        pInfoHeader->ulFileAttributes  = pFileInfoCursor->FileAttributes;
        pInfoHeader->ulFileNameLength  = pFileInfoCursor->FileNameLength;
        pInfoHeader->llFileId          = pFileInfoCursor->FileId;

        pDataCursor  += sizeof(SMB2_FILE_ID_FULL_DIR_INFO_HEADER);
        ulDataOffset += sizeof(SMB2_FILE_ID_FULL_DIR_INFO_HEADER);
        ulOffset     += sizeof(SMB2_FILE_ID_FULL_DIR_INFO_HEADER);

        if (pFileInfoCursor->FileNameLength)
        {
            memcpy(pDataCursor, (PBYTE)pFileInfoCursor->FileName, pFileInfoCursor->FileNameLength);

            pDataCursor += pFileInfoCursor->FileNameLength;
            ulOffset += pFileInfoCursor->FileNameLength;
            ulDataOffset += pFileInfoCursor->FileNameLength;
        }
        else
        {
            pDataCursor += sizeof(wchar16_t);
            ulOffset += sizeof(wchar16_t);
            ulDataOffset += sizeof(wchar16_t);
        }

        if (ulOffset % 8)
        {
            USHORT usAlignment = (8 - (ulOffset % 8));

            pDataCursor += usAlignment;
            ulOffset += usAlignment;
            ulDataOffset += usAlignment;
        }

        if (pFileInfoCursor->NextEntryOffset)
        {
            pSearchSpace->pFileInfoCursor = (((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pSearchSpace->pFileInfoCursor = NULL;
        }
    }

    *pulSearchCount = ulSearchCount;
    *pulBytesUsed = ulBytesUsed;
    *ppLastInfoHeader = pInfoHeader;

    return ntStatus;
}

static
NTSTATUS
NfsFindFullDirInformation(
    PNFS_EXEC_CONTEXT         pExecContext,
    PLWIO_NFS_FILE_2          pFile,
    PSMB2_FIND_REQUEST_HEADER pRequestHeader,
    PBYTE                     pDataBuffer,
    ULONG                     ulDataOffset,
    ULONG                     ulMaxDataLength,
    PULONG                    pulDataLength
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    PBYTE    pData = pDataBuffer;
    ULONG    ulDataLength = 0;
    ULONG    ulSearchResultCount = 0;
    ULONG    ulBytesUsed = 0;
    ULONG    ulOffset = ulDataOffset;
    ULONG    ulBytesAvailable = ulMaxDataLength;
    BOOLEAN  bEndOfSearch = FALSE;
    BOOLEAN  bRestartScan = LwIsSetFlag(
                               pRequestHeader->ucSearchFlags,
                               SMB2_SEARCH_FLAGS_RESTART_SCAN);
    BOOLEAN  bReturnSingleEntry = LwIsSetFlag(
                                       pRequestHeader->ucSearchFlags,
                                       SMB2_SEARCH_FLAGS_RETURN_SINGLE_ENTRY);
    PFILE_FULL_DIR_INFORMATION pFileInfoCursor = NULL;
    PSMB2_FILE_FULL_DIR_INFO_HEADER pLastInfoHeader = NULL; // Do not free
    PLWIO_NFS_SEARCH_SPACE_2 pSearchSpace = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    pSearchSpace = pFile->pSearchSpace;

    if (!pSearchSpace->pFileInfo)
    {
        // Keep space for 10 items
        USHORT usBytesAllocated = (sizeof(FILE_FULL_DIR_INFORMATION) +
                                   256 * sizeof(wchar16_t)) * 10;

        ntStatus = NfsAllocateMemory(
                        usBytesAllocated,
                        (PVOID*)&pSearchSpace->pFileInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pSearchSpace->usFileInfoLen = usBytesAllocated;
    }

    while (!bEndOfSearch && ulBytesAvailable)
    {
        ULONG ulIterSearchCount = 0;

        pFileInfoCursor = (PFILE_FULL_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;
        if (!pFileInfoCursor)
        {
            IO_MATCH_FILE_SPEC ioFileSpec = {0};
            IO_STATUS_BLOCK ioStatusBlock = {0};

            ioFileSpec.Type = IO_MATCH_FILE_SPEC_TYPE_UNKNOWN;
            // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
            RtlUnicodeStringInit(
                            &ioFileSpec.Pattern,
                            pSearchSpace->pwszSearchPattern);

            pSearchSpace->pFileInfoCursor = NULL;

            do
            {
                memset(pSearchSpace->pFileInfo, 0x0, pSearchSpace->usFileInfoLen);

                ntStatus = IoQueryDirectoryFile(
                                pFile->hFile,
                                NULL,
                                &ioStatusBlock,
                                pSearchSpace->pFileInfo,
                                pSearchSpace->usFileInfoLen,
                                FileFullDirectoryInformation,
                                bReturnSingleEntry,
                                &ioFileSpec,
                                bRestartScan);
                if (ntStatus == STATUS_SUCCESS)
                {
                    pSearchSpace->pFileInfoCursor = pSearchSpace->pFileInfo;

                    break;
                }
                else if (ntStatus == STATUS_NO_MORE_MATCHES)
                {
                    bEndOfSearch = TRUE;
                    ntStatus = STATUS_SUCCESS;

                    break;
                }
                else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
                {
                    USHORT usNewSize = pSearchSpace->usFileInfoLen + 256 * sizeof(wchar16_t);

                    ntStatus = SMBReallocMemory(
                                    pSearchSpace->pFileInfo,
                                    (PVOID*)&pSearchSpace->pFileInfo,
                                    usNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    memset(pSearchSpace->pFileInfo + pSearchSpace->usFileInfoLen,
                           0,
                           usNewSize - pSearchSpace->usFileInfoLen);

                    pSearchSpace->usFileInfoLen = usNewSize;

                    continue;
                }
                BAIL_ON_NT_STATUS(ntStatus);

            } while (TRUE);
        }

        if (pSearchSpace->pFileInfoCursor)
        {
            ntStatus = NfsMarshalFullDirInfoSearchResults(
                            pSearchSpace,
                            pDataBuffer + ulDataLength,
                            ulBytesAvailable,
                            ulOffset,
                            &ulBytesUsed,
                            &ulIterSearchCount,
                            &pLastInfoHeader);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!ulIterSearchCount)
            {
                break;
            }
            else
            {
                pData            += ulBytesUsed;
                ulDataLength     += ulBytesUsed;
                ulOffset         += ulBytesUsed;
                ulBytesAvailable -= ulBytesUsed;
            }

            ulSearchResultCount += ulIterSearchCount;

            if (bReturnSingleEntry && ulSearchResultCount)
            {
                break;
            }
        }
    }

    if (!ulSearchResultCount)
    {
        ntStatus = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pLastInfoHeader)
    {
        pLastInfoHeader->ulNextEntryOffset = 0;
    }

    *pulDataLength = ulDataLength;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return ntStatus;

error:

    *pulDataLength = 0;

    goto cleanup;
}

static
NTSTATUS
NfsMarshalFullDirInfoSearchResults(
    PLWIO_NFS_SEARCH_SPACE_2         pSearchSpace,
    PBYTE                            pBuffer,
    ULONG                            ulBytesAvailable,
    ULONG                            ulOffset,
    PULONG                           pulBytesUsed,
    PULONG                           pulSearchCount,
    PSMB2_FILE_FULL_DIR_INFO_HEADER* ppLastInfoHeader
    )
{
    NTSTATUS ntStatus = 0;
    PSMB2_FILE_FULL_DIR_INFO_HEADER pInfoHeader = *ppLastInfoHeader;
    PFILE_FULL_DIR_INFORMATION pFileInfoCursor = NULL;
    PBYTE pDataCursor = pBuffer;
    ULONG ulBytesUsed = 0;
    ULONG ulSearchCount = 0;
    ULONG iSearchCount = 0;
    ULONG ulDataOffset = ulOffset;

    pFileInfoCursor = (PFILE_FULL_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

    while (pFileInfoCursor && (ulBytesAvailable > 0))
    {
        ULONG ulInfoBytesRequired = 0;

        ulInfoBytesRequired = sizeof(SMB2_FILE_FULL_DIR_INFO_HEADER);
        ulDataOffset += sizeof(SMB2_FILE_FULL_DIR_INFO_HEADER);

        ulInfoBytesRequired += pFileInfoCursor->FileNameLength;
        ulDataOffset += pFileInfoCursor->FileNameLength;

        if (!pFileInfoCursor->FileNameLength)
        {
            ulInfoBytesRequired += sizeof(wchar16_t);
            ulDataOffset += sizeof(wchar16_t);
        }

        if (ulDataOffset % 8)
        {
            USHORT usAlignment = (8 - (ulDataOffset % 8));

            ulInfoBytesRequired += usAlignment;
            ulDataOffset += usAlignment;
        }

        if (ulBytesAvailable < ulInfoBytesRequired)
        {
            break;
        }

        ulSearchCount++;
        ulBytesAvailable -= ulInfoBytesRequired;
        ulBytesUsed += ulInfoBytesRequired;

        if (pFileInfoCursor->NextEntryOffset)
        {
            pFileInfoCursor = (PFILE_FULL_DIR_INFORMATION)(((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileInfoCursor = NULL;
        }
    }

    if (pInfoHeader)
    {
        ulDataOffset = pBuffer - (PBYTE)pInfoHeader;
    }

    for (; iSearchCount < ulSearchCount; iSearchCount++)
    {
        pFileInfoCursor = (PFILE_FULL_DIR_INFORMATION)pSearchSpace->pFileInfoCursor;

        if (pInfoHeader)
        {
            pInfoHeader->ulNextEntryOffset = ulDataOffset;
        }

        pInfoHeader = (PSMB2_FILE_FULL_DIR_INFO_HEADER)pDataCursor;

        ulDataOffset = 0;

        pInfoHeader->ulFileIndex = pFileInfoCursor->FileIndex;
        pInfoHeader->ulEaSize = pFileInfoCursor->EaSize;
        pInfoHeader->ullCreationTime = pFileInfoCursor->CreationTime;
        pInfoHeader->ullLastAccessTime = pFileInfoCursor->LastAccessTime;
        pInfoHeader->ullLastWriteTime = pFileInfoCursor->LastWriteTime;
        pInfoHeader->ullChangeTime = pFileInfoCursor->ChangeTime;
        pInfoHeader->ullEndOfFile = pFileInfoCursor->EndOfFile;
        pInfoHeader->ullAllocationSize = pFileInfoCursor->AllocationSize;
        pInfoHeader->ulFileAttributes = pFileInfoCursor->FileAttributes;
        pInfoHeader->ulFileNameLength = pFileInfoCursor->FileNameLength;

        pDataCursor += sizeof(SMB2_FILE_FULL_DIR_INFO_HEADER);
        ulDataOffset += sizeof(SMB2_FILE_FULL_DIR_INFO_HEADER);
        ulOffset += sizeof(SMB2_FILE_FULL_DIR_INFO_HEADER);

        if (pFileInfoCursor->FileNameLength)
        {
            memcpy(pDataCursor, (PBYTE)pFileInfoCursor->FileName, pFileInfoCursor->FileNameLength);

            pDataCursor += pFileInfoCursor->FileNameLength;
            ulOffset += pFileInfoCursor->FileNameLength;
            ulDataOffset += pFileInfoCursor->FileNameLength;
        }
        else
        {
            pDataCursor += sizeof(wchar16_t);
            ulOffset += sizeof(wchar16_t);
            ulDataOffset += sizeof(wchar16_t);
        }

        if (ulOffset % 8)
        {
            USHORT usAlignment = (8 - (ulOffset % 8));

            pDataCursor += usAlignment;
            ulOffset += usAlignment;
            ulDataOffset += usAlignment;
        }

        if (pFileInfoCursor->NextEntryOffset)
        {
            pSearchSpace->pFileInfoCursor = (((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pSearchSpace->pFileInfoCursor = NULL;
        }
    }

    *pulSearchCount = ulSearchCount;
    *pulBytesUsed = ulBytesUsed;
    *ppLastInfoHeader = pInfoHeader;

    return ntStatus;
}

static
NTSTATUS
NfsFindDirectoryInformation(
    PNFS_EXEC_CONTEXT         pExecContext,
    PLWIO_NFS_FILE_2          pFile,
    PSMB2_FIND_REQUEST_HEADER pRequestHeader,
    PBYTE                     pDataBuffer,
    ULONG                     ulDataOffset,
    ULONG                     ulMaxDataLength,
    PULONG                    pulDataLength
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    PBYTE    pData = pDataBuffer;
    ULONG    ulDataLength = 0;
    ULONG    ulSearchResultCount = 0;
    ULONG    ulBytesUsed = 0;
    ULONG    ulOffset = ulDataOffset;
    ULONG    ulBytesAvailable = ulMaxDataLength;
    BOOLEAN  bEndOfSearch = FALSE;
    BOOLEAN  bRestartScan = LwIsSetFlag(
                               pRequestHeader->ucSearchFlags,
                               SMB2_SEARCH_FLAGS_RESTART_SCAN);
    BOOLEAN  bReturnSingleEntry = LwIsSetFlag(
                                       pRequestHeader->ucSearchFlags,
                                       SMB2_SEARCH_FLAGS_RETURN_SINGLE_ENTRY);
    PFILE_DIRECTORY_INFORMATION pFileInfoCursor = NULL;
    PSMB2_FILE_DIRECTORY_INFO_HEADER pLastInfoHeader = NULL; // Do not free
    PLWIO_NFS_SEARCH_SPACE_2 pSearchSpace = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    pSearchSpace = pFile->pSearchSpace;

    if (!pSearchSpace->pFileInfo)
    {
        // Keep space for 10 items
        USHORT usBytesAllocated = (sizeof(FILE_DIRECTORY_INFORMATION) +
                                   256 * sizeof(wchar16_t)) * 10;

        ntStatus = NfsAllocateMemory(
                        usBytesAllocated,
                        (PVOID*)&pSearchSpace->pFileInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pSearchSpace->usFileInfoLen = usBytesAllocated;
    }

    while (!bEndOfSearch && ulBytesAvailable)
    {
        ULONG ulIterSearchCount = 0;

        pFileInfoCursor = (PFILE_DIRECTORY_INFORMATION)pSearchSpace->pFileInfoCursor;
        if (!pFileInfoCursor)
        {
            IO_MATCH_FILE_SPEC ioFileSpec = {0};
            IO_STATUS_BLOCK ioStatusBlock = {0};

            ioFileSpec.Type = IO_MATCH_FILE_SPEC_TYPE_UNKNOWN;
            // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
            RtlUnicodeStringInit(
                            &ioFileSpec.Pattern,
                            pSearchSpace->pwszSearchPattern);

            pSearchSpace->pFileInfoCursor = NULL;

            do
            {
                memset(pSearchSpace->pFileInfo, 0x0, pSearchSpace->usFileInfoLen);

                ntStatus = IoQueryDirectoryFile(
                                pFile->hFile,
                                NULL,
                                &ioStatusBlock,
                                pSearchSpace->pFileInfo,
                                pSearchSpace->usFileInfoLen,
                                FileDirectoryInformation,
                                bReturnSingleEntry,
                                &ioFileSpec,
                                bRestartScan);
                if (ntStatus == STATUS_SUCCESS)
                {
                    pSearchSpace->pFileInfoCursor = pSearchSpace->pFileInfo;

                    break;
                }
                else if (ntStatus == STATUS_NO_MORE_MATCHES)
                {
                    bEndOfSearch = TRUE;
                    ntStatus = STATUS_SUCCESS;

                    break;
                }
                else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
                {
                    USHORT usNewSize = pSearchSpace->usFileInfoLen + 256 * sizeof(wchar16_t);

                    ntStatus = SMBReallocMemory(
                                    pSearchSpace->pFileInfo,
                                    (PVOID*)&pSearchSpace->pFileInfo,
                                    usNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    memset(pSearchSpace->pFileInfo + pSearchSpace->usFileInfoLen,
                           0,
                           usNewSize - pSearchSpace->usFileInfoLen);

                    pSearchSpace->usFileInfoLen = usNewSize;

                    continue;
                }
                BAIL_ON_NT_STATUS(ntStatus);

            } while (TRUE);
        }

        if (pSearchSpace->pFileInfoCursor)
        {
            ntStatus = NfsMarshalDirectoryInfoSearchResults(
                            pSearchSpace,
                            pDataBuffer + ulDataLength,
                            ulBytesAvailable,
                            ulOffset,
                            &ulBytesUsed,
                            &ulIterSearchCount,
                            &pLastInfoHeader);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!ulIterSearchCount)
            {
                break;
            }
            else
            {
                pData            += ulBytesUsed;
                ulDataLength     += ulBytesUsed;
                ulOffset         += ulBytesUsed;
                ulBytesAvailable -= ulBytesUsed;
            }

            ulSearchResultCount += ulIterSearchCount;

            if (bReturnSingleEntry && ulSearchResultCount)
            {
                break;
            }
        }
    }

    if (!ulSearchResultCount)
    {
        ntStatus = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pLastInfoHeader)
    {
        pLastInfoHeader->ulNextEntryOffset = 0;
    }

    *pulDataLength = ulDataLength;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return ntStatus;

error:

    *pulDataLength = 0;

    goto cleanup;
}

static
NTSTATUS
NfsMarshalDirectoryInfoSearchResults(
    PLWIO_NFS_SEARCH_SPACE_2          pSearchSpace,
    PBYTE                             pBuffer,
    ULONG                             ulBytesAvailable,
    ULONG                             ulOffset,
    PULONG                            pulBytesUsed,
    PULONG                            pulSearchCount,
    PSMB2_FILE_DIRECTORY_INFO_HEADER* ppLastInfoHeader
    )
{
    NTSTATUS ntStatus = 0;
    PSMB2_FILE_DIRECTORY_INFO_HEADER pInfoHeader = *ppLastInfoHeader;
    PFILE_DIRECTORY_INFORMATION pFileInfoCursor = NULL;
    PBYTE pDataCursor = pBuffer;
    ULONG ulBytesUsed = 0;
    ULONG ulSearchCount = 0;
    ULONG iSearchCount = 0;
    ULONG ulDataOffset = ulOffset;

    pFileInfoCursor = (PFILE_DIRECTORY_INFORMATION)pSearchSpace->pFileInfoCursor;

    while (pFileInfoCursor && (ulBytesAvailable > 0))
    {
        ULONG ulInfoBytesRequired = 0;

        ulInfoBytesRequired = sizeof(SMB2_FILE_DIRECTORY_INFO_HEADER);
        ulDataOffset += sizeof(SMB2_FILE_DIRECTORY_INFO_HEADER);

        ulInfoBytesRequired += pFileInfoCursor->FileNameLength;
        ulDataOffset += pFileInfoCursor->FileNameLength;

        if (!pFileInfoCursor->FileNameLength)
        {
            ulInfoBytesRequired += sizeof(wchar16_t);
            ulDataOffset += sizeof(wchar16_t);
        }

        if (ulDataOffset % 8)
        {
            USHORT usAlignment = (8 - (ulDataOffset % 8));

            ulInfoBytesRequired += usAlignment;
            ulDataOffset += usAlignment;
        }

        if (ulBytesAvailable < ulInfoBytesRequired)
        {
            break;
        }

        ulSearchCount++;
        ulBytesAvailable -= ulInfoBytesRequired;
        ulBytesUsed += ulInfoBytesRequired;

        if (pFileInfoCursor->NextEntryOffset)
        {
            pFileInfoCursor = (PFILE_DIRECTORY_INFORMATION)(((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileInfoCursor = NULL;
        }
    }

    if (pInfoHeader)
    {
        ulDataOffset = pBuffer - (PBYTE)pInfoHeader;
    }

    for (; iSearchCount < ulSearchCount; iSearchCount++)
    {
        pFileInfoCursor = (PFILE_DIRECTORY_INFORMATION)pSearchSpace->pFileInfoCursor;

        if (pInfoHeader)
        {
            pInfoHeader->ulNextEntryOffset = ulDataOffset;
        }

        pInfoHeader = (PSMB2_FILE_DIRECTORY_INFO_HEADER)pDataCursor;

        ulDataOffset = 0;

        pInfoHeader->ulFileIndex = pFileInfoCursor->FileIndex;
        pInfoHeader->ullCreationTime = pFileInfoCursor->CreationTime;
        pInfoHeader->ullLastAccessTime = pFileInfoCursor->LastAccessTime;
        pInfoHeader->ullLastWriteTime = pFileInfoCursor->LastWriteTime;
        pInfoHeader->ullChangeTime = pFileInfoCursor->ChangeTime;
        pInfoHeader->ullEndOfFile = pFileInfoCursor->EndOfFile;
        pInfoHeader->ullAllocationSize = pFileInfoCursor->AllocationSize;
        pInfoHeader->ulFileAttributes = pFileInfoCursor->FileAttributes;
        pInfoHeader->ulFileNameLength = pFileInfoCursor->FileNameLength;

        pDataCursor += sizeof(SMB2_FILE_DIRECTORY_INFO_HEADER);
        ulDataOffset += sizeof(SMB2_FILE_DIRECTORY_INFO_HEADER);
        ulOffset += sizeof(SMB2_FILE_DIRECTORY_INFO_HEADER);

        if (pFileInfoCursor->FileNameLength)
        {
            memcpy(pDataCursor, (PBYTE)pFileInfoCursor->FileName, pFileInfoCursor->FileNameLength);

            pDataCursor += pFileInfoCursor->FileNameLength;
            ulOffset += pFileInfoCursor->FileNameLength;
            ulDataOffset += pFileInfoCursor->FileNameLength;
        }
        else
        {
            pDataCursor += sizeof(wchar16_t);
            ulOffset += sizeof(wchar16_t);
            ulDataOffset += sizeof(wchar16_t);
        }

        if (ulOffset % 8)
        {
            USHORT usAlignment = (8 - (ulOffset % 8));

            pDataCursor += usAlignment;
            ulOffset += usAlignment;
            ulDataOffset += usAlignment;
        }

        if (pFileInfoCursor->NextEntryOffset)
        {
            pSearchSpace->pFileInfoCursor = (((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pSearchSpace->pFileInfoCursor = NULL;
        }
    }

    *pulSearchCount = ulSearchCount;
    *pulBytesUsed = ulBytesUsed;
    *ppLastInfoHeader = pInfoHeader;

    return ntStatus;
}

static
NTSTATUS
NfsFindNamesInformation(
    PNFS_EXEC_CONTEXT         pExecContext,
    PLWIO_NFS_FILE_2          pFile,
    PSMB2_FIND_REQUEST_HEADER pRequestHeader,
    PBYTE                     pDataBuffer,
    ULONG                     ulDataOffset,
    ULONG                     ulMaxDataLength,
    PULONG                    pulDataLength
    )
{
    NTSTATUS ntStatus            = 0;
    BOOLEAN  bInLock             = FALSE;
    PBYTE    pData               = pDataBuffer;
    ULONG    ulDataLength        = 0;
    ULONG    ulSearchResultCount = 0;
    ULONG    ulBytesUsed         = 0;
    ULONG    ulOffset            = ulDataOffset;
    ULONG    ulBytesAvailable    = ulMaxDataLength;
    BOOLEAN  bEndOfSearch = FALSE;
    BOOLEAN  bRestartScan = LwIsSetFlag(
                               pRequestHeader->ucSearchFlags,
                               SMB2_SEARCH_FLAGS_RESTART_SCAN);
    BOOLEAN  bReturnSingleEntry = LwIsSetFlag(
                                       pRequestHeader->ucSearchFlags,
                                       SMB2_SEARCH_FLAGS_RETURN_SINGLE_ENTRY);
    PFILE_NAMES_INFORMATION      pFileInfoCursor = NULL;
    PSMB2_FILE_NAMES_INFO_HEADER pLastInfoHeader = NULL; // Do not free
    PLWIO_NFS_SEARCH_SPACE_2     pSearchSpace = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    pSearchSpace = pFile->pSearchSpace;

    if (!pSearchSpace->pFileInfo)
    {
        // Keep space for 10 items
        USHORT usBytesAllocated = (sizeof(FILE_NAMES_INFORMATION) +
                                       256 * sizeof(wchar16_t)) * 10;

        ntStatus = NfsAllocateMemory(
                        usBytesAllocated,
                        (PVOID*)&pSearchSpace->pFileInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pSearchSpace->usFileInfoLen = usBytesAllocated;
    }

    while (!bEndOfSearch && ulBytesAvailable)
    {
        ULONG ulIterSearchCount = 0;

        pFileInfoCursor = (PFILE_NAMES_INFORMATION)pSearchSpace->pFileInfoCursor;
        if (!pFileInfoCursor)
        {
            IO_MATCH_FILE_SPEC ioFileSpec = {0};
            IO_STATUS_BLOCK ioStatusBlock = {0};

            ioFileSpec.Type = IO_MATCH_FILE_SPEC_TYPE_UNKNOWN;
            // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
            RtlUnicodeStringInit(
                            &ioFileSpec.Pattern,
                            pSearchSpace->pwszSearchPattern);

            pSearchSpace->pFileInfoCursor = NULL;

            do
            {
                memset(pSearchSpace->pFileInfo, 0x0, pSearchSpace->usFileInfoLen);

                ntStatus = IoQueryDirectoryFile(
                                pFile->hFile,
                                NULL,
                                &ioStatusBlock,
                                pSearchSpace->pFileInfo,
                                pSearchSpace->usFileInfoLen,
                                FileNamesInformation,
                                bReturnSingleEntry,
                                &ioFileSpec,
                                bRestartScan);
                if (ntStatus == STATUS_SUCCESS)
                {
                    pSearchSpace->pFileInfoCursor = pSearchSpace->pFileInfo;

                    break;
                }
                else if (ntStatus == STATUS_NO_MORE_MATCHES)
                {
                    bEndOfSearch = TRUE;
                    ntStatus = STATUS_SUCCESS;

                    break;
                }
                else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
                {
                    USHORT usNewSize = pSearchSpace->usFileInfoLen +
                                            256 * sizeof(wchar16_t);

                    ntStatus = SMBReallocMemory(
                                    pSearchSpace->pFileInfo,
                                    (PVOID*)&pSearchSpace->pFileInfo,
                                    usNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    memset(pSearchSpace->pFileInfo + pSearchSpace->usFileInfoLen,
                           0,
                           usNewSize - pSearchSpace->usFileInfoLen);

                    pSearchSpace->usFileInfoLen = usNewSize;

                    continue;
                }
                BAIL_ON_NT_STATUS(ntStatus);

            } while (TRUE);
        }

        if (pSearchSpace->pFileInfoCursor)
        {
            ntStatus = NfsMarshalNamesInfoSearchResults(
                            pSearchSpace,
                            pDataBuffer + ulDataLength,
                            ulBytesAvailable,
                            ulOffset,
                            &ulBytesUsed,
                            &ulIterSearchCount,
                            &pLastInfoHeader);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!ulIterSearchCount)
            {
                break;
            }
            else
            {
                pData            += ulBytesUsed;
                ulDataLength     += ulBytesUsed;
                ulOffset         += ulBytesUsed;
                ulBytesAvailable -= ulBytesUsed;
            }

            ulSearchResultCount += ulIterSearchCount;

            if (bReturnSingleEntry && ulSearchResultCount)
            {
                break;
            }
        }
    }

    if (!ulSearchResultCount)
    {
        ntStatus = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pLastInfoHeader)
    {
        pLastInfoHeader->ulNextEntryOffset = 0;
    }

    *pulDataLength = ulDataLength;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return ntStatus;

error:

    *pulDataLength = 0;

    goto cleanup;
}

static
NTSTATUS
NfsMarshalNamesInfoSearchResults(
    PLWIO_NFS_SEARCH_SPACE_2      pSearchSpace,
    PBYTE                         pBuffer,
    ULONG                         ulBytesAvailable,
    ULONG                         ulOffset,
    PULONG                        pulBytesUsed,
    PULONG                        pulSearchCount,
    PSMB2_FILE_NAMES_INFO_HEADER* ppLastInfoHeader
    )
{
    NTSTATUS ntStatus = 0;
    PSMB2_FILE_NAMES_INFO_HEADER pInfoHeader = *ppLastInfoHeader;
    PFILE_NAMES_INFORMATION      pFileInfoCursor = NULL;
    PBYTE pDataCursor   = pBuffer;
    ULONG ulBytesUsed   = 0;
    ULONG ulSearchCount = 0;
    ULONG iSearchCount  = 0;
    ULONG ulDataOffset  = ulOffset;

    pFileInfoCursor = (PFILE_NAMES_INFORMATION)pSearchSpace->pFileInfoCursor;

    while (pFileInfoCursor && (ulBytesAvailable > 0))
    {
        ULONG ulInfoBytesRequired = 0;

        ulInfoBytesRequired = sizeof(SMB2_FILE_NAMES_INFO_HEADER);
        ulDataOffset += sizeof(SMB2_FILE_NAMES_INFO_HEADER);

        if (!pFileInfoCursor->FileNameLength)
        {
            ulInfoBytesRequired += sizeof(wchar16_t);
            ulDataOffset += sizeof(wchar16_t);
        }
        else
        {
            ulInfoBytesRequired +=  pFileInfoCursor->FileNameLength;

            ulDataOffset += pFileInfoCursor->FileNameLength;
        }

        if (ulDataOffset % 8)
        {
            USHORT usAlignment = (8 - (ulDataOffset % 8));

            ulInfoBytesRequired += usAlignment;
            ulDataOffset += usAlignment;
        }

        if (ulBytesAvailable < ulInfoBytesRequired)
        {
            break;
        }

        ulSearchCount++;
        ulBytesAvailable -= ulInfoBytesRequired;
        ulBytesUsed += ulInfoBytesRequired;

        if (pFileInfoCursor->NextEntryOffset)
        {
            pFileInfoCursor = (PFILE_NAMES_INFORMATION)(((PBYTE)pFileInfoCursor)
                                    + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileInfoCursor = NULL;
        }
    }

    if (pInfoHeader)
    {
        ulDataOffset = pBuffer - (PBYTE)pInfoHeader;
    }

    for (; iSearchCount < ulSearchCount; iSearchCount++)
    {
        pFileInfoCursor = (PFILE_NAMES_INFORMATION)pSearchSpace->pFileInfoCursor;

        if (pInfoHeader)
        {
            pInfoHeader->ulNextEntryOffset = ulDataOffset;
        }

        pInfoHeader = (PSMB2_FILE_NAMES_INFO_HEADER)pDataCursor;

        ulDataOffset = 0;

        pInfoHeader->ulFileIndex = pFileInfoCursor->FileIndex;
        pInfoHeader->ulFileNameLength = pFileInfoCursor->FileNameLength;

        pDataCursor += sizeof(SMB2_FILE_NAMES_INFO_HEADER);
        ulDataOffset += sizeof(SMB2_FILE_NAMES_INFO_HEADER);
        ulOffset += sizeof(SMB2_FILE_NAMES_INFO_HEADER);

        if (pInfoHeader->ulFileNameLength)
        {
            memcpy( pDataCursor,
                    (PBYTE)pFileInfoCursor->FileName,
                    pInfoHeader->ulFileNameLength);

            pDataCursor  += pInfoHeader->ulFileNameLength;
            ulOffset     += pInfoHeader->ulFileNameLength;
            ulDataOffset += pInfoHeader->ulFileNameLength;
        }
        else
        {
            pDataCursor += sizeof(wchar16_t);
            ulOffset += sizeof(wchar16_t);
            ulDataOffset += sizeof(wchar16_t);
        }

        if (ulOffset % 8)
        {
            USHORT usAlignment = (8 - (ulOffset % 8));

            pDataCursor += usAlignment;
            ulOffset += usAlignment;
            ulDataOffset += usAlignment;
        }

        if (pFileInfoCursor->NextEntryOffset)
        {
            pSearchSpace->pFileInfoCursor = (((PBYTE)pFileInfoCursor) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pSearchSpace->pFileInfoCursor = NULL;
        }
    }

    *pulSearchCount = ulSearchCount;
    *pulBytesUsed = ulBytesUsed;
    *ppLastInfoHeader = pInfoHeader;

    return ntStatus;
}

