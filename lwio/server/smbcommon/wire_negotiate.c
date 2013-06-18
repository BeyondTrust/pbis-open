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
 *        negotiate.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB NEGOTIATE "wire" API
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: support big endian architectures
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "includes.h"

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */

    uint8_t bufferFormat;       /* 0x02 -- Dialect */
    uchar8_t szDialectName[];    /* ASCII null-terminated string */
}  __attribute__((__packed__))  NEGOTIATE_REQUEST_DIALECT;

/**
 * Marshall an SMB NEGOTIATE request
 *
 * @param pszDialects
 *                An array of zero terminated ASCII dialect strings
 * @param dialectCount
 *                Count of dialect strings
 * @param pBuffer Marshall buffer
 * @param bufferLen
 *                Size of marshall buffer
 * @param pBufferUsed
 *                On success, the amount of the marshall buffer used.  On
 *                STATUS_INVALID_BUFFER_SIZE, the total amount of marshall buffer needed
 *
 * @return Non-zero error code on error
 */
NTSTATUS
MarshallNegotiateRequest(
    uint8_t        *pBuffer,
    uint32_t        bufferLen,
    uint32_t       *pBufferUsed,
    const uchar8_t *pszDialects[],
    uint32_t        dialectCount
    )
{
    NTSTATUS ntStatus = 0;

    NEGOTIATE_REQUEST_DIALECT *pDialect = (NEGOTIATE_REQUEST_DIALECT*) pBuffer;
    uint32_t bufferUsed = 0;

    uint32_t i = 0;

    /* Input strings are trusted */
    for (i = 0; i < dialectCount; i++)
    {
        uint32_t len = sizeof(NEGOTIATE_REQUEST_DIALECT);

        if (bufferUsed + len <= bufferLen)
            // No endianness (single byte):
            pDialect->bufferFormat = 0x02;
        bufferUsed += len;

        if (bufferUsed + sizeof(NUL) <= bufferLen)
        {
            uint8_t *pCursor =
                (uint8_t*) lsmb_stpncpy((char*) pDialect->szDialectName,
                    (const char *) pszDialects[i], bufferLen - bufferUsed);
            if (!*pCursor)
            {
                /* string fits */
                pCursor += sizeof(NUL);
                len = pCursor - pBuffer - bufferUsed;
                pDialect = (NEGOTIATE_REQUEST_DIALECT*) pCursor;
            }
            else
            {
                /* expensive length check */
                len = strlen((const char*) pszDialects[i]) + sizeof(NUL);
            }
        }
        else
        {
            /* expensive length check */
            len = strlen((const char *) pszDialects[i]) + sizeof(NUL);
        }

        bufferUsed += len;
    }

    if (bufferUsed > bufferLen)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
    }

    *pBufferUsed = bufferUsed;

    return ntStatus;
}

NTSTATUS
UnmarshallNegotiateRequest(
    const uint8_t   *pBuffer,
    uint32_t         bufferLen,        /* From caller's byteCount */
    uchar8_t        *pszDialects[],
    uint32_t        *pDialectCount
    )
{
    NTSTATUS ntStatus = 0;
    NEGOTIATE_REQUEST_HEADER* pHeader = NULL;
    uint32_t bufferLeft = bufferLen;

    if (bufferLeft < sizeof(NEGOTIATE_REQUEST_HEADER))
    {
        return EBADMSG;
    }

    pHeader = (NEGOTIATE_REQUEST_HEADER*)pBuffer;

    /* NOTE: The buffer format cannot be trusted! */
    NEGOTIATE_REQUEST_DIALECT *pDialect = (NEGOTIATE_REQUEST_DIALECT*) (pBuffer + sizeof(NEGOTIATE_REQUEST_HEADER));

    uint32_t i = 0;

    while ((uint8_t *) pDialect < (pBuffer + sizeof(NEGOTIATE_REQUEST_HEADER) + pHeader->byteCount))
    {
        uint32_t len = strnlen((const char *) pDialect->szDialectName,
            bufferLeft) + sizeof(NEGOTIATE_REQUEST_DIALECT) + sizeof(NUL);

        /* If the last string was (sneakily) not null terminated, bail! */
        if (len > bufferLeft)
            return EBADMSG;

        if (i < *pDialectCount)
            pszDialects[i] = (uchar8_t *) pDialect->szDialectName;

        pDialect = (NEGOTIATE_REQUEST_DIALECT*) ((uint8_t *) pDialect + len);
        bufferLeft -= len;
        i++;
    }

    if (i > *pDialectCount)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
    }

    *pDialectCount = i;

    return ntStatus;
}

typedef struct
{
    /* byteCount is handled at a higher layer */

    /* Non-CAP_EXTENDED_SECURITY fields are not implemented */
    /* @todo: decide if CAP_EXTENDED_SECURITY will be mandatory */

    uint8_t guid[16];           /* A globally unique identifier assigned to the
                                 * server; Present only when
                                 * CAP_EXTENDED_SECURITY is on in Capabilities
                                 * field */
    uint8_t securityBlob[];      /* Opaque Security Blob associated with the
                                 * security package if CAP_EXTENDED_SECURITY
                                 * is on in the Capabilities field; else
                                 * challenge for CIFS challenge/response
                                 * authentication */
}  __attribute__((__packed__))  NEGOTIATE_RESPONSE_DATA;

NTSTATUS
MarshallNegotiateResponseData(
    uint8_t  *pBuffer,
    uint32_t  bufferLen,
    uint32_t *pBufferUsed,
    const uint8_t *pGUID,
    const uint8_t *pSecurityBlob,
    uint32_t  blobLen
    )
{
    NTSTATUS ntStatus = 0;

    NEGOTIATE_RESPONSE_DATA *pData = (NEGOTIATE_RESPONSE_DATA*) pBuffer;
    uint32_t bufferUsed = 0;
    uint32_t len = sizeof(pData->guid) + blobLen;

    if (bufferUsed + len <= bufferLen)
    {
        memcpy(&(pData->guid), pGUID, sizeof(pData->guid));
        if (blobLen)
        {
            memcpy(&(pData->securityBlob), pSecurityBlob, blobLen);
        }
    }
    bufferUsed += len;

    if (bufferUsed > bufferLen)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
    }

    *pBufferUsed = bufferUsed;

    return ntStatus;
}

NTSTATUS
UnmarshallNegotiateResponse(
    const uint8_t  *pBuffer,
    uint32_t        bufferLen,
    NEGOTIATE_RESPONSE_HEADER **ppHeader,
    uint8_t       **ppGUID,
    uint8_t       **ppSecurityBlob,
    uint32_t       *pBlobLen
    )
{
    NEGOTIATE_RESPONSE_DATA *pData = NULL;

    /* NOTE: The buffer format cannot be trusted! */
    uint32_t bufferUsed = sizeof(NEGOTIATE_RESPONSE_HEADER);
    if (bufferLen < bufferUsed)
        return STATUS_INVALID_NETWORK_RESPONSE;

    /* @todo: endian swap as appropriate */
    *ppHeader = (NEGOTIATE_RESPONSE_HEADER*) pBuffer;
    pData = (NEGOTIATE_RESPONSE_DATA*) (pBuffer + bufferUsed);
    bufferUsed += sizeof(pData->guid);

    if (bufferLen < bufferUsed)
        return STATUS_INVALID_NETWORK_RESPONSE;

    *pBlobLen = bufferLen - bufferUsed;

    if (*pBlobLen == 0)           /* zero length blob */
        *ppSecurityBlob = NULL;
    else
        *ppSecurityBlob = (uint8_t *) &(pData->securityBlob);

    return 0;
}

