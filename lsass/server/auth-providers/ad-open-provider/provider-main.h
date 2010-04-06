/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 *        provider-main.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#ifndef __PROVIDER_MAIN_H__
#define __PROVIDER_MAIN_H__

#ifdef ENABLE_STATIC_PROVIDERS
#define LsaInitializeProvider2 LsaInitializeProvider_ActiveDirectory
#endif

DWORD
LsaInitializeProvider2(
    OUT PCSTR* ppszProviderName,
    OUT PLSA_PROVIDER_FUNCTION_TABLE_2* ppFunctionTable
    );

DWORD
AD_ShutdownProvider(
    VOID
    );

DWORD
AD_OpenHandle(
    HANDLE hServer,
    PHANDLE phProvider
    );

void
AD_CloseHandle(
    HANDLE hProvider
    );

BOOLEAN
AD_ServicesDomain(
    PCSTR pszDomain
    );

DWORD
AD_ServicesDomainWithDiscovery(
    IN PCSTR pszNetBiosName,
    OUT PBOOLEAN pbFoundDomain
    );

DWORD
AD_AuthenticateUser(
    HANDLE hProvider,
    PCSTR  pszUserName,
    PCSTR  pszPassword,
    PSTR*  ppszMessage
    );

DWORD
AD_AuthenticateUserEx(
    HANDLE hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUSerInfo
    );

DWORD
AD_ValidateUser(
    HANDLE hProvider,
    PCSTR  pszUserName,
    PCSTR  pszPassword
    );

DWORD
AD_CheckUserInList(
    HANDLE hProvider,
    PCSTR  pszUserName,
    PCSTR  pszListName
    );

DWORD
AD_FindUserObjectById(
    IN HANDLE  hProvider,
    IN uid_t uid,
    OUT PLSA_SECURITY_OBJECT* ppResult
    );

DWORD
AD_EnumUsersFromCache(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    );

DWORD
AD_RemoveUserByNameFromCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID,
    IN PCSTR  pszLoginId
    );

DWORD
AD_RemoveUserByIdFromCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID,
    IN uid_t  uid
    );

DWORD
AD_GetUserGroupObjectMembership(
    IN HANDLE hProvider,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN BOOLEAN bIsCacheOnlyMode,
    OUT size_t* psNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppResult
    );

DWORD
AD_EnumGroupsFromCache(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    );

DWORD
AD_RemoveGroupByNameFromCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID,
    IN PCSTR  pszGroupName
    );

DWORD
AD_RemoveGroupByIdFromCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID,
    IN gid_t  gid
    );

DWORD
AD_ChangePassword(
    HANDLE hProvider,
    PCSTR pszUserName,
    PCSTR pszPassword,
    PCSTR pszOldPassword
    );

DWORD
AD_SetPassword(
    HANDLE hProvider,
    PCSTR pszLoginId,
    PCSTR pszPassword
    );

DWORD
AD_AddUser(
    HANDLE hProvider,
    PLSA_USER_ADD_INFO pInfo
    );

DWORD
AD_ModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO_2 pUserModInfo
    );

DWORD
AD_DeleteObject(
    HANDLE hProvider,
    PCSTR pszSid
    );

DWORD
AD_AddGroup(
    HANDLE hProvider,
    PLSA_GROUP_ADD_INFO pInfo
    );

DWORD
AD_ModifyGroup(
    HANDLE hProvider,
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo
    );

DWORD
AD_EmptyCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID
    );

DWORD
AD_OpenSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    );

DWORD
AD_CloseSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    );

DWORD
AD_FindNSSArtefactByKey(
    HANDLE hProvider,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    );

DWORD
AD_BeginEnumNSSArtefacts(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PHANDLE phResume
    );

DWORD
AD_EnumNSSArtefacts(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    );

VOID
AD_EndEnumNSSArtefacts(
    HANDLE hProvider,
    HANDLE hResume
    );

DWORD
AD_GetStatus(
    HANDLE hProvider,
    PLSA_AUTH_PROVIDER_STATUS* ppProviderStatus
    );

DWORD
AD_GetTrustedDomainInfo(
    PLSA_TRUSTED_DOMAIN_INFO* ppDomainInfoArray,
    PDWORD pdwNumTrustedDomains
    );

VOID
AD_FreeTrustedDomainsInList(
    PVOID pItem,
    PVOID pUserData
    );

DWORD
AD_FillTrustedDomainInfo(
    IN PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo,
    OUT PLSA_TRUSTED_DOMAIN_INFO pTrustedDomainInfo
    );

DWORD
AD_BuildDCInfo(
    PLSA_DM_DC_INFO pDCInfo,
    PLSA_DC_INFO*   ppDCInfo
    );

VOID
AD_FreeStatus(
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus
    );

DWORD
AD_RefreshConfiguration(
    HANDLE hProvider
    );

DWORD
AD_ProviderIoControl(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwIoControlCode,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    );

DWORD
AD_FindUserObjectByName(
    IN HANDLE  hProvider,
    IN PCSTR   pszLoginId,
    OUT PLSA_SECURITY_OBJECT* ppResult
    );

DWORD
AD_FindGroupObjectByName(
    IN HANDLE  hProvider,
    IN PCSTR   pszGroupName,
    OUT PLSA_SECURITY_OBJECT* ppResult
    );

DWORD
AD_InitializeOperatingMode(
    IN PCSTR pszDomain,
    IN PCSTR pszHostName,
    IN BOOLEAN bIsDomainOffline
    );

DWORD
AD_MachineCredentialsCacheInitialize(
    VOID
    );

DWORD
AD_GetExpandedGroupUsers(
    IN HANDLE hProvider,
    IN PCSTR pszDomainName,
    IN PCSTR pszGroupSid,
    IN BOOLEAN bIsCacheOnlyMode,
    IN int iMaxDepth,
    OUT BOOLEAN* pbAllExpanded,
    OUT size_t* psCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    );

DWORD
AD_FindObjects(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
AD_OpenEnumObjects(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    );

DWORD
AD_EnumObjects(
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
AD_OpenEnumMembers(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    );

DWORD
AD_EnumMembers(
    IN HANDLE hEnum,
    IN DWORD dwMaxMemberSidCount,
    OUT PDWORD pdwMemberSidCount,
    OUT PSTR** pppszMemberSids
    );

DWORD
AD_QueryMemberOf(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    );

VOID
AD_CloseEnum(
    IN OUT HANDLE hEnum
    );

DWORD
AD_UpdateObject(
    IN OUT PLSA_SECURITY_OBJECT pObject
    );

#endif /* __PROVIDER_MAIN_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
