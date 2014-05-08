/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        adnetapi.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Wrappers for calls to NETAPI
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __ADNETAPI_H__
#define __ADNETAPI_H__

#include <uuid/uuid.h>
#include <openssl/rc4.h>

#include <lw/ntstatus.h>
#include <lw/winerror.h>
#include <lw/rpc/lsa.h>
#include <lw/rpc/netlogon.h>
#include <lw/rpc/samr.h>

typedef struct _LSA_TRANSLATED_NAME_OR_SID {
    PSTR pszNT4NameOrSid;
    LSA_OBJECT_TYPE ObjectType;
} LSA_TRANSLATED_NAME_OR_SID, *PLSA_TRANSLATED_NAME_OR_SID;

typedef struct _LSA_SCHANNEL_STATE* PLSA_SCHANNEL_STATE;

DWORD
AD_SetSystemAccess(
    IN PLSA_AD_PROVIDER_STATE pState,
    OUT OPTIONAL LW_PIO_CREDS* ppOldToken
    );

DWORD
AD_NetCreateSchannelState(
    OUT PLSA_SCHANNEL_STATE* ppSchannelState
    );

VOID
AD_NetDestroySchannelState(
    IN PLSA_SCHANNEL_STATE pSchannelState
    );

DWORD
AD_NetUserChangePassword(
    PCSTR pszDomainName,
    PCSTR pszLoginId,
    PCSTR pszUserPrincipalName,
    PCSTR pszOldPassword,
    PCSTR pszNewPassword
    );

DWORD
AD_NetLookupObjectSidByName(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszHostname,
    IN PCSTR pszObjectName,
    OUT PSTR* ppszObjectSid,
    OUT LSA_OBJECT_TYPE* pObjectType,
    OUT PBOOLEAN pbIsNetworkError
    );

DWORD
AD_NetLookupObjectSidsByNames(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszHostname,
    IN DWORD dwNamesCount,
    IN PSTR* ppszNames,
    OUT PLSA_TRANSLATED_NAME_OR_SID** pppTranslatedSids,
    OUT OPTIONAL PDWORD pdwFoundSidsCount,
    OUT PBOOLEAN pbIsNetworkError
    );

DWORD
AD_NetLookupObjectNameBySid(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszHostname,
    IN PCSTR pszObjectSid,
    OUT PSTR* ppszNT4Name,
    OUT LSA_OBJECT_TYPE* pObjectType,
    OUT PBOOLEAN pbIsNetworkError
    );

DWORD
AD_NetLookupObjectNamesBySids(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszHostname,
    IN DWORD dwSidsCount,
    IN PSTR* ppszObjectSids,
    OUT PLSA_TRANSLATED_NAME_OR_SID** pppTranslatedNames,
    OUT OPTIONAL PDWORD pdwFoundNamesCount,
    OUT PBOOLEAN pbIsNetworkError
    );

DWORD
AD_DsEnumerateDomainTrusts(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszDomainControllerName,
    IN DWORD dwFlags,
    OUT NetrDomainTrust** ppTrusts,
    OUT PDWORD pdwCount,
    OUT OPTIONAL PBOOLEAN pbIsNetworkError
    );

VOID
AD_FreeDomainTrusts(
    IN OUT NetrDomainTrust** ppTrusts
    );

DWORD
AD_DsGetDcName(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszServerName,
    IN PCSTR pszDomainName,
    IN BOOLEAN bReturnDnsName,
    OUT PSTR* ppszDomainDnsOrFlatName,
    OUT PSTR* ppszDomainForestDnsName,
    OUT OPTIONAL PBOOLEAN pbIsNetworkError
    );

DWORD
AD_MapNetApiError(
    DWORD dwADError
    );

void
LsaFreeTranslatedNameInfo(
    IN OUT PLSA_TRANSLATED_NAME_OR_SID pNameInfo
    );

void
LsaFreeTranslatedNameList(
    IN OUT PLSA_TRANSLATED_NAME_OR_SID* pNameList,
    IN DWORD dwNumNames
    );

DWORD
AD_NetlogonAuthenticationUserEx(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PSTR pszDomainController,
    IN PLSA_AUTH_USER_PARAMS pUserParams,
    OUT PLSA_AUTH_USER_INFO *ppUserInfo,
    OUT PBOOLEAN pbIsNetworkError
    );

INT64
WinTimeToInt64(
    WinNtTime WinTime
    );

#endif /* __ADNETAPI_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
