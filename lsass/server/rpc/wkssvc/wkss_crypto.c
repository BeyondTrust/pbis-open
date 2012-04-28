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
 *        wkss_crypto.c
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
DWORD
WkssSrvDecodePasswordBuffer(
    IN  PBYTE   pBlob,
    IN  DWORD   dwBlobSize,
    OUT PWSTR  *ppwszPassword,
    OUT PDWORD  pdwPasswordLen
    );


static
DWORD
WkssSrvEncodePasswordBuffer(
    IN  PCWSTR  pwszPassword,
    IN  PBYTE   pBlobInit,
    OUT PBYTE  *ppBlob,
    OUT PDWORD  pdwBlobSize
    );


DWORD
WkssSrvDecryptPasswordBlob(
    IN  PWKSS_SRV_CONTEXT          pSrvCtx,
    IN  PENC_JOIN_PASSWORD_BUFFER  pPassBuffer,
    IN  PBYTE                      pCryptKey,
    IN  DWORD                      dwCryptKeyLen,
    OUT PWSTR                     *ppwszPassword
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus ATTRIBUTE_UNUSED = STATUS_SUCCESS;
    PWSTR pwszPassword = NULL;
    PBYTE pPlainTextBlob = NULL;
    DWORD dwPlainTextBlobSize = 0;
    DWORD dwPassBlobSize = 0;
    PBYTE pKey = NULL;
    DWORD dwKeyLen = 0;
    BYTE KeyInit[8] = {0};
    BYTE DigestedSessionKey[16] = {0};
    DWORD iByte = 0;
    MD5_CTX ctx;
    RC4_KEY key;
    DWORD dwPasswordLen = 0;
    ENC_JOIN_PASSWORD_BUFFER PassBlobVerifier;

    BAIL_ON_INVALID_PTR(pSrvCtx, ntStatus);
    BAIL_ON_INVALID_PTR(pPassBuffer, ntStatus);
    BAIL_ON_INVALID_PTR(ppwszPassword, ntStatus);

    dwPassBlobSize      = sizeof(pPassBuffer->data);
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
        pKey     = pSrvCtx->pSessionKey;
        dwKeyLen = pSrvCtx->dwSessionKeyLen;
    }

    /*
     * Copy crypto key initialiser
     */
    iByte = 0;
    memcpy(KeyInit,
           &(pPassBuffer->data[iByte]),
           sizeof(KeyInit));

    /*
     * Prepare the session key digested with key initialisator
     */
    MD5_Init(&ctx);
    MD5_Update(&ctx, pKey, dwKeyLen);
    MD5_Update(&ctx, KeyInit, sizeof(KeyInit));
    MD5_Final(DigestedSessionKey, &ctx);

    /*
     * Set the key and decrypt the plain text password buffer
     */
    RC4_set_key(&key,
                sizeof(DigestedSessionKey),
                (unsigned char*)DigestedSessionKey);

    iByte += sizeof(KeyInit);
    RC4(&key,
        dwPlainTextBlobSize,
        (const unsigned char*)&(pPassBuffer->data[iByte]),
        (unsigned char*)pPlainTextBlob);

    /*
     * Get the unicode password from plain text blob
     */
    dwError = WkssSrvDecodePasswordBuffer(pPlainTextBlob,
                                          dwPlainTextBlobSize,
                                          &pwszPassword,
                                          &dwPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * More careful check - password, key init and blob init should
     * yield the same encrypted blob as passed in the function input
     */
    dwError = WkssSrvEncryptPasswordBlobEx(pSrvCtx,
                                           pwszPassword,
                                           pKey,
                                           dwKeyLen,
                                           KeyInit,
                                           sizeof(KeyInit),
                                           pPlainTextBlob,
                                           &PassBlobVerifier);
    BAIL_ON_LSA_ERROR(dwError);

    if (memcmp(pPassBuffer->data, PassBlobVerifier.data,
               sizeof(pPassBuffer->data)))
    {
        dwError = ERROR_INVALID_PASSWORD;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppwszPassword = pwszPassword;

cleanup:
    LW_SECURE_FREE_MEMORY(pPlainTextBlob, dwPlainTextBlobSize);

    return dwError;

error:
    *ppwszPassword = NULL;
    
    goto cleanup;
}


static
DWORD
WkssSrvDecodePasswordBuffer(
    IN  PBYTE   pBlob,
    IN  DWORD   dwBlobSize,
    OUT PWSTR  *ppwszPassword,
    OUT PDWORD  pdwPasswordLen
    )
{
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
        dwError = ERROR_INVALID_PASSWORD;
        BAIL_ON_LSA_ERROR(dwError);
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
    wc16lestowc16s(pwszPassword, pwszPasswordLE, dwPasswordLen);

    *ppwszPassword  = pwszPassword;
    *pdwPasswordLen = dwPasswordLen / 2;

cleanup:
    LW_SECURE_FREE_WSTRING(pwszPasswordLE);

    return dwError;

error:
    LW_SECURE_FREE_WSTRING(pwszPassword);

    *ppwszPassword = NULL;

    goto cleanup;
}


DWORD
WkssSrvEncryptPasswordBlobEx(
    IN  PWKSS_SRV_CONTEXT          pSrvCtx,
    IN  PCWSTR                     pwszPassword,
    IN  PBYTE                      pCryptoKey,
    IN  DWORD                      dwCryptoKeyLen,
    IN  PBYTE                      pKeyInit,
    IN  DWORD                      dwKeyInitLen,
    IN  PBYTE                      pBlobInit,
    OUT PENC_JOIN_PASSWORD_BUFFER  pEncryptedPassBlob
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
    DWORD iByte = 0;

    memset(&ctx, 0, sizeof(ctx));
    memset(&key, 0, sizeof(key));

    dwError = WkssSrvEncodePasswordBuffer(pwszPassword,
                                          pBlobInit,
                                          &pPassBlob,
                                          &dwPassBlobLen);
    BAIL_ON_LSA_ERROR(dwError);

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
        pKey     = pSrvCtx->pSessionKey;
        dwKeyLen = pSrvCtx->dwSessionKeyLen;
    }

    /*
     * Prepare the session key digested with key initialisator
     */
    MD5_Init(&ctx);
    MD5_Update(&ctx, pKey, dwKeyLen);
    MD5_Update(&ctx, pKeyInit, dwKeyInitLen);
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
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    iByte = 0;
    memcpy(&(pEncryptedPassBlob->data[iByte]),
           pKeyInit,
           dwKeyInitLen);
    iByte += dwKeyInitLen;
    memcpy(&(pEncryptedPassBlob->data[iByte]),
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
DWORD
WkssSrvEncodePasswordBuffer(
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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
