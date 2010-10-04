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
 *        batch_marshal.c
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
#include "batch_marshal.h"

static
DWORD
LsaAdBatchMarshalUserInfoFixHomeDirectory(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN OUT PSTR* ppszHomeDirectory,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName
    )
{
    DWORD dwError = 0;
    PSTR pszHomeDirectory = *ppszHomeDirectory;
    PSTR pszNewHomeDirectory = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(pszHomeDirectory))
    {
        dwError = AD_GetUnprovisionedModeHomedirTemplate(
                      pState,
                      &pszHomeDirectory);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pszHomeDirectory);
    }

    if (strstr(pszHomeDirectory, "%"))
    {
        dwError = AD_BuildHomeDirFromTemplate(
                      pState,
                      pszHomeDirectory,
                      pszNetbiosDomainName,
                      pszSamAccountName,
                      &pszNewHomeDirectory);
        if (dwError)
        {
            // If we encounter a problem with fixing up the shell, leave the user object with the actual
            // value stored in AD and log the problem.
            LSA_LOG_INFO("While processing information for user (%s), an invalid homedir value was detected (homedir: '%s')",
                         LSA_SAFE_LOG_STRING(pszSamAccountName),
                         LSA_SAFE_LOG_STRING(pszHomeDirectory));
            dwError = 0;
            goto cleanup;
        }

        LW_SAFE_FREE_STRING(pszHomeDirectory);
        LSA_XFER_STRING(pszNewHomeDirectory, pszHomeDirectory);
    }

    LwStrCharReplace(pszHomeDirectory, ' ', '_');

cleanup:
    *ppszHomeDirectory = pszHomeDirectory;
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalUserInfoFixShell(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN OUT PSTR* ppszShell
    )
{
    DWORD dwError = 0;
    PSTR pszShell = *ppszShell;

    if (LW_IS_NULL_OR_EMPTY_STR(pszShell))
    {
        dwError = AD_GetUnprovisionedModeShell(
                      pState,
                      &pszShell);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pszShell);
    }

cleanup:
    *ppszShell = pszShell;
    return dwError;

error:
    goto cleanup;
}

// This function fills in all of the booleans in pObjectUserInfo except for
// bPromptPasswordChange and bAccountExpired
static
VOID
LsaAdBatchMarshalUserInfoAccountControl(
    IN UINT32 AccountControl,
    IN OUT PLSA_SECURITY_OBJECT_USER_INFO pObjectUserInfo
    )
{
    pObjectUserInfo->bPasswordNeverExpires = IsSetFlag(AccountControl, LSA_AD_UF_DONT_EXPIRE_PASSWD);
    if (pObjectUserInfo->bPasswordNeverExpires)
    {
        pObjectUserInfo->bPasswordExpired = FALSE;
    }
    else
    {
        pObjectUserInfo->bPasswordExpired = IsSetFlag(AccountControl, LSA_AD_UF_PASSWORD_EXPIRED);
    }
    pObjectUserInfo->bUserCanChangePassword = !IsSetFlag(AccountControl, LSA_AD_UF_CANT_CHANGE_PASSWD);
    pObjectUserInfo->bAccountDisabled = IsSetFlag(AccountControl, LSA_AD_UF_ACCOUNTDISABLE);
    pObjectUserInfo->bAccountLocked = IsSetFlag(AccountControl, LSA_AD_UF_LOCKOUT);
}

DWORD
LsaAdBatchMarshalUserInfoAccountExpires(
    IN UINT64 AccountExpires,
    IN OUT PLSA_SECURITY_OBJECT_USER_INFO pObjectUserInfo,
    IN PCSTR pszSamAccountName
    )
{
    DWORD dwError = 0;

    if (AccountExpires == 0LL ||
        AccountExpires == 9223372036854775807LL)
    {
        // This means the account will never expire.
        pObjectUserInfo->bAccountExpired = FALSE;
    }
    else
    {
        // in 100ns units:
        UINT64 currentNtTime = 0;

        dwError = ADGetCurrentNtTime(&currentNtTime);
        if (dwError)
        {
            LSA_LOG_INFO("While processing information for user (%s), lsass was unable to determine if the account is expired. Defaulting to not expired.", pszSamAccountName);
            dwError = 0;
            pObjectUserInfo->bAccountExpired = FALSE;
            goto error;
        }

        if (currentNtTime <= AccountExpires)
        {
            pObjectUserInfo->bAccountExpired = FALSE;
        }
        else
        {
            pObjectUserInfo->bAccountExpired = TRUE;
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaAdBatchMarshalUserInfoPasswordExpires(
    IN UINT64 PasswordExpires,
    IN OUT PLSA_SECURITY_OBJECT_USER_INFO pObjectUserInfo,
    IN PCSTR pszSamAccountName
    )
{
    DWORD dwError = 0;
    UINT64 currentNtTime = 0;

    dwError = ADGetCurrentNtTime(&currentNtTime);
    if (dwError)
    {
        LSA_LOG_INFO("While processing information for user (%s), lsass was unable to determine if the need to prompt to change user password is required. Defaulting to no.", pszSamAccountName);
        dwError = 0;
        pObjectUserInfo->bPromptPasswordChange = FALSE;
        goto error;
    }

    // ISSUE-2008/11/18-dalmeida -- The number of days
    // should be a setting.
    if (PasswordExpires != 0 &&
        (currentNtTime >= PasswordExpires ||
        (PasswordExpires - currentNtTime) / (10000000LL * 24*60*60) <= 14))
    {
        //The password will expire in 14 days or less
        pObjectUserInfo->bPromptPasswordChange = TRUE;
    }
    else
    {
        pObjectUserInfo->bPromptPasswordChange = FALSE;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalUnprovisionedUser(
    IN PAD_PROVIDER_DATA pProviderData,
    IN OUT PLSA_AD_BATCH_ITEM_USER_INFO pUserInfo,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    DWORD dwId = 0;
    PSTR pszNT4Name = NULL;
    PSTR pszPrimaryGroupSid = NULL;

    dwError = LwAllocateStringPrintf(
                   &pszNT4Name,
                   "%s\\%s",
                   pszNetbiosDomainName,
                   pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);
    // uid
    dwError = ADUnprovPlugin_QueryByReal(
                   pProviderData,
                   TRUE,
                   pszNT4Name,
                   pszSid,
                   &pUserInfo->pszAlias,
                   &dwId);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->uid = (uid_t)dwId;

    // gid
    dwError = LsaReplaceSidRid(
                    pszSid,
                    pUserInfo->dwPrimaryGroupRid,
                    &pszPrimaryGroupSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADUnprovPlugin_QueryByReal(
                   pProviderData,
                   FALSE,
                   NULL, // no knowledge of primarygroup's NT4 Name
                   pszPrimaryGroupSid,
                   NULL, //optional alias
                   &dwId);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->gid = (gid_t)dwId;


cleanup:
    LW_SAFE_FREE_STRING(pszNT4Name);
    LW_SAFE_FREE_STRING(pszPrimaryGroupSid);

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalUnprovisionedGroup(
    IN PAD_PROVIDER_DATA pProviderData,
    IN OUT PLSA_AD_BATCH_ITEM_GROUP_INFO pGroupInfo,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    DWORD dwId = 0;
    PSTR pszNT4Name = NULL;

    dwError = LwAllocateStringPrintf(
                   &pszNT4Name,
                   "%s\\%s",
                   pszNetbiosDomainName,
                   pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    // gid, alias
    dwError = ADUnprovPlugin_QueryByReal(
                   pProviderData,
                   FALSE,
                   pszNT4Name,
                   pszSid,
                   &pGroupInfo->pszAlias,
                   &dwId);
    BAIL_ON_LSA_ERROR(dwError);

    pGroupInfo->gid = (gid_t)dwId;

cleanup:
    LW_SAFE_FREE_STRING(pszNT4Name);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalUserInfo(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN OUT PLSA_AD_BATCH_ITEM_USER_INFO pUserInfo,
    OUT PLSA_SECURITY_OBJECT_USER_INFO pObjectUserInfo,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_DATA pProviderData = pState->pProviderData;

    pObjectUserInfo->bIsGeneratedUPN = FALSE;

    if (LsaAdBatchIsUnprovisionedMode(pProviderData))
    {
        dwError = LsaAdBatchMarshalUnprovisionedUser(
                        pProviderData,
                        pUserInfo,
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        pszSamAccountName,
                        pszSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pObjectUserInfo->uid = pUserInfo->uid;
    pObjectUserInfo->gid = pUserInfo->gid;

    if (pUserInfo->dwPrimaryGroupRid)
    {
        dwError = LsaReplaceSidRid(
            pszSid,
            pUserInfo->dwPrimaryGroupRid,
            &pObjectUserInfo->pszPrimaryGroupSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    LSA_XFER_STRING(pUserInfo->pszAlias, pObjectUserInfo->pszAliasName);
    LSA_XFER_STRING(pUserInfo->pszPasswd, pObjectUserInfo->pszPasswd);
    LSA_XFER_STRING(pUserInfo->pszGecos, pObjectUserInfo->pszGecos);
    LSA_XFER_STRING(pUserInfo->pszShell, pObjectUserInfo->pszShell);
    LSA_XFER_STRING(pUserInfo->pszHomeDirectory, pObjectUserInfo->pszHomedir);
    LSA_XFER_STRING(pUserInfo->pszUserPrincipalName, pObjectUserInfo->pszUPN);

    pObjectUserInfo->qwPwdLastSet = pUserInfo->PasswordLastSet;
    pObjectUserInfo->qwPwdExpires = pUserInfo->PasswordExpires;
    pObjectUserInfo->qwAccountExpires = pUserInfo->AccountExpires;

    // Handle shell.
    LwStripWhitespace(pObjectUserInfo->pszShell, TRUE, TRUE);
    dwError = LsaAdBatchMarshalUserInfoFixShell(
                  pState,
                  &pObjectUserInfo->pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    // Handle home directory.
    LwStripWhitespace(pObjectUserInfo->pszHomedir, TRUE, TRUE);
    dwError = LsaAdBatchMarshalUserInfoFixHomeDirectory(
                    pState,
                    &pObjectUserInfo->pszHomedir,
                    pszNetbiosDomainName,
                    pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    // Handle UPN.
    if (!pObjectUserInfo->pszUPN)
    {
        dwError = ADGetLDAPUPNString(
                        0,
                        NULL,
                        pszDnsDomainName,
                        pszSamAccountName,
                        &pObjectUserInfo->pszUPN,
                        &pObjectUserInfo->bIsGeneratedUPN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Decode account control flags.
    LsaAdBatchMarshalUserInfoAccountControl(
            pUserInfo->UserAccountControl,
            pObjectUserInfo);

    // Figure out account expiration.
    dwError = LsaAdBatchMarshalUserInfoAccountExpires(
                    pUserInfo->AccountExpires,
                    pObjectUserInfo,
                    pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    // Figure out password prompting.
    dwError = LsaAdBatchMarshalUserInfoPasswordExpires(
                    pUserInfo->PasswordExpires,
                    pObjectUserInfo,
                    pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalGroupInfo(
    IN PAD_PROVIDER_DATA pProviderData,
    IN OUT PLSA_AD_BATCH_ITEM_GROUP_INFO pGroupInfo,
    OUT PLSA_SECURITY_OBJECT_GROUP_INFO pObjectGroupInfo,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;

    if (LsaAdBatchIsUnprovisionedMode(pProviderData))
    {
        dwError = LsaAdBatchMarshalUnprovisionedGroup(
                        pProviderData,
                        pGroupInfo,
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        pszSamAccountName,
                        pszSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pObjectGroupInfo->gid = pGroupInfo->gid;

    LSA_XFER_STRING(pGroupInfo->pszAlias, pObjectGroupInfo->pszAliasName);
    LSA_XFER_STRING(pGroupInfo->pszPasswd, pObjectGroupInfo->pszPasswd);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaAdBatchMarshal(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_DATA pProviderData = pState->pProviderData;
    PLSA_SECURITY_OBJECT pObject = NULL;

    // To marshal, the following conditions to be satisfied:
    //
    // 1) Object must have user or group type.
    // 2) Object must have real information.
    if ((LSA_AD_BATCH_OBJECT_TYPE_UNDEFINED == pItem->ObjectType) ||
        !IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_REAL))
    {
        PCSTR pszType = NULL;
        BOOLEAN bIsString = FALSE;
        PCSTR pszString = NULL;
        DWORD dwId = 0;

        LsaAdBatchQueryTermDebugInfo(
                &pItem->QueryTerm,
                &pszType,
                &bIsString,
                &pszString,
                &dwId);
        if (bIsString)
        {
            LSA_LOG_VERBOSE("Did not find object by %s '%s'", pszType, pszString);
        }
        else
        {
            LSA_LOG_VERBOSE("Did not find object by %s %u", pszType, dwId);
        }
        dwError = 0;
        goto cleanup;
    }

    if (!IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO) &&
        !LsaAdBatchIsUnprovisionedMode(pProviderData))
    {
        SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED);
    }

#if 0
    // Disable in svn commit r45217
    // Fix regression that could cause NULL objects to be returned by LsaEnumObjects():
    // Don't filter out disabled objects when enumerating in the AD provider - If we
    // ever do filter out objects, don't leave NULLs in the array
    //
    // This code also has to be disabled for to support returning non-provisioned objects
    // (computers, users, ...) that need an access token for use in Likewise-CIFS

    if (IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED) &&
        (pItem->ObjectType != LSA_AD_BATCH_OBJECT_TYPE_GROUP))
    {
        // Skip any disabled non-groups.
        LSA_LOG_VERBOSE("Skipping disabled object (sid = %s, name = %s\\%s)",
                LSA_SAFE_LOG_STRING(pItem->pszSid),
                LSA_SAFE_LOG_STRING(pszNetbiosDomainName),
                LSA_SAFE_LOG_STRING(pItem->pszSamAccountName));
        dwError = 0;
        goto cleanup;
    }
#endif

    dwError = LwAllocateMemory(sizeof(*pObject), (PVOID*)&pObject);
    BAIL_ON_LSA_ERROR(dwError);

    pObject->version.qwDbId = -1;

    pObject->enabled = !IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED);

    // Transfer the data
    LSA_XFER_STRING(pItem->pszSid, pObject->pszObjectSid);
    LSA_XFER_STRING(pItem->pszSamAccountName, pObject->pszSamAccountName);
    LSA_XFER_STRING(pItem->pszDn, pObject->pszDN);

    dwError = LwAllocateString(
                    pszNetbiosDomainName,
                    &pObject->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pItem->ObjectType)
    {
        case LSA_AD_BATCH_OBJECT_TYPE_USER:
            pObject->type = LSA_OBJECT_TYPE_USER;
            dwError = LsaAdBatchMarshalUserInfo(
                            pState,
                            &pItem->UserInfo,
                            &pObject->userInfo,
                            pszDnsDomainName,
                            pObject->pszNetbiosDomainName,
                            pObject->pszSamAccountName,
                            pObject->pszObjectSid);
            BAIL_ON_LSA_ERROR(dwError);

            pObject->userInfo.bIsAccountInfoKnown =
                IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_ACCOUNT_INFO_KNOWN);
            break;

        case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
            pObject->type = LSA_OBJECT_TYPE_GROUP;
            dwError = LsaAdBatchMarshalGroupInfo(
                            pProviderData,
                            &pItem->GroupInfo,
                            &pObject->groupInfo,
                            pszDnsDomainName,
                            pObject->pszNetbiosDomainName,
                            pObject->pszSamAccountName,
                            pObject->pszObjectSid);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppObject = pObject;
    return dwError;

error:
    if (pObject)
    {
        ADCacheSafeFreeObject(&pObject);
    }
    goto cleanup;
}

DWORD
LsaAdBatchMarshalList(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    IN DWORD dwAvailableCount,
    OUT PLSA_SECURITY_OBJECT* ppObjects,
    OUT PDWORD pdwUsedCount
    )
{
    DWORD dwError = 0;
    PLSA_LIST_LINKS pLinks = NULL;
    DWORD dwIndex = 0;

    for (pLinks = pBatchItemList->Next;
         pLinks != pBatchItemList;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        if (dwIndex >= dwAvailableCount)
        {
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAdBatchMarshal(
                        pState,
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        pItem,
                        &ppObjects[dwIndex]);
        BAIL_ON_LSA_ERROR(dwError);
        if (ppObjects[dwIndex])
        {
            dwIndex++;
        }
    }

cleanup:
    *pdwUsedCount = dwIndex;
    return dwError;

error:
    goto cleanup;
}
