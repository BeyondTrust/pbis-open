/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        lwkrb5.h
 *
 * Abstract:
 *
 *        BeyondTrust Advanced API (lwadvapi)
 *
 *        Kerberos 5 API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak <rszczesniak@likewisesoftware.com>
 *
 */
#ifndef __LWKRB5_H__
#define __LWKRB5_H__

#include <lw/types.h>
#include <lw/attrs.h>

// Needed for krb5_context/krb5_error_code for LwTranslateKrb5Error().
#include <krb5.h>

typedef enum
{
    KRB5_InMemory_Cache,
    KRB5_File_Cache
} Krb5CacheType;


LW_BEGIN_EXTERN_C

DWORD
LwKrb5GetDefaultRealm(
    PSTR* ppszRealm
    );

DWORD
LwKrb5GetDefaultCachePath(
    OUT PSTR* ppszCachePath
    );

DWORD
LwKrb5GetUserCachePath(
    uid_t         uid,
    Krb5CacheType cacheType,
    PSTR*         ppszCachePath
    );

#define LW_MSDS_DES_CBC_CRC             0x00000001
#define LW_MSDS_DES_CBC_MD5             0x00000002
#define LW_MSDS_RC4_HMAC                0x00000004
#define LW_MSDS_AES128_CTS_HMAC_SHA1_96 0x00000008
#define LW_MSDS_AES256_CTS_HMAC_SHA1_96 0x00000010

#define LW_MSDS_AES128_CTS  LW_MSDS_AES128_CTS_HMAC_SHA1_96
#define LW_MSDS_AES256_CTS  LW_MSDS_AES256_CTS_HMAC_SHA1_96

DWORD
LwKrb5GetSupportedEncryptionTypes(
    OUT PDWORD pdwSupportedEncryptionTypes
);
		
DWORD
LwKrb5SetThreadDefaultCachePath(
    IN PCSTR pszCachePath,
    OUT OPTIONAL PSTR* ppszPreviousCachePath
    );

DWORD
LwKrb5SetProcessDefaultCachePath(
    PCSTR pszCachePath
    );

DWORD
LwKrb5InitializeCredentials(
    IN PCSTR pszUserPrincipalName,
    IN PCSTR pszPassword,
    IN PCSTR pszCachePath,
    OUT OPTIONAL PDWORD pdwGoodUntilTime
    );

DWORD
LwKrb5GetTgt(
    PCSTR  pszUserPrincipal,
    PCSTR  pszPassword,
    PCSTR  pszCcPath,
    PDWORD pdwGoodUntilTime
    );

DWORD
LwKrb5VerifySmartCardUserPin(
    PCSTR pszUserPrincipalName, 
    PCSTR pszPIN
    );

DWORD
LwKrb5GetTgtWithSmartCard(
    PCSTR  pszUserPrincipal,
    PCSTR  pszPassword,
    PCSTR  pszCcPath,
    PDWORD pdwGoodUntilTime
    );

DWORD
LwKrb5DestroyCache(
    IN PCSTR pszCcPath
    );

#define LW_KRB5_ERROR_TO_LW_ERROR(Error, Context) \
    ((Error) ? \
     LwTranslateKrb5Error(Context, Error, __FUNCTION__, __FILE__, __LINE__) : \
     0)

DWORD
LwMapKrb5ErrorToLwError(
    krb5_error_code krbError
    );

DWORD
LwTranslateKrb5Error(
    krb5_context ctx,
    krb5_error_code krbError,
    PCSTR pszFunction,
    PCSTR pszFile,
    DWORD dwLine
    );

typedef DWORD LW_KRB5_LOGIN_FLAGS, *PLW_KRB5_LOGIN_FLAGS;

#define LW_KRB5_LOGIN_FLAG_SMART_CARD      0x00000001
#define LW_KRB5_LOGIN_FLAG_UPDATE_CACHE    0x00000002

DWORD
LwKrb5InitializeUserLoginCredentials(
    IN PCSTR pszUserPrincipalName,
    IN PCSTR pszPassword,
    IN uid_t uid,
    IN gid_t gid,
    IN LW_KRB5_LOGIN_FLAGS Flags,
    IN PCSTR pszServicePrincipal,
    IN PCSTR pszSaltPrincipal,
    IN PCSTR pszServiceRealm,
    IN PCSTR pszServicePassword,
    OUT PVOID* ppNdrPacInfo,
    OUT size_t* pNdrPacInfoSize,
    OUT PDWORD pdwGoodUntilTime
    );

DWORD
LwKrb5CheckInitiatorCreds(
    IN PCSTR pszTargetPrincipalName,
    OUT PBOOLEAN pbNeedCredentials
    );

/**
 * @brief Find the PAC data in the supplied decoded and decrypted ticket.
 *
 * @param ctx
 * @param pTgsTicket TGS ticket
 * @param serviceKey key used to originally encrypt the TGS ticket
 * @param ppchLogonInfo pointer to an RPC encoded PAC structure, NULL on error
 * @param psLogonInfo the size of RPC encoded PAC structure, may be non-zero
 *          on error
 *
 * @return LW_ERROR_SUCCESS on success, various LW errors on failure
 */
DWORD
LwKrb5FindPac(
    krb5_context ctx,
    const krb5_ticket *pTgsTicket,
    const krb5_keyblock *serviceKey,
    OUT PVOID* ppchLogonInfo,
    OUT size_t* psLogonInfo
    );

// Flags for use in LwKrb5GroupMembershipFromPac
#define LW_KRB5_PAC_INCLUDE_USER_SID             0x00000001
#define LW_KRB5_PAC_INCLUDE_PRIMARY_GROUP        0x00000002
#define LW_KRB5_PAC_INCLUDE_AUTHENTICATED_USERS  0x00000004

/**
 * @brief Extract the Group Membership SIDs from the PAC.
 *
 * @param PAC blob
 * @param dwFlags flags to determine what extra SIDs to include in the list
 * @param pdwMembershipCount number of SIDs returned in pppszMembershipList
 * @param pppszMembershipList array of SID strings
 *
 * @return c
 */
DWORD
LwKrb5GroupMembershipFromPac(
    IN PVOID pchLogonInfo,
    IN DWORD dwFlags,
    OUT PDWORD pdwMembershipCount,
    OUT PSTR** pppszMembershipList
    );

/**
 * @brief Use the credentials associated with the Credential Cache to determine the PAC
 * 
 * @param pszCachePath Location of the krb5 Credential Cache
 * @param ppchLogonInfo PAC blob to return
 * @param psLogonInfo Size of the PAC blob returned
 * 
 * @return pppszMembershipList
 */
DWORD
LwKrb5GetPACForCredentialCache(
    IN PCSTR  pszCachePath,
    OUT PVOID* ppchLogonInfo,
    OUT size_t* psLogonInfo
    );

LW_END_EXTERN_C

#endif /* __LWKRB5_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
