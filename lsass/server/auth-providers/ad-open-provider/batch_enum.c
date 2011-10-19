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
 *        batch_enum.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "adprovider.h"
#include "batch_common.h"
#include "batch_gather.h"
#include "batch_marshal.h"

typedef struct __AD_CELL_COOKIE_DATA
{
    // Initially, this is set to NULL to indicate that the primary cell
    // should be searched.
    const DLINKEDLIST* pCurrentCell;
    PLSA_DM_LDAP_CONNECTION pLdapConn;
    LW_SEARCH_COOKIE LdapCookie;

    // This hash table is used to ensure that the same user/group is not
    // returned twice through linked cells. It is only allocated if linked
    // cells are used in the computer's cell.
    PLSA_HASH_TABLE pEnumeratedSids;
} AD_CELL_COOKIE_DATA, *PAD_CELL_COOKIE_DATA;

static
VOID
LsaAdBatchFreeCellCookie(
    IN OUT PVOID pvCookie
    )
{
    PAD_CELL_COOKIE_DATA pData = (PAD_CELL_COOKIE_DATA)pvCookie;

    if (pData != NULL)
    {
        LwFreeCookieContents(&pData->LdapCookie);
        LsaDmLdapClose(pData->pLdapConn);
        LsaHashSafeFree(&pData->pEnumeratedSids);

        LW_SAFE_FREE_MEMORY(pData);
    }
}

static
DWORD
LsaAdBatchEnumGetNoMoreError(
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType
    )
{
    DWORD dwError = 0;

    switch (ObjectType)
    {
        case LSA_AD_BATCH_OBJECT_TYPE_USER:
            dwError = LW_ERROR_NO_MORE_USERS;
            break;
        case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
            dwError = LW_ERROR_NO_MORE_GROUPS;
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INTERNAL;
    }

    return dwError;
}

static
DWORD
LsaAdBatchEnumGetScopeRoot(
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN BOOLEAN bIsByRealObject,
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszCellDn,
    OUT PSTR* ppszScopeRoot
    )
{
    DWORD dwError = 0;
    PSTR pszScopeRoot = NULL;

    if (bIsByRealObject)
    {
        dwError = LwLdapConvertDomainToDN(pszDnsDomainName, &pszScopeRoot);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        PCSTR pszContainer = NULL;
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszContainer = "Users";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszContainer = "Groups";
                break;
            default:
                LSA_ASSERT(FALSE);
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LwAllocateStringPrintf(
                        &pszScopeRoot,
                        "CN=%s,%s",
                        pszContainer,
                        pszCellDn);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppszScopeRoot = pszScopeRoot;
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszScopeRoot);
    goto cleanup;
}


static
PCSTR
LsaAdBatchEnumGetRealQuery(
    IN BOOLEAN bIsSchemaMode,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType
    )
{
    PCSTR pszQuery = NULL;

    if (bIsSchemaMode)
    {
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszQuery = "(&(objectClass=User)(!(objectClass=Computer))(sAMAccountName=*)(uidNumber=*))";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszQuery = "(&(objectClass=Group)(!(objectClass=Computer))(sAMAccountName=*)(gidNumber=*))";
                break;
        }
    }
    else
    {
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszQuery = "(&(objectClass=User)(!(objectClass=Computer))(sAMAccountName=*))";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszQuery = "(&(objectClass=Group)(!(objectClass=Computer))(sAMAccountName=*))";
                break;
        }
    }

    return pszQuery;
}

static
PCSTR
LsaAdBatchEnumGetPseudoQuery(
    IN BOOLEAN bIsSchemaMode,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType
    )
{
    PCSTR pszQuery = NULL;

    if (bIsSchemaMode)
    {
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszQuery = "(&(objectClass=posixAccount)(keywords=objectClass=centerisLikewiseUser)(uidNumber=*))";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszQuery = "(&(objectClass=posixGroup)(keywords=objectClass=centerisLikewiseGroup)(gidNumber=*))";
                break;
        }
    }
    else
    {
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszQuery = "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseUser)(keywords=uidNumber=*))";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszQuery = "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseGroup)(keywords=gidNumber=*))";
                break;
        }
    }

    return pszQuery;
}

static
PCSTR
LsaAdBatchEnumGetQuery(
    IN BOOLEAN bIsByRealObject,
    IN BOOLEAN bIsSchemaMode,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType
    )
{
    PCSTR pszQuery = NULL;

    if (bIsByRealObject)
    {
        pszQuery = LsaAdBatchEnumGetRealQuery(bIsSchemaMode, ObjectType);
    }
    else
    {
        pszQuery = LsaAdBatchEnumGetPseudoQuery(bIsSchemaMode, ObjectType);
    }

    return pszQuery;
}

static
DWORD
LsaAdBatchEnumProcessRealMessages(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN DWORD dwDirectoryMode,
    IN ADConfigurationMode adMode,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessages,
    IN DWORD dwCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    LDAPMessage* pCurrentMessage = NULL;
    LDAP* pLd = LwLdapGetSession(hDirectory);
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwObjectsCount = 0;
    LSA_AD_BATCH_ITEM batchItem = { { 0 }, 0 };

    dwError = LwAllocateMemory(
                    dwCount * sizeof(*ppObjects),
                    (PVOID*)&ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    pCurrentMessage = ldap_first_entry(pLd, pMessages);
    while (pCurrentMessage)
    {
        dwError = LsaAdBatchGatherRealObjectInternal(
                        &batchItem,
                        &dwDirectoryMode,
                        &adMode,
                        ObjectType,
                        NULL,
                        hDirectory,
                        pCurrentMessage);
        BAIL_ON_LSA_ERROR(dwError);

        //
        // Need to discard special groups (such as BUILTIN)
        // that can show up in domain group enumeration.
        //
        if (!AdIsSpecialDomainSidPrefix(batchItem.pszSid))
        {
            dwError = LsaAdBatchMarshal(
                            pszDnsDomainName,
                            pszNetbiosDomainName,
                            &batchItem,
                            &ppObjects[dwObjectsCount]);
            BAIL_ON_LSA_ERROR(dwError);
            
            if (ppObjects[dwObjectsCount])
            {
                dwObjectsCount++;
            }
        }

        LsaAdBatchDestroyBatchItemContents(&batchItem);

        pCurrentMessage = ldap_next_entry(pLd, pCurrentMessage);
    }

cleanup:
    LsaAdBatchDestroyBatchItemContents(&batchItem);

    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

    return dwError;

error:
    // set OUT params out cleanup.
    ADCacheSafeFreeObjectList(dwObjectsCount, &ppObjects);
    dwObjectsCount = 0;

    goto cleanup;
}

static
DWORD
LsaAdBatchEnumProcessPseudoMessages(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessages,
    IN DWORD dwCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    LDAPMessage* pCurrentMessage = NULL;
    LDAP* pLd = LwLdapGetSession(hDirectory);
    PSTR* ppszSids = NULL;
    DWORD dwSidsCount = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwObjectsCount = 0;
    PSTR* ppszKeywordValues = NULL;
    DWORD dwKeywordValuesCount = 0;

    dwError = LwAllocateMemory(
                    dwCount * sizeof(*ppszSids),
                    (PVOID*)&ppszSids);
    BAIL_ON_LSA_ERROR(dwError);

    pCurrentMessage = ldap_first_entry(pLd, pMessages);
    while (pCurrentMessage)
    {
        PCSTR pszSidFromKeywords = NULL;

        dwError = LwLdapGetStrings(
                        hDirectory,
                        pCurrentMessage,
                        AD_LDAP_KEYWORDS_TAG,
                        &ppszKeywordValues,
                        &dwKeywordValuesCount);
        BAIL_ON_LSA_ERROR(dwError);

        pszSidFromKeywords = LsaAdBatchFindKeywordAttributeStatic(
                                    dwKeywordValuesCount,
                                    ppszKeywordValues,
                                    AD_LDAP_BACKLINK_PSEUDO_TAG);
        if (LW_IS_NULL_OR_EMPTY_STR(pszSidFromKeywords))
        {
            dwError = LW_ERROR_INVALID_SID;
            BAIL_ON_LSA_ERROR(dwError);
        }

        //
        // There should never be any special groups (such as BUILTIN)
        // that were provisioned in the domain.
        //
        if (!AdIsSpecialDomainSidPrefix(pszSidFromKeywords))
        {
            dwError = LwAllocateString(pszSidFromKeywords,
                                        &ppszSids[dwSidsCount]);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwSidsCount++;
        }
        else
        {
            LSA_LOG_WARNING("Got unexpected special domain SID in enumeration of pseudo-objects: '%s'", pszSidFromKeywords);
        }

        LwFreeStringArray(ppszKeywordValues, dwKeywordValuesCount);
        ppszKeywordValues = NULL;
        dwKeywordValuesCount = 0;

        pCurrentMessage = ldap_next_entry(pLd, pCurrentMessage);
    }

    dwError = LsaAdBatchFindObjects(
                    LSA_AD_BATCH_QUERY_TYPE_BY_SID,
                    dwSidsCount,
                    ppszSids,
                    NULL,
                    &dwObjectsCount,
                    &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    // Ideally, do extra sanity check on object type.
    // In the future, find should take the object type
    // and restrict the search based on that.
    XXX;

cleanup:
    LwFreeStringArray(ppszKeywordValues, dwKeywordValuesCount);
    LwFreeStringArray(ppszSids, dwSidsCount);

    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

    return dwError;

error:
    // set OUT params out cleanup.
    ADCacheSafeFreeObjectList(dwObjectsCount, &ppObjects);
    dwObjectsCount = 0;

    goto cleanup;
}

static
DWORD
LsaAdBatchEnumProcessMessages(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN DWORD dwDirectoryMode,
    IN ADConfigurationMode adMode,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessages,
    IN DWORD dwMaxCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    LDAP* pLd = LwLdapGetSession(hDirectory);
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwObjectsCount = 0;

    dwCount = ldap_count_entries(pLd, pMessages);
    if ((int)dwCount < 0)
    {
       dwError = LW_ERROR_LDAP_ERROR;
       BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwCount == 0)
    {
        dwError = LsaAdBatchEnumGetNoMoreError(ObjectType);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwCount > dwMaxCount)
    {
       dwError = LW_ERROR_LDAP_ERROR;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if ((DEFAULT_MODE == dwDirectoryMode && SchemaMode == adMode) ||
        (UNPROVISIONED_MODE == dwDirectoryMode))
    {
        dwError = LsaAdBatchEnumProcessRealMessages(
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        dwDirectoryMode,
                        adMode,
                        ObjectType,
                        hDirectory,
                        pMessages,
                        dwCount,
                        &dwObjectsCount,
                        &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaAdBatchEnumProcessPseudoMessages(
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        ObjectType,
                        hDirectory,
                        pMessages,
                        dwCount,
                        &dwObjectsCount,
                        &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

    return dwError;

error:
    // set OUT params out cleanup.
    ADCacheSafeFreeObjectList(dwObjectsCount, &ppObjects);
    dwObjectsCount = 0;

    goto cleanup;
}

// If this function returns with an error, the position stored in pCookie will
// be advanced, even though no results are returned. pCookie will still need
// to be freed outside of this function, even if this function returns with an
// error.
static
DWORD
LsaAdBatchEnumObjectsInCell(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN OUT PLW_SEARCH_COOKIE pCookie,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN DWORD dwDirectoryMode,
    IN ADConfigurationMode adMode,
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszCellDn,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    DWORD dwRemainingObjectsWanted = dwMaxObjectsCount;
    PSTR szBacklinkAttributeList[] =
    {
        AD_LDAP_KEYWORDS_TAG,
        NULL
    };
    PSTR szRealAttributeList[] =
    {
        // AD attributes:
        // - common:
        AD_LDAP_OBJECTCLASS_TAG,
        AD_LDAP_OBJECTSID_TAG,
        AD_LDAP_SAM_NAME_TAG,
        AD_LDAP_DN_TAG,
        // - user-specific:
        AD_LDAP_PRIMEGID_TAG,
        AD_LDAP_UPN_TAG,
        AD_LDAP_USER_CTRL_TAG,
        AD_LDAP_ACCOUT_EXP_TAG,
        AD_LDAP_PWD_LASTSET_TAG,
        // schema mode:
        // - (group alias) or (user gecos in unprovisioned mode):
        AD_LDAP_DISPLAY_NAME_TAG,
        // - unix properties (alias is just user alias):
        AD_LDAP_ALIAS_TAG,
        AD_LDAP_UID_TAG,
        AD_LDAP_GID_TAG,
        AD_LDAP_PASSWD_TAG,
        AD_LDAP_GECOS_TAG,
        AD_LDAP_HOMEDIR_TAG,
        AD_LDAP_SHELL_TAG,
        NULL
    };
    PSTR* pszAttributeList = NULL;
    PSTR pszScopeRoot = NULL;
    PCSTR pszQuery = NULL;
    PSTR pszNetbiosDomainName = NULL;
    LDAPMessage* pMessage = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppTotalObjects = NULL;
    DWORD dwTotalObjectsCount = 0;
    HANDLE hDirectory = NULL;
    BOOLEAN bIsByRealObject = FALSE;
    BOOLEAN bIsSchemaMode = SchemaMode == adMode;

    if (pCookie->bSearchFinished)
    {
        // Client programs cannot directly call this function, so typically
        // this function will not get called after the search is finished.
        // However, if this function failed in a previous invocation, the
        // bSearchFinished would be set to true, but the caller would bail out
        // before moving to the next cell.
        dwError = LsaAdBatchEnumGetNoMoreError(ObjectType);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ((DEFAULT_MODE == dwDirectoryMode && SchemaMode == adMode) ||
        (UNPROVISIONED_MODE == dwDirectoryMode))
    {
        bIsByRealObject = TRUE;
    }

    pszAttributeList = bIsByRealObject ? szRealAttributeList : szBacklinkAttributeList;

    dwError = LsaAdBatchEnumGetScopeRoot(
                    ObjectType,
                    bIsByRealObject,
                    pszDnsDomainName,
                    pszCellDn,
                    &pszScopeRoot);
    BAIL_ON_LSA_ERROR(dwError);

    pszQuery = LsaAdBatchEnumGetQuery(bIsByRealObject, bIsSchemaMode, ObjectType);
    LSA_ASSERT(pszQuery);

    dwError = LsaDmWrapGetDomainName(pszDnsDomainName, NULL,
                                     &pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    while (!pCookie->bSearchFinished && dwRemainingObjectsWanted)
    {
        dwError = LsaDmLdapDirectoryOnePagedSearch(
                        pConn,
                        pszScopeRoot,
                        pszQuery,
                        pszAttributeList,
                        dwRemainingObjectsWanted,
                        pCookie,
                        bIsByRealObject ? LDAP_SCOPE_SUBTREE : LDAP_SCOPE_ONELEVEL,
                        &hDirectory,
                        &pMessage);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAdBatchEnumProcessMessages(
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        ObjectType,
                        dwDirectoryMode,
                        adMode,
                        hDirectory,
                        pMessage,
                        dwRemainingObjectsWanted,
                        &dwObjectsCount,
                        &ppObjects);
        if (dwError == LW_ERROR_NO_MORE_USERS || dwError == LW_ERROR_NO_MORE_GROUPS)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);

        ldap_msgfree(pMessage);
        pMessage = NULL;

        dwRemainingObjectsWanted -= dwObjectsCount;

        dwError = LsaAppendAndFreePtrs(
                        &dwTotalObjectsCount,
                        (PVOID**)&ppTotalObjects,
                        &dwObjectsCount,
                        (PVOID**)&ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LW_SAFE_FREE_STRING(pszScopeRoot);
    LW_SAFE_FREE_STRING(pszNetbiosDomainName);
    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }
    ADCacheSafeFreeObjectList(dwObjectsCount, &ppObjects);

    *pdwObjectsCount = dwTotalObjectsCount;
    *pppObjects = ppTotalObjects;

    return dwError;

error:
    // set OUT params in cleanup.
    ADCacheSafeFreeObjectList(dwTotalObjectsCount, &ppTotalObjects);
    dwTotalObjectsCount = 0;

    goto cleanup;
}

// If this function returns with an error, the position stored in pCookie will
// be advanced, even though no results are returned. pCookie will still need
// to be freed outside of this function, even if this function returns with an
// error.
static
DWORD
LsaAdBatchEnumObjectsInLinkedCells(
    IN OUT PLW_SEARCH_COOKIE pCookie,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppObjectsInOneCell = NULL;
    DWORD dwObjectsCountInOneCell = 0;
    PLSA_SECURITY_OBJECT* ppObjects = *pppObjects;
    DWORD dwObjectsCount = *pdwObjectsCount;
    PAD_CELL_COOKIE_DATA pCookieData = NULL;
    ADConfigurationMode adMode = gpADProviderData->adConfigurationMode;
    BOOLEAN bIsDefaultCell = gpADProviderData->dwDirectoryMode == DEFAULT_MODE;

    LSA_ASSERT(pCookie->pfnFree == LsaAdBatchFreeCellCookie);
    pCookieData = (PAD_CELL_COOKIE_DATA)pCookie->pvData;

    while (dwObjectsCount < dwMaxObjectsCount && pCookieData->pCurrentCell)
    {
        PAD_LINKED_CELL_INFO pCellInfo = (PAD_LINKED_CELL_INFO)
            pCookieData->pCurrentCell->pItem;

        if (pCookieData->pLdapConn == NULL)
        {
            dwError = LsaDmLdapOpenDc(
                            pCellInfo->pszDomain,
                            &pCookieData->pLdapConn);
            BAIL_ON_LSA_ERROR(dwError);
        }

        // determine schema/non-schema mode in the current cell
        dwError = LsaAdBatchQueryCellConfigurationMode(
                       gpADProviderData->szDomain,
                       pCellInfo->pszCellDN,
                       &adMode);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAdBatchIsDefaultCell(pCellInfo->pszCellDN,
                                          &bIsDefaultCell);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAdBatchEnumObjectsInCell(
                        pCookieData->pLdapConn,
                        &pCookieData->LdapCookie,
                        ObjectType,
                        bIsDefaultCell ? DEFAULT_MODE : CELL_MODE,
                        adMode,
                        pCellInfo->pszDomain,
                        pCellInfo->pszCellDN,
                        dwMaxObjectsCount - dwObjectsCount,
                        &dwObjectsCountInOneCell,
                        &ppObjectsInOneCell);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAppendAndFreePtrs(
                        &dwObjectsCount,
                        (PVOID**)&ppObjects,
                        &dwObjectsCountInOneCell,
                        (PVOID**)&ppObjectsInOneCell);
        BAIL_ON_LSA_ERROR(dwError);

        if (pCookieData->LdapCookie.bSearchFinished)
        {
            pCookieData->pCurrentCell = pCookieData->pCurrentCell->pNext;
            LwFreeCookieContents(&pCookieData->LdapCookie);
            LsaDmLdapClose(pCookieData->pLdapConn);
            pCookieData->pLdapConn = NULL;
        }
    }

    if (!pCookieData->pCurrentCell)
    {
        pCookie->bSearchFinished = TRUE;
    }

cleanup:
    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

    ADCacheSafeFreeObjectList(dwObjectsCountInOneCell, &ppObjectsInOneCell);

    return dwError;

error:
   // set OUT params in cleanup...
   ADCacheSafeFreeObjectList(dwObjectsCount, &ppObjects);
   dwObjectsCount = 0;

   goto cleanup;
}

// If this function returns with an error, the position stored in pCookie will
// be advanced, even though no results are returned. pCookie will still need
// to be freed outside of this function, even if this function returns with an
// error.
DWORD
LsaAdBatchEnumObjects(
    IN OUT PLW_SEARCH_COOKIE pCookie,
    IN LSA_OBJECT_TYPE AccountType,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    LSA_AD_BATCH_OBJECT_TYPE objectType = 0;
    DWORD dwObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PAD_CELL_COOKIE_DATA pCookieData = NULL;
    DWORD dwTotalObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppTotalObjects = NULL;
    DWORD dwInput = 0;
    DWORD dwOutput = 0;
    PSTR pszCopiedSid = NULL;

    dwError = LsaAdBatchAccountTypeToObjectType(AccountType, &objectType);
    BAIL_ON_LSA_ERROR(dwError);

    if (pCookie->bSearchFinished)
    {
        dwError = LsaAdBatchEnumGetNoMoreError(objectType);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pCookie->pfnFree == NULL)
    {
        dwError = LwAllocateMemory(
                        sizeof(AD_CELL_COOKIE_DATA),
                        &pCookie->pvData);
        BAIL_ON_LSA_ERROR(dwError);

        pCookieData = (PAD_CELL_COOKIE_DATA)pCookie->pvData;
        if (gpADProviderData->pCellList != NULL)
        {
            // There are linked cells, so we need to keep track of which
            // sids have been enumerated.
            dwError = LsaHashCreate(
                            10 * 1024,
                            LsaHashCaselessStringCompare,
                            LsaHashCaselessStringHash,
                            LsaHashFreeStringKey,
                            NULL,
                            &pCookieData->pEnumeratedSids);
            BAIL_ON_LSA_ERROR(dwError);
        }

        pCookie->pfnFree = LsaAdBatchFreeCellCookie;
    }
    else
    {
        LSA_ASSERT(pCookie->pfnFree == LsaAdBatchFreeCellCookie);
        pCookieData = (PAD_CELL_COOKIE_DATA)pCookie->pvData;
    }

    while (dwTotalObjectsCount < dwMaxObjectsCount &&
            !pCookie->bSearchFinished)
    {
        if (pCookieData->pCurrentCell == NULL)
        {
            if (pCookieData->pLdapConn == NULL)
            {
                dwError = LsaDmLdapOpenDc(
                                gpADProviderData->szDomain,
                                &pCookieData->pLdapConn);
                BAIL_ON_LSA_ERROR(dwError);
            }

            // First get the objects from the primary cell
            dwError = LsaAdBatchEnumObjectsInCell(
                            pCookieData->pLdapConn,
                            &pCookieData->LdapCookie,
                            objectType,
                            gpADProviderData->dwDirectoryMode,
                            gpADProviderData->adConfigurationMode,
                            gpADProviderData->szDomain,
                            gpADProviderData->cell.szCellDN,
                            dwMaxObjectsCount - dwTotalObjectsCount,
                            &dwObjectsCount,
                            &ppObjects);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAppendAndFreePtrs(
                            &dwTotalObjectsCount,
                            (PVOID**)&ppTotalObjects,
                            &dwObjectsCount,
                            (PVOID**)&ppObjects);
            BAIL_ON_LSA_ERROR(dwError);

            if (pCookieData->LdapCookie.bSearchFinished)
            {
                LwFreeCookieContents(&pCookieData->LdapCookie);
                LsaDmLdapClose(pCookieData->pLdapConn);
                pCookieData->pLdapConn = NULL;
                pCookieData->pCurrentCell = gpADProviderData->pCellList;
                if (pCookieData->pCurrentCell == NULL)
                {
                    pCookie->bSearchFinished = TRUE;
                }
            }
        }
        else
        {
            dwError = LsaAdBatchEnumObjectsInLinkedCells(
                         pCookie,
                         objectType,
                         dwMaxObjectsCount - dwTotalObjectsCount,
                         &dwObjectsCount,
                         &ppObjects);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAppendAndFreePtrs(
                            &dwTotalObjectsCount,
                            (PVOID**)&ppTotalObjects,
                            &dwObjectsCount,
                            (PVOID**)&ppObjects);
            BAIL_ON_LSA_ERROR(dwError);
        }

        // Remove any sids that have already been enumerated
        if (pCookieData->pEnumeratedSids != NULL)
        {
            for (; dwInput < dwTotalObjectsCount; dwInput++)
            {
                dwError = LsaHashGetValue(
                            pCookieData->pEnumeratedSids,
                            ppTotalObjects[dwInput]->pszObjectSid,
                            NULL);
                if (dwError == LW_ERROR_SUCCESS)
                {
                    // The object is already in the hash
                    ADCacheSafeFreeObject(&ppTotalObjects[dwInput]);
                }
                else if (dwError == ERROR_NOT_FOUND)
                {
                    // This is a new entry; let's track it in the hash

                    if (pCookieData->pEnumeratedSids->sCount * 2 >
                        pCookieData->pEnumeratedSids->sTableSize)
                    {
                        // Enlarge the hash table to avoid collisions
                        dwError = LsaHashResize(
                                    pCookieData->pEnumeratedSids,
                                    pCookieData->pEnumeratedSids->sCount * 4);
                        BAIL_ON_LSA_ERROR(dwError);
                    }

                    dwError = LwAllocateString(
                                    ppTotalObjects[dwInput]->pszObjectSid,
                                    &pszCopiedSid);
                    BAIL_ON_LSA_ERROR(dwError);

                    dwError = LsaHashSetValue(
                                pCookieData->pEnumeratedSids,
                                pszCopiedSid,
                                NULL);
                    BAIL_ON_LSA_ERROR(dwError);

                    // This is now owned by the hash table
                    pszCopiedSid = NULL;

                    ppTotalObjects[dwOutput++] = ppTotalObjects[dwInput];
                }
                else
                {
                    BAIL_ON_LSA_ERROR(dwError);
                }
            }
            // The array is now smaller since duplicates have been removed
            dwTotalObjectsCount = dwOutput;
            dwInput = dwOutput;
        }
    }

cleanup:
    *pdwObjectsCount = dwTotalObjectsCount;
    *pppObjects = ppTotalObjects;
    LW_SAFE_FREE_STRING(pszCopiedSid);

    return dwError;

error:
    // set OUT params in cleanup...
    ADCacheSafeFreeObjectList(dwObjectsCount, &ppObjects);
    dwObjectsCount = 0;
    ADCacheSafeFreeObjectList(dwTotalObjectsCount, &ppTotalObjects);
    dwTotalObjectsCount = 0;
    goto cleanup;
}
