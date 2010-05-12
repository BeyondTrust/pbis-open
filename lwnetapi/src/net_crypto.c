/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2010
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
 *        net_crypto.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetAPI cryptographic functions.
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
DWORD
NetPrepareDesKey(
    IN  PBYTE  pInput,
    OUT PBYTE  pOutput
    );


DWORD
NetGetNtPasswordHash(
    IN  PCWSTR  pwszPassword,
    OUT PBYTE   pNtHash,
    IN  DWORD   dwNtHashSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    size_t sPasswordLen = 0;
    BYTE Hash[16] = {0};

    BAIL_ON_INVALID_PTR(pwszPassword, dwError);
    BAIL_ON_INVALID_PTR(pNtHash, dwError);

    if (dwNtHashSize < sizeof(Hash))
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_WIN_ERROR(dwError);

    MD4((PBYTE)pwszPassword,
        sPasswordLen * sizeof(pwszPassword[0]),
        Hash);

    memcpy(pNtHash, Hash, sizeof(Hash));

cleanup:
    memset(Hash, 0, sizeof(Hash));

    return dwError;

error:
    memset(pNtHash, 0, dwNtHashSize);

    goto cleanup;
}


DWORD
NetEncodePasswordBuffer(
    IN  PCWSTR  pwszPassword,
    OUT PBYTE   pBlob,
    IN  DWORD   dwBlobSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    size_t sPasswordLen = 0;
    size_t sPasswordSize = 0;
    PWSTR pwszPasswordLE = NULL;
    BYTE PasswordBlob[516] = {0};
    BYTE BlobInit[512] = {0};
    DWORD iByte = 0;

    BAIL_ON_INVALID_PTR(pwszPassword, dwError);
    BAIL_ON_INVALID_PTR(pBlob, dwError);

    if (dwBlobSize < sizeof(PasswordBlob))
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_WIN_ERROR(dwError);

    /* size doesn't include terminating zero here */
    sPasswordSize = sPasswordLen * sizeof(pwszPassword[0]);

    /*
     * Make sure encoded password is 2-byte little-endian
     */
    dwError = LwAllocateMemory(sPasswordSize + sizeof(pwszPassword[0]),
                               OUT_PPVOID(&pwszPasswordLE));
    BAIL_ON_WIN_ERROR(dwError);

    wc16stowc16les(pwszPasswordLE, pwszPassword, sPasswordLen);

    /*
     * Encode the password length (in bytes) - the last 4 bytes
     */
    iByte = sizeof(PasswordBlob);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 24) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 16) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 8) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize) & 0xff);

    /*
     * Copy the password and the initial random bytes
     */
    iByte -= sPasswordSize;
    memcpy(&(PasswordBlob[iByte]), pwszPasswordLE, sPasswordSize);

    /*
     * Fill the rest of the buffer with (pseudo) random mess
     * to increase security.
     */
    if (!RAND_bytes((unsigned char*)BlobInit, iByte))
    {
        dwError = ERROR_ENCRYPTION_FAILED;
        BAIL_ON_WIN_ERROR(dwError);
    }
    memcpy(PasswordBlob, BlobInit, iByte);

    memcpy(pBlob, PasswordBlob, sizeof(PasswordBlob));

cleanup:
    memset(PasswordBlob, 0, sizeof(PasswordBlob));

    if (pwszPasswordLE)
    {
        memset(pwszPasswordLE, 0, sPasswordSize);
        LW_SAFE_FREE_MEMORY(pwszPasswordLE);
    }

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    if (pBlob)
    {
        memset(pBlob, 0, dwBlobSize);
    }

    goto cleanup;
}


DWORD
NetEncryptNtHashVerifier(
    IN  PBYTE    pNewNtHash,
    IN  DWORD    dwNewNtHashLen,
    IN  PBYTE    pOldNtHash,
    IN  DWORD    dwOldNtHashLen,
    OUT PBYTE    pNtVerifier,
    IN  DWORD    dwNtVerifierSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DES_cblock KeyBlockLo;
    DES_cblock KeyBlockHi;
    DES_key_schedule KeyLo;
    DES_key_schedule KeyHi;
    BYTE Verifier[16] = {0};

    BAIL_ON_INVALID_PTR(pNewNtHash, dwError);
    BAIL_ON_INVALID_PTR(pOldNtHash, dwError);
    BAIL_ON_INVALID_PTR(pNtVerifier, dwError);

    if (dwNtVerifierSize < sizeof(Verifier))
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    memset(&KeyBlockLo, 0, sizeof(KeyBlockLo));
    memset(&KeyBlockHi, 0, sizeof(KeyBlockHi));
    memset(&KeyLo, 0, sizeof(KeyLo));
    memset(&KeyHi, 0, sizeof(KeyHi));

    dwError = NetPrepareDesKey(&pNewNtHash[0],
			       (PBYTE)KeyBlockLo);
    BAIL_ON_WIN_ERROR(dwError);

    DES_set_odd_parity(&KeyBlockLo);
    DES_set_key_unchecked(&KeyBlockLo, &KeyLo);

    dwError = NetPrepareDesKey(&pNewNtHash[7],
			       (PBYTE)KeyBlockHi);
    BAIL_ON_WIN_ERROR(dwError);

    DES_set_odd_parity(&KeyBlockHi);
    DES_set_key_unchecked(&KeyBlockHi, &KeyHi);

    DES_ecb_encrypt((DES_cblock*)&pOldNtHash[0],
                    (DES_cblock*)&Verifier[0],
                    &KeyLo,
                    DES_ENCRYPT);
    DES_ecb_encrypt((DES_cblock*)&pOldNtHash[8],
                    (DES_cblock*)&Verifier[8],
                    &KeyHi,
                    DES_ENCRYPT);

    memcpy(pNtVerifier, Verifier, sizeof(Verifier));

cleanup:
    memset(&KeyBlockLo, 0, sizeof(KeyBlockLo));
    memset(&KeyBlockHi, 0, sizeof(KeyBlockHi));
    memset(&KeyLo, 0, sizeof(KeyLo));
    memset(&KeyHi, 0, sizeof(KeyHi));

    return dwError;

error:
    goto cleanup;
}


static
DWORD
NetPrepareDesKey(
    IN  PBYTE  pInput,
    OUT PBYTE  pOutput
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD i = 0;

    BAIL_ON_INVALID_PTR(pInput, dwError);
    BAIL_ON_INVALID_PTR(pOutput, dwError);

    /*
     * Expand the input 7x8 bits so that each 7 bits are
     * appended with 1 bit space for parity bit and yield
     * 8x8 bits ready to become a DES key
     */
    pOutput[0] = pInput[0] >> 1;
    pOutput[1] = ((pInput[0]&0x01) << 6) | (pInput[1] >> 2);
    pOutput[2] = ((pInput[1]&0x03) << 5) | (pInput[2] >> 3);
    pOutput[3] = ((pInput[2]&0x07) << 4) | (pInput[3] >> 4);
    pOutput[4] = ((pInput[3]&0x0F) << 3) | (pInput[4] >> 5);
    pOutput[5] = ((pInput[4]&0x1F) << 2) | (pInput[5] >> 6);
    pOutput[6] = ((pInput[5]&0x3F) << 1) | (pInput[6] >> 7);
    pOutput[7] = pInput[6]&0x7F;

    for (i = 0; i < 8; i++)
    {
        pOutput[i] = pOutput[i] << 1;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
NetEncodeJoinPasswordBuffer(
    IN  PCWSTR  pwszPassword,
    OUT PBYTE   pBlob,
    IN  DWORD   dwBlobSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    size_t sPasswordLen = 0;
    size_t sPasswordSize = 0;
    PWSTR pwszPasswordLE = NULL;
    BYTE PasswordBlob[516] = {0};
    BYTE BlobInit[512] = {0};
    DWORD iByte = 0;

    BAIL_ON_INVALID_PTR(pwszPassword, dwError);
    BAIL_ON_INVALID_PTR(pBlob, dwError);

    if (dwBlobSize < sizeof(PasswordBlob))
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_WIN_ERROR(dwError);

    /* size doesn't include terminating zero here */
    sPasswordSize = sPasswordLen * sizeof(pwszPassword[0]);

    /*
     * Make sure encoded password is 2-byte little-endian
     */
    dwError = LwAllocateMemory(sPasswordSize + sizeof(pwszPassword[0]),
                               OUT_PPVOID(&pwszPasswordLE));
    BAIL_ON_WIN_ERROR(dwError);

    wc16stowc16les(pwszPasswordLE, pwszPassword, sPasswordLen);

    /*
     * Encode the password length (in bytes) - the last 4 bytes
     */
    iByte = sizeof(PasswordBlob);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 24) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 16) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 8) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize) & 0xff);

    /*
     * Copy the password and the initial random bytes
     */
    iByte -= sPasswordSize;
    memcpy(&(PasswordBlob[iByte]), pwszPasswordLE, sPasswordSize);

    /*
     * Fill the rest of the buffer with (pseudo) random mess
     * to increase security.
     */
    if (!RAND_bytes((unsigned char*)BlobInit, iByte))
    {
        dwError = ERROR_ENCRYPTION_FAILED;
        BAIL_ON_WIN_ERROR(dwError);
    }
    memcpy(PasswordBlob, BlobInit, iByte);

    memcpy(pBlob, PasswordBlob, sizeof(PasswordBlob));

cleanup:
    memset(PasswordBlob, 0, sizeof(PasswordBlob));

    if (pwszPasswordLE)
    {
        memset(pwszPasswordLE, 0, sPasswordSize);
        LW_SAFE_FREE_MEMORY(pwszPasswordLE);
    }

    return dwError;

error:
    if (pBlob)
    {
        memset(pBlob, 0, dwBlobSize);
    }

    goto cleanup;
}


DWORD
NetEncryptJoinPasswordBuffer(
    IN  PNET_CONN                 pConn,
    IN  PCWSTR                    pwszPassword,
    OUT PENC_JOIN_PASSWORD_BUFFER pPasswordBuffer
    )
{
    DWORD dwError = ERROR_SUCCESS;
    BYTE PasswordBlob[516] = {0};
    BYTE KeyInit[8] = {0};
    MD5_CTX ctx;
    BYTE DigestedSessionKey[16] = {0};
    RC4_KEY key;
    DWORD iByte = 0;

    BAIL_ON_INVALID_PTR(pwszPassword, dwError);
    BAIL_ON_INVALID_PTR(pPasswordBuffer, dwError);

    memset(&ctx, 0, sizeof(ctx));
    memset(&key, 0, sizeof(key));

    dwError = NetEncodeJoinPasswordBuffer(pwszPassword,
                                          PasswordBlob,
                                          sizeof(PasswordBlob));
    BAIL_ON_WIN_ERROR(dwError);

    if (!RAND_bytes(KeyInit, sizeof(KeyInit)))
    {
        dwError = ERROR_GEN_FAILURE;
        BAIL_ON_WIN_ERROR(dwError);
    }

    MD5_Init(&ctx);
    MD5_Update(&ctx, pConn->SessionKey, pConn->dwSessionKeyLen);
    MD5_Update(&ctx, KeyInit, sizeof(KeyInit));
    MD5_Final(DigestedSessionKey, &ctx);

    RC4_set_key(&key,
                sizeof(DigestedSessionKey),
                (unsigned char*)DigestedSessionKey);

    RC4(&key,
        sizeof(PasswordBlob),
        (const unsigned char*)PasswordBlob,
        (unsigned char*)PasswordBlob);

    iByte = 0;
    memcpy(&pPasswordBuffer->data[iByte], KeyInit, sizeof(KeyInit));
    iByte += sizeof(KeyInit);
    memcpy(&pPasswordBuffer->data[iByte], PasswordBlob, sizeof(PasswordBlob));

cleanup:
    return dwError;

error:
    memset(pPasswordBuffer, 0, sizeof(*pPasswordBuffer));

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
