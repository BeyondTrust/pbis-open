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
 *        accountdb.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Lsa Accounts database
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
int
LsaSrvSidCompare(
    PCVOID sid1,
    PCVOID sid2
    );

static
size_t
LsaSrvSidHash(
    PCVOID sid
    );

static
VOID
LsaSrvAccountDbEntryFree(
    const LW_HASH_ENTRY *pEntry
    );

static
VOID
LsaSrvFreeAccountEntry(
    PLSA_ACCOUNT pEntry
    );

static
DWORD
LsaSrvGetPrivilegeValuesFromNames(
    PSTR pszPrivilegeNames,
    DWORD PrivilegeNamesSize,
    DWORD MaxPrivilegeValues,
    PLUID_AND_ATTRIBUTES pPrivilegeValues,
    PDWORD pPrivilegeValuesCount
    );

static
DWORD
LsaSrvSetAccountEntry_inlock(
    IN PLW_HASH_TABLE pAccountsTable,
    IN PSID pAccountSid,
    IN PLSA_ACCOUNT pAccountEntry,
    IN BOOLEAN Overwrite
    );

static
DWORD
LsaSrvSaveAccount_inlock(
    IN PSID pAccountSid,
    IN PWSTR *ppwszPrivilegeNames,
    IN DWORD SystemAccessRights,
    IN BOOLEAN Overwrite
    );

static
DWORD
LsaSrvGetPrivilegeNames(
    IN PLSA_ACCOUNT pAccountEntry,
    OUT PWSTR **ppPrivilegeNames
    );


DWORD
LsaSrvCreateAccountsDb(
    IN HANDLE hRegistry,
    IN HKEY hAccountsKey,
    OUT PLW_HASH_TABLE *ppAccounts
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN Locked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PLW_HASH_TABLE pHash = NULL;
    PSTR pszPrivilegeName = LSA_ACCOUNTS_PRIVILEGES_SUBKEY;
    PSTR pszSysAccessRightsName = LSA_ACCOUNTS_SYS_ACCESS_RIGHTS_SUBKEY;
    HKEY hPrivilegesKey = NULL;
    HKEY hSystemAccessRightsKey = NULL;
    DWORD i = 0;
    DWORD numValues = 0;
    DWORD maxValueNameLen = 0;
    DWORD maxValueLen = 0;
    PSTR pszAccount = NULL;
    PSID pAccountSid = NULL;
    PSTR pszPrivileges = NULL;
    DWORD systemAccess = 0;
    PLSA_ACCOUNT pAccountEntry = NULL;
    DWORD accountSidSize = 0;

    LSASRV_PRIVS_WRLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);

    //
    // Create the database hash table
    //
    err = LwHashCreate(
                 LSA_ACCOUNTS_DB_SIZE,
                 LsaSrvSidCompare,
                 LsaSrvSidHash,
                 LsaSrvAccountDbEntryFree,
                 NULL,
                 &pHash);
    BAIL_ON_LSA_ERROR(err);

    //
    // Open Privileges and SystemAccessRights subkeys
    //
    err = RegOpenKeyExA(
                 hRegistry,
                 hAccountsKey,
                 pszPrivilegeName,
                 0,
                 KEY_READ,
                 &hPrivilegesKey);
    BAIL_ON_LSA_ERROR(err);

    err = RegOpenKeyExA(
                 hRegistry,
                 hAccountsKey,
                 pszSysAccessRightsName,
                 0,
                 KEY_READ,
                 &hSystemAccessRightsKey);
    BAIL_ON_LSA_ERROR(err);

    //
    // Prepare to enumerating values under the Privileges key
    //
    err = RegQueryInfoKeyA(
                 hRegistry,
                 hPrivilegesKey,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 &numValues,
                 &maxValueNameLen,
                 &maxValueLen,
                 NULL,
                 NULL);
    BAIL_ON_LSA_ERROR(err);

    err = LwAllocateMemory(
                 maxValueNameLen,
                 OUT_PPVOID(&pszAccount));
    BAIL_ON_LSA_ERROR(err);

    err = LwAllocateMemory(
                 maxValueLen,
                 OUT_PPVOID(&pszPrivileges));
    BAIL_ON_LSA_ERROR(err);

    for (i = 0; i < numValues; i++)
    {
        DWORD accountLen = maxValueNameLen;
        DWORD privilegesSize = maxValueLen;
        DWORD privilegesType = 0;
        DWORD systemAccessSize = 0;

        err = RegEnumValueA(
                     hRegistry,
                     hPrivilegesKey,
                     i,
                     pszAccount,
                     &accountLen,
                     NULL,
                     &privilegesType,
                     (PBYTE)pszPrivileges,
                     &privilegesSize);
        BAIL_ON_LSA_ERROR(err);

        //
        // Each account is a SID
        //
        ntStatus = RtlAllocateSidFromCString(
                     &pAccountSid,
                     pszAccount);
        if (ntStatus == STATUS_INVALID_SID)
        {
            err = ERROR_INVALID_DATA;
            BAIL_ON_LSA_ERROR(err);
        }
        else if (ntStatus != STATUS_SUCCESS)
        {
            BAIL_ON_NT_STATUS(ntStatus);
        }

        //
        // Get system access rights value under SystemAccessRights subkey
        systemAccessSize = sizeof(systemAccess);
        err = RegGetValueA(
                     hRegistry,
                     hSystemAccessRightsKey,
                     NULL,
                     pszAccount,
                     RRF_RT_REG_DWORD,
                     NULL,
                     (PBYTE)&systemAccess,
                     &systemAccessSize);
        BAIL_ON_LSA_ERROR(err);

        //
        // Create LSA_ACCOUNT entry
        //
        err = LwAllocateMemory(
                     sizeof(*pAccountEntry),
                     OUT_PPVOID(&pAccountEntry));
        BAIL_ON_LSA_ERROR(err);

        pthread_rwlock_init(&pAccountEntry->accountRwLock, NULL);

        accountSidSize = RtlLengthSid(pAccountSid);
        err = LwAllocateMemory(
                     accountSidSize,
                     OUT_PPVOID(&pAccountEntry->pSid));
        BAIL_ON_LSA_ERROR(err);

        ntStatus = RtlCopySid(
                     accountSidSize,
                     pAccountEntry->pSid,
                     pAccountSid);
        BAIL_ON_NT_STATUS(ntStatus);

        err = LsaSrvGetPrivilegeValuesFromNames(
                     pszPrivileges,
                     privilegesSize,
                     sizeof(pAccountEntry->Privileges)/
                     sizeof(pAccountEntry->Privileges[0]),
                     pAccountEntry->Privileges,
                     &pAccountEntry->NumPrivileges);
        if (err == ERROR_INCORRECT_SIZE)
        {
            LSA_LOG_ERROR("Incorrect size of the privileges list assigned to %s",
                          pszAccount);

            err = ERROR_INVALID_DATA;
            BAIL_ON_LSA_ERROR(err);
        }
        else if (err != ERROR_SUCCESS)
        {
            BAIL_ON_LSA_ERROR(err);
        }

        pAccountEntry->SystemAccessRights = systemAccess;

        err = LsaSrvSetAccountEntry_inlock(
                     pHash,
                     pAccountSid,
                     pAccountEntry,
                     FALSE);
        BAIL_ON_LSA_ERROR(err);

        pAccountSid   = NULL;
        pAccountEntry = NULL;

        memset(pszAccount, 0, maxValueNameLen);
        memset(pszPrivileges, 0, maxValueLen);
        systemAccess = 0;

    }

    *ppAccounts = pHash;

error:
    if (err)
    {
        if (pHash)
        {
            LwHashSafeFree(&pHash);
        }

        *ppAccounts = NULL;
    }

    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);

    if (hPrivilegesKey)
    {
        RegCloseKey(
                 hRegistry,
                 hPrivilegesKey);
    }

    if (hSystemAccessRightsKey)
    {
        RegCloseKey(
                 hRegistry,
                 hSystemAccessRightsKey);
    }

    LW_SAFE_FREE_MEMORY(pszAccount);
    LW_SAFE_FREE_MEMORY(pszPrivileges);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
int
LsaSrvSidCompare(
    PCVOID sid1,
    PCVOID sid2
    )
{
    const PSID pSid1 = (PSID)sid1;
    const PSID pSid2 = (PSID)sid2;
    DWORD i = 0;
    DWORD subAuthorityCount = LW_MIN(pSid2->SubAuthorityCount,
                                     pSid2->SubAuthorityCount);

    for (i = 0; i < subAuthorityCount; i++)
    {
        if (pSid1->SubAuthority[i] != pSid2->SubAuthority[i])
        {
            return (int)(pSid1->SubAuthority[i] - pSid2->SubAuthority[i]);
        }
    }

    return 0;
}


static
size_t
LsaSrvSidHash(
    PCVOID sid
    )
{
    const PSID pSid = (PSID)sid;
    DWORD i = 0;
    size_t hash = 0;

    for (i = 0; i < pSid->SubAuthorityCount; i++)
    {
        hash += pSid->SubAuthority[i];
    }

    return hash;
}


static
VOID
LsaSrvAccountDbEntryFree(
    const LW_HASH_ENTRY *pEntry
    )
{
    LwFreeMemory(pEntry->pKey);
    LsaSrvFreeAccountEntry(pEntry->pValue);
}


static
VOID
LsaSrvFreeAccountEntry(
    PLSA_ACCOUNT pEntry
    )
{
    BOOLEAN Locked = FALSE;

    LSASRV_PRIVS_WRLOCK_RWLOCK(Locked, &pEntry->accountRwLock);

    LW_SAFE_FREE_MEMORY(pEntry->pSid);
    LW_SAFE_FREE_MEMORY(pEntry->pSecDesc);

    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pEntry->accountRwLock);
    pthread_rwlock_destroy(&pEntry->accountRwLock);

    LW_SAFE_FREE_MEMORY(pEntry);
}


static
DWORD
LsaSrvGetPrivilegeValuesFromNames(
    PSTR pszPrivilegeNames,
    DWORD PrivilegeNamesSize,
    DWORD MaxPrivilegeValues,
    PLUID_AND_ATTRIBUTES pPrivilegeValues,
    PDWORD pPrivilegeValuesCount
    )
{
    DWORD err = ERROR_SUCCESS;
    PSTR pszPrivilegeName = NULL;
    PLSA_PRIVILEGE pPrivilegeEntry = NULL;
    DWORD iValue = 0;

    pszPrivilegeName = pszPrivilegeNames;

    while (pszPrivilegeName && *pszPrivilegeName != '\0')
    {
        err = LsaSrvGetPrivilegeEntryByName(
                                  pszPrivilegeName,
                                  &pPrivilegeEntry);
        if (err == ERROR_NO_SUCH_PRIVILEGE)
        {
            LSA_LOG_ERROR("%s privilege is assigned but not supported",
                          pszPrivilegeName);

            err = ERROR_INVALID_DATA;
            BAIL_ON_LSA_ERROR(err);
        }
        else if (err != ERROR_SUCCESS)
        {
            BAIL_ON_LSA_ERROR(err);
        }

        if (iValue < MaxPrivilegeValues)
        {
            pPrivilegeValues[iValue].Luid = pPrivilegeEntry->Luid;
            if (pPrivilegeEntry->EnabledByDefault)
            {
                pPrivilegeValues[iValue].Attributes
                    |= SE_PRIVILEGE_ENABLED_BY_DEFAULT;
            }

            iValue++;
        }
        else
        {
            LSA_LOG_ERROR("Too many privileges assigned (max number = %d)",
                          MaxPrivilegeValues);

            err = ERROR_INCORRECT_SIZE;
            BAIL_ON_LSA_ERROR(err);
        }

        pszPrivilegeName += strlen(pszPrivilegeName) + 1;
    }

    *pPrivilegeValuesCount = iValue;

error:
    return err;
}


DWORD
LsaSrvGetAccountEntry(
    IN PSID pAccountSid,
    OUT PLSA_ACCOUNT *ppAccountEntry
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN Locked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PLSA_ACCOUNT pAccountEntry = NULL;

    LSASRV_PRIVS_RDLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);

    err = LwHashGetValue(gLsaPrivilegeGlobals.pAccounts,
                         pAccountSid,
                         (PVOID)&pAccountEntry);
    if (err == ERROR_NOT_FOUND)
    {
        // Maps to STATUS_OBJECT_NAME_NOT_FOUND
        err = ERROR_FILE_NOT_FOUND;
        BAIL_ON_LSA_ERROR(err);
    }
    else if (err != ERROR_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(err);
    }

    *ppAccountEntry = pAccountEntry;

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);

    if (err)
    {
        if (ppAccountEntry)
        {
            *ppAccountEntry = NULL;
        }
    }

    return err;
}


DWORD
LsaSrvSetAccountEntry(
    IN PSID pAccountSid,
    IN PLSA_ACCOUNT pAccountEntry,
    IN BOOLEAN Overwrite
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN Locked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;

    LSASRV_PRIVS_WRLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);
    err = LsaSrvSetAccountEntry_inlock(
                      pGlobals->pAccounts,
                      pAccountSid,
                      pAccountEntry,
                      Overwrite);
    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);

    return err;
}


static
DWORD
LsaSrvSetAccountEntry_inlock(
    IN PLW_HASH_TABLE pAccountsTable,
    IN PSID pAccountSid,
    IN PLSA_ACCOUNT pAccountEntry,
    IN BOOLEAN Overwrite
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSA_ACCOUNT pExistingEntry = NULL;
    PSTR pszSid = NULL;

    //
    // Check if a value already exists under this key and
    // overwrite it if requested
    //
    err = LwHashGetValue(pAccountsTable,
                         pAccountSid,
                         (PVOID)&pExistingEntry);
    if (err == ERROR_SUCCESS && !Overwrite)
    {
        ntStatus = RtlAllocateCStringFromSid(
                         &pszSid,
                         pAccountSid);
        BAIL_ON_NT_STATUS(ntStatus);

        LSA_LOG_WARNING("Duplicate %s account entry found", pszSid);

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

    err = LwHashSetValue(pAccountsTable,
                         pAccountSid,
                         pAccountEntry);

    BAIL_ON_LSA_ERROR(err);

error:
    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    RTL_FREE(&pszSid);

    return err;
}


DWORD
LsaSrvAddAccount(
    IN PSID pAccountSid,
    IN PLSA_ACCOUNT pAccountEntry
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN Locked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PWSTR *pPrivilegeNames = NULL;
    BOOLEAN Overwrite = FALSE;

    err = LsaSrvGetPrivilegeNames(
                      pAccountEntry,
                      &pPrivilegeNames);
    BAIL_ON_LSA_ERROR(err);

    LSASRV_PRIVS_WRLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);
    err = LsaSrvSetAccountEntry_inlock(
                      pGlobals->pAccounts,
                      pAccountSid,
                      pAccountEntry,
                      Overwrite);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvSaveAccount_inlock(
                      pAccountSid,
                      pPrivilegeNames,
                      pAccountEntry->SystemAccessRights,
                      Overwrite);
    BAIL_ON_LSA_ERROR(err);

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);

    if (pPrivilegeNames)
    {
        DWORD i = 0;

        for (i = 0; pPrivilegeNames[i]; i++)
        {
            LW_SAFE_FREE_MEMORY(pPrivilegeNames[i]);
        }
        LW_SAFE_FREE_MEMORY(pPrivilegeNames);
    }

    return err;
}


static
DWORD
LsaSrvSaveAccount_inlock(
    IN PSID pAccountSid,
    IN PWSTR *ppwszPrivilegeNames,
    IN DWORD SystemAccessRights,
    IN BOOLEAN Overwrite
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HANDLE hRegistry = NULL;
    HKEY hAccountsKey = NULL;
    PSTR pszPrivilegeName = LSA_ACCOUNTS_PRIVILEGES_SUBKEY;
    PSTR pszSysAccessRightsName = LSA_ACCOUNTS_SYS_ACCESS_RIGHTS_SUBKEY;
    HKEY hPrivilegesKey = NULL;
    HKEY hSystemAccessRightsKey = NULL;
    PWSTR account = NULL;
    DWORD existingPrivilegesBlobSize = 0;
    PBYTE privilegeNamesBlob = NULL;
    SSIZE_T privilegeNamesBlobSize = 0;

    err = RegOpenServer(&hRegistry);
    BAIL_ON_LSA_ERROR(err);

    err = RegOpenKeyExA(
                 hRegistry,
                 NULL,
                 LSA_ACCOUNTS_REG_KEY,
                 0,
                 KEY_READ,
                 &hAccountsKey);
    BAIL_ON_LSA_ERROR(err);

    //
    // Open Privileges and SystemAccessRights subkeys
    //
    err = RegOpenKeyExA(
                 hRegistry,
                 hAccountsKey,
                 pszPrivilegeName,
                 0,
                 KEY_WRITE,
                 &hPrivilegesKey);
    BAIL_ON_LSA_ERROR(err);

    err = RegOpenKeyExA(
                 hRegistry,
                 hAccountsKey,
                 pszSysAccessRightsName,
                 0,
                 KEY_WRITE,
                 &hSystemAccessRightsKey);
    BAIL_ON_LSA_ERROR(err);

    // Check if account already exists
    ntStatus = RtlAllocateWC16StringFromSid(
                 &account,
                 pAccountSid);
    BAIL_ON_NT_STATUS(ntStatus);

    err = RegGetValueW(
                 hRegistry,
                 hPrivilegesKey,
                 NULL,
                 account,
                 RRF_RT_REG_MULTI_SZ,
                 NULL,
                 NULL,
                 &existingPrivilegesBlobSize);
    if (err == ERROR_SUCCESS &&
        !Overwrite)
    {
        err = ERROR_ALREADY_EXISTS;
        BAIL_ON_LSA_ERROR(err);
    }
    else if (err == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        err = ERROR_SUCCESS;
    }
    else if (err != ERROR_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(err);
    }

    // Convert privilege names array to blob and save it
    err = RegMultiStrsToByteArrayW(
                 ppwszPrivilegeNames,
                 &privilegeNamesBlob,
                 &privilegeNamesBlobSize);
    BAIL_ON_LSA_ERROR(err);

    err = RegSetValueExW(
                 hRegistry,
                 hPrivilegesKey,
                 account,
                 0,
                 REG_MULTI_SZ,
                 privilegeNamesBlob,
                 privilegeNamesBlobSize);
    BAIL_ON_LSA_ERROR(err);

    // Save system access rights
    err = RegSetValueExW(
                 hRegistry,
                 hSystemAccessRightsKey,
                 account,
                 0,
                 REG_DWORD,
                 (PBYTE)&SystemAccessRights,
                 sizeof(SystemAccessRights));
    BAIL_ON_LSA_ERROR(err);

error:
    if (hPrivilegesKey)
    {
        RegCloseKey(
                 hRegistry,
                 hPrivilegesKey);
    }

    if (hSystemAccessRightsKey)
    {
        RegCloseKey(
                 hRegistry,
                 hSystemAccessRightsKey);
    }

    if (hAccountsKey)
    {
        RegCloseKey(hRegistry, hAccountsKey);
    }

    if (hRegistry)
    {
        RegCloseServer(hRegistry);
    }

    RTL_FREE(&account);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
LsaSrvGetPrivilegeNames(
    IN PLSA_ACCOUNT pAccountEntry,
    OUT PWSTR **ppPrivilegeNames
    )
{
    DWORD err = ERROR_SUCCESS;
    PWSTR *pPrivilegeNames = NULL;
    DWORD i = 0;

    err = LwAllocateMemory(
                 sizeof(PWSTR) * (pAccountEntry->NumPrivileges + 1),
                 OUT_PPVOID(&pPrivilegeNames));
    BAIL_ON_LSA_ERROR(err);

    for (i = 0; i < pAccountEntry->NumPrivileges; i++)
    {
        PLSA_PRIVILEGE pPrivilege = NULL;

        err = LsaSrvGetPrivilegeEntryByValue(
                     &pAccountEntry->Privileges[i].Luid,
                     &pPrivilege);
        BAIL_ON_LSA_ERROR(err);

        err = LwMbsToWc16s(
                     pPrivilege->pszName,
                     &pPrivilegeNames[i]);
        BAIL_ON_LSA_ERROR(err);
    }

    *ppPrivilegeNames = pPrivilegeNames;

error:
    if (err)
    {
        for (i = 0; pPrivilegeNames && i < pAccountEntry->NumPrivileges; i++)
        {
            LW_SAFE_FREE_MEMORY(pPrivilegeNames[i]);
        }
        LW_SAFE_FREE_MEMORY(pPrivilegeNames);

        *ppPrivilegeNames = NULL;
    }

    return err;
}


DWORD
LsaSrvUpdateAccount(
    IN PSID pAccountSid,
    IN PLSA_ACCOUNT pAccountEntry
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN Locked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PWSTR *pPrivilegeNames = NULL;
    BOOLEAN Overwrite = TRUE;

    err = LsaSrvGetPrivilegeNames(
                      pAccountEntry,
                      &pPrivilegeNames);
    BAIL_ON_LSA_ERROR(err);

    LSASRV_PRIVS_WRLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);
    err = LsaSrvSaveAccount_inlock(
                      pAccountSid,
                      pPrivilegeNames,
                      pAccountEntry->SystemAccessRights,
                      Overwrite);
    BAIL_ON_LSA_ERROR(err);

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);

    if (pPrivilegeNames)
    {
        DWORD i = 0;

        for (i = 0; pPrivilegeNames[i]; i++)
        {
            LW_SAFE_FREE_MEMORY(pPrivilegeNames[i]);
        }
        LW_SAFE_FREE_MEMORY(pPrivilegeNames);
    }

    return err;
}
