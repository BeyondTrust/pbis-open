/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2011
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
 *        privilegedb.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Privileges database
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
DWORD
LsaSrvCreatePrivilegesDb(
    IN HANDLE hRegistry,
    IN HKEY hPrivilegesKey,
    OUT PLW_HASH_TABLE *ppPrivileges
    );

static
VOID
LsaSrvPrivilegeDbEntryFree(
    const LW_HASH_ENTRY *pEntry
    );

static
DWORD
LsaSrvReadPrivilegeEntry(
    IN HANDLE hRegistry,
    IN HKEY hEntry,
    IN PSTR pszPrivilegeName,
    OUT PLSA_PRIVILEGE *ppPrivilegeEntry
    );

static
VOID
LsaSrvFreePrivilegeEntry(
    PLSA_PRIVILEGE  pEntry
    );

static
DWORD
LsaSrvCreateAccountsDb(
    IN HANDLE hRegistry,
    IN HKEY hAccountsKey,
    OUT PLW_HASH_TABLE *ppAccounts
    );


DWORD
LsaSrvInitPrivileges(
    VOID
    )
{
    DWORD err = ERROR_SUCCESS;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    HANDLE hRegistry = NULL;
    HKEY hPrivileges = NULL;
    HKEY hAccounts = NULL;

    err = RegOpenServer(&hRegistry);
    BAIL_ON_LSA_ERROR(err);

    err = RegOpenKeyExA(
                 hRegistry,
                 NULL,
                 LSA_PRIVILEGES_REG_KEY,
                 0,
                 KEY_READ,
                 &hPrivileges);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvCreatePrivilegesDb(
                 hRegistry,
                 hPrivileges,
                 &pGlobals->pPrivileges);
    BAIL_ON_LSA_ERROR(err);

    err = RegOpenKeyExA(
                 hRegistry,
                 NULL,
                 LSA_ACCOUNTS_REG_KEY,
                 0,
                 KEY_READ,
                 &hAccounts);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvCreateAccountsDb(
                 hRegistry,
                 hAccounts,
                 &pGlobals->pAccounts);
    BAIL_ON_LSA_ERROR(err);

error:
    if (hRegistry)
    {
        RegCloseServer(hRegistry);
    }

    if (hPrivileges)
    {
        RegCloseKey(hRegistry, hPrivileges);
    }

    if (hAccounts)
    {
        RegCloseKey(hRegistry, hAccounts);
    }

    return err;
}


static
DWORD
LsaSrvCreatePrivilegesDb(
    IN HANDLE hRegistry,
    IN HKEY hPrivilegesKey,
    OUT PLW_HASH_TABLE *ppPrivileges
    )
{
    DWORD err = ERROR_SUCCESS;
    PLW_HASH_TABLE pHash = NULL;
    DWORD numKeys = 0;
    DWORD maxKeyLen = 0;
    DWORD i = 0;
    PSTR pszPrivilegeName = NULL;
    HKEY hPrivilege = NULL;
    PSTR pszKey = NULL;
    PLSA_PRIVILEGE pPrivilegeEntry = NULL;

    //
    // Create the database hash table
    //

    err = LwHashCreate(
                 LSA_PRIVILEGES_DB_SIZE,
                 LwHashCaselessStringCompare,
                 LwHashCaselessStringHash,
                 LsaSrvPrivilegeDbEntryFree,
                 NULL,
                 &pHash);
    BAIL_ON_LSA_ERROR(err);

    err = RegQueryInfoKeyA(
                 hRegistry,
                 hPrivilegesKey,
                 NULL,
                 NULL,
                 NULL,
                 &numKeys,
                 &maxKeyLen,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL);
    BAIL_ON_LSA_ERROR(err);

    err = LwAllocateMemory(
                 maxKeyLen,
                 OUT_PPVOID(&pszPrivilegeName));
    BAIL_ON_LSA_ERROR(err);

    //
    // Enumerate through privileges keys. Each key represents a single
    // privilege with the key name being privilege name.
    //

    for (i = 0; i < numKeys; i++)
    {
        DWORD privilegeNameLen = maxKeyLen;

        err = RegEnumKeyExA(
                     hRegistry,
                     hPrivilegesKey,
                     i,
                     pszPrivilegeName,
                     &privilegeNameLen,
                     NULL,
                     NULL,
                     NULL,
                     NULL);
        BAIL_ON_LSA_ERROR(err);

        err = RegOpenKeyExA(
                     hRegistry,
                     hPrivilegesKey,
                     pszPrivilegeName,
                     0,
                     KEY_READ,
                     &hPrivilege);
        BAIL_ON_LSA_ERROR(err);

        //
        // Read privilege info to the database entry
        //

        err = LsaSrvReadPrivilegeEntry(
                     hRegistry,
                     hPrivilege,
                     pszPrivilegeName,
                     &pPrivilegeEntry);
        BAIL_ON_LSA_ERROR(err);

        err = LwAllocateString(
                     pszPrivilegeName,
                     &pszKey);
        BAIL_ON_LSA_ERROR(err);

        err = LwHashSetValue(
                     pHash,
                     pszKey,
                     pPrivilegeEntry);
        BAIL_ON_LSA_ERROR(err);

        pszKey          = NULL;
        pPrivilegeEntry = NULL;

        //
        // Close the privilege subkey and cleanup the
        // name and the handle
        //
        err = RegCloseKey(
                     hRegistry,
                     hPrivilege);
        BAIL_ON_LSA_ERROR(err);

        memset(pszPrivilegeName, 0, maxKeyLen);
        hPrivilege = NULL;
    }

    *ppPrivileges = pHash;

error:
    if (err)
    {
        if (pPrivilegeEntry)
        {
            LsaSrvFreePrivilegeEntry(pPrivilegeEntry);
        }

        LW_SAFE_FREE_MEMORY(pszKey);

        if (pHash)
        {
            LwHashSafeFree(&pHash);
        }

        *ppPrivileges = NULL;
    }

    LW_SAFE_FREE_MEMORY(pszPrivilegeName);

    if (hPrivilege)
    {
        RegCloseKey(
                 hRegistry,
                 hPrivilege);
    }

    return err;
}


static
VOID
LsaSrvPrivilegeDbEntryFree(
    const LW_HASH_ENTRY *pEntry
    )
{
    LwFreeMemory(pEntry->pKey);
    LsaSrvFreePrivilegeEntry(pEntry->pValue);
}


static
VOID
LsaSrvFreePrivilegeEntry(
    PLSA_PRIVILEGE pEntry
    )
{
    LW_SAFE_FREE_MEMORY(pEntry->pszName);
    LW_SAFE_FREE_MEMORY(pEntry->pwszDescription);
    LW_SAFE_FREE_MEMORY(pEntry);
}


static
DWORD
LsaSrvReadPrivilegeEntry(
    IN HANDLE hRegistry,
    IN HKEY hEntry,
    IN PSTR pszPrivilegeName,
    OUT PLSA_PRIVILEGE *ppPrivilegeEntry
    )
{
    DWORD err = ERROR_SUCCESS;
    LUID privilegeValue = {0};
    DWORD enabledByDefault = 0;
    PSTR pszDescription = NULL;
    DWORD valueSize = 0;
    DWORD valueType = REG_NONE;
    PLSA_PRIVILEGE pEntry = NULL;

    //
    // Privilege value (LUID)
    //
    valueSize = sizeof(privilegeValue.LowPart);
    err = RegQueryValueEx(
                 hRegistry,
                 hEntry,
                 "ValueLow",
                 0,
                 &valueType,
                 (PBYTE)&privilegeValue.LowPart,
                 &valueSize);
    BAIL_ON_LSA_ERROR(err);

    if (valueType != REG_DWORD)
    {
        err = ERROR_INVALID_DATA;
        BAIL_ON_LSA_ERROR(err);
    }

    valueSize = sizeof(privilegeValue.HighPart);
    err = RegQueryValueEx(
                 hRegistry,
                 hEntry,
                 "ValueHigh",
                 0,
                 &valueType,
                 (PBYTE)&privilegeValue.HighPart,
                 &valueSize);
    BAIL_ON_LSA_ERROR(err);

    if (valueType != REG_DWORD)
    {
        err = ERROR_INVALID_DATA;
        BAIL_ON_LSA_ERROR(err);
    }

    //
    // "Enabled By Default" flag
    //
    valueSize = sizeof(enabledByDefault);
    err = RegQueryValueEx(
                 hRegistry,
                 hEntry,
                 "EnabledByDefault",
                 0,
                 &valueType,
                 (PBYTE)&enabledByDefault,
                 &valueSize);
    BAIL_ON_LSA_ERROR(err);

    if (valueType != REG_DWORD)
    {
        err = ERROR_INVALID_DATA;
        BAIL_ON_LSA_ERROR(err);
    }

    //
    // Privilege description (display name) string
    //
    valueSize = 0;
    err = RegQueryValueEx(
                 hRegistry,
                 hEntry,
                 "Description",
                 0,
                 &valueType,
                 NULL,
                 &valueSize);
    BAIL_ON_LSA_ERROR(err);

    if (valueType != REG_SZ)
    {
        err = ERROR_INVALID_DATA;
        BAIL_ON_LSA_ERROR(err);
    }

    err = LwAllocateMemory(
                 valueSize,
                 OUT_PPVOID(&pszDescription));
    BAIL_ON_LSA_ERROR(err);

    err = RegQueryValueEx(
                 hRegistry,
                 hEntry,
                 "Description",
                 0,
                 &valueType,
                 (PBYTE)pszDescription,
                 &valueSize);
    BAIL_ON_LSA_ERROR(err);

    //
    // Allocate the actual privilege db entry
    //
    err = LwAllocateMemory(
                 sizeof(*pEntry),
                 OUT_PPVOID(&pEntry));
    BAIL_ON_LSA_ERROR(err);

    pEntry->Luid              = privilegeValue;
    pEntry->EnabledByDefault  = enabledByDefault;

    err = LwAllocateString(pszPrivilegeName,
                           &pEntry->pszName);
    BAIL_ON_LSA_ERROR(err);

    err = LwMbsToWc16s(pszDescription,
                       &pEntry->pwszDescription);
    BAIL_ON_LSA_ERROR(err);

    *ppPrivilegeEntry = pEntry;

error:
    if (err)
    {
        LsaSrvFreePrivilegeEntry(pEntry);

        *ppPrivilegeEntry = NULL;
    }

    return err;
}


static
DWORD
LsaSrvCreateAccountsDb(
    IN HANDLE hRegistry,
    IN HKEY hAccountsKey,
    OUT PLW_HASH_TABLE *ppAccounts
    )
{
    DWORD err = ERROR_SUCCESS;

    return err;
}
