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
