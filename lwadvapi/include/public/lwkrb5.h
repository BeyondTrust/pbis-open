/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 *        lwkrb5.h
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi)
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

// TODO: find out if these includes are necessary
//#include <uuid/uuid.h>
//#include <lwrpc/krb5pac.h>
//#include <lwio/lwio.h>
#include <krb5.h>

#include <lwdef.h>

typedef enum
{
    KRB5_InMemory_Cache,
    KRB5_File_Cache
} Krb5CacheType;

typedef BOOLEAN (*LW_KRB5_REALM_IS_OFFLINE_CALLBACK)(IN PCSTR pszRealmName);
typedef VOID (*LW_KRB5_REALM_TRANSITION_OFFLINE_CALLBACK)(IN PCSTR pszRealmName);


LW_BEGIN_EXTERN_C

DWORD
LwKrb5GetDefaultRealm(
    PSTR* ppszRealm
    );

DWORD
LwKrb5GetSystemCachePath(
    PSTR*         ppszCachePath
    );

DWORD
LwKrb5GetUserCachePath(
    uid_t         uid,
    Krb5CacheType cacheType,
    PSTR*         ppszCachePath
    );

DWORD
LwKrb5SetDefaultCachePath(
    PCSTR pszCachePath,
    PSTR* ppszOriginalCachePath
    );

DWORD
LwKrb5SetProcessDefaultCachePath(
    PCSTR pszCachePath
    );

DWORD
LwSetupMachineSession(
    PCSTR  pszSamAccountName,
    PCSTR  pszPassword,
    PCSTR  pszRealm,
    PCSTR  pszIgnoredDomain,
    PDWORD pdwGoodUntilTime
    );

DWORD
LwSetupMachineSessionWithCache(
    PCSTR  pszSamAccountName,
    PCSTR  pszPassword,
    PCSTR  pszRealm,
    PCSTR  pszIgnoredDomain,
    PCSTR  pszCachePath,
    PDWORD pdwGoodUntilTime
    );

DWORD
LwKrb5CleanupMachineSession(
    VOID
    );

DWORD
LwKrb5GetTgt(
    PCSTR  pszUserPrincipal,
    PCSTR  pszPassword,
    PCSTR  pszCcPath,
    PDWORD pdwGoodUntilTime
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

DWORD
LwKrb5GetServiceTicketForUser(
    uid_t         uid,
    PCSTR         pszUserPrincipal,
    PCSTR         pszServername,
    PCSTR         pszDomain,
    Krb5CacheType cacheType
    );

DWORD
LwTranslateKrb5Error(
    krb5_context ctx,
    krb5_error_code krbError,
    PCSTR pszFunction,
    PCSTR pszFile,
    DWORD dwLine
    );

DWORD
LwSetupUserLoginSession(
    uid_t uid,
    gid_t gid,
    PCSTR pszUsername,
    PCSTR pszPassword,
    BOOLEAN bUpdateUserCache,
    PCSTR pszServicePrincipal,
    PCSTR pszServiceRealm,
    PCSTR pszServicePassword,
    char** ppchLogonInfo,
    size_t* psLogonInfo,
    PDWORD pdwGoodUntilTime,
    DWORD dwFlags
    );

#define LW_USER_LOGIN_SESSION_FLAG_SMART_CARD   0x00000001

DWORD
LwKrb5CheckInitiatorCreds(
    IN PCSTR pszTargetPrincipalName,
    OUT PBOOLEAN pbNeedCredentials
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
