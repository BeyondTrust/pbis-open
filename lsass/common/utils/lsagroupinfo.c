/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        groupinfo.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Group Info
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "includes.h"

static
DWORD
LsaAllocateGroupInfo_0(
    PLSA_GROUP_INFO_0 *ppDstGroupInfo,
    DWORD dwLevel,
    PLSA_GROUP_INFO_0 pSrcGroupInfo
    );

static
DWORD
LsaAllocateGroupInfo_1(
    PLSA_GROUP_INFO_1 *ppDstGroupInfo,
    DWORD dwLevel,
    PLSA_GROUP_INFO_1 pSrcGroupInfo
    );

static
void
LsaFreeGroupInfo_0(
    PLSA_GROUP_INFO_0 pGroupInfo
    )
{
    LW_SAFE_FREE_STRING(pGroupInfo->pszName);
    LW_SAFE_FREE_STRING(pGroupInfo->pszSid);
    LwFreeMemory(pGroupInfo);
}

static
void
LsaFreeGroupInfo_1(
    PLSA_GROUP_INFO_1 pGroupInfo
    )
{
    LW_SAFE_FREE_STRING(pGroupInfo->pszName);
    LW_SAFE_FREE_STRING(pGroupInfo->pszPasswd);
    LW_SAFE_FREE_STRING(pGroupInfo->pszSid);
    LW_SAFE_FREE_STRING(pGroupInfo->pszDN);
    LW_SAFE_FREE_STRING_ARRAY(pGroupInfo->ppszMembers);
    LwFreeMemory(pGroupInfo);
}

void
LsaFreeGroupInfo(
    DWORD  dwLevel,
    PVOID  pGroupInfo
    )
{
    switch(dwLevel)
    {
        case 0:
        {
            LsaFreeGroupInfo_0((PLSA_GROUP_INFO_0)pGroupInfo);
            break;
        }
        case 1:
        {
            LsaFreeGroupInfo_1((PLSA_GROUP_INFO_1)pGroupInfo);
            break;
        }
        default:
        {
            LSA_LOG_ERROR("Unsupported Group Info Level [%u]", dwLevel);
        }
    }
}

DWORD
LsaAllocateGroupInfo(
    PVOID *ppDstGroupInfo,
    DWORD dwLevel,
    PVOID pSrcGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;

    switch (dwLevel)
    {
    case 0:
        dwError = LsaAllocateGroupInfo_0((PLSA_GROUP_INFO_0*)&pGroupInfo,
                                         dwLevel,
                                         (PLSA_GROUP_INFO_0)pSrcGroupInfo);
        break;

    case 1:
        dwError = LsaAllocateGroupInfo_1((PLSA_GROUP_INFO_1*)&pGroupInfo,
                                         dwLevel,
                                         (PLSA_GROUP_INFO_1)pSrcGroupInfo);
        break;

    default:
        dwError = LW_ERROR_INVALID_GROUP_INFO_LEVEL;
    }
    BAIL_ON_LSA_ERROR(dwError);

    *ppDstGroupInfo = pGroupInfo;

cleanup:
    return dwError;

error:
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwLevel, pGroupInfo);
    }

    *ppDstGroupInfo = NULL;
    goto cleanup;
}

static
DWORD
LsaAllocateGroupInfo_0(
    PLSA_GROUP_INFO_0 *ppDstGroupInfo,
    DWORD dwLevel,
    PLSA_GROUP_INFO_0 pSrcGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pGroupInfo),
                                (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pSrcGroupInfo) {
        pGroupInfo->gid = pSrcGroupInfo->gid;

        if (pSrcGroupInfo->pszName) {
            dwError = LwAllocateString(pSrcGroupInfo->pszName,
                                        &pGroupInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (pSrcGroupInfo->pszSid) {
            dwError = LwAllocateString(pSrcGroupInfo->pszSid,
                                        &pGroupInfo->pszSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *ppDstGroupInfo = pGroupInfo;

cleanup:
    return dwError;

error:
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwLevel, pGroupInfo);
    }

    *ppDstGroupInfo = NULL;
    goto cleanup;
}

static
DWORD
LsaAllocateGroupInfo_1(
    PLSA_GROUP_INFO_1 *ppDstGroupInfo,
    DWORD dwLevel,
    PLSA_GROUP_INFO_1 pSrcGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pGroupInfo),
                                (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pSrcGroupInfo) {
        pGroupInfo->gid = pSrcGroupInfo->gid;

        if (pSrcGroupInfo->pszName) {
            dwError = LwAllocateString(pSrcGroupInfo->pszName,
                                        &pGroupInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (pSrcGroupInfo->pszSid) {
            dwError = LwAllocateString(pSrcGroupInfo->pszSid,
                                        &pGroupInfo->pszSid);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (pSrcGroupInfo->pszDN) {
            dwError = LwAllocateString(pSrcGroupInfo->pszDN,
                                        &pGroupInfo->pszDN);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (pSrcGroupInfo->pszPasswd) {
            dwError = LwAllocateString(pSrcGroupInfo->pszPasswd,
                                        &pGroupInfo->pszPasswd);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *ppDstGroupInfo = pGroupInfo;

cleanup:
    return dwError;

error:
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwLevel, pGroupInfo);
    }

    *ppDstGroupInfo = NULL;
    goto cleanup;
}

DWORD
LsaBuildGroupModInfo(
    gid_t gid,
    PLSA_GROUP_MOD_INFO* ppGroupModInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_MOD_INFO pGroupModInfo = NULL;

    dwError = LwAllocateMemory(
                    sizeof(LSA_GROUP_MOD_INFO),
                    (PVOID*)&pGroupModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    pGroupModInfo->gid = gid;

    *ppGroupModInfo = pGroupModInfo;

cleanup:
    return dwError;

error:
    if (pGroupModInfo) {
        LsaFreeGroupModInfo(pGroupModInfo);
    }

    *ppGroupModInfo = NULL;
    goto cleanup;
}

DWORD
LsaModifyGroup_AddMembers(
    PLSA_GROUP_MOD_INFO pGroupModInfo,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PCSTR pszSID = NULL;
    DWORD iMember = 0;

    BAIL_ON_INVALID_POINTER(pGroupModInfo);

    pszSID = (PCSTR)pData;
    pGroupModInfo->dwAddMembersNum++;

    dwError = LwReallocMemory(pGroupModInfo->pAddMembers,
                               (PVOID*)&pGroupModInfo->pAddMembers,
                               sizeof(pGroupModInfo->pAddMembers[0]) *
                               pGroupModInfo->dwAddMembersNum);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszSID)
    {
        iMember = pGroupModInfo->dwAddMembersNum - 1;

        dwError = LwAllocateString(
                    pszSID,
                    &pGroupModInfo->pAddMembers[iMember].pszSid);
        BAIL_ON_LSA_ERROR(dwError);

        pGroupModInfo->actions.bAddMembers = TRUE;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaModifyGroup_RemoveMembers(
    PLSA_GROUP_MOD_INFO pGroupModInfo,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PCSTR pszSID = NULL;
    DWORD iMember = 0;

    BAIL_ON_INVALID_POINTER(pGroupModInfo);

    pszSID = (PCSTR)pData;
    pGroupModInfo->dwRemoveMembersNum++;

    dwError = LwReallocMemory(pGroupModInfo->pRemoveMembers,
                               (PVOID*)&pGroupModInfo->pRemoveMembers,
                               sizeof(pGroupModInfo->pRemoveMembers[0]) *
                               pGroupModInfo->dwRemoveMembersNum);
    BAIL_ON_LSA_ERROR(dwError);


    if (pszSID)
    {
        iMember = pGroupModInfo->dwRemoveMembersNum - 1;

        dwError = LwAllocateString(
                    pszSID,
                    &pGroupModInfo->pRemoveMembers[iMember].pszSid);
        BAIL_ON_LSA_ERROR(dwError);

        pGroupModInfo->actions.bRemoveMembers = TRUE;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

void
LsaFreeGroupModInfo(
    PLSA_GROUP_MOD_INFO pGroupModInfo
    )
{
    DWORD i = 0;

    for (i = 0; i < pGroupModInfo->dwAddMembersNum; i++)
    {
        LW_SAFE_FREE_STRING(pGroupModInfo->pAddMembers[i].pszSid);
    }
    LW_SAFE_FREE_MEMORY(pGroupModInfo->pAddMembers);

    for (i = 0; i < pGroupModInfo->dwRemoveMembersNum; i++)
    {
        LW_SAFE_FREE_STRING(pGroupModInfo->pRemoveMembers[i].pszSid);
    }
    LW_SAFE_FREE_MEMORY(pGroupModInfo->pRemoveMembers);

    LwFreeMemory(pGroupModInfo);
}

void
LsaFreeGroupModInfo2(
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo
    )
{
    LW_SAFE_FREE_STRING(pGroupModInfo->pszSid);
    LwFreeStringArray(pGroupModInfo->ppszAddMembers, pGroupModInfo->dwAddMembersNum);
    LwFreeStringArray(pGroupModInfo->ppszRemoveMembers, pGroupModInfo->dwRemoveMembersNum);
    LwFreeMemory(pGroupModInfo);
}

void
LsaFreeGroupAddInfo(
    PLSA_GROUP_ADD_INFO pGroupAddInfo
    )
{
    DWORD i = 0;

    LW_SAFE_FREE_STRING(pGroupAddInfo->pszName);

    if (pGroupAddInfo->ppszMemberSids)
    {
        for (i = 0; i < pGroupAddInfo->dwMemberCount; i++)
        {
            LW_SAFE_FREE_STRING(pGroupAddInfo->ppszMemberSids[i]);
        }
    }

    LwFreeMemory(pGroupAddInfo);
}

void
LsaFreeIpcGroupInfoList(
    PLSA_GROUP_INFO_LIST pGroupIpcInfoList
    )
{
    if (pGroupIpcInfoList)
    {
        switch (pGroupIpcInfoList->dwGroupInfoLevel)
        {
            case 0:
                LsaFreeGroupInfoList(0, (PVOID*)pGroupIpcInfoList->ppGroupInfoList.ppInfoList0, pGroupIpcInfoList->dwNumGroups);
                break;
            case 1:
                LsaFreeGroupInfoList(1, (PVOID*)pGroupIpcInfoList->ppGroupInfoList.ppInfoList1, pGroupIpcInfoList->dwNumGroups);
                break;

            default:
            {
                LSA_LOG_ERROR("Unsupported Group Info Level [%u]", pGroupIpcInfoList->dwGroupInfoLevel);
            }
        }
        LwFreeMemory(pGroupIpcInfoList);
    }
}

void
LsaFreeGroupInfoList(
    DWORD  dwLevel,
    PVOID* pGroupInfoList,
    DWORD  dwNumGroups
    )
{
    DWORD iGroup = 0;
    for (;iGroup < dwNumGroups; iGroup++) {
        PVOID pGroupInfo = *(pGroupInfoList+iGroup);
        if (pGroupInfo) {
           LsaFreeGroupInfo(dwLevel, pGroupInfo);
        }
    }
    LwFreeMemory(pGroupInfoList);
}

DWORD
LsaValidateGroupInfoLevel(
    DWORD dwGroupInfoLevel
    )
{
    return (((dwGroupInfoLevel < 0) || (dwGroupInfoLevel > 1)) ? LW_ERROR_INVALID_GROUP_INFO_LEVEL : 0);
}

DWORD
LsaValidateGroupName(
    PCSTR pszName
    )
{
    DWORD dwError = 0;
    size_t sNameLen = 0;

    sNameLen = strlen(pszName);
    if (sNameLen > LSA_MAX_GROUP_NAME_LENGTH || sNameLen == 0)
    {
        dwError = LW_ERROR_INVALID_GROUP_NAME;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:

    goto cleanup;
}

DWORD
LsaValidateGroupInfo(
    PVOID pGroupInfo,
    DWORD dwGroupInfoLevel
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pGroupInfo);

    dwError = LsaValidateGroupInfoLevel(dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

    switch (dwGroupInfoLevel)
    {
        case 0:
        {
            PLSA_GROUP_INFO_0 pGroupInfo_0 =
                (PLSA_GROUP_INFO_0)pGroupInfo;

            dwError = LsaValidateGroupName(pGroupInfo_0->pszName);
            BAIL_ON_LSA_ERROR(dwError);

            break;
        }

        case 1:
        {
            PLSA_GROUP_INFO_1 pGroupInfo_1 =
                (PLSA_GROUP_INFO_1)pGroupInfo;

            dwError = LsaValidateGroupName(pGroupInfo_1->pszName);
            BAIL_ON_LSA_ERROR(dwError);

            break;
        }

        default:

            dwError = LW_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

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
