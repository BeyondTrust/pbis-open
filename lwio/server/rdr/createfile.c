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
 *        createnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CreateNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "rdr.h"

static
NTSTATUS
ParseSharePath(
    PCWSTR pwszPath,
    PWSTR*  ppwszServer,
    PSTR*   ppszShare,
    PSTR*   ppszFilename
    );

static
NTSTATUS
RdrTransactCreateFile(
    SMB_TREE *pTree,
    USHORT usRootFid,
    PCWSTR pwszPath,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions,
    PUSHORT pusFid,
    PUSHORT pusFileType
    );

NTSTATUS
RdrCreateFileEx(
    PIO_CREDS pCreds,
    PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcessInfo,
    PIO_FILE_NAME pFilename,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions,
    PHANDLE phFile
    )
{
    NTSTATUS ntStatus = 0;
    PWSTR  pwszServer = NULL;
    PSTR   pszShare = NULL;
    PSTR   pszFilename = NULL;
    PWSTR  pwszFilename = NULL;
    PSMB_CLIENT_FILE_HANDLE pFile = NULL;
    PSTR   pszCachePath = NULL;
    PWSTR  pwszUsername = NULL;
    PWSTR  pwszDomain = NULL;
    PWSTR  pwszPassword = NULL;
    USHORT usRootFid    = 0;

    if (!pCreds)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    switch (pCreds->type)
    {
    case IO_CREDS_TYPE_KRB5_TGT:
        pwszUsername = pCreds->payload.krb5Tgt.pwszClientPrincipal;

        ntStatus = SMBCredTokenToKrb5CredCache(pCreds, &pszCachePath);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBKrb5SetDefaultCachePath(pszCachePath, NULL);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    case IO_CREDS_TYPE_PLAIN:
        pwszUsername = pCreds->payload.plain.pwszUsername;
        pwszDomain = pCreds->payload.plain.pwszDomain;
        pwszPassword = pCreds->payload.plain.pwszPassword;
        break;
    default:
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    
    ntStatus = SMBAllocateMemory(
        sizeof(SMB_CLIENT_FILE_HANDLE),
        (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(ntStatus);
    
    ntStatus = pthread_mutex_init(&pFile->mutex, NULL);
    BAIL_ON_NT_STATUS(ntStatus);
    
    pFile->pMutex = &pFile->mutex;
    
    if (pFilename->RootFileHandle == NULL)
    {
        ntStatus = ParseSharePath(
                        pFilename->FileName,
                        &pwszServer,
                        &pszShare,
                        &pszFilename);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBSrvClientTreeOpen(
                        pwszServer,
                        pszShare,
                        pCreds,
                        pProcessInfo->Uid,
                        &pFile->pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBMbsToWc16s(
                        pszFilename,
                        &pwszFilename);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        PSMB_CLIENT_FILE_HANDLE pRoot =
                IoFileGetContext(pFilename->RootFileHandle);

        pFile->pTree = pRoot->pTree;

        SMBTreeAddReference(pRoot->pTree);

        usRootFid = pRoot->fid;

        ntStatus = LwRtlWC16StringAllocatePrintfW(
                        &pwszFilename,
                        L"%ws\\%ws",
                        pRoot->pwszPath,
                        pFilename->FileName);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = RdrTransactCreateFile(
                    pFile->pTree,
                    usRootFid,
                    usRootFid ? pFilename->FileName : pwszFilename,
                    desiredAccess,
                    llAllocationSize,
                    fileAttributes,
                    shareAccess,
                    createDisposition,
                    createOptions,
                    &pFile->fid,
                    &pFile->usFileType);
    BAIL_ON_NT_STATUS(ntStatus);

    pFile->pwszPath = pwszFilename;
    pwszFilename = NULL;

    *phFile = (HANDLE)pFile;

cleanup:

    LWIO_SAFE_FREE_MEMORY(pwszServer);
    LWIO_SAFE_FREE_STRING(pszShare);
    LWIO_SAFE_FREE_STRING(pszFilename);
    LWIO_SAFE_FREE_MEMORY(pwszFilename);

    if (pszCachePath)
    {
        SMBKrb5DestroyCache(pszCachePath);
        LWIO_SAFE_FREE_MEMORY(pszCachePath);
    }

    return ntStatus;

error:

    if (pFile)
    {
        RdrReleaseFile(pFile);
    }

    *phFile = NULL;

    goto cleanup;
}

static
NTSTATUS
ParseSharePath(
    PCWSTR pwszPath,
    PWSTR*  ppwszServer,
    PSTR*   ppszShare,
    PSTR*   ppszFilename
    )
{
    NTSTATUS ntStatus = 0;
    PSTR  pszPath = NULL;
    PSTR  pszIndex = NULL;
    PSTR  pszServer = NULL;
    PSTR  pszCanonical = NULL;
    PSTR  pszShare  = NULL;
    PSTR  pszFilename = NULL;
    PSTR  pszCursor = NULL;
    size_t sLen = 0;
    size_t i = 0;
    struct in_addr ipAddr;

    memset(&ipAddr, 0, sizeof(ipAddr));
    ntStatus = SMBWc16sToMbs(
                    pwszPath,
                    &pszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBStripWhitespace(pszPath, TRUE, TRUE);

    pszIndex = pszPath;

    // Skip optional initial decoration
    if (!strncmp(pszIndex, "/", sizeof("/") - 1) ||
        !strncmp(pszIndex, "\\", sizeof("\\") - 1))
    {
        pszIndex += 1;
    }

    if (IsNullOrEmptyString(pszIndex))
    {
        ntStatus = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Seek server name
    sLen = strcspn(pszIndex, "\\/");
    if (!sLen)
    {
        ntStatus = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBStrndup(
                    pszIndex,
                    sLen,
                    &pszServer);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBStrndup(
                    pszIndex,
                    sLen,
                    &pszCanonical);
    BAIL_ON_NT_STATUS(ntStatus);

    for (pszCursor = pszCanonical; *pszCursor; pszCursor++)
    {
        if (*pszCursor == '@')
        {
            *pszCursor = '\0';
            break;
        }
    }

    pszIndex += sLen;

    // Skip delimiter
    sLen = strspn(pszIndex, "\\/");
    if (!sLen)
    {
        ntStatus = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pszIndex += sLen;

    // Seek share name
    sLen = strcspn(pszIndex, "\\/");
    if (!sLen)
    {
        ntStatus = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBAllocateMemory(
        sizeof("\\\\") - 1 + strlen(pszCanonical) + sizeof("\\") - 1 + sLen + 1,
        (PVOID*)&pszShare);
    BAIL_ON_NT_STATUS(ntStatus);
    
    sprintf(pszShare, "\\\\%s\\", pszCanonical);
    strncat(pszShare, pszIndex, sLen);

    pszIndex += sLen;

    // Skip delimiter
    sLen = strspn(pszIndex, "\\/");
    if (!sLen)
    {
        SMBAllocateMemory(
            strlen("\\") + 1,
            (PVOID*)&pszFilename);
        BAIL_ON_NT_STATUS(ntStatus);

        pszFilename[0] = '\\';
        pszFilename[1] = '\0';
    }
    else
    {
        pszIndex += sLen;

        SMBAllocateMemory(
            strlen(pszIndex) + 2,
            (PVOID*)&pszFilename);
        BAIL_ON_NT_STATUS(ntStatus);

        pszFilename[0] = '\\';
    
        for (i = 0; pszIndex[i]; i++)
        {
            switch (pszIndex[i])
            {
            case '/':
                pszFilename[1 + i] = '\\';
                break;
            default:
                pszFilename[1 + i] = pszIndex[i];
            }
        }

        pszFilename[1 + i] = '\0';
    }

    ntStatus = LwRtlWC16StringAllocateFromCString(ppwszServer, pszServer);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppszShare  = pszShare;
    *ppszFilename = pszFilename;

cleanup:

    LWIO_SAFE_FREE_STRING(pszCanonical);
    LWIO_SAFE_FREE_STRING(pszServer);
    LWIO_SAFE_FREE_STRING(pszPath);

    return ntStatus;

error:

    LWIO_SAFE_FREE_STRING(pszServer);
    LWIO_SAFE_FREE_STRING(pszShare);
    LWIO_SAFE_FREE_STRING(pszFilename);

    *ppwszServer = NULL;
    *ppszShare = NULL;
    *ppszFilename = NULL;

    goto cleanup;
}

static
NTSTATUS
RdrTransactCreateFile(
    SMB_TREE *pTree,
    USHORT usRootFid,
    PCWSTR pwszPath,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions,
    PUSHORT pusFid,
    PUSHORT pusFileType
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    uint32_t packetByteCount = 0;
    CREATE_REQUEST_HEADER *pHeader = NULL;
    CREATE_RESPONSE_HEADER *pResponseHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;

    /* @todo: make initial length configurable */
    ntStatus = SMBPacketBufferAllocate(
                    pTree->pSession->pSocket->hPacketAllocator,
                    1024*64,
                    &packet.pRawBuffer,
                    &packet.bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBTreeAcquireMid(
                    pTree,
                    &usMid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                packet.pRawBuffer,
                packet.bufferLen,
                COM_NT_CREATE_ANDX,
                0,
                0,
                pTree->tid,
                gRdrRuntime.SysPid,
                pTree->pSession->uid,
                usMid,
                TRUE,
                &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    packet.pData = packet.pParams + sizeof(CREATE_REQUEST_HEADER);

    /* @todo: handle size restart */
    packet.bufferUsed += sizeof(CREATE_REQUEST_HEADER);

    /* If most commands have word counts which are easy to compute, this
       should be folded into a parameter to SMBPacketMarshallHeader() */
    packet.pSMBHeader->wordCount = 24;

    pHeader = (CREATE_REQUEST_HEADER *) packet.pParams;

    pHeader->reserved = 0;
    /* @todo: does the length include alignment padding? */
    pHeader->nameLength = (wc16slen(pwszPath) + 1) * sizeof(wchar16_t);
    pHeader->flags = 0;
    pHeader->rootDirectoryFid = usRootFid;
    pHeader->desiredAccess = desiredAccess;
    pHeader->allocationSize = llAllocationSize;
    pHeader->extFileAttributes = fileAttributes;
    pHeader->shareAccess = shareAccess;
    pHeader->createDisposition = createDisposition;
    pHeader->createOptions = createOptions;
    pHeader->impersonationLevel = 0x2; /* FIXME */

    ntStatus = WireMarshallCreateRequestData(
                packet.pData,
                packet.bufferLen - packet.bufferUsed,
                (packet.pData - (uint8_t *) packet.pSMBHeader) % 2,
                &packetByteCount,
                pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    assert(packetByteCount <= UINT16_MAX);
    pHeader->byteCount = (uint16_t) packetByteCount;
    packet.bufferUsed += packetByteCount;

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

    ntStatus = SMBPacketMarshallFooter(&packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBResponseCreate(usMid, &pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSrvClientTreeAddResponse(pTree, pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    /* @todo: on send packet error, the response must be removed from the
       tree.*/
    ntStatus = SMBSocketSend(pTree->pSession->pSocket, &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBTreeReceiveResponse(
                    pTree,
                    packet.haveSignature,
                    packet.sequence + 1,
                    pResponse,
                    &pResponsePacket);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = pResponsePacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireUnmarshallSMBResponseCreate(
                pResponsePacket->pParams,
                pResponsePacket->bufferLen - pResponsePacket->bufferUsed,
                &pResponseHeader);
    BAIL_ON_NT_STATUS(ntStatus);

    *pusFid = pResponseHeader->fid;
    *pusFileType = pResponseHeader->fileType;

cleanup:

    if (pResponsePacket)
    {
        SMBPacketRelease(
            pTree->pSession->pSocket->hPacketAllocator,
            pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBPacketBufferFree(pTree->pSession->pSocket->hPacketAllocator,
                            packet.pRawBuffer,
                            packet.bufferLen);
    }

    if (pResponse)
    {
        SMBResponseFree(pResponse);
    }

    return ntStatus;

error:

    *pusFid = 0;

    goto cleanup;
}
