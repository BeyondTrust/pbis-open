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
 *        ad_users.c
 *
 * Abstract:
 *
 *        User monitor service for users and groups
 *
 *        Functions for enumerating and tracking ad users.
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#include "includes.h"

#define AD_PROVIDER_REGKEY "Services\\lsass\\Parameters\\Providers\\ActiveDirectory"
#define AD_PROVIDER_POLICY_REGKEY "Policy\\" AD_PROVIDER_REGKEY

VOID
UmnSrvFreeADUserContents(
    PAD_USER_INFO pUser
    )
{
    LW_SAFE_FREE_MEMORY(pUser->pszDN);
    LW_SAFE_FREE_MEMORY(pUser->pszObjectSid);
    LW_SAFE_FREE_MEMORY(pUser->pszNetbiosDomainName);
    LW_SAFE_FREE_MEMORY(pUser->pszSamAccountName);
    LW_SAFE_FREE_MEMORY(pUser->pszPrimaryGroupSid);
    LW_SAFE_FREE_MEMORY(pUser->pszUPN);
    LW_SAFE_FREE_MEMORY(pUser->pszAliasName);
    LW_SAFE_FREE_MEMORY(pUser->pszWindowsHomeFolder);
    LW_SAFE_FREE_MEMORY(pUser->pszLocalWindowsHomeFolder);

    LW_SAFE_FREE_MEMORY(pUser->pw_name);
    LW_SAFE_FREE_MEMORY(pUser->pw_passwd);
    LW_SAFE_FREE_MEMORY(pUser->pw_gecos);
    LW_SAFE_FREE_MEMORY(pUser->pw_dir);
    LW_SAFE_FREE_MEMORY(pUser->pw_shell);
    LW_SAFE_FREE_MEMORY(pUser->pDisplayName);
}

/**
 * @brief Report true if usermonitor considers the user "enabled",
 * this controls whether usermonitor reports on this user.
 */
static inline
BOOL
UmnSrvUserIsEnabled(
        const PLSA_SECURITY_OBJECT pUser
        )
{
    return (pUser->enabled);
}

static
DWORD
UmnSrvAddUsersFromMembership(
    HANDLE hLsass,
    LW_HASH_TABLE *pUsers,
    PCSTR pLookup
    )
{
    DWORD dwError = 0;
    LSA_QUERY_LIST list;
    PLSA_SECURITY_OBJECT *ppObjects = NULL;
    PLSA_SECURITY_OBJECT *ppMembers = NULL;
    DWORD memberCount = 0;
    DWORD i = 0;

    list.ppszStrings = &pLookup;

    dwError = LsaFindObjects(
                    hLsass,
                    NULL,
                    0,
                    LSA_OBJECT_TYPE_UNDEFINED,
                    LSA_QUERY_TYPE_BY_NT4,
                    1,
                    list,
                    &ppObjects);
    switch(dwError)
    {
        // The string was not a valid NT4 name
        case LW_ERROR_INVALID_PARAMETER:
        // The user/group does not exist
        case LW_ERROR_NO_SUCH_OBJECT:
            goto cleanup;
        default:
            BAIL_ON_UMN_ERROR(dwError);
    }

    if (ppObjects[0] && ppObjects[0]->type == LSA_OBJECT_TYPE_USER)
    {
        UMN_LOG_DEBUG("Processing AD user %s (%s)",
                ppObjects[0]->pszSamAccountName,
                ppObjects[0]->pszObjectSid);

        if (UmnSrvUserIsEnabled(ppObjects[0]) &&
                !LwHashExists(pUsers, ppObjects[0]->pszObjectSid))
        {
            UMN_LOG_VERBOSE("Found AD user %s that can login",
                        ppObjects[0]->userInfo.pszUnixName);

            dwError = LwHashSetValue(
                            pUsers,
                            ppObjects[0]->pszObjectSid,
                            ppObjects[0]);
            BAIL_ON_UMN_ERROR(dwError);
            ppObjects[0] = NULL;
        }
    }
    else if (ppObjects[0] && ppObjects[0]->type == LSA_OBJECT_TYPE_GROUP)
    {
        UMN_LOG_DEBUG("Querying membership of group %s", ppObjects[0]->pszSamAccountName);
        dwError = LsaQueryExpandedGroupMembers(
                        hLsass,
                        NULL,
                        0,
                        LSA_OBJECT_TYPE_USER,
                        ppObjects[0]->pszObjectSid,
                        &memberCount,
                        &ppMembers);
        BAIL_ON_UMN_ERROR(dwError);

        UMN_LOG_DEBUG("Group %s membership count %d", ppObjects[0]->pszSamAccountName, memberCount);

        for (i = 0; i < memberCount; i++)
        {
            UMN_LOG_DEBUG("Processing group %s member %s (%s)",
                    ppObjects[0]->pszSamAccountName,
                    ppMembers[i]->pszSamAccountName,
                    ppMembers[i]->pszObjectSid);

            if (UmnSrvUserIsEnabled(ppMembers[i]) &&
                    !LwHashExists(pUsers, ppMembers[i]->pszObjectSid))
            {
                UMN_LOG_VERBOSE("Found AD user %s that can login because of group %s",
                        ppMembers[i]->userInfo.pszUnixName, pLookup);

                dwError = LwHashSetValue(
                                pUsers,
                                ppMembers[i]->pszObjectSid,
                                ppMembers[i]);
                BAIL_ON_UMN_ERROR(dwError);
                ppMembers[i] = NULL;
            }
        }
    }

cleanup:
    if (ppObjects)
    {
        LsaFreeSecurityObjectList(
            1,
            ppObjects);
    }
    if (ppMembers)
    {
        LsaFreeSecurityObjectList(
            memberCount,
            ppMembers);
    }
    return dwError;

error:
    goto cleanup;
}

static
void
UmnSrvHashFreeObjectValue(
    const LW_HASH_ENTRY *pEntry
    )
{
    PLSA_SECURITY_OBJECT pUser = (PLSA_SECURITY_OBJECT)pEntry->pValue;

    if (pUser)
    {
        LsaFreeSecurityObject(pUser);
    }
}


DWORD
UmnSrvReadADUser(
    PCSTR pParentKey,
    PCSTR pName,
    PAD_USER_INFO pADUser
    )
{
    DWORD dwError = 0;
    PSTR pUserPath = NULL;

    /* n.b. not all attributes are written to
     * the registry, see UmnSrvWriteADUserValues()
     */
    LWREG_CONFIG_ITEM userLayout[] =
    {
        {
            "pw_name",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pw_name,
            NULL
        },
        {
            "pw_passwd",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pw_passwd,
            NULL
        },
        {
            "pw_uid",
            FALSE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &pADUser->pw_uid,
            NULL
        },
        {
            "pw_gid",
            FALSE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &pADUser->pw_gid,
            NULL
        },
        {
            "pw_gecos",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pw_gecos,
            NULL
        },
        {
            "pw_dir",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pw_dir,
            NULL
        },
        {
            "pw_shell",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pw_shell,
            NULL
        },
        {
            "pDisplayname",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pDisplayName,
            NULL
        },
        {
            "ad_objectsid",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pszObjectSid,
            NULL
        },
        {
            "ad_dn",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pszDN,
            NULL
        },
        {
            "ad_netbiosdomainname",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pszNetbiosDomainName,
            NULL
        },
        {
            "ad_samaccountname",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pszSamAccountName,
            NULL
        },
        {
            "ad_primarygroupsid",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pszPrimaryGroupSid,
            NULL
        },
        {
            "ad_upn",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pszUPN,
            NULL
        },
        {
            "ad_aliasname",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pszAliasName,
            NULL
        },
        {
            "ad_passwordexpired",
            FALSE,
            LwRegTypeBoolean,
            0,
            -1,
            NULL,
            &pADUser->bPasswordExpired,
            NULL
        },
        {
            "ad_passwordneverexpires",
            FALSE,
            LwRegTypeBoolean,
            0,
            -1,
            NULL,
            &pADUser->bPasswordNeverExpires,
            NULL
        },
        {
            "ad_promptpasswordchange",
            FALSE,
            LwRegTypeBoolean,
            0,
            -1,
            NULL,
            &pADUser->bPromptPasswordChange,
            NULL
        },
        {
            "ad_usercanchangepassword",
            FALSE,
            LwRegTypeBoolean,
            0,
            -1,
            NULL,
            &pADUser->bUserCanChangePassword,
            NULL
        },
        {
            "ad_accountdisabled",
            FALSE,
            LwRegTypeBoolean,
            0,
            -1,
            NULL,
            &pADUser->bAccountDisabled,
            NULL
        },
        {
            "ad_accountexpired",
            FALSE,
            LwRegTypeBoolean,
            0,
            -1,
            NULL,
            &pADUser->bAccountExpired,
            NULL
        },
        {
            "ad_accountlocked",
            FALSE,
            LwRegTypeBoolean,
            0,
            -1,
            NULL,
            &pADUser->bAccountLocked,
            NULL
        },
        {
            "ad_windowshomefolder",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pszWindowsHomeFolder,
            NULL
        },
        {
            "ad_localwindowshomefolder",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pADUser->pszLocalWindowsHomeFolder,
            NULL
        },
        {
            "LastUpdated",
            FALSE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &pADUser->LastUpdated,
            NULL
        },
    };

    UMN_LOG_VERBOSE("Reading previous values for user '%s'",
                    pName);

    dwError = LwAllocateStringPrintf(
                    &pUserPath,
                    "Services\\" SERVICE_NAME "\\Parameters\\%s\\%s",
                    pParentKey,
                    pName);
    BAIL_ON_UMN_ERROR(dwError);

    pADUser->version = AD_USER_INFO_VERSION;
    dwError = LwRegProcessConfig(
                pUserPath,
                NULL,
                userLayout,
                sizeof(userLayout)/sizeof(userLayout[0]));
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pUserPath);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
UmnSrvGetADUserEventDescription(PWSTR *ppszDescription,
            PCSTR pOperation,
            const char * const oldTimeBuf,
            PAD_USER_INFO pOld,
            const char * const newTimeBuf,
            PLSA_SECURITY_OBJECT pNew)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    dwError = LwAllocateWc16sPrintfW(
                    ppszDescription,
                    L"Between %hhs and %hhs, user '%hhs' was %hhs.\n"
                    L"Passwd (from passwd struct)\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Uid\n"
                    L"\tOld: %d\n"
                    L"\tNew: %d\n"
                    L"Primary group id\n"
                    L"\tOld: %d\n"
                    L"\tNew: %d\n"
                    L"Gecos\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Home directory\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Shell\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Display Name\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"DN\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"SID\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Netbios domain name\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Sam account name\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Primary group SID\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"UPN\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Alias name\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Generated UPN\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Password expired\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Password never expires\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Must change password\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Can change password\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Account disabled\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Account expired\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Account locked\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Windows home folder\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Local Windows home folder\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs",
                    oldTimeBuf,
                    newTimeBuf,
                    pOld ? pOld->pw_name : pNew->userInfo.pszUnixName,
                    pOperation,
                    pOld ? pOld->pw_passwd : "",
                    pNew ? (pNew->userInfo.pszPasswd ?  pNew->userInfo.pszPasswd : "x") : "",
                    pOld ? pOld->pw_uid : -1,
                    pNew ? pNew->userInfo.uid : -1,
                    pOld ? pOld->pw_gid : -1,
                    pNew ? pNew->userInfo.gid : -1,
                    pOld ? pOld->pw_gecos : "",
                    (pNew && pNew->userInfo.pszGecos) ?  pNew->userInfo.pszGecos : "",
                    pOld ? pOld->pw_dir : "",
                    pNew ? pNew->userInfo.pszHomedir : "",
                    pOld ? pOld->pw_shell : "",
                    pNew ? pNew->userInfo.pszShell : "",
                    pOld ? pOld->pDisplayName : "",
                    pNew ? pNew->userInfo.pszDisplayName : "",

                    pOld ? pOld->pszDN : "",
                    (pNew && pNew->pszDN) ? pNew->pszDN : "",

                    pOld ? pOld->pszObjectSid : "",
                    (pNew && pNew->pszObjectSid) ? pNew->pszObjectSid : "",

                    pOld ? pOld->pszNetbiosDomainName : "",
                    (pNew && pNew->pszNetbiosDomainName) ? pNew->pszNetbiosDomainName : "",

                    pOld ? pOld->pszSamAccountName : "",
                    (pNew && pNew->pszSamAccountName) ? pNew->pszSamAccountName : "",

                    pOld ? pOld->pszPrimaryGroupSid : "",
                    (pNew && pNew->userInfo.pszPrimaryGroupSid) ? pNew->userInfo.pszPrimaryGroupSid : "",

                    pOld ? pOld->pszUPN : "",
                    (pNew && pNew->userInfo.pszUPN) ? pNew->userInfo.pszUPN : "",

                    pOld ? pOld->pszAliasName : "",
                    (pNew && pNew->userInfo.pszAliasName) ? pNew->userInfo.pszAliasName : "",

                    pOld ? (pOld->bIsGeneratedUPN ? "true" : "false") : "",
                    pNew ? (pNew->userInfo.bIsGeneratedUPN ? "true" : "false") : "",

                    pOld ? (pOld->bPasswordExpired ? "true" : "false") : "",
                    pNew ? (pNew->userInfo.bPasswordExpired ? "true" : "false") : "",

                    pOld ? (pOld->bPasswordNeverExpires ? "true" : "false") : "",
                    pNew ? (pNew->userInfo.bPasswordNeverExpires ? "true" : "false") : "",

                    pOld ? (pOld->bPromptPasswordChange ? "true" : "false") : "",
                    pNew ? (pNew->userInfo.bPromptPasswordChange ? "true" : "false") : "",

                    pOld ? (pOld->bUserCanChangePassword ? "true" : "false") : "",
                    pNew ? (pNew->userInfo.bUserCanChangePassword ? "true" : "false") : "",

                    pOld ? (pOld->bAccountDisabled ? "true" : "false") : "",
                    pNew ? (pNew->userInfo.bAccountDisabled ? "true" : "false") : "",

                    pOld ? (pOld->bAccountExpired ? "true" : "false") : "",
                    pNew ? (pNew->userInfo.bAccountExpired ? "true" : "false") : "",

                    pOld ? (pOld->bAccountLocked ? "true" : "false") : "",
                    pNew ? (pNew->userInfo.bAccountLocked ? "true" : "false") : "",

                    pOld ? pOld->pszWindowsHomeFolder : "",
                    (pNew && pNew->userInfo.pszWindowsHomeFolder) ? pNew->userInfo.pszWindowsHomeFolder : "",

                    pOld ? pOld->pszLocalWindowsHomeFolder : "",
                    (pNew && pNew->userInfo.pszLocalWindowsHomeFolder) ? pNew->userInfo.pszLocalWindowsHomeFolder : ""
            );

    return dwError;
}


DWORD
UmnSrvWriteADUserEvent(
    PLW_EVENTLOG_CONNECTION pEventlog,
    long long PreviousRun,
    PAD_USER_INFO pOld,
    long long Now,
    PLSA_SECURITY_OBJECT pNew
    )
{
    DWORD dwError = 0;
    // Do not free. The field values are borrowed from other structures.
    AD_USER_CHANGE change = { { 0 } };
    LW_EVENTLOG_RECORD record = { 0 };
    char oldTimeBuf[128] = { 0 };
    char newTimeBuf[128] = { 0 };
    struct tm oldTmBuf = { 0 };
    struct tm newTmBuf = { 0 };
    time_t temp = 0;
    PCSTR pOperation = NULL;

    if (PreviousRun)
    {
        temp = PreviousRun;
        localtime_r(&temp, &oldTmBuf);
        strftime(
                oldTimeBuf,
                sizeof(oldTimeBuf),
                "%Y/%m/%d %H:%M:%S",
                &oldTmBuf);
    }
    else
    {
        strcpy(oldTimeBuf, "unknown");
    }
    temp = Now;
    localtime_r(&temp, &newTmBuf);

    strftime(
            newTimeBuf,
            sizeof(newTimeBuf),
            "%Y/%m/%d %H:%M:%S",
            &newTmBuf);

    if (pOld)
    {
        memcpy(&change.OldValue, pOld, sizeof(change.OldValue));
    }

    if (pNew)
    {
        assert(pNew->type == LSA_OBJECT_TYPE_USER);

        change.ADNewValue.version = AD_USER_INFO_VERSION;

        change.ADNewValue.pszDN = pNew->pszDN;
        change.ADNewValue.pszObjectSid = pNew->pszObjectSid;
        change.ADNewValue.enabled = pNew->enabled;
        change.ADNewValue.bIsLocal = pNew->bIsLocal;
        change.ADNewValue.pszNetbiosDomainName = pNew->pszNetbiosDomainName;
        change.ADNewValue.pszSamAccountName = pNew->pszSamAccountName;
        change.ADNewValue.pszPrimaryGroupSid = pNew->userInfo.pszPrimaryGroupSid;
        change.ADNewValue.pszUPN = pNew->userInfo.pszUPN;
        change.ADNewValue.pszAliasName = pNew->userInfo.pszAliasName;

        change.ADNewValue.qwPwdLastSet = pNew->userInfo.qwPwdLastSet;
        change.ADNewValue.qwMaxPwdAge = pNew->userInfo.qwMaxPwdAge;
        change.ADNewValue.qwPwdExpires = pNew->userInfo.qwPwdExpires;
        change.ADNewValue.qwAccountExpires = pNew->userInfo.qwAccountExpires;

        change.ADNewValue.bIsGeneratedUPN = pNew->userInfo.bIsGeneratedUPN;
        change.ADNewValue.bIsAccountInfoKnown = pNew->userInfo.bIsAccountInfoKnown;
        change.ADNewValue.bPasswordExpired = pNew->userInfo.bPasswordExpired;
        change.ADNewValue.bPasswordNeverExpires = pNew->userInfo.bPasswordNeverExpires;
        change.ADNewValue.bPromptPasswordChange = pNew->userInfo.bPromptPasswordChange;
        change.ADNewValue.bUserCanChangePassword = pNew->userInfo.bUserCanChangePassword;
        change.ADNewValue.bAccountDisabled = pNew->userInfo.bAccountDisabled;
        change.ADNewValue.bAccountExpired = pNew->userInfo.bAccountExpired;
        change.ADNewValue.bAccountLocked = pNew->userInfo.bAccountLocked;

        change.ADNewValue.pw_uid = pNew->userInfo.uid;
        change.ADNewValue.pw_gid = pNew->userInfo.gid;
        change.ADNewValue.pw_name = pNew->userInfo.pszUnixName;
        change.ADNewValue.pw_passwd = pNew->userInfo.pszPasswd 
                                        ? pNew->userInfo.pszPasswd
                                        : "x";
        change.ADNewValue.pw_gecos = pNew->userInfo.pszGecos
                                        ? pNew->userInfo.pszGecos
                                        : "";
        change.ADNewValue.pw_shell = pNew->userInfo.pszShell;
        change.ADNewValue.pw_dir = pNew->userInfo.pszHomedir;

        change.ADNewValue.pDisplayName = pNew->userInfo.pszDisplayName;
        change.ADNewValue.pszWindowsHomeFolder = pNew->userInfo.pszWindowsHomeFolder;
        change.ADNewValue.pszLocalWindowsHomeFolder = pNew->userInfo.pszLocalWindowsHomeFolder; 
        change.ADNewValue.LastUpdated = Now;
    }

    dwError = LwMbsToWc16s( "Application", &record.pLogname);
    BAIL_ON_UMN_ERROR(dwError);

    if (!PreviousRun)
    {
        dwError = LwMbsToWc16s( "Success Audit", &record.pEventType);
    }
    else
    {
        dwError = LwMbsToWc16s( "Information", &record.pEventType);
    }
    BAIL_ON_UMN_ERROR(dwError);

    record.EventDateTime = Now;

    dwError = LwMbsToWc16s( "User Monitor", &record.pEventSource);
    BAIL_ON_UMN_ERROR(dwError);

    if (pOld != NULL && pNew != NULL)
    {
        pOperation = "changed";
    }
    else if (pOld != NULL && pNew == NULL)
    {
        pOperation = "deleted";
    }
    else if (pOld == NULL && pNew != NULL)
    {
        pOperation = "added";
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_UMN_ERROR(dwError);
    }

    dwError = LwAllocateWc16sPrintfW(
                    &record.pEventCategory,
                    L"AD User %hhs " AD_USER_CHANGE_VERSION ,
                    pOperation);
    BAIL_ON_UMN_ERROR(dwError);

    if (pNew != NULL)
    {
        record.EventSourceId = pNew->userInfo.uid;

        dwError = LwMbsToWc16s(
                        pNew->userInfo.pszUnixName,
                        &record.pUser);
        BAIL_ON_UMN_ERROR(dwError);
    }
    else
    {
        record.EventSourceId = pOld->pw_uid;

        dwError = LwMbsToWc16s(
                        pOld->pw_name,
                        &record.pUser);
        BAIL_ON_UMN_ERROR(dwError);
    }

    // Do not free. This value is borrowed from other structures.
    record.pComputer = (PWSTR)UmnEvtGetEventComputerName();

    dwError = UmnSrvGetADUserEventDescription(&record.pDescription,
            pOperation,
            oldTimeBuf,
            pOld,
            newTimeBuf,
            pNew);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = EncodeADUserChange(
                    &change,
                    &record.DataLen,
                    (PVOID*)&record.pData);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LwEvtWriteRecords(
                    pEventlog,
                    1,
                    &record);
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(record.pLogname);
    LW_SAFE_FREE_MEMORY(record.pEventType);
    LW_SAFE_FREE_MEMORY(record.pEventSource);
    LW_SAFE_FREE_MEMORY(record.pEventCategory);
    LW_SAFE_FREE_MEMORY(record.pUser);
    LW_SAFE_FREE_MEMORY(record.pDescription);
    LW_SAFE_FREE_MEMORY(record.pData);
    return dwError;

error:
    goto cleanup;
}


static
DWORD
UmnSrvWriteADUserValues(
    HANDLE hReg,
    HKEY hUser,
    PLSA_SECURITY_OBJECT pUser
    )
{

    /* This doesn't write the following attributes
     * enabled
     * bIsLocal
     * type
     * qwPwdLastSet
     * qwMaxPwdAge
     * qwPwdExpires
     * qwAccountExpires
     * bIsGeneratedUPN
     * bIsAccountInfoKnown
     * dwLmHashLen
     * pLmHash
     * dwNtHashLen
     * pNtHash
     */

    DWORD dwError = 0;
    DWORD dword = 0;
    PCSTR pString = NULL;

    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_name",
                    0,
                    REG_SZ,
                    (PBYTE) pUser->userInfo.pszUnixName,
                    strlen(pUser->userInfo.pszUnixName) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    pString = pUser->userInfo.pszPasswd;
    if (!pString)
    {
        pString = "x";
    }
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_passwd",
                    0,
                    REG_SZ,
                    (PBYTE) pString,
                    strlen(pString) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    dword = pUser->userInfo.uid;
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_uid",
                    0,
                    REG_DWORD,
                    (PBYTE)&dword,
                    sizeof(dword));
    BAIL_ON_UMN_ERROR(dwError);

    dword = pUser->userInfo.gid;
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_gid",
                    0,
                    REG_DWORD,
                    (PBYTE)&dword,
                    sizeof(dword));
    BAIL_ON_UMN_ERROR(dwError);

    pString = pUser->userInfo.pszGecos;
    if (!pString)
    {
        pString = "";
    }
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_gecos",
                    0,
                    REG_SZ,
                    (PBYTE) pString,
                    strlen(pString) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_dir",
                    0,
                    REG_SZ,
                    (PBYTE) pUser->userInfo.pszHomedir,
                    strlen(pUser->userInfo.pszHomedir) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_shell",
                    0,
                    REG_SZ,
                    (PBYTE) pUser->userInfo.pszShell,
                    strlen(pUser->userInfo.pszShell) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    pString = pUser->userInfo.pszDisplayName;
    if (!pString)
    {
        pString = "";
    }
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pDisplayName",
                    0,
                    REG_SZ,
                    (PBYTE) pString,
                    strlen(pString) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    pString = pUser->pszObjectSid;
    if (!pString)
    {
        pString = "";
    }
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_objectsid",
                    0,
                    REG_SZ,
                    (PBYTE) pString,
                    strlen(pString) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    pString = pUser->pszDN;
    if (!pString)
    {
        pString = "";
    }
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_dn",
                    0,
                    REG_SZ,
                    (PBYTE) pString,
                    strlen(pString) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    pString = pUser->pszNetbiosDomainName;
    if (!pString)
    {
        pString = "";
    }
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_netbiosdomainname",
                    0,
                    REG_SZ,
                    (PBYTE) pString,
                    strlen(pString) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    pString = pUser->pszSamAccountName;
    if (!pString)
    {
        pString = "";
    }
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_samaccountname",
                    0,
                    REG_SZ,
                    (PBYTE) pString,
                    strlen(pString) + 1);
    BAIL_ON_UMN_ERROR(dwError);


    pString = pUser->userInfo.pszPrimaryGroupSid;
    if (!pString)
    {
        pString = "";
    }
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_primarygroupsid",
                    0,
                    REG_SZ,
                    (PBYTE) pString,
                    strlen(pString) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    pString = pUser->userInfo.pszUPN;
    if (!pString)
    {
        pString = "";
    }
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_upn",
                    0,
                    REG_SZ,
                    (PBYTE) pString,
                    strlen(pString) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    pString = pUser->userInfo.pszAliasName;
    if (!pString)
    {
        pString = "";
    }
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_aliasname",
                    0,
                    REG_SZ,
                    (PBYTE) pString,
                    strlen(pString) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    dword = pUser->userInfo.bPasswordExpired;
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_passwordexpired",
                    0,
                    REG_DWORD,
                    (PBYTE)&dword,
                    sizeof(dword));
    BAIL_ON_UMN_ERROR(dwError);

    dword = pUser->userInfo.bPasswordNeverExpires;
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_passwordneverexpires",
                    0,
                    REG_DWORD,
                    (PBYTE)&dword,
                    sizeof(dword));
    BAIL_ON_UMN_ERROR(dwError);

    dword = pUser->userInfo.bPromptPasswordChange;
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_promptpasswordchange",
                    0,
                    REG_DWORD,
                    (PBYTE)&dword,
                    sizeof(dword));
    BAIL_ON_UMN_ERROR(dwError);

    dword = pUser->userInfo.bUserCanChangePassword;
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_usercanchangepassword",
                    0,
                    REG_DWORD,
                    (PBYTE)&dword,
                    sizeof(dword));
    BAIL_ON_UMN_ERROR(dwError);

    dword = pUser->userInfo.bAccountDisabled;
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_accountdisabled",
                    0,
                    REG_DWORD,
                    (PBYTE)&dword,
                    sizeof(dword));
    BAIL_ON_UMN_ERROR(dwError);

    dword = pUser->userInfo.bAccountExpired;
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_accountexpired",
                    0,
                    REG_DWORD,
                    (PBYTE)&dword,
                    sizeof(dword));
    BAIL_ON_UMN_ERROR(dwError);

    dword = pUser->userInfo.bAccountLocked;
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_accountLocked",
                    0,
                    REG_DWORD,
                    (PBYTE)&dword,
                    sizeof(dword));
    BAIL_ON_UMN_ERROR(dwError);

    pString = pUser->userInfo.pszWindowsHomeFolder;
    if (!pString)
    {
        pString = "";
    }
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_windowshomefolder",
                    0,
                    REG_SZ,
                    (PBYTE) pString,
                    strlen(pString) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    pString = pUser->userInfo.pszLocalWindowsHomeFolder;
    if (!pString)
    {
        pString = "";
    }
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "ad_localwindowshomefolder",
                    0,
                    REG_SZ,
                    (PBYTE) pString,
                    strlen(pString) + 1);
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


static
BOOLEAN
UmnSrvStringsEqual(
    PCSTR pStr1,
    PCSTR pStr2
    )
{
    if (pStr1 == NULL)
    {
        pStr1 = "";
    }
    if (pStr2 == NULL)
    {
        pStr2 = "";
    }
    return !strcmp(pStr1, pStr2);
}


static
BOOLEAN
UmnSrvADUserChanged(
        const PAD_USER_INFO pOld,
        const PLSA_SECURITY_OBJECT pUser
        )
{
    return ( pUser->userInfo.bPasswordExpired != pOld->bPasswordExpired
             || pUser->userInfo.bAccountDisabled != pOld->bAccountDisabled
             || pUser->userInfo.bAccountExpired != pOld->bAccountExpired
             || pUser->userInfo.bAccountLocked != pOld->bAccountLocked
             || strcmp((pUser->userInfo.pszPasswd ?
                        pUser->userInfo.pszPasswd : "x"),
                    pOld->pw_passwd)
             || pUser->userInfo.uid != pOld->pw_uid
             || pUser->userInfo.gid != pOld->pw_gid
             || !UmnSrvStringsEqual(pUser->userInfo.pszGecos, pOld->pw_gecos)
             || !UmnSrvStringsEqual(pUser->userInfo.pszHomedir, pOld->pw_dir)
             || !UmnSrvStringsEqual(pUser->userInfo.pszShell, pOld->pw_shell)
             || !UmnSrvStringsEqual(pUser->userInfo.pszDisplayName, pOld->pDisplayName)
             || !UmnSrvStringsEqual(pUser->pszDN, pOld->pszDN)
             || !UmnSrvStringsEqual(pUser->pszObjectSid, pOld->pszObjectSid)
             || !UmnSrvStringsEqual(pUser->pszNetbiosDomainName, pOld->pszNetbiosDomainName)
             || !UmnSrvStringsEqual(pUser->pszSamAccountName, pOld->pszSamAccountName)
             || !UmnSrvStringsEqual(pUser->userInfo.pszPrimaryGroupSid, pOld->pszPrimaryGroupSid)
             || !UmnSrvStringsEqual(pUser->userInfo.pszUPN, pOld->pszUPN)
             || !UmnSrvStringsEqual(pUser->userInfo.pszAliasName, pOld->pszAliasName)
             || pUser->userInfo.bPasswordNeverExpires != pOld->bPasswordNeverExpires
             || pUser->userInfo.bPromptPasswordChange != pOld->bPromptPasswordChange
             || pUser->userInfo.bUserCanChangePassword != pOld->bUserCanChangePassword
             || !UmnSrvStringsEqual(pUser->userInfo.pszUnixName, pOld->pw_name)
             || !UmnSrvStringsEqual(pUser->userInfo.pszWindowsHomeFolder, pOld->pszWindowsHomeFolder)
             || !UmnSrvStringsEqual(pUser->userInfo.pszLocalWindowsHomeFolder, pOld->pszLocalWindowsHomeFolder));
}


static
DWORD
UmnSrvUpdateADUser(
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hUsers,
    long long PreviousRun,
    long long Now,
    PLSA_SECURITY_OBJECT pUser
    )
{
    DWORD dwError = 0;
    HKEY hKey = NULL;
    AD_USER_INFO old = { 0 };
    DWORD dwNow = Now;
    PSTR pEncodedUser = NULL;

    dwError = LwURLEncodeString(
                    pUser->userInfo.pszUnixName,
                    &pEncodedUser);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = RegOpenKeyExA(
                    hReg,
                    hUsers,
                    pEncodedUser,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        UMN_LOG_INFO("Adding user '%s' (uid %d)",
                        pUser->userInfo.pszUnixName, pUser->userInfo.uid);

        dwError = RegCreateKeyExA(
                        hReg,
                        hUsers,
                        pEncodedUser,
                        0,
                        NULL,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        NULL);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvWriteADUserValues(
                        hReg,
                        hKey,
                        pUser);
        if (dwError == ERROR_NO_UNICODE_TRANSLATION)
        {
            UMN_LOG_ERROR("Ignoring user with URL encoding %s because one of their fields has no UCS-2 representation", pEncodedUser);

            // Delete the key so it does not show up with blank values next
            // time.
            dwError = RegCloseKey(
                    hReg,
                    hKey);
            BAIL_ON_UMN_ERROR(dwError);
            hKey = NULL;

            dwError = RegDeleteKeyA(
                            hReg,
                            hUsers,
                            pEncodedUser);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = ERROR_NO_UNICODE_TRANSLATION;
            BAIL_ON_UMN_ERROR(dwError);
        }
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvWriteADUserEvent(
                        pEventlog,
                        PreviousRun,
                        NULL,
                        Now,
                        pUser);
        BAIL_ON_UMN_ERROR(dwError);
    }
    else
    {
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvReadADUser(
                        "AD Users",
                        pEncodedUser,
                        &old);
        BAIL_ON_UMN_ERROR(dwError);

        if (UmnSrvADUserChanged(&old, pUser))
        {
            UMN_LOG_INFO("User '%s' (uid %d) changed",
                            pUser->userInfo.pszUnixName, pUser->userInfo.uid);
            dwError = UmnSrvWriteADUserValues(
                            hReg,
                            hKey,
                            pUser);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = UmnSrvWriteADUserEvent(
                            pEventlog,
                            PreviousRun,
                            &old,
                            Now,
                            pUser);
            BAIL_ON_UMN_ERROR(dwError);
        }
    }

    dwError = RegSetValueExA(
                    hReg,
                    hKey,
                    "LastUpdated",
                    0,
                    REG_DWORD,
                    (PBYTE)&dwNow,
                    sizeof(dwNow));
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pEncodedUser);
    UmnSrvFreeADUserContents(&old);
    if (hKey)
    {
        RegCloseKey(
                hReg,
                hKey);
    }
    return dwError;

error:
    goto cleanup;
}


DWORD
UmnSrvFindDeletedADUsers(
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    PCSTR pUserKeyName,
    HKEY hUsers,
    long long Now
    )
{
    DWORD dwError = 0;
    DWORD subKeyCount = 0;
    DWORD maxSubKeyLen = 0;
    DWORD subKeyLen = 0;
    DWORD i = 0;
    PSTR pKeyName = NULL;
    DWORD lastUpdated = 0;
    DWORD lastUpdatedLen = 0;
    AD_USER_INFO old = { 0 };

    UMN_LOG_DEBUG("Finding deleted AD users");

    dwError = RegQueryInfoKeyA(
                    hReg,
                    hUsers,
                    NULL,
                    NULL,
                    NULL,
                    &subKeyCount,
                    &maxSubKeyLen,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LwAllocateMemory(
                    maxSubKeyLen + 1,
                    (PVOID *)&pKeyName);

    for (i = 0; i < subKeyCount; i++)
    {
        if (gbPollerThreadShouldExit)
        {
            dwError = ERROR_CANCELLED;
            BAIL_ON_UMN_ERROR(dwError);
        }
        subKeyLen = maxSubKeyLen;

        dwError = RegEnumKeyExA(
                        hReg,
                        hUsers,
                        i,
                        pKeyName,
                        &subKeyLen,
                        NULL,
                        NULL,
                        NULL,
                        NULL);
        BAIL_ON_UMN_ERROR(dwError);

        pKeyName[subKeyLen] = 0;

        lastUpdatedLen = sizeof(lastUpdated);
        dwError = RegGetValueA(
                        hReg,
                        hUsers,
                        pKeyName,
                        "LastUpdated",
                        0,
                        NULL,
                        (PBYTE)&lastUpdated,
                        &lastUpdatedLen);
        if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
        {
            UMN_LOG_WARNING("User %s not completely written. The user monitor service may have previously terminated ungracefully.",
                        LW_SAFE_LOG_STRING(pKeyName));
            lastUpdated = 0;
            dwError = 0;
        }
        else
        {
            BAIL_ON_UMN_ERROR(dwError);
        }

        if (lastUpdated < Now)
        {
            UmnSrvFreeADUserContents(&old);
            dwError = UmnSrvReadADUser(
                            pUserKeyName,
                            pKeyName,
                            &old);
            BAIL_ON_UMN_ERROR(dwError);

            UMN_LOG_INFO("User '%s' deleted",
                            old.pw_name);

            dwError = RegDeleteKeyA(
                            hReg,
                            hUsers,
                            pKeyName);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = UmnSrvWriteADUserEvent(
                            pEventlog,
                            old.LastUpdated,
                            &old,
                            Now,
                            NULL);
            BAIL_ON_UMN_ERROR(dwError);

            // Make sure we don't skip the next key since this one was deleted
            i--;
            subKeyCount--;
        }
    }

cleanup:
    UmnSrvFreeADUserContents(&old);

    LW_SAFE_FREE_STRING(pKeyName);
    return dwError;

error:
    goto cleanup;
}


static
DWORD
UmnSrvUpdateADAccountsByHash(
    HANDLE hLsass,
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hParameters,
    PLW_HASH_TABLE pUsers,
    long long PreviousRun,
    long long Now
    )
{
    DWORD dwError = 0;
    HKEY hUsers = NULL;
    HKEY hGroups = NULL;
    LW_HASH_ITERATOR usersIterator = { 0 };
    LW_HASH_ENTRY* pEntry = NULL;
    DWORD groupSidCount = 0;
    PSTR* ppGroupSids = NULL;
    DWORD lookupGroupSidCount = 0;
    DWORD lookupGroupSidCapacity = 0;
    // Only free the first level of this array, do not free the strings it
    // points to.
    PSTR* ppLookupGroupSids = NULL;
    LSA_QUERY_LIST list = { 0 };
    PLSA_SECURITY_OBJECT *ppLookedupGroups = NULL;
    PLW_HASH_TABLE pGroups = NULL;
    PLW_HASH_TABLE pNameToUser = NULL;
    PLW_HASH_TABLE pNameToGroup = NULL;
    DWORD i = 0;
    // Do not free
    PLSA_SECURITY_OBJECT pGroup = NULL;
    // Do not free
    PLSA_SECURITY_OBJECT pExisting = NULL;
    PSTR pNewName = NULL;

    dwError = LwHashCreate(
                    100,
                    LwHashStringCompare,
                    LwHashStringHash,
                    UmnSrvHashFreeObjectValue,
                    NULL,
                    &pGroups);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LwHashCreate(
                    100,
                    LwHashStringCompare,
                    LwHashStringHash,
                    NULL,
                    NULL,
                    &pNameToGroup);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LwHashCreate(
                    pUsers->sCount * 2,
                    LwHashStringCompare,
                    LwHashStringHash,
                    NULL,
                    NULL,
                    &pNameToUser);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = RegOpenKeyExA(
                hReg,
                hParameters,
                "AD Users",
                0,
                KEY_ALL_ACCESS,
                &hUsers);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = RegOpenKeyExA(
                hReg,
                hParameters,
                "AD Groups",
                0,
                KEY_ALL_ACCESS,
                &hGroups);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LwHashGetIterator(
                    pUsers,
                    &usersIterator);
    BAIL_ON_UMN_ERROR(dwError);

    while((pEntry = LwHashNext(&usersIterator)) != NULL)
    {
        PLSA_SECURITY_OBJECT pUser = (PLSA_SECURITY_OBJECT)pEntry->pValue;

        if (gbPollerThreadShouldExit)
        {
            dwError = ERROR_CANCELLED;
            BAIL_ON_UMN_ERROR(dwError);
        }

        dwError = LwHashGetValue(
                        pNameToUser,
                        pUser->userInfo.pszUnixName,
                        (PVOID*)&pExisting);
        if (dwError != ERROR_NOT_FOUND)
        {
            BAIL_ON_UMN_ERROR(dwError);

            dwError = LwAllocateStringPrintf(
                            &pNewName,
                            "%s\\%s",
                            pUser->pszNetbiosDomainName,
                            pUser->pszSamAccountName);
            BAIL_ON_UMN_ERROR(dwError);

            UMN_LOG_ERROR("Found conflict on user name '%hhs'. Sid %hhs will now be reported as name '%s' instead because its alias conflicts with sid %hhs.",
                                    pUser->userInfo.pszUnixName,
                                    pUser->pszObjectSid,
                                    pNewName,
                                    pExisting->pszObjectSid);
            BAIL_ON_UMN_ERROR(dwError);

            LW_SAFE_FREE_STRING(pUser->userInfo.pszUnixName);
            pUser->userInfo.pszUnixName = pNewName;
            pNewName = NULL;
        }

        dwError = LwHashSetValue(
                        pNameToUser,
                        pUser->userInfo.pszUnixName,
                        pUser);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvUpdateADUser(
                        pEventlog,
                        hReg,
                        hUsers,
                        PreviousRun,
                        Now,
                        pUser);
        if (dwError == ERROR_NO_UNICODE_TRANSLATION)
        {
            // Error message already logged
            dwError = 0;
            continue;
        }
        BAIL_ON_UMN_ERROR(dwError);

        if (ppGroupSids)
        {
            LsaFreeSidList(
                    groupSidCount,
                    ppGroupSids);
        }
        dwError = LsaQueryMemberOf(
                        hLsass,
                        NULL,
                        0,
                        1,
                        &pUser->pszObjectSid,
                        &groupSidCount,
                        &ppGroupSids);
        BAIL_ON_UMN_ERROR(dwError);

        if (groupSidCount > lookupGroupSidCapacity)
        {
            LW_SAFE_FREE_MEMORY(ppLookupGroupSids);

            dwError = LwAllocateMemory(
                            groupSidCount * sizeof(ppLookupGroupSids[0]),
                            (PVOID*)&ppLookupGroupSids);
            BAIL_ON_UMN_ERROR(dwError);

            lookupGroupSidCapacity = groupSidCount;
        }

        lookupGroupSidCount = 0;

        for (i = 0; i < groupSidCount; i++)
        {
            dwError = LwHashGetValue(
                            pGroups,
                            ppGroupSids[i],
                            (PVOID*)&pGroup);
            if (dwError == ERROR_NOT_FOUND)
            {
                ppLookupGroupSids[lookupGroupSidCount++] = ppGroupSids[i];
            }
            else
            {
                BAIL_ON_UMN_ERROR(dwError);

                UMN_LOG_VERBOSE("Found AD user %s is a member of processed group %s",
                        pUser->userInfo.pszUnixName,
                        pGroup->groupInfo.pszUnixName);

                if (!pGroup->enabled)
                {
                    UMN_LOG_VERBOSE("Skipping unenabled group %s",
                        pGroup->groupInfo.pszUnixName);
                }
                else
                {
                    dwError = UmnSrvUpdateADGroupMember(
                                    pEventlog,
                                    hReg,
                                    hGroups,
                                    PreviousRun,
                                    Now,
                                    pGroup,
                                    pUser->userInfo.pszUnixName);
                    BAIL_ON_UMN_ERROR(dwError);
                }
            }
        }
        
        if (lookupGroupSidCount)
        {
            list.ppszStrings = (PCSTR *)ppLookupGroupSids;

            dwError = LsaFindObjects(
                            hLsass,
                            NULL,
                            0,
                            LSA_OBJECT_TYPE_GROUP,
                            LSA_QUERY_TYPE_BY_SID,
                            lookupGroupSidCount,
                            list,
                            &ppLookedupGroups);
            BAIL_ON_UMN_ERROR(dwError);

            for (i = 0; i < lookupGroupSidCount; i++)
            {
                if (gbPollerThreadShouldExit)
                {
                    dwError = ERROR_CANCELLED;
                    BAIL_ON_UMN_ERROR(dwError);
                }
                pGroup = ppLookedupGroups[i];

                if (!pGroup)
                {
                    UMN_LOG_ERROR("Unable to find group sid %s that user %s is a member of",
                            ppLookupGroupSids[i],
                            pUser->userInfo.pszUnixName);
                    continue;
                }

                UMN_LOG_VERBOSE("Found AD user %s is a member of unprocessed group %s",
                        pUser->userInfo.pszUnixName,
                        pGroup->groupInfo.pszUnixName);

                dwError = LwHashGetValue(
                                pNameToGroup,
                                pGroup->groupInfo.pszUnixName,
                                (PVOID*)&pExisting);
                if (dwError != ERROR_NOT_FOUND)
                {
                    BAIL_ON_UMN_ERROR(dwError);

                    dwError = LwAllocateStringPrintf(
                                    &pNewName,
                                    "%s\\%s",
                                    pGroup->pszNetbiosDomainName,
                                    pGroup->pszSamAccountName);
                    BAIL_ON_UMN_ERROR(dwError);

                    UMN_LOG_ERROR("Found conflict on group name '%hhs'. Sid %hhs will now be reported as name '%s' instead because its alias conflicts with sid %hhs.",
                                            pGroup->groupInfo.pszUnixName,
                                            pGroup->pszObjectSid,
                                            pNewName,
                                            pExisting->pszObjectSid);
                    BAIL_ON_UMN_ERROR(dwError);

                    LW_SAFE_FREE_STRING(pGroup->groupInfo.pszUnixName);
                    pGroup->groupInfo.pszUnixName = pNewName;
                    pNewName = NULL;
                }

                dwError = LwHashSetValue(
                                pNameToGroup,
                                pGroup->groupInfo.pszUnixName,
                                pGroup);
                BAIL_ON_UMN_ERROR(dwError);

                if (!pGroup->enabled)
                {
                    UMN_LOG_VERBOSE("Skipping unenabled group %s",
                        pGroup->groupInfo.pszUnixName);
                }
                else
                {
                    dwError = UmnSrvUpdateADGroup(
                                    pEventlog,
                                    hReg,
                                    hGroups,
                                    PreviousRun,
                                    Now,
                                    pGroup);
                    BAIL_ON_UMN_ERROR(dwError);

                    dwError = UmnSrvUpdateADGroupMember(
                                    pEventlog,
                                    hReg,
                                    hGroups,
                                    PreviousRun,
                                    Now,
                                    pGroup,
                                    pUser->userInfo.pszUnixName);
                    BAIL_ON_UMN_ERROR(dwError);
                }
                dwError = LwHashSetValue(
                                pGroups,
                                pGroup->pszObjectSid,
                                pGroup);
                BAIL_ON_UMN_ERROR(dwError);

                ppLookedupGroups[i] = NULL;
            }

            LsaFreeSecurityObjectList(
                lookupGroupSidCount,
                ppLookedupGroups);
            ppLookedupGroups = NULL;
        }
    }

    dwError = UmnSrvFindDeletedADUsers(
                    pEventlog,
                    hReg,
                    "AD Users",
                    hUsers,
                    Now);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = UmnSrvFindDeletedGroups(
                    pEventlog,
                    hReg,
                    "AD Groups",
                    hGroups,
                    Now);
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pNewName);
    LW_SAFE_FREE_MEMORY(ppLookupGroupSids);
    if (ppGroupSids)
    {
        LsaFreeSidList(
                groupSidCount,
                ppGroupSids);
    }
    if (ppLookedupGroups)
    {
        LsaFreeSecurityObjectList(
            lookupGroupSidCount,
            ppLookedupGroups);
    }
    if (hUsers)
    {
        RegCloseKey(hReg, hUsers);
    }
    if (hGroups)
    {
        RegCloseKey(hReg, hGroups);
    }
    LwHashSafeFree(&pGroups);
    LwHashSafeFree(&pNameToUser);
    LwHashSafeFree(&pNameToGroup);
    return dwError;

error:
    goto cleanup;
}


DWORD
UmnSrvUpdateADAccounts(
    HANDLE hLsass,
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hParameters,
    long long PreviousRun,
    long long Now
    )
{
    DWORD dwError = 0;
    DWORD k = 0;
    PSTR  pRequireMemberList = NULL;
    PSTR  pHostAccessGroup = NULL;
    PSTR  pMember = NULL;
    PLW_HASH_TABLE pUsers = NULL;
    DWORD dwRequireMemberListCount = 0;
    DWORD dwHostAccessGroupCount = 0;
    DWORD dwMemberListCount = 0;
    PSTR* ppHostAccessGroupArray = NULL;
    PSTR* ppRequireMemberListArray = NULL;
    PSTR* ppMemberListArray = NULL;


    LWREG_CONFIG_ITEM ADConfigDescription[] =
    {
        {
            "RequireMembershipOf",
            TRUE,
            LwRegTypeMultiString,
            0,
            MAXDWORD,
            NULL,
            &pRequireMemberList,
            NULL
        },
        {
            "HostAccessGroup",
            TRUE,
            LwRegTypeMultiString,
            0,
            MAXDWORD,
            NULL,
            &pHostAccessGroup,
            NULL
        }
    };
    PLSASTATUS pLsaStatus = NULL;
    // Do not free
    PSTR pDomain = NULL;
    // Do not free
    PSTR pCell = NULL;
    PLSA_SECURITY_OBJECT pAllUsers = NULL;
    DWORD i = 0;

    UMN_LOG_DEBUG("Updating AD users");

    dwError = LwHashCreate(
                    100,
                    LwHashStringCompare,
                    LwHashStringHash,
                    UmnSrvHashFreeObjectValue,
                    NULL,
                    &pUsers);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = RegProcessConfig(
                AD_PROVIDER_REGKEY,
                AD_PROVIDER_POLICY_REGKEY,
                ADConfigDescription,
                sizeof(ADConfigDescription)/sizeof(ADConfigDescription[0]));
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LwConvertMultiStringToStringArray(pRequireMemberList,
                                                &ppRequireMemberListArray,
                                                &dwRequireMemberListCount);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LwConvertMultiStringToStringArray(pHostAccessGroup,
                                                &ppHostAccessGroupArray,
                                                &dwHostAccessGroupCount);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LwMergeStringArray(
                     ppRequireMemberListArray, dwRequireMemberListCount,
                     ppHostAccessGroupArray,   dwHostAccessGroupCount,
                     &ppMemberListArray,       &dwMemberListCount);
    BAIL_ON_UMN_ERROR(dwError);

    if (dwMemberListCount > 0)
    {
       for(k = 0; k < dwMemberListCount; k++)
       {
          UMN_LOG_DEBUG("RequireMembershipOf is set; will report on AD users belonging to RequireMembershipOf entries");

          dwError = LwStrDupOrNull(ppMemberListArray[k], &pMember);
          BAIL_ON_UMN_ERROR(dwError);

          LwStripWhitespace( pMember, TRUE, TRUE);

          UMN_LOG_DEBUG("Adding users belonging to RequireMembershipOf entry %s", pMember);
          dwError = UmnSrvAddUsersFromMembership(
                               hLsass,
                               pUsers,
                               pMember);
          BAIL_ON_UMN_ERROR(dwError);
          LW_SAFE_FREE_STRING(pMember);
       }
    }
    else
    {

        UMN_LOG_DEBUG("RequireMembershipOf is NOT set; will report on AD users based on joined cell/domain");
        dwError = LsaGetStatus2(
                        hLsass,
                        NULL,
                        &pLsaStatus);
        BAIL_ON_UMN_ERROR(dwError);

        for (i = 0; i < pLsaStatus->dwCount; i++)
        {
            if (pLsaStatus->pAuthProviderStatusList[i].pszDomain)
            {
                pDomain = pLsaStatus->pAuthProviderStatusList[i].pszDomain;
            }
            if (pLsaStatus->pAuthProviderStatusList[i].pszCell)
            {
                pCell = pLsaStatus->pAuthProviderStatusList[i].pszCell;
            }
        }

        if (pDomain || pCell)
        {
            UMN_LOG_DEBUG("Reporting all users %s %s can login",
                    (pCell) ? "in cell" : "accessible from domain",
                    (pCell) ? pCell : pDomain);

            dwError = LwAllocateMemory(
                            sizeof(*pAllUsers),
                            (PVOID*)&pAllUsers);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = LwAllocateString(
                            "S-INVALID",
                            &pAllUsers->pszObjectSid);
            BAIL_ON_UMN_ERROR(dwError);

            pAllUsers->enabled = TRUE;
            pAllUsers->bIsLocal = FALSE;

            dwError = LwAllocateString(
                            "AllDomains",
                            &pAllUsers->pszNetbiosDomainName);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = LwAllocateString(
                            "AllUsers",
                            &pAllUsers->pszSamAccountName);
            BAIL_ON_UMN_ERROR(dwError);

            pAllUsers->type = LSA_OBJECT_TYPE_USER;

            dwError = LwAllocateString(
                            "S-INVALID",
                            &pAllUsers->userInfo.pszPrimaryGroupSid);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = LwAllocateString(
                            "All Users",
                            &pAllUsers->userInfo.pszUnixName);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = LwAllocateString(
                            "All Users",
                            &pAllUsers->userInfo.pszGecos);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = LwAllocateString(
                            "",
                            &pAllUsers->userInfo.pszShell);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = LwAllocateString(
                            "",
                            &pAllUsers->userInfo.pszHomedir);
            BAIL_ON_UMN_ERROR(dwError);

            if (pCell)
            {
                dwError = LwAllocateStringPrintf(
                                &pAllUsers->userInfo.pszDisplayName,
                                "All Users in cell %s",
                                pCell);
                BAIL_ON_UMN_ERROR(dwError);
            }
            else
            {
                dwError = LwAllocateStringPrintf(
                                &pAllUsers->userInfo.pszDisplayName,
                                "All Users accessible from domain %s",
                                pDomain);
                BAIL_ON_UMN_ERROR(dwError);
            }

            dwError = LwHashSetValue(
                            pUsers,
                            pAllUsers->pszObjectSid,
                            pAllUsers);
            BAIL_ON_UMN_ERROR(dwError);

            pAllUsers = NULL;
        }
    }

    dwError = UmnSrvUpdateADAccountsByHash(
                    hLsass,
                    pEventlog,
                    hReg,
                    hParameters,
                    pUsers,
                    PreviousRun,
                    Now);
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    if (pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
    }
    LW_SAFE_FREE_STRING(pMember);
    LwHashSafeFree(&pUsers);
    if (pAllUsers)
    {
        LsaFreeSecurityObject(pAllUsers);
    }

    LW_SAFE_FREE_STRING(pRequireMemberList);
    LW_SAFE_FREE_STRING(pHostAccessGroup);
    LwFreeStringArray(ppRequireMemberListArray, dwRequireMemberListCount);
    LwFreeStringArray(ppHostAccessGroupArray, dwHostAccessGroupCount);
    LwFreeStringArray(ppMemberListArray, dwMemberListCount);

    return dwError;

error:
    goto cleanup;
}
