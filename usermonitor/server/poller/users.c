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
 *        users.c
 *
 * Abstract:
 *
 *        User monitor service for local users and groups
 *
 *        Functions for enumerating and tracking users.
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#include "includes.h"

VOID
UmnSrvFreeUserContents(
    PUSER_MONITOR_PASSWD pUser
    )
{
    LW_SAFE_FREE_MEMORY(pUser->pw_name);
    LW_SAFE_FREE_MEMORY(pUser->pw_passwd);
    LW_SAFE_FREE_MEMORY(pUser->pw_gecos);
    LW_SAFE_FREE_MEMORY(pUser->pw_dir);
    LW_SAFE_FREE_MEMORY(pUser->pw_shell);
}

static
DWORD
UmnSrvWriteUserEvent(
    PLW_EVENTLOG_CONNECTION pEventlog,
    BOOLEAN FirstRun,
    PUSER_MONITOR_PASSWD pOld,
    long long Now,
    struct passwd *pNew
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
        change.NewValue.pw_name = pNew->pw_name;
        change.NewValue.pw_passwd = pNew->pw_passwd;
        change.NewValue.pw_uid = pNew->pw_uid;
        change.NewValue.pw_gid = pNew->pw_gid;
        change.NewValue.pw_gecos = pNew->pw_gecos;
        change.NewValue.pw_dir = pNew->pw_dir;
        change.NewValue.pw_shell = pNew->pw_shell;
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
                    L"User %hhs",
                    pOperation);
    BAIL_ON_UMN_ERROR(dwError);

    if (pNew != NULL)
    {
        record.EventSourceId = pNew->pw_uid;

        dwError = LwMbsToWc16s(
                        pNew->pw_name,
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
                    pOld ? pOld->pw_name : pNew->pw_name,
                    pOperation,
                    pOld ? pOld->pw_passwd : "",
                    pNew ? pNew->pw_passwd : "",
                    pOld ? pOld->pw_uid : -1,
                    pNew ? pNew->pw_uid : -1,
                    pOld ? pOld->pw_gid : -1,
                    pNew ? pNew->pw_gid : -1,
                    pOld ? pOld->pw_gecos : "",
                    pNew ? pNew->pw_gecos : "",
                    pOld ? pOld->pw_dir : "",
                    pNew ? pNew->pw_dir : "",
                    pOld ? pOld->pw_shell : "",
                    pNew ? pNew->pw_shell : "");
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
UmnSrvWriteUserValues(
    HANDLE hReg,
    HKEY hUser,
    struct passwd *pUser
    )
{
    DWORD dwError = 0;
    DWORD dword = 0;

    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_name",
                    0,
                    REG_SZ,
                    (PBYTE) pUser->pw_name,
                    strlen(pUser->pw_name) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_passwd",
                    0,
                    REG_SZ,
                    (PBYTE) pUser->pw_passwd,
                    strlen(pUser->pw_passwd) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    dword = pUser->pw_uid;
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_uid",
                    0,
                    REG_DWORD,
                    (PBYTE)&dword,
                    sizeof(dword));
    BAIL_ON_UMN_ERROR(dwError);

    dword = pUser->pw_gid;
    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_gid",
                    0,
                    REG_DWORD,
                    (PBYTE)&dword,
                    sizeof(dword));
    BAIL_ON_UMN_ERROR(dwError);

    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_gecos",
                    0,
                    REG_SZ,
                    (PBYTE) pUser->pw_gecos,
                    strlen(pUser->pw_gecos) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_dir",
                    0,
                    REG_SZ,
                    (PBYTE) pUser->pw_dir,
                    strlen(pUser->pw_dir) + 1);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = RegSetValueExA(
                    hReg,
                    hUser,
                    "pw_shell",
                    0,
                    REG_SZ,
                    (PBYTE) pUser->pw_shell,
                    strlen(pUser->pw_shell) + 1);
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
UmnSrvReadUser(
    PCSTR pParentKey,
    PCSTR pName,
    PUSER_MONITOR_PASSWD pResult
    )
{
    DWORD dwError = 0;
    PSTR pUserPath = NULL;
    LWREG_CONFIG_ITEM userLayout[] =
    {
        {
            "pw_name",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pResult->pw_name,
            NULL
        },
        {
            "pw_passwd",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pResult->pw_passwd,
            NULL
        },
        {
            "pw_uid",
            FALSE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &pResult->pw_uid,
            NULL
        },
        {
            "pw_gid",
            FALSE,
            LwRegTypeDword,
            0,
            -1,
            NULL,
            &pResult->pw_gid,
            NULL
        },
        {
            "pw_gecos",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pResult->pw_gecos,
            NULL
        },
        {
            "pw_dir",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pResult->pw_dir,
            NULL
        },
        {
            "pw_shell",
            FALSE,
            LwRegTypeString,
            0,
            -1,
            NULL,
            &pResult->pw_shell,
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

    UMN_LOG_VERBOSE("Reading previous values for user '%s'",
                    pName);

    dwError = LwAllocateStringPrintf(
                    &pUserPath,
                    "Services\\" SERVICE_NAME "\\Parameters\\%s\\%s",
                    pParentKey,
                    pName);
    BAIL_ON_UMN_ERROR(dwError);

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
UmnSrvUpdateUser(
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hUsers,
    BOOLEAN FirstRun,
    long long Now,
    struct passwd *pUser
    )
{
    DWORD dwError = 0;
    HKEY hKey = NULL;
    USER_MONITOR_PASSWD old = { 0 };
    DWORD dwNow = Now;

    dwError = RegOpenKeyExA(
                    hReg,
                    hUsers,
                    pUser->pw_name,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        UMN_LOG_INFO("Adding user '%s' (uid %d)",
                        pUser->pw_name, pUser->pw_uid);

        dwError = RegCreateKeyExA(
                        hReg,
                        hUsers,
                        pUser->pw_name,
                        0,
                        NULL,
                        0,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        NULL);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvWriteUserValues(
                        hReg,
                        hKey,
                        pUser);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnSrvWriteUserEvent(
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
                        "Users",
                        pUser->pw_name,
                        &old);
        BAIL_ON_UMN_ERROR(dwError);

        if (strcmp(pUser->pw_name, old.pw_name) ||
                strcmp(pUser->pw_passwd, old.pw_passwd) ||
                pUser->pw_uid != old.pw_uid ||
                pUser->pw_gid != old.pw_gid ||
                strcmp(pUser->pw_gecos, old.pw_gecos) ||
                strcmp(pUser->pw_dir, old.pw_dir) ||
                strcmp(pUser->pw_shell, old.pw_shell))
        {
            UMN_LOG_INFO("User '%s' (uid %d) changed",
                            pUser->pw_name, pUser->pw_uid);
            dwError = UmnSrvWriteUserValues(
                            hReg,
                            hKey,
                            pUser);
            BAIL_ON_UMN_ERROR(dwError);

            dwError = UmnSrvWriteUserEvent(
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

DWORD
UmnSrvFindDeletedUsers(
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
    USER_MONITOR_PASSWD old = { 0 };

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
        BAIL_ON_UMN_ERROR(dwError);

        if (lastUpdated < Now)
        {
            UmnSrvFreeUserContents(&old);
            dwError = UmnSrvReadUser(
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

            // Users cannot be detected as deleted if there is no previous data
            // to compare, so pass FALSE for FirstRun
            if (!strcmp(pUserKeyName, "Users"))
            {
                dwError = UmnSrvWriteUserEvent(
                                pEventlog,
                                FALSE,
                                &old,
                                Now,
                                NULL);
                BAIL_ON_UMN_ERROR(dwError);
            }
            else
            {
                dwError = UmnSrvWriteADUserEvent(
                                pEventlog,
                                FALSE,
                                &old,
                                Now,
                                NULL);
                BAIL_ON_UMN_ERROR(dwError);
            }

            // Make sure we don't skip the next key since this one was deleted
            i--;
            subKeyCount--;
        }
    }

cleanup:
    UmnSrvFreeUserContents(&old);

    LW_SAFE_FREE_STRING(pKeyName);
    return dwError;

error:
    goto cleanup;
}

DWORD
UmnSrvUpdateUsers(
    HANDLE hLsass,
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hParameters,
    BOOLEAN FirstRun,
    long long Now
    )
{
    DWORD uid = 0;
    DWORD dwError = 0;
    struct passwd *pUser = NULL;
    LSA_QUERY_LIST list = { 0 };
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    HKEY hUsers = NULL;

    list.pdwIds = &uid;

    dwError = RegOpenKeyExA(
                hReg,
                hParameters,
                "Users",
                0,
                KEY_ALL_ACCESS,
                &hUsers);
    BAIL_ON_UMN_ERROR(dwError);

    while((pUser = getpwent()) != NULL)
    {
        uid = pUser->pw_uid;

        dwError = LsaFindObjects(
                    hLsass,
                    NULL,
                    0,
                    LSA_OBJECT_TYPE_USER,
                    LSA_QUERY_TYPE_BY_UNIX_ID,
                    1,
                    list,
                    &ppObjects);
        BAIL_ON_UMN_ERROR(dwError);

        if (ppObjects[0] &&
                ppObjects[0]->enabled &&
                !strcmp(ppObjects[0]->userInfo.pszUnixName, pUser->pw_name))
        {
            UMN_LOG_VERBOSE("Skipping enumerated user '%s' (uid %d) because they came from lsass",
                    pUser->pw_name, uid);
        }
        else
        {
            dwError = UmnSrvUpdateUser(
                            pEventlog,
                            hReg,
                            hUsers,
                            FirstRun,
                            Now,
                            pUser);
            BAIL_ON_UMN_ERROR(dwError);
        }

        LsaFreeSecurityObjectList(1, ppObjects);
        ppObjects = NULL;
    }

    dwError = UmnSrvFindDeletedUsers(
                    pEventlog,
                    hReg,
                    "Users",
                    hUsers,
                    Now);
    BAIL_ON_UMN_ERROR(dwError);

cleanup:
    if (ppObjects)
    {
        LsaFreeSecurityObjectList(1, ppObjects);
    }
    if (hUsers)
    {
        RegCloseKey(hReg, hUsers);
    }
    return dwError;
    
error:
    goto cleanup;
}
