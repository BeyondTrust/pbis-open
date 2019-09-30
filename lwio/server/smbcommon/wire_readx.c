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
WireUnmarshallReadAndXRequest_WC_10(
    const PBYTE pParams,
    ULONG       ulBytesAvailable,
    ULONG       ulBytesUsed,
    PREAD_ANDX_REQUEST_HEADER_WC_10* ppHeader
    )
{
    NTSTATUS ntStatus = 0;

    if (ulBytesAvailable < sizeof(READ_ANDX_REQUEST_HEADER_WC_10))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppHeader = (PREAD_ANDX_REQUEST_HEADER_WC_10)pParams;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;

    goto cleanup;
}

NTSTATUS
WireUnmarshallReadAndXRequest_WC_12(
    const PBYTE pParams,
    ULONG       ulBytesAvailable,
    ULONG       ulBytesUsed,
    PREAD_ANDX_REQUEST_HEADER_WC_12* ppHeader
    )
{
    NTSTATUS ntStatus = 0;

    if (ulBytesAvailable < sizeof(READ_ANDX_REQUEST_HEADER_WC_12))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppHeader = (PREAD_ANDX_REQUEST_HEADER_WC_12)pParams;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;

    goto cleanup;
}

NTSTATUS
WireMarshallReadResponseData(
    PBYTE  pDataBuffer,
    ULONG  ulBytesAvailable,
    ULONG  alignment,
    PVOID  pBuffer,
    ULONG  ulBytesToWrite,
    PULONG pulPackageByteCount
    )
{
    NTSTATUS ntStatus = 0;
    ULONG    ulBytesUsed = 0;
    PBYTE    pDataCursor = pDataBuffer;

    if (ulBytesAvailable < alignment)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulBytesAvailable -= alignment;
    ulBytesUsed += alignment;
    pDataCursor += alignment;

    if (ulBytesAvailable < ulBytesToWrite)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pDataCursor, pBuffer, ulBytesToWrite);

    // ulBytesAvailable -= ulBytesToWrite;
    ulBytesUsed += ulBytesToWrite;

    *pulPackageByteCount = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulPackageByteCount = 0;

    goto cleanup;
}

NTSTATUS
WireMarshallReadResponseDataEx(
    PBYTE   pDataBuffer,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PVOID   pBuffer,
    ULONG   ulBytesToWrite,
    PULONG  pulDataOffset,
    PULONG  pulPackageByteCount
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pDataCursor = pDataBuffer;
    ULONG    ulBytesUsed = 0;
    ULONG    ulAlignment = 0;

    if (ulBytesToWrite)
    {
        ulAlignment = (ulOffset % 2);
        /* Windows returns unaligned results past 0x1000 bytes */
        if (ulAlignment && ulBytesToWrite <= 0x1000)
        {
            if (ulBytesAvailable < ulAlignment)
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ulBytesAvailable -= ulAlignment;
            ulBytesUsed += ulAlignment;
            ulOffset += ulAlignment;
            pDataCursor += ulAlignment;
        }

        if (ulBytesAvailable < ulBytesToWrite)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (pBuffer)
        {
            memcpy(pDataCursor, pBuffer, ulBytesToWrite);
        }
        // ulBytesAvailable -= ulBytesToWrite;
        ulBytesUsed += ulBytesToWrite;
    }

    *pulDataOffset = ulOffset;
    *pulPackageByteCount = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulDataOffset = 0;
    *pulPackageByteCount = 0;

    goto cleanup;
}

NTSTATUS
MarshallReadRequestData(
    uint8_t         *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    uint32_t        *pBufferUsed
    )
{
    return 0;
}
