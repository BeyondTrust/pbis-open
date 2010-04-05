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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __PROTOTYPES_H__
#define __PROTOTYPES_H__

// cancel.c

NTSTATUS
NfsProcessCancel_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

// close.c

NTSTATUS
NfsProcessClose_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

// connection.c

NTSTATUS
NfsConnection2FindSession_SMB_V2(
    PNFS_EXEC_CONTEXT_SMB_V2 pSmb2Context,
    PLWIO_NFS_CONNECTION     pConnection,
    ULONG64                  ullUid,
    PLWIO_NFS_SESSION_2*     ppSession
    );

// config.c

ULONG
NfsConfigGetOplockTimeout_SMB_V2(
    VOID
    );

// create.c

NTSTATUS
NfsProcessCreate_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

NTSTATUS
NfsCancelCreate_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

// echo.c

NTSTATUS
NfsProcessEcho_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

// find.c

NTSTATUS
NfsProcessFind_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

// flush.c

NTSTATUS
NfsProcessFlush_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

// getfileinfo.c

NTSTATUS
NfsGetFileInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

NTSTATUS
NfsBuildFileInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

// getfsinfo.c

NTSTATUS
NfsGetFileSystemInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

NTSTATUS
NfsBuildFileSystemInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

// getinfo.c

NTSTATUS
NfsProcessGetInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

VOID
NfsPrepareGetInfoStateAsync_SMB_V2(
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState,
    PNFS_EXEC_CONTEXT          pExecContext
    );

VOID
NfsReleaseGetInfoStateAsync_SMB_V2(
    PNFS_GET_INFO_STATE_SMB_V2 pGetInfoState
    );

// getsecinfo.c

NTSTATUS
NfsGetSecurityInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

NTSTATUS
NfsBuildSecurityInfoResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

// ioctl.c

NTSTATUS
NfsProcessIOCTL_SMB_V2(
    PNFS_EXEC_CONTEXT pContext
    );

// libmain.c

NTSTATUS
NfsBuildInterimResponse_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext,
    ULONG64           ullAsyncId
    );

// lock.c

NTSTATUS
NfsProcessLock_SMB_V2(
    PNFS_EXEC_CONTEXT pContext
    );

VOID
NfsCancelAsyncLockState_SMB_V2(
    HANDLE hLockState
    );

NTSTATUS
NfsDetermineLocks_SMB_V2(
    PSMB2_LOCK_REQUEST_HEADER pLockRequestHeader,
    PSMB2_LOCK**              pppLockArray,
    PULONG                    pulNumLocks
    );

NTSTATUS
NfsDetermineUnlocks_SMB_V2(
    PSMB2_LOCK_REQUEST_HEADER pLockRequestHeader,
    PSMB2_LOCK**              pppUnlockArray,
    PULONG                    pulNumUnlocks
    );

// lockasync.c

NTSTATUS
NfsBuildAsyncLockState_SMB_V2(
    ULONG64                               ullAsyncId,
    PNFS_EXEC_CONTEXT                     pExecContext,
    PNFS_LOCK_REQUEST_STATE_SMB_V2        pLockRequestState,
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2* ppAsyncLockState
    );

NTSTATUS
NfsBuildExecContextAsyncLock_SMB_V2(
    PNFS_EXEC_CONTEXT              pExecContext,
    PNFS_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState,
    ULONG64                        ullAsyncId,
    PNFS_EXEC_CONTEXT*             ppExecContextAsync
    );

PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2
NfsAcquireAsyncLockState_SMB_V2(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    );

NTSTATUS
NfsCancelLock_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

NTSTATUS
NfsProcessAsyncLockRequest_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

VOID
NfsReleaseAsyncLockStateHandle_SMB_V2(
    HANDLE hAsyncLockState
    );

VOID
NfsReleaseAsyncLockState_SMB_V2(
    PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    );

// logoff.c

NTSTATUS
NfsProcessLogoff_SMB_V2(
    PNFS_EXEC_CONTEXT pContext
    );

// negotiate.c

NTSTATUS
NfsProcessNegotiate_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

// notify_request.c

NTSTATUS
NfsProcessNotify_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

NTSTATUS
NfsProcessNotifyCompletion_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

NTSTATUS
NfsCancelChangeNotify_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

VOID
NfsCancelNotifyState_SMB_V2(
    HANDLE hNotifyState
    );

// notify_state.c

NTSTATUS
NfsNotifyCreateState_SMB_V2(
    ULONG64                   ullAsyncId,
    PLWIO_NFS_CONNECTION      pConnection,
    PLWIO_NFS_SESSION_2       pSession,
    PLWIO_NFS_TREE_2          pTree,
    PLWIO_NFS_FILE_2          pFile,
    USHORT                    usEpoch,
    ULONG64                   ullCommandSequence,
    ULONG                     ulPid,
    ULONG                     ulCompletionFilter,
    BOOLEAN                   bWatchTree,
    ULONG                     ulMaxBufferSize,
    PNFS_NOTIFY_STATE_SMB_V2* ppNotifyState
    );

VOID
NfsPrepareNotifyStateAsync_SMB_V2(
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState
    );

VOID
NfsReleaseNotifyStateAsync_SMB_V2(
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState
    );

PNFS_NOTIFY_STATE_SMB_V2
NfsNotifyStateAcquire_SMB_V2(
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState
    );

VOID
NfsNotifyStateReleaseHandle_SMB_V2(
    HANDLE hNotifyState
    );

VOID
NfsNotifyStateRelease_SMB_V2(
    PNFS_NOTIFY_STATE_SMB_V2 pNotifyState
    );

// oplock.c

NTSTATUS
NfsBuildOplockState_SMB_V2(
    PLWIO_NFS_CONNECTION      pConnection,
    PLWIO_NFS_SESSION_2       pSession,
    PLWIO_NFS_TREE_2          pTree,
    PLWIO_NFS_FILE_2          pFile,
    PNFS_OPLOCK_STATE_SMB_V2* ppOplockState
    );

NTSTATUS
NfsProcessOplock_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

NTSTATUS
NfsProcessOplockBreak_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

NTSTATUS
NfsAcknowledgeOplockBreak_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState,
    PUCHAR  pucNewOplockLevel,
    BOOLEAN bFileIsClosed
    );

VOID
NfsReleaseOplockStateHandle_SMB_V2(
    HANDLE hOplockState
    );

VOID
NfsReleaseOplockState_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState
    );

VOID
NfsPrepareOplockStateAsync_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState
    );

VOID
NfsReleaseOplockStateAsync_SMB_V2(
    PNFS_OPLOCK_STATE_SMB_V2 pOplockState
    );

// session.c

NTSTATUS
NfsSession2FindTree_SMB_V2(
    PNFS_EXEC_CONTEXT_SMB_V2 pSmb2Context,
    PLWIO_NFS_SESSION_2      pSession,
    ULONG                    ulTid,
    PLWIO_NFS_TREE_2*        ppTree
    );

// session_setup.c

NTSTATUS
NfsProcessSessionSetup_SMB_V2(
    PNFS_EXEC_CONTEXT pContext
    );

// setinfo.c

NTSTATUS
NfsProcessSetInfo_SMB_V2(
    PNFS_EXEC_CONTEXT pContext
    );

// error.c

NTSTATUS
NfsSetErrorMessage_SMB_V2(
    PNFS_EXEC_CONTEXT_SMB_V2 pSmb2Context,
    PBYTE                    pErrorMessage,
    ULONG                    ulErrorMessageLength
    );

VOID
NfsFreeErrorMessage_SMB_V2(
    PNFS_EXEC_CONTEXT_SMB_V2 pSmb2Context
    );

NTSTATUS
NfsBuildErrorResponse_SMB_V2(
    PNFS_EXEC_CONTEXT    pExecContext,
    ULONG64              ullAsyncId,
    NTSTATUS             errorStatus
    );

// read.c

NTSTATUS
NfsProcessRead_SMB_V2(
    PNFS_EXEC_CONTEXT pContext
    );

// tree.c

NTSTATUS
NfsTree2FindFile_SMB_V2(
    PNFS_EXEC_CONTEXT_SMB_V2 pSmb2Context,
    PLWIO_NFS_TREE_2         pTree,
    PSMB2_FID                pFid,
    BOOLEAN                  bRelated,
    PLWIO_NFS_FILE_2*        ppFile
    );

// tree_connect.c

NTSTATUS
NfsProcessTreeConnect_SMB_V2(
    PNFS_EXEC_CONTEXT pContext
    );

// tree_disconnect.c

NTSTATUS
NfsProcessTreeDisconnect_SMB_V2(
    PNFS_EXEC_CONTEXT pExecContext
    );

// wire.c

NTSTATUS
SMB2InitPacket(
    PSMB_PACKET pSmbPacket,
    BOOLEAN     bAllowSignature
    );

NTSTATUS
NfsUnmarshalHeader_SMB_V2(
    PBYTE         pBuffer,
    ULONG         ulOffset,
    ULONG         ulBytesAvailable,
    PSMB2_HEADER* ppHeader,
    PULONG        pulBytesUsed
    );

NTSTATUS
SMB2MarshalHeader(
    PBYTE         pBuffer,
    ULONG         ulOffset,
    ULONG         ulBytesAvailable,
    USHORT        usCommand,
    USHORT        usEpoch,
    USHORT        usCredits,
    ULONG         ulPid,
    ULONG64       ullMid,
    ULONG         ulTid,
    ULONG64       ullSessionId,
    ULONG64       ullAsyncId,
    NTSTATUS      status,
    BOOLEAN       bIsResponse,
    BOOLEAN       bIsPartOfCompoundMessage,
    PSMB2_HEADER* ppSMB2Header,
    PULONG        pulBytesUsed
    );

NTSTATUS
SMB2GetAsyncId(
    PSMB2_HEADER pHeader,
    PULONG64     pUllAsyncId
    );

NTSTATUS
SMB2UnmarshalNegotiateRequest(
    PNFS_MESSAGE_SMB_V2             pRequest,
    PSMB2_NEGOTIATE_REQUEST_HEADER* ppHeader,
    PUSHORT*                        ppusDialects
    );

NTSTATUS
SMB2UnmarshallSessionSetup(
    PNFS_MESSAGE_SMB_V2                 pRequest,
    PSMB2_SESSION_SETUP_REQUEST_HEADER* ppHeader,
    PBYTE*                              ppSecurityBlob,
    PULONG                              pulSecurityBlobLen
    );

NTSTATUS
SMB2MarshalSessionSetup(
    PBYTE              pBuffer,
    ULONG              ulOffset,
    ULONG              ulBytesAvailable,
    SMB2_SESSION_FLAGS usFlags,
    PBYTE              pSecurityBlob,
    ULONG              ulSecurityBlobLen,
    PULONG             pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalLogoffRequest(
    PNFS_MESSAGE_SMB_V2          pRequest,
    PSMB2_LOGOFF_REQUEST_HEADER* ppHeader
    );

NTSTATUS
SMB2MarshalLogoffResponse(
    PBYTE  pBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PULONG pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalTreeConnect(
    PNFS_MESSAGE_SMB_V2                pRequest,
    PSMB2_TREE_CONNECT_REQUEST_HEADER* ppHeader,
    PUNICODE_STRING                    pwszPath
    );

NTSTATUS
SMB2MarshalTreeConnectResponse(
    PBYTE                               pBuffer,
    ULONG                               ulOffset,
    ULONG                               ulBytesAvailable,
    PLWIO_NFS_CONNECTION                pConnection,
    PLWIO_NFS_TREE_2                    pTree,
    PSMB2_TREE_CONNECT_RESPONSE_HEADER* ppResponseHeader,
    PULONG                              pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalTreeDisconnectRequest(
    PNFS_MESSAGE_SMB_V2                   pSmbRequest,
    PSMB2_TREE_DISCONNECT_REQUEST_HEADER* ppTreeDisconnectHeader
    );

NTSTATUS
SMB2MarshalTreeDisconnectResponse(
    PBYTE                pBuffer,
    ULONG                ulOffset,
    ULONG                ulBytesAvailable,
    PULONG               pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalCreateRequest(
    PNFS_MESSAGE_SMB_V2          pSmbRequest,
    PSMB2_CREATE_REQUEST_HEADER* ppCreateRequestHeader,
    PUNICODE_STRING              pwszFileName,
    PNFS_CREATE_CONTEXT*         ppCreateContexts,
    PULONG                       pulNumContexts
    );

NTSTATUS
SMB2MarshalCreateContext(
    IN OUT PBYTE                 pBuffer,
    IN     ULONG                 ulOffset,
    IN     PBYTE                 pName,
    IN     USHORT                usNameSize,
    IN     PBYTE                 pData,
    IN     ULONG                 ulDataSize,
    IN     ULONG                 ulBytesAvailable,
    IN OUT PULONG                pulBytesUsed,
    IN OUT PSMB2_CREATE_CONTEXT* ppCreateContext
    );

NTSTATUS
SMB2UnmarshalCloseRequest(
   IN     PNFS_MESSAGE_SMB_V2         pSmbRequest,
   IN OUT PSMB2_CLOSE_REQUEST_HEADER* ppHeader
   );

NTSTATUS
SMB2UnmarshalFlushRequest(
    PNFS_MESSAGE_SMB_V2 pSmbRequest,
    PSMB2_FID*          ppFid
    );

NTSTATUS
SMB2MarshalFlushResponse(
    PBYTE                pBuffer,
    ULONG                ulOffset,
    ULONG                ulBytesAvailable,
    PULONG               pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalEchoRequest(
   PNFS_MESSAGE_SMB_V2         pSmbRequest,
   PSMB2_ECHO_REQUEST_HEADER*  ppHeader
   );

NTSTATUS
SMB2MarshalEchoResponse(
    PBYTE  pBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PULONG pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalGetInfoRequest(
    PNFS_MESSAGE_SMB_V2            pSmbRequest,
    PSMB2_GET_INFO_REQUEST_HEADER* ppHeader
    );

NTSTATUS
SMB2UnmarshalSetInfoRequest(
    PNFS_MESSAGE_SMB_V2            pSmbRequest,
    PSMB2_SET_INFO_REQUEST_HEADER* ppHeader,
    PBYTE*                         ppData
    );

NTSTATUS
SMB2UnmarshalReadRequest(
    PNFS_MESSAGE_SMB_V2        pSmbRequest,
    PSMB2_READ_REQUEST_HEADER* ppRequestHeader
    );

NTSTATUS
SMB2MarshalReadResponse(
    PBYTE  pBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PBYTE  pData,
    ULONG  ulBytesRead,
    ULONG  ulBytesRemaining,
    PULONG pulDataOffset,
    PULONG pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalWriteRequest(
    PNFS_MESSAGE_SMB_V2         pSmbRequest,
    PSMB2_WRITE_REQUEST_HEADER* ppRequestHeader,
    PBYTE*                      ppData
    );

NTSTATUS
SMB2MarshalWriteResponse(
    PBYTE  pBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    ULONG  ulBytesWritten,
    ULONG  ulBytesRemaining,
    PULONG pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalLockRequest(
    PNFS_MESSAGE_SMB_V2        pSmbRequest,
    PSMB2_LOCK_REQUEST_HEADER* ppRequestHeader
    );

NTSTATUS
SMB2MarshalLockResponse(
    PBYTE                  pBuffer,
    ULONG                  ulOffset,
    ULONG                  ulBytesAvailable,
    PULONG                 pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalIOCTLRequest(
    PNFS_MESSAGE_SMB_V2         pSmbRequest,
    PSMB2_IOCTL_REQUEST_HEADER* ppRequestHeader,
    PBYTE*                      ppData
    );

NTSTATUS
SMB2MarshalIOCTLResponse(
    PBYTE                      pBuffer,
    ULONG                      ulOffset,
    ULONG                      ulBytesAvailable,
    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader,
    PBYTE                      pOutBuffer,
    ULONG                      ulOutLength,
    PULONG                     pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalFindRequest(
    PNFS_MESSAGE_SMB_V2        pSmbRequest,
    PSMB2_FIND_REQUEST_HEADER* ppRequestHeader,
    PUNICODE_STRING            pwszFilename
    );

NTSTATUS
SMB2UnmarshalOplockBreakRequest(
    IN     PNFS_MESSAGE_SMB_V2        pSmbRequest,
    IN OUT PSMB2_OPLOCK_BREAK_HEADER* ppRequestHeader
    );

NTSTATUS
SMB2MarshalFindResponse(
    PBYTE                       pBuffer,
    ULONG                       ulOffset,
    ULONG                       ulBytesAvailable,
    PBYTE                       pData,
    ULONG                       ulDataLength,
    PULONG                      pulDataOffset,
    PSMB2_FIND_RESPONSE_HEADER* ppHeader,
    PULONG                      pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalNotifyRequest(
    IN     PNFS_MESSAGE_SMB_V2         pSmbRequest,
    IN OUT PSMB2_NOTIFY_CHANGE_HEADER* ppNotifyRequestHeader
    );

NTSTATUS
SMB2MarshalNotifyResponse(
    IN OUT PBYTE                         pBuffer,
    IN     ULONG                         ulOffset,
    IN     ULONG                         ulBytesAvailable,
    IN OUT PBYTE                         pData,
    IN     ULONG                         ulDataLength,
    IN OUT PULONG                        pulDataOffset,
    IN OUT PSMB2_NOTIFY_RESPONSE_HEADER* ppHeader,
    IN OUT PULONG                        pulBytesUsed
    );

NTSTATUS
SMB2MarshalError(
    PBYTE    pBuffer,
    ULONG    ulOffset,
    ULONG    ulBytesAvailable,
    PBYTE    pMessage,
    ULONG    ulMessageLength,
    PULONG   pulBytesUsed
    );

NTSTATUS
SMB2MarshalFooter(
    PSMB_PACKET pPacket
    );

// write.c

NTSTATUS
NfsProcessWrite_SMB_V2(
    PNFS_EXEC_CONTEXT pContext
    );

#endif /* __PROTOTYPES_H__ */

