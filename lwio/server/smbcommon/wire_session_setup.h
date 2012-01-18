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


NTSTATUS
UnmarshallSessionSetupRequest_WC_12(
    const uint8_t   *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    SESSION_SETUP_REQUEST_HEADER_WC_12 **ppHeader,
    uint8_t        **ppSecurityBlob,
    wchar16_t      **ppwszNativeOS,
    wchar16_t      **ppwszNativeLanMan,
    wchar16_t      **ppwszNativeDomain
    );

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
    );

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t  maxBufferSize;   /* Client's maximum buffer size */
    uint16_t  maxMpxCount;     /* Actual maximum multiplexed pending
                                * requests */
    uint16_t  vcNumber;        /* 0 = first (only), nonzero=additional VC
                                * number */
    uint32_t  sessionKey;      /* Session key (valid iff VcNumber != 0) */
    uint16_t  ciPasswordLen;   /* Case insensitive password length, ASCII */
    uint16_t  csPasswordLen;   /* Case sensitive password length, Unicode */
    uint32_t  reserved;        /* Must be 0 */
    uint32_t  capabilities;    /* Client capabilities */
    uint16_t  byteCount;       /* Count of data bytes; min = 0 */

    /* Data immediately follows */
}  __attribute__((__packed__))  SESSION_SETUP_REQUEST_HEADER_NO_EXT;

