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
 *        tree_connect.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB TREE CONNECT "wire" API
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

    uint8_t   password[0];      /* Password */
    wchar16_t path[0];          /* Server name and share name */
    /* @todo: resolve inconsistency in SNIA spec which lists service as a
       Unicode string. */
    uchar8_t  service[0];       /* Service name */
} TREE_CONNECT_REQUEST_DATA_non_castable;

/* ASCII strings are not supported */
/* @todo: test alignment restrictions on Win2k */
NTSTATUS
MarshallTreeConnectRequestData(
    OUT PBYTE pBuffer,
    IN ULONG bufferLen,
    IN uint8_t messageAlignment,
    OUT PULONG pBufferUsed,
    IN PCWSTR pwszPath,
    IN PCSTR pszService
    )
{
    NTSTATUS ntStatus = 0;
    uint32_t bufferUsed = 0;
    uint32_t alignment = 0;

    /* The password field is obsolete in modern dialects */

    /* Align string */
    alignment = (bufferUsed + messageAlignment) % 2;
    if (alignment)
    {
        *(pBuffer + bufferUsed) = 0;
        bufferUsed += alignment;
    }

    ntStatus = SMBPacketAppendUnicodeString(pBuffer, bufferLen, &bufferUsed, pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketAppendString(pBuffer, bufferLen, &bufferUsed, pszService);
    BAIL_ON_NT_STATUS(ntStatus);

error:
    *pBufferUsed = bufferUsed;

    return ntStatus;
}

NTSTATUS
UnmarshallTreeConnectRequest(
    const PBYTE pParams,
    ULONG       ulBytesAvailable,
    ULONG       ulOffset,
    PTREE_CONNECT_REQUEST_HEADER* ppHeader,
    PBYTE*      ppPassword,
    PWSTR*      ppwszPath,
    PBYTE*      ppszService
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pDataCursor = pParams;
    PTREE_CONNECT_REQUEST_HEADER pHeader = NULL;
    PBYTE  pPassword = NULL;
    PWSTR  pwszPath = NULL;
    PWSTR  pwszCursor = NULL;
    PBYTE  pszService = NULL;
    PBYTE  pszServiceCursor = NULL;
    USHORT usAlignment = 0;

    if (ulBytesAvailable < sizeof(TREE_CONNECT_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PTREE_CONNECT_REQUEST_HEADER)pDataCursor;

    pDataCursor += sizeof(TREE_CONNECT_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(TREE_CONNECT_REQUEST_HEADER);
    ulOffset += sizeof(TREE_CONNECT_REQUEST_HEADER);

    if (pHeader->passwordLength)
    {
        if (ulBytesAvailable < pHeader->passwordLength)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pPassword = pDataCursor;

        pDataCursor += pHeader->passwordLength;
        ulBytesAvailable -= pHeader->passwordLength;
        ulOffset += pHeader->passwordLength;
    }

    usAlignment = ulOffset % 2;

    if (ulBytesAvailable < usAlignment)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulBytesAvailable -= usAlignment;
    pDataCursor += usAlignment;
    ulOffset += usAlignment;

    do
    {
        if (ulBytesAvailable < sizeof(wchar16_t))
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (!pwszPath)
        {
            pwszPath = pwszCursor = (PWSTR)pDataCursor;
        }
        else
        {
            pwszCursor++;
        }

        ulBytesAvailable -= sizeof(wchar16_t);
        pDataCursor += sizeof(wchar16_t);
        ulOffset += sizeof(wchar16_t);

    } while ((ulBytesAvailable > 0) && pwszCursor && *pwszCursor);

    if (!pwszCursor || *pwszCursor)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    do
    {
        if (ulBytesAvailable < sizeof(BYTE))
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (!pszService)
        {
            pszService = pszServiceCursor = pDataCursor;
        }
        else
        {
            pszServiceCursor++;
        }

        ulBytesAvailable -= sizeof(BYTE);
        pDataCursor += sizeof(BYTE);
        ulOffset += sizeof(BYTE);

    } while ((ulBytesAvailable > 0) && pszServiceCursor && *pszServiceCursor);

    if (!pszServiceCursor || *pszServiceCursor)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppHeader = pHeader;
    *ppPassword = pPassword;
    *ppwszPath = pwszPath;
    *ppszService = pszService;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppPassword = NULL;
    *ppwszPath = NULL;
    *ppszService = NULL;

    goto cleanup;
}

typedef struct
{
    uint8_t   FOR_REFERENCE_ONLY;

    uchar8_t  service[0];      /* Service type connected (always ASCII) */
    wchar16_t nativeFileSystem[0];  /* Native file system for this tree*/
} TREE_CONNECT_RESPONSE_DATA_non_castable;

NTSTATUS
MarshallTreeConnectResponseData(
    uint8_t         *pBuffer,
    uint32_t         bufferAvailable,
    uint32_t         bufferUsed,
    uint16_t        *pBufferUsed,
    const uchar8_t  *pszService,
    const wchar16_t *pwszNativeFileSystem
    )
{
    NTSTATUS ntStatus = 0;
    uint8_t* pData = pBuffer;
    uint16_t dataBufferUsed = 0;
    uint32_t alignment = 0;
    uint32_t wstrlen = 0;
    int iCh = 0;
    wchar16_t wszEmpty = WNUL;

    while (pszService && *pszService)
    {
        if (!bufferAvailable)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        *pData++ = *pszService++;
        dataBufferUsed++;
        bufferAvailable--;
    }

    if (!bufferAvailable)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pData++ = NUL;
    dataBufferUsed++;
    bufferAvailable--;

    /* Align string */
    alignment = (bufferUsed + dataBufferUsed) % 2;
    for (; iCh < alignment; iCh++)
    {
        if (!bufferAvailable)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        *pData++ = 0;
        dataBufferUsed++;
        bufferAvailable--;
    }

    wstrlen = (pwszNativeFileSystem ? (wc16slen(pwszNativeFileSystem) + 1) : 1);
    if (bufferAvailable < (wstrlen * sizeof(wchar16_t)))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    wstrlen = wc16oncpy(
                    (wchar16_t *)pData,
                    (pwszNativeFileSystem ? pwszNativeFileSystem : &wszEmpty),
                    wstrlen);

    dataBufferUsed += wstrlen * sizeof(wchar16_t);
    // bufferAvailable -= wstrlen * sizeof(wchar16_t);

    *pBufferUsed = dataBufferUsed;

error:

    return ntStatus;
}

NTSTATUS
UnmarshallTreeConnectExtResponse(
    const uint8_t    *pBuffer,
    uint32_t          bufferLen,
    uint8_t           messageAlignment,
    TREE_CONNECT_EXT_RESPONSE_HEADER **ppHeader
    )
{
    PTREE_CONNECT_EXT_RESPONSE_HEADER pHeader = NULL;
    /* NOTE: The buffer format cannot be trusted! */
    uint32_t bufferUsed = sizeof(TREE_CONNECT_RESPONSE_HEADER);
    if (bufferLen < bufferUsed)
        return STATUS_INVALID_NETWORK_RESPONSE;

    pHeader = (PTREE_CONNECT_EXT_RESPONSE_HEADER) pBuffer;
    SMB_HTOL16_INPLACE(pHeader->optionalSupport);
    SMB_HTOL32_INPLACE(pHeader->maximalShareAccessMask);
    SMB_HTOL32_INPLACE(pHeader->guestMaximalShareAccessMask);
    *ppHeader = pHeader;

    return 0;
}

