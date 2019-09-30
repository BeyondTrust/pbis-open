/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        samr_crypto.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Encrypted password blob handling functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _SAMR_CRYPTO_H_
#define _SAMR_CRYPTO_H_


NTSTATUS
SamrSrvDecryptPasswordBlobEx(
    IN  PCONNECT_CONTEXT  pConnCtx,
    IN  CryptPasswordEx  *pPassBlob,
    IN  PBYTE             pCryptKey,
    IN  DWORD             dwCryptKeyLen,
    IN  UINT8             PassLen,              
    OUT PWSTR            *ppwszPassword
    );


NTSTATUS
SamrSrvDecryptPasswordBlob(
    IN  PCONNECT_CONTEXT  pConnCtx,
    IN  CryptPassword    *pPassBlob,
    IN  PBYTE             pCryptKey,
    IN  DWORD             dwCryptKeyLen,
    IN  UINT8             PassLen,              
    OUT PWSTR            *ppwszPassword
    );


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
    );


NTSTATUS
SamrSrvEncryptPasswordBlob(
    IN  PCONNECT_CONTEXT  pConnCtx,
    IN  PCWSTR            pwszPassword,
    IN  PBYTE             pCryptoKey,
    IN  DWORD             dwCryptoKeyLen,
    IN  PBYTE             pBlobInit,
    OUT CryptPassword    *pEncryptedPassBlob
    );


NTSTATUS
SamrSrvGetNtPasswordHash(
    IN  PCWSTR  pwszPassword,
    OUT PBYTE  *ppNtHash,
    OUT PDWORD  pdwNtHashLen
    );


NTSTATUS
SamrSrvPrepareDesKey(
    IN  PBYTE  pInput,
    OUT PBYTE  pOutput
    );


NTSTATUS
SamrSrvVerifyNewNtPasswordHash(
    IN  PBYTE         pNewNtHash,
    IN  DWORD         dwNewNtHashLen,
    IN  PBYTE         pOldNtHash,
    IN  DWORD         dwOldNtHashLen,
    IN  HashPassword *pNtVerifier
    );


#endif /* _SAMR_CRYPTO_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
