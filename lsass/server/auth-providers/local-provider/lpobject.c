/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lpobject.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Object Management Routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
LocalFindObjectByName(
    HANDLE hProvider,
    PCSTR  pszName,
    PCSTR  pszDomainName,
    PDWORD pdwObjectClass,
    PWSTR* ppwszObjectDN
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t  wszAttrNameObjectClass[] = LOCAL_DIR_ATTR_OBJECT_CLASS;
    wchar16_t wszAttrNameDN[]           = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameObjectClass[0],
        &wszAttrNameDN[0],
        NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterTemplate =
                    LOCAL_DB_DIR_ATTR_SAM_ACCOUNT_NAME " = %Q" \
                    " AND " LOCAL_DB_DIR_ATTR_DOMAIN   " = %Q";
    PWSTR pwszFilter = NULL;
    PWSTR pwszObjectDN = NULL;
    DWORD dwObjectClass = LOCAL_OBJECT_CLASS_UNKNOWN;

    dwError = DirectoryAllocateWC16StringFilterPrintf(
                    &pwszFilter,
                    pszFilterTemplate,
                    pszName,
                    pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
                    pContext->hDirectory,
                    NULL,
                    0,
                    pwszFilter,
                    wszAttrs,
                    FALSE,
                    &pEntries,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pEntry = &pEntries[0];
    if (pEntry->ulNumAttributes != dwNumAttrs)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalMarshalAttrToInteger(
                    pEntry,
                    &wszAttrNameObjectClass[0],
                    &dwObjectClass);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToUnicodeString(
                    pEntry,
                    &wszAttrNameDN[0],
                    &pwszObjectDN);
    BAIL_ON_LSA_ERROR(dwError);

    *pdwObjectClass = dwObjectClass;
    *ppwszObjectDN = pwszObjectDN;

cleanup:

    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    *pdwObjectClass = LOCAL_OBJECT_CLASS_UNKNOWN;
    *ppwszObjectDN = NULL;

    LW_SAFE_FREE_MEMORY(pwszObjectDN);

    goto cleanup;
}

static
DWORD
LocalDirResolveUserObjectPrimaryGroupSid(
    IN HANDLE hProvider,
    IN OUT PLSA_SECURITY_OBJECT pUserObject
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    static WCHAR wszAttrNameObjectSID[] = LOCAL_DIR_ATTR_OBJECT_SID;
    static PWSTR wszAttrs[] =
    {
        wszAttrNameObjectSID,
        NULL
    };
    PCSTR pszTemplate = LOCAL_DB_DIR_ATTR_GID " = %u";
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    
    if (pUserObject->type != LSA_OBJECT_TYPE_USER)
    {
        goto cleanup;
    }

    dwError = DirectoryAllocateWC16StringFilterPrintf(
        &pwszFilter,
        pszTemplate,
        pUserObject->userInfo.gid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
        pContext->hDirectory,
        NULL,
        0,
        pwszFilter,
        wszAttrs,
        FALSE,
        &pEntry,
        &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries != 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
        pEntry,
        wszAttrNameObjectSID,
        &pUserObject->userInfo.pszPrimaryGroupSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntry)
    {
        DirectoryFreeEntries(pEntry, dwNumEntries);
    }

    return dwError;

error:

    goto cleanup;
}


static
DWORD
LocalDirFindObjectsInternal(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    IN OUT PLSA_SECURITY_OBJECT* ppObjects
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    static WCHAR wszAttrNameObjectClass[]    = LOCAL_DIR_ATTR_OBJECT_CLASS;
    static WCHAR wszAttrNameUID[]            = LOCAL_DIR_ATTR_UID;
    static WCHAR wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    static WCHAR wszAttrNamePrimaryGroup[]   = LOCAL_DIR_ATTR_PRIMARY_GROUP;
    static WCHAR wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    static WCHAR wszAttrNamePassword[]       = LOCAL_DIR_ATTR_PASSWORD;
    static WCHAR wszAttrNameGecos[]          = LOCAL_DIR_ATTR_GECOS;
    static WCHAR wszAttrNameShell[]          = LOCAL_DIR_ATTR_SHELL;
    static WCHAR wszAttrNameHomedir[]        = LOCAL_DIR_ATTR_HOME_DIR;
    static WCHAR wszAttrNameUPN[]            = LOCAL_DIR_ATTR_USER_PRINCIPAL_NAME;
    static WCHAR wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    static WCHAR wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    static WCHAR wszAttrNameNetBIOSDomain[]  = LOCAL_DIR_ATTR_NETBIOS_NAME;
    static WCHAR wszAttrNameUserInfoFlags[]  = LOCAL_DIR_ATTR_ACCOUNT_FLAGS;
    static WCHAR wszAttrNameAccountExpiry[]  = LOCAL_DIR_ATTR_ACCOUNT_EXPIRY;
    static WCHAR wszAttrNamePasswdLastSet[]  = LOCAL_DIR_ATTR_PASSWORD_LAST_SET;
    static WCHAR wszAttrNameNTHash[]         = LOCAL_DIR_ATTR_NT_HASH;
    static WCHAR wszAttrNameLMHash[]         = LOCAL_DIR_ATTR_LM_HASH;
    static PWSTR wszAttrs[] =
    {
        wszAttrNameObjectClass,
        wszAttrNameUID,
        wszAttrNameGID,
        wszAttrNamePrimaryGroup,
        wszAttrNameSamAccountName,
        wszAttrNamePassword,
        wszAttrNameGecos,
        wszAttrNameShell,
        wszAttrNameHomedir,
        wszAttrNameUPN,
        wszAttrNameObjectSID,
        wszAttrNameDN,
        wszAttrNameNetBIOSDomain,
        wszAttrNameUserInfoFlags,
        wszAttrNameAccountExpiry,
        wszAttrNamePasswdLastSet,
        wszAttrNameNTHash,
        wszAttrNameLMHash,
        NULL
    };
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterTemplateQualified = 
        LOCAL_DB_DIR_ATTR_NETBIOS_NAME " = %Q" \
        " AND " LOCAL_DB_DIR_ATTR_SAM_ACCOUNT_NAME " = %Q%s";
    PCSTR pszFilterTemplateString = "%s = %Q%s";
    PCSTR pszFilterTemplateDword = "%s = %u%s";
    PCSTR pszFilterTemplateType = " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %u";
    PCSTR pszFilterTemplateUserOrGroup = " AND (" \
            LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %u OR " \
            LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %u)";
    PCSTR pszFilterBy = NULL;
    PSTR pszFilterType = NULL;
    PWSTR pwszFilter = NULL;
    DWORD dwObjectClass = LOCAL_OBJECT_CLASS_UNKNOWN;
    DWORD dwIndex = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    BOOLEAN bLocked = FALSE;
    BOOLEAN bFoundInvalidObject = FALSE;

    /* FIXME: support generic queries */
    switch (ObjectType)
    {
    case LSA_OBJECT_TYPE_UNDEFINED:
        dwObjectClass = LOCAL_OBJECT_CLASS_UNKNOWN;
        break;
    case LSA_OBJECT_TYPE_USER:
        dwObjectClass = LOCAL_OBJECT_CLASS_USER;
        break;
    case LSA_OBJECT_TYPE_GROUP:
        dwObjectClass = LOCAL_OBJECT_CLASS_GROUP;
        break;
    default:
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch (QueryType)
    {
    case LSA_QUERY_TYPE_BY_DN:
        pszFilterBy = LOCAL_DB_DIR_ATTR_DISTINGUISHED_NAME;
        break;
    case LSA_QUERY_TYPE_BY_SID:
        pszFilterBy = LOCAL_DB_DIR_ATTR_OBJECT_SID;
        break;
    case LSA_QUERY_TYPE_BY_NT4:
    case LSA_QUERY_TYPE_BY_ALIAS:
        break;
    case LSA_QUERY_TYPE_BY_UPN:
        pszFilterBy = LOCAL_DB_DIR_ATTR_USER_PRINCIPAL_NAME;
        break;
    case LSA_QUERY_TYPE_BY_UNIX_ID:
        if (dwObjectClass == LOCAL_OBJECT_CLASS_USER)
        {
            pszFilterBy = LOCAL_DB_DIR_ATTR_UID;
        }
        else
        {
            pszFilterBy = LOCAL_DB_DIR_ATTR_GID;
        }
        break;
    default:
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwObjectClass == LOCAL_OBJECT_CLASS_UNKNOWN)
    {
        dwError = LwAllocateStringPrintf(
            &pszFilterType,
            pszFilterTemplateUserOrGroup,
            LOCAL_OBJECT_CLASS_USER,
            LOCAL_OBJECT_CLASS_GROUP);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateStringPrintf(
            &pszFilterType,
            pszFilterTemplateType,
            dwObjectClass);
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        bFoundInvalidObject = FALSE;

        switch (QueryType)
        {
        case LSA_QUERY_TYPE_BY_ALIAS:
        case LSA_QUERY_TYPE_BY_NT4:
            dwError = LsaSrvCrackDomainQualifiedName(
                QueryList.ppszStrings[dwIndex],
                &pLoginInfo);
            BAIL_ON_LSA_ERROR(dwError);

            if (!pLoginInfo->pszDomain)
            {
                LOCAL_RDLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

                dwError = LwAllocateString(
                                gLPGlobals.pszNetBIOSName,
                                &pLoginInfo->pszDomain);
                BAIL_ON_LSA_ERROR(dwError);

                LOCAL_UNLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);
            }

            dwError = DirectoryAllocateWC16StringFilterPrintf(
                &pwszFilter,
                pszFilterTemplateQualified,
                pLoginInfo->pszDomain,
                pLoginInfo->pszName,
                pszFilterType ? pszFilterType : "");
            BAIL_ON_LSA_ERROR(dwError);
            break;

        case LSA_QUERY_TYPE_BY_DN:
        case LSA_QUERY_TYPE_BY_SID:
        case LSA_QUERY_TYPE_BY_UPN:
            dwError = DirectoryAllocateWC16StringFilterPrintf(
                &pwszFilter,
                pszFilterTemplateString,
                pszFilterBy,
                QueryList.ppszStrings[dwIndex],
                pszFilterType ? pszFilterType : "");
            BAIL_ON_LSA_ERROR(dwError);
            break;

        case LSA_QUERY_TYPE_BY_UNIX_ID:
            dwError = DirectoryAllocateWC16StringFilterPrintf(
                &pwszFilter,
                pszFilterTemplateDword,
                pszFilterBy,
                QueryList.pdwIds[dwIndex],
                pszFilterType ? pszFilterType : "");
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        dwError = DirectorySearch(
            pContext->hDirectory,
            NULL,
            0,
            pwszFilter,
            wszAttrs,
            FALSE,
            &pEntries,
            &dwNumEntries);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (dwNumEntries > 1)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (dwNumEntries == 1)
        {
            pEntry = &pEntries[0];
            
            dwError = LocalMarshalEntryToSecurityObject(
                pEntry,
                &ppObjects[dwIndex]);
            if (dwError)
            {
                if (dwError == LW_ERROR_NO_SUCH_OBJECT)
                {
                    bFoundInvalidObject = TRUE;
                    dwError = 0;
                }
                else
                {
                    BAIL_ON_LSA_ERROR(dwError);
                }
            }
            else
            {
                dwError = LocalDirResolveUserObjectPrimaryGroupSid(
                    hProvider,
                    ppObjects[dwIndex]);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        if ((dwNumEntries == 0 || bFoundInvalidObject) && QueryType == LSA_QUERY_TYPE_BY_UPN)
        {
            /* UPN lookup might fail because the UPN is generated, so try
               again as an NT4 lookup */
            LSA_QUERY_LIST Single;

            Single.ppszStrings = &QueryList.ppszStrings[dwIndex];
            
            dwError = LocalDirFindObjectsInternal(
                hProvider,
                FindFlags,
                ObjectType,
                LSA_QUERY_TYPE_BY_NT4,
                1,
                Single,
                &ppObjects[dwIndex]);
            BAIL_ON_LSA_ERROR(dwError);
        }

        LW_SAFE_FREE_MEMORY(pwszFilter);

        if (pEntries)
        {
            DirectoryFreeEntries(pEntries, dwNumEntries);
            pEntries = NULL;
        }

        if (pLoginInfo)
        {
            LsaSrvFreeNameInfo(pLoginInfo);
            pLoginInfo = NULL;
        }
    }

cleanup:
    LOCAL_UNLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);
    LW_SAFE_FREE_STRING(pszFilterType);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    if (pLoginInfo)
    {
        LsaSrvFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalDirFindObjects(
    IN HANDLE hProvider,
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

    dwError = LwAllocateMemory(
        sizeof(*ppObjects) * dwCount,
        OUT_PPVOID(&ppObjects));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirFindObjectsInternal(
        hProvider,
        FindFlags,
        ObjectType,
        QueryType,
        dwCount,
        QueryList,
        ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    *pppObjects = ppObjects;

cleanup:

    return dwError;

error:

    *pppObjects = NULL;

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCount, ppObjects);
    }

    goto cleanup;
}

typedef struct _LOCAL_ENUM_HANDLE
{
    HANDLE hProvider;
    enum
    {
        LOCAL_ENUM_HANDLE_OBJECTS,
        LOCAL_ENUM_HANDLE_MEMBERS
    } type;
    DWORD dwCount;
    PDIRECTORY_ENTRY pEntries;
    DWORD dwIndex;
    LONG64 llSequenceNumber;

} *LOCAL_ENUM_HANDLE, **PLOCAL_ENUM_HANDLE;

DWORD
LocalDirOpenEnumObjects(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    static WCHAR wszAttrNameObjectClass[]    = LOCAL_DIR_ATTR_OBJECT_CLASS;
    static WCHAR wszAttrNameUID[]            = LOCAL_DIR_ATTR_UID;
    static WCHAR wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    static WCHAR wszAttrNamePrimaryGroup[]   = LOCAL_DIR_ATTR_PRIMARY_GROUP;
    static WCHAR wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    static WCHAR wszAttrNamePassword[]       = LOCAL_DIR_ATTR_PASSWORD;
    static WCHAR wszAttrNameGecos[]          = LOCAL_DIR_ATTR_GECOS;
    static WCHAR wszAttrNameShell[]          = LOCAL_DIR_ATTR_SHELL;
    static WCHAR wszAttrNameHomedir[]        = LOCAL_DIR_ATTR_HOME_DIR;
    static WCHAR wszAttrNameUPN[]            = LOCAL_DIR_ATTR_USER_PRINCIPAL_NAME;
    static WCHAR wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    static WCHAR wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    static WCHAR wszAttrNameNetBIOSDomain[]  = LOCAL_DIR_ATTR_NETBIOS_NAME;
    static WCHAR wszAttrNameUserInfoFlags[]  = LOCAL_DIR_ATTR_ACCOUNT_FLAGS;
    static WCHAR wszAttrNameAccountExpiry[]  = LOCAL_DIR_ATTR_ACCOUNT_EXPIRY;
    static WCHAR wszAttrNamePasswdLastSet[]  = LOCAL_DIR_ATTR_PASSWORD_LAST_SET;
    static WCHAR wszAttrNameNTHash[]         = LOCAL_DIR_ATTR_NT_HASH;
    static WCHAR wszAttrNameLMHash[]         = LOCAL_DIR_ATTR_LM_HASH;
    static PWSTR wszAttrs[] =
    {
        wszAttrNameObjectClass,
        wszAttrNameUID,
        wszAttrNameGID,
        wszAttrNamePrimaryGroup,
        wszAttrNameSamAccountName,
        wszAttrNamePassword,
        wszAttrNameGecos,
        wszAttrNameShell,
        wszAttrNameHomedir,
        wszAttrNameUPN,
        wszAttrNameObjectSID,
        wszAttrNameDN,
        wszAttrNameNetBIOSDomain,
        wszAttrNameUserInfoFlags,
        wszAttrNameAccountExpiry,
        wszAttrNamePasswdLastSet,
        wszAttrNameNTHash,
        wszAttrNameLMHash,
        NULL
    };
    PSTR pszTypeFilter = NULL;
    PWSTR pwszFilter = NULL;
    DWORD dwObjectClass = LOCAL_OBJECT_CLASS_UNKNOWN;
    LOCAL_ENUM_HANDLE hEnum = NULL;

    dwError = LwAllocateMemory(sizeof(*hEnum), OUT_PPVOID(&hEnum));
    BAIL_ON_LSA_ERROR(dwError);

    hEnum->hProvider = hProvider;
    hEnum->type = LOCAL_ENUM_HANDLE_OBJECTS;

    switch (ObjectType)
    {
    case LSA_OBJECT_TYPE_UNDEFINED:
        dwObjectClass = LOCAL_OBJECT_CLASS_UNKNOWN;
        break;
    case LSA_OBJECT_TYPE_USER:
        dwObjectClass = LOCAL_OBJECT_CLASS_USER;
        break;
    case LSA_OBJECT_TYPE_GROUP:
        dwObjectClass = LOCAL_OBJECT_CLASS_GROUP;
        break;
    default:
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwObjectClass != LOCAL_OBJECT_CLASS_UNKNOWN)
    {
        dwError = LwAllocateStringPrintf(
            &pszTypeFilter,
            LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %u",
            dwObjectClass);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateStringPrintf(
            &pszTypeFilter,
            LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %u OR "
            LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %u",
            LOCAL_OBJECT_CLASS_USER,
            LOCAL_OBJECT_CLASS_GROUP);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszDomainName)
    {
        dwError = DirectoryAllocateWC16StringFilterPrintf(
            &pwszFilter,
            LOCAL_DB_DIR_ATTR_NETBIOS_NAME " = %Q AND (%s)",
            pszDomainName,
            pszTypeFilter);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LwMbsToWc16s(
            pszTypeFilter,
            &pwszFilter);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = DirectorySearch(
        pContext->hDirectory,
        NULL,
        0,
        pwszFilter,
        wszAttrs,
        FALSE,
        &hEnum->pEntries,
        &hEnum->dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalGetSequenceNumber(
        pContext,
        &hEnum->llSequenceNumber);
    BAIL_ON_LSA_ERROR(dwError);

    *phEnum = hEnum;
        
cleanup:
    LW_SAFE_FREE_STRING(pszTypeFilter);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    return dwError;

error:

    if (hEnum)
    {
        LocalDirCloseEnum(hEnum);
    }

    goto cleanup;
}

DWORD
LocalDirEnumObjects(
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    LOCAL_ENUM_HANDLE pEnum = (LOCAL_ENUM_HANDLE) hEnum;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT) pEnum->hProvider;
    LONG64 llSequenceNumber = 0;
    DWORD dwAllocCount = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwIndex = 0;
    PDIRECTORY_ENTRY pEntry = NULL;

    if (pEnum->dwIndex >= pEnum->dwCount)
    {
        dwError = ERROR_NO_MORE_ITEMS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalGetSequenceNumber(
        pContext,
        &llSequenceNumber);
    BAIL_ON_LSA_ERROR(dwError);

    if (llSequenceNumber != pEnum->llSequenceNumber)
    {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwAllocCount = dwMaxObjectsCount;
    if (dwAllocCount > pEnum->dwCount - pEnum->dwIndex)
    {
        dwAllocCount = pEnum->dwCount - pEnum->dwIndex;
    }

    dwError = LwAllocateMemory(sizeof(*ppObjects) * dwAllocCount, OUT_PPVOID(&ppObjects));
    BAIL_ON_LSA_ERROR(dwError);

    for(dwIndex = 0; dwIndex < dwAllocCount; dwIndex++)
    {
        pEntry = &pEnum->pEntries[pEnum->dwIndex++];

        dwError = LocalMarshalEntryToSecurityObject(
            pEntry,
            &ppObjects[dwIndex]);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LocalDirResolveUserObjectPrimaryGroupSid(
            pEnum->hProvider,
            ppObjects[dwIndex]);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pdwObjectsCount = dwAllocCount;
    *pppObjects = ppObjects;

cleanup:

    return dwError;

error:

    *pdwObjectsCount = 0;
    *pppObjects = NULL;

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwAllocCount, ppObjects);
    }

    goto cleanup;
}

VOID
LocalDirCloseEnum(
    IN OUT HANDLE hEnum
    )
{
    LOCAL_ENUM_HANDLE pEnum = (LOCAL_ENUM_HANDLE) hEnum;

    if (pEnum)
    {
        if (pEnum->pEntries)
        {
            DirectoryFreeEntries(pEnum->pEntries, pEnum->dwCount);
        }

        LwFreeMemory(pEnum);
    }
}

DWORD
LocalDirOpenEnumMembers(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    static WCHAR wszAttrNameObjectClass[]    = LOCAL_DIR_ATTR_OBJECT_CLASS;
    static WCHAR wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    static WCHAR wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    static PWSTR wszResolveAttrs[] =
    {
        wszAttrNameObjectClass,
        wszAttrNameDN,
        NULL
    };
    static PWSTR wszEnumAttrs[] =
    {
        wszAttrNameObjectSID,
        NULL
    };

    PWSTR pwszFilter = NULL;
    LOCAL_ENUM_HANDLE hEnum = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PWSTR pwszDN = NULL;

    dwError = LwAllocateMemory(sizeof(*hEnum), OUT_PPVOID(&hEnum));
    BAIL_ON_LSA_ERROR(dwError);

    hEnum->hProvider = hProvider;
    hEnum->type = LOCAL_ENUM_HANDLE_MEMBERS;

    dwError = DirectoryAllocateWC16StringFilterPrintf(
        &pwszFilter,
        LOCAL_DB_DIR_ATTR_OBJECT_SID " = %Q",
        pszSid);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = DirectorySearch(
        pContext->hDirectory,
        NULL,
        0,
        pwszFilter,
        wszResolveAttrs,
        FALSE,
        &pEntry,
        &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwNumEntries > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        /* FIXME: ensure that sid resolved to a group */
        dwError = LocalMarshalAttrToUnicodeString(
            pEntry,
            wszAttrNameDN,
            &pwszDN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetGroupMembers(
            pContext->hDirectory,
            pwszDN,
            wszEnumAttrs,
            &hEnum->pEntries,
            &hEnum->dwCount);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LocalGetSequenceNumber(
            pContext,
            &hEnum->llSequenceNumber);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *phEnum = hEnum;
        
cleanup:

    LW_SAFE_FREE_MEMORY(pwszFilter);
    LW_SAFE_FREE_MEMORY(pwszDN);

    if (pEntry)
    {
        DirectoryFreeEntries(pEntry, dwNumEntries);
    }

    return dwError;

error:

    if (hEnum)
    {
        LocalDirCloseEnum(hEnum);
    }

    goto cleanup;
}

DWORD
LocalDirEnumMembers(
    IN HANDLE hEnum,
    IN DWORD dwMaxMemberSidCount,
    OUT PDWORD pdwMemberSidCount,
    OUT PSTR** pppszMemberSids
    )
{
    DWORD dwError = 0;
    LOCAL_ENUM_HANDLE pEnum = (LOCAL_ENUM_HANDLE) hEnum;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT) pEnum->hProvider;
    LONG64 llSequenceNumber = 0;
    DWORD dwAllocCount = 0;
    PSTR* ppszMemberSids = NULL;
    DWORD dwIndex = 0;
    PDIRECTORY_ENTRY pEntry = NULL;
    static WCHAR wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;

    if (pEnum->dwIndex >= pEnum->dwCount)
    {
        dwError = ERROR_NO_MORE_ITEMS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalGetSequenceNumber(
        pContext,
        &llSequenceNumber);
    BAIL_ON_LSA_ERROR(dwError);

    if (llSequenceNumber != pEnum->llSequenceNumber)
    {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwAllocCount = dwMaxMemberSidCount;
    if (dwAllocCount > pEnum->dwCount - pEnum->dwIndex)
    {
        dwAllocCount = pEnum->dwCount - pEnum->dwIndex;
    }

    dwError = LwAllocateMemory(sizeof(*ppszMemberSids) * dwAllocCount, OUT_PPVOID(&ppszMemberSids));
    BAIL_ON_LSA_ERROR(dwError);

    for(dwIndex = 0; dwIndex < dwAllocCount; dwIndex++)
    {
        pEntry = &pEnum->pEntries[pEnum->dwIndex++];

        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            pEntry,
            wszAttrNameObjectSID,
            &ppszMemberSids[dwIndex]);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pdwMemberSidCount = dwAllocCount;
    *pppszMemberSids = ppszMemberSids;

cleanup:

    return dwError;

error:

    *pdwMemberSidCount = 0;
    *pppszMemberSids = NULL;

    if (ppszMemberSids)
    {
        LwFreeStringArray(ppszMemberSids, dwAllocCount);
    }

    goto cleanup;
}

static
DWORD
LocalDirQueryMemberOfInternal(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN PSTR pszSid,
    IN OUT PLW_HASH_TABLE pGroupHash
    );

static
DWORD
LocalDirQueryMemberOfDN(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN PWSTR pwszDN,
    IN OUT PLW_HASH_TABLE pGroupHash
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    static WCHAR wszAttrNameObjectSID[] = LOCAL_DIR_ATTR_OBJECT_SID;
    static PWSTR wszMemberAttrs[] =
    {
        wszAttrNameObjectSID,
        NULL
    };
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwNumEntries = 0;
    PSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    DWORD dwIndex = 0;
    PSTR pszGroupSid = NULL;
    PSTR pszPreviousGroupSid = NULL;

    dwError = DirectoryGetMemberships(
        pContext->hDirectory,
        pwszDN,
        wszMemberAttrs,
        &pEntries,
        &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);
            
    for (dwIndex = 0; dwIndex < dwNumEntries; dwIndex++)
    {
        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            &pEntries[dwIndex],
            wszAttrNameObjectSID,
            &pszGroupSid);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LwHashGetValue(
            pGroupHash,
            pszGroupSid,
            OUT_PPVOID(&pszPreviousGroupSid));
        if (dwError == ERROR_NOT_FOUND)
        {
            dwError = LwHashSetValue(
                pGroupHash,
                pszGroupSid,
                pszGroupSid);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwError = LocalDirQueryMemberOfInternal(
                hProvider,
                FindFlags,
                pszGroupSid,
                pGroupHash);
            pszGroupSid = NULL;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        LW_SAFE_FREE_MEMORY(pszGroupSid);
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pwszFilter);
    LW_SAFE_FREE_MEMORY(pszFilter);
    
    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LocalDirQueryMemberOfInternal(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN PSTR pszSid,
    IN OUT PLW_HASH_TABLE pGroupHash
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    static WCHAR wszAttrNameDN[] = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    static PWSTR wszResolveAttrs[] =
    {
        wszAttrNameDN,
        NULL
    };
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwNumEntries = 0;
    PWSTR pwszFilter = NULL;
    PWSTR pwszDN = NULL;

    dwError = DirectoryAllocateWC16StringFilterPrintf(
        &pwszFilter,
        LOCAL_DB_DIR_ATTR_OBJECT_SID " = %Q",
        pszSid);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = DirectorySearch(
        pContext->hDirectory,
        NULL,
        0,
        pwszFilter,
        wszResolveAttrs,
        FALSE,
        &pEntries,
        &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (dwNumEntries > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwNumEntries == 1)
    {
        dwError = LocalMarshalAttrToUnicodeString(
            pEntries,
            wszAttrNameDN,
            &pwszDN);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalDirQueryMemberOfDN(
            hProvider,
            FindFlags,
            pwszDN,
            pGroupHash);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
cleanup:

    LW_SAFE_FREE_MEMORY(pwszDN);
    LW_SAFE_FREE_MEMORY(pwszFilter);
    
    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    if (dwError == LW_ERROR_NO_SUCH_USER ||
        dwError == LW_ERROR_NO_SUCH_GROUP ||
        dwError == LW_ERROR_NO_SUCH_OBJECT)
    {
        dwError = 0;
    }

    return dwError;

error:

    goto cleanup;
}


DWORD
LocalDirQueryMemberOf(
    IN HANDLE hProvider,
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
                    NULL,
                    NULL,
                    &pGroupHash);
    BAIL_ON_LSA_ERROR(dwError);

    for (dwIndex = 0; dwIndex < dwSidCount; dwIndex++)
    {
        dwError = LocalDirQueryMemberOfInternal(
            hProvider,
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

    if (pGroupHash)
    {
        if (LwHashGetIterator(pGroupHash, &hashIterator) == 0)
        {
            while ((pHashEntry = LwHashNext(&hashIterator)))
            {
                LW_SAFE_FREE_MEMORY(pHashEntry->pValue);
            }
        }

        LwHashSafeFree(&pGroupHash);
    }

    return dwError;

error:

    *pdwGroupSidCount = 0;
    *pppszGroupSids = NULL;

    if (ppszGroupSids)
    {
        LwFreeStringArray(ppszGroupSids, dwGroupSidCount);
    }

    goto cleanup;
}

DWORD
LocalDirFindObjectByGenericName(
    HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    PCSTR pszName,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    LSA_QUERY_TYPE QueryType = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    BAIL_ON_INVALID_HANDLE(hProvider);

    dwError = LsaSrvCrackDomainQualifiedName(
        pszName,
        &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pLoginInfo->nameType)
    {
    case NameType_NT4:
        QueryType = LSA_QUERY_TYPE_BY_NT4;
        break;
    case NameType_UPN:
        QueryType = LSA_QUERY_TYPE_BY_UPN;
        break;
    case NameType_Alias:
        QueryType = LSA_QUERY_TYPE_BY_ALIAS;
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    QueryList.ppszStrings = &pszName;

    dwError = LocalFindObjects(
        hProvider,
        FindFlags,
        ObjectType,
        QueryType,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppObjects[0] == NULL)
    {
        switch (ObjectType)
        {
        case LSA_OBJECT_TYPE_USER:
            dwError = LW_ERROR_NO_SUCH_USER;
            break;
        case LSA_OBJECT_TYPE_GROUP:
            dwError = LW_ERROR_NO_SUCH_GROUP;
            break;
        default:
            dwError = LW_ERROR_NO_SUCH_OBJECT;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppObject = ppObjects[0];
    ppObjects[0] = NULL;

cleanup:

    if (pLoginInfo)
    {
        LsaSrvFreeNameInfo(pLoginInfo);
    }

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    return dwError;

error:

    goto cleanup;
}
