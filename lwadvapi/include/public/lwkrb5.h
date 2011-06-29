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
