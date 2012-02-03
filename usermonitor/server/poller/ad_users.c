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
    BAIL_ON_UMN_ERROR(dwError);

    if (ppObjects[0] && ppObjects[0]->type == LSA_OBJECT_TYPE_USER)
    {
        if (ppObjects[0]->enabled &&
                !LwHashExists(pUsers, ppObjects[0]->pszObjectSid))
        {
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
        dwError = LsaQueryExpandedGroupMembers(
                        hLsass,
                        NULL,
                        0,
                        LSA_OBJECT_TYPE_USER,
                        ppObjects[0]->pszObjectSid,
                        &memberCount,
                        &ppMembers);
        BAIL_ON_UMN_ERROR(dwError);

        for (i = 0; i < memberCount; i++)
        {
            if (ppMembers[i]->enabled &&
                    !LwHashExists(pUsers, ppMembers[i]->pszObjectSid))
            {
                UMN_LOG_VERBOSE("Found AD user %s that can login",
                        ppMembers[i]->userInfo.pszUnixName);

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
UmnSrvWriteADUserEvent(
    PLW_EVENTLOG_CONNECTION pEventlog,
    BOOLEAN FirstRun,
    PUSER_MONITOR_PASSWD pOld,
    long long Now,
    PLSA_SECURITY_OBJECT pNew
    )
{
    DWORD dwError = 0;
    // Do not free. The field values are borrowed from other structures.
    USER_CHANGE change = { { 0 } };
    LW_EVENTLOG_RECORD record = { 0 };
    char oldTimeBuf[128] = { 0 };
    char newTimeBuf[128] = { 0 };
    struct tm oldTmBuf = { 0 };
    struct tm newTmBuf = { 0 };
    time_t temp = 0;
    PCSTR pOperation = NULL;

    if (pOld)
    {
        temp = pOld->LastUpdated;
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
        change.NewValue.pw_name = pNew->userInfo.pszUnixName;
        change.NewValue.pw_passwd = pNew->userInfo.pszPasswd ?
                                        pNew->userInfo.pszPasswd : "x";
        change.NewValue.pw_uid = pNew->userInfo.uid;
        change.NewValue.pw_gid = pNew->userInfo.gid;
        change.NewValue.pw_gecos = pNew->userInfo.pszGecos ?
                                        pNew->userInfo.pszGecos : "";
        change.NewValue.pw_dir = pNew->userInfo.pszHomedir;
        change.NewValue.pw_shell = pNew->userInfo.pszShell;
        change.NewValue.LastUpdated = Now;
    }

    dwError = LwMbsToWc16s(
                    "Application",
                    &record.pLogname);
    BAIL_ON_UMN_ERROR(dwError);

    if (FirstRun)
    {
        dwError = LwMbsToWc16s(
                        "Success Audit",
                        &record.pEventType);
    }
    else
    {
        dwError = LwMbsToWc16s(
                        "Information",
                        &record.pEventType);
    }
    BAIL_ON_UMN_ERROR(dwError);

    record.EventDateTime = Now;

    dwError = LwMbsToWc16s(
                    "User Monitor",
                    &record.pEventSource);
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
                    L"AD User %hhs",
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

    // Leave computer NULL so it is filled in by the eventlog

    dwError = LwAllocateWc16sPrintfW(
                    &record.pDescription,
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
                    L"\tNew: %hhs",
                    oldTimeBuf,
                    newTimeBuf,
                    pOld ? pOld->pw_name : pNew->userInfo.pszUnixName,
                    pOperation,
                    pOld ? pOld->pw_passwd : "",
                    pNew ? (pNew->userInfo.pszPasswd ?
                                pNew->userInfo.pszPasswd : "x") : "",
                    pOld ? pOld->pw_uid : -1,
                    pNew ? pNew->userInfo.uid : -1,
                    pOld ? pOld->pw_gid : -1,
                    pNew ? pNew->userInfo.gid : -1,
                    pOld ? pOld->pw_gecos : "",
                    (pNew && pNew->userInfo.pszGecos) ?
                        pNew->userInfo.pszGecos : "",
                    pOld ? pOld->pw_dir : "",
                    pNew ? pNew->userInfo.pszHomedir : "",
                    pOld ? pOld->pw_shell : "",
                    pNew ? pNew->userInfo.pszShell : "");
    BAIL_ON_UMN_ERROR(dwError);

    dwError = EncodeUserChange(
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

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
UmnSrvUpdateADUser(
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hUsers,
    BOOLEAN FirstRun,
    long long Now,
    PLSA_SECURITY_OBJECT pUser
    )
{
    DWORD dwError = 0;
    HKEY hKey = NULL;
    USER_MONITOR_PASSWD old = { 0 };
    DWORD dwNow = Now;

    dwError = RegOpenKeyExA(
                    hReg,
                    hUsers,
                    pUser->pszObjectSid,
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
                        pUser->pszObjectSid,
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
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvWriteADUserEvent(
                        pEventlog,
                        FirstRun,
                        NULL,
                        Now,
                        pUser);
        BAIL_ON_UMN_ERROR(dwError);
    }
    else
    {
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvReadUser(
                        "AD Users",
                        pUser->pszObjectSid,
                        &old);
        BAIL_ON_UMN_ERROR(dwError);

        if (strcmp(pUser->userInfo.pszUnixName, old.pw_name))
        {
            // The user's name changed. This is too drastic of a change for a
            // change event. File a deletion and addition event.
            dwError = UmnSrvWriteADUserEvent(
                            pEventlog,
                            FirstRun,
                            &old,
                            Now,
                            NULL);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = UmnSrvWriteADUserEvent(
                            pEventlog,
                            FirstRun,
                            NULL,
                            Now,
                            pUser);
            BAIL_ON_UMN_ERROR(dwError);
        }
        else if (strcmp((pUser->userInfo.pszPasswd ?
                        pUser->userInfo.pszPasswd : "x"),
                    old.pw_passwd) ||
                pUser->userInfo.uid != old.pw_uid ||
                pUser->userInfo.gid != old.pw_gid ||
                strcmp((pUser->userInfo.pszGecos ?
                                pUser->userInfo.pszGecos : ""),
                            old.pw_gecos) ||
                strcmp(pUser->userInfo.pszHomedir, old.pw_dir) ||
                strcmp(pUser->userInfo.pszShell, old.pw_shell))
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
                            FirstRun,
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
    UmnSrvFreeUserContents(&old);
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

static
DWORD
UmnSrvUpdateADAccountsByHash(
    HANDLE hLsass,
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hParameters,
    PLW_HASH_TABLE pUsers,
    BOOLEAN FirstRun,
    long long Now
    )
{
    DWORD dwError = 0;
    HKEY hUsers = NULL;
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
    DWORD i = 0;
    // Do not free
    PLSA_SECURITY_OBJECT pGroup = NULL;

    dwError = LwHashCreate(
                    100,
                    LwHashStringCompare,
                    LwHashStringHash,
                    UmnSrvHashFreeObjectValue,
                    NULL,
                    &pGroups);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = RegOpenKeyExA(
                hReg,
                hParameters,
                "AD Users",
                0,
                KEY_ALL_ACCESS,
                &hUsers);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LwHashGetIterator(
                    pUsers,
                    &usersIterator);
    BAIL_ON_UMN_ERROR(dwError);

    while((pEntry = LwHashNext(&usersIterator)) != NULL)
    {
        PLSA_SECURITY_OBJECT pUser = (PLSA_SECURITY_OBJECT)pEntry->pValue;

        dwError = UmnSrvUpdateADUser(
                        pEventlog,
                        hReg,
                        hUsers,
                        FirstRun,
                        Now,
                        pUser);
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
                pGroup = ppLookedupGroups[i];

                UMN_LOG_VERBOSE("Found AD user %s is a member of unprocessed group %s",
                        pUser->userInfo.pszUnixName,
                        pGroup->groupInfo.pszUnixName);

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

    dwError = UmnSrvFindDeletedUsers(
                    pEventlog,
                    hReg,
                    "AD Users",
                    hUsers,
                    Now);
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
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
    LwHashSafeFree(&pGroups);
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
    BOOLEAN FirstRun,
    long long Now
    )
{
    DWORD dwError = 0;
    PSTR pMemberList = NULL;
    PCSTR pIter = NULL;
    PSTR  pMember = NULL;
    PLW_HASH_TABLE pUsers = NULL;
    LWREG_CONFIG_ITEM ADConfigDescription[] =
    {
        {
            "RequireMembershipOf",
            TRUE,
            LwRegTypeMultiString,
            0,
            MAXDWORD,
            NULL,
            &pMemberList,
            NULL
        },
    };

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

    if (pMemberList)
    {
        pIter = pMemberList;
        while (*pIter != 0)
        {
            dwError = LwStrDupOrNull(
                            pIter,
                            &pMember);
            BAIL_ON_UMN_ERROR(dwError);

            LwStripWhitespace(
                    pMember,
                    TRUE,
                    TRUE);

            dwError = UmnSrvAddUsersFromMembership(
                            hLsass,
                            pUsers,
                            pMember);
            BAIL_ON_UMN_ERROR(dwError);

            pIter += strlen(pIter) + 1;
        }
    }

    dwError = UmnSrvUpdateADAccountsByHash(
                    hLsass,
                    pEventlog,
                    hReg,
                    hParameters,
                    pUsers,
                    FirstRun,
                    Now);
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pMemberList);
    LW_SAFE_FREE_STRING(pMember);
    LwHashSafeFree(&pUsers);
    return dwError;

error:
    goto cleanup;
}
