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
 *        provider-main.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#ifndef __OFFLINE_H__
#define __OFFLINE_H__

BOOLEAN
AD_IsOffline(
    PLSA_AD_PROVIDER_STATE pState
    );

DWORD
AD_OfflineAuthenticateUserPam(
    PAD_PROVIDER_CONTEXT pContext,
    LSA_AUTH_USER_PAM_PARAMS* pParams,
    PLSA_AUTH_USER_PAM_INFO* ppPamAuthInfo
    );

DWORD
AD_OfflineEnumUsers(
    PAD_PROVIDER_CONTEXT pContext,
    HANDLE  hResume,
    DWORD   dwMaxNumUsers,
    PDWORD  pdwUsersFound,
    PVOID** pppUserInfoList
    );

DWORD
AD_OfflineEnumGroups(
    PAD_PROVIDER_CONTEXT pContext,
    HANDLE  hResume,
    DWORD   dwMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
AD_OfflineChangePassword(
    PAD_PROVIDER_CONTEXT pContext,
    PCSTR pszUserName,
    PCSTR pszPassword,
    PCSTR pszOldPassword
    );

DWORD
AD_OfflineFindNSSArtefactByKey(
    PAD_PROVIDER_CONTEXT pContext,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    );

DWORD
AD_OfflineEnumNSSArtefacts(
    PAD_PROVIDER_CONTEXT pContext,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    );

DWORD
AD_OfflineInitializeOperatingMode(
    OUT PAD_PROVIDER_DATA* ppProviderData,
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszDomain,
    IN PCSTR pszHostName
    );

DWORD
AD_OfflineFindObjects(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
AD_OfflineQueryMemberOf(
    PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    );

DWORD
AD_OfflineGetGroupMemberSids(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid,
    OUT PDWORD pdwSidCount,
    OUT PSTR** pppszSids
    );

#endif /* __OFFLINE_H__ */
