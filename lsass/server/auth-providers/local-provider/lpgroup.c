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
 *        lpgroup.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        User/Group Database Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */
#include "includes.h"

static
DWORD
LocalDirValidateGID(
    gid_t gid
    );

static
DWORD
LocalDirCheckLocalOrBuiltinSid(
    IN PCSTR pszSid,
    OUT PBOOLEAN pbIsLocalOrBuiltinSid
    );

static
DWORD
LocalAddMembersToGroup(
    PLOCAL_PROVIDER_CONTEXT pContext,
    PWSTR pwszGroupDN,
    DWORD dwMemberCount,
    PSTR* ppszMemberSids
    );

static
DWORD
LocalDirCreateForeignPrincipalDN(
    HANDLE     hProvider,
    PWSTR      pwszSID,
    PWSTR     *ppwszDN
    );

DWORD
LocalDirAddGroup(
    HANDLE            hProvider,
    PLSA_GROUP_ADD_INFO pGroupInfo
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    BOOLEAN bEventlogEnabled = FALSE;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PWSTR pwszGroupDN = NULL;
    enum AttrValueIndex
    {
        LOCAL_DAG1_IDX_SAM_ACCOUNT_NAME = 0,
        LOCAL_DAG1_IDX_COMMON_NAME,
        LOCAL_DAG1_IDX_OBJECTCLASS,
        LOCAL_DAG1_IDX_DOMAIN,
        LOCAL_DAG1_IDX_NETBIOS_DOMAIN,
        LOCAL_DAG1_IDX_GID,
        LOCAL_DAG1_IDX_SENTINEL
    };
    ATTRIBUTE_VALUE attrValues[] =
    {
        {       /* LOCAL_DIR_ADD_USER_0_IDX_SAM_ACCOUNT_NAME */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_COMMON_NAME */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_OBJECTCLASS */
                .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                .data.ulValue = LOCAL_OBJECT_CLASS_GROUP
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_DOMAIN */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_NETBIOS_DOMAIN */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_GID */
                .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                .data.ulValue = pGroupInfo->gid
        }
    };

    WCHAR wszAttrObjectClass[] = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrDistinguishedName[] = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectSID[] = LOCAL_DIR_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrCommonName[] = LOCAL_DIR_ATTR_COMMON_NAME;
    WCHAR wszAttrDomain[] = LOCAL_DIR_ATTR_DOMAIN;
    WCHAR wszAttrNameNetBIOSDomain[] = LOCAL_DIR_ATTR_NETBIOS_NAME;
    WCHAR wszAttrNameGID[] = LOCAL_DIR_ATTR_GID;

    DIRECTORY_MOD modObjectClass =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrObjectClass[0],
        1,
        &attrValues[LOCAL_DAG1_IDX_OBJECTCLASS]
    };
    DIRECTORY_MOD modSamAccountName =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrSamAccountName[0],
        1,
        &attrValues[LOCAL_DAG1_IDX_SAM_ACCOUNT_NAME]
    };
    DIRECTORY_MOD modCommonName =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrCommonName[0],
        1,
        &attrValues[LOCAL_DAG1_IDX_COMMON_NAME]
    };
    DIRECTORY_MOD modDomain =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrDomain[0],
        1,
        &attrValues[LOCAL_DAG1_IDX_DOMAIN]
    };
    DIRECTORY_MOD modNetBIOSDomain =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrNameNetBIOSDomain[0],
        1,
        &attrValues[LOCAL_DAG1_IDX_NETBIOS_DOMAIN]
    };
    DIRECTORY_MOD modGID =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrNameGID[0],
        1,
        &attrValues[LOCAL_DAG1_IDX_GID]
    };
    DIRECTORY_MOD mods[LOCAL_DAG1_IDX_SENTINEL + 1];
    DWORD iMod = 0;
    PWSTR pwszSamAccountName = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszNetBIOSDomain = NULL;
    PSTR pszGroupDn = NULL;
    PWSTR pwszFilter = NULL;
    PCSTR filterFormat = LOCAL_DB_DIR_ATTR_DISTINGUISHED_NAME " = %Q";
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;

    PWSTR wszGroupAttrs[] = {
        wszAttrDistinguishedName,
        wszAttrObjectSID,
        NULL
    };

    PDIRECTORY_ENTRY pGroup = NULL;
    DWORD dwNumEntries = 0;
    PWSTR pwszGroupSID = NULL;
    PSID pGroupSID = NULL;
    DWORD dwGroupRID = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    BOOLEAN bLocked = FALSE;

    memset(&mods[0], 0, sizeof(mods));

    BAIL_ON_INVALID_STRING(pGroupInfo->pszName);

    if (pGroupInfo->gid)
    {
        dwError = LocalDirValidateGID(pGroupInfo->gid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvCrackDomainQualifiedName(
                    pGroupInfo->pszName,
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

    dwError = LwMbsToWc16s(
                    pLoginInfo->pszDomain,
                    &pwszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAG1_IDX_DOMAIN].data.pwszStringValue = pwszDomain;

    dwError = LwMbsToWc16s(
                    pLoginInfo->pszDomain,
                    &pwszNetBIOSDomain);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAG1_IDX_NETBIOS_DOMAIN].data.pwszStringValue = pwszNetBIOSDomain;

    dwError = LwMbsToWc16s(
                    pGroupInfo->pszName,
                    &pwszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAG1_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue = pwszSamAccountName;
    attrValues[LOCAL_DAG1_IDX_COMMON_NAME].data.pwszStringValue = pwszSamAccountName;

    dwError = LocalBuildDN(
                    pLoginInfo,
                    &pwszGroupDN);
    BAIL_ON_LSA_ERROR(dwError);

    mods[iMod++] = modObjectClass;
    if (pGroupInfo->gid)
    {
        mods[iMod++] = modGID;
    }
    mods[iMod++] = modSamAccountName;
    mods[iMod++] = modCommonName;
    mods[iMod++] = modDomain;
    mods[iMod++] = modNetBIOSDomain;

    dwError = DirectoryAddObject(
                    pContext->hDirectory,
                    pwszGroupDN,
                    mods);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(
                    pwszGroupDN,
                    &pszGroupDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryAllocateWC16StringFilterPrintf(
                    &pwszFilter,
                    filterFormat,
                    pszGroupDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
                    pContext->hDirectory,
                    pwszBase,
                    dwScope,
                    pwszFilter,
                    wszGroupAttrs,
                    0,
                    &pGroup,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Set default security descriptor on the account
     */

    dwError = DirectoryGetEntryAttrValueByName(
                    pGroup,
                    wszAttrObjectSID,
                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    &pwszGroupSID);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlAllocateSidFromWC16String(&pGroupSID,
                                            pwszGroupSID);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlGetRidSid(&dwGroupRID,
                            pGroupSID);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LocalDirCreateNewAccountSecurityDescriptor(
                    gLPGlobals.pLocalDomainSID,
                    dwGroupRID,
                    DIR_OBJECT_CLASS_LOCAL_GROUP,
                    &pSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySetEntrySecurityDescriptor(
                                  pContext->hDirectory,
                                  pwszGroupDN,
                                  pSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    if (pGroupInfo->ppszMemberSids)
    {
        dwError = LocalAddMembersToGroup(
                        pContext,
                        pwszGroupDN,
                        pGroupInfo->dwMemberCount,
                        pGroupInfo->ppszMemberSids);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalCfgIsEventlogEnabled(&bEventlogEnabled);
    BAIL_ON_LSA_ERROR(dwError);

    if (bEventlogEnabled)
    {
        LocalEventLogGroupAdd(pLoginInfo->pszName,
                             ((PLSA_GROUP_INFO_0)pGroupInfo)->gid);
    }

cleanup:
    LOCAL_UNLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    if (pLoginInfo)
    {
        LsaSrvFreeNameInfo(pLoginInfo);
    }

    if (pGroup)
    {
        DirectoryFreeEntries(pGroup, dwNumEntries);
    }

    LW_SAFE_FREE_MEMORY(pwszGroupDN);
    LW_SAFE_FREE_MEMORY(pszGroupDn);
    LW_SAFE_FREE_MEMORY(pwszFilter);
    LW_SAFE_FREE_MEMORY(pwszSamAccountName);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszNetBIOSDomain);
    LW_SAFE_FREE_MEMORY(pwszFilter);
    RTL_FREE(&pGroupSID);
    
    LocalDirFreeSecurityDescriptor(&pSecDesc);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LocalDirValidateGID(
    gid_t gid
    )
{
    DWORD dwError = 0;

    /* Check whether group gid is within permitted range */
    if (gid < LOWEST_GID) {
        BAIL_WITH_LSA_ERROR(LW_ERROR_GID_TOO_LOW);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LocalDirModifyGroup(
    HANDLE hProvider,
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;  
    PWSTR pwszGroupDN = NULL;
    DWORD i = 0;
    PSTR pszSID = NULL;
    PWSTR pwszSID = NULL;
    PWSTR pwszDN = NULL;
    BOOLEAN bIsLocalOrBuiltinSid = FALSE;
    BOOLEAN bForeignSid = FALSE;
    DWORD dwObjectClassGroupMember = LOCAL_OBJECT_CLASS_GROUP_MEMBER;
    DWORD dwObjectClassLocalUser = LOCAL_OBJECT_CLASS_USER;
    PWSTR pwszBase = NULL;
    ULONG ulScope = 0;
    PCSTR filterFormatSidOnly = "(" LOCAL_DB_DIR_ATTR_OBJECT_CLASS "=%u OR "
                                    LOCAL_DB_DIR_ATTR_OBJECT_CLASS "=%u) AND "
                                 LOCAL_DB_DIR_ATTR_OBJECT_SID "=%Q";
    PWSTR pwszFilter = NULL;
    WCHAR wszAttrObjectClass[] = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrDistinguishedName[] = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectSid[] = LOCAL_DIR_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    PWSTR wszAttributes[] = {
        wszAttrObjectClass,
        wszAttrObjectSid,
        wszAttrDistinguishedName,
        wszAttrSamAccountName,
        NULL
    };

    PDIRECTORY_ENTRY pMember = NULL;
    DWORD dwNumEntries = 0;

    enum AttrValueIndex
    {
        GRP_MEMBER_IDX_OBJECTCLASS,
        GRP_MEMBER_IDX_DN,
        GRP_MEMBER_IDX_SID
    };

    ATTRIBUTE_VALUE attrValues[] = {
        {
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = LOCAL_OBJECT_CLASS_GROUP_MEMBER
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        }
    };

    DIRECTORY_MOD modObjectClass = {
        DIR_MOD_FLAGS_ADD,
        wszAttrObjectClass,
        1,
        &attrValues[GRP_MEMBER_IDX_OBJECTCLASS]
    };

    DIRECTORY_MOD modDistinguishedName = {
        DIR_MOD_FLAGS_ADD,
        wszAttrDistinguishedName,
        1,
        &attrValues[GRP_MEMBER_IDX_DN]
    };

    DIRECTORY_MOD modObjectSID = {
        DIR_MOD_FLAGS_ADD,
        wszAttrObjectSid,
        1,
        &attrValues[GRP_MEMBER_IDX_SID]
    };

    DIRECTORY_MOD MemberMods[] = {
        modObjectClass,
        modDistinguishedName,
        modObjectSID,
        { 0, NULL, 0, NULL }
    };

    QueryList.ppszStrings = (PCSTR*) &pGroupModInfo->pszSid;

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

    dwError = LwMbsToWc16s(
        ppObjects[0]->pszDN,
        &pwszGroupDN);
    BAIL_ON_LSA_ERROR(dwError);

    if (pGroupModInfo->actions.bAddMembers)
    {
        for (i = 0; i < pGroupModInfo->dwAddMembersNum; i++)
        {
            pszSID = pGroupModInfo->ppszAddMembers[i];

            dwError = LocalDirCheckLocalOrBuiltinSid(
                            pszSID,
                            &bIsLocalOrBuiltinSid);
            BAIL_ON_LSA_ERROR(dwError);

            bForeignSid = !bIsLocalOrBuiltinSid;

            dwError = DirectoryAllocateWC16StringFilterPrintf(
                        &pwszFilter,
                        filterFormatSidOnly,
                        dwObjectClassGroupMember,
                        dwObjectClassLocalUser,
                        pszSID);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = DirectorySearch(
                        pContext->hDirectory,
                        pwszBase,
                        ulScope,
                        pwszFilter,
                        wszAttributes,
                        0,
                        &pMember,
                        &dwNumEntries);
            BAIL_ON_LSA_ERROR(dwError);

            if (dwNumEntries == 0 &&
                bForeignSid)
            {
                dwError = LwMbsToWc16s(pszSID, &pwszSID);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LocalDirCreateForeignPrincipalDN(hProvider,
                                                           pwszSID,
                                                           &pwszDN);
                BAIL_ON_LSA_ERROR(dwError);

                MemberMods[GRP_MEMBER_IDX_DN].pAttrValues[0].data.pwszStringValue = pwszDN;
                MemberMods[GRP_MEMBER_IDX_SID].pAttrValues[0].data.pwszStringValue = pwszSID;

                dwError = DirectoryAddObject(
                            pContext->hDirectory,
                            pwszDN,
                            MemberMods);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = DirectorySearch(
                            pContext->hDirectory,
                            pwszBase,
                            ulScope,
                            pwszFilter,
                            wszAttributes,
                            0,
                            &pMember,
                            &dwNumEntries);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else if (dwNumEntries == 0 &&
                     !bForeignSid)
            {
                dwError = LW_ERROR_NO_SUCH_OBJECT;
                BAIL_ON_LSA_ERROR(dwError);
            }
            else if (dwNumEntries > 1)
            {
                dwError = LW_ERROR_SAM_DATABASE_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = DirectoryAddToGroup(
                        pContext->hDirectory,
                        pwszGroupDN,
                        pMember);
            BAIL_ON_LSA_ERROR(dwError);

            if (pwszDN)
            {
                LW_SAFE_FREE_MEMORY(pwszDN);
                pwszDN = NULL;
            }

            if (pwszSID)
            {
                LW_SAFE_FREE_MEMORY(pwszSID);
                pwszDN = NULL;
            }

            if (pwszFilter)
            {
                LW_SAFE_FREE_MEMORY(pwszFilter);
                pwszFilter = NULL;
            }

            if (pMember)
            {
                DirectoryFreeEntries(pMember, dwNumEntries);
                pMember = NULL;
            }
        }

    }
    else if (pGroupModInfo->actions.bRemoveMembers)
    {
        for (i = 0; i < pGroupModInfo->dwRemoveMembersNum; i++)
        {
            pszSID = pGroupModInfo->ppszRemoveMembers[i];

            dwError = DirectoryAllocateWC16StringFilterPrintf(
                        &pwszFilter,
                        filterFormatSidOnly,
                        dwObjectClassGroupMember,
                        dwObjectClassLocalUser,
                        pszSID);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = DirectorySearch(
                        pContext->hDirectory,
                        pwszBase,
                        ulScope,
                        pwszFilter,
                        wszAttributes,
                        0,
                        &pMember,
                        &dwNumEntries);
            BAIL_ON_LSA_ERROR(dwError);

            if (dwNumEntries == 0)
            {
                dwError = ERROR_MEMBER_NOT_IN_GROUP;
                BAIL_ON_LSA_ERROR(dwError);

            }
            else if (dwNumEntries > 1)
            {
                dwError = LW_ERROR_SAM_DATABASE_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = DirectoryRemoveFromGroup(
                        pContext->hDirectory,
                        pwszGroupDN,
                        pMember);
            BAIL_ON_LSA_ERROR(dwError);

            if (pwszDN)
            {
                LW_SAFE_FREE_MEMORY(pwszDN);
                pwszDN = NULL;
            }

            if (pwszFilter)
            {
                LW_SAFE_FREE_MEMORY(pwszFilter);
                pwszFilter = NULL;
            }

            if (pMember)
            {
                DirectoryFreeEntries(pMember, dwNumEntries);
                pMember = NULL;
            }
        }
    }

cleanup:

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    LW_SAFE_FREE_MEMORY(pwszFilter);
    LW_SAFE_FREE_MEMORY(pwszDN);
    LW_SAFE_FREE_MEMORY(pwszSID);
    LW_SAFE_FREE_MEMORY(pwszGroupDN);

    if (pMember)
    {
        DirectoryFreeEntries(pMember, dwNumEntries);
    }

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LocalDirCheckLocalOrBuiltinSid(
    IN PCSTR pszSid,
    OUT PBOOLEAN pbIsLocalOrBuiltinSid
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsLocalOrBuiltinSid = FALSE;
    PSID pSid = NULL;
    PSID pBuiltinSid = NULL;
    BOOLEAN bLocked = FALSE;

    dwError = LsaAllocateSidFromCString(&pSid, pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    LOCAL_RDLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    if (RtlIsPrefixSid(gLPGlobals.pLocalDomainSID, pSid))
    {
        bIsLocalOrBuiltinSid = TRUE;
        goto cleanup;
    }

    dwError = LsaAllocateSidFromCString(&pBuiltinSid, "S-1-5-32");
    BAIL_ON_LSA_ERROR(dwError);

    if (RtlIsPrefixSid(pBuiltinSid, pSid))
    {
        bIsLocalOrBuiltinSid = TRUE;
        goto cleanup;
    }

    bIsLocalOrBuiltinSid = FALSE;

cleanup:
    LOCAL_UNLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    LW_SAFE_FREE_MEMORY(pBuiltinSid);
    LW_SAFE_FREE_MEMORY(pSid);

    *pbIsLocalOrBuiltinSid = bIsLocalOrBuiltinSid;

    return dwError;

error:
    bIsLocalOrBuiltinSid = FALSE;

    goto cleanup;
}

static
DWORD
LocalAddMembersToGroup(
    PLOCAL_PROVIDER_CONTEXT pContext,
    PWSTR pwszGroupDN,
    DWORD dwMemberCount,
    PSTR* ppszMemberSids
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    LSA_GROUP_MOD_INFO_2 groupModInfo = {0};
    PSTR pszGroupDN = NULL;

    dwError = LwWc16sToMbs(pwszGroupDN, &pszGroupDN);
    BAIL_ON_LSA_ERROR(dwError);

    QueryList.ppszStrings = (PCSTR*) &pszGroupDN;

    dwError = LocalDirFindObjects(
        pContext,
        0,
        LSA_OBJECT_TYPE_GROUP,
        LSA_QUERY_TYPE_BY_DN,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    groupModInfo.actions.bAddMembers = TRUE;
    groupModInfo.ppszAddMembers = ppszMemberSids;
    groupModInfo.pszSid = ppObjects[0]->pszObjectSid;

    dwError = LocalDirModifyGroup(
        pContext,
        &groupModInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaUtilFreeSecurityObjectList(1, ppObjects);
    LW_SAFE_FREE_STRING(pszGroupDN);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LocalDirCreateForeignPrincipalDN(
    HANDLE     hProvider,
    PWSTR      pwszSID,
    PWSTR     *ppwszDN
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    PWSTR pwszBase = NULL;
    ULONG ulScope = 0;
    wchar_t wszFilterFmt[] = L"%ws=%u";
    DWORD dwFilterLen = 0;
    PWSTR pwszFilter = NULL;
    WCHAR wszAttrObjectClass[] = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectDomain[] = LOCAL_DIR_ATTR_DOMAIN;
    DWORD dwDomainObjectClass = LOCAL_OBJECT_CLASS_DOMAIN;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PWSTR pwszDomainName = NULL;
    wchar_t wszForeignDnFmt[] = L"CN=%ws,"
                                L"CN=ForeignSecurityPrincipals,"
                                L"DC=%ws";
    size_t sidStrLen = 0;
    size_t domainNameLen = 0;
    DWORD dwForeignDnLen = 0;
    PWSTR pwszDn = NULL;

    PWSTR wszAttributes[] = {
        wszAttrObjectClass,
        wszAttrObjectDomain,
        NULL
    };

    dwFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR)) - 1) +
                  10 +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwFilterLen,
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                wszAttrObjectClass,
                dwDomainObjectClass);

    dwError = DirectorySearch(pContext->hDirectory,
                              pwszBase,
                              ulScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntries,
                              &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0 ||
        dwNumEntries > 1)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pEntry = &(pEntries[0]);

    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrObjectDomain,
                              DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                              (PVOID)&pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszSID, &sidStrLen);
    BAIL_ON_LSA_ERROR(dwError);

    if (sidStrLen == 0)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwWc16sLen(pwszDomainName, &domainNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwForeignDnLen = (DWORD) sidStrLen +
                     (DWORD) domainNameLen +
                     (sizeof(wszForeignDnFmt)/sizeof(wszForeignDnFmt[0]));

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwForeignDnLen,
                               OUT_PPVOID(&pwszDn));
    BAIL_ON_LSA_ERROR(dwError);

    sw16printfw(pwszDn, dwForeignDnLen, wszForeignDnFmt,
                pwszSID,
                pwszDomainName);

    *ppwszDN = pwszDn;

cleanup:
    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    LW_SAFE_FREE_MEMORY(pwszFilter);

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszDn);

    *ppwszDN = NULL;

    goto cleanup;
}


DWORD
LocalDirDeleteGroup(
    HANDLE hProvider,
    PWSTR  pwszGroupDN
    )
{
    PCSTR filterTemplate = LOCAL_DB_DIR_ATTR_OBJECT_CLASS "=%u AND "
                           LOCAL_DB_DIR_ATTR_DISTINGUISHED_NAME "=%Q";
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    WCHAR wszAttrNameObjectClass[] = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrNameObjectSid[] = LOCAL_DIR_ATTR_OBJECT_SID;
    PSTR pszGroupDn = NULL;
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PSID pGroupSid = NULL;
    PSID pDomainSid = NULL;

    PWSTR pwszAttributes[] = {
        wszAttrNameObjectClass,
        wszAttrNameObjectSid,
        NULL
    };

    dwError = LwWc16sToMbs(
                    pwszGroupDN,
                    &pszGroupDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryAllocateWC16StringFilterPrintf(
                    &pwszFilter,
                    filterTemplate,
                    LOCAL_OBJECT_CLASS_GROUP,
                    pszGroupDn);
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
        dwError = ERROR_NO_SUCH_GROUP;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToSid(
                    pEntry,
                    &wszAttrNameObjectSid[0],
                    &pGroupSid);
    BAIL_ON_LSA_ERROR(dwError);

    pDomainSid = gLPGlobals.pLocalDomainSID;

    /*if (LocalDirIsBuiltinAccount(
                    pDomainSid,
                    pGroupSid))
    {
        dwError = ERROR_SPECIAL_ACCOUNT;
        BAIL_ON_LSA_ERROR(dwError);
    }*/

    dwError = DirectoryDeleteObject(
                    pContext->hDirectory,
                    pwszGroupDN);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pEntry)
    {
        DirectoryFreeEntries(pEntry, dwNumEntries);
    }

    LW_SAFE_FREE_MEMORY(pszGroupDn);
    LW_SAFE_FREE_MEMORY(pwszFilter);
    LW_SAFE_FREE_MEMORY(pGroupSid);

    return dwError;

error:
    goto cleanup;
}

VOID
LocalDirFreeGroupMemberList(
    PLOCAL_PROVIDER_GROUP_MEMBER* ppMemberList,
    DWORD                         dwNumMembers
    )
{
    DWORD iMember = 0;

    for (; iMember < dwNumMembers; iMember++)
    {
        if (ppMemberList[iMember])
        {
            LocalDirFreeGroupMember(ppMemberList[iMember]);
        }
    }

    LwFreeMemory(ppMemberList);
}

VOID
LocalDirFreeGroupMember(
    PLOCAL_PROVIDER_GROUP_MEMBER pMember
    )
{
    LW_SAFE_FREE_STRING(pMember->pszNetbiosDomain);
    LW_SAFE_FREE_STRING(pMember->pszSamAccountName);
    LW_SAFE_FREE_STRING(pMember->pszSID);

    LwFreeMemory(pMember);
}
