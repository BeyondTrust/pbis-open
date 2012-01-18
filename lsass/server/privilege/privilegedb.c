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
LsaSrvSetPrivilegeEntry_inlock(
    IN PLW_HASH_TABLE pPrivilegesTable,
    IN PSTR pszPrivilegeName,
    IN PLSA_PRIVILEGE pPrivilegeEntry,
    IN BOOLEAN Overwrite
    );


DWORD
LsaSrvInitPrivileges(
    VOID
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    HANDLE hRegistry = NULL;
    HKEY hPrivileges = NULL;
    HKEY hAccounts = NULL;

    ntStatus = LwMapSecurityCreateContext(&pGlobals->pSecurityContext);
    if (ntStatus)
    {
        err = LwNtStatusToWin32Error(ntStatus);
        BAIL_ON_LSA_ERROR(err);
    }

    err = LsaSrvInitPrivilegesSecurity(
                 &pGlobals->pPrivilegesSecDesc);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvInitAccountsSecurity(
                 &pGlobals->pAccountsSecDescRelative,
                 &pGlobals->accountsSecDescRelativeSize);
    BAIL_ON_LSA_ERROR(err);

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

    pthread_rwlock_init(&pGlobals->privilegesRwLock, NULL);

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

    pthread_rwlock_init(&pGlobals->accountsRwLock, NULL);

    err = LsaSrvCreateAccountsDb(
                 hRegistry,
                 hAccounts,
                 &pGlobals->pAccounts);
    BAIL_ON_LSA_ERROR(err);

error:
    if (err || ntStatus)
    {
        LsaSrvFreePrivileges();
    }

    if (hPrivileges)
    {
        RegCloseKey(hRegistry, hPrivileges);
    }

    if (hAccounts)
    {
        RegCloseKey(hRegistry, hAccounts);
    }

    if (hRegistry)
    {
        RegCloseServer(hRegistry);
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
    BOOLEAN Locked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
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

    //
    // Load the built-in privileges first
    //
    err = LsaSrvPrivsAddBuiltinPrivileges(pHash);
    BAIL_ON_LSA_ERROR(err);

    LSASRV_PRIVS_WRLOCK_RWLOCK(Locked, &pGlobals->privilegesRwLock);

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

    LSA_LOG_VERBOSE("Loading privileges database (%u privileges, "
                    "longest privilege name = %u)",
                    numKeys, maxKeyLen);
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

        LSA_LOG_VERBOSE("Loading privilege %s",
                        LSA_SAFE_LOG_STRING(pszPrivilegeName));

        if (!LsaSrvIsPrivilegeNameValid(pszPrivilegeName))
        {
            err = ERROR_INVALID_DATA;
            BAIL_ON_LSA_ERROR(err);
        }

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

        err = LsaSrvSetPrivilegeEntry_inlock(
                     pHash,
                     pszKey,
                     pPrivilegeEntry,
                     FALSE);
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

    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pGlobals->privilegesRwLock);

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
    WCHAR wszAttrValueLow[] = LSA_PRIVILEGE_LUID_LOW_NAME_W;
    WCHAR wszAttrValueHigh[] = LSA_PRIVILEGE_LUID_HIGH_NAME_W;
    WCHAR wszAttrEnabledByDefault[] = LSA_PRIVILEGE_ENABLED_NAME_W;
    WCHAR wszAttrDescription[] = LSA_PRIVILEGE_DESCRIPTION_NAME_W;
    LUID privilegeValue = {0};
    DWORD enabledByDefault = 0;
    PWSTR description = NULL;
    DWORD valueSize = 0;
    PLSA_PRIVILEGE pEntry = NULL;

    //
    // Privilege value (LUID)
    //
    valueSize = sizeof(privilegeValue.LowPart);
    err = RegGetValueW(
                 hRegistry,
                 hEntry,
                 NULL,
                 wszAttrValueLow,
                 RRF_RT_REG_DWORD,
                 NULL,
                 (PBYTE)&privilegeValue.LowPart,
                 &valueSize);
    BAIL_ON_LSA_ERROR(err);

    valueSize = sizeof(privilegeValue.HighPart);
    err = RegGetValueW(
                 hRegistry,
                 hEntry,
                 NULL,
                 wszAttrValueHigh,
                 RRF_RT_REG_DWORD,
                 NULL,
                 (PBYTE)&privilegeValue.HighPart,
                 &valueSize);
    BAIL_ON_LSA_ERROR(err);

    if (!LsaSrvIsPrivilegeValueValid(&privilegeValue))
    {
        err = ERROR_INVALID_DATA;
        BAIL_ON_LSA_ERROR(err);
    }

    //
    // "Enabled By Default" flag
    //
    valueSize = sizeof(enabledByDefault);
    err = RegGetValueW(
                 hRegistry,
                 hEntry,
                 NULL,
                 wszAttrEnabledByDefault,
                 RRF_RT_REG_DWORD,
                 NULL,
                 (PBYTE)&enabledByDefault,
                 &valueSize);
    BAIL_ON_LSA_ERROR(err);

    //
    // Privilege description (display name) string
    //
    valueSize = 0;
    err = RegGetValueW(
                 hRegistry,
                 hEntry,
                 NULL,
                 wszAttrDescription,
                 RRF_RT_REG_SZ,
                 NULL,
                 NULL,
                 &valueSize);
    BAIL_ON_LSA_ERROR(err);

    err = LwAllocateMemory(
                 valueSize,
                 OUT_PPVOID(&description));
    BAIL_ON_LSA_ERROR(err);

    err = RegGetValueW(
                 hRegistry,
                 hEntry,
                 NULL,
                 wszAttrDescription,
                 RRF_RT_REG_SZ,
                 NULL,
                 (PBYTE)description,
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
    pEntry->pwszDescription   = description;

    err = LwAllocateString(pszPrivilegeName,
                           &pEntry->pszName);
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


VOID
LsaSrvFreePrivileges(
    VOID
    )
{
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    BOOLEAN Locked = FALSE;

    LSASRV_PRIVS_WRLOCK_RWLOCK(Locked, &pGlobals->privilegesRwLock);
    LSASRV_PRIVS_WRLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);

    if (pGlobals->pPrivileges)
    {
        LwHashSafeFree(&pGlobals->pPrivileges);
    }

    if (pGlobals->pAccounts)
    {
        LwHashSafeFree(&pGlobals->pAccounts);
    }

    if (pGlobals->pPrivilegesSecDesc)
    {
        LsaSrvFreeSecurityDescriptor(pGlobals->pPrivilegesSecDesc);
    }

    LW_SAFE_FREE_MEMORY(pGlobals->pAccountsSecDescRelative);

    if (pGlobals->pSecurityContext)
    {
        LwMapSecurityFreeContext(&pGlobals->pSecurityContext);
    }

    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);
    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pGlobals->privilegesRwLock);
    pthread_rwlock_destroy(&pGlobals->accountsRwLock);
    pthread_rwlock_destroy(&pGlobals->privilegesRwLock);
}


DWORD
LsaSrvGetPrivilegeEntryByName(
    IN PCSTR pszPrivilegeName,
    OUT PLSA_PRIVILEGE *ppPrivilegeEntry
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN Locked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PLSA_PRIVILEGE pPrivilegeEntry = NULL;

    LSASRV_PRIVS_RDLOCK_RWLOCK(Locked, &pGlobals->privilegesRwLock);

    err = LwHashGetValue(gLsaPrivilegeGlobals.pPrivileges,
                         pszPrivilegeName,
                         (PVOID)&pPrivilegeEntry);
    if (err == ERROR_NOT_FOUND)
    {
        // Maps to STATUS_NO_SUCH_PRIVILEGE
        err = ERROR_NO_SUCH_PRIVILEGE;
        BAIL_ON_LSA_ERROR(err);
    }
    else if (err != ERROR_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(err);
    }

    *ppPrivilegeEntry = pPrivilegeEntry;

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pGlobals->privilegesRwLock);

    if (err)
    {
        if (ppPrivilegeEntry)
        {
            *ppPrivilegeEntry = NULL;
        }
    }

    return err;
}


DWORD
LsaSrvGetPrivilegeEntryByValue(
    IN PLUID PrivilegeValue,
    OUT PLSA_PRIVILEGE *ppPrivilegeEntry
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN Locked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    LW_HASH_ITERATOR iterator = {0};
    LW_HASH_ENTRY* pEntry = NULL;
    PLSA_PRIVILEGE pPrivilegeEntry = NULL;

    LSASRV_PRIVS_RDLOCK_RWLOCK(Locked, &pGlobals->privilegesRwLock);

    err = LwHashGetIterator(
                 pGlobals->pPrivileges,
                 &iterator);
    BAIL_ON_LSA_ERROR(err);

    do
    {
        pEntry = LwHashNext(&iterator);

        pPrivilegeEntry = (PLSA_PRIVILEGE)pEntry->pValue;
        if (!RtlEqualLuid(
                     &pPrivilegeEntry->Luid,
                     PrivilegeValue))
        {
            pPrivilegeEntry = NULL;
        }

    } while (pEntry && !pPrivilegeEntry);

    if (!pPrivilegeEntry)
    {
        // Maps to STATUS_NO_SUCH_PRIVILEGE
        err = ERROR_NO_SUCH_PRIVILEGE;
        BAIL_ON_LSA_ERROR(err);
    }

    *ppPrivilegeEntry = pPrivilegeEntry;

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pGlobals->privilegesRwLock);

    if (err)
    {
        if (ppPrivilegeEntry)
        {
            *ppPrivilegeEntry = NULL;
        }
    }

    return err;
}


static
DWORD
LsaSrvSetPrivilegeEntry_inlock(
    IN PLW_HASH_TABLE pPrivilegesTable,
    IN PSTR pszPrivilegeName,
    IN PLSA_PRIVILEGE pPrivilegeEntry,
    IN BOOLEAN Overwrite
    )
{
    DWORD err = ERROR_SUCCESS;
    PLSA_PRIVILEGE pExistingEntry = NULL;

    //
    // Check if a value already exists under this key and
    // overwrite it if requested
    //
    err = LwHashGetValue(pPrivilegesTable,
                         pszPrivilegeName,
                         (PVOID)&pExistingEntry);
    if (err == ERROR_SUCCESS && !Overwrite)
    {
        LSA_LOG_ERROR("Duplicate %s privilege entry found", pszPrivilegeName);

        err = ERROR_ALREADY_EXISTS;
        BAIL_ON_LSA_ERROR(err);
    }
    else if (err == ERROR_NOT_FOUND)
    {
        err = ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_LSA_ERROR(err);
    }

    err = LwHashSetValue(pPrivilegesTable,
                         pszPrivilegeName,
                         pPrivilegeEntry);

    BAIL_ON_LSA_ERROR(err);

error:
    return err;
}


DWORD
LsaSrvSetPrivilegeEntry(
    IN PSTR pszPrivilegeName,
    IN PLSA_PRIVILEGE pPrivilegeEntry,
    IN BOOLEAN Overwrite
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN Locked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;

    LSASRV_PRIVS_WRLOCK_RWLOCK(Locked, &pGlobals->privilegesRwLock);
    err = LsaSrvSetPrivilegeEntry_inlock(
                      pGlobals->pPrivileges,
                      pszPrivilegeName,
                      pPrivilegeEntry,
                      Overwrite);
    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pGlobals->privilegesRwLock);

    return err;
}


DWORD
LsaSrvPrivsGetPrivilegeEntries(
    IN OUT PDWORD pResume,
    IN DWORD PreferredMaxSize,
    OUT PLSA_PRIVILEGE **pppPrivileges,
    OUT PDWORD pCount
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD enumerationStatus = ERROR_SUCCESS;
    BOOLEAN Locked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    DWORD totalSize = 0;
    DWORD entrySize = 0;
    DWORD count = 0;
    DWORD index = 0;
    PLSA_PRIVILEGE *ppPrivileges = NULL;
    LW_HASH_ITERATOR iterator = {0};
    LW_HASH_ENTRY* pEntry = NULL;
    PLSA_PRIVILEGE pPrivilegeEntry = NULL;

    LSASRV_PRIVS_RDLOCK_RWLOCK(Locked, &pGlobals->privilegesRwLock);

    err = LwHashGetIterator(
                 pGlobals->pPrivileges,
                 &iterator);
    BAIL_ON_LSA_ERROR(err);

    //
    // First, calculate how many privileges are we going to return
    // (at least one)
    //
    if (PreferredMaxSize != (DWORD)(-1))
    {
        pEntry = LwHashNext(&iterator);

        do
        {
            if (index++ < (*pResume))
            {
                pEntry = LwHashNext(&iterator);
                continue;
            }
            else if (!pEntry)
            {
                break;
            }

            pPrivilegeEntry = (PLSA_PRIVILEGE)pEntry->pValue;
            totalSize += entrySize;

            // LSA_POLICY_PRIVILEGE_DEF includes UNICODE_STRING name and LUID
            entrySize += sizeof(LSA_POLICY_PRIVILEGE_DEF);
            entrySize += sizeof(WCHAR) * strlen(pPrivilegeEntry->pszName);

            count++;
            pEntry = LwHashNext(&iterator);

        } while (pEntry && (totalSize + entrySize < PreferredMaxSize));

        err = LwHashGetIterator(
                     pGlobals->pPrivileges,
                     &iterator);
        BAIL_ON_LSA_ERROR(err);

        index     = 0;
        totalSize = 0;
        entrySize = 0;
    }
    else
    {
        count = LwHashGetKeyCount(pGlobals->pPrivileges);
    }

    //
    // Allocate array of LSA_PRIVILEGE pointers to return
    //
    err = LwAllocateMemory(
                 sizeof(ppPrivileges[0]) * count,
                 OUT_PPVOID(&ppPrivileges));
    BAIL_ON_LSA_ERROR(err);

    count = 0;
    pEntry = LwHashNext(&iterator);

    do
    {
        if (index++ < (*pResume))
        {
            pEntry = LwHashNext(&iterator);
            continue;
        }
        else if (!pEntry)
        {
            break;
        }

        pPrivilegeEntry = (PLSA_PRIVILEGE)pEntry->pValue;
        totalSize += entrySize;

        // LSA_POLICY_PRIVILEGE_DEF includes UNICODE_STRING name and LUID
        entrySize += sizeof(LSA_POLICY_PRIVILEGE_DEF);
        entrySize += sizeof(WCHAR) * strlen(pPrivilegeEntry->pszName);

        ppPrivileges[count++] = pPrivilegeEntry;
        pEntry = LwHashNext(&iterator);

    } while (pEntry && (totalSize + entrySize < PreferredMaxSize));

    if (pEntry && count > 0)
    {
        enumerationStatus = ERROR_MORE_DATA;
    }
    else if (!pEntry && count == 0)
    {
        enumerationStatus = ERROR_NO_MORE_ITEMS;
    }

    *pResume       = index;
    *pppPrivileges = ppPrivileges;
    *pCount        = count;

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pGlobals->privilegesRwLock);

    if (err)
    {
        LW_SAFE_FREE_MEMORY(ppPrivileges);

        if (pppPrivileges)
        {
            *pppPrivileges = NULL;
        }

        if (pCount)
        {
            *pCount = 0;
        }

        if (pResume)
        {
            *pResume = 0;
        }
    }

    if (err == ERROR_SUCCESS)
    {
        err = enumerationStatus;
    }

    return err;
}

