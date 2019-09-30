/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

#include "includes.h"

NTSTATUS
WireUnmarshallWriteAndXRequest_WC_12(
    const PBYTE            pParams,
    ULONG                  ulBytesAvailable,
    ULONG                  ulOffset,
    PWRITE_ANDX_REQUEST_HEADER_WC_12* ppHeader,
    PBYTE*                 ppData
    )
{
    NTSTATUS ntStatus = 0;
    PWRITE_ANDX_REQUEST_HEADER_WC_12 pHeader = NULL;
    PBYTE pDataCursor = pParams;
    PBYTE pSmbHeader = pParams - ulOffset;
    ULONG ulTotalBytesAvailable = ulBytesAvailable + ulOffset;

    if (ulBytesAvailable < sizeof(WRITE_ANDX_REQUEST_HEADER_WC_12))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PWRITE_ANDX_REQUEST_HEADER_WC_12)pDataCursor;

    ulBytesAvailable -= sizeof(WRITE_ANDX_REQUEST_HEADER_WC_12);
    pDataCursor += sizeof(WRITE_ANDX_REQUEST_HEADER_WC_12);

    // Total number of bytes must be more than the data offset
    if (ulTotalBytesAvailable < pHeader->dataOffset)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // TODO: check if we have enough data bytes available

    *ppHeader = pHeader;
    *ppData = pSmbHeader + pHeader->dataOffset;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppData = NULL;

    goto cleanup;
}

NTSTATUS
WireUnmarshallWriteAndXRequest_WC_14(
    const PBYTE            pParams,
    ULONG                  ulBytesAvailable,
    ULONG                  ulOffset,
    PWRITE_ANDX_REQUEST_HEADER_WC_14* ppHeader,
    PBYTE*                 ppData
    )
{
    NTSTATUS ntStatus = 0;
    PWRITE_ANDX_REQUEST_HEADER_WC_14 pHeader = NULL;
    PBYTE pDataCursor = pParams;
    PBYTE pSmbHeader = pParams - ulOffset;
    ULONG ulTotalBytesAvailable = ulBytesAvailable + ulOffset;

    if (ulBytesAvailable < sizeof(WRITE_ANDX_REQUEST_HEADER_WC_14))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PWRITE_ANDX_REQUEST_HEADER_WC_14)pDataCursor;

    ulBytesAvailable -= sizeof(WRITE_ANDX_REQUEST_HEADER_WC_14);
    pDataCursor += sizeof(WRITE_ANDX_REQUEST_HEADER_WC_14);

    // Total number of bytes must be more than the data offset
    if (ulTotalBytesAvailable < pHeader->dataOffset)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // TODO: check if we have enough data bytes available

    *ppHeader = pHeader;
    *ppData = pSmbHeader + pHeader->dataOffset;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppData = NULL;

    goto cleanup;
}

NTSTATUS
MarshallWriteRequestData(
    uint8_t         *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    uint32_t        *pBufferUsed,
    uint16_t        *pDataOffset,
    uint8_t         *pWriteBuffer,
    uint16_t        wWriteLen
    )
{
    NTSTATUS ntStatus = 0;

    uint32_t bufferUsed = 0;
    uint32_t alignment = 0;
    uint32_t dataOffset = 0;

    alignment = (bufferUsed + messageAlignment) % 2;
    if (alignment)
    {
        *(pBuffer + bufferUsed) = 0;
        bufferUsed += alignment;
    }

    dataOffset = bufferUsed;

    memcpy(pBuffer + bufferUsed, pWriteBuffer, wWriteLen);

    bufferUsed += wWriteLen;

    if (bufferUsed > bufferLen)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        goto error;
    }

    *pBufferUsed = bufferUsed;
    *pDataOffset = (uint16_t)dataOffset;

cleanup:

    return ntStatus;

error:

    *pBufferUsed = 0;

    goto cleanup;
}

