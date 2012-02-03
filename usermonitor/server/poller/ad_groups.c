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
 *        ad_groups.c
 *
 * Abstract:
 *
 *        User monitor service for users and groups
 *
 *        Functions for enumerating and tracking ad groups.
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#include "includes.h"

DWORD
UmnSrvWriteADGroupEvent(
    PLW_EVENTLOG_CONNECTION pEventlog,
    BOOLEAN FirstRun,
    PUSER_MONITOR_GROUP pOld,
    long long Now,
    PLSA_SECURITY_OBJECT pNew
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
        change.NewValue.gr_name = pNew->groupInfo.pszUnixName;
        change.NewValue.gr_passwd = pNew->groupInfo.pszPasswd ?
                                        pNew->groupInfo.pszPasswd : "x";
        change.NewValue.gr_gid = pNew->groupInfo.gid;
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
                    L"AD Group %hhs",
                    pOperation);
    BAIL_ON_UMN_ERROR(dwError);

    if (pNew != NULL)
    {
        record.EventSourceId = pNew->groupInfo.gid;

        dwError = LwMbsToWc16s(
                        pNew->groupInfo.pszUnixName,
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
                    pOld ? pOld->gr_name : pNew->groupInfo.pszUnixName,
                    pOperation,
                    pOld ? pOld->gr_passwd : "",
                    pNew ? (pNew->groupInfo.pszPasswd ?
                                pNew->groupInfo.pszPasswd : "x") : "",
                    pOld ? pOld->gr_gid : -1,
                    pNew ? pNew->groupInfo.gid : -1);
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
UmnSrvWriteADGroupValues(
    HANDLE hReg,
    HKEY hGroup,
    PLSA_SECURITY_OBJECT pGroup
    )
{
    DWORD dwError = 0;
    DWORD dword = 0;
    PCSTR pString = NULL;

    dwError = RegSetValueExA(
                    hReg,
                    hGroup,
                    "gr_name",
                    0,
                    REG_SZ,
                    (PBYTE)pGroup->groupInfo.pszUnixName,
                    strlen(pGroup->groupInfo.pszUnixName) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    pString = pGroup->groupInfo.pszPasswd;
    if (!pString)
    {
        pString = "x";
    }
    dwError = RegSetValueExA(
                    hReg,
                    hGroup,
                    "gr_passwd",
                    0,
                    REG_SZ,
                    (PBYTE)pString,
                    strlen(pString) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    dword = pGroup->groupInfo.gid;
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

DWORD
UmnSrvUpdateADGroup(
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hGroups,
    BOOLEAN FirstRun,
    long long Now,
    PLSA_SECURITY_OBJECT pGroup
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
                    pGroup->pszObjectSid,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        UMN_LOG_INFO("Adding group '%s' (gid %d)",
                        pGroup->groupInfo.pszUnixName, pGroup->groupInfo.gid);

        dwError = RegCreateKeyExA(
                        hReg,
                        hGroups,
                        pGroup->pszObjectSid,
                        0,
                        NULL,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        NULL);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvWriteADGroupValues(
                        hReg,
                        hKey,
                        pGroup);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvWriteADGroupEvent(
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
                        "AD Groups",
                        pGroup->pszObjectSid,
                        &old);
        BAIL_ON_UMN_ERROR(dwError);

        if (strcmp(pGroup->groupInfo.pszUnixName, old.gr_name))
        {
            // The group's name changed. This is too drastic of a change for a
            // change event. File a deletion and addition event.
            dwError = UmnSrvWriteADGroupEvent(
                            pEventlog,
                            FirstRun,
                            &old,
                            Now,
                            NULL);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = UmnSrvWriteADGroupEvent(
                            pEventlog,
                            FirstRun,
                            NULL,
                            Now,
                            pGroup);
            BAIL_ON_UMN_ERROR(dwError);
        }
        else if (strcmp((pGroup->groupInfo.pszPasswd ?
                        pGroup->groupInfo.pszPasswd : "x"),
                    old.gr_passwd) ||
                pGroup->groupInfo.gid != old.gr_gid)
        {
            UMN_LOG_INFO("Group '%s' (gid %d) changed",
                            pGroup->groupInfo.pszUnixName,
                            pGroup->groupInfo.gid);

            dwError = UmnSrvWriteADGroupValues(
                            hReg,
                            hKey,
                            pGroup);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = UmnSrvWriteADGroupEvent(
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
