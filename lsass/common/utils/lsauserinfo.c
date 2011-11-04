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
 *        userinfo.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Authentication Provider Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "includes.h"


static
void
LsaFreeUserInfoContents_0(
    PLSA_USER_INFO_0 pUserInfo
    );

static
void
LsaFreeUserInfoContents_1(
    PLSA_USER_INFO_1 pUserInfo
    );

static
void
LsaFreeUserInfoContents_2(
    PLSA_USER_INFO_2 pUserInfo
    );

static
DWORD
LsaModifyUser_SetPasswordHash(
    PLW_LSA_DATA_BLOB *ppHashBlob,
    PCSTR pszHash
    );


static
void
LsaFreeUserInfoContents_0(
    PLSA_USER_INFO_0 pUserInfo
    )
{
    LW_SAFE_FREE_STRING(pUserInfo->pszName);
    LW_SECURE_FREE_STRING(pUserInfo->pszPasswd);
    LW_SAFE_FREE_STRING(pUserInfo->pszGecos);
    LW_SAFE_FREE_STRING(pUserInfo->pszShell);
    LW_SAFE_FREE_STRING(pUserInfo->pszHomedir);
    LW_SAFE_FREE_STRING(pUserInfo->pszSid);
}

static
void
LsaFreeUserInfoContents_1(
    PLSA_USER_INFO_1 pUserInfo
    )
{
    LsaFreeUserInfoContents_0(&pUserInfo->info0);
    LW_SAFE_FREE_STRING(pUserInfo->pszDN);
    LW_SAFE_FREE_STRING(pUserInfo->pszUPN);
    LW_SECURE_FREE_MEMORY(pUserInfo->pLMHash, pUserInfo->dwLMHashLen);
    LW_SECURE_FREE_MEMORY(pUserInfo->pNTHash, pUserInfo->dwNTHashLen);
}

static
void
LsaFreeUserInfoContents_2(
    PLSA_USER_INFO_2 pUserInfo
    )
{
    LsaFreeUserInfoContents_1(&pUserInfo->info1);
}

void
LsaFreeUserInfo(
    DWORD dwLevel,
    PVOID pUserInfo
    )
{
    switch(dwLevel)
    {
        case 0:
        {
            LsaFreeUserInfoContents_0((PLSA_USER_INFO_0)pUserInfo);
            break;
        }
        case 1:
        {
            LsaFreeUserInfoContents_1((PLSA_USER_INFO_1)pUserInfo);
            break;
        }
        case 2:
        {
            LsaFreeUserInfoContents_2((PLSA_USER_INFO_2)pUserInfo);
            break;
        }
        default:
        {
            LSA_LOG_ERROR("Unsupported User Info Level [%u]", dwLevel);
        }
    }
    if (dwLevel < 3)
    {
        LW_SAFE_FREE_MEMORY(pUserInfo);
    }
}

DWORD
LsaBuildUserModInfo(
    uid_t uid,
    PLSA_USER_MOD_INFO* ppUserModInfo
    )
{
    DWORD dwError = 0;
    PLSA_USER_MOD_INFO pUserModInfo = NULL;

    dwError = LwAllocateMemory(
                    sizeof(LSA_USER_MOD_INFO),
                    (PVOID*)&pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    pUserModInfo->uid = uid;

    *ppUserModInfo = pUserModInfo;

cleanup:

    return dwError;

error:

    *ppUserModInfo = NULL;

    if (pUserModInfo) {
       LsaFreeUserModInfo(pUserModInfo);
    }

    goto cleanup;
}

DWORD
LsaModifyUser_EnableUser(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    pUserModInfo->actions.bEnableUser = bValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_DisableUser(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    pUserModInfo->actions.bDisableUser = bValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_Unlock(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    pUserModInfo->actions.bUnlockUser = bValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_AddToGroups(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGroupList
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    LW_SAFE_FREE_STRING(pUserModInfo->pszAddToGroups);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszGroupList))
    {
       dwError = LwAllocateString(
                   pszGroupList,
                   &pUserModInfo->pszAddToGroups);
       BAIL_ON_LSA_ERROR(dwError);

       pUserModInfo->actions.bAddToGroups = TRUE;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_RemoveFromGroups(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGroupList
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    LW_SAFE_FREE_STRING(pUserModInfo->pszRemoveFromGroups);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszGroupList))
    {
       dwError = LwAllocateString(
                   pszGroupList,
                   &pUserModInfo->pszRemoveFromGroups);
       BAIL_ON_LSA_ERROR(dwError);

       pUserModInfo->actions.bRemoveFromGroups = TRUE;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_ChangePasswordAtNextLogon(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    pUserModInfo->actions.bSetChangePasswordOnNextLogon = bValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_SetPasswordNeverExpires(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    pUserModInfo->actions.bSetPasswordNeverExpires = bValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_SetPasswordMustExpire(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    pUserModInfo->actions.bSetPasswordMustExpire = bValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_SetExpiryDate(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszDate
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    LW_SAFE_FREE_STRING(pUserModInfo->pszExpiryDate);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszDate)) {
       struct tm timebuf;

       if (NULL == strptime(pszDate, "%Y-%m-%d", &timebuf)) {
          dwError = LW_ERROR_FAILED_TIME_CONVERSION;
          BAIL_ON_LSA_ERROR(dwError);
       }

       dwError = LwAllocateString(pszDate, &pUserModInfo->pszExpiryDate);
       BAIL_ON_LSA_ERROR(dwError);

       pUserModInfo->actions.bSetAccountExpiryDate = TRUE;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_SetPrimaryGroup(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGid
    )
{
    DWORD dwError = 0;
    gid_t Gid = -1;
    PSTR pszEndPtr = NULL;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszGid))
    {
        Gid = (gid_t) strtoul(pszGid, &pszEndPtr, 10);
        if (errno)
        {
            dwError = LwErrnoToWin32Error(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (pszGid == pszEndPtr)
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        pUserModInfo->gid = Gid;
        pUserModInfo->actions.bSetPrimaryGroup = TRUE;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaModifyUser_SetHomedir(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszHomedir
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszHomedir))
    {
        dwError = LwAllocateString(
                    pszHomedir,
                    &pUserModInfo->pszHomedir);
        BAIL_ON_LSA_ERROR(dwError);

        pUserModInfo->actions.bSetHomedir = TRUE;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaModifyUser_SetShell(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszShell
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszShell))
    {
        dwError = LwAllocateString(
                    pszShell,
                    &pUserModInfo->pszShell);
        BAIL_ON_LSA_ERROR(dwError);

        pUserModInfo->actions.bSetShell = TRUE;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaModifyUser_SetGecos(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGecos
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszGecos))
    {
        dwError = LwAllocateString(
                    pszGecos,
                    &pUserModInfo->pszGecos);
        BAIL_ON_LSA_ERROR(dwError);

        pUserModInfo->actions.bSetGecos = TRUE;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaModifyUser_SetPassword(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszPassword
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszPassword))
    {
        dwError = LwAllocateString(
                    pszPassword,
                    &pUserModInfo->pszPassword);
        BAIL_ON_LSA_ERROR(dwError);

        pUserModInfo->actions.bSetPassword = TRUE;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaModifyUser_SetNtPasswordHash(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszHash
    )
{
    DWORD dwError = 0;

    dwError = LsaModifyUser_SetPasswordHash(
                   &pUserModInfo->pNtPasswordHash,
                   pszHash);
    BAIL_ON_LSA_ERROR(dwError);

    pUserModInfo->actions.bSetNtPasswordHash = TRUE;

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaModifyUser_SetLmPasswordHash(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszHash
    )
{
    DWORD dwError = 0;

    dwError = LsaModifyUser_SetPasswordHash(
                   &pUserModInfo->pLmPasswordHash,
                   pszHash);
    BAIL_ON_LSA_ERROR(dwError);

    pUserModInfo->actions.bSetLmPasswordHash = TRUE;

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaModifyUser_SetPasswordHash(
    PLW_LSA_DATA_BLOB *ppHashBlob,
    PCSTR pszHash
    )
{
    DWORD dwError = 0;
    BYTE Hash[16] = {0};
    PCSTR pszHashCursor = NULL;
    DWORD i = 0;
    int ret = 0;
    PLW_LSA_DATA_BLOB pHashBlob = NULL;

    BAIL_ON_INVALID_POINTER(ppHashBlob);
    BAIL_ON_INVALID_POINTER(pszHash);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszHash)) {
        for (i = 0, pszHashCursor = pszHash;
             pszHashCursor[0] && pszHashCursor[1] && i < sizeof(Hash);
             i++, pszHashCursor += 2)
        {
            ret = sscanf(pszHashCursor, "%02hhx", &Hash[i]);
            if (ret == 0) {
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    dwError = LwAllocateMemory(sizeof(*pHashBlob),
                                (PVOID*)&pHashBlob);
    BAIL_ON_LSA_ERROR(dwError);

    pHashBlob->dwLen = sizeof(Hash);

    dwError = LwAllocateMemory(pHashBlob->dwLen,
                                (PVOID*)&pHashBlob->pData);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pHashBlob->pData, Hash, pHashBlob->dwLen);

    *ppHashBlob = pHashBlob;

cleanup:
    return dwError;

error:
    if (pHashBlob &&
        pHashBlob->pData) {
        LW_SECURE_FREE_MEMORY(pHashBlob->pData, pHashBlob->dwLen);
    }

    if (pHashBlob) {
        LW_SAFE_FREE_MEMORY(pHashBlob);
    }

    *ppHashBlob = NULL;

    goto cleanup;
}

void
LsaFreeUserModInfo(
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    LW_SAFE_FREE_STRING(pUserModInfo->pszAddToGroups);
    LW_SAFE_FREE_STRING(pUserModInfo->pszRemoveFromGroups);
    LW_SAFE_FREE_STRING(pUserModInfo->pszExpiryDate);
    LW_SAFE_FREE_STRING(pUserModInfo->pszHomedir);
    LW_SAFE_FREE_STRING(pUserModInfo->pszShell);
    LW_SAFE_FREE_STRING(pUserModInfo->pszGecos);
    LW_SECURE_FREE_STRING(pUserModInfo->pszPassword);

    if (pUserModInfo->pNtPasswordHash) {
        LW_SECURE_FREE_MEMORY(pUserModInfo->pNtPasswordHash->pData, pUserModInfo->pNtPasswordHash->dwLen);
    }
    LW_SAFE_FREE_MEMORY(pUserModInfo->pNtPasswordHash);

    if (pUserModInfo->pLmPasswordHash) {
        LW_SECURE_FREE_MEMORY(pUserModInfo->pLmPasswordHash->pData, pUserModInfo->pLmPasswordHash->dwLen);
    }
    LW_SAFE_FREE_MEMORY(pUserModInfo->pLmPasswordHash);

    LwFreeMemory(pUserModInfo);
}

void
LsaFreeUserModInfo2(
    PLSA_USER_MOD_INFO_2 pUserModInfo
    )
{
    LW_SAFE_FREE_STRING(pUserModInfo->pszSid);
    LW_SAFE_FREE_STRING(pUserModInfo->pszPrimaryGroupSid);
    LW_SAFE_FREE_STRING(pUserModInfo->pszAddToGroups);
    LW_SAFE_FREE_STRING(pUserModInfo->pszRemoveFromGroups);
    LW_SAFE_FREE_STRING(pUserModInfo->pszExpiryDate);
    LW_SAFE_FREE_STRING(pUserModInfo->pszHomedir);
    LW_SAFE_FREE_STRING(pUserModInfo->pszShell);
    LW_SAFE_FREE_STRING(pUserModInfo->pszGecos);
    LW_SECURE_FREE_STRING(pUserModInfo->pszPassword);

    if (pUserModInfo->pNtPasswordHash) {
        LW_SECURE_FREE_MEMORY(pUserModInfo->pNtPasswordHash->pData, pUserModInfo->pNtPasswordHash->dwLen);
    }
    LW_SAFE_FREE_MEMORY(pUserModInfo->pNtPasswordHash);

    if (pUserModInfo->pLmPasswordHash) {
        LW_SECURE_FREE_MEMORY(pUserModInfo->pLmPasswordHash->pData, pUserModInfo->pLmPasswordHash->dwLen);
    }
    LW_SAFE_FREE_MEMORY(pUserModInfo->pLmPasswordHash);

    LwFreeMemory(pUserModInfo);
}

void
LsaFreeUserAddInfo(
    PLSA_USER_ADD_INFO pUserAddInfo
    )
{
    LW_SAFE_FREE_STRING(pUserAddInfo->pszName);
    LW_SAFE_FREE_STRING(pUserAddInfo->pszPrimaryGroupSid);
    LW_SECURE_FREE_STRING(pUserAddInfo->pszPassword);
    LW_SAFE_FREE_STRING(pUserAddInfo->pszGecos);
    LW_SAFE_FREE_STRING(pUserAddInfo->pszShell);
    LW_SAFE_FREE_STRING(pUserAddInfo->pszHomedir);
    LW_SAFE_FREE_MEMORY(pUserAddInfo);
}


void
LsaFreeUserInfoList(
    DWORD  dwLevel,
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers
    )
{
    int iUser = 0;
    for (; iUser < dwNumUsers; iUser++) {
        PVOID pUserInfo = *(ppUserInfoList+iUser);
        if (pUserInfo) {
           LsaFreeUserInfo(dwLevel, pUserInfo);
        }
    }
    LwFreeMemory(ppUserInfoList);
}

void
LsaFreeIpcUserInfoList(
    PLSA_USER_INFO_LIST pUserIpcInfoList
    )
{
    if (pUserIpcInfoList)
    {
        switch (pUserIpcInfoList->dwUserInfoLevel)
        {
            case 0:
                LsaFreeUserInfoList(
                        0,
                        (PVOID*)pUserIpcInfoList->ppUserInfoList.ppInfoList0,
                        pUserIpcInfoList->dwNumUsers);
                break;
            case 1:
                LsaFreeUserInfoList(
                        1,
                        (PVOID*)pUserIpcInfoList->ppUserInfoList.ppInfoList1,
                        pUserIpcInfoList->dwNumUsers);
                break;
            case 2:
                LsaFreeUserInfoList(
                        2,
                        (PVOID*)pUserIpcInfoList->ppUserInfoList.ppInfoList2,
                        pUserIpcInfoList->dwNumUsers);
                break;

            default:
            {
                LSA_LOG_ERROR(
                        "Unsupported User Info Level [%u]",
                        pUserIpcInfoList->dwUserInfoLevel);
            }
        }
        LwFreeMemory(pUserIpcInfoList);
    }
}

DWORD
LsaValidateUserName(
    PCSTR pszName
    )
{
    DWORD dwError = 0;
    size_t sNameLen = 0;

    sNameLen = strlen(pszName);
    if (sNameLen > LSA_MAX_USER_NAME_LENGTH || sNameLen == 0)
    {
        dwError = LW_ERROR_INVALID_USER_NAME;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:

    goto cleanup;
}

DWORD
LsaValidateUserInfoLevel(
    DWORD dwUserInfoLevel
    )
{
    return ((dwUserInfoLevel >= 0) &&
            (dwUserInfoLevel <= 2)) ? 0 : LW_ERROR_INVALID_USER_INFO_LEVEL;
}

DWORD
LsaValidateUserInfo(
    PVOID pUserInfo,
    DWORD dwUserInfoLevel
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserInfo);

    dwError = LsaValidateUserInfoLevel(dwUserInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

    switch (dwUserInfoLevel)
    {
        case 0:

            {
                PLSA_USER_INFO_0 pUserInfo_0 = (PLSA_USER_INFO_0)pUserInfo;

                dwError = LsaValidateUserName(pUserInfo_0->pszName);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }

        case 1:

            {
                PLSA_USER_INFO_1 pUserInfo_1 = (PLSA_USER_INFO_1)pUserInfo;

                dwError = LsaValidateUserName(pUserInfo_1->pszName);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }

        case 2:

            {
                PLSA_USER_INFO_2 pUserInfo_2 = (PLSA_USER_INFO_2)pUserInfo;

                dwError = LsaValidateUserName(pUserInfo_2->pszName);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }

        default:

            dwError = LW_ERROR_UNSUPPORTED_USER_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);

            break;
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
