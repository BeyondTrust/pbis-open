/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
#ifndef __ONLINE_H__
#define __ONLINE_H__

#define PRIMARY_GROUP_EXPIRATION    (void *)0
#define STANDARD_GROUP_EXPIRATION   (void *)1
#define PAC_GROUP_EXPIRATION        (void *)2

typedef DWORD (*LSA_AD_CACHEDB_FIND_OBJECTS_BY_LIST_CALLBACK)(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    );

typedef DWORD (*LSA_AD_LDAP_FIND_OBJECTS_BY_LIST_BATCHED_CALLBACK)(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN PSTR* ppszList,
    OUT PDWORD pdwCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

//
// The LSA_REMAP_FIND_<TYPE>_BY_<INDEX>_ERROR() macros are used
// in the AD_{Online,Offline}Find<TYPE>by<INDEX> functions to do
// the error code remapping required by the LSASS SRV API layer
// and required for handling offline errors that occur in online
// operation.  They also provide uniformity in logging.  The latter
// (logging) is the reason for doing these as macros instead of
// functions (i.e., so we can call logging macros and preserve
// stack location information).
//
// In the offline case, we always remap the error to the desired error.
//
// In the online case, we must preserve the "domain is offline" error
// so that the calling code will retry by calling the offline code.
//

#define _LSA_REMAP_FIND_X_BY_Y_ERROR(ErrorVariable, IsOfflineCode, DesiredError, ObjectTypeString, IndexString, IndexFormatString, IndexValue) \
    do { \
        if ((ErrorVariable) != (DesiredError)) \
        { \
            LSA_LOG_DEBUG("Failed to find " ObjectTypeString " " IndexString " " IndexFormatString " (error = %u)", \
                          IndexValue, ErrorVariable); \
            if ((IsOfflineCode) || (LW_ERROR_DOMAIN_IS_OFFLINE != (ErrorVariable))) \
            { \
                (ErrorVariable) = (DesiredError); \
            } \
        } \
    } while (0)

#define _LSA_REMAP_FIND_X_BY_ID_ERROR(ErrorVariable, IsOfflineCode, DesiredError, ObjectTypeString, IdValue) \
    _LSA_REMAP_FIND_X_BY_Y_ERROR(ErrorVariable, \
                                 IsOfflineCode, \
                                 DesiredError, \
                                 ObjectTypeString, \
                                 "id", \
                                 "%lu", \
                                 (unsigned long)(IdValue))

#define _LSA_REMAP_FIND_X_BY_NAME_ERROR(ErrorVariable, IsOfflineCode, DesiredError, ObjectTypeString, NameValue) \
    _LSA_REMAP_FIND_X_BY_Y_ERROR(ErrorVariable, \
                                 IsOfflineCode, \
                                 DesiredError, \
                                 ObjectTypeString, \
                                 "name", \
                                 "'%s'", \
                                 LSA_SAFE_LOG_STRING(NameValue))


#define LSA_REMAP_FIND_USER_BY_ID_ERROR(ErrorVariable, IsOfflineCode, IdValue) \
    _LSA_REMAP_FIND_X_BY_ID_ERROR(ErrorVariable, \
                                  IsOfflineCode, \
                                  LW_ERROR_NO_SUCH_USER, \
                                  "user", \
                                  IdValue)

#define LSA_REMAP_FIND_USER_BY_NAME_ERROR(ErrorVariable, IsOfflineCode, NameValue) \
    _LSA_REMAP_FIND_X_BY_NAME_ERROR(ErrorVariable, \
                                    IsOfflineCode, \
                                    LW_ERROR_NO_SUCH_USER, \
                                    "user", \
                                    NameValue)

#define LSA_REMAP_FIND_GROUP_BY_ID_ERROR(ErrorVariable, IsOfflineCode, IdValue) \
    _LSA_REMAP_FIND_X_BY_ID_ERROR(ErrorVariable, \
                                  IsOfflineCode, \
                                  LW_ERROR_NO_SUCH_GROUP, \
                                  "group", \
                                  IdValue)

#define LSA_REMAP_FIND_GROUP_BY_NAME_ERROR(ErrorVariable, IsOfflineCode, NameValue) \
    _LSA_REMAP_FIND_X_BY_NAME_ERROR(ErrorVariable, \
                                    IsOfflineCode, \
                                    LW_ERROR_NO_SUCH_GROUP, \
                                    "group", \
                                    NameValue)

DWORD
AD_AddAllowedGroup(
    PCSTR    pszGroupname
    );

DWORD
AD_OnlineFindCellDN(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszComputerDN,
    IN PCSTR pszRootDN,
    OUT PSTR* ppszCellDN
    );

DWORD
AD_OnlineInitializeOperatingMode(
    OUT PAD_PROVIDER_DATA* ppProviderData,
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszDomain,
    IN PCSTR pszHostName
    );

DWORD
AD_DetermineTrustModeandDomainName(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszDomain,
    OUT OPTIONAL LSA_TRUST_DIRECTION* pdwTrustDirection,
    OUT OPTIONAL LSA_TRUST_MODE* pdwTrustMode,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName
    );

DWORD
AD_OnlineAuthenticateUserPam(
    PAD_PROVIDER_CONTEXT pContext,
    LSA_AUTH_USER_PAM_PARAMS* pParams,
    PLSA_AUTH_USER_PAM_INFO* ppPamAuthInfo
    );

DWORD
AD_OnlineCheckUserPassword(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN PCSTR  pszPassword,
    OUT PDWORD pdwGoodUntilTime,
    OUT OPTIONAL PLSA_SECURITY_OBJECT *ppUpdatedUserInfo
    );

DWORD
AD_CrackDomainQualifiedName(
    PCSTR pszId,
    PLSA_LOGIN_NAME_INFO* ppNameInfo
    );

DWORD
AD_OnlineChangePassword(
    PAD_PROVIDER_CONTEXT pContext,
    PCSTR pszUserName,
    PCSTR pszPassword,
    PCSTR pszOldPassword
    );

DWORD
AD_CreateHomeDirectory(
    PLSA_AD_PROVIDER_STATE pState,
    PLSA_SECURITY_OBJECT pObject
    );

DWORD
AD_CreateHomeDirectory_Generic(
    PLSA_AD_PROVIDER_STATE pState,
    PLSA_SECURITY_OBJECT pObject
    );

DWORD
AD_ProvisionHomeDir(
    PLSA_AD_PROVIDER_STATE pState,
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszHomedirPath
    );

DWORD
AD_CreateK5Login(
    PLSA_AD_PROVIDER_STATE pState,
    PLSA_SECURITY_OBJECT pObject
    );

DWORD
AD_CheckExpiredObject(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN OUT PLSA_SECURITY_OBJECT* ppCachedUser
    );

DWORD
AD_StoreAsExpiredObject(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN OUT PLSA_SECURITY_OBJECT* ppCachedUser
    );

int
AD_CompareObjectSids(
    IN PCVOID pObjectA,
    IN PCVOID pObjectB
    );

size_t
AD_HashObjectSid(
    IN PCVOID pObject
    );

void
AD_FreeHashObject(
    IN OUT const LW_HASH_ENTRY *pEntry
    );

DWORD
AD_FindObjectsByDNList(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN size_t sCount,
    IN PSTR* ppszDNList,
    OUT OPTIONAL size_t* psResultsCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    );

DWORD
AD_FindObjectByNameTypeNoCache(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszName,
    IN ADLogInNameType NameType,
    IN LSA_OBJECT_TYPE AccountType,
    OUT PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
AD_FindObjectByIdTypeNoCache(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN DWORD dwId,
    IN LSA_OBJECT_TYPE AccountType,
    OUT PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
AD_FindObjectBySid(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszSid,
    OUT PLSA_SECURITY_OBJECT* ppResult
    );

DWORD
AD_FindObjectsBySidList(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT OPTIONAL size_t* psResultsCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    );

DWORD
AD_GetLinkedCellInfo(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszCellDN,
    IN PCSTR pszDomain,
    OUT PDLINKEDLIST* ppCellList
    );

DWORD
AD_CacheGroupMembershipFromPac(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_TRUST_DIRECTION dwTrustDirection,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN PAC_LOGON_INFO* pPac
    );

DWORD
AD_CacheUserRealInfoFromPac(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN OUT PLSA_SECURITY_OBJECT pUserInfo,
    IN PAC_LOGON_INFO* pPac
    );

void
AD_FilterNullEntries(
    IN OUT PLSA_SECURITY_OBJECT* ppEntries,
    IN OUT size_t* psCount
    );

DWORD
AD_OnlineFindNSSArtefactByKey(
    PAD_PROVIDER_CONTEXT pContext,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    );

DWORD
AD_OnlineEnumNSSArtefacts(
    PAD_PROVIDER_CONTEXT pContext,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    );

DWORD
AD_GetCachedPasswordHash(
    IN PCSTR pszSamAccount,
    IN PCSTR pszPassword,
    OUT PBYTE *ppbHash
    );

DWORD
AD_VerifyUserAccountCanLogin(
    PAD_PROVIDER_CONTEXT pContext,
    IN PLSA_SECURITY_OBJECT pUserInfo
    );

DWORD
AD_FindObjectsByList(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_AD_CACHEDB_FIND_OBJECTS_BY_LIST_CALLBACK pFindInCacheCallback,
    IN LSA_AD_LDAP_FIND_OBJECTS_BY_LIST_BATCHED_CALLBACK pFindByListBatchedCallback,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN size_t sCount,
    IN PSTR* ppszList,
    OUT OPTIONAL size_t* psResultsCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    );

DWORD
AD_OnlineFindObjects(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );
/**
 * @brief Moves hash table values into an array.
 *
 * Moves hash table values into an array, leaving the actual
 * hash entries otherwise intact.  The only valid operation
 * to do on the hash table after this is to free it.
 *
 * On error, this function may have moved some of the values.
 *
 * @param[in,out] pHash the hash table
 * @param[out] pCount returns count of value moved
 * @param[out] pppValues returns array of value pointers
 *
 * @retval LW_ERROR_SUCCESS on success
 * @retval !LW_ERROR_SUCCESS on failure
 */
DWORD
AD_MoveHashValuesToArray(
    IN OUT PLW_HASH_TABLE pHash,
    OUT PDWORD pCount,
    OUT PVOID** pppValues
    );

DWORD
AD_OnlineEnumObjects(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
AD_OnlineQueryMemberOf(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    );

DWORD
AD_OnlineGetGroupMemberSids(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid,
    OUT PDWORD pdwSidCount,
    OUT PSTR** pppszSids
    );

#endif /* __ONLINE_H__ */
