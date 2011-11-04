/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        crc32.c
 *
 * Abstract:
 *        CRC32-C generator
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *
 */
#include "ntlmsrvapi.h"
#include <krb5.h>

DWORD
NtlmCrc32(
    const SecBufferDesc* pMessage,
    PDWORD pdwCrc32
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    krb5_error_code KrbError= 0;
    DWORD dwChecksum = 0;
    size_t cKiov = 0;
    krb5_crypto_iov *kiov = NULL;
    DWORD dwIndex = 0;
    DWORD dwKiovPos = 0;
    // Do not free
    SecBuffer *pData = NULL;
    BYTE pbMagic[] = { 0x62, 0xF5, 0x26, 0x92 };

    for (dwIndex = 0 ; dwIndex < pMessage->cBuffers ; dwIndex++)
    {
        pData = &pMessage->pBuffers[dwIndex];

        if ((pData->BufferType & ~SECBUFFER_ATTRMASK) == SECBUFFER_DATA)
        {
            if (!pData->pvBuffer)
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }

            cKiov++;
        }
    }
    if (cKiov == 0)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Add 1 for the checksum buffer and 1 for the magic buffer
    cKiov += 2;

    dwError = LwAllocateMemory(
                  cKiov * sizeof(*kiov),
                  OUT_PPVOID(&kiov));
    BAIL_ON_LSA_ERROR(dwError);

    kiov[dwKiovPos].flags = KRB5_CRYPTO_TYPE_CHECKSUM;
    kiov[dwKiovPos].data.length = sizeof(dwChecksum);
    kiov[dwKiovPos].data.data = (void *)&dwChecksum;

    dwKiovPos++;

    // NTLM uses the "Preset to -1" and "Post-invert" variant of CRC32, where
    // the checksum is initialized to -1 before taking the input data into
    // account. After processing the input data, the one's complement of the
    // checksum is calculated. (see
    // http://en.wikipedia.org/wiki/Computation_of_CRC#Preset_to_.E2.88.921 )
    
    // Calculating this 4 byte magic sequence will set the current checksum
    // to -1.
    kiov[dwKiovPos].flags = KRB5_CRYPTO_TYPE_SIGN_ONLY;
    kiov[dwKiovPos].data.length = sizeof(pbMagic);
    kiov[dwKiovPos++].data.data = (void*)&pbMagic[0];

    for (dwIndex = 0 ; dwIndex < pMessage->cBuffers ; dwIndex++)
    {
        pData = &pMessage->pBuffers[dwIndex];

        if ((pData->BufferType & ~SECBUFFER_ATTRMASK) == SECBUFFER_DATA)
        {
            kiov[dwKiovPos].flags = KRB5_CRYPTO_TYPE_SIGN_ONLY;
            kiov[dwKiovPos].data.length = pData->cbBuffer;
            kiov[dwKiovPos++].data.data = pData->pvBuffer;
        }
    }

    KrbError = krb5_c_make_checksum_iov(
                   NULL,
                   CKSUMTYPE_CRC32,
                   NULL,
                   0,
                   kiov,
                   cKiov
                   );
    BAIL_ON_KRB_ERROR(NULL, KrbError);

    LW_ASSERT(kiov[0].data.length == 4);

    memcpy(&dwChecksum, kiov[0].data.data, kiov[0].data.length);
    // Do the post-invert
    *pdwCrc32 = dwChecksum ^ 0xFFFFFFFF;

cleanup:
    LW_SAFE_FREE_MEMORY(kiov);

    return dwError;

error:
    *pdwCrc32 = 0;
    goto cleanup;
}
