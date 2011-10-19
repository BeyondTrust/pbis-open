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
 *        open.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB OPEN "wire" API
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: support big endian architectures
 * @todo: support AndX chain parsing
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "includes.h"

typedef struct
{
    uint8_t   FOR_REFERENCE_ONLY;

    wchar16_t name[0];          /* File to open or create */
} CREATE_REQUEST_DATA_non_castable;

NTSTATUS
WireUnmarshallCreateFileRequest(
    PBYTE  pParams,
    ULONG  ulBytesAvailable,
    ULONG  ulBytesUsed,
    PCREATE_REQUEST_HEADER* ppHeader,
    PWSTR* ppwszFilename
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pDataCursor = pParams;
    PCREATE_REQUEST_HEADER pHeader = NULL;
    PWSTR pwszFilename = NULL;
    USHORT alignment = 0;

    if (ulBytesAvailable < sizeof(CREATE_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PCREATE_REQUEST_HEADER)pParams;
    pDataCursor += sizeof(CREATE_REQUEST_HEADER);
    ulBytesUsed += sizeof(CREATE_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(CREATE_REQUEST_HEADER);

    if (ulBytesAvailable < pHeader->byteCount)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    alignment = (ulBytesUsed % 2);

    if (ulBytesAvailable < alignment)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulBytesUsed += alignment;
    pDataCursor += alignment;
    ulBytesAvailable -= alignment;

    pwszFilename = (PWSTR)pDataCursor;

    *ppHeader = pHeader;
    *ppwszFilename = pwszFilename;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppwszFilename = NULL;

    goto cleanup;
}

/* ASCII is not supported */
/* @todo: test alignment restrictions on Win2k */
NTSTATUS
WireMarshallCreateRequestData(
    OUT PBYTE pBuffer,
    IN ULONG bufferLen,
    IN uint8_t messageAlignment,
    OUT PULONG pBufferUsed,
    IN PCWSTR pwszPath
    )
{
    NTSTATUS ntStatus = 0;
    uint32_t bufferUsed = 0;
    uint32_t alignment = 0;

    /* Align strings */
    alignment = (bufferUsed + messageAlignment) % 2;
    if (alignment)
    {
        *(pBuffer + bufferUsed) = 0;
        bufferUsed += alignment;
    }

    ntStatus = SMBPacketAppendUnicodeString(pBuffer, bufferLen, &bufferUsed, pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

error:
    *pBufferUsed = bufferUsed;

    return ntStatus;
}

NTSTATUS
WireUnmarshallSMBResponseCreate(
    IN PBYTE pBuffer,
    IN ULONG bufferLen,
    OUT PCREATE_RESPONSE_HEADER* ppHeader
    )
{
    NTSTATUS ntStatus = 0;
    CREATE_RESPONSE_HEADER* pHeader = (CREATE_RESPONSE_HEADER*) pBuffer;
    ULONG bufferUsed = sizeof(CREATE_RESPONSE_HEADER);

    /* NOTE: The buffer format cannot be trusted! */
    if (bufferLen < bufferUsed)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // byte order conversions
    SMB_LTOH8_INPLACE(pHeader->oplockLevel);
    SMB_LTOH16_INPLACE(pHeader->fid);
    SMB_LTOH32_INPLACE(pHeader->createAction);
    SMB_LTOH64_INPLACE(pHeader->creationTime);
    SMB_LTOH64_INPLACE(pHeader->lastAccessTime);
    SMB_LTOH64_INPLACE(pHeader->lastWriteTime);
    SMB_LTOH64_INPLACE(pHeader->changeTime);
    SMB_LTOH32_INPLACE(pHeader->extFileAttributes);
    SMB_LTOH64_INPLACE(pHeader->allocationSize);
    SMB_LTOH64_INPLACE(pHeader->endOfFile);
    SMB_LTOH16_INPLACE(pHeader->fileType);
    SMB_LTOH16_INPLACE(pHeader->deviceState);
    SMB_LTOH8_INPLACE(pHeader->isDirectory);
    SMB_LTOH16_INPLACE(pHeader->byteCount);

error:
    if (ntStatus)
    {
        pHeader = NULL;
    }

    *ppHeader = pHeader;

    return ntStatus;
}
