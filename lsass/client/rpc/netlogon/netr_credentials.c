/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        netr_credentials.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Netlogon credentials functions (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
NetrEncryptChallenge(
    OUT BYTE   EncryptedChal[8],
    IN  BYTE   Challenge[8],
    IN  BYTE   SessionKey[14]
    );


static
NTSTATUS
NetrPrepareDesKey(
    IN  PBYTE  pInput,
    OUT PBYTE  pOutput
    );


static
NTSTATUS
NetrEncryptSessionKey(
    OUT BYTE   EncryptedSessKey[8],
    IN  BYTE   SessionKey[8],
    IN  BYTE   Key[16]
    );


static
NTSTATUS
NetrGenerateResponse(
    BYTE  EncryptedChal[24],
    BYTE  Challenge[8],
    BYTE  SessKey[16]
    );


VOID
NetrCredentialsInit(
    OUT NetrCredentials *pCreds,
    IN  BYTE             CliChal[8],
    IN  BYTE             SrvChal[8],
    IN  BYTE             PassHash[16],
    IN  UINT32           NegFlags
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HMAC_CTX HmacCtx;
    MD5_CTX MD5Ctx;

    if (pCreds == NULL) return;

    memset(&HmacCtx, 0, sizeof(HmacCtx));
    memset(&MD5Ctx, 0, sizeof(MD5Ctx));

    pCreds->negotiate_flags = NegFlags;
    pCreds->channel_type    = SCHANNEL_WKSTA;  /* default schannel type */
    pCreds->sequence        = time(NULL);

    memcpy(pCreds->pass_hash, PassHash, sizeof(pCreds->pass_hash));
    memset(pCreds->session_key, 0, sizeof(pCreds->session_key));

    if (pCreds->negotiate_flags & NETLOGON_NEG_128BIT)
    {
        BYTE Zero[4] = {0};
        BYTE Digest[16] = {0};
        DWORD dwSessionKeyLen = sizeof(pCreds->session_key);

        HMAC_CTX_init(&HmacCtx);
        HMAC_Init_ex(&HmacCtx,
                     pCreds->pass_hash, sizeof(pCreds->pass_hash),
                     EVP_md5(), NULL);

        MD5_Init(&MD5Ctx);
        MD5_Update(&MD5Ctx, Zero, sizeof(Zero));
        MD5_Update(&MD5Ctx, CliChal, 8);
        MD5_Update(&MD5Ctx, SrvChal, 8);
        MD5_Final(Digest, &MD5Ctx);

        HMAC_Update(&HmacCtx, Digest, sizeof(Digest));
        HMAC_Final(&HmacCtx, pCreds->session_key, &dwSessionKeyLen);

        HMAC_CTX_cleanup(&HmacCtx);

        ntStatus = NetrEncryptChallenge(pCreds->cli_chal.data,
                                        CliChal,
                                        pCreds->session_key);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NetrEncryptChallenge(pCreds->srv_chal.data,
                                        SrvChal,
                                        pCreds->session_key);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pCreds->seed.data,
               pCreds->cli_chal.data,
               sizeof(pCreds->seed.data));
    }
    else
    {
        UINT32 Sum1[2] = {0};
        BYTE Sum2[8] = {0};

        Sum1[0] = GETUINT32(CliChal, 0) +
                  GETUINT32(SrvChal, 0);
        Sum1[1] = GETUINT32(CliChal, 4) +
                  GETUINT32(SrvChal, 4);

        SETUINT32(Sum2, 0, Sum1[0]);
        SETUINT32(Sum2, 4, Sum1[1]);

        memset(pCreds->session_key,
               0,
               sizeof(pCreds->session_key));

        ntStatus = NetrEncryptSessionKey(pCreds->session_key,
                                         Sum2,
                                         pCreds->pass_hash);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NetrEncryptChallenge(pCreds->cli_chal.data,
                                        CliChal,
                                        pCreds->session_key);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NetrEncryptChallenge(pCreds->srv_chal.data,
                                        SrvChal,
                                        pCreds->session_key);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pCreds->seed.data,
               pCreds->cli_chal.data,
               sizeof(pCreds->seed.data));
    }

error:
    return;
}


static
NTSTATUS
NetrEncryptChallenge(
    OUT BYTE   EncryptedChal[8],
    IN  BYTE   Challenge[8],
    IN  BYTE   SessionKey[14]
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DES_cblock KeyBlockLo;
    DES_cblock KeyBlockHi;
    DES_key_schedule KeyLo;
    DES_key_schedule KeyHi;
    BYTE InOutBuffer[8] = {0};
    BYTE EncryptedChallenge[8] = {0};

    memset(&KeyBlockLo, 0, sizeof(KeyBlockLo));
    memset(&KeyBlockHi, 0, sizeof(KeyBlockHi));
    memset(&KeyLo, 0, sizeof(KeyLo));
    memset(&KeyHi, 0, sizeof(KeyHi));

    ntStatus = NetrPrepareDesKey(&SessionKey[0],
                                 (PBYTE)KeyBlockLo);
    BAIL_ON_NT_STATUS(ntStatus);

    DES_set_odd_parity(&KeyBlockLo);
    DES_set_key_unchecked(&KeyBlockLo, &KeyLo);

    ntStatus = NetrPrepareDesKey(&SessionKey[7],
                                 (PBYTE)KeyBlockHi);
    BAIL_ON_NT_STATUS(ntStatus);

    DES_set_odd_parity(&KeyBlockHi);
    DES_set_key_unchecked(&KeyBlockHi, &KeyHi);

    DES_ecb_encrypt((DES_cblock*)Challenge,
                    (DES_cblock*)InOutBuffer,
                    &KeyLo,
                    DES_ENCRYPT);
    DES_ecb_encrypt((DES_cblock*)InOutBuffer,
                    (DES_cblock*)EncryptedChallenge,
                    &KeyHi,
                    DES_ENCRYPT);

    memcpy(EncryptedChal, EncryptedChallenge, sizeof(EncryptedChallenge));

cleanup:
    memset(EncryptedChallenge, 0, sizeof(EncryptedChallenge));

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
NetrPrepareDesKey(
    IN  PBYTE  pInput,
    OUT PBYTE  pOutput
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD i = 0;

    BAIL_ON_INVALID_PTR(pInput, ntStatus);
    BAIL_ON_INVALID_PTR(pOutput, ntStatus);

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


static
NTSTATUS
NetrEncryptSessionKey(
    OUT BYTE   EncryptedSessKey[8],
    IN  BYTE   SessionKey[8],
    IN  BYTE   Key[16]
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DES_cblock KeyBlockLo;
    DES_cblock KeyBlockHi;
    DES_key_schedule KeyLo;
    DES_key_schedule KeyHi;
    BYTE InOutBuffer[8] = {0};
    BYTE EncryptedSessionKey[8] = {0};

    memset(&KeyBlockLo, 0, sizeof(KeyBlockLo));
    memset(&KeyBlockHi, 0, sizeof(KeyBlockHi));
    memset(&KeyLo, 0, sizeof(KeyLo));
    memset(&KeyHi, 0, sizeof(KeyHi));

    ntStatus = NetrPrepareDesKey(&Key[0],
                                 (PBYTE)KeyBlockLo);
    BAIL_ON_NT_STATUS(ntStatus);

    DES_set_odd_parity(&KeyBlockLo);
    DES_set_key_unchecked(&KeyBlockLo, &KeyLo);

    ntStatus = NetrPrepareDesKey(&Key[8],
                                 (PBYTE)KeyBlockHi);
    BAIL_ON_NT_STATUS(ntStatus);

    DES_set_odd_parity(&KeyBlockHi);
    DES_set_key_unchecked(&KeyBlockHi, &KeyHi);

    DES_ecb_encrypt((DES_cblock*)SessionKey,
                    (DES_cblock*)InOutBuffer,
                    &KeyLo,
                    DES_ENCRYPT);
    DES_ecb_encrypt((DES_cblock*)InOutBuffer,
                    (DES_cblock*)EncryptedSessionKey,
                    &KeyHi,
                    DES_ENCRYPT);

    memcpy(EncryptedSessKey, EncryptedSessionKey, sizeof(EncryptedSessionKey));

cleanup:
    memset(EncryptedSessionKey, 0, sizeof(EncryptedSessionKey));

    return ntStatus;

error:
    goto cleanup;
}


BOOLEAN
NetrCredentialsCorrect(
    IN  NetrCredentials  *pCreds,
    IN  BYTE              SrvCreds[8]
    )
{
    BOOLEAN bCorrect = FALSE;

    if (pCreds == NULL) goto error;

    if (memcmp(pCreds->srv_chal.data,
               SrvCreds,
               sizeof(pCreds->srv_chal.data)) == 0)
    {
        bCorrect = TRUE;
    }

cleanup:
    return bCorrect;

error:
    goto cleanup;
}


VOID
NetrCredentialsCliStep(
    IN OUT NetrCredentials *pCreds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NetrCred Chal;

    memset(&Chal, 0, sizeof(Chal));

    memcpy(Chal.data,
           pCreds->seed.data,
           sizeof(Chal.data));
    SETUINT32(Chal.data,
              0,
              GETUINT32(pCreds->seed.data, 0) + pCreds->sequence);

    ntStatus = NetrEncryptChallenge(pCreds->cli_chal.data,
                                    Chal.data,
                                    pCreds->session_key);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(Chal.data,
           pCreds->seed.data,
           sizeof(Chal.data));
    SETUINT32(Chal.data,
              0,
              GETUINT32(pCreds->seed.data, 0) + pCreds->sequence + 1);
    ntStatus = NetrEncryptChallenge(pCreds->srv_chal.data,
                                    Chal.data,
                                    pCreds->session_key);
    BAIL_ON_NT_STATUS(ntStatus);

    /* reseed */
    memcpy(Chal.data,
           pCreds->seed.data,
           sizeof(Chal.data));
    SETUINT32(Chal.data,
              0,
              GETUINT32(pCreds->seed.data, 0) + pCreds->sequence + 1);

    pCreds->seed = Chal;

error:
    return;
}


VOID
NetrCredentialsSrvStep(
    IN OUT NetrCredentials *pCreds
    )
{
}


VOID
NetrGetNtHash(
    OUT BYTE    Hash[16],
    IN  PCWSTR  pwszPassword
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    size_t sPasswordLen = 0;
    size_t sPasswordSize = 0;
    /* Little-endian encoded password string */
    PWSTR pwszPasswordLE = NULL;

    BAIL_ON_INVALID_PTR(Hash, ntStatus);
    BAIL_ON_INVALID_PTR(pwszPassword, ntStatus);

    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_WIN_ERROR(dwError);

    sPasswordSize = sPasswordLen * sizeof(pwszPasswordLE[0]);

    ntStatus = NetrAllocateMemory(OUT_PPVOID(&pwszPasswordLE),
                                  sPasswordSize + sizeof(pwszPasswordLE[0]));
    BAIL_ON_NT_STATUS(ntStatus);

    wc16stowc16les(pwszPasswordLE, pwszPassword, sPasswordLen);
    MD4((PBYTE)pwszPasswordLE, sPasswordSize, Hash);
    
error:
    if (pwszPasswordLE)
    {
        memset(pwszPasswordLE, 0, sPasswordSize + sizeof(pwszPasswordLE[0]));
        NetrFreeMemory(pwszPasswordLE);
    }

    return;
}


VOID
NetrGetLmHash(
    OUT BYTE    Hash[16],
    IN  PCWSTR  pwszPassword
    )
{
    const size_t sMaxPasswordLen = 14;
    const BYTE InputData[] = "KGS!@#$%";

    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    size_t sPasswordLen = 0;
    PSTR pszPassword = NULL;
    DWORD i = 0;
    DES_cblock KeyBlockLo;
    DES_cblock KeyBlockHi;
    DES_key_schedule KeyLo;
    DES_key_schedule KeyHi;

    memset(&KeyBlockLo, 0, sizeof(KeyBlockLo));
    memset(&KeyBlockHi, 0, sizeof(KeyBlockHi));
    memset(&KeyLo, 0, sizeof(KeyLo));
    memset(&KeyHi, 0, sizeof(KeyHi));


    /* password can be 14 characters long at most */
    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_WIN_ERROR(dwError);

    if (sPasswordLen > sMaxPasswordLen)
    {
        goto error;
    }

    dwError = LwWc16sToMbs(pwszPassword, &pszPassword);
    BAIL_ON_WIN_ERROR(dwError);
    
    for (i = 0; i < sPasswordLen; i++)
    {
        pszPassword[i] = (CHAR) toupper(pszPassword[i]);
    }

    ntStatus = NetrPrepareDesKey((PBYTE)&pszPassword[0],
                                 (PBYTE)KeyBlockLo);
    BAIL_ON_NT_STATUS(ntStatus);

    DES_set_odd_parity(&KeyBlockLo);
    DES_set_key_unchecked(&KeyBlockLo, &KeyLo);

    ntStatus = NetrPrepareDesKey((PBYTE)&pszPassword[7],
                                 (PBYTE)KeyBlockHi);
    BAIL_ON_NT_STATUS(ntStatus);

    DES_set_odd_parity(&KeyBlockHi);
    DES_set_key_unchecked(&KeyBlockHi, &KeyHi);

    DES_ecb_encrypt((DES_cblock*)InputData,
                    (DES_cblock*)&Hash[0],
                    &KeyLo,
                    DES_ENCRYPT);
    DES_ecb_encrypt((DES_cblock*)InputData,
                    (DES_cblock*)&Hash[8],
                    &KeyHi,
                    DES_ENCRYPT);

error:
    if (pszPassword)
    {
        memset(pszPassword, 0, sizeof(pszPassword[0]) * sPasswordLen);
        NetrFreeMemory(pszPassword);
    }
}


NTSTATUS
NetrNTLMv1EncryptChallenge(
    BYTE  Challenge[8],
    PBYTE pLmHash,
    PBYTE pNtHash,
    BYTE  LmResponse[24],
    BYTE  NtResponse[24]
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!pLmHash && !pNtHash)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pLmHash)
    {
        ntStatus = NetrGenerateResponse(LmResponse,
                                        Challenge,
                                        pLmHash);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pNtHash)
    {
        ntStatus = NetrGenerateResponse(NtResponse,
                                        Challenge,
                                        pNtHash);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    return ntStatus;

error:
    memset(LmResponse, 0, 24);
    memset(NtResponse, 0, 24);

    goto cleanup;
}


static
NTSTATUS
NetrGenerateResponse(
    BYTE  EncryptedChal[24],
    BYTE  Challenge[8],
    BYTE  SessKey[16]
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BYTE Key[21] = {0};
    DES_cblock KeyBlock1;
    DES_cblock KeyBlock2;
    DES_cblock KeyBlock3;
    DES_key_schedule Key1;
    DES_key_schedule Key2;
    DES_key_schedule Key3;

    memset(&KeyBlock1, 0, sizeof(KeyBlock1));
    memset(&KeyBlock2, 0, sizeof(KeyBlock2));
    memset(&KeyBlock3, 0, sizeof(KeyBlock3));
    memset(&Key1, 0, sizeof(Key1));
    memset(&Key2, 0, sizeof(Key2));
    memset(&Key3, 0, sizeof(Key3));

    memcpy(Key, SessKey, 16);

    ntStatus = NetrPrepareDesKey((PBYTE)&Key[0],
                                 (PBYTE)KeyBlock1);
    BAIL_ON_NT_STATUS(ntStatus);

    DES_set_odd_parity(&KeyBlock1);
    DES_set_key_unchecked(&KeyBlock1, &Key1);

    ntStatus = NetrPrepareDesKey((PBYTE)&Key[7],
                                 (PBYTE)KeyBlock2);
    BAIL_ON_NT_STATUS(ntStatus);

    DES_set_odd_parity(&KeyBlock2);
    DES_set_key_unchecked(&KeyBlock2, &Key2);

    ntStatus = NetrPrepareDesKey((PBYTE)&Key[14],
                                 (PBYTE)KeyBlock3);
    BAIL_ON_NT_STATUS(ntStatus);

    DES_set_odd_parity(&KeyBlock3);
    DES_set_key_unchecked(&KeyBlock3, &Key3);

    DES_ecb_encrypt((DES_cblock*)Challenge,
                    (DES_cblock*)&EncryptedChal[0],
                    &Key1,
                    DES_ENCRYPT);

    DES_ecb_encrypt((DES_cblock*)Challenge,
                    (DES_cblock*)&EncryptedChal[8],
                    &Key2,
                    DES_ENCRYPT);

    DES_ecb_encrypt((DES_cblock*)Challenge,
                    (DES_cblock*)&EncryptedChal[16],
                    &Key3,
                    DES_ENCRYPT);

cleanup:
    memset(Key, 0, 21);

    return ntStatus;

error:
    memset(EncryptedChal, 0, 24);

    goto cleanup;
}
