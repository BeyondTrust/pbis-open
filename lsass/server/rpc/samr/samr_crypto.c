/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        samr_crypto.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Encrypted password blob handling functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
SamrSrvDecodePasswordBuffer(
    IN  PBYTE   pBlob,
    IN  DWORD   dwBlobSize,
    OUT PWSTR  *ppwszPassword,
    OUT PDWORD  pdwPasswordLen
    );


static
NTSTATUS
SamrSrvEncodePasswordBuffer(
    IN  PCWSTR  pwszPassword,
    IN  PBYTE   pBlobInit,
    OUT PBYTE  *ppBlob,
    OUT PDWORD  pdwBlobSize
    );


NTSTATUS
SamrSrvDecryptPasswordBlobEx(
    IN  PCONNECT_CONTEXT  pConnCtx,
    IN  CryptPasswordEx  *pPassBlob,
    IN  PBYTE             pCryptKey,
    IN  DWORD             dwCryptKeyLen,
    IN  UINT8             PassLen,
    OUT PWSTR            *ppwszPassword
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PBYTE pPlainTextBlob = NULL;
    DWORD dwPlainTextBlobSize = 0;
    DWORD dwPassBlobSize = 0;
    PBYTE pKey = NULL;
    DWORD dwKeyLen = 0;
    BYTE KeyInit[16] = {0};
    BYTE DigestedSessionKey[16] = {0};
    MD5_CTX ctx;
    RC4_KEY key;
    PWSTR pwszPassword = NULL;
    DWORD dwPasswordLen = 0;
    CryptPasswordEx PassBlobVerifier;

    BAIL_ON_INVALID_PTR(pConnCtx);
    BAIL_ON_INVALID_PTR(pPassBlob);
    BAIL_ON_INVALID_PTR(ppwszPassword);

    dwPassBlobSize      = sizeof(pPassBlob->data);
    dwPlainTextBlobSize = dwPassBlobSize - sizeof(KeyInit);
    memset(&ctx, 0, sizeof(ctx));
    memset(&key, 0, sizeof(key));
    memset(&PassBlobVerifier, 0, sizeof(PassBlobVerifier));

    /*
     * Allocate memory for plain text password blob
     */
    dwError = LwAllocateMemory(dwPlainTextBlobSize,
                               OUT_PPVOID(&pPlainTextBlob));
    BAIL_ON_LSA_ERROR(dwError);

    if (pCryptKey)
    {
        pKey     = pCryptKey;
        dwKeyLen = dwCryptKeyLen;
    }
    else
    {
        pKey     = pConnCtx->pSessionKey;
        dwKeyLen = pConnCtx->dwSessionKeyLen;
    }

    /*
     * Copy crypto key initialisator
     */
    memcpy(KeyInit,
           &(pPassBlob->data[dwPassBlobSize - sizeof(KeyInit)]),
           sizeof(KeyInit));

    /*
     * Prepare the session key digested with key initialisator
     */
    MD5_Init(&ctx);
    MD5_Update(&ctx, KeyInit, sizeof(KeyInit));
    MD5_Update(&ctx, pKey, dwKeyLen);
    MD5_Final(DigestedSessionKey, &ctx);

    /*
     * Set the key and decrypt the plain text password buffer
     */
    RC4_set_key(&key,
                sizeof(DigestedSessionKey),
                (unsigned char*)DigestedSessionKey);
    RC4(&key,
        dwPlainTextBlobSize,
        (const unsigned char*)pPassBlob->data,
        (unsigned char*)pPlainTextBlob);

    /*
     * Get the unicode password from plain text blob
     */
    ntStatus = SamrSrvDecodePasswordBuffer(pPlainTextBlob,
                                           dwPlainTextBlobSize,
                                           &pwszPassword,
                                           &dwPasswordLen);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Basic check if the password has been decrypted correctly
     * (if password length has been passed)
     */
    if (PassLen &&
        PassLen != dwPasswordLen)
    {
        ntStatus = STATUS_WRONG_PASSWORD;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * More careful check - password, key init and blob init should
     * yield the same encrypted blob as passed in the function input
     */
    ntStatus = SamrSrvEncryptPasswordBlobEx(pConnCtx,
                                            pwszPassword,
                                            pKey,
                                            dwKeyLen,
                                            KeyInit,
                                            sizeof(KeyInit),
                                            pPlainTextBlob,
                                            &PassBlobVerifier);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (memcmp(pPassBlob->data, PassBlobVerifier.data,
               sizeof(pPassBlob->data)))
    {
        ntStatus = STATUS_WRONG_PASSWORD;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    *ppwszPassword = pwszPassword;

cleanup:
    LW_SECURE_FREE_MEMORY(pPlainTextBlob, dwPlainTextBlobSize);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    *ppwszPassword = NULL;
    
    goto cleanup;
}


NTSTATUS
SamrSrvDecryptPasswordBlob(
    IN  PCONNECT_CONTEXT  pConnCtx,
    IN  CryptPassword    *pPassBlob,
    IN  PBYTE             pCryptKey,
    IN  DWORD             dwCryptKeyLen,
    IN  UINT8             PassLen,              
    OUT PWSTR            *ppwszPassword
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PBYTE pPlainTextBlob = NULL;
    DWORD dwPlainTextBlobSize = 0;
    DWORD dwPassBlobSize = 0;
    PBYTE pKey = NULL;
    DWORD dwKeyLen = 0;
    RC4_KEY key;
    PWSTR pwszPassword = NULL;
    DWORD dwPasswordLen = 0;
    CryptPassword PassBlobVerifier;

    BAIL_ON_INVALID_PTR(pConnCtx);
    BAIL_ON_INVALID_PTR(pPassBlob);
    BAIL_ON_INVALID_PTR(ppwszPassword);

    dwPassBlobSize      = sizeof(pPassBlob->data);
    dwPlainTextBlobSize = dwPassBlobSize - dwKeyLen;
    memset(&key, 0, sizeof(key));
    memset(&PassBlobVerifier, 0, sizeof(PassBlobVerifier));

    /*
     * Allocate memory for plain text password blob
     */
    dwError = LwAllocateMemory(dwPlainTextBlobSize,
                               OUT_PPVOID(&pPlainTextBlob));
    BAIL_ON_LSA_ERROR(dwError);

    if (pCryptKey)
    {
        pKey     = pCryptKey;
        dwKeyLen = dwCryptKeyLen;
    }
    else
    {
        pKey     = pConnCtx->pSessionKey;
        dwKeyLen = pConnCtx->dwSessionKeyLen;
    }

    /*
     * Set the key and decrypt the plain text password buffer
     */
    RC4_set_key(&key,
                (int)dwKeyLen,
                (unsigned char*)pKey);
    RC4(&key,
        dwPlainTextBlobSize,
        (const unsigned char*)pPassBlob->data,
        (unsigned char*)pPlainTextBlob);

    /*
     * Get the unicode password from plain text blob
     */
    ntStatus = SamrSrvDecodePasswordBuffer(pPlainTextBlob,
                                           dwPlainTextBlobSize,
                                           &pwszPassword,
                                           &dwPasswordLen);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Basic check if the password has been decrypted correctly
     * (if password length has been passed)
     */
    if (PassLen &&
        PassLen != dwPasswordLen)
    {
        ntStatus = STATUS_WRONG_PASSWORD;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * More careful check - password, key init and blob init should
     * yield the same encrypted blob as passed in the function input
     */
    ntStatus = SamrSrvEncryptPasswordBlob(pConnCtx,
                                          pwszPassword,
                                          pKey,
                                          dwKeyLen,
                                          pPlainTextBlob,
                                          &PassBlobVerifier);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (memcmp(pPassBlob->data, PassBlobVerifier.data,
               sizeof(pPassBlob->data)))
    {
        ntStatus = STATUS_WRONG_PASSWORD;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    *ppwszPassword = pwszPassword;

cleanup:
    LW_SECURE_FREE_MEMORY(pPlainTextBlob, dwPlainTextBlobSize);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    *ppwszPassword = NULL;
    
    goto cleanup;
}


static
NTSTATUS
SamrSrvDecodePasswordBuffer(
    IN  PBYTE   pBlob,
    IN  DWORD   dwBlobSize,
    OUT PWSTR  *ppwszPassword,
    OUT PDWORD  pdwPasswordLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwPasswordLen = 0;
    DWORD iByte = dwBlobSize;
    PWSTR pwszPasswordLE = NULL;
    PWSTR pwszPassword = NULL;

    /*
     * Decode the password length (in bytes) - the last 4 bytes
     */
    dwPasswordLen |= pBlob[--iByte] << 24;
    dwPasswordLen |= pBlob[--iByte] << 16;
    dwPasswordLen |= pBlob[--iByte] << 8;
    dwPasswordLen |= pBlob[--iByte];

    dwError = LwAllocateMemory(dwPasswordLen + sizeof(pwszPasswordLE[0]),
                               OUT_PPVOID(&pwszPasswordLE));
    BAIL_ON_LSA_ERROR(dwError);

    if (dwPasswordLen > iByte)
    {
        ntStatus = STATUS_WRONG_PASSWORD;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Copy the password - it's right before the length bytes 
     */
    iByte -= dwPasswordLen;
    memcpy(pwszPasswordLE, &(pBlob[iByte]), dwPasswordLen);

    dwError = LwAllocateMemory(dwPasswordLen + sizeof(pwszPassword[0]),
                               OUT_PPVOID(&pwszPassword));
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Copied password is a 2-byte little-endian string. Make
     * sure we return a string in native endianness
     */
    wc16lestowc16s(pwszPassword,
                   pwszPasswordLE,
                   (dwPasswordLen/sizeof(pwszPassword[0])) + 1);

    *ppwszPassword  = pwszPassword;
    *pdwPasswordLen = dwPasswordLen / sizeof(pwszPassword[0]);

cleanup:
    LW_SECURE_FREE_WSTRING(pwszPasswordLE);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    LW_SECURE_FREE_WSTRING(pwszPassword);

    *ppwszPassword = NULL;

    goto cleanup;
}


NTSTATUS
SamrSrvEncryptPasswordBlobEx(
    IN  PCONNECT_CONTEXT  pConnCtx,
    IN  PCWSTR            pwszPassword,
    IN  PBYTE             pCryptoKey,
    IN  DWORD             dwCryptoKeyLen,
    IN  PBYTE             pKeyInit,
    IN  DWORD             dwKeyInitLen,
    IN  PBYTE             pBlobInit,
    OUT CryptPasswordEx  *pEncryptedPassBlob
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PBYTE pPassBlob = NULL;
    DWORD dwPassBlobLen = 0;
    PBYTE pKey = NULL;
    DWORD dwKeyLen = 0;
    BYTE DigestedSessionKey[16] = {0};
    MD5_CTX ctx;
    RC4_KEY key;
    PBYTE pEncryptedBlob = NULL;

    memset(&ctx, 0, sizeof(ctx));
    memset(&key, 0, sizeof(key));

    ntStatus = SamrSrvEncodePasswordBuffer(pwszPassword,
                                           pBlobInit,
                                           &pPassBlob,
                                           &dwPassBlobLen);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwAllocateMemory(dwPassBlobLen,
                               OUT_PPVOID(&pEncryptedBlob));
    BAIL_ON_LSA_ERROR(dwError);

    if (pCryptoKey)
    {
        pKey     = pCryptoKey;
        dwKeyLen = dwCryptoKeyLen;
    }
    else
    {
        pKey     = pConnCtx->pSessionKey;
        dwKeyLen = pConnCtx->dwSessionKeyLen;
    }

    /*
     * Prepare the session key digested with key initialisator
     */
    MD5_Init(&ctx);
    MD5_Update(&ctx, pKeyInit, dwKeyInitLen);
    MD5_Update(&ctx, pKey, dwKeyLen);
    MD5_Final(DigestedSessionKey, &ctx);

    /*
     * Set the key and encrypt the plain text password buffer
     */
    RC4_set_key(&key,
                sizeof(DigestedSessionKey),
                (unsigned char*)DigestedSessionKey);

    RC4(&key,
        dwPassBlobLen,
        (const unsigned char*)pPassBlob,
        (unsigned char*)pEncryptedBlob);

    if (dwPassBlobLen + dwKeyInitLen > sizeof(pEncryptedPassBlob->data))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    memcpy(pEncryptedPassBlob->data,
           pEncryptedBlob,
           dwPassBlobLen);
    memcpy(&(pEncryptedPassBlob->data[dwPassBlobLen]),
           pKeyInit,
           dwKeyInitLen);

cleanup:
    LW_SECURE_FREE_MEMORY(pPassBlob, dwPassBlobLen);
    LW_SECURE_FREE_MEMORY(pEncryptedBlob, dwPassBlobLen);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    memset(pEncryptedBlob, 0, sizeof(*pEncryptedBlob));

    goto cleanup;
}


NTSTATUS
SamrSrvEncryptPasswordBlob(
    IN  PCONNECT_CONTEXT  pConnCtx,
    IN  PCWSTR            pwszPassword,
    IN  PBYTE             pCryptoKey,
    IN  DWORD             dwCryptoKeyLen,
    IN  PBYTE             pBlobInit,
    OUT CryptPassword    *pEncryptedPassBlob
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PBYTE pPassBlob = NULL;
    DWORD dwPassBlobLen = 0;
    PBYTE pKey = NULL;
    DWORD dwKeyLen = 0;
    MD5_CTX ctx;
    RC4_KEY key;
    PBYTE pEncryptedBlob = NULL;

    memset(&ctx, 0, sizeof(ctx));
    memset(&key, 0, sizeof(key));

    ntStatus = SamrSrvEncodePasswordBuffer(pwszPassword,
                                           pBlobInit,
                                           &pPassBlob,
                                           &dwPassBlobLen);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwAllocateMemory(dwPassBlobLen,
                               OUT_PPVOID(&pEncryptedBlob));
    BAIL_ON_LSA_ERROR(dwError);

    if (pCryptoKey)
    {
        pKey     = pCryptoKey;
        dwKeyLen = dwCryptoKeyLen;
    }
    else
    {
        pKey     = pConnCtx->pSessionKey;
        dwKeyLen = pConnCtx->dwSessionKeyLen;
    }

    /*
     * Set the key and encrypt the plain text password buffer
     */
    RC4_set_key(&key,
                (int)dwKeyLen,
                (unsigned char*)pKey);

    RC4(&key,
        dwPassBlobLen,
        (const unsigned char*)pPassBlob,
        (unsigned char*)pEncryptedBlob);

    if (dwPassBlobLen > sizeof(pEncryptedPassBlob->data))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    memcpy(pEncryptedPassBlob->data,
           pEncryptedBlob,
           dwPassBlobLen);

cleanup:
    LW_SECURE_FREE_MEMORY(pPassBlob, dwPassBlobLen);
    LW_SECURE_FREE_MEMORY(pEncryptedBlob, dwPassBlobLen);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    memset(pEncryptedBlob, 0, sizeof(*pEncryptedBlob));

    goto cleanup;
}


static
NTSTATUS
SamrSrvEncodePasswordBuffer(
    IN  PCWSTR  pwszPassword,
    IN  PBYTE   pBlobInit,
    OUT PBYTE  *ppBlob,
    OUT PDWORD  pdwBlobSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    size_t sPasswordLen = 0;
    size_t sPasswordSize = 0;
    PWSTR pwszPasswordLE = NULL;
    BYTE PasswordBlob[516] = {0};
    DWORD iByte = 0;
    PBYTE pBlob = NULL;
    DWORD dwBlobSize = 0;

    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    /* size doesn't include terminating zero here */
    sPasswordSize = sPasswordLen * sizeof(pwszPassword[0]);

    /*
     * Make sure encoded password is 2-byte little-endian
     */
    dwError = LwAllocateMemory(sPasswordSize + sizeof(pwszPassword[0]),
                               OUT_PPVOID(&pwszPasswordLE));
    BAIL_ON_LSA_ERROR(dwError);

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
    memcpy(&(PasswordBlob[iByte]),
           pwszPasswordLE,
           sPasswordSize);

    memcpy(PasswordBlob,
           pBlobInit,
           iByte);

    dwBlobSize = sizeof(PasswordBlob);
    dwError = LwAllocateMemory(dwBlobSize,
                               OUT_PPVOID(&pBlob));
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pBlob, PasswordBlob, dwBlobSize);

    *ppBlob      = pBlob;
    *pdwBlobSize = dwBlobSize;

cleanup:
    if (pBlob)
    {
        memset(PasswordBlob, 0, sizeof(PasswordBlob));
    }

    LW_SECURE_FREE_WSTRING(pwszPasswordLE);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    LW_SECURE_FREE_MEMORY(pBlob, dwBlobSize);

    *ppBlob      = NULL;
    *pdwBlobSize = 0;

    goto cleanup;
}


NTSTATUS
SamrSrvGetNtPasswordHash(
    IN  PCWSTR  pwszPassword,
    OUT PBYTE  *ppNtHash,
    OUT PDWORD  pdwNtHashLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    size_t sPasswordLen = 0;
    BYTE Hash[16] = {0};
    PBYTE pNtHash = NULL;
    DWORD dwNtHashLen = 0;

    BAIL_ON_INVALID_PTR(pwszPassword);
    BAIL_ON_INVALID_PTR(ppNtHash);

    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    MD4((PBYTE)pwszPassword,
        sPasswordLen * sizeof(pwszPassword[0]),
        Hash);

    dwNtHashLen = sizeof(Hash);
    dwError = LwAllocateMemory(dwNtHashLen,
                               OUT_PPVOID(&pNtHash));
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pNtHash, (void*)Hash, dwNtHashLen);

    *ppNtHash     = pNtHash;
    *pdwNtHashLen = dwNtHashLen;

cleanup:
    memset((void*)Hash, 0, sizeof(Hash));

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    LW_SECURE_FREE_MEMORY(pNtHash, dwNtHashLen);

    *ppNtHash     = NULL;
    *pdwNtHashLen = 0;

    goto cleanup;
}


NTSTATUS
SamrSrvPrepareDesKey(
    IN  PBYTE  pInput,
    OUT PBYTE  pOutput
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD i = 0;

    BAIL_ON_INVALID_PTR(pInput);
    BAIL_ON_INVALID_PTR(pOutput);

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
    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrSrvVerifyNewNtPasswordHash(
    IN  PBYTE         pNewNtHash,
    IN  DWORD         dwNewNtHashLen,
    IN  PBYTE         pOldNtHash,
    IN  DWORD         dwOldNtHashLen,
    IN  HashPassword *pNtVerifier
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DES_cblock KeyBlockLo;
    DES_cblock KeyBlockHi;
    DES_key_schedule KeyLo;
    DES_key_schedule KeyHi;
    BYTE Verifier[16];

    BAIL_ON_INVALID_PTR(pNewNtHash);
    BAIL_ON_INVALID_PTR(pOldNtHash);
    BAIL_ON_INVALID_PTR(pNtVerifier);

    memset(&KeyBlockLo, 0, sizeof(KeyBlockLo));
    memset(&KeyBlockHi, 0, sizeof(KeyBlockHi));
    memset(&KeyLo, 0, sizeof(KeyLo));
    memset(&KeyHi, 0, sizeof(KeyHi));

    ntStatus = SamrSrvPrepareDesKey(&pNewNtHash[0],
                                    (PBYTE)KeyBlockLo);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    DES_set_odd_parity(&KeyBlockLo);
    DES_set_key_unchecked(&KeyBlockLo, &KeyLo);

    ntStatus = SamrSrvPrepareDesKey(&pNewNtHash[7],
                                    (PBYTE)KeyBlockHi);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

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

    if (memcmp((void*)Verifier,
               (void*)pNtVerifier->data,
               sizeof(Verifier)))
    {
        ntStatus = STATUS_WRONG_PASSWORD;
    }

cleanup:
    memset(&KeyBlockLo, 0, sizeof(KeyBlockLo));
    memset(&KeyBlockHi, 0, sizeof(KeyBlockHi));
    memset(&KeyLo, 0, sizeof(KeyLo));
    memset(&KeyHi, 0, sizeof(KeyHi));

    return ntStatus;

error:
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
