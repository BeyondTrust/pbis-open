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
 *        groups.c
 *
 * Abstract:
 *
 *        User monitor service for local users and groups
 *
 *        Functions for enumerating and tracking groups.
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#include "includes.h"

static
VOID
UmnSrvFreeGroupContents(
    PUSER_MONITOR_GROUP pGroup
    )
{
    LW_SAFE_FREE_MEMORY(pGroup->gr_name);
    LW_SAFE_FREE_MEMORY(pGroup->gr_passwd);
}

static
DWORD
UmnSrvWriteGroupEvent(
    PLW_EVENTLOG_CONNECTION pEventlog,
    BOOLEAN FirstRun,
    PUSER_MONITOR_GROUP pOld,
    long long Now,
    struct group *pNew
    )
{
    DWORD dwError = 0;
    // Do not free. The field values are borrowed from other structures.
    GROUP_CHANGE change = { { 0 } };
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
        change.NewValue.gr_name = pNew->gr_name;
        change.NewValue.gr_passwd = pNew->gr_passwd;
        change.NewValue.gr_gid = pNew->gr_gid;
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
                    L"Group %hhs",
                    pOperation);
    BAIL_ON_UMN_ERROR(dwError);

    if (pNew != NULL)
    {
        record.EventSourceId = pNew->gr_gid;

        dwError = LwMbsToWc16s(
                        pNew->gr_name,
                        &record.pUser);
        BAIL_ON_UMN_ERROR(dwError);
    }
    else
    {
        record.EventSourceId = pOld->gr_gid;

        dwError = LwMbsToWc16s(
                        pOld->gr_name,
                        &record.pUser);
        BAIL_ON_UMN_ERROR(dwError);
    }

    // Leave computer NULL so it is filled in by the eventlog

    dwError = LwAllocateWc16sPrintfW(
                    &record.pDescription,
                    L"Between %hhs and %hhs, group '%hhs' was %hhs.\n"
                    L"Passwd (from group struct)\n"
                    L"\tOld: %hhs\n"
                    L"\tNew: %hhs\n"
                    L"Gid\n"
                    L"\tOld: %d\n"
                    L"\tNew: %d",
                    oldTimeBuf,
                    newTimeBuf,
                    pOld ? pOld->gr_name : pNew->gr_name,
                    pOperation,
                    pOld ? pOld->gr_passwd : "",
                    pNew ? pNew->gr_passwd : "",
                    pOld ? pOld->gr_gid : -1,
                    pNew ? pNew->gr_gid : -1);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = EncodeGroupChange(
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
UmnSrvWriteGroupMemberEvent(
    PLW_EVENTLOG_CONNECTION pEventlog,
    long long Now,
    BOOLEAN FirstRun,
    BOOLEAN AddMember,
    BOOLEAN OnlyGidChange,
    PCSTR pUserName,
    DWORD Gid,
    PCSTR pGroupName
    )
{
    DWORD dwError = 0;
    // Do not free. The field values are borrowed from other structures.
    GROUP_MEMBERSHIP_CHANGE change = { 0 };
    LW_EVENTLOG_RECORD record = { 0 };
    PCSTR pOperation = NULL;

    change.Added = AddMember;
    change.OnlyGidChange = OnlyGidChange;
    change.pUserName = (PSTR)pUserName;
    change.Gid = Gid;
    change.pGroupName = (PSTR)pGroupName;

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

    dwError = LwAllocateWc16sPrintfW(
                    &record.pEventCategory,
                    AddMember ? L"Group membership added" : L"Group membership deleted",
                    pOperation);
    BAIL_ON_UMN_ERROR(dwError);

    // Leave computer NULL so it is filled in by the eventlog

    dwError = LwAllocateWc16sPrintfW(
                    &record.pDescription,
                    L"User %s was %s group %s (gid %d)",
                    pUserName,
                    AddMember ? "added to" : "deleted from",
                    pGroupName,
                    Gid);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = EncodeGroupMembershipChange(
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
UmnSrvWriteGroupValues(
    HANDLE hReg,
    HKEY hGroup,
    struct group *pGroup
    )
{
    DWORD dwError = 0;
    DWORD dword = 0;

    dwError = RegSetValueExA(
                    hReg,
                    hGroup,
                    "gr_name",
                    0,
                    REG_SZ,
                    (PBYTE)pGroup->gr_name,
                    strlen(pGroup->gr_name) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = RegSetValueExA(
                    hReg,
                    hGroup,
                    "gr_passwd",
                    0,
                    REG_SZ,
                    (PBYTE)pGroup->gr_passwd,
                    strlen(pGroup->gr_passwd) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    dword = pGroup->gr_gid;
    dwError = RegSetValueExA(
                    hReg,
                    hGroup,
                    "gr_gid",
                    0,
                    REG_DWORD,
                    (PBYTE)&dword,
                    sizeof(dword));
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
UmnSrvReadGroup(
    PSTR pName,
    PUSER_MONITOR_GROUP pResult
    )
{
    DWORD dwError = 0;
    PSTR pGroupPath = NULL;
    LWREG_CONFIG_ITEM groupLayout[] =
    {
        {
            "gr_name",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pResult->gr_name,
            NULL
        },
        {
            "gr_passwd",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pResult->gr_passwd,
            NULL
        },
        {
            "gr_gid",
            FALSE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &pResult->gr_gid,
            NULL
        },
        {
            "LastUpdated",
            FALSE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &pResult->LastUpdated,
            NULL
        },
    };

    UMN_LOG_VERBOSE("Reading previous values for group '%s'",
                    pName);

    dwError = LwAllocateStringPrintf(
                    &pGroupPath,
                    "Services\\" SERVICE_NAME "\\Parameters\\Groups\\%s",
                    pName);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LwRegProcessConfig(
                pGroupPath,
                NULL,
                groupLayout,
                sizeof(groupLayout)/sizeof(groupLayout[0]));
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pGroupPath);
    return dwError;
    
error:
    goto cleanup;
}

static
DWORD
UmnSrvUpdateGroupMember(
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hMembers,
    BOOLEAN FirstRun,
    long long Now,
    DWORD OldGid,
    struct group *pGroup,
    PCSTR pMember
    )
{
    DWORD dwError = 0;
    HKEY hKey = NULL;
    DWORD dwNow = Now;

    dwError = RegOpenKeyExA(
                    hReg,
                    hMembers,
                    pMember,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        UMN_LOG_INFO("Adding user member '%s' to group '%s' (gid %d)",
                        pMember, pGroup->gr_name, pGroup->gr_gid);

        dwError = RegCreateKeyExA(
                        hReg,
                        hMembers,
                        pMember,
                        0,
                        NULL,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        NULL);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvWriteGroupMemberEvent(
                        pEventlog,
                        Now,
                        FirstRun,
                        TRUE, //Add member
                        FALSE, //Not gid change
                        pMember,
                        pGroup->gr_gid,
                        pGroup->gr_name);
        BAIL_ON_UMN_ERROR(dwError);
    }
    else if (OldGid != pGroup->gr_gid)
    {
        dwError = UmnSrvWriteGroupMemberEvent(
                        pEventlog,
                        Now,
                        FirstRun,
                        FALSE, //Remove member
                        TRUE, //Gid change
                        pMember,
                        OldGid,
                        pGroup->gr_name);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvWriteGroupMemberEvent(
                        pEventlog,
                        Now,
                        FirstRun,
                        TRUE, //Add member
                        TRUE, //Gid change
                        pMember,
                        pGroup->gr_gid,
                        pGroup->gr_name);
        BAIL_ON_UMN_ERROR(dwError);
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
UmnSrvFindDeletedGroupMembers(
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hMembers,
    long long Now,
    DWORD Gid,
    PCSTR pGroupName
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

    dwError = RegQueryInfoKeyA(
                    hReg,
                    hMembers,
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
        subKeyLen = maxSubKeyLen;

        dwError = RegEnumKeyExA(
                        hReg,
                        hMembers,
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
                        hMembers,
                        pKeyName,
                        "LastUpdated",
                        0,
                        NULL,
                        (PBYTE)&lastUpdated,
                        &lastUpdatedLen);
        BAIL_ON_UMN_ERROR(dwError);

        if (lastUpdated < Now)
        {
            dwError = RegDeleteKeyA(
                            hReg,
                            hMembers,
                            pKeyName);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = UmnSrvWriteGroupMemberEvent(
                            pEventlog,
                            Now,
                            FALSE,
                            FALSE, //Remove member
                            FALSE, //Not Gid change
                            pKeyName,
                            Gid,
                            pGroupName);
            BAIL_ON_UMN_ERROR(dwError);

            // Make sure we don't skip the next key since this one was deleted
            i--;
            subKeyCount--;
        }
    }

cleanup:

    LW_SAFE_FREE_STRING(pKeyName);
    return dwError;

error:
    goto cleanup;
}


static
DWORD
UmnSrvUpdateGroupMembers(
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hGroup,
    BOOLEAN FirstRun,
    long long Now,
    DWORD OldGid,
    struct group *pGroup
    )
{
    DWORD dwError = 0;
    DWORD iMember = 0;
    HKEY hMembers = NULL;

    dwError = RegOpenKeyExA(
                hReg,
                hGroup,
                "Members",
                0,
                KEY_ALL_ACCESS,
                &hMembers);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = RegCreateKeyExA(
                        hReg,
                        hGroup,
                        "Members",
                        0,
                        NULL,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hMembers,
                        NULL);
        BAIL_ON_UMN_ERROR(dwError);
    }
    else
    {
        BAIL_ON_UMN_ERROR(dwError);
    }

    for (iMember = 0; pGroup->gr_mem[iMember]; iMember++)
    {
        dwError = UmnSrvUpdateGroupMember(
                        pEventlog,
                        hReg,
                        hMembers,
                        FirstRun,
                        Now,
                        OldGid,
                        pGroup,
                        pGroup->gr_mem[iMember]);
        BAIL_ON_UMN_ERROR(dwError);
    }

    dwError = UmnSrvFindDeletedGroupMembers(
                    pEventlog,
                    hReg,
                    hMembers,
                    Now,
                    pGroup->gr_gid,
                    pGroup->gr_name);
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    if (hMembers)
    {
        RegCloseKey(hReg, hMembers);
    }
    return dwError;
    
error:
    goto cleanup;
}

static
DWORD
UmnSrvUpdateGroup(
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hGroups,
    BOOLEAN FirstRun,
    long long Now,
    struct group *pGroup
    )
{
    DWORD dwError = 0;
    HKEY hKey = NULL;
    USER_MONITOR_GROUP old = { 0 };
    DWORD dwNow = Now;
    old.gr_gid = -1;

    dwError = RegOpenKeyExA(
                    hReg,
                    hGroups,
                    pGroup->gr_name,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        UMN_LOG_INFO("Adding group '%s' (gid %d)",
                        pGroup->gr_name, pGroup->gr_gid);

        dwError = RegCreateKeyExA(
                        hReg,
                        hGroups,
                        pGroup->gr_name,
                        0,
                        NULL,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        NULL);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvWriteGroupValues(
                        hReg,
                        hKey,
                        pGroup);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvWriteGroupEvent(
                        pEventlog,
                        FirstRun,
                        NULL,
                        Now,
                        pGroup);
        BAIL_ON_UMN_ERROR(dwError);
    }
    else
    {
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvReadGroup(
                        pGroup->gr_name,
                        &old);
        BAIL_ON_UMN_ERROR(dwError);

        if (strcmp(pGroup->gr_name, old.gr_name) ||
                strcmp(pGroup->gr_passwd, old.gr_passwd) ||
                pGroup->gr_gid != old.gr_gid)
        {
            UMN_LOG_INFO("Group '%s' (gid %d) changed",
                            pGroup->gr_name, pGroup->gr_gid);
            dwError = UmnSrvWriteGroupValues(
                            hReg,
                            hKey,
                            pGroup);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = UmnSrvWriteGroupEvent(
                            pEventlog,
                            FirstRun,
                            &old,
                            Now,
                            pGroup);
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

    dwError = UmnSrvUpdateGroupMembers(
                    pEventlog,
                    hReg,
                    hKey,
                    FirstRun,
                    Now,
                    old.gr_gid,
                    pGroup);
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    UmnSrvFreeGroupContents(&old);
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
UmnSrvFindDeletedGroups(
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hGroups,
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
    USER_MONITOR_GROUP old = { 0 };
    HKEY hMembers = NULL;
    PSTR pMembersName = NULL;

    dwError = RegQueryInfoKeyA(
                    hReg,
                    hGroups,
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
        subKeyLen = maxSubKeyLen;

        dwError = RegEnumKeyExA(
                        hReg,
                        hGroups,
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
                        hGroups,
                        pKeyName,
                        "LastUpdated",
                        0,
                        NULL,
                        (PBYTE)&lastUpdated,
                        &lastUpdatedLen);
        BAIL_ON_UMN_ERROR(dwError);

        if (lastUpdated < Now)
        {
            UMN_LOG_INFO("Group '%s' deleted",
                            pKeyName);

            UmnSrvFreeGroupContents(&old);
            if (hMembers)
            {
                RegCloseKey(
                        hReg,
                        hMembers);
            }
            LW_SAFE_FREE_STRING(pMembersName);

            dwError = UmnSrvReadGroup(
                            pKeyName,
                            &old);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = LwAllocateStringPrintf(
                            &pMembersName,
                            "%s\\Members",
                            pKeyName);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = RegOpenKeyExA(
                            hReg,
                            hGroups,
                            pMembersName,
                            0,
                            KEY_ALL_ACCESS,
                            &hMembers);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = UmnSrvFindDeletedGroupMembers(
                            pEventlog,
                            hReg,
                            hMembers,
                            Now,
                            old.gr_gid,
                            pKeyName);
            BAIL_ON_UMN_ERROR(dwError);

            // RegDeleteKeyA is not recursive, so the Members key must be
            // deleted before the group key
            dwError = RegDeleteKeyA(
                            hReg,
                            hGroups,
                            pMembersName);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = RegDeleteKeyA(
                            hReg,
                            hGroups,
                            pKeyName);
            BAIL_ON_UMN_ERROR(dwError);

            // Groups cannot be detected as deleted if there is no previous
            // data to compare, so pass FALSE for FirstRun
            dwError = UmnSrvWriteGroupEvent(
                            pEventlog,
                            FALSE,
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
    if (hMembers)
    {
        RegCloseKey(
                hReg,
                hMembers);
    }
    LW_SAFE_FREE_STRING(pMembersName);
    UmnSrvFreeGroupContents(&old);

    LW_SAFE_FREE_STRING(pKeyName);
    return dwError;

error:
    goto cleanup;
}

DWORD
UmnSrvUpdateGroups(
    HANDLE hLsass,
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hParameters,
    BOOLEAN FirstRun,
    long long Now
    )
{
    DWORD gid = 0;
    DWORD dwError = 0;
    struct group *pGroup = NULL;
    LSA_QUERY_LIST list = { 0 };
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    HKEY hGroups = NULL;

    list.pdwIds = &gid;

    dwError = RegOpenKeyExA(
                hReg,
                hParameters,
                "Groups",
                0,
                KEY_ALL_ACCESS,
                &hGroups);
    BAIL_ON_UMN_ERROR(dwError);

    while((pGroup = getgrent()) != NULL)
    {
        gid = pGroup->gr_gid;

        dwError = LsaFindObjects(
                    hLsass,
                    NULL,
                    0,
                    LSA_OBJECT_TYPE_GROUP,
                    LSA_QUERY_TYPE_BY_UNIX_ID,
                    1,
                    list,
                    &ppObjects);
        BAIL_ON_UMN_ERROR(dwError);

        if (ppObjects[0] &&
                ppObjects[0]->enabled &&
                !strcmp(ppObjects[0]->groupInfo.pszUnixName, pGroup->gr_name))
        {
            UMN_LOG_VERBOSE("Skipping enumerated group '%s' (gid %d) because they came from lsass",
                    pGroup->gr_name, gid);
        }
        else
        {
            dwError = UmnSrvUpdateGroup(
                            pEventlog,
                            hReg,
                            hGroups,
                            FirstRun,
                            Now,
                            pGroup);
            BAIL_ON_UMN_ERROR(dwError);
        }

        LsaFreeSecurityObjectList(1, ppObjects);
        ppObjects = NULL;
    }

    dwError = UmnSrvFindDeletedGroups(
                    pEventlog,
                    hReg,
                    hGroups,
                    Now);
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    if (ppObjects)
    {
        LsaFreeSecurityObjectList(1, ppObjects);
    }
    if (hGroups)
    {
        RegCloseKey(hReg, hGroups);
    }
    return dwError;
    
error:
    goto cleanup;
}
