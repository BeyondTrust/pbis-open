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
 *        setinfo.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Set Information
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
NfsBuildSetInfoState_SMB_V2(
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PLWIO_NFS_FILE_2              pFile,
    PBYTE                         pData,
    PNFS_SET_INFO_STATE_SMB_V2*   ppSetInfoState
    );

static
NTSTATUS
NfsSetInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildSetInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsSetFileInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildSetFileInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsSetFileBasicInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsSetFileEOFInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsSetFileDispositionInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsSetFileRenameInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsUnmarshalRenameHeader_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsSetFileAllocationInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildSetInfoCommonResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsSetFileSystemInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildSetFileSystemInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsSetSecurityInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsSetSecurityDescriptor_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
NfsBuildSetSecurityInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

static
VOID
NfsPrepareSetInfoStateAsync_SMB_V2(
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState,
    PNFS_EXEC_CONTEXT          pExecContext
    );

static
VOID
NfsExecuteSetInfoAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
NfsReleaseSetInfoStateAsync_SMB_V2(
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState
    );

static
VOID
NfsReleaseSetInfoStateHandle_SMB_V2(
    HANDLE hSetInfoState
    );

static
VOID
NfsReleaseSetInfoState_SMB_V2(
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState
    );

static
VOID
NfsFreeSetInfoState_SMB_V2(
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState
    );

NTSTATUS
NfsProcessSetInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_NFS_CONNECTION       pConnection   = pExecContext->pConnection;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;
    PLWIO_NFS_SESSION_2        pSession      = NULL;
    PLWIO_NFS_TREE_2           pTree         = NULL;
    PLWIO_NFS_FILE_2           pFile         = NULL;
    BOOLEAN                    bInLock       = FALSE;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    if (pSetInfoState)
    {
        InterlockedIncrement(&pSetInfoState->refCount);
    }
    else
    {
        ULONG                      iMsg          = pCtxSmb2->iMsg;
        PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
        PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PBYTE                         pData          = NULL; // Do not free

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

        ntStatus = SMB2UnmarshalSetInfoRequest(
                        pSmbRequest,
                        &pRequestHeader,
                        &pData);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NfsTree2FindFile_SMB_V2(
                            pCtxSmb2,
                            pTree,
                            &pRequestHeader->fid,
                            LwIsSetFlag(
                                pSmbRequest->pHeader->ulFlags,
                                SMB2_FLAGS_RELATED_OPERATION),
                            &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NfsBuildSetInfoState_SMB_V2(
                            pRequestHeader,
                            pFile,
                            pData,
                            &pSetInfoState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pSetInfoState;
        InterlockedIncrement(&pSetInfoState->refCount);
        pCtxSmb2->pfnStateRelease = &NfsReleaseSetInfoStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pSetInfoState->mutex);

    switch (pSetInfoState->stage)
    {
        case NFS_SET_INFO_STAGE_SMB_V2_INITIAL:

            pSetInfoState->stage = NFS_GET_INFO_STAGE_SMB_V2_ATTEMPT_IO;

            // intentional fall through

        case NFS_SET_INFO_STAGE_SMB_V2_ATTEMPT_IO:

            ntStatus = NfsSetInfo_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pSetInfoState->stage = NFS_SET_INFO_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case NFS_SET_INFO_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = NfsBuildSetInfoResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pSetInfoState->stage = NFS_SET_INFO_STAGE_SMB_V2_DONE;

        case NFS_SET_INFO_STAGE_SMB_V2_DONE:

            break;
    }

cleanup:

    if (pFile)
    {
        NfsFile2Release(pFile);
    }

    if (pTree)
    {
        NfsTree2Release(pTree);
    }

    if (pSession)
    {
        NfsSession2Release(pSession);
    }

    if (pSetInfoState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pSetInfoState->mutex);

        NfsReleaseSetInfoState_SMB_V2(pSetInfoState);
    }

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_PENDING:

            // TODO: Add an indicator to the file object to trigger a
            //       cleanup if the connection gets closed and all the
            //       files involved have to be closed

            break;

        default:

            if (pSetInfoState)
            {
                NfsReleaseSetInfoStateAsync_SMB_V2(pSetInfoState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
NfsBuildSetInfoState_SMB_V2(
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PLWIO_NFS_FILE_2              pFile,
    PBYTE                         pData,
    PNFS_SET_INFO_STATE_SMB_V2*   ppSetInfoState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_SET_INFO_STATE_SMB_V2),
                    (PVOID*)&pSetInfoState);
    BAIL_ON_NT_STATUS(ntStatus);

    pSetInfoState->refCount = 1;

    pthread_mutex_init(&pSetInfoState->mutex, NULL);
    pSetInfoState->pMutex = &pSetInfoState->mutex;

    pSetInfoState->stage = NFS_SET_INFO_STAGE_SMB_V2_INITIAL;

    pSetInfoState->pRequestHeader = pRequestHeader;
    pSetInfoState->pData          = pData;

    pSetInfoState->pFile = NfsFile2Acquire(pFile);

    *ppSetInfoState = pSetInfoState;

cleanup:

    return ntStatus;

error:

    *ppSetInfoState = NULL;

    if (pSetInfoState)
    {
        NfsFreeSetInfoState_SMB_V2(pSetInfoState);
    }

    goto cleanup;
}

static
NTSTATUS
NfsSetInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pSetInfoState->pRequestHeader->ucInfoType)
    {
        case SMB2_INFO_TYPE_FILE:

            ntStatus = NfsSetFileInfo_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_FILE_SYSTEM:

            ntStatus = NfsSetFileSystemInfo_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_SECURITY:

            ntStatus = NfsSetSecurityInfo_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
NfsBuildSetInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pSetInfoState->pRequestHeader->ucInfoType)
    {
        case SMB2_INFO_TYPE_FILE:

            ntStatus = NfsBuildSetFileInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_FILE_SYSTEM:

            ntStatus = NfsBuildSetFileSystemInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_SECURITY:

            ntStatus = NfsBuildSetSecurityInfoResponse_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
NfsSetFileInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pSetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_FILE_INFO_CLASS_POSITION :
        case SMB2_FILE_INFO_CLASS_MODE :
        case SMB2_FILE_INFO_CLASS_PIPE :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        case SMB2_FILE_INFO_CLASS_EOF :

            ntStatus = NfsSetFileEOFInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_DISPOSITION :

            ntStatus = NfsSetFileDispositionInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_RENAME :

            ntStatus = NfsSetFileRenameInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_BASIC :

            ntStatus = NfsSetFileBasicInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALLOCATION :

            ntStatus = NfsSetFileAllocationInfo_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
NfsBuildSetFileInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pSetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_FILE_INFO_CLASS_POSITION :
        case SMB2_FILE_INFO_CLASS_MODE :
        case SMB2_FILE_INFO_CLASS_PIPE :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        case SMB2_FILE_INFO_CLASS_EOF :
        case SMB2_FILE_INFO_CLASS_DISPOSITION :
        case SMB2_FILE_INFO_CLASS_RENAME :
        case SMB2_FILE_INFO_CLASS_BASIC :
        case SMB2_FILE_INFO_CLASS_ALLOCATION :

            ntStatus = NfsBuildSetInfoCommonResponse_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
NfsSetFileEOFInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT   pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2     pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2   pSetInfoState = NULL;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (pSetInfoState->pRequestHeader->ulInputBufferLen <
                        sizeof(FILE_END_OF_FILE_INFORMATION))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        NfsPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

        ntStatus = IoSetInformationFile(
                        pSetInfoState->pFile->hFile,
                        pSetInfoState->pAcb,
                        &pSetInfoState->ioStatusBlock,
                        (PFILE_END_OF_FILE_INFORMATION)pSetInfoState->pData,
                        sizeof(FILE_END_OF_FILE_INFORMATION),
                        FileEndOfFileInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        NfsReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync

error:

    return ntStatus;
}

static
NTSTATUS
NfsSetFileDispositionInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT   pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2     pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2   pSetInfoState = NULL;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (pSetInfoState->pRequestHeader->ulInputBufferLen <
                    sizeof(FILE_DISPOSITION_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    NfsPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

    ntStatus = IoSetInformationFile(
                    pSetInfoState->pFile->hFile,
                    pSetInfoState->pAcb,
                    &pSetInfoState->ioStatusBlock,
                    (PFILE_DISPOSITION_INFORMATION)pSetInfoState->pData,
                    sizeof(FILE_DISPOSITION_INFORMATION),
                    FileDispositionInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    NfsReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync

error:

    return ntStatus;
}

static
NTSTATUS
NfsSetFileRenameInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT   pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2     pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2   pSetInfoState = NULL;
    BOOLEAN                      bShareInLock   = FALSE;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (!pSetInfoState->pData2)
    {
        ntStatus = NfsUnmarshalRenameHeader_SMB_V2(pExecContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pSetInfoState->pRootDir)
    {
        ((PFILE_RENAME_INFORMATION)pSetInfoState->pData2)->RootDirectory =
                                                pSetInfoState->pRootDir->hFile;
    }
    else if (!pSetInfoState->hDir)
    {
        LWIO_LOCK_RWMUTEX_SHARED(   bShareInLock,
                                    &pCtxSmb2->pTree->pShareInfo->mutex);

        ntStatus = NfsAllocateStringW(
                        pCtxSmb2->pTree->pShareInfo->pwszPath,
                        &pSetInfoState->dirPath.FileName);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_RWMUTEX(bShareInLock,
                            &pCtxSmb2->pTree->pShareInfo->mutex);

        // Catch failed CreateFile calls when they come back around

        ntStatus = pSetInfoState->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);

        NfsPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

        ntStatus = NfsIoCreateFile(
                                pCtxSmb2->pTree->pShareInfo,
                                &pSetInfoState->hDir,
                                pSetInfoState->pAcb,
                                &pSetInfoState->ioStatusBlock,
                                pCtxSmb2->pSession->pIoSecurityContext,
                                &pSetInfoState->dirPath,
                                (PSECURITY_DESCRIPTOR_RELATIVE)pSetInfoState->pSecurityDescriptor,
                                pSetInfoState->pSecurityQOS,
                                GENERIC_READ,
                                0,
                                FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                FILE_OPEN,
                                FILE_DIRECTORY_FILE,
                                NULL, /* EA Buffer */
                                0,    /* EA Length */
                                &pSetInfoState->pEcpList
                                );
        BAIL_ON_NT_STATUS(ntStatus);

        NfsReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync
    }

    if (!pSetInfoState->pRootDir)
    {
        ((PFILE_RENAME_INFORMATION)pSetInfoState->pData2)->RootDirectory =
                                                pSetInfoState->hDir;
    }

    NfsPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

    ntStatus = IoSetInformationFile(
                    pSetInfoState->pFile->hFile,
                    pSetInfoState->pAcb,
                    &pSetInfoState->ioStatusBlock,
                    (PFILE_RENAME_INFORMATION)pSetInfoState->pData2,
                    pSetInfoState->ulData2Length,
                    FileRenameInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    NfsReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync

error:

    LWIO_UNLOCK_RWMUTEX(bShareInLock, &pCtxSmb2->pTree->pShareInfo->mutex);

    return ntStatus;
}

static
NTSTATUS
NfsUnmarshalRenameHeader_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;
    BOOLEAN                    bTreeInLock   = FALSE;
    PSMB2_FILE_RENAME_INFO_HEADER pRenameInfoHeader = NULL;
    ULONG                         ulBytesAvailable  = 0;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (pSetInfoState->pRequestHeader->ulInputBufferLen <
                    sizeof(SMB2_FILE_RENAME_INFO_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRenameInfoHeader = (PSMB2_FILE_RENAME_INFO_HEADER)pSetInfoState->pData;

    ulBytesAvailable = pSetInfoState->pRequestHeader->ulInputBufferLen;
    ulBytesAvailable -= sizeof(SMB2_FILE_RENAME_INFO_HEADER);
    ulBytesAvailable += sizeof(wchar16_t);

    if (ulBytesAvailable < pRenameInfoHeader->ulFileNameLength)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pRenameInfoHeader->ullRootDir)
    {
        // TODO: Figure out if this one is a persistent or volatile fid
        SMB2_FID rootDirFid = { .ullPersistentId = 0xFFFFFFFFFFFFFFFFLL,
                                .ullVolatileId   = pRenameInfoHeader->ullRootDir
                              };

        LWIO_LOCK_RWMUTEX_SHARED(bTreeInLock, &pCtxSmb2->pTree->mutex);

        ntStatus = NfsTree2FindFile(
                        pCtxSmb2->pTree,
                        &rootDirFid,
                        &pSetInfoState->pRootDir);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pCtxSmb2->pTree->mutex);
    }

    pSetInfoState->ulData2Length =
          sizeof(FILE_RENAME_INFORMATION) + pRenameInfoHeader->ulFileNameLength;

    ntStatus = NfsAllocateMemory(
                    pSetInfoState->ulData2Length,
                    (PVOID*)&pSetInfoState->pData2);
    BAIL_ON_NT_STATUS(ntStatus);

    ((PFILE_RENAME_INFORMATION)pSetInfoState->pData2)->ReplaceIfExists =
                    pRenameInfoHeader->ucReplaceIfExists;
    ((PFILE_RENAME_INFORMATION)pSetInfoState->pData2)->FileNameLength =
                    pRenameInfoHeader->ulFileNameLength;

    memcpy((PBYTE)((PFILE_RENAME_INFORMATION)pSetInfoState->pData2)->FileName,
           (PBYTE)pRenameInfoHeader->wszFileName,
           pRenameInfoHeader->ulFileNameLength);

error:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pCtxSmb2->pTree->mutex);

    return ntStatus;
}

static
NTSTATUS
NfsSetFileBasicInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT   pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2     pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2   pSetInfoState = NULL;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (pSetInfoState->pRequestHeader->ulInputBufferLen <
                    sizeof(FILE_BASIC_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    NfsPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

    ntStatus = IoSetInformationFile(
                    pSetInfoState->pFile->hFile,
                    pSetInfoState->pAcb,
                    &pSetInfoState->ioStatusBlock,
                    (PFILE_BASIC_INFORMATION)pSetInfoState->pData,
                    sizeof(FILE_BASIC_INFORMATION),
                    FileBasicInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    NfsReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync

error:

    return ntStatus;
}

static
NTSTATUS
NfsSetFileAllocationInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT   pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2     pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2   pSetInfoState = NULL;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (pSetInfoState->pRequestHeader->ulInputBufferLen <
                    sizeof(FILE_ALLOCATION_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    NfsPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

    ntStatus = IoSetInformationFile(
                    pSetInfoState->pFile->hFile,
                    pSetInfoState->pAcb,
                    &pSetInfoState->ioStatusBlock,
                    (PFILE_ALLOCATION_INFORMATION)pSetInfoState->pData,
                    sizeof(FILE_ALLOCATION_INFORMATION),
                    FileAllocationInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    NfsReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync

error:

    return ntStatus;
}

static
NTSTATUS
NfsBuildSetInfoCommonResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PNFS_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PNFS_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PSMB2_SET_INFO_RESPONSE_HEADER pResponseHeader = NULL; // Do not free

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_SETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL, /* Async Id */
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_SET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB2_SET_INFO_RESPONSE_HEADER)pOutBuffer;
    pResponseHeader->usLength = sizeof(SMB2_SET_INFO_RESPONSE_HEADER);

    // pOutBuffer += sizeof(SMB2_SET_INFO_RESPONSE_HEADER);
    // ulOffset += sizeof(SMB2_SET_INFO_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(SMB2_SET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_SET_INFO_RESPONSE_HEADER);

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

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
NfsSetFileSystemInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
NfsBuildSetFileSystemInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
NfsSetSecurityInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pSetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_SEC_INFO_CLASS_BASIC:

            ntStatus = NfsSetSecurityDescriptor_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
NfsSetSecurityDescriptor_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus      = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT   pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2     pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2   pSetInfoState = NULL;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (!pSetInfoState->pRequestHeader->ulAdditionalInfo ||
        !pSetInfoState->pRequestHeader->ulInputBufferLen)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    NfsPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

    ntStatus = IoSetSecurityFile(
                    pSetInfoState->pFile->hFile,
                    pSetInfoState->pAcb,
                    &pSetInfoState->ioStatusBlock,
                    pSetInfoState->pRequestHeader->ulAdditionalInfo,
                    (PSECURITY_DESCRIPTOR_RELATIVE)pSetInfoState->pData,
                    pSetInfoState->pRequestHeader->ulInputBufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    NfsReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync

error:

    return ntStatus;
}

static
NTSTATUS
NfsBuildSetSecurityInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNFS_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PNFS_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    pSetInfoState = (PNFS_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pSetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_SEC_INFO_CLASS_BASIC:

            ntStatus = NfsBuildSetInfoCommonResponse_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
VOID
NfsPrepareSetInfoStateAsync_SMB_V2(
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState,
    PNFS_EXEC_CONTEXT          pExecContext
    )
{
    pSetInfoState->acb.Callback        = &NfsExecuteSetInfoAsyncCB_SMB_V2;

    pSetInfoState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pSetInfoState->acb.AsyncCancelContext = NULL;

    pSetInfoState->pAcb = &pSetInfoState->acb;
}

static
VOID
NfsExecuteSetInfoAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PNFS_EXEC_CONTEXT          pExecContext     = (PNFS_EXEC_CONTEXT)pContext;
    PNFS_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState    = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pSetInfoState =
        (PNFS_SET_INFO_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pSetInfoState->mutex);

    if (pSetInfoState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pSetInfoState->pAcb->AsyncCancelContext);
    }

    pSetInfoState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pSetInfoState->mutex);

    ntStatus = NfsProdConsEnqueue(gProtocolGlobals_SMB_V2.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                       ntStatus);

        NfsReleaseExecContext(pExecContext);
    }
}

static
VOID
NfsReleaseSetInfoStateAsync_SMB_V2(
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState
    )
{
    if (pSetInfoState->pAcb)
    {
        pSetInfoState->acb.Callback = NULL;

        if (pSetInfoState->pAcb->CallbackContext)
        {
            PNFS_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PNFS_EXEC_CONTEXT)pSetInfoState->pAcb->CallbackContext;

            NfsReleaseExecContext(pExecContext);

            pSetInfoState->pAcb->CallbackContext = NULL;
        }

        if (pSetInfoState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pSetInfoState->pAcb->AsyncCancelContext);
        }

        pSetInfoState->pAcb = NULL;
    }
}

static
VOID
NfsReleaseSetInfoStateHandle_SMB_V2(
    HANDLE hSetInfoState
    )
{
    return NfsReleaseSetInfoState_SMB_V2(
                    (PNFS_SET_INFO_STATE_SMB_V2)hSetInfoState);
}

static
VOID
NfsReleaseSetInfoState_SMB_V2(
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState
    )
{
    if (InterlockedDecrement(&pSetInfoState->refCount) == 0)
    {
        NfsFreeSetInfoState_SMB_V2(pSetInfoState);
    }
}

static
VOID
NfsFreeSetInfoState_SMB_V2(
    PNFS_SET_INFO_STATE_SMB_V2 pSetInfoState
    )
{
    if (pSetInfoState->pAcb && pSetInfoState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pSetInfoState->pAcb->AsyncCancelContext);
    }

    if (pSetInfoState->pEcpList)
    {
        IoRtlEcpListFree(&pSetInfoState->pEcpList);
    }

    if (pSetInfoState->pFile)
    {
        NfsFile2Release(pSetInfoState->pFile);
    }

    if (pSetInfoState->pRootDir)
    {
        NfsFile2Release(pSetInfoState->pRootDir);
    }

    if (pSetInfoState->hDir)
    {
        IoCloseFile(pSetInfoState->hDir);
    }

    if (pSetInfoState->dirPath.FileName)
    {
        NfsFreeMemory(pSetInfoState->dirPath.FileName);
    }

    if (pSetInfoState->pData2)
    {
        NfsFreeMemory(pSetInfoState->pData2);
    }

    if (pSetInfoState->pMutex)
    {
        pthread_mutex_destroy(&pSetInfoState->mutex);
    }

    NfsFreeMemory(pSetInfoState);
}
