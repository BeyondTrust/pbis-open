/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        smbfileapi.h
 *
 * Abstract:
 *
 *        SMB-specific API functions
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 */

#include "includes.h"

#define MAX_KEY_LENGTH (1024*10)
#define ACCESS_TOKEN_LENGTH (8192)
#define RESOLVED_PATH_LENGTH (512)

LW_NTSTATUS
LwIoGetSessionKey(
    IO_FILE_HANDLE File,
    LW_PUSHORT pKeyLength,
    LW_PBYTE* ppKeyBuffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatus;
    BYTE Buffer[MAX_KEY_LENGTH];
    PBYTE pKeyBuffer = NULL;

    Status = 
        LwNtFsControlFile(
            File,
            NULL,
            &IoStatus,
            IO_FSCTL_SMB_GET_SESSION_KEY,
            NULL,
            0,
            Buffer,
            sizeof(Buffer));
    BAIL_ON_NT_STATUS(Status);

    if (IoStatus.BytesTransferred > 0)
    {
        Status = LwIoAllocateMemory(IoStatus.BytesTransferred, OUT_PPVOID(&pKeyBuffer));
        BAIL_ON_NT_STATUS(Status);

        memcpy(pKeyBuffer, Buffer, IoStatus.BytesTransferred);
        
        *pKeyLength = IoStatus.BytesTransferred;
        *ppKeyBuffer = pKeyBuffer;
    }
    else
    {
        *pKeyLength = 0;
        *ppKeyBuffer = NULL;
    }

cleanup:

    return Status;

error:

    *pKeyLength = 0;
    *ppKeyBuffer = NULL;
    
    goto cleanup;
}

LW_NTSTATUS
LwIoGetPeerAccessToken(
    IO_FILE_HANDLE File,
    PACCESS_TOKEN* ppToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatus;
    ULONG ulLength = ACCESS_TOKEN_LENGTH;
    PBYTE pBuffer = NULL;
    PBYTE pNewBuffer = NULL;

    Status = RTL_ALLOCATE(&pBuffer, BYTE, ulLength);
    BAIL_ON_NT_STATUS(Status);

    do
    {
        Status =
            LwNtFsControlFile(
                File,
                NULL,
                &IoStatus,
                IO_FSCTL_SMB_GET_PEER_ACCESS_TOKEN,
                NULL,
                0,
                pBuffer,
                ulLength);
        
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            ulLength *= 2;
            pNewBuffer = LwRtlMemoryRealloc(pBuffer, ulLength);
            if (!pNewBuffer)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                BAIL_ON_NT_STATUS(Status);
            }
            pBuffer = pNewBuffer;
        }
    } while (Status == STATUS_BUFFER_TOO_SMALL);

    BAIL_ON_NT_STATUS(Status);
        
    if (IoStatus.BytesTransferred > 0)
    {
        Status = RtlSelfRelativeAccessTokenToAccessToken(
            (PACCESS_TOKEN_SELF_RELATIVE) pBuffer,
            IoStatus.BytesTransferred,
            ppToken);
        BAIL_ON_NT_STATUS(Status);
    }
    else
    {
        *ppToken = NULL;
    }
    
cleanup:

    RTL_FREE(&pBuffer);

    return Status;

error:

    *ppToken = NULL;

    goto cleanup;
}

LW_NTSTATUS
LwIoGetPeerAddress(
    IO_FILE_HANDLE File,
    LW_PBYTE pAddress,
    LW_PUSHORT pusAddressLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatus;

    Status = 
        LwNtFsControlFile(
            File,
            NULL,
            &IoStatus,
            IO_FSCTL_SMB_GET_PEER_ADDRESS,
            NULL,
            0,
            pAddress,
            *pusAddressLength);
    BAIL_ON_NT_STATUS(Status);
    
    *pusAddressLength = (USHORT) IoStatus.BytesTransferred;

cleanup:

    return Status;

error:

    *pusAddressLength = 0;

    goto cleanup;
}


LW_NTSTATUS
LwIoConnectNamedPipe(
    IO_FILE_HANDLE File,
    PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    PIO_STATUS_BLOCK IoStatusBlock
    )
{
    return NtFsControlFile(
        File,
        AsyncControlBlock,
        IoStatusBlock,
        0x2,
        NULL,
        0,
        NULL,
        0
        );
}

LW_NTSTATUS
LwIoSetRdrDomainHints(
    LW_PWSTR* ppwszDomains,
    ULONG ulCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatus = {0};
    WCHAR wszRdrPath[] = {'\\', 'r', 'd', 'r', '\0'};
    UNICODE_STRING rdrPath = LW_RTL_CONSTANT_STRING(wszRdrPath);
    IO_FILE_NAME fileName = {0};
    IO_FILE_HANDLE hFile = NULL;
    PWSTR pwszBuffer = NULL;
    ULONG ulLength = 0;
    ULONG ulIndex = 0;
    ULONG ulOffset = 0;
    ULONG ulStrLen = 0;

    for (ulIndex = 0; ulIndex < ulCount; ulIndex++)
    {
        ulLength += (LwRtlWC16StringNumChars(ppwszDomains[ulIndex]) + 1) * sizeof(WCHAR);
    }

    status = RTL_ALLOCATE(&pwszBuffer, WCHAR, ulLength);
    BAIL_ON_NT_STATUS(status);

    for (ulIndex = 0; ulIndex < ulCount; ulIndex++)
    {
        ulStrLen = (LwRtlWC16StringNumChars(ppwszDomains[ulIndex]) + 1) * sizeof(WCHAR);

        memcpy((PBYTE) pwszBuffer + ulOffset, ppwszDomains[ulIndex], ulStrLen);
        ulOffset += ulStrLen;
    }

    fileName.Name = rdrPath;

    status = LwNtCreateFile(
        &hFile,
        NULL,
        &ioStatus,
        &fileName,
        NULL,
        NULL,
        FILE_GENERIC_WRITE,
        0,
        0,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        0,
        NULL,
        0,
        NULL,
        NULL);
    BAIL_ON_NT_STATUS(status);

    status = LwNtDeviceIoControlFile(
        hFile,
        NULL,
        &ioStatus,
        RDR_DEVCTL_SET_DOMAIN_HINTS,
        pwszBuffer,
        ulLength,
        NULL,
        0);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RTL_FREE(&pwszBuffer);

    if (hFile)
    {
        LwNtCloseFile(hFile);
    }

    return status;

error:

    goto cleanup;
}

LW_NTSTATUS
LwIoRdrGetPhysicalPath(
    IO_FILE_HANDLE File,
    LW_PWSTR* ppResolved
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatus;
    ULONG length = RESOLVED_PATH_LENGTH;
    PBYTE pBuffer = NULL;
    PBYTE pNewBuffer = NULL;
    PWSTR pResolved = NULL;

    status = RTL_ALLOCATE(&pBuffer, BYTE, length);
    GOTO_CLEANUP_ON_STATUS(status);

    do
    {
        status =
            LwNtDeviceIoControlFile(
                File,
                NULL,
                &ioStatus,
                RDR_DEVCTL_GET_PHYSICAL_PATH,
                NULL,
                0,
                pBuffer,
                length);

        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            length *= 2;
            pNewBuffer = LwRtlMemoryRealloc(pBuffer, length);
            if (!pNewBuffer)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                GOTO_CLEANUP_ON_STATUS(status);
            }
            pBuffer = pNewBuffer;
        }
    } while (status == STATUS_BUFFER_TOO_SMALL);

    GOTO_CLEANUP_ON_STATUS(status);

    if (ioStatus.BytesTransferred > 0)
    {
        status = LW_RTL_ALLOCATE(
            &pResolved,
            WCHAR,
            ioStatus.BytesTransferred + sizeof(WCHAR));
        GOTO_CLEANUP_ON_STATUS(status);

#ifdef WORDS_BIGENDIAN
        swab(pBuffer, pResolved, ioStatus.BytesTransferred);
#else
        memcpy(pResolved, pBuffer, ioStatus.BytesTransferred);
#endif
    }

cleanup:

    *ppResolved = pResolved;

    RTL_FREE(&pBuffer);

    return status;
}

VOID
LwIoRdrFreePhysicalPath(
    LW_PWSTR pResolved
    )
{
    RTL_FREE(&pResolved);
}
