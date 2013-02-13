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
 *        batch_build.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *
 */

#include "adprovider.h"
#include "batch_build.h"

typedef DWORD (*LSA_AD_BATCH_BUILDER_GET_ATTR_VALUE_CALLBACK)(
    IN PAD_PROVIDER_DATA pProviderData,
    IN PVOID pCallbackContext,
    IN PVOID pItem,
    OUT PCSTR* ppszValue,
    OUT PVOID* ppFreeValueContext
    );

typedef VOID (*LSA_AD_BATCH_BUILDER_FREE_VALUE_CONTEXT_CALLBACK)(
    IN PVOID pCallbackContext,
    IN OUT PVOID* ppFreeValueContext
    );

typedef PVOID (*LSA_AD_BATCH_BUILDER_NEXT_ITEM_CALLBACK)(
    IN PVOID pCallbackContext,
    IN PVOID pItem
    );

static
DWORD
LsaAdBatchBuilderAppend(
    IN OUT PDWORD pdwQueryOffset,
    IN OUT PSTR pszQuery,
    IN DWORD dwQuerySize,
    IN PCSTR pszAppend,
    IN DWORD dwAppendLength
    )
{
    DWORD dwError = 0;
    DWORD dwQueryOffset = *pdwQueryOffset;
    DWORD dwNewQueryOffset = 0;

    if (dwAppendLength > 0)
    {
        dwNewQueryOffset = dwQueryOffset + dwAppendLength;
        if (dwNewQueryOffset < dwQueryOffset)
        {
            // overflow
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (dwNewQueryOffset - 1 >= dwQuerySize)
        {
            // overflow
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
        memcpy(pszQuery + dwQueryOffset, pszAppend, dwAppendLength);
        pszQuery[dwNewQueryOffset] = 0;
        *pdwQueryOffset = dwNewQueryOffset;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchBuilderCreateQuery(
    IN PAD_PROVIDER_DATA pProviderData,
    IN PCSTR pszQueryPrefix,
    IN PCSTR pszQuerySuffix,
    IN PCSTR pszAttributeName,
    IN PVOID pFirstItem,
    IN PVOID pEndItem,
    OUT PVOID* ppNextItem,
    IN OPTIONAL PVOID pCallbackContext,
    IN LSA_AD_BATCH_BUILDER_GET_ATTR_VALUE_CALLBACK pGetAttributeValueCallback,
    IN OPTIONAL LSA_AD_BATCH_BUILDER_FREE_VALUE_CONTEXT_CALLBACK pFreeValueContextCallback,
    IN LSA_AD_BATCH_BUILDER_NEXT_ITEM_CALLBACK pNextItemCallback,
    IN DWORD dwMaxQuerySize,
    IN DWORD dwMaxQueryCount,
    OUT PDWORD pdwQueryCount,
    OUT PSTR* ppszQuery
    )
{
    DWORD dwError = 0;
    PVOID pCurrentItem = NULL;
    PSTR pszQuery = NULL;
    PVOID pLastItem = pFirstItem;
    const char szOrPrefix[] = "(|";
    const char szOrSuffix[] = ")";
    const DWORD dwOrPrefixLength = sizeof(szOrPrefix)-1;
    const DWORD dwOrSuffixLength = sizeof(szOrSuffix)-1;
    DWORD dwAttributeNameLength = strlen(pszAttributeName);
    DWORD dwQuerySize = 0;
    DWORD dwQueryCount = 0;
    PVOID pFreeValueContext = NULL;
    DWORD dwQueryPrefixLength = 0;
    DWORD dwQuerySuffixLength = 0;
    DWORD dwQueryOffset = 0;
    DWORD dwSavedQueryCount = 0;

    if (pszQueryPrefix)
    {
        dwQueryPrefixLength = strlen(pszQueryPrefix);
    }
    if (pszQuerySuffix)
    {
        dwQuerySuffixLength = strlen(pszQuerySuffix);
    }

    // The overhead is:
    // prefix + orPrefix + <CONTENT> + orSuffix + suffix + NULL
    dwQuerySize = dwQueryPrefixLength + dwOrPrefixLength + dwOrSuffixLength + dwQuerySuffixLength + 1;

    pCurrentItem = pFirstItem;
    while (pCurrentItem != pEndItem)
    {
        PCSTR pszAttributeValue = NULL;

        if (pFreeValueContext)
        {
            pFreeValueContextCallback(pCallbackContext, &pFreeValueContext);
        }

        dwError = pGetAttributeValueCallback(
                        pProviderData,
                        pCallbackContext,
                        pCurrentItem,
                        &pszAttributeValue,
                        &pFreeValueContext);
        BAIL_ON_LSA_ERROR(dwError);

        if (pszAttributeValue)
        {
            // "(" + attributeName + "=" + attributeValue + ")"
            DWORD dwAttributeValueLength = strlen(pszAttributeValue);
            DWORD dwItemLength = (1 + dwAttributeNameLength + 1 + dwAttributeValueLength + 1);
            DWORD dwNewQuerySize = dwQuerySize + dwItemLength;
            DWORD dwNewQueryCount = dwQueryCount + 1;

            if (dwNewQuerySize < dwQuerySize)
            {
                // overflow
                dwError = LW_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }
            if (dwMaxQuerySize && (dwNewQuerySize > dwMaxQuerySize))
            {
                break;
            }
            if (dwMaxQueryCount && (dwNewQueryCount > dwMaxQueryCount))
            {
                break;
            }
            dwQuerySize = dwNewQuerySize;
            dwQueryCount = dwNewQueryCount;
        }

        pCurrentItem = pNextItemCallback(pCallbackContext, pCurrentItem);
    }
    pLastItem = pCurrentItem;
    dwSavedQueryCount = dwQueryCount;

    if (dwQueryCount < 1)
    {
        goto cleanup;
    }

    dwError = LwAllocateMemory(dwQuerySize, (PVOID*)&pszQuery);
    BAIL_ON_LSA_ERROR(dwError);

    // Set up the query
    dwQueryOffset = 0;

    dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                      pszQueryPrefix, dwQueryPrefixLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                      szOrPrefix, dwOrPrefixLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwQueryCount = 0;
    pCurrentItem = pFirstItem;
    while (pCurrentItem != pLastItem)
    {
        PCSTR pszAttributeValue = NULL;

        if (pFreeValueContext)
        {
            pFreeValueContextCallback(pCallbackContext, &pFreeValueContext);
        }

        dwError = pGetAttributeValueCallback(
                        pProviderData,
                        pCallbackContext,
                        pCurrentItem,
                        &pszAttributeValue,
                        &pFreeValueContext);
        BAIL_ON_LSA_ERROR(dwError);

        if (pszAttributeValue)
        {
            DWORD dwAttributeValueLength = strlen(pszAttributeValue);

            dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                              "(", 1);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                              pszAttributeName, dwAttributeNameLength);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                              "=", 1);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                              pszAttributeValue, dwAttributeValueLength);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                              ")", 1);
            BAIL_ON_LSA_ERROR(dwError);

            dwQueryCount++;
        }

        pCurrentItem = pNextItemCallback(pCallbackContext, pCurrentItem);
    }

    dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                      szOrSuffix, dwOrSuffixLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                      pszQuerySuffix, dwQuerySuffixLength);
    BAIL_ON_LSA_ERROR(dwError);

    assert(dwQueryOffset + 1 == dwQuerySize);
    assert(dwSavedQueryCount == dwQueryCount);

cleanup:
    // We handle error here instead of error label
    // because there is a goto cleanup above.
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszQuery);
        dwQueryCount = 0;
        pLastItem = pFirstItem;
    }

    if (pFreeValueContext)
    {
        pFreeValueContextCallback(pCallbackContext, &pFreeValueContext);
    }

    *ppszQuery = pszQuery;
    *pdwQueryCount = dwQueryCount;
    *ppNextItem = pLastItem;

    return dwError;

error:
    // Do not actually handle any errors here.
    goto cleanup;
}

typedef struct _LSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT {
    LSA_AD_BATCH_QUERY_TYPE QueryType;
    BOOLEAN bIsForRealObject;
} LSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT, *PLSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT;

static
DWORD
LsaAdBatchBuilderBatchItemGetAttributeValue(
    IN PAD_PROVIDER_DATA pProviderData,
    IN PVOID pCallbackContext,
    IN PVOID pItem,
    OUT PCSTR* ppszValue,
    OUT PVOID* ppFreeValueContext
    )
{
    DWORD dwError = 0;
    PLSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT pContext = (PLSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT)pCallbackContext;
    LSA_AD_BATCH_QUERY_TYPE QueryType = pContext->QueryType;
    BOOLEAN bIsForRealObject = pContext->bIsForRealObject;
    PLSA_LIST_LINKS pLinks = (PLSA_LIST_LINKS)pItem;
    PLSA_AD_BATCH_ITEM pBatchItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);
    PSTR pszValueToEscape = NULL;
    PSTR pszValueToHex = NULL;
    PSTR pszValue = NULL;
    PVOID pFreeValueContext = NULL;
    // Free this on error only.
    PSTR pszMatchTerm = NULL;
    PSTR pszAllocatedMatchTermValue = NULL;
    BOOLEAN bHaveReal = IsSetFlag(pBatchItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_REAL);
    BOOLEAN bHavePseudo = IsSetFlag(pBatchItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO);

    if (IsSetFlag(pBatchItem->Flags, LSA_AD_BATCH_ITEM_FLAG_ALLOCATED_MATCH_TERM))
    {
        LW_SAFE_FREE_STRING(pBatchItem->pszQueryMatchTerm);
        ClearFlag(pBatchItem->Flags, LSA_AD_BATCH_ITEM_FLAG_ALLOCATED_MATCH_TERM);
    }

    if ((bIsForRealObject && bHaveReal) ||
        (!bIsForRealObject && bHavePseudo))
    {
        // This can only happen in the linked cells case, but even
        // that should go away in the future as we just keep an
        // unresolved batch items list.

        LSA_ASSERT(!bIsForRealObject && (pProviderData->dwDirectoryMode == CELL_MODE));
        goto cleanup;
    }

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            // Must be looking for real object
            LSA_ASSERT(bIsForRealObject);
            LSA_ASSERT(QueryType == pBatchItem->QueryTerm.Type);

            pszValueToEscape = (PSTR)pBatchItem->QueryTerm.pszString;
            LSA_ASSERT(pszValueToEscape);
            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            if (pBatchItem->pszSid)
            {
                // This is case where we already got the SID by resolving
                // the pseudo object by id/alias.
                pszValue = pBatchItem->pszSid;
            }
            else if (QueryType == pBatchItem->QueryTerm.Type)
            {
                // This is the case where the original query was
                // a query by SID.
                pszValue = (PSTR)pBatchItem->QueryTerm.pszString;
                LSA_ASSERT(pszValue);
            }
            // Will be NULL if we cannot find a SID for which to query.
            // This can happen if this batch item is for an object
            // that we did not find real but are trying to look up pseudo.
            // In that case, we have NULL and will just skip it.

            // If we have a SID string, make sure it looks like a SID.
            // Note that we must do some sanity checking to make sure
            // that the string does not need escaping since we are
            // not setting pszValueToEscape.  (The SID check takes
            // care of that.)
            if (pszValue && !LsaAdBatchHasValidCharsForSid(pszValue))
            {
                LSA_ASSERT(FALSE);
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }

            if (bIsForRealObject)
            {
                // "S-"-style SID string are only handled starting with
                // Windows 2003.  So an LDAP hex-formatted SID string
                // is needed to handle Windows 2000.

                pszValueToHex = pszValue;
                pszValue = NULL;
            }
            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            LSA_ASSERT(bIsForRealObject);
            if (pBatchItem->pszSamAccountName)
            {
                // Unprovisioned id/alias case where id mapper returned
                // a SAM account name (and domain name) but no SID.
                pszValueToEscape = pBatchItem->pszSamAccountName;
                // However, we currently do not have this sort of id mapper
                // support, so we LSA_ASSERT(FALSE) for now.
                LSA_ASSERT(FALSE);
            }
            else if (QueryType == pBatchItem->QueryTerm.Type)
            {
                pszValueToEscape = (PSTR)pBatchItem->QueryTerm.pszString;
                LSA_ASSERT(pszValueToEscape);
            }
            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
        case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
            LSA_ASSERT(!bIsForRealObject);
            LSA_ASSERT(QueryType == pBatchItem->QueryTerm.Type);

            dwError = LwAllocateStringPrintf(
                            &pszAllocatedMatchTermValue,
                            "%u",
                            (unsigned int)pBatchItem->QueryTerm.dwId);
            BAIL_ON_LSA_ERROR(dwError);
            pszValue = pszAllocatedMatchTermValue;

            // Note: It is ok to set this here because it is ok for
            // this flag to be set if the match term field in the
            // batch item is still NULL (i.e., if we fail later in
            // this function).
            SetFlag(pBatchItem->Flags, LSA_AD_BATCH_ITEM_FLAG_ALLOCATED_MATCH_TERM);

            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
        case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
            LSA_ASSERT(!bIsForRealObject);
            LSA_ASSERT(QueryType == pBatchItem->QueryTerm.Type);

            pszValueToEscape = (PSTR)pBatchItem->QueryTerm.pszString;
            LSA_ASSERT(pszValueToEscape);
            break;

        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    LSA_ASSERT(!pszMatchTerm);
    LSA_ASSERT(!(pszValueToEscape && pszValueToHex));

    if (pszValueToEscape)
    {
        LSA_ASSERT(!pszValue);
        dwError = LwLdapEscapeString(&pszValue, pszValueToEscape);
        BAIL_ON_LSA_ERROR(dwError);

        pszMatchTerm = pszValueToEscape;
        pFreeValueContext = pszValue;
    }
    else if (pszValueToHex)
    {
        LSA_ASSERT(!pszValue);
        dwError = LsaSidStrToLdapFormatHexStr(pszValueToHex, &pszValue);
        BAIL_ON_LSA_ERROR(dwError);

        pszMatchTerm = pszValueToHex;
        pFreeValueContext = pszValue;
    }
    else
    {
        pszMatchTerm = pszValue;
    }

cleanup:
    // Note that the match value can be different from the value,
    // which we may need to escape or hex.
    pBatchItem->pszQueryMatchTerm = pszMatchTerm;
    *ppszValue = pszValue;
    *ppFreeValueContext = pFreeValueContext;

    return dwError;

error:
    // assing output in cleanup in case of goto cleanup in function.
    pszValueToEscape = NULL;
    pszValue = NULL;
    LW_SAFE_FREE_STRING(pFreeValueContext);
    LW_SAFE_FREE_STRING(pszAllocatedMatchTermValue);
    goto cleanup;
}

static
VOID
LsaAdBatchBuilderGenericFreeValueContext(
    IN PVOID pCallbackContext,
    IN OUT PVOID* ppFreeValueContext
    )
{
    LW_SAFE_FREE_MEMORY(*ppFreeValueContext);
}

static
PVOID
LsaAdBatchBuilderBatchItemNextItem(
    IN PVOID pCallbackContext,
    IN PVOID pItem
    )
{
    PLSA_LIST_LINKS pLinks = (PLSA_LIST_LINKS)pItem;
    return pLinks->Next;
}

#define AD_LDAP_QUERY_LW_USER  "(keywords=objectClass=" AD_LDAP_CLASS_LW_USER ")"
#define AD_LDAP_QUERY_LW_GROUP "(keywords=objectClass=" AD_LDAP_CLASS_LW_GROUP ")"
#define AD_LDAP_QUERY_SCHEMA_USER  "(objectClass=" AD_LDAP_CLASS_SCHEMA_USER ")"
#define AD_LDAP_QUERY_SCHEMA_GROUP "(objectClass=" AD_LDAP_CLASS_SCHEMA_GROUP ")"
#define AD_LDAP_QUERY_NON_SCHEMA "(objectClass=" AD_LDAP_CLASS_NON_SCHEMA ")"
#define AD_LDAP_QUERY_USER "(objectClass=User)"
#define AD_LDAP_QUERY_GROUP "(objectClass=Group)"
#define AD_LDAP_QUERY_DEFAULT_SCHEMA_ENABLED_USER "(uidNumber=*)"
#define AD_LDAP_QUERY_DEFAULT_SCHEMA_ENABLED_GROUP "(gidNumber=*)"


static
PCSTR
LsaAdBatchBuilderGetPseudoQueryPrefixInternal(
    IN PAD_PROVIDER_DATA pProviderData,
    IN OPTIONAL PDWORD pdwDirectoryMode,
    IN BOOLEAN bIsSchemaMode,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    OUT PCSTR* ppszSuffix
    )
{
    PCSTR pszPrefix = NULL;
    DWORD dwDirectoryMode = (pdwDirectoryMode == NULL ?
                             pProviderData->dwDirectoryMode :
                             *pdwDirectoryMode);

    if ((DEFAULT_MODE == dwDirectoryMode) && bIsSchemaMode)
    {
        if (LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS == QueryType ||
            LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS == QueryType)
        {
            switch (ObjectType)
            {
                case LSA_AD_BATCH_OBJECT_TYPE_USER:
                    pszPrefix =
                        "(&"
                        "(&" AD_LDAP_QUERY_USER AD_LDAP_QUERY_DEFAULT_SCHEMA_ENABLED_USER")"
                        "";
                    break;
                case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                    pszPrefix =
                        "(&"
                        "(&" AD_LDAP_QUERY_GROUP AD_LDAP_QUERY_DEFAULT_SCHEMA_ENABLED_GROUP")"
                        "";
                    break;
                default:
                    pszPrefix =
                        "(&"
                        "(|"
                        "(&" AD_LDAP_QUERY_USER AD_LDAP_QUERY_DEFAULT_SCHEMA_ENABLED_USER")"
                        "(&" AD_LDAP_QUERY_GROUP AD_LDAP_QUERY_DEFAULT_SCHEMA_ENABLED_GROUP")"
                        ")"
                        "";
            }
        }
        else
        {
            switch (ObjectType)
            {
                case LSA_AD_BATCH_OBJECT_TYPE_USER:
                    pszPrefix =
                        "(&"
                        "(&" AD_LDAP_QUERY_USER ")"
                        "";
                    break;
                case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                    pszPrefix =
                        "(&"
                        "(&" AD_LDAP_QUERY_GROUP ")"
                        "";
                    break;
                default:
                    pszPrefix =
                        "(&"
                        "(|"
                        "(&" AD_LDAP_QUERY_USER ")"
                        "(&" AD_LDAP_QUERY_GROUP ")"
                        ")"
                        "";
            }
        }
    }
    else if (bIsSchemaMode)
    {
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszPrefix =
                    "(&"
                    "(&" AD_LDAP_QUERY_SCHEMA_USER AD_LDAP_QUERY_LW_USER ")"
                    "";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszPrefix =
                    "(&"
                    "(&" AD_LDAP_QUERY_SCHEMA_GROUP AD_LDAP_QUERY_LW_GROUP ")"
                    "";
                break;
            default:
                pszPrefix =
                    "(&"
                    "(|"
                    "(&" AD_LDAP_QUERY_SCHEMA_USER AD_LDAP_QUERY_LW_USER ")"
                    "(&" AD_LDAP_QUERY_SCHEMA_GROUP AD_LDAP_QUERY_LW_GROUP ")"
                    ")"
                    "";
        }
    }
    else
    {
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszPrefix =
                    "(&"
                    AD_LDAP_QUERY_NON_SCHEMA
                    AD_LDAP_QUERY_LW_USER
                    "";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszPrefix =
                    "(&"
                    AD_LDAP_QUERY_NON_SCHEMA
                    AD_LDAP_QUERY_LW_GROUP
                    "";
                break;
            default:
                pszPrefix =
                    "(&"
                    AD_LDAP_QUERY_NON_SCHEMA
                    "(|"
                    AD_LDAP_QUERY_LW_USER
                    AD_LDAP_QUERY_LW_GROUP
                    ")"
                    "";
        }
    }

    *ppszSuffix = pszPrefix ? ")" : NULL;
    return pszPrefix;
}

static
PCSTR
LsaAdBatchBuilderGetPseudoQueryAttributeInternal(
    IN OPTIONAL PDWORD pdwDirectoryMode,
    IN BOOLEAN bIsSchemaMode,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType
    )
{
    PCSTR pszAttributeName = NULL;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            if (bIsSchemaMode &&
                pdwDirectoryMode != NULL && *pdwDirectoryMode == DEFAULT_MODE)
            {
                pszAttributeName = AD_LDAP_OBJECTSID_TAG;
            }
            else
            {
                pszAttributeName = "keywords=" AD_LDAP_BACKLINK_PSEUDO_TAG;
            }
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
            if (bIsSchemaMode)
            {
                pszAttributeName = AD_LDAP_UID_TAG;
            }
            else
            {
                pszAttributeName = "keywords=" AD_LDAP_UID_TAG;
            }
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
            if (bIsSchemaMode)
            {
                pszAttributeName = AD_LDAP_GID_TAG;
            }
            else
            {
                pszAttributeName = "keywords=" AD_LDAP_GID_TAG;
            }
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
            if (bIsSchemaMode)
            {
                pszAttributeName = AD_LDAP_ALIAS_TAG;
            }
            else
            {
                pszAttributeName = "keywords=" AD_LDAP_ALIAS_TAG;
            }
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
            if (bIsSchemaMode)
            {
                pszAttributeName = AD_LDAP_DISPLAY_NAME_TAG;
            }
            else
            {
                pszAttributeName = "keywords=" AD_LDAP_DISPLAY_NAME_TAG;
            }
            break;
    }

    return pszAttributeName;
}

DWORD
LsaAdBatchBuildQueryForRpc(
    IN PCSTR pszNetbiosDomainName,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN PLSA_LIST_LINKS pFirstLinks,
    IN PLSA_LIST_LINKS pEndLinks,
    OUT PLSA_LIST_LINKS* ppNextLinks,
    IN DWORD dwMaxQueryCount,
    OUT PDWORD pdwQueryCount,
    OUT PSTR** pppszQueryList
    )
{
    DWORD dwError = 0;
    PLSA_LIST_LINKS pLinks = NULL;
    PSTR* ppszQueryList = NULL;
    PLSA_LIST_LINKS pLastLinks = pFirstLinks;
    DWORD dwQueryCount = 0;
    DWORD dwSavedQueryCount = 0;

    pLinks = pFirstLinks;
    for (pLinks = pFirstLinks; pLinks != pEndLinks; pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pBatchItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);
        PCSTR pszQueryTerm = NULL;

        switch (QueryType)
        {
            case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            {
                if (pBatchItem->pszSid)
                {
                    pszQueryTerm = pBatchItem->pszSid;
                }
                else if (QueryType == pBatchItem->QueryTerm.Type)
                {
                    pszQueryTerm = pBatchItem->QueryTerm.pszString;
                }
                break;
            }
            case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            {
                if (pBatchItem->pszSamAccountName)
                {
                    pszQueryTerm = pBatchItem->pszSamAccountName;
                }
                else if (QueryType == pBatchItem->QueryTerm.Type)
                {
                    pszQueryTerm = pBatchItem->QueryTerm.pszString;
                }
                break;
            }
            default:
                LSA_ASSERT(FALSE);
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
        }

        if (!LW_IS_NULL_OR_EMPTY_STR(pszQueryTerm))
        {
            DWORD dwNewQueryCount = dwQueryCount + 1;

            if (dwMaxQueryCount && (dwNewQueryCount > dwMaxQueryCount))
            {
                break;
            }
            dwQueryCount = dwNewQueryCount;
        }
    }
    pLastLinks = pLinks;
    dwSavedQueryCount = dwQueryCount;

    if (dwQueryCount < 1)
    {
        goto cleanup;
    }

    dwError = LwAllocateMemory(dwQueryCount*sizeof(*ppszQueryList), (PVOID*)&ppszQueryList);
    BAIL_ON_LSA_ERROR(dwError);

    dwQueryCount = 0;
    // Loop until we reach last links.
    for (pLinks = pFirstLinks; pLinks != pLastLinks; pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pBatchItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        if (IsSetFlag(pBatchItem->Flags, LSA_AD_BATCH_ITEM_FLAG_ALLOCATED_MATCH_TERM))
        {
            LW_SAFE_FREE_STRING(pBatchItem->pszQueryMatchTerm);
            ClearFlag(pBatchItem->Flags, LSA_AD_BATCH_ITEM_FLAG_ALLOCATED_MATCH_TERM);
        }

        switch (QueryType)
        {
            case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            {
                PCSTR pszUseSid = NULL;
                if (pBatchItem->pszSid)
                {
                    pszUseSid = pBatchItem->pszSid;
                }
                else if (QueryType == pBatchItem->QueryTerm.Type)
                {
                    pszUseSid = pBatchItem->QueryTerm.pszString;
                }
                // We might not have a SID if we failed to find a pseudo.
                if (pszUseSid)
                {
                    dwError = LwAllocateString(pszUseSid,
                                                &ppszQueryList[dwQueryCount++]);
                    BAIL_ON_LSA_ERROR(dwError);
                    pBatchItem->pszQueryMatchTerm = (PSTR)pszUseSid;
                }
                break;
            }

            case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            {
                PCSTR pszUseSamAccountName = NULL;
                if (pBatchItem->pszSamAccountName)
                {
                    pszUseSamAccountName = pBatchItem->pszSamAccountName;
                }
                else if (QueryType == pBatchItem->QueryTerm.Type)
                {
                    pszUseSamAccountName = pBatchItem->QueryTerm.pszString;
                }
                if (pszUseSamAccountName)
                {
                    dwError = LwAllocateStringPrintf(
                                    &ppszQueryList[dwQueryCount++],
                                    "%s\\%s",
                                    pszNetbiosDomainName,
                                    pszUseSamAccountName);
                    BAIL_ON_LSA_ERROR(dwError);
                    pBatchItem->pszQueryMatchTerm = (PSTR)pszUseSamAccountName;
                }
                break;
            }
            default:
                LSA_ASSERT(FALSE);
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
        }
    }

    assert(dwSavedQueryCount == dwQueryCount);

cleanup:
    // We handle error here instead of error label
    // because there is a goto cleanup above.
    if (dwError)
    {
        LwFreeStringArray(ppszQueryList, dwSavedQueryCount);
        ppszQueryList = NULL;
        dwQueryCount = 0;
        dwSavedQueryCount = 0;
        pLastLinks = pFirstLinks;
    }

    *pppszQueryList = ppszQueryList;
    *pdwQueryCount = dwQueryCount;
    *ppNextLinks = pLastLinks;

    return dwError;

error:
    // Do not actually handle any errors here.
    goto cleanup;
}

DWORD
LsaAdBatchBuildQueryForReal(
    IN PAD_PROVIDER_DATA pProviderData,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN PLSA_LIST_LINKS pFirstLinks,
    IN PLSA_LIST_LINKS pEndLinks,
    OUT PLSA_LIST_LINKS* ppNextLinks,
    IN DWORD dwMaxQuerySize,
    IN DWORD dwMaxQueryCount,
    OUT PDWORD pdwQueryCount,
    OUT PSTR* ppszQuery
    )
{
    DWORD dwError = 0;
    PLSA_LIST_LINKS pNextLinks = NULL;
    DWORD dwQueryCount = 0;
    PSTR pszQuery = NULL;
    PCSTR pszAttributeName = NULL;
    PCSTR pszPrefix = NULL;
    LSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT context = { 0 };

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            pszAttributeName = AD_LDAP_DN_TAG;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            pszAttributeName = AD_LDAP_OBJECTSID_TAG;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            pszAttributeName = AD_LDAP_SAM_NAME_TAG;
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    // In Default/schema case, filter out disabled users
    // when querying real objects.
    if ((pProviderData->dwDirectoryMode == DEFAULT_MODE) &&
        (pProviderData->adConfigurationMode == SchemaMode))
    {
        pszPrefix = "(&(|(&(objectClass=user)(uidNumber=*))(objectClass=group))(!(objectClass=computer))";
    }
    else
    {
#if 0
        pszPrefix = "(&(|(objectClass=user)(objectClass=group))(!(objectClass=computer))";
#else
	/* Enable machine accounts in the unprovisioned provider.
	   This is needed for the srv driver in lwio.  Clients frequently
	   connect using their machine account credentials for dfs referrals,
	   querying server capabilities, etc....  -- gcarter@likewise.com */

        pszPrefix = "(&(|(objectClass=user)(&(objectClass=group)(groupType<=0)))";
#endif
    }

    context.QueryType = QueryType;
    context.bIsForRealObject = TRUE;

    dwError = LsaAdBatchBuilderCreateQuery(
                    pProviderData,
                    pszPrefix,
                    ")",
                    pszAttributeName,
                    pFirstLinks,
                    pEndLinks,
                    (PVOID*)&pNextLinks,
                    &context,
                    LsaAdBatchBuilderBatchItemGetAttributeValue,
                    LsaAdBatchBuilderGenericFreeValueContext,
                    LsaAdBatchBuilderBatchItemNextItem,
                    dwMaxQuerySize,
                    dwMaxQueryCount,
                    &dwQueryCount,
                    &pszQuery);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppNextLinks = pNextLinks;
    *pdwQueryCount = dwQueryCount;
    *ppszQuery = pszQuery;
    return dwError;

error:
    // set output on cleanup
    pNextLinks = pFirstLinks;
    dwQueryCount = 0;
    LW_SAFE_FREE_STRING(pszQuery);
    goto cleanup;
}

static
DWORD
LsaAdBatchBuildQueryForPseudoInternal(
    IN PAD_PROVIDER_DATA pProviderData,
    IN BOOLEAN bIsSchemaMode,
    IN OPTIONAL PDWORD pdwDirectoryMode,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN PLSA_LIST_LINKS pFirstLinks,
    IN PLSA_LIST_LINKS pEndLinks,
    OUT PLSA_LIST_LINKS* ppNextLinks,
    IN DWORD dwMaxQuerySize,
    IN DWORD dwMaxQueryCount,
    OUT PDWORD pdwQueryCount,
    OUT PSTR* ppszQuery
    )
{
    DWORD dwError = 0;
    PLSA_LIST_LINKS pNextLinks = NULL;
    DWORD dwQueryCount = 0;
    PSTR pszQuery = NULL;
    PCSTR pszAttributeName = NULL;
    PCSTR pszPrefix = NULL;
    PCSTR pszSuffix = NULL;
    LSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT context = { 0 };

    pszAttributeName = LsaAdBatchBuilderGetPseudoQueryAttributeInternal(
                            pdwDirectoryMode,
                            bIsSchemaMode,
                            QueryType);
    if (!pszAttributeName)
    {
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pszPrefix = LsaAdBatchBuilderGetPseudoQueryPrefixInternal(
                        pProviderData,
                        pdwDirectoryMode,
                        bIsSchemaMode,
                        QueryType,
                        LsaAdBatchGetObjectTypeFromQueryType(QueryType),
                        &pszSuffix);

    if (!pszPrefix || !pszSuffix)
    {
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    context.QueryType = QueryType;
    context.bIsForRealObject = FALSE;

    dwError = LsaAdBatchBuilderCreateQuery(
                    pProviderData,
                    pszPrefix,
                    pszSuffix,
                    pszAttributeName,
                    pFirstLinks,
                    pEndLinks,
                    (PVOID*)&pNextLinks,
                    &context,
                    LsaAdBatchBuilderBatchItemGetAttributeValue,
                    LsaAdBatchBuilderGenericFreeValueContext,
                    LsaAdBatchBuilderBatchItemNextItem,
                    dwMaxQuerySize,
                    dwMaxQueryCount,
                    &dwQueryCount,
                    &pszQuery);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppNextLinks = pNextLinks;
    *pdwQueryCount = dwQueryCount;
    *ppszQuery = pszQuery;
    return dwError;

error:
    // set output on cleanup
    pNextLinks = pFirstLinks;
    dwQueryCount = 0;
    LW_SAFE_FREE_STRING(pszQuery);
    goto cleanup;
}

DWORD
LsaAdBatchBuildQueryForPseudo(
    IN PAD_PROVIDER_DATA pProviderData,
    IN BOOLEAN bIsSchemaMode,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN PLSA_LIST_LINKS pFirstLinks,
    IN PLSA_LIST_LINKS pEndLinks,
    OUT PLSA_LIST_LINKS* ppNextLinks,
    IN DWORD dwMaxQuerySize,
    IN DWORD dwMaxQueryCount,
    OUT PDWORD pdwQueryCount,
    OUT PSTR* ppszQuery
    )
{
    return  LsaAdBatchBuildQueryForPseudoInternal(
                    pProviderData,
                    bIsSchemaMode,
                    NULL,
                    QueryType,
                    pFirstLinks,
                    pEndLinks,
                    ppNextLinks,
                    dwMaxQuerySize,
                    dwMaxQueryCount,
                    pdwQueryCount,
                    ppszQuery);
}

DWORD
LsaAdBatchBuildQueryForPseudoDefaultSchema(
    IN PAD_PROVIDER_DATA pProviderData,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN PLSA_LIST_LINKS pFirstLinks,
    IN PLSA_LIST_LINKS pEndLinks,
    OUT PLSA_LIST_LINKS* ppNextLinks,
    IN DWORD dwMaxQuerySize,
    IN DWORD dwMaxQueryCount,
    OUT PDWORD pdwQueryCount,
    OUT PSTR* ppszQuery
    )
{
    DWORD dwDirectoryMode = DEFAULT_MODE;

    return LsaAdBatchBuildQueryForPseudoInternal(
                    pProviderData,
                    TRUE,
                    &dwDirectoryMode,
                    QueryType,
                    pFirstLinks,
                    pEndLinks,
                    ppNextLinks,
                    dwMaxQuerySize,
                    dwMaxQueryCount,
                    pdwQueryCount,
                    ppszQuery);
}
