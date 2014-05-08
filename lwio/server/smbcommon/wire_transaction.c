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

typedef struct
{
    uint8_t   FOR_REFERENCE_ONLY;

    uint16_t  setup[0];         /* Setup words (# = setupWordCount) */
    uint16_t  byteCount;        /* Count of data bytes */
    wchar16_t name[0];          /* Must be NULL */
    uint8_t   pad[0];           /* Pad to SHORT or LONG */
    uint8_t   parameters[0];    /* Param. bytes (# = parameterCount) */
    uint8_t   pad1[0];          /* Pad to SHORT or LONG */
    uint8_t   data[0];          /* Data bytes (# = DataCount) */
} TRANSACTION_REQUEST_DATA_non_castable;

static
NTSTATUS
WireUnmarshallTransactionSetupData(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    ULONG       ulParameterOffset,
    ULONG       ulDataOffset,
    PUSHORT*    ppSetup,
    BYTE        setupLen,
    PUSHORT*    ppByteCount,
    PWSTR*      ppwszName,
    PBYTE*      ppParameters,
    USHORT      parameterLen,
    PBYTE*      ppData,
    USHORT      dataLen
    );

static
NTSTATUS
WireUnmarshallTransactionParameterData(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    ULONG       ulParameterOffset,
    ULONG       ulDataOffset,
    PWSTR*      ppwszName,
    PBYTE*      ppParameters,
    ULONG       parameterLen,
    PBYTE*      ppData,
    ULONG       dataLen
    );

static
NTSTATUS
WireMarshallTransactionSetupData(
    PBYTE      pBuffer,
    uint32_t   bufferLen,
    uint32_t  *pBufferUsed,
    uint16_t  *pSetup,
    uint8_t    setupLen,
    wchar16_t *pwszName,
    uint8_t   *pParameters,
    uint32_t   parameterLen,
    uint16_t  *pParameterOffset,
    uint8_t   *pData,
    uint32_t   dataLen,
    uint16_t  *pDataOffset
    );

static
NTSTATUS
WireMarshallTransactionParameterData(
    uint8_t  *pBuffer,
    uint32_t  bufferLen,
    uint32_t *pBufferUsed,
    uint8_t  *pParameters,
    uint32_t  parameterLen,
    uint16_t *pParameterOffset,
    uint8_t  *pData,
    uint32_t  dataLen,
    uint16_t *pDataOffset
    );

NTSTATUS
WireUnmarshallTransactionRequest(
    const PBYTE                  pBuffer,
    ULONG                        ulNumBytesAvailable,
    ULONG                        ulOffset,
    PTRANSACTION_REQUEST_HEADER* ppHeader,
    PUSHORT*                     ppSetup,
    PUSHORT*                     ppByteCount,
    PWSTR*                       ppwszName,
    PBYTE*                       ppParameters,
    PBYTE*                       ppData
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pDataCursor = pBuffer;
    PTRANSACTION_REQUEST_HEADER pHeader = NULL;
    PUSHORT pSetup = NULL;
    PUSHORT pByteCount = NULL;
    PWSTR   pwszName = NULL;
    PBYTE   pParameters = NULL;
    PBYTE   pData = NULL;

    if (ulNumBytesAvailable < sizeof(TRANSACTION_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PTRANSACTION_REQUEST_HEADER) pDataCursor;

    pDataCursor += sizeof(TRANSACTION_REQUEST_HEADER);
    ulNumBytesAvailable -= sizeof(TRANSACTION_REQUEST_HEADER);
    ulOffset += sizeof(TRANSACTION_REQUEST_HEADER);

    ntStatus = WireUnmarshallTransactionSetupData(
                    pDataCursor,
                    ulNumBytesAvailable,
                    ulOffset,
                    pHeader->parameterOffset,
                    pHeader->dataOffset,
                    &pSetup,
                    pHeader->setupCount,
                    &pByteCount,
                    (ppwszName ? &pwszName : NULL),
                    &pParameters,
                    pHeader->parameterCount,
                    &pData,
                    pHeader->dataCount);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppHeader = pHeader;
    *ppSetup = pSetup;
    *ppByteCount = pByteCount;

    if (ppwszName)
    {
        *ppwszName = pwszName;
    }

    *ppParameters = pParameters;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppSetup = NULL;
    *ppByteCount = NULL;

    if (ppwszName)
    {
        *ppwszName = NULL;
    }

    *ppParameters = NULL;
    *ppData = NULL;

    goto cleanup;
}

NTSTATUS
WireUnmarshallTransactionSecondaryRequest(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    PTRANSACTION_SECONDARY_REQUEST_HEADER* ppHeader,
    PWSTR*      ppwszName,
    PBYTE*      ppParameters,
    PBYTE*      ppData,
    USHORT      dataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pDataCursor = pBuffer;
    PTRANSACTION_SECONDARY_REQUEST_HEADER pHeader = NULL;
    PWSTR pwszName = NULL;
    PBYTE pParameters = NULL;
    PBYTE pData = NULL;

    if (ulNumBytesAvailable < sizeof(TRANSACTION_SECONDARY_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PTRANSACTION_SECONDARY_REQUEST_HEADER)pDataCursor;

    pDataCursor += sizeof(TRANSACTION_SECONDARY_REQUEST_HEADER);
    ulNumBytesAvailable -= sizeof(TRANSACTION_SECONDARY_REQUEST_HEADER);
    ulOffset += sizeof(TRANSACTION_SECONDARY_REQUEST_HEADER);

    ntStatus = WireUnmarshallTransactionParameterData(
                    pDataCursor,
                    ulNumBytesAvailable,
                    ulOffset,
                    pHeader->parameterOffset,
                    pHeader->dataOffset,
                    (ppwszName ? &pwszName : NULL),
                    &pParameters,
                    pHeader->parameterCount,
                    &pData,
                    pHeader->dataCount);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppHeader = pHeader;
    if (ppwszName)
    {
        *ppwszName = pwszName;
    }
    *ppParameters = pParameters;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    if (ppwszName)
    {
        *ppwszName = NULL;
    }
    *ppParameters = NULL;
    *ppData = NULL;

    goto cleanup;
}

NTSTATUS
WireUnmarshallTransactionSecondaryResponse(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    PTRANSACTION_SECONDARY_RESPONSE_HEADER* ppHeader,
    PUSHORT*    ppSetup,
    PUSHORT*    ppByteCount,
    PWSTR*      ppwszName,
    PBYTE*      ppParameters,
    PBYTE*      ppData,
    USHORT      dataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pDataCursor = pBuffer;
    PTRANSACTION_SECONDARY_RESPONSE_HEADER pHeader = NULL;
    PUSHORT pSetup = NULL;
    PBYTE   pParameters = NULL;
    PBYTE   pData = NULL;
    PUSHORT pByteCount = NULL;
    PWSTR   pwszName = NULL;

    if (ulNumBytesAvailable < sizeof(TRANSACTION_SECONDARY_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PTRANSACTION_SECONDARY_RESPONSE_HEADER)pDataCursor;

    pDataCursor += sizeof(TRANSACTION_SECONDARY_RESPONSE_HEADER);
    ulNumBytesAvailable -= sizeof(TRANSACTION_SECONDARY_RESPONSE_HEADER);
    ulOffset += sizeof(TRANSACTION_SECONDARY_RESPONSE_HEADER);

    ntStatus = WireUnmarshallTransactionSetupData(
                    pDataCursor,
                    ulNumBytesAvailable,
                    ulOffset,
                    SMB_LTOH16(pHeader->parameterOffset),
                    SMB_LTOH16(pHeader->dataOffset),
                    &pSetup,
                    SMB_LTOH8(pHeader->setupCount),
                    &pByteCount,
                    (ppwszName ? &pwszName : NULL),
                    &pParameters,
                    SMB_LTOH16(pHeader->parameterCount),
                    &pData,
                    SMB_LTOH16(pHeader->dataCount));
    BAIL_ON_NT_STATUS(ntStatus);

    *ppHeader = pHeader;
    *ppSetup = pSetup;
    *ppByteCount = pByteCount;
    if (ppwszName)
    {
        *ppwszName = pwszName;
    }
    *ppParameters = pParameters;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppSetup = NULL;
    *ppByteCount = NULL;
    if (ppwszName)
    {
        *ppwszName = NULL;
    }
    *ppParameters = NULL;
    *ppData = NULL;

    goto cleanup;
}

static
NTSTATUS
WireUnmarshallTransactionSetupData(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    ULONG       ulParameterOffset,
    ULONG       ulDataOffset,
    PUSHORT*    ppSetup,
    BYTE        setupLen,
    PUSHORT*    ppByteCount,
    PWSTR*      ppwszName,
    PBYTE*      ppParameters,
    USHORT      parameterLen,
    PBYTE*      ppData,
    USHORT      dataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pDataCursor = pBuffer;
    USHORT  usSetupLen = 0;
    PUSHORT pSetup = NULL;
    PUSHORT pByteCount = NULL;
    PBYTE   pParameters = NULL;
    PBYTE   pData = NULL;
    PWSTR   pwszName = NULL;

    usSetupLen = (setupLen * sizeof(USHORT));

    if (usSetupLen)
    {
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

    pByteCount = (PUSHORT)pDataCursor;
    pDataCursor += sizeof(USHORT);
    ulNumBytesAvailable -= sizeof(USHORT);
    ulOffset += sizeof(USHORT);

    ntStatus = WireUnmarshallTransactionParameterData(
                    pDataCursor,
                    ulNumBytesAvailable,
                    ulOffset,
                    ulParameterOffset,
                    ulDataOffset,
                    (ppwszName ? &pwszName : NULL),
                    &pParameters,
                    parameterLen,
                    &pData,
                    dataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSetup = pSetup;
    *ppByteCount = pByteCount;
    if (ppwszName)
    {
        *ppwszName = pwszName;
    }
    *ppParameters = pParameters;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppSetup = NULL;
    *ppByteCount = NULL;
    if (ppwszName)
    {
        *ppwszName = NULL;
    }
    *ppParameters = NULL;
    *ppData = NULL;

    goto cleanup;
}

static
NTSTATUS
WireUnmarshallTransactionParameterData(
    const PBYTE pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    ULONG       ulParameterOffset,
    ULONG       ulDataOffset,
    PWSTR*      ppwszName,
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
    PWSTR    pwszName = NULL;
    PWSTR    pwszCursor = NULL;

    if (ppwszName)
    {
        if (ulOffset % 2)
        {
            USHORT usAlignment = ulOffset % 2;

            if (ulNumBytesAvailable < usAlignment)
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pDataCursor += usAlignment;
            ulNumBytesAvailable -= usAlignment;
            ulOffset += usAlignment;
        }

        do
        {
            if (ulNumBytesAvailable < sizeof(wchar16_t))
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            if (!pwszName)
            {
                pwszName = pwszCursor = (PWSTR)pDataCursor;
            }
            else
            {
                pwszCursor++;
            }

            pDataCursor += sizeof(wchar16_t);
            ulNumBytesAvailable -= sizeof(wchar16_t);
            ulOffset += sizeof(wchar16_t);

        } while ((ulNumBytesAvailable > 0) && pwszCursor && *pwszCursor);
    }

    /* OS X 10.5 sends a 0 param Offset and 0 paramCount
       in the TransactNmPipe for RPC binds */

    if ((ulOffset > ulParameterOffset) && (parameterLen != 0))
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

    if ((ulOffset % 2) && (ulOffset %4))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
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
        if (ulOffset % 2)
        {
            USHORT usAlignment = (2 - (ulOffset % 2));

            if (ulNumBytesAvailable < usAlignment)
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pDataCursor += usAlignment;
            ulNumBytesAvailable -= usAlignment;
            ulOffset += usAlignment;
        }

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

    if (ppwszName)
    {
        *ppwszName = pwszName;
    }
    *ppParameters = pParameters;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    if (ppwszName)
    {
        *ppwszName = NULL;
    }
    *ppParameters = NULL;
    *ppData = NULL;

    goto cleanup;
}

typedef struct
{
    uint8_t   FOR_REFERENCE_ONLY;

    uint8_t   pad[0];            /* Pad to SHORT or LONG */
    uint8_t   parameters[0];     /* Param. bytes (# = parameterCount) */
    uint8_t   pad1[0];           /* Pad to SHORT or LONG */
    uint8_t   data[0];           /* Data bytes (# = DataCount) */
} TRANSACTION_SECONDARY_REQUEST_DATA_non_castable;

NTSTATUS
WireMarshallTransactionRequestData(
    uint8_t   *pBuffer,
    uint32_t   bufferLen,
    uint32_t  *pBufferUsed,
    uint16_t  *pSetup,
    uint8_t    setupLen,
    wchar16_t *pwszName,
    uint8_t   *pParameters,
    uint32_t   parameterLen,
    uint16_t  *pParameterOffset,
    uint8_t   *pData,
    uint32_t   dataLen,
    uint16_t  *pDataOffset
    )
{
    return WireMarshallTransactionSetupData(
                    pBuffer,
                    bufferLen,
                    pBufferUsed,
                    pSetup,
                    setupLen,
                    pwszName,
                    pParameters,
                    parameterLen,
                    pParameterOffset,
                    pData,
                    dataLen,
                    pDataOffset);
}

NTSTATUS
WireMarshallTransactionSecondaryRequestData(
    uint8_t  *pBuffer,
    uint32_t  bufferLen,
    uint32_t *pBufferUsed,
    uint8_t  *pParameters,
    uint32_t  parameterLen,
    uint16_t *pParameterOffset,
    uint8_t  *pData,
    uint32_t  dataLen,
    uint16_t *pDataOffset
    )
{
    return WireMarshallTransactionParameterData(
                    pBuffer,
                    bufferLen,
                    pBufferUsed,
                    pParameters,
                    parameterLen,
                    pParameterOffset,
                    pData,
                    dataLen,
                    pDataOffset);
}

/* This is identical to TRANSACTION_SECONDARY_REQUEST_DATA */
typedef struct
{
    uint8_t   FOR_REFERENCE_ONLY;

    uint16_t  setup[0];         /* Setup words (# = SetupWordCount) */
    uint16_t  byteCount;        /* Count of data bytes */
    uint8_t   pad[0];           /* Pad to SHORT or LONG */
    uint8_t   parameters[0];    /* Parameter bytes (# = ParameterCount) */
    uint8_t   pad1[0];          /* Pad to SHORT or LONG */
    uint8_t   data[0];          /* Data bytes (# = DataCount) */
} TRANSACTION_SECONDARY_RESPONSE_DATA_non_castable;

NTSTATUS
WireMarshallTransactionSecondaryResponseData(
    uint8_t  *pBuffer,
    uint32_t  bufferLen,
    uint32_t *pBufferUsed,
    uint16_t *pSetup,
    uint8_t   setupLen,
    uint8_t  *pParameters,
    uint32_t  parameterLen,
    uint16_t *pParameterOffset,
    uint8_t  *pData,
    uint32_t  dataLen,
    uint16_t *pDataOffset
    )
{
    return WireMarshallTransactionSetupData(
                    pBuffer,
                    bufferLen,
                    pBufferUsed,
                    pSetup,
                    setupLen,
                    NULL,
                    pParameters,
                    parameterLen,
                    pParameterOffset,
                    pData,
                    dataLen,
                    pDataOffset);
}

static
NTSTATUS
WireMarshallTransactionSetupData(
    PBYTE      pBuffer,
    uint32_t   bufferLen,
    uint32_t  *pBufferUsed,
    uint16_t  *pSetup,
    uint8_t    setupLen,
    wchar16_t *pwszName,
    uint8_t   *pParameters,
    uint32_t   parameterLen,
    uint16_t  *pParameterOffset,
    uint8_t   *pData,
    uint32_t   dataLen,
    uint16_t  *pDataOffset
    )
{
    NTSTATUS  ntStatus = 0;
    uint32_t  bufferUsed = 0;
    uint32_t  bufferUsedData = 0;
    uint32_t  alignment = 0;
    uint32_t  wstrlen = 0;
    uint8_t  *pByteCount = NULL;
    uint8_t   i = 0;
    PBYTE     pCursor = NULL;

    if (setupLen && bufferUsed + setupLen * sizeof(USHORT) <= bufferLen)
    {
        for (i = 0; i < setupLen; i++)
        {
            pCursor = pBuffer + i * sizeof(USHORT);
            MarshalUshort(&pCursor, NULL, pSetup[i]);
        }
    }
    bufferUsed += setupLen * sizeof(USHORT);

    /* byteCount */
    pByteCount = pBuffer + bufferUsed;
    bufferUsed += sizeof(uint16_t);

    if (pwszName)
    {
        /* Align string */
        alignment = (bufferUsed + 1) % 2;
        if (alignment)
        {
            *(pBuffer + bufferUsed) = 0;
            bufferUsed += alignment;
        }
        wstrlen = wc16oncpy((wchar16_t *) (pBuffer + bufferUsed), pwszName,
            bufferLen > bufferUsed ? bufferLen - bufferUsed : 0);
        bufferUsed += wstrlen * sizeof(uint16_t);
    }

    ntStatus = WireMarshallTransactionParameterData(
                    pBuffer + bufferUsed,
                    bufferLen > bufferUsed ? bufferLen - bufferUsed : 0,
                    &bufferUsedData,
                    pParameters,
                    parameterLen,
                    pParameterOffset,
                    pData,
                    dataLen,
                    pDataOffset);
    if (ntStatus && ntStatus != STATUS_INVALID_BUFFER_SIZE)
    {
        return ntStatus;
    }
    *pParameterOffset += bufferUsed;
    *pDataOffset += bufferUsed;
    bufferUsed += bufferUsedData;

    if (bufferUsed > bufferLen)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
    }
    else
    {
        /* Fill in the byte count */
        MarshalUshort(&pByteCount, NULL, (USHORT) (pBuffer + bufferUsed - pByteCount - sizeof(USHORT)));
    }

    *pBufferUsed = bufferUsed;

    return ntStatus;
}

static
NTSTATUS
WireMarshallTransactionParameterData(
    uint8_t  *pBuffer,
    uint32_t  bufferLen,
    uint32_t *pBufferUsed,
    uint8_t  *pParameters,
    uint32_t  parameterLen,
    uint16_t *pParameterOffset,
    uint8_t  *pData,
    uint32_t  dataLen,
    uint16_t *pDataOffset
    )
{
    uint32_t bufferUsed = 0;
    uint32_t alignment = 0;
    NTSTATUS ntStatus = 0;

    /* Align data to a four byte boundary */
    alignment = (4 - ((size_t) pBuffer % 4)) % 4;
    memset(pBuffer + bufferUsed, 0, alignment);
    bufferUsed += alignment;

    if (parameterLen && bufferUsed + parameterLen <= bufferLen)
        memcpy(pBuffer + bufferUsed, pParameters, parameterLen);
    *pParameterOffset = bufferUsed;
    bufferUsed += parameterLen;

    if (dataLen > 0)
    {
        /* Align data to a four byte boundary */
        alignment = (4 - (bufferUsed % 4)) % 4;
        memset(pBuffer + bufferUsed, 0, alignment);
        bufferUsed += alignment;

        if (dataLen && bufferUsed + dataLen <= bufferLen)
            memcpy(pBuffer + bufferUsed, pData, dataLen);
        *pDataOffset = bufferUsed;
        bufferUsed += dataLen;

        if (bufferUsed > bufferLen)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
        }
    }

    *pBufferUsed = bufferUsed;

    return ntStatus;
}

NTSTATUS
WireMarshallTransaction2Response(
    PBYTE       pBuffer,
    ULONG       ulNumBytesAvailable,
    ULONG       ulOffset,
    PUSHORT     pSetup,
    BYTE        setupCount,
    PBYTE       pParams,
    USHORT      usParamLength,
    PBYTE       pData,
    USHORT      usDataLen,
    PUSHORT     pusDataOffset,
    PUSHORT     pusParameterOffset,
    PUSHORT     pusNumPackageBytesUsed
    )
{
    NTSTATUS ntStatus = 0;
    PTRANSACTION_SECONDARY_RESPONSE_HEADER pResponseHeader = NULL;
    PBYTE    pByteCount = NULL;
    USHORT   usNumPackageBytesUsed = 0;
    USHORT   usNumBytesUsed = 0;
    PBYTE    pDataCursor = pBuffer;

    if (ulNumBytesAvailable < sizeof(TRANSACTION_SECONDARY_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PTRANSACTION_SECONDARY_RESPONSE_HEADER)pDataCursor;
    memset(pResponseHeader, 0, sizeof(*pResponseHeader));

    pDataCursor += sizeof(TRANSACTION_SECONDARY_RESPONSE_HEADER);
    ulOffset += sizeof(TRANSACTION_SECONDARY_RESPONSE_HEADER);
    ulNumBytesAvailable -= sizeof(TRANSACTION_SECONDARY_RESPONSE_HEADER);
    usNumPackageBytesUsed += sizeof(TRANSACTION_SECONDARY_RESPONSE_HEADER);

    pResponseHeader->setupCount = setupCount;

    if (setupCount)
    {
        USHORT usSetupLen = setupCount * sizeof(USHORT);

        if (ulNumBytesAvailable < usSetupLen)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pDataCursor, pSetup, usSetupLen);

        pDataCursor += usSetupLen;
        ulOffset += usSetupLen;
        ulNumBytesAvailable -= usSetupLen;
        usNumPackageBytesUsed += usSetupLen;
    }

    if (ulNumBytesAvailable < sizeof(USHORT))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pByteCount = pDataCursor;

    pDataCursor += sizeof(USHORT);
    ulNumBytesAvailable -= sizeof(USHORT);
    ulOffset += sizeof(USHORT);
    usNumPackageBytesUsed += sizeof(USHORT);

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
        usNumPackageBytesUsed += usAlignment;
    }

    pResponseHeader->parameterCount = usParamLength;
    pResponseHeader->totalParameterCount = usParamLength;
    pResponseHeader->parameterOffset = (USHORT)ulOffset;
    pResponseHeader->parameterDisplacement = 0;

    if (pParams)
    {
        if (ulNumBytesAvailable < usParamLength)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pDataCursor, pParams, usParamLength);

        pDataCursor += usParamLength;

        ulNumBytesAvailable -= usParamLength;
        ulOffset += usParamLength;
        usNumBytesUsed += usParamLength;
        usNumPackageBytesUsed += usParamLength;
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
        usNumPackageBytesUsed += usAlignment;
    }

    pResponseHeader->dataCount = usDataLen;
    pResponseHeader->totalDataCount = usDataLen;
    pResponseHeader->dataOffset = (USHORT)ulOffset;
    pResponseHeader->dataDisplacement = 0;

    if (pData)
    {
        if (ulNumBytesAvailable < usDataLen)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pDataCursor, pData, usDataLen);

        pDataCursor += usDataLen;
        ulNumBytesAvailable -= usDataLen;
        ulOffset += usDataLen;
        usNumBytesUsed += usDataLen;
        usNumPackageBytesUsed += usDataLen;
    }

    MarshalUshort(&pByteCount, NULL, usNumBytesUsed);

    *pusDataOffset = pResponseHeader->dataOffset;
    *pusParameterOffset = pResponseHeader->parameterOffset;
    *pusNumPackageBytesUsed = usNumPackageBytesUsed;

cleanup:

    return ntStatus;

error:

    *pusDataOffset = 0;
    *pusParameterOffset = 0;
    *pusNumPackageBytesUsed = 0;

    goto cleanup;
}

NTSTATUS
WireMarshalTrans2RequestSetup(
    IN OUT PSMB_HEADER               pSmbHeader,
    IN OUT PBYTE*                    ppCursor,
    IN OUT PULONG                    pulRemainingSpace,
    IN PUSHORT                       pusSetupWords,
    IN USHORT                        usSetupWordCount,
    OUT PTRANSACTION_REQUEST_HEADER* ppRequestHeader,
    OUT PBYTE*                       ppByteCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pCursor = *ppCursor;
    ULONG ulRemainingSpace = *pulRemainingSpace;
    PTRANSACTION_REQUEST_HEADER pRequestHeader = NULL;
    PBYTE pByteCount = NULL;
    USHORT usIndex = 0;

    /* Save reference to request header */
    pRequestHeader = (PTRANSACTION_REQUEST_HEADER) pCursor;

    /* Advance past request header */
    ntStatus = Advance(&pCursor, &ulRemainingSpace, sizeof(TRANSACTION_REQUEST_HEADER));
    BAIL_ON_NT_STATUS(ntStatus);

    /* Write setup words */
    for (usIndex = 0; usIndex < usSetupWordCount; usIndex++)
    {
        ntStatus = MarshalUshort(&pCursor, &ulRemainingSpace, pusSetupWords[usIndex]);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Save position of ByteCount field */
    pByteCount = pCursor;

    /* Advance over ByteCount field */
    ntStatus = Advance(&pCursor, &ulRemainingSpace, sizeof(USHORT));
    BAIL_ON_NT_STATUS(ntStatus);

    /* Align to LONG */
    ntStatus = Align((PBYTE) pSmbHeader, &pCursor, &ulRemainingSpace, sizeof(LONG));
    BAIL_ON_NT_STATUS(ntStatus);

    /* Write WordCount */
    pSmbHeader->wordCount = 14 + usSetupWordCount;

    *ppCursor = pCursor;
    *pulRemainingSpace = ulRemainingSpace;
    *ppRequestHeader = pRequestHeader;
    *ppByteCount = pByteCount;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;
    *ppByteCount = NULL;

    goto cleanup;
}

NTSTATUS
WireUnmarshalTrans2ReplySetup(
    IN PSMB_HEADER                                        pSmbHeader,
    IN OUT PBYTE*                                         ppCursor,
    IN OUT PULONG                                         pulRemainingSpace,
    OPTIONAL OUT PTRANSACTION_SECONDARY_RESPONSE_HEADER*  ppResponseHeader,
    OPTIONAL OUT PUSHORT                                  pusTotalParameterCount,
    OPTIONAL OUT PUSHORT                                  pusTotalDataCount,
    OPTIONAL OUT PUSHORT*                                 ppusSetupWords,
    OPTIONAL OUT PUSHORT                                  pusSetupWordCount,
    OPTIONAL OUT PUSHORT                                  pusByteCount,
    OPTIONAL OUT PBYTE*                                   ppParameterBlock,
    OPTIONAL OUT PUSHORT                                  pusParameterCount,
    OPTIONAL OUT PBYTE*                                   ppDataBlock,
    OPTIONAL OUT PUSHORT                                  pusDataCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pCursor = *ppCursor;
    ULONG ulRemainingSpace = *pulRemainingSpace;
    PTRANSACTION_SECONDARY_RESPONSE_HEADER pResponseHeader = NULL;
    PUSHORT pusSetupWords = NULL;
    USHORT usSetupWordCount = 0;
    USHORT usByteCount = 0;
    PBYTE pParameterBlock = NULL;
    PBYTE pDataBlock = NULL;
    USHORT usParameterCount = 0;
    USHORT usDataCount = 0;
    USHORT usParameterOffset = 0;
    USHORT usDataOffset = 0;
    USHORT usTotalParameterCount = 0;
    USHORT usTotalDataCount = 0;

    /* Save reference to response header */
    pResponseHeader = (PTRANSACTION_SECONDARY_RESPONSE_HEADER) pCursor;

    /* Advance past response header */
    ntStatus = Advance(&pCursor, &ulRemainingSpace, sizeof(TRANSACTION_SECONDARY_RESPONSE_HEADER));
    BAIL_ON_NT_STATUS(ntStatus);

    /* Unmarshal some important values from the header */
    usSetupWordCount  = SMB_LTOH8(pResponseHeader->setupCount);
    usParameterCount  = SMB_LTOH16(pResponseHeader->parameterCount);
    usDataCount       = SMB_LTOH16(pResponseHeader->dataCount);
    usParameterOffset = SMB_LTOH16(pResponseHeader->parameterOffset);
    usDataOffset      = SMB_LTOH16(pResponseHeader->dataOffset);

    if (usSetupWordCount)
    {
        /* Save reference to setup words */
        pusSetupWords = (PUSHORT) pCursor;

        /* Advance past setup words */
        ntStatus = Advance(&pCursor, &ulRemainingSpace, sizeof(USHORT) * usSetupWordCount);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Unmarshal ByteCount field */
    ntStatus = UnmarshalUshort(&pCursor, &ulRemainingSpace, &usByteCount);
    BAIL_ON_NT_STATUS(ntStatus);

    if (usParameterOffset && usParameterCount)
    {
        /* Ensure that parameter offset is at least SHORT aligned */
        if (usParameterOffset % sizeof(SHORT) != 0)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        /* Calculate parameter block pointer */
        pParameterBlock = (PBYTE) pSmbHeader + usParameterOffset;

        /* Verify that parameter block occurs after ByteCount */
        if (pParameterBlock < pCursor)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        /* Advance cursor to start of parameter block */
        ntStatus = AdvanceTo(&pCursor, &ulRemainingSpace, pParameterBlock);
        BAIL_ON_NT_STATUS(ntStatus);

        /* Advance cursor to end of parameter block */
        ntStatus = Advance(&pCursor, &ulRemainingSpace, usParameterCount);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (usDataOffset && usDataCount)
    {
        /* Enusre that data offset is at least SHORT aligned */
        if (usDataOffset % sizeof(SHORT) != 0)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        /* Calculate data block pointer */
        pDataBlock = (PBYTE) pSmbHeader + usDataOffset;

        /* Verify that data block occurs after parameter block */
        if (pDataBlock < pCursor)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        /* Advance cursor to start of data block */
        ntStatus = AdvanceTo(&pCursor, &ulRemainingSpace, pDataBlock);
        BAIL_ON_NT_STATUS(ntStatus);

        /* Advance cursor to end of data block */
        ntStatus = Advance(&pCursor, &ulRemainingSpace, usDataCount);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppCursor = pCursor;
    *pulRemainingSpace = ulRemainingSpace;

    if (ppResponseHeader)
    {
        *ppResponseHeader = pResponseHeader;
    }

    if (pusTotalParameterCount)
    {
        *pusTotalParameterCount = usTotalParameterCount;
    }

    if (pusTotalDataCount)
    {
        *pusTotalDataCount = usTotalDataCount;
    }

    if (ppusSetupWords)
    {
        *ppusSetupWords = pusSetupWords;
    }

    if (pusSetupWordCount)
    {
        *pusSetupWordCount = usSetupWordCount;
    }

    if (pusByteCount)
    {
        *pusByteCount = usByteCount;
    }

    if (ppParameterBlock)
    {
        *ppParameterBlock = pParameterBlock;
    }

    if (pusParameterCount)
    {
        *pusParameterCount = usParameterCount;
    }

    if (ppDataBlock)
    {
        *ppDataBlock = pDataBlock;
    }

    if (pusDataCount)
    {
        *pusDataCount = usDataCount;
    }

cleanup:

    return ntStatus;

error:

    if (ppResponseHeader)
    {
        *ppResponseHeader = NULL;
    }

    if (pusTotalParameterCount)
    {
        *pusTotalParameterCount = 0;
    }

    if (pusTotalDataCount)
    {
        *pusTotalDataCount = 0;
    }

    if (ppusSetupWords)
    {
        *ppusSetupWords = NULL;
    }

    if (pusSetupWordCount)
    {
        *pusSetupWordCount = 0;
    }

    if (pusByteCount)
    {
        *pusByteCount = 0;
    }

    if (ppParameterBlock)
    {
        *ppParameterBlock = NULL;
    }

    if (pusParameterCount)
    {
        *pusParameterCount = 0;
    }

    if (ppDataBlock)
    {
        *ppDataBlock = NULL;
    }

    if (pusDataCount)
    {
        *pusDataCount = 0;
    }

    goto cleanup;
}
