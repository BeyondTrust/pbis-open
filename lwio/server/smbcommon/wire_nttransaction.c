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


#include "includes.h"

static
NTSTATUS
WireUnmarshallNtTransactionSetupData(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    ULONG       ulParameterOffset,
    ULONG       ulDataOffset,
    PUSHORT*    ppSetup,
    UCHAR       ucSetupLen,
    PUSHORT*    ppByteCount,
    PBYTE*      ppParameters,
    USHORT      parameterLen,
    PBYTE*      ppData,
    USHORT      dataLen
    );

static
NTSTATUS
WireUnmarshallNtTransactionParameterData(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    ULONG       ulParameterOffset,
    ULONG       ulDataOffset,
    PBYTE*      ppParameters,
    ULONG       parameterLen,
    PBYTE*      ppData,
    ULONG       dataLen
    );

NTSTATUS
WireUnmarshallNtTransactionRequest(
    const PBYTE                     pBuffer,
    ULONG                           ulNumBytesAvailable,
    ULONG                           ulOffset,
    PNT_TRANSACTION_REQUEST_HEADER* ppHeader,
    PUSHORT*                        ppSetup,
    PUSHORT*                        ppByteCount,
    PBYTE*                          ppParameters,
    PBYTE*                          ppData
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pDataCursor = pBuffer;
    PNT_TRANSACTION_REQUEST_HEADER pHeader = NULL;
    PUSHORT pSetup = NULL;
    PBYTE   pParameters = NULL;
    PBYTE   pData = NULL;
    PUSHORT pByteCount = NULL;

    if (ulNumBytesAvailable < sizeof(NT_TRANSACTION_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PNT_TRANSACTION_REQUEST_HEADER) pDataCursor;

    pDataCursor += sizeof(NT_TRANSACTION_REQUEST_HEADER);
    ulNumBytesAvailable -= sizeof(NT_TRANSACTION_REQUEST_HEADER);
    ulOffset += sizeof(NT_TRANSACTION_REQUEST_HEADER);

    ntStatus = WireUnmarshallNtTransactionSetupData(
        pDataCursor,
        ulNumBytesAvailable,
        ulOffset,
        pHeader->ulParameterOffset,
        pHeader->ulDataOffset,
        &pSetup,
        pHeader->ucSetupCount,
        &pByteCount,
        &pParameters,
        pHeader->ulParameterCount,
        &pData,
        pHeader->ulDataCount);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppHeader = pHeader;
    *ppSetup = pSetup;
    *ppByteCount = pByteCount;
    *ppParameters = pParameters;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppSetup = NULL;
    *ppByteCount = NULL;
    *ppParameters = NULL;
    *ppData = NULL;

    goto cleanup;
}

static
NTSTATUS
WireUnmarshallNtTransactionSetupData(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    ULONG       ulParameterOffset,
    ULONG       ulDataOffset,
    PUSHORT*    ppSetup,
    UCHAR       ucSetupLen,
    PUSHORT*    ppByteCount,
    PBYTE*      ppParameters,
    USHORT      parameterLen,
    PBYTE*      ppData,
    USHORT      dataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pDataCursor = pBuffer;
    PUSHORT pSetup = NULL;
    PBYTE   pParameters = NULL;
    PBYTE   pData = NULL;
    PUSHORT pByteCount = NULL;

    if (ucSetupLen)
    {
        USHORT  usSetupLen = (ucSetupLen * sizeof(USHORT));

        if (ulNumBytesAvailable < usSetupLen)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pSetup = (PUSHORT) pDataCursor;
        pDataCursor += usSetupLen;
        ulNumBytesAvailable -= usSetupLen;
        ulOffset += usSetupLen;
    }

    if (ulNumBytesAvailable < sizeof(USHORT))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pByteCount = (PUSHORT) pDataCursor;

    pDataCursor += sizeof(USHORT);
    ulNumBytesAvailable -= sizeof(USHORT);
    ulOffset += sizeof(USHORT);

    if (parameterLen || dataLen)
    {
        ntStatus = WireUnmarshallNtTransactionParameterData(
            pDataCursor,
            ulNumBytesAvailable,
            ulOffset,
            ulParameterOffset,
            ulDataOffset,
            &pParameters,
            parameterLen,
            &pData,
            dataLen);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppSetup = pSetup;
    *ppParameters = pParameters;
    *ppData = pData;
    *ppByteCount = pByteCount;

cleanup:

    return ntStatus;

error:

    *ppSetup = NULL;
    *ppParameters = NULL;
    *ppData = NULL;
    *ppByteCount = NULL;

    goto cleanup;
}

static
NTSTATUS
WireUnmarshallNtTransactionParameterData(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    ULONG       ulParameterOffset,
    ULONG       ulDataOffset,
    PBYTE*      ppParameters,
    ULONG       parameterLen,
    PBYTE*      ppData,
    ULONG       dataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pDataCursor = pBuffer;
    PBYTE    pParameters = NULL;
    PBYTE    pData = NULL;

    if (ulOffset % 4)
    {
        USHORT usAlignment = (4 - (ulOffset % 4));

        if (ulNumBytesAvailable < usAlignment)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pDataCursor += usAlignment;
        ulNumBytesAvailable -= usAlignment;
        ulOffset += usAlignment;
    }

    if (ulOffset > ulParameterOffset)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else if (ulOffset < ulParameterOffset)
    {
        USHORT usOffsetDelta = ulParameterOffset - ulOffset;

        if (ulNumBytesAvailable < usOffsetDelta)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulOffset += usOffsetDelta;
        pDataCursor += usOffsetDelta;
        ulNumBytesAvailable -= usOffsetDelta;
    }

    if (ulNumBytesAvailable < parameterLen)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (parameterLen)
    {
        pParameters = pDataCursor;

        pDataCursor += parameterLen;
        ulNumBytesAvailable -= parameterLen;
        ulOffset += parameterLen;
    }

    if (dataLen)
    {
        if (ulOffset > ulDataOffset)
        {
            ntStatus = STATUS_DATA_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else if (ulOffset < ulDataOffset)
        {
            USHORT usOffsetDelta = ulDataOffset - ulOffset;

            if (ulNumBytesAvailable < usOffsetDelta)
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ulOffset += usOffsetDelta;
            pDataCursor += usOffsetDelta;
            ulNumBytesAvailable -= usOffsetDelta;
        }

        pData = pDataCursor;
    }

    *ppParameters = pParameters;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppParameters = NULL;
    *ppData = NULL;

    goto cleanup;
}

NTSTATUS
WireMarshallNtTransactionResponse(
    PBYTE   pBuffer,
    ULONG   ulNumBytesAvailable,
    ULONG   ulOffset,
    PUSHORT pSetup,
    UCHAR   ucSetupCount,
    PBYTE   pParams,
    ULONG   ulParamLength,
    PBYTE   pData,
    ULONG   ulDataLen,
    PULONG  pulDataOffset,
    PULONG  pulParameterOffset,
    PULONG  pulNumPackageBytesUsed
    )
{
    NTSTATUS ntStatus = 0;
    PNT_TRANSACTION_SECONDARY_RESPONSE_HEADER pResponseHeader = NULL;
    PUSHORT  pByteCount = NULL;
    ULONG    ulNumPackageBytesUsed = 0;
    USHORT   usNumBytesUsed = 0;
    PBYTE    pDataCursor = pBuffer;

    if (ulNumBytesAvailable < sizeof(NT_TRANSACTION_SECONDARY_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PNT_TRANSACTION_SECONDARY_RESPONSE_HEADER)pDataCursor;
    memset(pResponseHeader, 0, sizeof(*pResponseHeader));

    pDataCursor += sizeof(NT_TRANSACTION_SECONDARY_RESPONSE_HEADER);
    ulOffset += sizeof(NT_TRANSACTION_SECONDARY_RESPONSE_HEADER);
    ulNumBytesAvailable -= sizeof(NT_TRANSACTION_SECONDARY_RESPONSE_HEADER);
    ulNumPackageBytesUsed += sizeof(NT_TRANSACTION_SECONDARY_RESPONSE_HEADER);

    pResponseHeader->ucSetupCount = ucSetupCount;

    if (ucSetupCount)
    {
        USHORT usSetupLen = ucSetupCount * sizeof(USHORT);

        if (ulNumBytesAvailable < usSetupLen)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pDataCursor, (PBYTE)pSetup, usSetupLen);

        pDataCursor += usSetupLen;
        ulOffset += usSetupLen;
        ulNumBytesAvailable -= usSetupLen;
        ulNumPackageBytesUsed += usSetupLen;
    }

    if (ulNumBytesAvailable < sizeof(USHORT))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pByteCount = (PUSHORT)pDataCursor;

    pDataCursor += sizeof(USHORT);
    ulNumBytesAvailable -= sizeof(USHORT);
    ulOffset += sizeof(USHORT);
    ulNumPackageBytesUsed += sizeof(USHORT);

    if (ulOffset % 4)
    {
        USHORT usAlignment = 4 - (ulOffset % 4);
        if (ulNumBytesAvailable < usAlignment)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pDataCursor += usAlignment;
        ulNumBytesAvailable -= usAlignment;
        ulOffset += usAlignment;
        usNumBytesUsed += usAlignment;
        ulNumPackageBytesUsed += usAlignment;
    }

    pResponseHeader->ulParameterCount = ulParamLength;
    pResponseHeader->ulTotalParameterCount = ulParamLength;
    pResponseHeader->ulParameterOffset = ulOffset;
    pResponseHeader->ulParameterDisplacement = 0;

    if (pParams)
    {
        if (ulNumBytesAvailable < ulParamLength)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pDataCursor, pParams, ulParamLength);

        pDataCursor += ulParamLength;

        ulNumBytesAvailable -= ulParamLength;
        ulOffset += ulParamLength;
        usNumBytesUsed += ulParamLength;
        ulNumPackageBytesUsed += ulParamLength;
    }

    if (ulOffset % 4)
    {
        USHORT usAlignment = 4 - (ulOffset % 4);
        if (ulNumBytesAvailable < usAlignment)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pDataCursor += usAlignment;
        ulNumBytesAvailable -= usAlignment;
        ulOffset += usAlignment;
        usNumBytesUsed += usAlignment;
        ulNumPackageBytesUsed += usAlignment;
    }

    pResponseHeader->ulDataCount = ulDataLen;
    pResponseHeader->ulTotalDataCount = ulDataLen;
    pResponseHeader->ulDataOffset = ulOffset;
    pResponseHeader->ulDataDisplacement = 0;

    if (pData)
    {
        if (ulNumBytesAvailable < ulDataLen)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pDataCursor, pData, ulDataLen);

        pDataCursor += ulDataLen;
        ulNumBytesAvailable -= ulDataLen;
        ulOffset += ulDataLen;
        usNumBytesUsed += ulDataLen;
        ulNumPackageBytesUsed += ulDataLen;
    }

    *pByteCount = usNumBytesUsed;

    *pulDataOffset = pResponseHeader->ulDataOffset;
    *pulParameterOffset = pResponseHeader->ulParameterOffset;
    *pulNumPackageBytesUsed = ulNumPackageBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulDataOffset = 0;
    *pulParameterOffset = 0;
    *pulNumPackageBytesUsed = 0;

    goto cleanup;
}

NTSTATUS
WireMarshallNtTransactionRequest(
    PBYTE   pBuffer,
    ULONG   ulNumBytesAvailable,
    ULONG   ulOffset,
    USHORT  usFunction,
    PUSHORT pSetup,
    UCHAR   ucSetupCount,
    PBYTE   pParams,
    ULONG   ulParamLength,
    PBYTE   pData,
    ULONG   ulDataLen,
    PULONG  pulDataOffset,
    PULONG  pulParameterOffset,
    PULONG  pulNumPackageBytesUsed
    )
{
    NTSTATUS ntStatus = 0;
    PNT_TRANSACTION_REQUEST_HEADER pRequestHeader = NULL;
    PUSHORT  pByteCount = NULL;
    ULONG    ulNumPackageBytesUsed = 0;
    USHORT   usNumBytesUsed = 0;
    PBYTE    pDataCursor = pBuffer;

    if (ulNumBytesAvailable < sizeof(NT_TRANSACTION_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PNT_TRANSACTION_REQUEST_HEADER)pDataCursor;

    pDataCursor += sizeof(NT_TRANSACTION_REQUEST_HEADER);
    ulOffset += sizeof(NT_TRANSACTION_REQUEST_HEADER);
    ulNumBytesAvailable -= sizeof(NT_TRANSACTION_REQUEST_HEADER);
    ulNumPackageBytesUsed += sizeof(NT_TRANSACTION_REQUEST_HEADER);

    pRequestHeader->usFunction   = usFunction;
    pRequestHeader->ucSetupCount = ucSetupCount;

    if (ucSetupCount)
    {
        USHORT usSetupLen = ucSetupCount * sizeof(USHORT);

        if (ulNumBytesAvailable < usSetupLen)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pDataCursor, (PBYTE)pSetup, usSetupLen);

        pDataCursor += usSetupLen;
        ulOffset += usSetupLen;
        ulNumBytesAvailable -= usSetupLen;
        ulNumPackageBytesUsed += usSetupLen;
    }

    if (ulNumBytesAvailable < sizeof(USHORT))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pByteCount = (PUSHORT)pDataCursor;

    pDataCursor += sizeof(USHORT);
    ulNumBytesAvailable -= sizeof(USHORT);
    ulOffset += sizeof(USHORT);
    usNumBytesUsed += sizeof(USHORT);
    ulNumPackageBytesUsed += sizeof(USHORT);

    if (ulOffset % 4)
    {
        USHORT usAlignment = 4 - (ulOffset % 4);
        if (ulNumBytesAvailable < usAlignment)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pDataCursor += usAlignment;
        ulNumBytesAvailable -= usAlignment;
        ulOffset += usAlignment;
        usNumBytesUsed += usAlignment;
        ulNumPackageBytesUsed += usAlignment;
    }

    pRequestHeader->ulParameterCount = ulParamLength;
    pRequestHeader->ulTotalParameterCount = ulParamLength;
    pRequestHeader->ulParameterOffset = ulOffset;

    if (pParams)
    {
        if (ulNumBytesAvailable < ulParamLength)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pDataCursor, pParams, ulParamLength);

        pDataCursor += ulParamLength;

        ulNumBytesAvailable -= ulParamLength;
        ulOffset += ulParamLength;
        usNumBytesUsed += ulParamLength;
        ulNumPackageBytesUsed += ulParamLength;
    }

    if (ulOffset % 4)
    {
        USHORT usAlignment = 4 - (ulOffset % 4);
        if (ulNumBytesAvailable < usAlignment)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pDataCursor += usAlignment;
        ulNumBytesAvailable -= usAlignment;
        ulOffset += usAlignment;
        usNumBytesUsed += usAlignment;
        ulNumPackageBytesUsed += usAlignment;
    }

    pRequestHeader->ulDataCount = ulDataLen;
    pRequestHeader->ulTotalDataCount = ulDataLen;
    pRequestHeader->ulDataOffset = ulOffset;

    if (pData)
    {
        if (ulNumBytesAvailable < ulDataLen)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pDataCursor, pData, ulDataLen);

        pDataCursor += ulDataLen;
        ulNumBytesAvailable -= ulDataLen;
        ulOffset += ulDataLen;
        usNumBytesUsed += ulDataLen;
        ulNumPackageBytesUsed += ulDataLen;
    }

    *pByteCount = usNumBytesUsed;

    *pulDataOffset = pRequestHeader->ulDataOffset;
    *pulParameterOffset = pRequestHeader->ulParameterOffset;
    *pulNumPackageBytesUsed = ulNumPackageBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulDataOffset = 0;
    *pulParameterOffset = 0;
    *pulNumPackageBytesUsed = 0;

    goto cleanup;
}


