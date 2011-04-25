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
 *        provider-main.c
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
#include "adprovider.h"


BOOLEAN
AD_IsOffline(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    return LsaDmIsDomainOffline(pState->hDmState, NULL);
}

DWORD
AD_OfflineAuthenticateUserPam(
    PAD_PROVIDER_CONTEXT pContext,
    LSA_AUTH_USER_PAM_PARAMS* pParams,
    PLSA_AUTH_USER_PAM_INFO* ppPamAuthInfo
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;
    PLSA_PASSWORD_VERIFIER pVerifier = NULL;
    PSTR pszEnteredPasswordVerifier = NULL;
    PBYTE pbHash = NULL;
    PSTR pszNT4UserName = NULL;
    PLSA_AUTH_USER_PAM_INFO pPamAuthInfo = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*pPamAuthInfo),
                    (PVOID*)&pPamAuthInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_FindUserObjectByName(
                pContext,
                pParams->pszLoginName,
                &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_VerifyUserAccountCanLogin(
                pContext,
                pUserInfo);
    if (dwError == LW_ERROR_PASSWORD_EXPIRED)
    {
        // Do not block users in offline mode for expired passwords, and do not
        // attempt to change them.
        dwError = 0;
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheGetPasswordVerifier(
                pContext->pState->hCacheConnection,
                pUserInfo->pszObjectSid,
                &pVerifier
                );
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GetCachedPasswordHash(
                pUserInfo->pszSamAccountName,
                pParams->pszPassword,
                &pbHash);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaByteArrayToHexStr(
                pbHash,
                16,
                &pszEnteredPasswordVerifier);
    BAIL_ON_LSA_ERROR(dwError);

    if (strcmp(pszEnteredPasswordVerifier, pVerifier->pszPasswordVerifier))
    {
        dwError = LW_ERROR_PASSWORD_MISMATCH;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
        &pszNT4UserName,
        "%s\\%s",
        pUserInfo->pszNetbiosDomainName,
        pUserInfo->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    if (pContext->pState->bIsDefault)
    {
        dwError = LsaUmAddUser(
                      pUserInfo->userInfo.uid,
                      pszNT4UserName,
                      pParams->pszPassword,
                      0);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pPamAuthInfo->bOnlineLogon = FALSE;

    *ppPamAuthInfo = pPamAuthInfo;

cleanup:
    ADCacheSafeFreeObject(&pUserInfo);
    LSA_DB_SAFE_FREE_PASSWORD_VERIFIER(pVerifier);
    LW_SECURE_FREE_STRING(pszEnteredPasswordVerifier);
    LW_SAFE_FREE_MEMORY(pbHash);
    LW_SAFE_FREE_STRING(pszNT4UserName);

    return dwError;

error:
    *ppPamAuthInfo = NULL;

    if (pPamAuthInfo)
    {
        LsaFreeAuthUserPamInfo(pPamAuthInfo);
    }

    goto cleanup;
}

DWORD
AD_OfflineEnumUsers(
    PAD_PROVIDER_CONTEXT pContext,
    HANDLE  hResume,
    DWORD   dwMaxNumUsers,
    PDWORD  pdwUsersFound,
    PVOID** pppUserInfoList
    )
{
    return LW_ERROR_NO_MORE_USERS;
}

DWORD
AD_OfflineEnumGroups(
    PAD_PROVIDER_CONTEXT pContext,
    HANDLE  hResume,
    DWORD   dwMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    return LW_ERROR_NO_MORE_GROUPS;
}

DWORD
AD_OfflineChangePassword(
    PAD_PROVIDER_CONTEXT pContext,
    PCSTR pszUserName,
    PCSTR pszPassword,
    PCSTR pszOldPassword
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
AD_OfflineFindNSSArtefactByKey(
    PAD_PROVIDER_CONTEXT pContext,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    *ppNSSArtefactInfo = NULL;

    return LW_ERROR_NO_SUCH_NSS_MAP;
}

DWORD
AD_OfflineEnumNSSArtefacts(
    PAD_PROVIDER_CONTEXT pContext,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    )
{
    return LW_ERROR_NO_MORE_GROUPS;
}

DWORD
AD_OfflineInitializeOperatingMode(
    OUT PAD_PROVIDER_DATA* ppProviderData,
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszDomain,
    IN PCSTR pszHostName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_AD_PROVIDER_STATE pState = pContext->pState;
    PAD_PROVIDER_DATA pProviderData = NULL;
    PDLINKEDLIST pDomains = NULL;
    const DLINKEDLIST* pPos = NULL;
    const LSA_DM_ENUM_DOMAIN_INFO* pDomain = NULL;

    dwError = ADState_GetDomainTrustList(
                  pState->pszDomainName,
                  &pDomains);
    BAIL_ON_LSA_ERROR(dwError);

    pPos = pDomains;
    while (pPos != NULL)
    {
        pDomain = (const LSA_DM_ENUM_DOMAIN_INFO*)pPos->pItem;

        dwError = LsaDmAddTrustedDomain(
            pState->hDmState,
            pDomain->pszDnsDomainName,
            pDomain->pszNetbiosDomainName,
            pDomain->pSid,
            pDomain->pGuid,
            pDomain->pszTrusteeDnsDomainName,
            pDomain->dwTrustFlags,
            pDomain->dwTrustType,
            pDomain->dwTrustAttributes,
            pDomain->dwTrustDirection,
            pDomain->dwTrustMode,
            IsSetFlag(pDomain->Flags, LSA_DM_DOMAIN_FLAG_TRANSITIVE_1WAY_CHILD) ? TRUE : FALSE,
            pDomain->pszForestName,
            NULL
            );
        if (IsSetFlag(pDomain->dwTrustFlags, NETR_TRUST_FLAG_PRIMARY))
        {
            BAIL_ON_LSA_ERROR(dwError);
        }

        pPos = pPos->pNext;
    }

    dwError = ADState_GetProviderData(
                  pState->pszDomainName,
                  &pProviderData);
    BAIL_ON_LSA_ERROR(dwError);

    *ppProviderData = pProviderData;

cleanup:
    ADState_FreeEnumDomainInfoList(pDomains);
    return dwError;

error:
    *ppProviderData = NULL;

    if (pProviderData)
    {
        ADProviderFreeProviderData(pProviderData);
        pProviderData = NULL;
    }

    goto cleanup;
}


static
DWORD
AD_OfflineFindObjectsByName(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;
    PSTR  pszLoginId_copy = NULL;
    PLSA_SECURITY_OBJECT pCachedUser = NULL;
    DWORD dwIndex = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_TYPE type = LSA_QUERY_TYPE_UNDEFINED;
    PSTR pszDefaultPrefix = NULL;

    dwError = AD_GetUserDomainPrefix(
                  pContext->pState,
                  &pszDefaultPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*ppObjects) * dwCount, OUT_PPVOID(&ppObjects));
    BAIL_ON_LSA_ERROR(dwError);

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        dwError = LwAllocateString(
            QueryList.ppszStrings[dwIndex],
            &pszLoginId_copy);
        BAIL_ON_LSA_ERROR(dwError);

        LwStrCharReplace(
            pszLoginId_copy,
            LsaSrvSpaceReplacement(),
            ' ');
        
        dwError = LsaSrvCrackDomainQualifiedName(
            pszLoginId_copy,
            &pUserNameInfo);
        BAIL_ON_LSA_ERROR(dwError);

        switch(pUserNameInfo->nameType)
        {
        case NameType_NT4:
            type = LSA_QUERY_TYPE_BY_NT4;
            break;
        case NameType_UPN:
            type = LSA_QUERY_TYPE_BY_UPN;
            break;
        case NameType_Alias:
            type = LSA_QUERY_TYPE_BY_ALIAS;
            break;
        }

        if (type != QueryType)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        switch(ObjectType)
        {
        case LSA_OBJECT_TYPE_USER:
            dwError = ADCacheFindUserByName(
                pContext->pState->hCacheConnection,
                pUserNameInfo,
                &pCachedUser);
            break;
        case LSA_OBJECT_TYPE_GROUP:
            dwError = ADCacheFindGroupByName(
                pContext->pState->hCacheConnection,
                pUserNameInfo,
                &pCachedUser);
            break;
        default:
            dwError = ADCacheFindUserByName(
                pContext->pState->hCacheConnection,
                pUserNameInfo,
                &pCachedUser);
            if (dwError == LW_ERROR_NO_SUCH_USER  ||
                dwError == LW_ERROR_NOT_HANDLED)
            {
                dwError = ADCacheFindGroupByName(
                    pContext->pState->hCacheConnection,
                    pUserNameInfo,
                    &pCachedUser);
            }
            break;
        }

        switch (dwError)
        {
        case LW_ERROR_SUCCESS:
            ppObjects[dwIndex] = pCachedUser;
            pCachedUser = NULL;
            break;
        case LW_ERROR_NOT_HANDLED:
        case LW_ERROR_NO_SUCH_USER:
        case LW_ERROR_NO_SUCH_GROUP:
        case LW_ERROR_NO_SUCH_OBJECT:
            dwError = LW_ERROR_SUCCESS;
            
            if (QueryType == LSA_QUERY_TYPE_BY_ALIAS &&
                AD_ShouldAssumeDefaultDomain(pContext->pState))
            {
                LW_SAFE_FREE_STRING(pszLoginId_copy);
                LsaSrvFreeNameInfo(pUserNameInfo);
                pUserNameInfo = NULL;

                dwError = LwAllocateStringPrintf(
                    &pszLoginId_copy,
                    "%s%c%s",
                    pszDefaultPrefix,
                    LsaSrvDomainSeparator(),
                    QueryList.ppszStrings[dwIndex]);
                BAIL_ON_LSA_ERROR(dwError);

                LwStrCharReplace(
                    pszLoginId_copy,
                    LsaSrvSpaceReplacement(),
                    ' ');

                dwError = LsaSrvCrackDomainQualifiedName(
                    pszLoginId_copy,
                    &pUserNameInfo);
                BAIL_ON_LSA_ERROR(dwError);

                if (ObjectType != LSA_OBJECT_TYPE_GROUP)
                {
                    dwError = ADCacheFindUserByName(
                        pContext->pState->hCacheConnection,
                        pUserNameInfo,
                        &pCachedUser);
                }
                if (ObjectType == LSA_OBJECT_TYPE_GROUP ||
                    (dwError == LW_ERROR_NO_SUCH_USER  ||
                        dwError == LW_ERROR_NOT_HANDLED))
                {
                    dwError = ADCacheFindGroupByName(
                        pContext->pState->hCacheConnection,
                        pUserNameInfo,
                        &pCachedUser);
                }
                switch (dwError)
                {
                case LW_ERROR_SUCCESS:
                    break;
                case LW_ERROR_NOT_HANDLED:
                case LW_ERROR_NO_SUCH_USER:
                case LW_ERROR_NO_SUCH_GROUP:
                case LW_ERROR_NO_SUCH_OBJECT:
                    dwError = LW_ERROR_SUCCESS;
                    break;
                default:
                    BAIL_ON_LSA_ERROR(dwError);
                }
                ppObjects[dwIndex] = pCachedUser;
            }
            break;
        default:
            BAIL_ON_LSA_ERROR(dwError);
        }

        LW_SAFE_FREE_STRING(pszLoginId_copy);
        LsaSrvFreeNameInfo(pUserNameInfo);
        pUserNameInfo = NULL;
    }

    *pppObjects = ppObjects;

cleanup:

    LW_SAFE_FREE_STRING(pszDefaultPrefix);
    LW_SAFE_FREE_STRING(pszLoginId_copy);

    if (pUserNameInfo)
    {
        LsaSrvFreeNameInfo(pUserNameInfo);
    }

    return dwError;

error:

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCount, ppObjects);
    }

    goto cleanup;
}

static
DWORD
AD_OfflineFindObjectsById(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pCachedUser = NULL;
    DWORD dwIndex = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    dwError = LwAllocateMemory(sizeof(*ppObjects) * dwCount, OUT_PPVOID(&ppObjects));
    BAIL_ON_LSA_ERROR(dwError);

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        switch(ObjectType)
        {
        case LSA_OBJECT_TYPE_USER:
            dwError = ADCacheFindUserById(
                pContext->pState->hCacheConnection,
                QueryList.pdwIds[dwIndex],
                &pCachedUser);
            break;
        case LSA_OBJECT_TYPE_GROUP:
            dwError = ADCacheFindGroupById(
                pContext->pState->hCacheConnection,
                QueryList.pdwIds[dwIndex],
                &pCachedUser);
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        switch (dwError)
        {
        case LW_ERROR_SUCCESS:
            ppObjects[dwIndex] = pCachedUser;
            pCachedUser = NULL;
            break;
        case LW_ERROR_NOT_HANDLED:
        case LW_ERROR_NO_SUCH_USER:
        case LW_ERROR_NO_SUCH_GROUP:
        case LW_ERROR_NO_SUCH_OBJECT:
            dwError = 0;
            break;
        default:
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *pppObjects = ppObjects;

cleanup:

    return dwError;

error:

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCount, ppObjects);
    }

    goto cleanup;
}

DWORD
AD_OfflineFindObjects(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_OBJECT_TYPE type = LSA_OBJECT_TYPE_UNDEFINED;
    DWORD dwIndex = 0;

    switch(QueryType)
    {
    case LSA_QUERY_TYPE_BY_SID:
        dwError = ADCacheFindObjectsBySidList(
                    pContext->pState->hCacheConnection,
                    dwCount,
                    (PSTR*) QueryList.ppszStrings,
                    &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case LSA_QUERY_TYPE_BY_DN:
        dwError = ADCacheFindObjectsByDNList(
            pContext->pState->hCacheConnection,
            dwCount,
            (PSTR*) QueryList.ppszStrings,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case LSA_QUERY_TYPE_BY_NT4:
    case LSA_QUERY_TYPE_BY_UPN:
    case LSA_QUERY_TYPE_BY_ALIAS:
        dwError = AD_OfflineFindObjectsByName(
            pContext,
            FindFlags,
            ObjectType,
            QueryType,
            dwCount,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case LSA_QUERY_TYPE_BY_UNIX_ID:
        dwError = AD_OfflineFindObjectsById(
            pContext,
            FindFlags,
            ObjectType,
            QueryType,
            dwCount,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        if (ppObjects[dwIndex])
        {
            switch (ppObjects[dwIndex]->type)
            {
            case LSA_OBJECT_TYPE_GROUP:
                type = LSA_OBJECT_TYPE_GROUP;
                break;
            case LSA_OBJECT_TYPE_USER:
                type = LSA_OBJECT_TYPE_USER;
                break;
                /*
            case LSA_OBJECT_TYPE_DOMAIN:
                type = LSA_OBJECT_TYPE_DOMAIN;
                break;
                */
            }

            if (ObjectType != LSA_OBJECT_TYPE_UNDEFINED && type != ObjectType)
            {
                LsaUtilFreeSecurityObject(ppObjects[dwIndex]);
                ppObjects[dwIndex] = NULL;
            }
        }
    }

    *pppObjects = ppObjects;

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_OfflineQueryMemberOfForSid(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN PSTR pszSid,
    IN OUT PLW_HASH_TABLE pGroupHash
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sMembershipCount = 0;
    PLSA_GROUP_MEMBERSHIP* ppMemberships = NULL;
    // Only free top level array, do not free string pointers.
    PSTR pszGroupSid = NULL;
    PLSA_SECURITY_OBJECT* ppUserObject = NULL;
    DWORD dwIndex = 0;

    dwError = AD_OfflineFindObjectsBySidList(
                  pContext->pState,
                  1,
                  &pszSid,
                  &ppUserObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (!ppUserObject[0])
    {
        dwError = LW_ERROR_NO_SUCH_USER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheGetGroupsForUser(
                    pContext->pState->hCacheConnection,
                    pszSid,
                    AD_GetTrimUserMembershipEnabled(pContext->pState),
                    &sMembershipCount,
                    &ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    for (dwIndex = 0; dwIndex < sMembershipCount; dwIndex++)
    {
        if (ppMemberships[dwIndex]->pszParentSid &&
            !LwHashExists(pGroupHash, ppMemberships[dwIndex]->pszParentSid))
        {
            dwError = LwAllocateString(
                ppMemberships[dwIndex]->pszParentSid,
                &pszGroupSid);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwError = LwHashSetValue(pGroupHash, pszGroupSid, pszGroupSid);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = AD_OfflineQueryMemberOfForSid(
                pContext,
                FindFlags,
                pszGroupSid,
                pGroupHash);
            pszGroupSid = NULL;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pszGroupSid);
    ADCacheSafeFreeObjectList(1, &ppUserObject);
    ADCacheSafeFreeGroupMembershipList(sMembershipCount, &ppMemberships);

    return dwError;

error:

    goto cleanup;
}

static
VOID
AD_OfflineFreeMemberOfHashEntry(
    const LW_HASH_ENTRY* pEntry
    )
{
    if (pEntry->pValue)
    {
        LwFreeMemory(pEntry->pValue);
    }
}

DWORD
AD_OfflineQueryMemberOf(
    PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    PLW_HASH_TABLE   pGroupHash = NULL;
    LW_HASH_ITERATOR hashIterator = {0};
    LW_HASH_ENTRY*   pHashEntry = NULL;
    DWORD dwGroupSidCount = 0;
    PSTR* ppszGroupSids = NULL;

    dwError = LwHashCreate(
        13,
        LwHashCaselessStringCompare,
        LwHashCaselessStringHash,
        AD_OfflineFreeMemberOfHashEntry,
        NULL,
        &pGroupHash);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (dwIndex = 0; dwIndex < dwSidCount; dwIndex++)
    {
        dwError = AD_OfflineQueryMemberOfForSid(
            pContext,
            FindFlags,
            ppszSids[dwIndex],
            pGroupHash);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwGroupSidCount = (DWORD) LwHashGetKeyCount(pGroupHash);
    
    if (dwGroupSidCount)
    {
        dwError = LwAllocateMemory(
            sizeof(*ppszGroupSids) * dwGroupSidCount,
            OUT_PPVOID(&ppszGroupSids));
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = LwHashGetIterator(pGroupHash, &hashIterator);
        BAIL_ON_LSA_ERROR(dwError);
        
        for(dwIndex = 0; (pHashEntry = LwHashNext(&hashIterator)) != NULL; dwIndex++)
        {
            ppszGroupSids[dwIndex] = (PSTR) pHashEntry->pValue;
            pHashEntry->pValue = NULL;
        }
    }

    *pdwGroupSidCount = dwGroupSidCount;
    *pppszGroupSids = ppszGroupSids;

cleanup:

    LwHashSafeFree(&pGroupHash);

    return dwError;

error:

    if (ppszGroupSids)
    {
        LwFreeStringArray(ppszGroupSids, dwGroupSidCount);
    }

    goto cleanup;
}

DWORD
AD_OfflineGetGroupMemberSids(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid,
    OUT PDWORD pdwSidCount,
    OUT PSTR** pppszSids
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sMembershipCount = 0;
    PLSA_GROUP_MEMBERSHIP* ppMemberships = NULL;
    // Only free top level array, do not free string pointers.
    PSTR* ppszSids = NULL;
    DWORD dwSidCount = 0;
    DWORD dwIndex = 0;

    dwError = ADCacheGetGroupMembers(
                    pContext->pState->hCacheConnection,
                    pszSid,
                    AD_GetTrimUserMembershipEnabled(pContext->pState),
                    &sMembershipCount,
                    &ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    if (sMembershipCount)
    {
        dwError = LwAllocateMemory(
            sizeof(*ppszSids) * sMembershipCount,
            OUT_PPVOID(&ppszSids));
        
        for (dwIndex = 0; dwIndex < sMembershipCount; dwIndex++)
        {
            if (ppMemberships[dwIndex]->pszChildSid)
            {
                dwError = LwAllocateString(ppMemberships[dwIndex]->pszChildSid, &ppszSids[dwSidCount++]);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    *pdwSidCount = dwSidCount;
    *pppszSids = ppszSids;

cleanup:

    ADCacheSafeFreeGroupMembershipList(sMembershipCount, &ppMemberships);
    
    return dwError;

error:

    *pdwSidCount = 0;
    *pppszSids = NULL;

    if (ppszSids)
    {
        LwFreeStringArray(ppszSids, dwSidCount);
    }

    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
