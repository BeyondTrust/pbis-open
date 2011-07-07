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
 *        lsassdb.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Directory User Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

static
DWORD
LocalDirValidateUID(
    uid_t uid
    );

static
DWORD
LocalDirGetBuiltinAdministratorsSid(
    HANDLE hProvider,
    PSTR* ppszSid
    );

DWORD
LocalDirGetUserInfoFlags(
    HANDLE hProvider,
    uid_t  uid,
    PDWORD pdwUserInfoFlags
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameUserInfoFlags[]  = LOCAL_DIR_ATTR_ACCOUNT_FLAGS;
    PWSTR pwszAttrs[] =
    {
            &wszAttrNameUserInfoFlags[0],
            NULL
    };
    DWORD dwNumAttrs = (sizeof(pwszAttrs)/sizeof(pwszAttrs[0])) - 1;
    PCSTR pszFilterTemplate =
                        LOCAL_DB_DIR_ATTR_UID " = %u" \
                        " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %u";
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwUserInfoFlags = 0;

    dwError = DirectoryAllocateWC16StringFilterPrintf(
                    &pwszFilter,
                    pszFilterTemplate,
                    uid,
                    LOCAL_OBJECT_CLASS_USER);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
                    pContext->hDirectory,
                    NULL,
                    0,
                    pwszFilter,
                    pwszAttrs,
                    FALSE,
                    &pEntries,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
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
                    &wszAttrNameUserInfoFlags[0],
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);

    *pdwUserInfoFlags = dwUserInfoFlags;

cleanup:

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    LW_SAFE_FREE_MEMORY(pwszFilter);

    return dwError;

error:

    *pdwUserInfoFlags = 0;

    goto cleanup;
}

DWORD
LocalDirAddUser(
    HANDLE           hProvider,
    PLSA_USER_ADD_INFO pUserInfo
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    BOOLEAN bEventlogEnabled = FALSE;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PWSTR pwszUserDN = NULL;
    PSTR pszUserDn = NULL;
    enum AttrValueIndex {
        LOCAL_DAU0_IDX_GID = 0,
        LOCAL_DAU0_IDX_OBJECTCLASS,
        LOCAL_DAU0_IDX_SAM_ACCOUNT_NAME,
        LOCAL_DAU0_IDX_COMMON_NAME,
        LOCAL_DAU0_IDX_GECOS,
        LOCAL_DAU0_IDX_SHELL,
        LOCAL_DAU0_IDX_HOMEDIR,
        LOCAL_DAU0_IDX_OBJECTSID,
        LOCAL_DAU0_IDX_DOMAIN,
        LOCAL_DAU0_IDX_NETBIOS_DOMAIN,
        LOCAL_DAU0_IDX_UID,
        LOCAL_DAU0_IDX_ACCOUNT_FLAGS,
        LOCAL_DAU0_IDX_SENTINEL
    };
    ATTRIBUTE_VALUE attrValues[] =
    {
            {       /* LOCAL_DIR_ADD_USER_0_IDX_GID */
                    .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                    .data.ulValue = -1
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_OBJECTCLASS */
                    .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                    .data.ulValue = LOCAL_OBJECT_CLASS_USER
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_SAM_ACCOUNT_NAME */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_COMMON_NAME */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_GECOS */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_SHELL */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {        /* LOCAL_DIR_ADD_USER_0_IDX_HOMEDIR */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_OBJECTSID */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_DOMAIN */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_NETBIOS_DOMAIN */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_UID */
                    .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                    .data.ulValue = pUserInfo->uid
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_ACCOUNT_FLAGS */
                    .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                    .data.ulValue = 0
            }
    };
    WCHAR wszAttrObjectClass[]       = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrDistinguishedName[] = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectSID[]         = LOCAL_DIR_ATTR_OBJECT_SID;
    WCHAR wszAttrGID[]               = LOCAL_DIR_ATTR_PRIMARY_GROUP;
    WCHAR wszAttrSamAccountName[]    = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrCommonName[]        = LOCAL_DIR_ATTR_COMMON_NAME;
    WCHAR wszAttrGecos[]             = LOCAL_DIR_ATTR_GECOS;
    WCHAR wszAttrShell[]             = LOCAL_DIR_ATTR_SHELL;
    WCHAR wszAttrHomedir[]           = LOCAL_DIR_ATTR_HOME_DIR;
    WCHAR wszAttrDomain[]            = LOCAL_DIR_ATTR_DOMAIN;
    WCHAR wszAttrNameNetBIOSDomain[] = LOCAL_DIR_ATTR_NETBIOS_NAME;
    WCHAR wszAttrNameUID[]           = LOCAL_DIR_ATTR_UID;
    WCHAR wszAttrNameAccountFlags[]  = LOCAL_DIR_ATTR_ACCOUNT_FLAGS;
    DIRECTORY_MOD modGID =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrGID[0],
        1,
        &attrValues[LOCAL_DAU0_IDX_GID]
    };
    DIRECTORY_MOD modObjectClass =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrObjectClass[0],
        1,
        &attrValues[LOCAL_DAU0_IDX_OBJECTCLASS]
    };
    DIRECTORY_MOD modSamAccountName =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrSamAccountName[0],
        1,
        &attrValues[LOCAL_DAU0_IDX_SAM_ACCOUNT_NAME]
    };
    DIRECTORY_MOD modCommonName =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrCommonName[0],
        1,
        &attrValues[LOCAL_DAU0_IDX_COMMON_NAME]
    };
    DIRECTORY_MOD modGecos =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrGecos[0],
        1,
        &attrValues[LOCAL_DAU0_IDX_GECOS]
    };
    DIRECTORY_MOD modShell =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrShell[0],
        1,
        &attrValues[LOCAL_DAU0_IDX_SHELL]
    };
    DIRECTORY_MOD modHomedir =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrHomedir[0],
        1,
        &attrValues[LOCAL_DAU0_IDX_HOMEDIR]
    };
    DIRECTORY_MOD modDomain =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrDomain[0],
        1,
        &attrValues[LOCAL_DAU0_IDX_DOMAIN]
    };
    DIRECTORY_MOD modNetBIOSDomain =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrNameNetBIOSDomain[0],
        1,
        &attrValues[LOCAL_DAU0_IDX_NETBIOS_DOMAIN]
    };
    DIRECTORY_MOD modUID =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrNameUID[0],
        1,
        &attrValues[LOCAL_DAU0_IDX_UID]
    };
    DIRECTORY_MOD modAcctFlags =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrNameAccountFlags[0],
        1,
        &attrValues[LOCAL_DAU0_IDX_ACCOUNT_FLAGS]
    };
    DIRECTORY_MOD mods[LOCAL_DAU0_IDX_SENTINEL + 1];

    DWORD iMod = 0;
    PWSTR pwszSamAccountName = NULL;
    PWSTR pwszGecos = NULL;
    PWSTR pwszShell = NULL;
    PSTR  pszShell  = NULL;
    PWSTR pwszHomedir = NULL;
    PSTR  pszHomedir = NULL;
    PCSTR pszPassword = "";
    PWSTR pwszPassword = NULL;
    PWSTR pwszDomain        = NULL;
    PWSTR pwszNetBIOSDomain = NULL;
    PWSTR pwszGroupDN = NULL;
    PWSTR pwszBase = NULL;
    ULONG ulScope = 0;
    PCSTR filterFormat = LOCAL_DB_DIR_ATTR_DISTINGUISHED_NAME " = %Q";
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pMember = NULL;
    DWORD dwNumEntries = 0;
    BOOLEAN bUserAdded = FALSE;
    PWSTR pwszUserSID = NULL;
    PSID pUserSID = NULL;
    DWORD dwUserRID = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    gid_t gid = 0;
    PSID pGroupSID = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    DWORD dwGid = -1;
    BOOLEAN bLocked = FALSE;

    PWSTR wszMemberAttrs[] = {
        wszAttrObjectClass,
        wszAttrDistinguishedName,
        wszAttrObjectSID,
        wszAttrGID,
        NULL
    };

    memset(&mods[0], 0, sizeof(mods));

    BAIL_ON_INVALID_STRING(pUserInfo->pszName);

    if (pUserInfo->uid) {
        dwError = LocalDirValidateUID(pUserInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvCrackDomainQualifiedName(
                    pUserInfo->pszName,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    LOCAL_RDLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    if (!pLoginInfo->pszDomain)
    {
        dwError = LwAllocateString(
                        gLPGlobals.pszNetBIOSName,
                        &pLoginInfo->pszDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LocalServicesDomainInternal(pLoginInfo->pszDomain))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->pszPrimaryGroupSid)
    {
        QueryList.ppszStrings = (PCSTR*) &pUserInfo->pszPrimaryGroupSid;

        dwError = LocalDirFindObjects(
            hProvider,
            0,
            LSA_OBJECT_TYPE_GROUP,
            LSA_QUERY_TYPE_BY_SID,
            1,
            QueryList,
            &ppObjects);

        if (ppObjects[0] == NULL)
        {
            dwError = LW_ERROR_NO_SUCH_GROUP;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = RtlAllocateSidFromCString(
                        &pGroupSID,
                        ppObjects[0]->pszObjectSid);
        BAIL_ON_LSA_ERROR(dwError);

        if (!RtlIsPrefixSid(
                        gLPGlobals.pLocalDomainSID,
                        pGroupSID))
        {
            dwError = LW_ERROR_INVALID_GROUP;
            BAIL_ON_LSA_ERROR(dwError);
        }

        attrValues[LOCAL_DAU0_IDX_GID].data.ulValue = ppObjects[0]->groupInfo.gid;

        LsaUtilFreeSecurityObjectList(1, ppObjects);
        ppObjects = NULL;

        RTL_FREE(&pGroupSID);
    }

    dwError = LwMbsToWc16s(
                    pLoginInfo->pszDomain,
                    &pwszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAU0_IDX_DOMAIN].data.pwszStringValue = pwszDomain;

    dwError = LwMbsToWc16s(
                    pLoginInfo->pszDomain,
                    &pwszNetBIOSDomain);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAU0_IDX_NETBIOS_DOMAIN].data.pwszStringValue = pwszNetBIOSDomain;

    dwError = LwMbsToWc16s(
                    pLoginInfo->pszName,
                    &pwszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAU0_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue = pwszSamAccountName;

    attrValues[LOCAL_DAU0_IDX_COMMON_NAME].data.pwszStringValue = pwszSamAccountName;

    dwError = LocalBuildDN(
                    pLoginInfo,
                    &pwszUserDN);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszGecos))
    {
        dwError = LwMbsToWc16s(
                    pUserInfo->pszGecos,
                    &pwszGecos);
        BAIL_ON_LSA_ERROR(dwError);

        attrValues[LOCAL_DAU0_IDX_GECOS].data.pwszStringValue = pwszGecos;
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszHomedir))
    {
        dwError = LwMbsToWc16s(
                    pUserInfo->pszHomedir,
                    &pwszHomedir);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LocalBuildHomeDirPathFromTemplate(
                        pLoginInfo->pszName,
                        pLoginInfo->pszDomain,
                        &pszHomedir);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pszHomedir,
                        &pwszHomedir);
        BAIL_ON_LSA_ERROR(dwError);
    }

    attrValues[LOCAL_DAU0_IDX_HOMEDIR].data.pwszStringValue = pwszHomedir;

    if (!LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszShell))
    {
        dwError = LwMbsToWc16s(
                    pUserInfo->pszShell,
                    &pwszShell);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LocalCfgGetDefaultShell(&pszShell);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwMbsToWc16s(
                    pszShell,
                    &pwszShell);
        BAIL_ON_LSA_ERROR(dwError);
    }

    attrValues[LOCAL_DAU0_IDX_SHELL].data.pwszStringValue = pwszShell;

    attrValues[LOCAL_DAU0_IDX_ACCOUNT_FLAGS].data.ulValue = LOCAL_ACB_NORMAL;

    if (LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszPassword))
    {
        attrValues[LOCAL_DAU0_IDX_ACCOUNT_FLAGS].data.ulValue |= LOCAL_ACB_DISABLED;
    }

    mods[iMod++] = modObjectClass;
    if (pUserInfo->uid)
    {
        mods[iMod++] = modUID;
    }
    if (pUserInfo->pszPrimaryGroupSid)
    {
        mods[iMod++] = modGID;
    }
    mods[iMod++] = modSamAccountName;
    mods[iMod++] = modCommonName;
    mods[iMod++] = modGecos;
    mods[iMod++] = modShell;
    mods[iMod++] = modHomedir;
    mods[iMod++] = modDomain;
    mods[iMod++] = modNetBIOSDomain;
    mods[iMod++] = modAcctFlags;

    dwError = DirectoryAddObject(
                    pContext->hDirectory,
                    pwszUserDN,
                    mods);
    BAIL_ON_LSA_ERROR(dwError);

    bUserAdded = TRUE;

    dwError = LwWc16sToMbs(
                    pwszUserDN,
                    &pszUserDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryAllocateWC16StringFilterPrintf(
                    &pwszFilter,
                    filterFormat,
                    pszUserDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
                    pContext->hDirectory,
                    pwszBase,
                    ulScope,
                    pwszFilter,
                    wszMemberAttrs,
                    0,
                    &pMember,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Set default security descriptor on the account
     */
    dwError = DirectoryGetEntryAttrValueByName(
                    pMember,
                    wszAttrObjectSID,
                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    &pwszUserSID);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlAllocateSidFromWC16String(&pUserSID,
                                            pwszUserSID);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlGetRidSid(&dwUserRID,
                            pUserSID);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LocalDirCreateNewAccountSecurityDescriptor(
                    gLPGlobals.pLocalDomainSID,
                    dwUserRID,
                    DIR_OBJECT_CLASS_USER,
                    &pSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySetEntrySecurityDescriptor(
                                  pContext->hDirectory,
                                  pwszUserDN,
                                  pSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Add user to primary group
     */
    if (!pwszGroupDN)
    {
        dwError = DirectoryGetEntryAttrValueByName(
                        pMember,
                        wszAttrGID,
                        DIRECTORY_ATTR_TYPE_INTEGER,
                        &gid);
        BAIL_ON_LSA_ERROR(dwError);

        dwGid = (DWORD) gid;
        QueryList.pdwIds = &dwGid;

        dwError = LocalDirFindObjects(
            hProvider,
            0,
            LSA_OBJECT_TYPE_GROUP,
            LSA_QUERY_TYPE_BY_UNIX_ID,
            1,
            QueryList,
            &ppObjects);

        if (ppObjects[0] == NULL)
        {
            dwError = LW_ERROR_NO_SUCH_GROUP;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LwMbsToWc16s(
            ppObjects[0]->pszDN,
            &pwszGroupDN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = RtlAllocateSidFromCString(
                        &pGroupSID,
                        ppObjects[0]->pszObjectSid);
        BAIL_ON_LSA_ERROR(dwError);

        if (!RtlIsPrefixSid(
                        gLPGlobals.pLocalDomainSID,
                        pGroupSID))
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        RTL_FREE(&pGroupSID);
    }

    dwError = DirectoryAddToGroup(
                    pContext->hDirectory,
                    pwszGroupDN,
                    pMember);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Set user password
     */

    if (pUserInfo->pszPassword)
    {
        pszPassword = pUserInfo->pszPassword;
    }

    dwError = LwMbsToWc16s(
                    pszPassword,
                    &pwszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySetPassword(
                    pContext->hDirectory,
                    pwszUserDN,
                    pwszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCfgIsEventlogEnabled(&bEventlogEnabled);
    BAIL_ON_LSA_ERROR(dwError);

    if (bEventlogEnabled)
    {
        LocalEventLogUserAdd(pLoginInfo->pszName,
                             pUserInfo->uid);
    }

cleanup:
    LOCAL_UNLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    if (pLoginInfo)
    {
        LsaSrvFreeNameInfo(pLoginInfo);
    }

    LW_SAFE_FREE_MEMORY(pszUserDn);
    LW_SAFE_FREE_MEMORY(pwszUserDN);
    LW_SAFE_FREE_MEMORY(pwszGroupDN);
    LW_SAFE_FREE_MEMORY(pwszSamAccountName);
    LW_SAFE_FREE_MEMORY(pwszGecos);
    LW_SAFE_FREE_MEMORY(pwszShell);
    LW_SAFE_FREE_STRING(pszShell);
    LW_SAFE_FREE_MEMORY(pwszHomedir);
    LW_SAFE_FREE_STRING(pszHomedir);
    LW_SECURE_FREE_WSTRING(pwszPassword);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszNetBIOSDomain);
    RTL_FREE(&pGroupSID);
    RTL_FREE(&pUserSID);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (pMember)
    {
        DirectoryFreeEntries(pMember, dwNumEntries);
    }

    LocalDirFreeSecurityDescriptor(&pSecDesc);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:

    if (bUserAdded)
    {
        DWORD dwError2 = 0;

        dwError2 = DirectoryDeleteObject(
                        pContext->hDirectory,
                        pwszUserDN);
        if (dwError2)
        {
            LSA_LOG_ERROR("Failed to remove user [code: %u]", dwError2);
        }
    }

    goto cleanup;
}

static
DWORD
LocalDirValidateUID(
    uid_t uid
    )
{
    DWORD dwError = 0;

    /* Check whether account uid is within permitted range */
    if (uid < LOWEST_UID) {
        BAIL_WITH_LSA_ERROR(LW_ERROR_UID_TOO_LOW);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LocalDirModifyUser(
    HANDLE             hProvider,
    PLSA_USER_MOD_INFO_2 pUserModInfo
    )
{
    DWORD   dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    PWSTR   pwszUserDN = NULL;
    DWORD   dwUserInfoFlags = 0;
    DWORD   dwOrigUserInfoFlags = 0;
    WCHAR   wszAttrDN[] = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    DIRECTORY_MOD mods[10] = {{0}};

    ATTRIBUTE_VALUE UserAttrVal[] = {
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        }
    };

    DIRECTORY_ATTRIBUTE UserAttr[] = {
        {
            /* placeholder for DN */
            .pwszName    = wszAttrDN,
            .ulNumValues = 1,
            .pValues     = UserAttrVal
        },
        {
            /* termination */
            NULL,
            0,
            NULL
        }
    };

    DIRECTORY_ENTRY UserEntry[] = {
        { 1, UserAttr },
        { 0, NULL }
    };

    WCHAR wszAttrNameDn[] = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrNameLastPasswordSet[] = LOCAL_DIR_ATTR_PASSWORD_LAST_SET;
    WCHAR wszAttrNameUserInfoFlags[] = LOCAL_DIR_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrNameAccountExpiry[] = LOCAL_DIR_ATTR_ACCOUNT_EXPIRY;
    WCHAR wszAttrNameNtHash [] = LOCAL_DIR_ATTR_NT_HASH;
    WCHAR wszAttrNameLmHash [] = LOCAL_DIR_ATTR_LM_HASH;
    WCHAR wszAttrNamePrimaryGroup [] = LOCAL_DIR_ATTR_PRIMARY_GROUP;
    WCHAR wszAttrNameShell [] = LOCAL_DIR_ATTR_SHELL;
    WCHAR wszAttrNameGecos [] = LOCAL_DIR_ATTR_GECOS;
    WCHAR wszAttrNameHomedir [] = LOCAL_DIR_ATTR_HOME_DIR;
    ATTRIBUTE_VALUE avLastPasswordChange = {0};
    ATTRIBUTE_VALUE avUserInfoFlags = {0};
    ATTRIBUTE_VALUE avAccountExpiry = {0};
    ATTRIBUTE_VALUE avNtHash = {0};
    ATTRIBUTE_VALUE avLmHash = {0};
    ATTRIBUTE_VALUE avPrimaryGroup = {0};
    ATTRIBUTE_VALUE avShell = {0};
    ATTRIBUTE_VALUE avGecos = {0};
    ATTRIBUTE_VALUE avHomedir = {0};
    DWORD dwNumMods = 0;
    PWSTR pwszPrimaryGroupDn = NULL;
    PDIRECTORY_ENTRY pLocalGroupMemberships = NULL;
    DWORD dwNumLocalGroups = 0;
    DWORD iGroup = 0;
    PDIRECTORY_ENTRY pLocalGroupEntry = NULL;
    PWSTR pwszLocalGroupDn  = NULL;
    BOOLEAN bIsMember = FALSE;
    PWSTR pwszGroupDN_remove = NULL;
    PWSTR pwszGroupDN_add   = NULL;
    PWSTR pwszPassword      = NULL;
    OCTET_STRING NtHashBlob = {0};
    OCTET_STRING LmHashBlob = {0};
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    PWSTR wszMembershipAttributes[] = {
        wszAttrNameDn,
        NULL
    };

    QueryList.ppszStrings = (PCSTR*) &pUserModInfo->pszSid;
        
    dwError = LocalDirFindObjects(
        hProvider,
        0,
        LSA_OBJECT_TYPE_USER,
        LSA_QUERY_TYPE_BY_SID,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);
        
    if (ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LwMbsToWc16s(
        ppObjects[0]->pszDN,
        &pwszUserDN);
    BAIL_ON_LSA_ERROR(dwError);

    UserAttr[0].pValues[0].data.pwszStringValue = pwszUserDN;

    dwError = LocalDirGetUserInfoFlags(
                    hProvider,
                    ppObjects[0]->userInfo.uid,
                    &dwOrigUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);

    LsaUtilFreeSecurityObjectList(1, ppObjects);
    ppObjects = NULL;

    dwUserInfoFlags = dwOrigUserInfoFlags;

    if (pUserModInfo->actions.bEnableUser)
    {
        dwUserInfoFlags &= ~LOCAL_ACB_DISABLED;
    }
    else if (pUserModInfo->actions.bDisableUser)
    {
        dwUserInfoFlags |= LOCAL_ACB_DISABLED;
    }

    if (pUserModInfo->actions.bSetPasswordMustExpire)
    {
        dwUserInfoFlags &= ~LOCAL_ACB_PWNOEXP;
    }
    else if (pUserModInfo->actions.bSetPasswordNeverExpires)
    {
        dwUserInfoFlags |= LOCAL_ACB_PWNOEXP;
    }

    if (pUserModInfo->actions.bSetChangePasswordOnNextLogon)
    {
        mods[dwNumMods].ulOperationFlags = DIR_MOD_FLAGS_REPLACE;
        mods[dwNumMods].pwszAttrName = &wszAttrNameLastPasswordSet[0];
        mods[dwNumMods].ulNumValues = 1;
        avLastPasswordChange.Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;
        avLastPasswordChange.data.llValue = 0;
        mods[dwNumMods].pAttrValues = &avLastPasswordChange;

        dwNumMods++;
    }

    if (pUserModInfo->actions.bUnlockUser)
    {
        dwUserInfoFlags &= ~LOCAL_ACB_NORMAL;
    }

    if (dwUserInfoFlags != dwOrigUserInfoFlags)
    {
        mods[dwNumMods].ulOperationFlags = DIR_MOD_FLAGS_REPLACE;
        mods[dwNumMods].pwszAttrName = &wszAttrNameUserInfoFlags[0];
        mods[dwNumMods].ulNumValues = 1;
        avUserInfoFlags.Type = DIRECTORY_ATTR_TYPE_INTEGER;
        avUserInfoFlags.data.ulValue = dwUserInfoFlags;
        mods[dwNumMods].pAttrValues = &avUserInfoFlags;

        dwNumMods++;
    }

    if (pUserModInfo->actions.bSetAccountExpiryDate)
    {
        struct tm tmbuf = {0};

        if (strptime(pUserModInfo->pszExpiryDate, "%Y-%m-%d", &tmbuf) == NULL)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }

        mods[dwNumMods].ulOperationFlags = DIR_MOD_FLAGS_REPLACE;
        mods[dwNumMods].pwszAttrName = &wszAttrNameAccountExpiry[0];
        mods[dwNumMods].ulNumValues = 1;
        avAccountExpiry.Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;
        avAccountExpiry.data.llValue = LocalGetNTTime(mktime(&tmbuf));
        mods[dwNumMods].pAttrValues = &avAccountExpiry;

        dwNumMods++;
    }

    if (pUserModInfo->actions.bSetNtPasswordHash)
    {
        NtHashBlob.ulNumBytes = pUserModInfo->pNtPasswordHash->dwLen;
        NtHashBlob.pBytes     = pUserModInfo->pNtPasswordHash->pData;

        mods[dwNumMods].ulOperationFlags = DIR_MOD_FLAGS_REPLACE;
        mods[dwNumMods].pwszAttrName = &wszAttrNameNtHash[0];
        mods[dwNumMods].ulNumValues = 1;
        avNtHash.Type = DIRECTORY_ATTR_TYPE_OCTET_STREAM;
        avNtHash.data.pOctetString = &NtHashBlob;
        mods[dwNumMods].pAttrValues = &avNtHash;

        dwNumMods++;
    }

    if (pUserModInfo->actions.bSetLmPasswordHash)
    {
        LmHashBlob.ulNumBytes = pUserModInfo->pLmPasswordHash->dwLen;
        LmHashBlob.pBytes     = pUserModInfo->pLmPasswordHash->pData;

        mods[dwNumMods].ulOperationFlags = DIR_MOD_FLAGS_REPLACE;
        mods[dwNumMods].pwszAttrName = &wszAttrNameLmHash[0];
        mods[dwNumMods].ulNumValues = 1;
        avLmHash.Type = DIRECTORY_ATTR_TYPE_OCTET_STREAM;
        avLmHash.data.pOctetString = &LmHashBlob;
        mods[dwNumMods].pAttrValues = &avLmHash;

        dwNumMods++;
    }

    if (pUserModInfo->actions.bSetPrimaryGroup)
    {
        QueryList.ppszStrings = (PCSTR*) &pUserModInfo->pszPrimaryGroupSid;
        
        dwError = LocalDirFindObjects(
            hProvider,
            0,
            LSA_OBJECT_TYPE_GROUP,
            LSA_QUERY_TYPE_BY_SID,
            1,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (ppObjects[0] == NULL)
        {
            dwError = LW_ERROR_NO_SUCH_GROUP;
            BAIL_ON_LSA_ERROR(dwError);
        }

        /*
         * Check if the user is member of this group before making
         * it their primary group
         */
        dwError = LwMbsToWc16s(
            ppObjects[0]->pszDN,
            &pwszPrimaryGroupDn);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetMemberships(
            pContext->hDirectory,
            pwszUserDN,
            wszMembershipAttributes,
            &pLocalGroupMemberships,
            &dwNumLocalGroups);
        BAIL_ON_LSA_ERROR(dwError);

        for (iGroup = 0; !bIsMember && iGroup < dwNumLocalGroups; iGroup++)
        {
            pLocalGroupEntry = &(pLocalGroupMemberships[iGroup]);

            dwError = DirectoryGetEntryAttrValueByName(
                pLocalGroupEntry,
                wszAttrNameDn,
                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                &pwszLocalGroupDn);
            BAIL_ON_LSA_ERROR(dwError);

            bIsMember = LwRtlWC16StringIsEqual(pwszPrimaryGroupDn,
                                               pwszLocalGroupDn,
                                               FALSE);
        }

        if (!bIsMember)
        {
            dwError = ERROR_MEMBER_NOT_IN_ALIAS;
            BAIL_ON_LSA_ERROR(dwError);
        }

        mods[dwNumMods].ulOperationFlags = DIR_MOD_FLAGS_REPLACE;
        mods[dwNumMods].pwszAttrName = &wszAttrNamePrimaryGroup[0];
        mods[dwNumMods].ulNumValues = 1;
        avPrimaryGroup.Type = DIRECTORY_ATTR_TYPE_INTEGER;
        avPrimaryGroup.data.ulValue = ppObjects[0]->groupInfo.gid;
        mods[dwNumMods].pAttrValues = &avPrimaryGroup;

        dwNumMods++;
    }
    if (pUserModInfo->actions.bSetShell)
    {
        mods[dwNumMods].ulOperationFlags = DIR_MOD_FLAGS_REPLACE;
        mods[dwNumMods].pwszAttrName = &wszAttrNameShell[0];
        mods[dwNumMods].ulNumValues = 1;
        avShell.Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;
        avShell.data.pszStringValue = pUserModInfo->pszShell;
        mods[dwNumMods].pAttrValues = &avShell;

        dwNumMods++;
    }
    if (pUserModInfo->actions.bSetGecos)
    {
        mods[dwNumMods].ulOperationFlags = DIR_MOD_FLAGS_REPLACE;
        mods[dwNumMods].pwszAttrName = &wszAttrNameGecos[0];
        mods[dwNumMods].ulNumValues = 1;
        avGecos.Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;
        avGecos.data.pszStringValue = pUserModInfo->pszGecos;
        mods[dwNumMods].pAttrValues = &avGecos;

        dwNumMods++;
    }
    if (pUserModInfo->actions.bSetHomedir)
    {
        mods[dwNumMods].ulOperationFlags = DIR_MOD_FLAGS_REPLACE;
        mods[dwNumMods].pwszAttrName = &wszAttrNameHomedir[0];
        mods[dwNumMods].ulNumValues = 1;
        avHomedir.Type = DIRECTORY_ATTR_TYPE_ANSI_STRING;
        avHomedir.data.pszStringValue = pUserModInfo->pszHomedir;
        mods[dwNumMods].pAttrValues = &avHomedir;

        dwNumMods++;
    }


    if (dwNumMods)
    {
        dwError = DirectoryModifyObject(
                        pContext->hDirectory,
                        pwszUserDN,
                        mods);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bRemoveFromGroups)
    {
        dwError = LocalDirFindObjectByGenericName(
            hProvider,
            0,
            LSA_OBJECT_TYPE_GROUP,
            pUserModInfo->pszRemoveFromGroups,
            &pObject);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LwMbsToWc16s(
            pObject->pszDN,
            &pwszGroupDN_remove);
        BAIL_ON_LSA_ERROR(dwError);
        
        LsaUtilFreeSecurityObject(pObject);
        pObject = NULL;
        
        dwError = DirectoryRemoveFromGroup(
            pContext->hDirectory,
            pwszGroupDN_remove,
            UserEntry);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bAddToGroups)
    {
        dwError = LocalDirFindObjectByGenericName(
            hProvider,
            0,
            LSA_OBJECT_TYPE_GROUP,
            pUserModInfo->pszAddToGroups,
            &pObject);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LwMbsToWc16s(
            pObject->pszDN,
            &pwszGroupDN_add);
        BAIL_ON_LSA_ERROR(dwError);

        LsaUtilFreeSecurityObject(pObject);
        pObject = NULL;

        dwError = DirectoryAddToGroup(
                        pContext->hDirectory,
                        pwszGroupDN_add,
                        UserEntry);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetPassword)
    {
        dwError = LwMbsToWc16s(
                        pUserModInfo->pszPassword ? pUserModInfo->pszPassword : "",
                        &pwszPassword);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LocalDirSetPassword(
                        hProvider,
                        pwszUserDN,
                        pwszPassword);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LsaUtilFreeSecurityObject(pObject);
    LsaUtilFreeSecurityObjectList(1, ppObjects);

    if (pLocalGroupMemberships)
    {
        DirectoryFreeEntries(pLocalGroupMemberships, dwNumLocalGroups);
    }

    LW_SAFE_FREE_MEMORY(pwszGroupDN_remove);
    LW_SAFE_FREE_MEMORY(pwszGroupDN_add);
    LW_SECURE_FREE_WSTRING(pwszPassword);
    LW_SAFE_FREE_MEMORY(pwszUserDN);
    LW_SAFE_FREE_MEMORY(pwszPrimaryGroupDn);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalDirDeleteUser(
    HANDLE hProvider,
    PWSTR  pwszUserDN
    )
{
    PCSTR filterTemplate = LOCAL_DB_DIR_ATTR_OBJECT_CLASS "=%u AND "
                           LOCAL_DB_DIR_ATTR_DISTINGUISHED_NAME "=%Q";
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    WCHAR wszAttrNameObjectClass[] = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrNameObjectSid[] = LOCAL_DIR_ATTR_OBJECT_SID;
    PSTR pszUserDn = NULL;
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PSID pUserSid = NULL;
    PSID pDomainSid = NULL;
    BOOLEAN bLocked = FALSE;

    PWSTR pwszAttributes[] = {
        wszAttrNameObjectClass,
        wszAttrNameObjectSid,
        NULL
    };

    dwError = LwWc16sToMbs(
                    pwszUserDN,
                    &pszUserDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryAllocateWC16StringFilterPrintf(
                    &pwszFilter,
                    filterTemplate,
                    LOCAL_OBJECT_CLASS_USER,
                    pszUserDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
                    pContext->hDirectory,
                    NULL,
                    0,
                    pwszFilter,
                    pwszAttributes,
                    FALSE,
                    &pEntry,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        dwError = ERROR_NO_SUCH_USER;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToSid(
                    pEntry,
                    &wszAttrNameObjectSid[0],
                    &pUserSid);
    BAIL_ON_LSA_ERROR(dwError);

    LOCAL_RDLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    pDomainSid = gLPGlobals.pLocalDomainSID;

    if (LocalDirIsBuiltinAccount(
                    pDomainSid,
                    pUserSid))
    {
        dwError = ERROR_SPECIAL_ACCOUNT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = DirectoryDeleteObject(
                    pContext->hDirectory,
                    pwszUserDN);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LOCAL_UNLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    if (pEntry)
    {
        DirectoryFreeEntries(pEntry, dwNumEntries);
    }

    LW_SAFE_FREE_MEMORY(pszUserDn);
    LW_SAFE_FREE_MEMORY(pwszFilter);
    LW_SAFE_FREE_MEMORY(pUserSid);

    return dwError;

error:
    goto cleanup;
}

DWORD
LocalDirChangePassword(
    HANDLE hProvider,
    PWSTR  pwszUserDN,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    )
{
    PCSTR userFilter = LOCAL_DB_DIR_ATTR_DISTINGUISHED_NAME " = %Q";
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    BOOLEAN bLocked = FALSE;
    PACCESS_TOKEN pUserToken = NULL;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    PSTR pszUserDn = NULL;
    PWSTR pwszFilter = NULL;
    WCHAR wszAttrUid[] = LOCAL_DIR_ATTR_UID;
    WCHAR wszAttrGid[] = LOCAL_DIR_ATTR_GID;
    WCHAR wszAttrSecurityDescriptor[] = LOCAL_DIR_ATTR_SECURITY_DESCRIPTOR;
    PWSTR wszAttributes[] = {
        wszAttrUid,
        wszAttrGid,
        wszAttrSecurityDescriptor,
        NULL
    };

    PDIRECTORY_ENTRY pUserEntry = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwUid = 0;
    DWORD dwGid = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    GENERIC_MAPPING GenericMapping = {0};
    DWORD dwAccessGranted = 0;

    dwError = LwWc16sToMbs(
                      pwszUserDN,
                      &pszUserDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryAllocateWC16StringFilterPrintf(
                    &pwszFilter,
                    userFilter,
                    pszUserDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
                    pContext->hDirectory,
                    pwszBase,
                    dwScope,
                    pwszFilter,
                    wszAttributes,
                    FALSE,
                    &pUserEntry,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(
                    pUserEntry,
                    wszAttrUid,
                    DIRECTORY_ATTR_TYPE_INTEGER,
                    &dwUid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(
                    pUserEntry,
                    wszAttrGid,
                    DIRECTORY_ATTR_TYPE_INTEGER,
                    &dwGid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntrySecurityDescriptor(
                    pUserEntry,
                    &pSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    LOCAL_RDLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    /*
     * Check if user has right to change the password first
     */
    ntStatus = LwMapSecurityCreateAccessTokenFromUidGid(
                    gLPGlobals.pSecCtx,
                    &pUserToken,
                    dwUid,
                    dwGid);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!RtlAccessCheck(pSecDesc,
                        pUserToken,
                        USER_ACCESS_CHANGE_PASSWORD,
                        0,
                        &GenericMapping,
                        &dwAccessGranted,
                        &ntStatus))
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    dwError = DirectoryChangePassword(
                    pContext->hDirectory,
                    pwszUserDN,
                    pwszOldPassword,
                    pwszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LOCAL_UNLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    if (pUserEntry)
    {
        DirectoryFreeEntries(pUserEntry, dwNumEntries);
    }

    LW_SAFE_FREE_MEMORY(pszUserDn);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    DirectoryFreeEntrySecurityDescriptor(&pSecDesc);

    RtlReleaseAccessToken(&pUserToken);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


DWORD
LocalDirSetPassword(
    HANDLE hProvider,
    PWSTR pwszUserDN,
    PWSTR pwszNewPassword
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    BOOLEAN bIsAdmin = FALSE;

    dwError = LocalCheckIsAdministrator(
                    hProvider,
                    &bIsAdmin);
    BAIL_ON_LSA_ERROR(dwError);

    if (bIsAdmin)
    {
        dwError = DirectorySetPassword(
                        pContext->hDirectory,
                        pwszUserDN,
                        pwszNewPassword);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LocalCreateHomeDirectory(
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    mode_t  umask = LOCAL_CFG_DEFAULT_HOMEDIR_UMASK;
    mode_t  perms = (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BOOLEAN bRemoveDir = FALSE;

    if (LW_IS_NULL_OR_EMPTY_STR(pObject->userInfo.pszHomedir))
    {
       LSA_LOG_ERROR("The user's [Uid:%ld] home directory is not defined",
                     (long)pObject->userInfo.uid);
       dwError = LW_ERROR_FAILED_CREATE_HOMEDIR;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaCheckDirectoryExists(
                    pObject->userInfo.pszHomedir,
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists)
    {
        dwError = LocalCfgGetHomedirUmask(&umask);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaCreateDirectory(
                    pObject->userInfo.pszHomedir,
                    perms & (~umask));
        BAIL_ON_LSA_ERROR(dwError);

        bRemoveDir = TRUE;

        dwError = LsaChangeOwner(
                    pObject->userInfo.pszHomedir,
                    pObject->userInfo.uid,
                    pObject->userInfo.gid);
        BAIL_ON_LSA_ERROR(dwError);

        bRemoveDir = FALSE;

        dwError = LocalProvisionHomeDir(
                       pObject->userInfo.uid,
                       pObject->userInfo.gid,
                       pObject->userInfo.pszHomedir);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    if (bRemoveDir) {
       LsaRemoveDirectory(pObject->userInfo.pszHomedir);
    }

    goto cleanup;
}

DWORD
LocalProvisionHomeDir(
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszHomedirPath
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    PSTR pszSkelPaths = NULL;
    PSTR pszSkelPath = NULL;
    PSTR pszIter = NULL;
    size_t stLen = 0;

    dwError = LocalCfgGetSkeletonDirs(&pszSkelPaths);
    BAIL_ON_LSA_ERROR(dwError);

    if (LW_IS_NULL_OR_EMPTY_STR(pszSkelPaths))
    {
        goto cleanup;
    }

    pszIter = pszSkelPaths;
    while ((stLen = strcspn(pszIter, ",")) != 0)
    {
        dwError = LwStrndup(
                      pszIter,
                      stLen,
                      &pszSkelPath);
        BAIL_ON_LSA_ERROR(dwError);

        LwStripWhitespace(pszSkelPath, TRUE, TRUE);

        if (LW_IS_NULL_OR_EMPTY_STR(pszSkelPath))
        {
            LW_SAFE_FREE_STRING(pszSkelPath);
            continue;
        }

        dwError = LsaCheckDirectoryExists(
                        pszSkelPath,
                        &bExists);
        BAIL_ON_LSA_ERROR(dwError);

        if (bExists)
        {
            dwError = LsaCopyDirectory(
                        pszSkelPath,
                        ownerUid,
                        ownerGid,
                        pszHomedirPath);
            BAIL_ON_LSA_ERROR(dwError);
        }

        LW_SAFE_FREE_STRING(pszSkelPath);

        pszIter += stLen;
        stLen = strspn(pszIter, ",");
        pszIter += stLen;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszSkelPath);
    LW_SAFE_FREE_STRING(pszSkelPaths);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalCheckIsGuest(
    PLSA_SECURITY_OBJECT pObject,
    PBOOLEAN pbUserIsGuest
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSID pUserSid = NULL;
    ULONG ulRid = 0;
    BOOLEAN bUserIsGuest = FALSE;

    dwError = LwNtStatusToWin32Error(
                  RtlAllocateSidFromCString(
                      &pUserSid,
                      pObject->pszObjectSid));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
                  RtlGetRidSid(
                      &ulRid,
                      pUserSid));
    BAIL_ON_LSA_ERROR(dwError);

    if (ulRid == DOMAIN_USER_RID_GUEST)
    {
        bUserIsGuest = TRUE;
    }    

cleanup:

    if (pUserSid)
    {
        LW_RTL_FREE(&pUserSid);
    }    

    *pbUserIsGuest = bUserIsGuest;
    
    return dwError;

error:

    goto cleanup;
}

DWORD
LocalCheckAccountFlags(
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    BAIL_ON_INVALID_POINTER(pObject);

    if (pObject->userInfo.bAccountDisabled)
    {
        dwError = LW_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pObject->userInfo.bAccountLocked)
    {
        dwError = LW_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pObject->userInfo.bAccountExpired)
    {
        dwError = LW_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pObject->userInfo.bPasswordExpired)
    {
        dwError = LW_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

error:

    return dwError;
}

DWORD
LocalCheckPasswordPolicy(
    PLSA_SECURITY_OBJECT pObject,
    PCSTR                pszPassword
    )
{
    DWORD dwError = 0;
    DWORD dwMinPasswordLen = 0;
    size_t sPasswordLen = 0;

    BAIL_ON_INVALID_POINTER(pObject);
    BAIL_ON_INVALID_POINTER(pszPassword);

    if (!pObject->userInfo.bUserCanChangePassword)
    {
        dwError = LW_ERROR_PASSWORD_RESTRICTION;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalCfgGetMinPwdLength(&dwMinPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    sPasswordLen = strlen(pszPassword);
    if (dwMinPasswordLen > sPasswordLen)
    {
        dwError = LW_ERROR_PASSWORD_RESTRICTION;
        BAIL_ON_LSA_ERROR(dwError);
    }

error:
    return dwError;
}

DWORD
LocalGetUserLogonInfo(
    HANDLE   hProvider,
    PSTR     pszUserDn,
    PDWORD   pdwLogonCount,
    PDWORD   pdwBadPasswordCount
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    WCHAR wszAttrNameLogonCount[] = LOCAL_DIR_ATTR_LOGON_COUNT;
    WCHAR wszAttrNameBadPasswordCount[] = LOCAL_DIR_ATTR_BAD_PASSWORD_COUNT;
    PCSTR pszFilterTemplate = LOCAL_DB_DIR_ATTR_DISTINGUISHED_NAME " = %Q";
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwLogonCount = 0;
    DWORD dwBadPasswordCount = 0;

    PWSTR pwszAttributes[] = {
        wszAttrNameLogonCount,
        wszAttrNameBadPasswordCount,
        NULL
    };

    BAIL_ON_INVALID_POINTER(hProvider);
    BAIL_ON_INVALID_POINTER(pszUserDn);

    dwError = DirectoryAllocateWC16StringFilterPrintf(
                    &pwszFilter,
                    pszFilterTemplate,
                    pszUserDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
                    pContext->hDirectory,
                    NULL,
                    0,
                    pwszFilter,
                    pwszAttributes,
                    FALSE,
                    &pEntries,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pEntry = &pEntries[0];

    dwError = LocalMarshalAttrToInteger(
                   pEntry,
                   wszAttrNameLogonCount,
                   &dwLogonCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToInteger(
                   pEntry,
                   wszAttrNameBadPasswordCount,
                   &dwBadPasswordCount);
    BAIL_ON_LSA_ERROR(dwError);

    if (pdwLogonCount)
    {
        *pdwLogonCount = dwLogonCount;
    }

    if (pdwBadPasswordCount)
    {
        *pdwBadPasswordCount = dwBadPasswordCount;
    }

cleanup:
    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    LW_SAFE_FREE_MEMORY(pwszFilter);

    return dwError;

error:
    if (pdwLogonCount)
    {
        *pdwLogonCount = 0;
    }

    if (pdwBadPasswordCount)
    {
        *pdwBadPasswordCount = 0;
    }

    goto cleanup;
}

DWORD
LocalSetUserLogonInfo(
    HANDLE  hProvider,
    PSTR    pszUserDn,
    PDWORD  pdwLogonCount,
    PDWORD  pdwBadPasswordCount,
    PLONG64 pllLastLogonTime,
    PLONG64 pllLastLogoffTime
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    WCHAR wszAttrNameLogonCount[] = LOCAL_DIR_ATTR_LOGON_COUNT;
    WCHAR wszAttrNameBadPasswordCount[] = LOCAL_DIR_ATTR_BAD_PASSWORD_COUNT;
    WCHAR wszAttrNameLastLogon[] = LOCAL_DIR_ATTR_LAST_LOGON;
    WCHAR wszAttrNameLastLogoff[] = LOCAL_DIR_ATTR_LAST_LOGOFF;
    DWORD iMod = 0;
    PWSTR pwszUserDn = NULL;

    enum AttrValueIndex {
        ATTR_VAL_IDX_LOGON_COUNT = 0,
        ATTR_VAL_IDX_BAD_PASSWORD_COUNT,
        ATTR_VAL_IDX_LAST_LOGON,
        ATTR_VAL_IDX_LAST_LOGOFF,
        ATTR_VAL_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {   /* ATTR_VAL_IDX_LOGON_COUNT */
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {   /* ATTR_VAL_IDX_BAD_PASSWORD_COUNT */
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {   /* ATTR_VAL_IDX_LAST_LOGON */
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.llValue = 0
        },
        {   /* ATTR_VAL_IDX_LAST_LOGOFF */
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.llValue = 0
        }
    };

    DIRECTORY_MOD ModLogonCount = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrNameLogonCount,
        1,
        &AttrValues[ATTR_VAL_IDX_LOGON_COUNT]
    };

    DIRECTORY_MOD ModBadPasswordCount = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrNameBadPasswordCount,
        1,
        &AttrValues[ATTR_VAL_IDX_BAD_PASSWORD_COUNT]
    };

    DIRECTORY_MOD ModLastLogon = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrNameLastLogon,
        1,
        &AttrValues[ATTR_VAL_IDX_LAST_LOGON]
    };

    DIRECTORY_MOD ModLastLogoff = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrNameLastLogoff,
        1,
        &AttrValues[ATTR_VAL_IDX_LAST_LOGOFF]
    };

    DIRECTORY_MOD Mods[ATTR_VAL_IDX_SENTINEL + 1];
    memset(&Mods, 0, sizeof(Mods));

    BAIL_ON_INVALID_POINTER(hProvider);
    BAIL_ON_INVALID_POINTER(pszUserDn);

    if (pdwLogonCount)
    {
        AttrValues[ATTR_VAL_IDX_LOGON_COUNT].data.ulValue =
            (*pdwLogonCount);
        Mods[iMod++] = ModLogonCount;
    }

    if (pdwBadPasswordCount)
    {
        AttrValues[ATTR_VAL_IDX_BAD_PASSWORD_COUNT].data.ulValue =
            (*pdwBadPasswordCount);
        Mods[iMod++] = ModBadPasswordCount;
    }

    if (pllLastLogonTime)
    {
        AttrValues[ATTR_VAL_IDX_LAST_LOGON].data.llValue =
            (*pllLastLogonTime);
        Mods[iMod++] = ModLastLogon;
    }

    if (pllLastLogoffTime)
    {
        AttrValues[ATTR_VAL_IDX_LAST_LOGOFF].data.llValue =
            (*pllLastLogoffTime);
        Mods[iMod++] = ModLastLogoff;
    }

    if (iMod)
    {
        dwError = LwMbsToWc16s(pszUserDn, &pwszUserDn);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryModifyObject(
                        pContext->hDirectory,
                        pwszUserDn,
                        Mods);
        BAIL_ON_LSA_ERROR(dwError);
    }

error:
    LW_SAFE_FREE_MEMORY(pwszUserDn);

    return dwError;
}

DWORD
LocalUpdateUserLoginTime(
    HANDLE hProvider,
    PWSTR  pwszUserDN
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    ATTRIBUTE_VALUE attrValue =
    {
        .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
        .data.llValue = 0
    };
    WCHAR wszAttrNameLastLogonTime[] = LOCAL_DIR_ATTR_LAST_LOGON;
    DIRECTORY_MOD mods[] =
    {
        {
            DIR_MOD_FLAGS_REPLACE,
            &wszAttrNameLastLogonTime[0],
            1,
            &attrValue
        },
        {
            DIR_MOD_FLAGS_REPLACE,
            NULL,
            0,
            NULL
        }
    };

    attrValue.data.llValue = LocalGetNTTime(time(NULL));

    dwError = DirectoryModifyObject(
                    pContext->hDirectory,
                    pwszUserDN,
                    mods);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalUpdateUserLogoffTime(
    HANDLE hProvider,
    PWSTR  pwszUserDN
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    ATTRIBUTE_VALUE attrValue =
    {
        .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
        .data.llValue = 0
    };
    WCHAR wszAttrNameLastLogoffTime[] = LOCAL_DIR_ATTR_LAST_LOGOFF;
    DIRECTORY_MOD mods[] =
    {
        {
            DIR_MOD_FLAGS_REPLACE,
            &wszAttrNameLastLogoffTime[0],
            1,
            &attrValue
        },
        {
            DIR_MOD_FLAGS_ADD,
            NULL,
            0,
            NULL
        }
    };

    attrValue.data.llValue = LocalGetNTTime(time(NULL));

    dwError = DirectoryModifyObject(
                    pContext->hDirectory,
                    pwszUserDN,
                    mods);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalDirCheckIfAdministrator(
    HANDLE   hProvider,
    uid_t    uid,
    PBOOLEAN pbIsAdmin
    )
{
    DWORD dwError = 0;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PSTR* ppszGroupSids = NULL;
    DWORD dwNumGroups = 0;
    BOOLEAN bIsAdmin = FALSE;
    DWORD dwUid = (DWORD) uid;
    PSTR pszBuiltinAdminSid = NULL;
    DWORD iGroup = 0;

    if (uid == 0)
    {
        bIsAdmin = TRUE;
    }
    else
    {
        QueryList.pdwIds = &dwUid;

        dwError = LocalDirFindObjects(
            hProvider,
            0,
            LSA_OBJECT_TYPE_USER,
            LSA_QUERY_TYPE_BY_UNIX_ID,
            1,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);

        if (ppObjects[0])
        {
            dwError = LocalQueryMemberOf(
                hProvider,
                0,
                1,
                &ppObjects[0]->pszObjectSid,
                &dwNumGroups,
                &ppszGroupSids);
            BAIL_ON_LSA_ERROR(dwError);

            if (dwNumGroups)
            {
                dwError = LocalDirGetBuiltinAdministratorsSid(
                    hProvider,
                    &pszBuiltinAdminSid);
                BAIL_ON_LSA_ERROR(dwError);
                
                for (; iGroup < dwNumGroups; iGroup++)
                {
                    if (!strcmp(ppszGroupSids[iGroup], pszBuiltinAdminSid))
                    {
                        bIsAdmin = TRUE;
                        break;
                    }
                }
            }
        }
    }

    *pbIsAdmin = bIsAdmin;

cleanup:

    LwFreeStringArray(ppszGroupSids, dwNumGroups);
    LW_SAFE_FREE_STRING(pszBuiltinAdminSid);

    return dwError;

error:

    *pbIsAdmin = FALSE;

    goto cleanup;
}

static
DWORD
LocalDirGetBuiltinAdministratorsSid(
    HANDLE hProvider,
    PSTR* ppszSid
    )
{
    DWORD dwError = 0;
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    static PSTR pszBuiltinAdminSid = 0;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PCSTR pszGroupName = "BUILTIN\\Administrators";

    pthread_mutex_lock(&mutex);

    if (!pszBuiltinAdminSid)
    {
        QueryList.ppszStrings = &pszGroupName;
        
        dwError = LocalDirFindObjects(
            hProvider,
            0,
            LSA_OBJECT_TYPE_GROUP,
            LSA_QUERY_TYPE_BY_NT4,
            1,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (ppObjects[0] == NULL)
        {
            dwError = LW_ERROR_NO_SUCH_GROUP;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LwAllocateString(ppObjects[0]->pszObjectSid, &pszBuiltinAdminSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateString(pszBuiltinAdminSid, ppszSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    pthread_mutex_unlock(&mutex);

    return dwError;

error:

    *ppszSid = NULL;

    goto cleanup;
}
