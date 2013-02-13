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
DWORD
LsaSrvReadAccountEntry(
    IN HANDLE hRegistry,
    IN HKEY hEntry,
    IN PSID AccountSid,
    OUT PLSA_ACCOUNT *ppAccountEntry
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
    IN PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDesc,
    IN BOOLEAN Overwrite
    );

static
DWORD
LsaSrvGetPrivilegeNames_inlock(
    IN PLSA_ACCOUNT pAccountEntry,
    OUT PWSTR **ppPrivilegeNames
    );

static
DWORD
LsaSrvDeleteAccount_inlock(
    PSTR pszAccountSid
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
    DWORD i = 0;
    DWORD numKeys = 0;
    DWORD maxKeyLen = 0;
    PWSTR pwszAccount = NULL;
    PSTR pszAccount = NULL;
    HKEY hAccount = NULL;
    PSID accountSid = NULL;
    PLSA_ACCOUNT pAccountEntry = NULL;

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
    // Prepare for enumerating values under the Privileges key
    //
    err = RegQueryInfoKeyW(
                 hRegistry,
                 hAccountsKey,
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

    LSA_LOG_VERBOSE("Loading lsa accounts database (%u accounts, "
                    "longest account name = %u)",
                    numKeys, maxKeyLen);                    

    err = LwAllocateMemory(
                 sizeof(WCHAR) * (maxKeyLen + 1),
                 OUT_PPVOID(&pwszAccount));
    BAIL_ON_LSA_ERROR(err);

    for (i = 0; i < numKeys; i++)
    {
        DWORD accountKeyLen = sizeof(WCHAR) * maxKeyLen;

        err = RegEnumKeyExW(
                     hRegistry,
                     hAccountsKey,
                     i,
                     pwszAccount,
                     &accountKeyLen,
                     NULL,
                     NULL,
                     NULL,
                     NULL);
        BAIL_ON_LSA_ERROR(err);

        err = LwWc16sToMbs(
                     pwszAccount,
                     &pszAccount);
        BAIL_ON_LSA_ERROR(err);

        LSA_LOG_VERBOSE("Loading account %s",
                        LSA_SAFE_LOG_STRING(pszAccount));

        //
        // Each account is a SID
        //
        ntStatus = RtlAllocateSidFromWC16String(
                     &accountSid,
                     pwszAccount);
        if (ntStatus == STATUS_INVALID_SID)
        {
            err = ERROR_INVALID_DATA;
            BAIL_ON_LSA_ERROR(err);
        }
        else if (ntStatus != STATUS_SUCCESS)
        {
            BAIL_ON_NT_STATUS(ntStatus);
        }

        err = RegOpenKeyExW(
                     hRegistry,
                     hAccountsKey,
                     pwszAccount,
                     0,
                     KEY_READ,
                     &hAccount);
        BAIL_ON_LSA_ERROR(err);

        err = LsaSrvReadAccountEntry(
                     hRegistry,
                     hAccount,
                     accountSid,
                     &pAccountEntry);
        BAIL_ON_LSA_ERROR(err);

        err = LsaSrvSetAccountEntry_inlock(
                     pHash,
                     accountSid,
                     pAccountEntry,
                     FALSE);
        BAIL_ON_LSA_ERROR(err);

        accountSid    = NULL;
        pAccountEntry = NULL;

        //
        // Close the account subkey and cleanup the
        // name and the handle
        //
        err = RegCloseKey(
                     hRegistry,
                     hAccount);
        BAIL_ON_LSA_ERROR(err);

        LW_SAFE_FREE_MEMORY(pszAccount);

        memset(pwszAccount, 0, sizeof(WCHAR) * maxKeyLen);
        hAccount = NULL;
    }

    *ppAccounts = pHash;

error:
    if (err || ntStatus)
    {
        if (pAccountEntry)
        {
            LsaSrvPrivsReleaseAccountEntry(pAccountEntry);
        }

        LW_SAFE_FREE_MEMORY(accountSid);

        if (pHash)
        {
            LwHashSafeFree(&pHash);
        }

        *ppAccounts = NULL;
    }

    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);

    LW_SAFE_FREE_MEMORY(pwszAccount);
    LW_SAFE_FREE_MEMORY(pszAccount);

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
    LsaSrvPrivsReleaseAccountEntry(pEntry->pValue);
}


VOID
LsaSrvPrivsReleaseAccountEntry(
    PLSA_ACCOUNT pEntry
    )
{
    BOOLEAN Locked = FALSE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOLEAN defaulted = FALSE;
    BOOLEAN present = FALSE;

    InterlockedDecrement(&pEntry->Refcount);
    if (pEntry->Refcount)
    {
        return;
    }

    LSASRV_PRIVS_WRLOCK_RWLOCK(Locked, &pEntry->accountRwLock);

    LW_SAFE_FREE_MEMORY(pEntry->pSid);

    ntStatus = RtlGetOwnerSecurityDescriptor(
                        pEntry->pSecurityDesc,
                        &pOwnerSid,
                        &defaulted);
    if (ntStatus == STATUS_SUCCESS && pOwnerSid)
    {
        LW_SAFE_FREE_MEMORY(pOwnerSid);
    }

    ntStatus = RtlGetGroupSecurityDescriptor(
                        pEntry->pSecurityDesc,
                        &pGroupSid,
                        &defaulted);
    if (ntStatus == STATUS_SUCCESS && pGroupSid)
    {
        LW_SAFE_FREE_MEMORY(pGroupSid);
    }

    ntStatus = RtlGetDaclSecurityDescriptor(
                        pEntry->pSecurityDesc,
                        &present,
                        &pDacl,
                        &defaulted);
    if (ntStatus == STATUS_SUCCESS && pDacl)
    {
        LW_SAFE_FREE_MEMORY(pDacl);
    }

    ntStatus = RtlGetSaclSecurityDescriptor(
                        pEntry->pSecurityDesc,
                        &present,
                        &pSacl,
                        &defaulted);
    if (ntStatus == STATUS_SUCCESS && pSacl)
    {
        LW_SAFE_FREE_MEMORY(pSacl);
    }

    LW_SAFE_FREE_MEMORY(pEntry->pSecurityDesc);

    LSASRV_PRIVS_UNLOCK_RWLOCK(Locked, &pEntry->accountRwLock);
    pthread_rwlock_destroy(&pEntry->accountRwLock);

    LW_SAFE_FREE_MEMORY(pEntry);
}


VOID
LsaSrvPrivsAcquireAccountEntry(
    PLSA_ACCOUNT pEntry
    )
{
    InterlockedIncrement(&pEntry->Refcount);
}


static
DWORD
LsaSrvReadAccountEntry(
    IN HANDLE hRegistry,
    IN HKEY hEntry,
    IN PSID AccountSid,
    OUT PLSA_ACCOUNT *ppAccountEntry
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PSTR privilegesValueName = LSA_ACCOUNT_PRIVILEGES_NAME;
    DWORD privilegesValueSize = 0;
    PSTR privilegesValue = NULL;
    PSTR sysAccountRightsValueName = LSA_ACCOUNT_SYS_ACCESS_RIGHTS_NAME;
    DWORD systemAccess = 0;
    DWORD systemAccessSize = 0;
    PSTR securityDescValueName = LSA_PRIVILEGE_SECURITY_DESC_NAME;
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescRelative = NULL;
    DWORD securityDescRelativeSize = 0;
    BOOLEAN defaultSecurityDesc = FALSE;
    PLSA_ACCOUNT pAccountEntry = NULL;
    DWORD accountSidSize = 0;
    PSTR pszAccountSid = NULL;
    PWSTR *pPrivilegeNames = NULL;

    //
    // Get privileges value
    //
    err = RegGetValueA(
                 hRegistry,
                 hEntry,
                 NULL,
                 privilegesValueName,
                 RRF_RT_REG_MULTI_SZ,
                 NULL,
                 NULL,
                 &privilegesValueSize);
    BAIL_ON_LSA_ERROR(err);

    err = LwAllocateMemory(
                 privilegesValueSize,
                 OUT_PPVOID(&privilegesValue));
    BAIL_ON_LSA_ERROR(err);

    err = RegGetValueA(
                 hRegistry,
                 hEntry,
                 NULL,
                 privilegesValueName,
                 RRF_RT_REG_MULTI_SZ,
                 NULL,
                 (PBYTE)privilegesValue,
                 &privilegesValueSize);
    BAIL_ON_LSA_ERROR(err);

    //
    // Get system access rights value
    //
    systemAccessSize = sizeof(systemAccess);
    err = RegGetValueA(
                 hRegistry,
                 hEntry,
                 NULL,
                 sysAccountRightsValueName,
                 RRF_RT_REG_DWORD,
                 NULL,
                 (PBYTE)&systemAccess,
                 &systemAccessSize);
    BAIL_ON_LSA_ERROR(err);

    //
    // Get account security descriptor
    //
    err = RegGetValueA(
                 hRegistry,
                 hEntry,
                 NULL,
                 securityDescValueName,
                 RRF_RT_REG_BINARY,
                 NULL,
                 NULL,
                 &securityDescRelativeSize);
    if (err == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        //
        // This is probably the first lsassd start so there is no
        // security descriptor value assigned.
        //
        defaultSecurityDesc = TRUE;
        securityDescRelativeSize = pGlobals->accountsSecDescRelativeSize;
    }
    else if (err != STATUS_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(err);
    }

    err = LwAllocateMemory(
                 securityDescRelativeSize,
                 OUT_PPVOID(&pSecurityDescRelative));
    BAIL_ON_LSA_ERROR(err);

    if (defaultSecurityDesc)
    {
        memcpy(pSecurityDescRelative,
               pGlobals->pAccountsSecDescRelative,
               securityDescRelativeSize);
    }
    else
    {
        err = RegGetValueA(
                     hRegistry,
                     hEntry,
                     NULL,
                     securityDescValueName,
                     RRF_RT_REG_BINARY,
                     NULL,
                     (PBYTE)pSecurityDescRelative,
                     &securityDescRelativeSize);
        BAIL_ON_LSA_ERROR(err);
    }

    //
    // Create LSA_ACCOUNT entry
    //
    err = LwAllocateMemory(
                 sizeof(*pAccountEntry),
                 OUT_PPVOID(&pAccountEntry));
    BAIL_ON_LSA_ERROR(err);

    pthread_rwlock_init(&pAccountEntry->accountRwLock, NULL);
    pAccountEntry->Refcount = 1;

    accountSidSize = RtlLengthSid(AccountSid);
    err = LwAllocateMemory(
                 accountSidSize,
                 OUT_PPVOID(&pAccountEntry->pSid));
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlCopySid(
                 accountSidSize,
                 pAccountEntry->pSid,
                 AccountSid);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LsaSrvGetPrivilegeValuesFromNames(
                 privilegesValue,
                 privilegesValueSize,
                 sizeof(pAccountEntry->Privileges)/
                 sizeof(pAccountEntry->Privileges[0]),
                 pAccountEntry->Privileges,
                 &pAccountEntry->NumPrivileges);
    if (err == ERROR_INCORRECT_SIZE)
    {
        ntStatus = RtlAllocateCStringFromSid(
                     &pszAccountSid,
                     AccountSid);
        BAIL_ON_NT_STATUS(ntStatus);

        LSA_LOG_ERROR("Incorrect size of the privileges list assigned to %s",
                      pszAccountSid);

        err = ERROR_INVALID_DATA;
        BAIL_ON_LSA_ERROR(err);
    }
    else if (err != ERROR_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(err);
    }

    pAccountEntry->SystemAccessRights = systemAccess;

    err = LsaSrvAllocateAbsoluteFromSelfRelativeSD(
                 pSecurityDescRelative,
                 &pAccountEntry->pSecurityDesc);
    BAIL_ON_LSA_ERROR(err);

    if (defaultSecurityDesc)
    {
        BOOLEAN overwrite = TRUE;

        err = LsaSrvGetPrivilegeNames_inlock(
                          pAccountEntry,
                          &pPrivilegeNames);
        BAIL_ON_LSA_ERROR(err);

        //
        // Update account because it has just got its default 
        // security descriptor
        //
        err = LsaSrvSaveAccount_inlock(
                     AccountSid,
                     pPrivilegeNames,
                     pAccountEntry->SystemAccessRights,
                     pAccountEntry->pSecurityDesc,
                     overwrite);
        BAIL_ON_LSA_ERROR(err);
    }

    *ppAccountEntry = pAccountEntry;

error:
    if (err || ntStatus)
    {
        if (pAccountEntry)
        {
            LsaSrvPrivsReleaseAccountEntry(pAccountEntry);
        }

        *ppAccountEntry = NULL;
    }

    LW_SAFE_FREE_MEMORY(privilegesValue);

    if (pPrivilegeNames)
    {
        DWORD i = 0;

        for (i = 0; pPrivilegeNames[i]; i++)
        {
            LW_SAFE_FREE_MEMORY(pPrivilegeNames[i]);
        }
        LW_SAFE_FREE_MEMORY(pPrivilegeNames);
    }

    LW_SAFE_FREE_MEMORY(pSecurityDescRelative);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
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

    LsaSrvPrivsAcquireAccountEntry(pAccountEntry);

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
LsaSrvAddAccount_inlock(
    IN PSID pAccountSid,
    IN PLSA_ACCOUNT pAccountEntry
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN Locked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PWSTR *pPrivilegeNames = NULL;
    BOOLEAN overwrite = FALSE;

    err = LsaSrvGetPrivilegeNames_inlock(
                      pAccountEntry,
                      &pPrivilegeNames);
    BAIL_ON_LSA_ERROR(err);

    LSASRV_PRIVS_WRLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);
    err = LsaSrvSetAccountEntry_inlock(
                      pGlobals->pAccounts,
                      pAccountSid,
                      pAccountEntry,
                      overwrite);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvSaveAccount_inlock(
                      pAccountSid,
                      pPrivilegeNames,
                      pAccountEntry->SystemAccessRights,
                      pAccountEntry->pSecurityDesc,
                      overwrite);
    BAIL_ON_LSA_ERROR(err);

    //
    // LsaSrvAddAccount_inlock is called by LsaSrvPrivsCreateAccount so
    // new account has to be acquired before releasing accounts database
    // lock and making the account available for other threads.
    //
    LsaSrvPrivsAcquireAccountEntry(pAccountEntry);

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
    IN PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDesc,
    IN BOOLEAN Overwrite
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HANDLE hRegistry = NULL;
    HKEY hAccountsKey = NULL;
    PWSTR account = NULL;
    HKEY hAccount = NULL;
    WCHAR wszPrivilegeValueName[] = LSA_ACCOUNT_PRIVILEGES_NAME_W;
    WCHAR wszSysAccessRightsValueName[] = LSA_ACCOUNT_SYS_ACCESS_RIGHTS_NAME_W;
    WCHAR wszSecurityDescriptorValueName[] = LSA_PRIVILEGE_SECURITY_DESC_NAME_W;
    PBYTE privilegeNamesBlob = NULL;
    SSIZE_T privilegeNamesBlobSize = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescRelative = NULL;
    DWORD securityDescRelativeSize = 0;

    err = RegOpenServer(&hRegistry);
    BAIL_ON_LSA_ERROR(err);

    err = RegOpenKeyExA(
                 hRegistry,
                 NULL,
                 LSA_ACCOUNTS_REG_KEY,
                 0,
                 KEY_WRITE,
                 &hAccountsKey);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlAllocateWC16StringFromSid(
                 &account,
                 pAccountSid);
    BAIL_ON_NT_STATUS(ntStatus);

    //
    // Open the account subkey or create it if not existing
    //
    err = RegOpenKeyExW(
                 hRegistry,
                 hAccountsKey,
                 account,
                 0,
                 KEY_WRITE,
                 &hAccount);
    if (err == ERROR_SUCCESS && !Overwrite)
    {
        err = ERROR_ALREADY_EXISTS;
        BAIL_ON_LSA_ERROR(err);
    }
    else if (err == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        err = RegCreateKeyExW(
                     hRegistry,
                     hAccountsKey,
                     account,
                     0,
                     NULL,
                     0,
                     KEY_WRITE,
                     NULL,
                     &hAccount,
                     NULL);
        BAIL_ON_LSA_ERROR(err);
    }
    else if (err != ERROR_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(err);
    }

    //
    // Save Privileges value
    //

    // Convert privilege names array to blob and save it
    err = RegMultiStrsToByteArrayW(
                 ppwszPrivilegeNames,
                 &privilegeNamesBlob,
                 &privilegeNamesBlobSize);
    BAIL_ON_LSA_ERROR(err);

    err = RegSetValueExW(
                 hRegistry,
                 hAccount,
                 wszPrivilegeValueName,
                 0,
                 REG_MULTI_SZ,
                 privilegeNamesBlob,
                 privilegeNamesBlobSize);
    BAIL_ON_LSA_ERROR(err);

    //
    // Save system access rights
    //

    err = RegSetValueExW(
                 hRegistry,
                 hAccount,
                 wszSysAccessRightsValueName,
                 0,
                 REG_DWORD,
                 (PBYTE)&SystemAccessRights,
                 sizeof(SystemAccessRights));
    BAIL_ON_LSA_ERROR(err);

    //
    // Save account security descriptor
    //

    ntStatus = RtlAbsoluteToSelfRelativeSD(
                 pSecurityDesc,
                 pSecurityDescRelative,
                 &securityDescRelativeSize);
    if (ntStatus == STATUS_BUFFER_TOO_SMALL)
    {
        ntStatus = STATUS_SUCCESS;
    }
    else if (ntStatus != STATUS_SUCCESS)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    err = LwAllocateMemory(
                 securityDescRelativeSize,
                 OUT_PPVOID(&pSecurityDescRelative));
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlAbsoluteToSelfRelativeSD(
                 pSecurityDesc,
                 pSecurityDescRelative,
                 &securityDescRelativeSize);
    BAIL_ON_NT_STATUS(ntStatus);

    err = RegSetValueExW(
                 hRegistry,
                 hAccount,
                 wszSecurityDescriptorValueName,
                 0,
                 REG_BINARY,
                 (PBYTE)pSecurityDescRelative,
                 securityDescRelativeSize);
    BAIL_ON_LSA_ERROR(err);

error:
    if (hAccount)
    {
        RegCloseKey(hRegistry, hAccount);
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
    LW_SAFE_FREE_MEMORY(privilegeNamesBlob);
    LW_SAFE_FREE_MEMORY(pSecurityDescRelative);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
LsaSrvGetPrivilegeNames_inlock(
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
LsaSrvUpdateAccount_inlock(
    IN PSID pAccountSid,
    IN PLSA_ACCOUNT pAccountEntry
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN Locked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PWSTR *pPrivilegeNames = NULL;
    BOOLEAN Overwrite = TRUE;

    err = LsaSrvGetPrivilegeNames_inlock(
                      pAccountEntry,
                      &pPrivilegeNames);
    BAIL_ON_LSA_ERROR(err);

    LSASRV_PRIVS_WRLOCK_RWLOCK(Locked, &pGlobals->accountsRwLock);
    err = LsaSrvSaveAccount_inlock(
                      pAccountSid,
                      pPrivilegeNames,
                      pAccountEntry->SystemAccessRights,
                      pAccountEntry->pSecurityDesc,
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


DWORD
LsaSrvPrivsGetAccountEntries(
    IN OUT PDWORD pResume,
    IN DWORD PreferredMaxSize,
    OUT PLSA_ACCOUNT **pppAccounts,
    OUT PDWORD pCount
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD enumerationStatus = ERROR_SUCCESS;
    BOOLEAN accountsLocked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    DWORD totalSize = 0;
    DWORD entrySize = 0;
    DWORD count = 0;
    DWORD index = 0;
    PLSA_ACCOUNT *ppAccounts = NULL;
    LW_HASH_ITERATOR iterator = {0};
    LW_HASH_ENTRY* pEntry = NULL;
    PLSA_ACCOUNT pAccountEntry = NULL;
    DWORD i = 0;

    LSASRV_PRIVS_RDLOCK_RWLOCK(accountsLocked, &pGlobals->accountsRwLock);

    err = LwHashGetIterator(
                 pGlobals->pAccounts,
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

            pAccountEntry = (PLSA_ACCOUNT)pEntry->pValue;
            totalSize += entrySize;

            // LSA_ACCOUNT_INFO includes pointer to account SID
            entrySize += sizeof(LSA_ACCOUNT_INFO);
            entrySize += RtlLengthSid(pAccountEntry->pSid);

            count++;
            pEntry = LwHashNext(&iterator);

        } while (pEntry && (totalSize + entrySize < PreferredMaxSize));

        err = LwHashGetIterator(
                     pGlobals->pAccounts,
                     &iterator);
        BAIL_ON_LSA_ERROR(err);

        index     = 0;
        totalSize = 0;
        entrySize = 0;
    }
    else
    {
        count = LwHashGetKeyCount(pGlobals->pAccounts);
    }

    //
    // Allocate array of LSA_PRIVILEGE pointers to return
    //
    err = LwAllocateMemory(
                 sizeof(ppAccounts[0]) * count,
                 OUT_PPVOID(&ppAccounts));
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

        pAccountEntry = (PLSA_ACCOUNT)pEntry->pValue;
        LsaSrvPrivsAcquireAccountEntry(pAccountEntry);

        totalSize += entrySize;

        // LSA_ACCOUNT_INFO includes pointer to account SID
        entrySize += sizeof(LSA_ACCOUNT_INFO);
        entrySize += RtlLengthSid(pAccountEntry->pSid);

        ppAccounts[count++] = pAccountEntry;
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

    *pResume      = index;
    *pppAccounts  = ppAccounts;
    *pCount       = count;

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountsLocked, &pGlobals->accountsRwLock);

    if (err)
    {
        for (i = 0; i < count; i++)
        {
            LsaSrvPrivsReleaseAccountEntry(ppAccounts[i]);
        }
        LW_SAFE_FREE_MEMORY(ppAccounts);

        if (pppAccounts)
        {
            *pppAccounts = NULL;
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

    if (err == ERROR_SUCCESS &&
        enumerationStatus != ERROR_SUCCESS)
    {
        err = enumerationStatus;
    }

    return err;
}


VOID
LsaSrvPrivsCheckIfDeleteAccount(
    PLSA_ACCOUNT pAccount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;
    BOOLEAN accountsLocked = FALSE;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PSID accountSid = NULL;
    PSTR pszAccountSid = NULL;

    LSASRV_PRIVS_WRLOCK_RWLOCK(accountsLocked, &pGlobals->accountsRwLock);

    if (!pAccount->Delete || pAccount->Refcount > 1)
    {
        goto error;
    }

    accountSid = pAccount->pSid;
    ntStatus = RtlAllocateCStringFromSid(
                        &pszAccountSid,
                        accountSid);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LwHashRemoveKey(
                        pGlobals->pAccounts,
                        accountSid);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvDeleteAccount_inlock(pszAccountSid);
    BAIL_ON_LSA_ERROR(err);

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountsLocked, &pGlobals->accountsRwLock);

    RTL_FREE(&pszAccountSid);
}


static
DWORD
LsaSrvDeleteAccount_inlock(
    PSTR pszAccountSid
    )
{
    DWORD err = ERROR_SUCCESS;
    HANDLE hRegistry = NULL;
    HKEY hAccounts = NULL;

    err = RegOpenServer(&hRegistry);
    BAIL_ON_LSA_ERROR(err);

    err = RegOpenKeyExA(
                 hRegistry,
                 NULL,
                 LSA_ACCOUNTS_REG_KEY,
                 0,
                 KEY_WRITE | DELETE,
                 &hAccounts);
    BAIL_ON_LSA_ERROR(err);

    err = RegDeleteKeyA(
                 hRegistry,
                 hAccounts,
                 pszAccountSid);
    BAIL_ON_LSA_ERROR(err);

error:
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
