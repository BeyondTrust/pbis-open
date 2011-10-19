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
 *        session_setup.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB SESSION SETUP "wire" API
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

    uint8_t   securityBlob[0];  /* The opaque security blob */
    wchar16_t nativeOS[0];      /* Client's native operating system, Unicode */
    wchar16_t nativeLanMan[0];  /* Client's native LAN Manager type, Unicode */
    wchar16_t nativeDomain[0];  /* Client's native domain, Unicode */
} SESSION_SETUP_REQUEST_DATA_non_castable;

/* ASCII is not supported */
/* @todo: test alignment restrictions on Win2k */
static
NTSTATUS
_MarshallSessionSetupData(
    OUT PBYTE pBuffer,
    IN ULONG bufferLen,
    IN uint8_t messageAlignment,
    OUT PULONG pBufferUsed,
    IN const BYTE* pSecurityBlob,
    IN USHORT blobLen,
    IN PCWSTR pwszNativeOS,
    IN PCWSTR pwszNativeLanMan,
    IN PCWSTR pwszNativeDomain
    )
{
    NTSTATUS ntStatus = 0;
    uint32_t bufferUsed = 0;
    uint32_t alignment = 0;

    if (blobLen && bufferUsed + blobLen <= bufferLen)
        memcpy(pBuffer, pSecurityBlob, blobLen);
    bufferUsed += blobLen;

    /* Align strings */
    alignment = (bufferUsed + messageAlignment) % 2;
    if (alignment)
    {
        *(pBuffer + bufferUsed) = 0;
        bufferUsed += alignment;
    }

    ntStatus = SMBPacketAppendUnicodeString(pBuffer, bufferLen, &bufferUsed, pwszNativeOS);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketAppendUnicodeString(pBuffer, bufferLen, &bufferUsed, pwszNativeLanMan);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pwszNativeDomain)   /* NULL when extended security is not used */
    {
        ntStatus = SMBPacketAppendUnicodeString(pBuffer, bufferLen, &bufferUsed, pwszNativeDomain);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:
    *pBufferUsed = bufferUsed;

    return ntStatus;
}

NTSTATUS
MarshallSessionSetupRequestData(
    uint8_t         *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    uint32_t        *pBufferUsed,
    const uint8_t   *pSecurityBlob,
    uint16_t         blobLen,
    const wchar16_t *pwszNativeOS,
    const wchar16_t *pwszNativeLanMan,
    const wchar16_t *pwszNativeDomain
    )
{
    return _MarshallSessionSetupData(pBuffer, bufferLen, messageAlignment,
        pBufferUsed, pSecurityBlob, blobLen, pwszNativeOS, pwszNativeLanMan,
        pwszNativeDomain);
}

/* ASCII is not supported */
static NTSTATUS
_UnmarshallSessionSetupData(
    const uint8_t *pBuffer,
    uint32_t       bufferLen,
    uint8_t        messageAlignment,
    uint8_t      **ppSecurityBlob,
    uint16_t       blobLen,
    wchar16_t    **ppwszNativeOS,
    wchar16_t    **ppwszNativeLanMan,
    wchar16_t    **ppwszNativeDomain
    )
{
    uint32_t bufferUsed = 0;

    if (blobLen > bufferLen)
        return EBADMSG;

    if (blobLen == 0)
    {
        *ppSecurityBlob = NULL;     /* Zero length blob */
    }
    else
    {
        *ppSecurityBlob = (uint8_t*) pBuffer;
    }
    bufferUsed += blobLen;

    /* Align strings */
    bufferUsed += (bufferUsed + messageAlignment) % 2;
    if (bufferUsed > bufferLen)
    {
        return EBADMSG;
    }

    // TODO -- change function to copy strings so we handle alignment.
#if 1
    *ppwszNativeOS = NULL;
    *ppwszNativeLanMan = NULL;
    *ppwszNativeDomain = NULL;
#else
    *ppwszNativeOS = (wchar16_t *) (pBuffer + bufferUsed);
    bufferUsed += sizeof(wchar16_t) * wc16snlen(*ppwszNativeOS,
        (bufferLen - bufferUsed) / sizeof(wchar16_t)) + sizeof(WNUL);
    if (bufferUsed > bufferLen)
    {
        return EBADMSG;
    }

    *ppwszNativeLanMan = (wchar16_t *) (pBuffer + bufferUsed);
    bufferUsed += sizeof(wchar16_t) * wc16snlen(*ppwszNativeLanMan,
        (bufferLen - bufferUsed) / sizeof(wchar16_t)) + sizeof(WNUL);
    if (bufferUsed > bufferLen)
    {
        return EBADMSG;
    }

    *ppwszNativeDomain = (wchar16_t *) (pBuffer + bufferUsed);
    bufferUsed += sizeof(wchar16_t) * wc16snlen(*ppwszNativeDomain,
        (bufferLen - bufferUsed) / sizeof(wchar16_t)) + sizeof(WNUL);
    if (bufferUsed > bufferLen)
    {
        return EBADMSG;
    }
#endif

    return 0;
}

NTSTATUS
UnmarshallSessionSetupRequest(
    const uint8_t *pBuffer,
    uint32_t       bufferLen,
    uint8_t        messageAlignment,
    SESSION_SETUP_REQUEST_HEADER **ppHeader,
    uint8_t      **ppSecurityBlob,
    wchar16_t    **ppwszNativeOS,
    wchar16_t    **ppwszNativeLanMan,
    wchar16_t    **ppwszNativeDomain
    )
{
    /* NOTE: The buffer format cannot be trusted! */
    uint32_t bufferUsed = sizeof(SESSION_SETUP_REQUEST_HEADER);
    if (bufferLen < bufferUsed)
        return EBADMSG;

    /* @todo: endian swap as appropriate */
    *ppHeader = (SESSION_SETUP_REQUEST_HEADER*) pBuffer;

    return _UnmarshallSessionSetupData(pBuffer + bufferUsed,
        bufferLen - bufferUsed, messageAlignment, ppSecurityBlob,
        (*ppHeader)->securityBlobLength, ppwszNativeOS, ppwszNativeLanMan,
        ppwszNativeDomain);
}

typedef struct
{
    uint8_t   FOR_REFERENCE_ONLY;

    uint8_t   securityBlob[0];  /* SecurityBlob of length specified by the
                                 * field, SecurityBlobLength */
    wchar16_t nativeOS[0];      /* Server's native operating system */
    wchar16_t nativeLanMan[0];  /* Server's native LAN Manager type */
    wchar16_t nativeDomain[0];  /* Server's primary domain */
} SESSION_SETUP_RESPONSE_DATA_non_castable;

NTSTATUS
MarshallSessionSetupResponseData(
    uint8_t         *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    uint32_t        *pBufferUsed,
    const uint8_t   *pSecurityBlob,
    uint16_t         blobLen,
    const wchar16_t *pwszNativeOS,
    const wchar16_t *pwszNativeLanMan,
    const wchar16_t *pwszNativeDomain
    )
{
    return _MarshallSessionSetupData(pBuffer, bufferLen, messageAlignment,
        pBufferUsed, pSecurityBlob, blobLen, pwszNativeOS, pwszNativeLanMan,
        pwszNativeDomain);
}

NTSTATUS
UnmarshallSessionSetupResponse(
    const uint8_t    *pBuffer,
    uint32_t          bufferLen,
    uint8_t           messageAlignment,
    SESSION_SETUP_RESPONSE_HEADER **ppHeader,
    uint8_t         **ppSecurityBlob,
    wchar16_t       **ppwszNativeOS,
    wchar16_t       **ppwszNativeLanMan,
    wchar16_t       **ppwszNativeDomain
    )
{
    NTSTATUS ntStatus = 0;
    PSESSION_SETUP_RESPONSE_HEADER pHeader = (PSESSION_SETUP_RESPONSE_HEADER) pBuffer;
    ULONG bufferUsed = sizeof(SESSION_SETUP_RESPONSE_HEADER);

    /* NOTE: The buffer format cannot be trusted! */
    if (bufferLen < bufferUsed)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // byte order conversions
    SMB_LTOH16_INPLACE(pHeader->action);
    SMB_LTOH16_INPLACE(pHeader->securityBlobLength);
    SMB_LTOH16_INPLACE(pHeader->byteCount);

    ntStatus = _UnmarshallSessionSetupData(pBuffer + bufferUsed,
        bufferLen - bufferUsed, messageAlignment, ppSecurityBlob,
        pHeader->securityBlobLength, ppwszNativeOS, ppwszNativeLanMan,
        ppwszNativeDomain);

error:
    if (ntStatus)
    {
        pHeader = NULL;
    }

    *ppHeader = pHeader;

    return ntStatus;
}
