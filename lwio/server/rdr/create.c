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
 *        SMB Client Redirector
 *
 *        Create Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kaya Bekiroglu (kaya@likewisesoftware.com)
 *          Brian Koropoff (bkoropoff@likewise.com)
 */

#include "rdr.h"

static
BOOLEAN
RdrCreateQueryInfoPathComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
BOOLEAN
RdrFinishCreate(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrTransceiveCreate(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions
    );

static
void
RdrCancelCreate(
    PIRP pIrp,
    PVOID _pContext
    )
{
    return;
}

static
BOOLEAN
RdrCreateIsDeferred(
    ACCESS_MASK DesiredAccess,
    FILE_CREATE_OPTIONS CreateOptions
    )
{
    return (
        DesiredAccess == DELETE ||
        (DesiredAccess == FILE_LIST_DIRECTORY && (CreateOptions & FILE_DIRECTORY_FILE))
    );
}

BOOLEAN
RdrCreateTreeConnectComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_TREE pTree = NULL;
    PIRP pIrp = pContext->pIrp;
    ACCESS_MASK DesiredAccess = pIrp->Args.Create.DesiredAccess;
    LONG64 AllocationSize = pIrp->Args.Create.AllocationSize;
    FILE_SHARE_FLAGS ShareAccess = pIrp->Args.Create.ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition = pIrp->Args.Create.CreateDisposition;
    FILE_CREATE_OPTIONS CreateOptions = pIrp->Args.Create.CreateOptions;
    FILE_ATTRIBUTES FileAttributes =  pIrp->Args.Create.FileAttributes;
    PIO_CREDS pCreds = IoSecurityGetCredentials(pIrp->Args.Create.SecurityContext);
    PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcessInfo =
        IoSecurityGetProcessInfo(pIrp->Args.Create.SecurityContext);

    PRDR_CCB pFile = NULL;

    if (pParam)
    {
        switch (RDR_OBJECT_PROTOCOL(pParam))
        {
        case SMB_PROTOCOL_VERSION_1:
            pTree = (PRDR_TREE) pParam;
            break;
        case SMB_PROTOCOL_VERSION_2:
            /* We ended up with an SMB2 tree, short circuit to create2.c */
            return RdrCreateTreeConnect2Complete(pContext, status, pParam);
        default:
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(status);
        }
    }

    BAIL_ON_NT_STATUS(status);

    status = LwIoAllocateMemory(
        sizeof(RDR_CCB),
        (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(status);
    
    status = LwErrnoToNtStatus(pthread_mutex_init(&pFile->mutex, NULL));
    BAIL_ON_NT_STATUS(status);
    
    pFile->bMutexInitialized = TRUE;
    pFile->version = SMB_PROTOCOL_VERSION_1;
    pFile->pTree = pTree;
    pTree = NULL;
    pFile->Params.CreateOptions = CreateOptions;

    pFile->pwszPath = pContext->State.Create.pwszFilename;
    pContext->State.Create.pwszFilename = NULL;

    pFile->pwszCanonicalPath = pContext->State.Create.pwszCanonicalPath;
    pContext->State.Create.pwszCanonicalPath = NULL;

    pContext->State.Create.pFile = pFile;

    if (RdrCreateIsDeferred(DesiredAccess, CreateOptions))
    {
        /*
         * The operation we are requesting access for is path-based, so there
         * is no point in creating a fid.
         */
        if (RDR_CCB_IS_DFS(pFile))
        {
            /* However, in the DFS case, we can't afford to get a STATUS_PATH_NOT_COVERED
             * after completing the create IRP because we're stuck with out current file handle.
             * We can't change the file handle after the fact because:
             *
             * - It would violate the invariant that pFile->pTree is constant for the lifetime
             *   of the handle
             * - We might end up with a different type of connection (SMB1/2), which means
             *   we don't even have the correct file handle structure
             *
             * Query FileBasicInformation now so we can detect this case
             */
            pContext->Continue = RdrCreateQueryInfoPathComplete;

            status = RdrTransceiveQueryInfoPath(
                pContext,
                pFile->pTree,
                RDR_CCB_PATH(pFile),
                SMB_QUERY_FILE_BASIC_INFO,
                sizeof(FILE_BASIC_INFORMATION));
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            status = IoFileSetContext(pContext->pIrp->FileHandle, pFile);
            BAIL_ON_NT_STATUS(status);
        }
    }
    else
    {
        pContext->Continue = RdrFinishCreate;

        status = RdrTransceiveCreate(
            pContext,
            pFile,
            DesiredAccess,
            AllocationSize,
            FileAttributes,
            ShareAccess,
            CreateDisposition,
            CreateOptions);
        switch (status)
        {
        case STATUS_SUCCESS:
        case STATUS_PENDING:
            break;
        default:
            pContext->Continue = RdrCreateTreeConnectComplete;
            pContext->State.Create.pFile = NULL;

            status = RdrDfsConnect(
                pFile->pTree->pSession->pSocket,
                &pIrp->Args.Create.FileName.Name,
                pCreds,
                pProcessInfo->Uid,
                status,
                &pContext->usTry,
                &pContext->State.Create.pwszFilename,
                &pContext->State.Create.pwszCanonicalPath,
                pContext);

            RdrReleaseFile(pFile);
            pFile = NULL;
        }
        BAIL_ON_NT_STATUS(status);
    }
    
cleanup:

    if (status != STATUS_PENDING)
    {
        RTL_FREE(&pContext->State.Create.pwszFilename);
        RTL_FREE(&pContext->State.Create.pwszCanonicalPath);
        RdrFreeContext(pContext);
        pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pIrp);
    }

    return FALSE;

error:

    if (status != STATUS_PENDING && pFile)
    {
        RdrReleaseFile(pFile);
    }

    if (status != STATUS_PENDING && pTree)
    {
        RdrTreeRelease(pTree);
    }


    goto cleanup;
}

static
BOOLEAN
RdrCreateQueryInfoPathComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_CCB pFile = pContext->State.Create.pFile;
    PSMB_PACKET pPacket = pParam;
    PIRP pIrp = pContext->pIrp;
    PIO_CREDS pCreds = IoSecurityGetCredentials(pIrp->Args.Create.SecurityContext);
    PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcessInfo =
          IoSecurityGetProcessInfo(pIrp->Args.Create.SecurityContext);

    if (status == STATUS_SUCCESS)
    {
        status = pPacket->pSMBHeader->error;
    }

    switch (status)
    {
    case STATUS_SUCCESS:
        status = IoFileSetContext(pContext->pIrp->FileHandle, pFile);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        /* Go back into DFS connect */
        pContext->Continue = RdrCreateTreeConnectComplete;
        pContext->State.Create.pFile = NULL;

        status = RdrDfsConnect(
            pFile->pTree->pSession->pSocket,
            &pIrp->Args.Create.FileName.Name,
            pCreds,
            pProcessInfo->Uid,
            status,
            &pContext->usTry,
            &pContext->State.Create.pwszFilename,
            &pContext->State.Create.pwszCanonicalPath,
            pContext);

        RdrReleaseFile(pFile);
        pFile = NULL;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    RdrFreePacket(pPacket);

    if (status != STATUS_PENDING)
    {
        pContext->pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pContext->pIrp);
        RTL_FREE(&pContext->State.Create.pwszFilename);
        RTL_FREE(&pContext->State.Create.pwszCanonicalPath);
        RdrFreeContext(pContext);
    }

    return FALSE;

error:

    if (pFile)
    {
        RdrReleaseFile(pFile);
    }

    goto cleanup;
}

static
BOOLEAN
RdrFinishCreate(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_CCB pFile = pContext->State.Create.pFile;
    PSMB_PACKET pPacket = pParam;
    PCREATE_RESPONSE_HEADER pResponseHeader = NULL;
    PIRP pIrp = pContext->pIrp;
    PIO_CREDS pCreds = IoSecurityGetCredentials(pIrp->Args.Create.SecurityContext);
    PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcessInfo =
        IoSecurityGetProcessInfo(pIrp->Args.Create.SecurityContext);

    if (status == STATUS_SUCCESS)
    {
        status = pPacket->pSMBHeader->error;
    }

    switch (status)
    {
    case STATUS_SUCCESS:
        break;
    default:
        pContext->Continue = RdrCreateTreeConnectComplete;
        pContext->State.Create.pFile = NULL;

        status = RdrDfsConnect(
            pFile->pTree->pSession->pSocket,
            &pIrp->Args.Create.FileName.Name,
            pCreds,
            pProcessInfo->Uid,
            status,
            &pContext->usTry,
            &pContext->State.Create.pwszFilename,
            &pContext->State.Create.pwszCanonicalPath,
            pContext);

        RdrReleaseFile(pFile);
        pFile = NULL;
    }
    BAIL_ON_NT_STATUS(status);

    status = WireUnmarshallSMBResponseCreate(
                pPacket->pParams,
                pPacket->bufferLen - pPacket->bufferUsed,
                &pResponseHeader);
    BAIL_ON_NT_STATUS(status);

    pFile->fid = pResponseHeader->fid;
    pFile->usFileType = pResponseHeader->fileType;

    status = IoFileSetContext(pContext->pIrp->FileHandle, pFile);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RdrFreePacket(pPacket);

    if (status != STATUS_PENDING)
    {
        pContext->pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pContext->pIrp);
        RTL_FREE(&pContext->State.Create.pwszFilename);
        RTL_FREE(&pContext->State.Create.pwszCanonicalPath);
        RdrFreeContext(pContext);
    }

    return FALSE;

error:

    if (pFile)
    {
        RdrReleaseFile(pFile);
    }

    goto cleanup;
}

NTSTATUS
RdrCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;
    PIO_CREDS pCreds = IoSecurityGetCredentials(pIrp->Args.Create.SecurityContext);
    PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcessInfo =
        IoSecurityGetProcessInfo(pIrp->Args.Create.SecurityContext);

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);
    
    IoIrpMarkPending(pIrp, RdrCancelCreate, pContext);

    if (!pCreds)
    {
        status = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(status);
    }

    pContext->Continue = RdrCreateTreeConnectComplete;
    status = RdrDfsConnect(
        NULL,
        &pIrp->Args.Create.FileName.Name,
        pCreds,
        pProcessInfo->Uid,
        status,
        &pContext->usTry,
        &pContext->State.Create.pwszFilename,
        &pContext->State.Create.pwszCanonicalPath,
        pContext);

cleanup:

    if (status != STATUS_PENDING && pContext)
    {
        pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pIrp);
        RdrFreeContext(pContext);
        status = STATUS_PENDING;
    }

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveCreate(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32_t packetByteCount = 0;
    CREATE_REQUEST_HEADER *pHeader = NULL;
    PWSTR pwszPath = RDR_CCB_PATH(pFile);

    status = RdrAllocateContextPacket(
        pContext,
        1024*64);
    BAIL_ON_NT_STATUS(status);

    status = SMBPacketMarshallHeader(
                pContext->Packet.pRawBuffer,
                pContext->Packet.bufferLen,
                COM_NT_CREATE_ANDX,
                0,
                0,
                pFile->pTree->tid,
                gRdrRuntime.SysPid,
                pFile->pTree->pSession->uid,
                0,
                TRUE,
                &pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    if (RDR_CCB_IS_DFS(pFile))
    {
        pContext->Packet.pSMBHeader->flags2 |= FLAG2_DFS;
    }

    pContext->Packet.pData = pContext->Packet.pParams + sizeof(CREATE_REQUEST_HEADER);

    pContext->Packet.bufferUsed += sizeof(CREATE_REQUEST_HEADER);

    pContext->Packet.pSMBHeader->wordCount = 24;

    pHeader = (CREATE_REQUEST_HEADER *) pContext->Packet.pParams;

    pHeader->reserved = 0;
    pHeader->nameLength = (wc16slen(pwszPath) + 1) * sizeof(wchar16_t);
    pHeader->flags = 0;
    pHeader->rootDirectoryFid = 0;
    pHeader->desiredAccess = desiredAccess;
    pHeader->allocationSize = llAllocationSize;
    pHeader->extFileAttributes = fileAttributes;
    pHeader->shareAccess = shareAccess;
    pHeader->createDisposition = createDisposition;
    pHeader->createOptions = createOptions;
    pHeader->impersonationLevel = 0x2; /* FIXME: magic constant */

    status = WireMarshallCreateRequestData(
                pContext->Packet.pData,
                pContext->Packet.bufferLen - pContext->Packet.bufferUsed,
                (pContext->Packet.pData - (uint8_t *) pContext->Packet.pSMBHeader) % 2,
                &packetByteCount,
                pwszPath);
    BAIL_ON_NT_STATUS(status);

    assert(packetByteCount <= UINT16_MAX);
    pHeader->byteCount = (uint16_t) packetByteCount;
    pContext->Packet.bufferUsed += packetByteCount;

    // byte order conversions
    SMB_HTOL8_INPLACE(pHeader->reserved);
    SMB_HTOL16_INPLACE(pHeader->nameLength);
    SMB_HTOL32_INPLACE(pHeader->flags);
    SMB_HTOL32_INPLACE(pHeader->rootDirectoryFid);
    SMB_HTOL32_INPLACE(pHeader->desiredAccess);
    SMB_HTOL64_INPLACE(pHeader->allocationSize);
    SMB_HTOL32_INPLACE(pHeader->extFileAttributes);
    SMB_HTOL32_INPLACE(pHeader->shareAccess);
    SMB_HTOL32_INPLACE(pHeader->createDisposition);
    SMB_HTOL32_INPLACE(pHeader->createOptions);
    SMB_HTOL32_INPLACE(pHeader->impersonationLevel);
    SMB_HTOL8_INPLACE(pHeader->securityFlags);
    SMB_HTOL16_INPLACE(pHeader->byteCount);

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pFile->pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}
