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
 *        account.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Privileges (LSA Accounts API)
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
DWORD
LsaSrvResolveNamesToPrivilegesAndSar(
    IN HANDLE hProvider,
    IN PACCESS_TOKEN AccessToken,
    IN PWSTR *pAccountRights,
    IN DWORD NumAccountRights,
    OUT PPRIVILEGE_SET *pPrivilegeSet,
    OUT PDWORD pSystemAccessRights
    );

static
DWORD
LsaSrvMergePrivilegeLists(
    IN PLUID_AND_ATTRIBUTES pPrivs1,
    IN DWORD NumPrivs1,
    IN PLUID_AND_ATTRIBUTES pPrivs2,
    IN DWORD NumPrivs2,
    IN DWORD RetPrivsLen,
    IN OUT OPTIONAL PLUID_AND_ATTRIBUTES pRetPrivs,
    OUT PDWORD pNumRetPrivs
    );

static
DWORD
LsaSrvAccountRightNameToSar(
    IN PWSTR AccessRightName,
    OUT PDWORD pAccessRight
    );

static
DWORD
LsaSrvSarToAccountRightName(
    IN DWORD AccessRight,
    OUT PWSTR* pAccessRightName
    );

static
DWORD
LsaSrvLookupSysAccessRightNameMap(
    IN OUT PDWORD pSystemAccessRight,
    IN OUT PWSTR* pAccessRightName,
    IN BOOLEAN ResolveName
    );


DWORD
LsaSrvPrivsEnumAccountRightsSids(
    IN HANDLE hProvider,
    IN PSTR *ppszSids,
    IN DWORD NumSids,
    OUT PLUID_AND_ATTRIBUTES *ppPrivileges,
    OUT PDWORD pNumPrivileges,
    OUT PDWORD pSystemAccessRights
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID accountSid = NULL;
    ACCESS_MASK accessRights = LSA_ACCOUNT_VIEW;
    PLSA_ACCOUNT_CONTEXT accountContext = NULL;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = NULL;
    DWORD privilegesLen = LSA_MAX_PRIVILEGES_COUNT;
    PLUID_AND_ATTRIBUTES pPrivileges = NULL;
    DWORD numPrivileges = 0;
    DWORD i = 0;

    err = LwAllocateMemory(
                   sizeof(pPrivileges[0]) * privilegesLen,
                   OUT_PPVOID(&pPrivileges));
    BAIL_ON_LSA_ERROR(err);

    //
    // Get each SID privileges (if there are any)
    //
    for (i = 0; i < NumSids; i++)
    {
        ntStatus = RtlAllocateSidFromCString(
                                &accountSid,
                                ppszSids[i]);
        if (ntStatus)
        {
            err = LwNtStatusToWin32Error(ntStatus);
            BAIL_ON_LSA_ERROR(err); 
        }

        err = LsaSrvPrivsOpenAccount(
                                hProvider,
                                NULL,
                                accountSid,
                                accessRights,
                                &accountContext);
        if (err == ERROR_FILE_NOT_FOUND)
        {
            // Such account (SID) does not exist in the database.
            // Try the next one.
            RTL_FREE(&accountSid);
            err = ERROR_SUCCESS;

            continue;
        }
        else if (err != ERROR_SUCCESS)
        {
            BAIL_ON_LSA_ERROR(err);
        }

        pAccount = accountContext->pAccount;

        LSASRV_PRIVS_RDLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

        err = LsaSrvMergePrivilegeLists(
                                pPrivileges,
                                numPrivileges,
                                pAccount->Privileges,
                                pAccount->NumPrivileges,
                                privilegesLen,
                                pPrivileges,
                                &numPrivileges);
        // Error check goes after unlocking account to prevent from leaving
        // account locked if we bailed after error code was returned
        LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);
        BAIL_ON_LSA_ERROR(err);

        LsaSrvPrivsCloseAccount(hProvider,
                                &accountContext);
        accountContext = NULL;
        pAccount       = NULL;

        RTL_FREE(&accountSid);
    }

    //
    // Enable privileges if they are enabled by default
    //
    for (i = 0; i < numPrivileges; i++)
    {
        if (pPrivileges[i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)
        {
            pPrivileges[i].Attributes |= SE_PRIVILEGE_ENABLED;
        }
    }

    *ppPrivileges   = pPrivileges;
    *pNumPrivileges = numPrivileges;

error:
    if (err)
    {
        LW_SAFE_FREE_MEMORY(pPrivileges);

        if (ppPrivileges)
        {
            *ppPrivileges = NULL;
        }

        if (pNumPrivileges)
        {
            *pNumPrivileges = 0;
        }
    }

    if (accountContext)
    {
        LsaSrvPrivsCloseAccount(
                  hProvider,
                  &accountContext);
    }

    RTL_FREE(&accountSid);

    return err;
}


DWORD
LsaSrvPrivsAddAccountRights(
    IN HANDLE hProvider,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PSID pAccountSid,
    IN PWSTR *ppwszAccountRights,
    IN DWORD NumAccountRights
    )
{
    DWORD err = ERROR_SUCCESS;
    ACCESS_MASK accessRights = LSA_ACCOUNT_ADJUST_PRIVILEGES;
    PACCESS_TOKEN accessToken = AccessToken;
    PLSA_ACCOUNT_CONTEXT accountContext = NULL;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = NULL;
    PPRIVILEGE_SET pPrivilegeSet = NULL;
    DWORD addAccessRights = 0;
    DWORD newAccessRights = 0;
    
    err = LsaSrvPrivsOpenAccount(
                            hProvider,
                            accessToken,
                            pAccountSid,
                            accessRights,
                            &accountContext);
    if (err == ERROR_FILE_NOT_FOUND)
    {
        err = LsaSrvPrivsCreateAccount(
                                hProvider,
                                accessToken,
                                pAccountSid,
                                accessRights,
                                &accountContext);
        BAIL_ON_LSA_ERROR(err);
    }
    else if (err != ERROR_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(err);
    }

    pAccount = accountContext->pAccount;

    err = LsaSrvResolveNamesToPrivilegesAndSar(
                        hProvider,
                        accessToken,
                        ppwszAccountRights,
                        NumAccountRights,
                        &pPrivilegeSet,
                        &addAccessRights);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvPrivsAddPrivilegesToAccount(
                        hProvider,
                        accessToken,
                        accountContext,
                        pPrivilegeSet);
    BAIL_ON_LSA_ERROR(err);

    LSASRV_PRIVS_RDLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);
    newAccessRights = pAccount->SystemAccessRights | addAccessRights;
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    err = LsaSrvPrivsSetSystemAccessRights(
                        hProvider,
                        accessToken,
                        accountContext,
                        newAccessRights);
    BAIL_ON_LSA_ERROR(err);

error:
    if (accountContext)
    {
        LsaSrvPrivsCloseAccount(
                    hProvider,
                    &accountContext);
    }

    LW_SAFE_FREE_MEMORY(pPrivilegeSet);

    return err;
}


static
DWORD
LsaSrvResolveNamesToPrivilegesAndSar(
    IN HANDLE hProvider,
    IN PACCESS_TOKEN AccessToken,
    IN PWSTR *pAccountRights,
    IN DWORD NumAccountRights,
    OUT PPRIVILEGE_SET *ppPrivilegeSet,
    OUT PDWORD pSystemAccessRights
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD privilegeSetSize = 0;
    PPRIVILEGE_SET pPrivilegeSet = NULL;
    DWORD i = 0;
    DWORD systemAccessRights = 0;
    DWORD systemAccess = 0;
    LUID privilegeValue = {0};
    DWORD numPrivileges = 0;

    privilegeSetSize = RtlLengthRequiredPrivilegeSet(NumAccountRights);
    err = LwAllocateMemory(privilegeSetSize,
                           OUT_PPVOID(&pPrivilegeSet));
    BAIL_ON_LSA_ERROR(err);

    for (i = 0; i < NumAccountRights; i++)
    {
        err = LsaSrvPrivsLookupPrivilegeValue(
                            hProvider,
                            AccessToken,
                            pAccountRights[i],
                            &privilegeValue);
        if (err == ERROR_SUCCESS)
        {
            pPrivilegeSet->Privilege[numPrivileges++].Luid = privilegeValue;
        }
        else if (err == ERROR_NO_SUCH_PRIVILEGE)
        {
            err = LsaSrvAccountRightNameToSar(
                                pAccountRights[i],
                                &systemAccess);
            BAIL_ON_LSA_ERROR(err);

            systemAccessRights |= systemAccess;
        }
        else
        {
            BAIL_ON_LSA_ERROR(err);
        }
    }

    pPrivilegeSet->PrivilegeCount = numPrivileges;

    *ppPrivilegeSet = pPrivilegeSet;
    *pSystemAccessRights = systemAccessRights;

error:
    if (err)
    {
        LW_SAFE_FREE_MEMORY(pPrivilegeSet);

        *ppPrivilegeSet = NULL;
        *pSystemAccessRights = 0;
    }

    return err;
}


DWORD
LsaSrvPrivsRemoveAccountRights(
    IN HANDLE hProvider,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PSID AccountSid,
    IN BOOLEAN RemoveAll,
    IN PWSTR *ppwszAccountRights,
    IN DWORD NumAccountRights
    )
{
    DWORD err = ERROR_SUCCESS;
    ACCESS_MASK accessRights = LSA_ACCOUNT_ADJUST_PRIVILEGES;
    PACCESS_TOKEN accessToken = AccessToken;
    PLSA_ACCOUNT_CONTEXT accountContext = NULL;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = NULL;
    PPRIVILEGE_SET pPrivilegeSet = NULL;
    DWORD removeAccessRights = 0;
    DWORD newAccessRights = 0;

    err = LsaSrvPrivsOpenAccount(
                            hProvider,
                            accessToken,
                            AccountSid,
                            accessRights,
                            &accountContext);
    BAIL_ON_LSA_ERROR(err);

    pAccount = accountContext->pAccount;

    err = LsaSrvResolveNamesToPrivilegesAndSar(
                        hProvider,
                        accessToken,
                        ppwszAccountRights,
                        NumAccountRights,
                        &pPrivilegeSet,
                        &removeAccessRights);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvPrivsRemovePrivilegesFromAccount(
                        hProvider,
                        accessToken,
                        accountContext,
                        RemoveAll,
                        pPrivilegeSet);
    BAIL_ON_LSA_ERROR(err);

    LSASRV_PRIVS_RDLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);
    newAccessRights = pAccount->SystemAccessRights ^ removeAccessRights;
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    err = LsaSrvPrivsSetSystemAccessRights(
                        hProvider,
                        accessToken,
                        accountContext,
                        newAccessRights);
    BAIL_ON_LSA_ERROR(err);

error:
    if (accountContext)
    {
        LsaSrvPrivsCloseAccount(
                    hProvider,
                    &accountContext);
    }

    LW_SAFE_FREE_MEMORY(pPrivilegeSet);

    return err;
}


DWORD
LsaSrvPrivsEnumAccountRights(
    IN HANDLE hProvider,
    IN OPTIONAL PACCESS_TOKEN pAccessToken,
    IN PSID AccountSid,
    OUT PWSTR **ppAccountRights,
    OUT PDWORD pNumAccountRights
    )
{
    DWORD err = ERROR_SUCCESS;
    PACCESS_TOKEN accessToken = pAccessToken;
    ACCESS_MASK accessRights = LSA_ACCOUNT_VIEW;
    PLSA_ACCOUNT_CONTEXT accountContext = NULL;
    DWORD numSysAccessRights = 0;
    DWORD systemAccessRights = 0;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = NULL;
    DWORD numAccountRightNames = 0;
    PWSTR *pAccountRightNames = NULL;
    PWSTR accountRightName = NULL;
    DWORD i = 0;
    DWORD sysAccessMask = 0x00000001;

    err = LsaSrvPrivsOpenAccount(
                            hProvider,
                            accessToken,
                            AccountSid,
                            accessRights,
                            &accountContext);
    BAIL_ON_LSA_ERROR(err);

    pAccount = accountContext->pAccount;

    LSASRV_PRIVS_RDLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    systemAccessRights = pAccount->SystemAccessRights;

    // Calculate the total number of system access right bits
    // by right-shifting the access rights word bit by bit
    while (systemAccessRights)
    {
        if (systemAccessRights & sysAccessMask)
        {
            numSysAccessRights++;
        }

        systemAccessRights >>= 1;
    }

    numAccountRightNames = pAccount->NumPrivileges + numSysAccessRights;
    err = LwAllocateMemory(
                  sizeof(PWSTR) * numAccountRightNames,
                  OUT_PPVOID(&pAccountRightNames));
    BAIL_ON_LSA_ERROR(err);

    //
    // Lookup Privilege names
    //
    for (i = 0; i < pAccount->NumPrivileges; i++)
    {
        err = LsaSrvPrivsLookupPrivilegeName(
                            hProvider,
                            accessToken,
                            &pAccount->Privileges[i].Luid,
                            &accountRightName);
        BAIL_ON_LSA_ERROR(err);

        pAccountRightNames[i] = accountRightName;
    }

    //
    // Lookup System Access Right names
    //

    // Start from the least significant bit
    while (sysAccessMask &&
           pAccount->SystemAccessRights)
    {
        if (pAccount->SystemAccessRights & sysAccessMask)
        {
            err = LsaSrvSarToAccountRightName(
                              pAccount->SystemAccessRights & sysAccessMask,
                              &accountRightName);
            BAIL_ON_LSA_ERROR(err);

            pAccountRightNames[i++] = accountRightName;
        }

        sysAccessMask <<= 1;
    }

    *ppAccountRights   = pAccountRightNames;
    *pNumAccountRights = numAccountRightNames;

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    if (err)
    {
        for (i = 0; i < numAccountRightNames; i++)
        {
            LW_SAFE_FREE_MEMORY(pAccountRightNames[i]);
        }
        LW_SAFE_FREE_MEMORY(pAccountRightNames);

        *ppAccountRights   = NULL;
        *pNumAccountRights = 0;
    }

    if (accountContext)
    {
        LsaSrvPrivsCloseAccount(
                  hProvider,
                  &accountContext);
    }

    return err;
}


DWORD
LsaSrvPrivsOpenAccount(
    IN HANDLE hProvider,
    IN OPTIONAL PACCESS_TOKEN pAccessToken,
    IN PSID Sid,
    IN ACCESS_MASK AccessRights,
    OUT PLSA_ACCOUNT_CONTEXT *pAccountContext
    )
{
    DWORD err = ERROR_SUCCESS;
    PLSA_ACCOUNT pAccount = NULL;
    PLSA_ACCOUNT_CONTEXT accountContext = NULL;

    err = LsaSrvGetAccountEntry(
                     Sid,
                     &pAccount);
    BAIL_ON_LSA_ERROR(err);

    err = LwAllocateMemory(
                     sizeof(*accountContext),
                     OUT_PPVOID(&accountContext));
    BAIL_ON_LSA_ERROR(err);

    accountContext->pAccount = pAccount;

    *pAccountContext = accountContext;

error:
    if (err)
    {
        LW_SAFE_FREE_MEMORY(accountContext);

        pAccountContext = NULL;
    }

    return err;
}


DWORD
LsaSrvPrivsCreateAccount(
    IN HANDLE hProvider,
    IN OPTIONAL PACCESS_TOKEN pAccessToken,
    IN PSID Sid,
    IN ACCESS_MASK AccessRights,
    OUT PLSA_ACCOUNT_CONTEXT *pAccountContext
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSA_ACCOUNT pAccount = NULL;
    DWORD sidSize = 0;
    PSID accountSid = NULL;
    PLSA_ACCOUNT_CONTEXT accountContext = NULL;

    err = LwAllocateMemory(
                     sizeof(*pAccount),
                     OUT_PPVOID(&pAccount));
    BAIL_ON_LSA_ERROR(err);

    sidSize = RtlLengthSid(Sid);
    err = LwAllocateMemory(
                     sidSize,
                     OUT_PPVOID(&accountSid));
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlCopySid(
                     sidSize,
                     accountSid,
                     Sid);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LwAllocateMemory(
                     sidSize,
                     OUT_PPVOID(&pAccount->pSid));
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlCopySid(
                     sidSize,
                     pAccount->pSid,
                     Sid);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LsaSrvAddAccount(
                     accountSid,
                     pAccount);
    if (err)
    {
        // Adding account to the hash table failed so free it
        // to avoid memory leak
        LW_SAFE_FREE_MEMORY(accountSid);
        LW_SAFE_FREE_MEMORY(pAccount->pSid);
        LW_SAFE_FREE_MEMORY(pAccount);

        BAIL_ON_LSA_ERROR(err);
    }

    err = LwAllocateMemory(
                     sizeof(*accountContext),
                     OUT_PPVOID(&accountContext));
    BAIL_ON_LSA_ERROR(err);

    accountContext->pAccount = pAccount;

    *pAccountContext = accountContext;

error:
    if (err)
    {
        LW_SAFE_FREE_MEMORY(accountContext);

        pAccountContext = NULL;
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


VOID
LsaSrvPrivsCloseAccount(
    IN HANDLE hProvider,
    IN OUT PLSA_ACCOUNT_CONTEXT *pAccountContext
    )
{
    PLSA_ACCOUNT_CONTEXT accountContext = *pAccountContext;

    accountContext->pAccount = NULL;
    LW_SAFE_FREE_MEMORY(accountContext);

    *pAccountContext = NULL;
}


DWORD
LsaSrvPrivsAddPrivilegesToAccount(
    IN HANDLE hProvider,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN PPRIVILEGE_SET pPrivilegeSet
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = pAccountContext->pAccount;
    DWORD numPrivileges = 0;

    LSASRV_PRIVS_WRLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    err = LsaSrvMergePrivilegeLists(
                  pAccount->Privileges,
                  pAccount->NumPrivileges,
                  pPrivilegeSet->Privilege,
                  pPrivilegeSet->PrivilegeCount,
                  sizeof(pAccount->Privileges)/sizeof(pAccount->Privileges[0]),
                  NULL,
                  &numPrivileges);
    BAIL_ON_LSA_ERROR(err);

    pAccountContext->Dirty = (numPrivileges > pAccount->NumPrivileges);
    pAccount->NumPrivileges = numPrivileges;

    if (pAccountContext->Dirty)
    {
        err = LsaSrvUpdateAccount(pAccount->pSid,
                                  pAccount);
        BAIL_ON_LSA_ERROR(err);

        pAccountContext->Dirty = FALSE;
    }
                   
error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    return err;
}


static
DWORD
LsaSrvMergePrivilegeLists(
    IN PLUID_AND_ATTRIBUTES pPrivs1,
    IN DWORD NumPrivs1,
    IN PLUID_AND_ATTRIBUTES pPrivs2,
    IN DWORD NumPrivs2,
    IN DWORD RetPrivsLen,
    IN OUT OPTIONAL PLUID_AND_ATTRIBUTES pRetPrivs,
    OUT PDWORD pNumRetPrivs
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD i = 0;
    DWORD j = 0;
    BOOLEAN found = FALSE;
    PLUID_AND_ATTRIBUTES pMergedPrivs = NULL;
    DWORD numMergedPrivs = 0;

    if (pRetPrivs &&
        pPrivs1 != pRetPrivs)
    {
        pMergedPrivs = pRetPrivs;

        if (NumPrivs1 > RetPrivsLen)
        {
            err = ERROR_INSUFFICIENT_BUFFER;
            BAIL_ON_LSA_ERROR(err);
        }

        memcpy(pMergedPrivs, pPrivs1, sizeof(*pMergedPrivs)* NumPrivs1);
    }
    else
    {
        pMergedPrivs = pPrivs1;
    }

    numMergedPrivs = NumPrivs1;

    for (i = 0; i < NumPrivs2; i++)
    {
        for (j = 0; !found && j < numMergedPrivs; j++)
        {
            found = RtlEqualLuid(
                         &pPrivs2[i].Luid,
                         &pMergedPrivs[j].Luid);
        }

        if (!found)
        {
            if (numMergedPrivs >= RetPrivsLen)
            {
                err = ERROR_INSUFFICIENT_BUFFER;
                BAIL_ON_LSA_ERROR(err);
            }

            pMergedPrivs[numMergedPrivs++].Luid = pPrivs2[i].Luid;
        }
    }

    *pNumRetPrivs = numMergedPrivs;
                   
error:
    if (err)
    {
        *pNumRetPrivs = 0;
    }

    return err;
}


DWORD
LsaSrvPrivsRemovePrivilegesFromAccount(
    IN HANDLE hProvider,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN BOOLEAN RemoveAll,
    IN PPRIVILEGE_SET pPrivilegeSet
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = pAccountContext->pAccount;
    DWORD i = 0;
    DWORD j = 0;
    DWORD offset = 0;
    PLUID_AND_ATTRIBUTES pPrivileges = pAccount->Privileges;
    BOOLEAN found = FALSE;

    LSASRV_PRIVS_WRLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    for (i = 0; !RemoveAll && i < pPrivilegeSet->PrivilegeCount; i++)
    {
        for (j = 0; j < pAccount->NumPrivileges; j++)
        {
            if (RtlEqualLuid(&pPrivilegeSet->Privilege[i].Luid,
                             &pAccount->Privileges[j].Luid))
            {
                offset++; 
                found = TRUE;
                pAccountContext->Dirty = TRUE;
            }

            if (found && (j + offset < pAccount->NumPrivileges))
            {
                pPrivileges[j].Luid = pAccount->Privileges[j + offset].Luid;
                pPrivileges[j].Attributes
                                  = pAccount->Privileges[j + offset].Attributes;

                pAccount->NumPrivileges--;
            }
            else if (found && (j + offset >= pAccount->NumPrivileges))
            {
                LUID emptyLuid = {0,0};

                pPrivileges[j].Luid       = emptyLuid;
                pPrivileges[j].Attributes = 0;

                pAccount->NumPrivileges--;
            }
        }
    }

    if (pAccountContext->Dirty)
    {
        err = LsaSrvUpdateAccount(pAccount->pSid,
                                  pAccount);
        BAIL_ON_LSA_ERROR(err);
                   
        pAccountContext->Dirty = FALSE;
    }
                   
error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    return err;
}


DWORD
LsaSrvPrivsGetSystemAccessRights(
    IN HANDLE hProvider,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    OUT PDWORD pSystemAccessRights
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = pAccountContext->pAccount;

    LSASRV_PRIVS_RDLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    *pSystemAccessRights = pAccount->SystemAccessRights;

    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    return err;
}


DWORD
LsaSrvPrivsSetSystemAccessRights(
    IN HANDLE hProvider,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN DWORD SystemAccessRights
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = pAccountContext->pAccount;

    LSASRV_PRIVS_WRLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    if (pAccount->SystemAccessRights != SystemAccessRights)
    {
        pAccount->SystemAccessRights = SystemAccessRights;
        pAccountContext->Dirty = TRUE;
    }

    if (pAccountContext->Dirty)
    {
        err = LsaSrvUpdateAccount(pAccount->pSid,
                                  pAccount);
        BAIL_ON_LSA_ERROR(err);

        pAccountContext->Dirty = FALSE;
    }
                   
error:
    LSASRV_PRIVS_WRLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    return err;
}


static
DWORD
LsaSrvAccountRightNameToSar(
    IN PWSTR AccessRightName,
    OUT PDWORD pSystemAccessRight
    )
{
    return LsaSrvLookupSysAccessRightNameMap(
                    pSystemAccessRight,
                    &AccessRightName,
                    TRUE);
}


static
DWORD
LsaSrvSarToAccountRightName(
    IN DWORD SystemAccessRight,
    OUT PWSTR *pAccessRightName
    )
{
    return LsaSrvLookupSysAccessRightNameMap(
                    &SystemAccessRight,
                    pAccessRightName,
                    FALSE);
}


static
DWORD
LsaSrvLookupSysAccessRightNameMap(
    IN OUT PDWORD pSystemAccessRight,
    IN OUT PWSTR* pAccessRightName,
    IN BOOLEAN ResolveName
    )
{
    DWORD err = ERROR_SUCCESS;
    WCHAR sarInteractiveLogonName[] = SE_INTERACTIVE_LOGON_NAME;
    WCHAR sarNetworkLogonName[] = SE_NETWORK_LOGON_NAME;
    WCHAR sarBatchLogonName[] = SE_BATCH_LOGON_NAME;
    WCHAR sarServiceLogonName[] = SE_SERVICE_LOGON_NAME;
    WCHAR sarDenyInteractiveLogonName[] = SE_DENY_INTERACTIVE_LOGON_NAME;
    WCHAR sarDenyNetworkLogonName[] = SE_DENY_INTERACTIVE_LOGON_NAME;
    WCHAR sarDenyBatchLogonName[] = SE_DENY_INTERACTIVE_LOGON_NAME;
    WCHAR sarDenyServiceLogonName[] = SE_DENY_SERVICE_LOGON_NAME;
    WCHAR sarRemoteInteractiveLogonName[] = SE_REMOTE_INTERACTIVE_LOGON_NAME;
    WCHAR sarDenyRemoteInteractiveLogonName[] = 
                                         SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME;
    struct _ACCESS_RIGHT_NAME_MAP {
        PWSTR Name;
        DWORD Flag;
    } accessRightNameMap[] = {
        { sarInteractiveLogonName, POLICY_MODE_INTERACTIVE },
        { sarNetworkLogonName, POLICY_MODE_NETWORK },
        { sarBatchLogonName, POLICY_MODE_BATCH },
        { sarServiceLogonName, POLICY_MODE_SERVICE },
        { sarDenyInteractiveLogonName, POLICY_MODE_DENY_INTERACTIVE },
        { sarDenyNetworkLogonName, POLICY_MODE_DENY_NETWORK },
        { sarDenyBatchLogonName, POLICY_MODE_DENY_BATCH },
        { sarDenyServiceLogonName, POLICY_MODE_DENY_SERVICE },
        { sarRemoteInteractiveLogonName, POLICY_MODE_REMOTE_INTERACTIVE },
        { sarDenyRemoteInteractiveLogonName,
                                         POLICY_MODE_DENY_REMOTE_INTERACTIVE },
    };
    DWORD i = 0;
    PWSTR accessRightName = NULL;
    DWORD systemAccessRight = 0;

    for (i = 0;
         i < (sizeof(accessRightNameMap)/sizeof(accessRightNameMap[0]));
         i++)
    {
        if (ResolveName)
        {
            if (RtlWC16StringIsEqual(
                        *pAccessRightName,
                        accessRightNameMap[i].Name,
                        TRUE))
            {
                systemAccessRight = accessRightNameMap[i].Flag;
                break;
            }
        }
        else /* Resolve a Flag */
        {
            if (*pSystemAccessRight == accessRightNameMap[i].Flag)
            {
                err = LwAllocateWc16String(
                            &accessRightName,
                            accessRightNameMap[i].Name);
                BAIL_ON_LSA_ERROR(err);
                break;
            }
        }

    }

    if (accessRightName == NULL && systemAccessRight == 0)
    {
        // Maps to STATUS_NO_SUCH_PRIVILEGE
        err = ERROR_NOT_FOUND;
    }

    if (ResolveName)
    {
        *pSystemAccessRight = systemAccessRight;
    }
    else /* Resolve a Flag */
    {
        *pAccessRightName = accessRightName;
    }

error:
    if (err)
    {
        LW_SAFE_FREE_MEMORY(accessRightName);
    }

    return err;
}
