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
    IN HANDLE hServer,
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
LsaSrvRemovePrivilegesFromPrivilegeList(
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

static
BOOLEAN
LsaSrvIsPrivilegeAssigned(
    IN PLSA_ACCOUNT pAccount,
    IN PLSA_PRIVILEGE pPrivilege
    );

static
BOOLEAN
LsaSrvIsSarAssigned(
    IN PLSA_ACCOUNT pAccount,
    IN DWORD SystemAccessRight
    );

static
VOID
LsaSrvPrivsMarkAccountEntryDeleted_inlock(
    PLSA_ACCOUNT pAccount
    );


DWORD
LsaSrvPrivsEnumAccountRightsSids(
    IN HANDLE hServer,
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
    DWORD systemAccessRights = 0;
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
                                hServer,
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

        systemAccessRights |= pAccount->SystemAccessRights;

        err = LsaSrvMergePrivilegeLists(
                                pPrivileges,
                                numPrivileges,
                                pAccount->Privileges,
                                pAccount->NumPrivileges,
                                privilegesLen,
                                pPrivileges,
                                &numPrivileges);
        // Error check goes after unlocking account to prevent from leaving
        // account locked if we would have bailed after error code had been
        // returned
        LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);
        BAIL_ON_LSA_ERROR(err);

        LsaSrvPrivsCloseAccount(&accountContext);
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

    *pNumPrivileges      = numPrivileges;
    *ppPrivileges        = (numPrivileges) ? pPrivileges : NULL;
    *pSystemAccessRights = systemAccessRights;

error:
    if (err || ntStatus)
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

        if (pSystemAccessRights)
        {
            *pSystemAccessRights = 0;
        }
    }

    if (accountContext)
    {
        LsaSrvPrivsCloseAccount(
                  &accountContext);
    }

    RTL_FREE(&accountSid);

    if (numPrivileges == 0)
    {
        LW_SAFE_FREE_MEMORY(pPrivileges);
    }

    return err;
}


DWORD
LsaSrvPrivsAddAccountRights(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PSID pAccountSid,
    IN PWSTR *ppwszAccountRights,
    IN DWORD NumAccountRights
    )
{
    DWORD err = ERROR_SUCCESS;
    ACCESS_MASK accessRights = LSA_ACCOUNT_ADJUST_PRIVILEGES;
    PACCESS_TOKEN accessToken = AccessToken;
    PLSA_ACCOUNT_CONTEXT pAccountContext = NULL;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = NULL;
    PPRIVILEGE_SET pPrivilegeSet = NULL;
    DWORD newNumPrivileges = 0;
    DWORD addAccessRights = 0;
    DWORD newAccessRights = 0;

    err = LsaSrvPrivsOpenAccount(
                            hServer,
                            accessToken,
                            pAccountSid,
                            accessRights,
                            &pAccountContext);
    if (err == ERROR_FILE_NOT_FOUND)
    {
        err = LsaSrvPrivsCreateAccount(
                                hServer,
                                accessToken,
                                pAccountSid,
                                accessRights,
                                &pAccountContext);
        BAIL_ON_LSA_ERROR(err);
    }
    else if (err != ERROR_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(err);
    }

    err = LsaSrvResolveNamesToPrivilegesAndSar(
                        hServer,
                        accessToken,
                        ppwszAccountRights,
                        NumAccountRights,
                        &pPrivilegeSet,
                        &addAccessRights);
    BAIL_ON_LSA_ERROR(err);

    pAccount = pAccountContext->pAccount;
    LSASRV_PRIVS_WRLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    // Add Privileges
    err = LsaSrvMergePrivilegeLists(
                  pAccount->Privileges,
                  pAccount->NumPrivileges,
                  pPrivilegeSet->Privilege,
                  pPrivilegeSet->PrivilegeCount,
                  sizeof(pAccount->Privileges)/sizeof(pAccount->Privileges[0]),
                  NULL,
                  &newNumPrivileges);
    BAIL_ON_LSA_ERROR(err);

    pAccountContext->Dirty = (newNumPrivileges > pAccount->NumPrivileges);
    pAccount->NumPrivileges = newNumPrivileges;

    // Add System Access Rights
    newAccessRights = pAccount->SystemAccessRights | addAccessRights;
    pAccountContext->Dirty |= (newAccessRights != pAccount->SystemAccessRights);

    pAccount->SystemAccessRights = newAccessRights;

    // Save the changes (if there were any)
    if (pAccountContext->Dirty)
    {
        err = LsaSrvUpdateAccount_inlock(
                      pAccount->pSid,
                      pAccount);
        BAIL_ON_LSA_ERROR(err);

        pAccountContext->Dirty = FALSE;
    }

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    if (pAccountContext)
    {
        LsaSrvPrivsCloseAccount(
                    &pAccountContext);
    }

    LW_SAFE_FREE_MEMORY(pPrivilegeSet);

    return err;
}


static
DWORD
LsaSrvResolveNamesToPrivilegesAndSar(
    IN HANDLE hServer,
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
                            hServer,
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
    IN OPTIONAL HANDLE hServer,
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
    PLSA_ACCOUNT_CONTEXT pAccountContext = NULL;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = NULL;
    PPRIVILEGE_SET pPrivilegeSet = NULL;
    DWORD newNumPrivileges = 0;
    DWORD removeAccessRights = 0;
    DWORD newAccessRights = 0;

    err = LsaSrvPrivsOpenAccount(
                            hServer,
                            accessToken,
                            AccountSid,
                            accessRights,
                            &pAccountContext);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvResolveNamesToPrivilegesAndSar(
                        hServer,
                        accessToken,
                        ppwszAccountRights,
                        NumAccountRights,
                        &pPrivilegeSet,
                        &removeAccessRights);
    BAIL_ON_LSA_ERROR(err);

    pAccount = pAccountContext->pAccount;
    LSASRV_PRIVS_WRLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    if (RemoveAll)
    {
        // Remove everything
        memset(pAccount->Privileges, 0, sizeof(pAccount->Privileges));
        newNumPrivileges = 0;

        removeAccessRights = pAccount->SystemAccessRights;
    }
    else
    {
        // Remove Privileges
        err = LsaSrvRemovePrivilegesFromPrivilegeList(
                            pAccount->Privileges,
                            pAccount->NumPrivileges,
                            pPrivilegeSet->Privilege,
                            pPrivilegeSet->PrivilegeCount,
                            (sizeof(pAccount->Privileges)/
                             sizeof(pAccount->Privileges[0])),
                            NULL,
                            &newNumPrivileges);
        BAIL_ON_LSA_ERROR(err);
    }

    pAccountContext->Dirty = (newNumPrivileges < pAccount->NumPrivileges);
    pAccount->NumPrivileges = newNumPrivileges;

    // Remove System Access Rights
    newAccessRights = pAccount->SystemAccessRights ^ removeAccessRights;
    pAccountContext->Dirty |= (newAccessRights != pAccount->SystemAccessRights);

    pAccount->SystemAccessRights = newAccessRights;

    // Save the changes (if there were any)
    if (pAccountContext->Dirty)
    {
        err = LsaSrvUpdateAccount_inlock(
                      pAccount->pSid,
                      pAccount);
        BAIL_ON_LSA_ERROR(err);

        pAccountContext->Dirty = FALSE;
    }

    // Delete the account if it has no account rights assigned any longer
    if (pAccount->SystemAccessRights == 0 &&
        pAccount->NumPrivileges == 0)
    {
        LsaSrvPrivsMarkAccountEntryDeleted_inlock(pAccount);
    }

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    if (pAccountContext)
    {
        LsaSrvPrivsCloseAccount(
                    &pAccountContext);
    }

    LW_SAFE_FREE_MEMORY(pPrivilegeSet);

    return err;
}


DWORD
LsaSrvPrivsEnumAccountRights(
    IN OPTIONAL HANDLE hServer,
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
                            hServer,
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
                            hServer,
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
                  &accountContext);
    }

    return err;
}


DWORD
LsaSrvPrivsOpenAccount(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN pAccessToken,
    IN PSID Sid,
    IN ACCESS_MASK AccessRights,
    OUT PLSA_ACCOUNT_CONTEXT *pAccountContext
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = pAccessToken;
    BOOLEAN releaseAccessToken = FALSE;
    GENERIC_MAPPING genericMapping = {0};
    ACCESS_MASK grantedAccess = 0;
    PLSA_ACCOUNT pAccount = NULL;
    PLSA_ACCOUNT_CONTEXT accountContext = NULL;

    if (!accessToken)
    {
        err = LsaSrvPrivsGetAccessTokenFromServerHandle(
                                hServer,
                                &accessToken);
        BAIL_ON_LSA_ERROR(err);

        releaseAccessToken = TRUE;
    }

    err = LsaSrvGetAccountEntry(
                     Sid,
                     &pAccount);
    BAIL_ON_LSA_ERROR(err);

    if (!RtlAccessCheck(pAccount->pSecurityDesc,
                        accessToken,
                        AccessRights,
                        grantedAccess,
                        &genericMapping,
                        &grantedAccess,
                        &ntStatus))
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }
                       
    err = LwAllocateMemory(
                     sizeof(*accountContext),
                     OUT_PPVOID(&accountContext));
    BAIL_ON_LSA_ERROR(err);

    accountContext->pAccount = pAccount;
    accountContext->accessToken = accessToken;
    accountContext->releaseAccessToken = releaseAccessToken;
    accountContext->grantedAccess = grantedAccess;

    *pAccountContext = accountContext;

error:
    if (err || ntStatus)
    {
        LsaSrvPrivsCloseAccount(&accountContext);

        *pAccountContext = NULL;
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


DWORD
LsaSrvPrivsCreateAccount(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN pAccessToken,
    IN PSID Sid,
    IN ACCESS_MASK AccessRights,
    OUT PLSA_ACCOUNT_CONTEXT *pAccountContext
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PACCESS_TOKEN accessToken = pAccessToken;
    BOOLEAN releaseAccessToken = FALSE;
    GENERIC_MAPPING genericMapping = {0};
    ACCESS_MASK grantedAccess = 0;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PLSA_ACCOUNT pAccount = NULL;
    DWORD sidSize = 0;
    PSID accountSid = NULL;
    PLSA_ACCOUNT_CONTEXT accountContext = NULL;

    if (!accessToken)
    {
        err = LsaSrvPrivsGetAccessTokenFromServerHandle(
                                hServer,
                                &accessToken);
        BAIL_ON_LSA_ERROR(err);

        releaseAccessToken = TRUE;
    }

    err = LwAllocateMemory(
                     sizeof(*pAccount),
                     OUT_PPVOID(&pAccount));
    BAIL_ON_LSA_ERROR(err);

    pthread_rwlock_init(&pAccount->accountRwLock, NULL);
    pAccount->Refcount = 1;

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

    err = LsaSrvAllocateAbsoluteFromSelfRelativeSD(
                     pGlobals->pAccountsSecDescRelative,
                     &pAccount->pSecurityDesc);
    BAIL_ON_LSA_ERROR(err);

    if (!RtlAccessCheck(pAccount->pSecurityDesc,
                        accessToken,
                        AccessRights,
                        grantedAccess,
                        &genericMapping,
                        &grantedAccess,
                        &ntStatus))
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }
                       
    err = LsaSrvAddAccount_inlock(
                     accountSid,
                     pAccount);
    if (err)
    {
        // Adding account to the hash table failed so free it
        // to avoid memory leak
        LW_SAFE_FREE_MEMORY(accountSid);
        LW_SAFE_FREE_MEMORY(pAccount->pSid);
        LsaSrvFreeSecurityDescriptor(pAccount->pSecurityDesc);
        LW_SAFE_FREE_MEMORY(pAccount);

        BAIL_ON_LSA_ERROR(err);
    }

    err = LwAllocateMemory(
                     sizeof(*accountContext),
                     OUT_PPVOID(&accountContext));
    BAIL_ON_LSA_ERROR(err);

    accountContext->grantedAccess = grantedAccess;
    accountContext->pAccount = pAccount;
    accountContext->accessToken = accessToken;
    accountContext->releaseAccessToken = releaseAccessToken;

    *pAccountContext = accountContext;

error:
    if (err || ntStatus)
    {
        LsaSrvPrivsCloseAccount(&accountContext);

        *pAccountContext = NULL;
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
    IN OUT PLSA_ACCOUNT_CONTEXT *pAccountContext
    )
{
    PLSA_ACCOUNT_CONTEXT accountContext = *pAccountContext;

    if (accountContext == NULL) return;

    LsaSrvPrivsReleaseAccountEntry(accountContext->pAccount);
    LsaSrvPrivsCheckIfDeleteAccount(accountContext->pAccount);

    accountContext->pAccount = NULL;

    if (accountContext->releaseAccessToken)
    {
        RtlReleaseAccessToken(&accountContext->accessToken);
    }

    LW_SAFE_FREE_MEMORY(accountContext);

    *pAccountContext = NULL;
}


DWORD
LsaSrvPrivsAddPrivilegesToAccount(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN PPRIVILEGE_SET pPrivilegeSet
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = pAccountContext->pAccount;
    DWORD numPrivileges = 0;

    if (!(pAccountContext->grantedAccess & LSA_ACCOUNT_ADJUST_PRIVILEGES))
    {
        err = ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(err);
    }

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
        err = LsaSrvUpdateAccount_inlock(
                      pAccount->pSid,
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

            pMergedPrivs[numMergedPrivs].Luid = pPrivs2[i].Luid;
            pMergedPrivs[numMergedPrivs++].Attributes = pPrivs2[i].Attributes;
        }

        found = FALSE;
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
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN BOOLEAN RemoveAll,
    IN PPRIVILEGE_SET pPrivilegeSet
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = pAccountContext->pAccount;
    DWORD newNumPrivileges = 0;

    if (!(pAccountContext->grantedAccess & LSA_ACCOUNT_ADJUST_PRIVILEGES))
    {
        err = ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(err);
    }

    LSASRV_PRIVS_WRLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    // Remove Privileges
    if (RemoveAll)
    {
        memset(pAccount->Privileges, 0, sizeof(pAccount->Privileges));
        newNumPrivileges = 0;
    }
    else
    {
        err = LsaSrvRemovePrivilegesFromPrivilegeList(
                      pAccount->Privileges,
                      pAccount->NumPrivileges,
                      pPrivilegeSet->Privilege,
                      pPrivilegeSet->PrivilegeCount,
                      (sizeof(pAccount->Privileges)/
                       sizeof(pAccount->Privileges[0])),
                      NULL,
                      &newNumPrivileges);
        BAIL_ON_LSA_ERROR(err);
    }

    pAccountContext->Dirty = (newNumPrivileges < pAccount->NumPrivileges);
    pAccount->NumPrivileges = newNumPrivileges;

    if (pAccountContext->Dirty)
    {
        err = LsaSrvUpdateAccount_inlock(
                      pAccount->pSid,
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
LsaSrvRemovePrivilegesFromPrivilegeList(
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
    DWORD offset = 0;
    DWORD removed = 0;
    PLUID_AND_ATTRIBUTES pDestPrivs = NULL;
    DWORD numDestPrivs = 0;

    if (pRetPrivs &&
        pPrivs1 != pRetPrivs)
    {
        pDestPrivs = pRetPrivs;

        if (NumPrivs1 > RetPrivsLen)
        {
            err = ERROR_INSUFFICIENT_BUFFER;
            BAIL_ON_LSA_ERROR(err);
        }

        memcpy(pDestPrivs, pPrivs1, sizeof(*pDestPrivs)* NumPrivs1);
    }
    else
    {
        pDestPrivs = pPrivs1;
    }

    numDestPrivs = NumPrivs1;

    for (i = 0; i < NumPrivs2; i++)
    {
        for (j = 0; j < numDestPrivs; j++)
        {
            if (RtlEqualLuid(
                     &pPrivs2[i].Luid,
                     &pDestPrivs[j].Luid))
            {
                offset++;
            }

            if (offset && (j + offset < RetPrivsLen - 1))
            {
                pDestPrivs[j].Luid = pDestPrivs[j + offset].Luid;
                pDestPrivs[j].Attributes = pDestPrivs[j + offset].Attributes;
            }
            else if (offset && (j + offset == RetPrivsLen - 1))
            {
                LUID emptyLuid = {0,0};

                pDestPrivs[j].Luid       = emptyLuid;
                pDestPrivs[j].Attributes = 0;
            }
        }

        removed += offset;
        offset   = 0;
    }

    *pNumRetPrivs = NumPrivs1 - removed;

error:
    if (err)
    {
        *pNumRetPrivs = 0;
    }

    return err;
}


DWORD
LsaSrvPrivsEnumAccountPrivileges(
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    OUT PPRIVILEGE_SET *ppPrivileges
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = pAccountContext->pAccount;
    DWORD numPrivileges = 0;
    DWORD privilegesSize = 0;
    PPRIVILEGE_SET pPrivileges = NULL;

    if (!(pAccountContext->grantedAccess & LSA_ACCOUNT_VIEW))
    {
        err = ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(err);
    }

    LSASRV_PRIVS_RDLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    numPrivileges  = pAccount->NumPrivileges;
    privilegesSize = RtlLengthRequiredPrivilegeSet(numPrivileges);

    err = LwAllocateMemory(privilegesSize,
                           OUT_PPVOID(&pPrivileges));
    BAIL_ON_LSA_ERROR(err);

    pPrivileges->PrivilegeCount = numPrivileges;
    memcpy(pPrivileges->Privilege,
           pAccount->Privileges,
           sizeof(pPrivileges->Privilege[0]) * numPrivileges);

    *ppPrivileges = pPrivileges;

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    if (err)
    {
        LW_SAFE_FREE_MEMORY(pPrivileges);

        if (ppPrivileges)
        {
            *ppPrivileges = NULL;
        }
    }

    return err;
}


DWORD
LsaSrvPrivsGetSystemAccessRights(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    OUT PDWORD pSystemAccessRights
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = pAccountContext->pAccount;

    if (!(pAccountContext->grantedAccess & LSA_ACCOUNT_VIEW))
    {
        err = ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(err);
    }

    LSASRV_PRIVS_RDLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    *pSystemAccessRights = pAccount->SystemAccessRights;

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    return err;
}


DWORD
LsaSrvPrivsSetSystemAccessRights(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN DWORD SystemAccessRights
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = pAccountContext->pAccount;

    if (!(pAccountContext->grantedAccess & LSA_ACCOUNT_ADJUST_SYSTEM_ACCESS))
    {
        err = ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(err);
    }

    LSASRV_PRIVS_WRLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    if (pAccount->SystemAccessRights != SystemAccessRights)
    {
        pAccount->SystemAccessRights = SystemAccessRights;
        pAccountContext->Dirty = TRUE;
    }

    if (pAccountContext->Dirty)
    {
        err = LsaSrvUpdateAccount_inlock(
                      pAccount->pSid,
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
        err = ERROR_NO_SUCH_PRIVILEGE;
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


DWORD
LsaSrvPrivsEnumAccounts(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PDWORD pResume,
    IN DWORD PreferredMaxSize,
    OUT PSID **ppAccountSids,
    OUT PDWORD pCount
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD enumerationStatus = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PACCESS_TOKEN accessToken = AccessToken;
    ACCESS_MASK accessRights = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                               LSA_ACCESS_VIEW_POLICY_INFO;
    ACCESS_MASK grantedAccess = 0;
    GENERIC_MAPPING genericMapping = {0};
    DWORD resume = 0;
    PLSA_ACCOUNT *ppAccounts = NULL;
    DWORD count = 0;
    DWORD i = 0;
    PSID *pAccountSids = NULL;

    if (!accessToken)
    {
        err = LsaSrvPrivsGetAccessTokenFromServerHandle(
                                hServer,
                                &accessToken);
        BAIL_ON_LSA_ERROR(err);
    }

    if (!RtlAccessCheck(pGlobals->pPrivilegesSecDesc,
                        accessToken,
                        accessRights,
                        0,
                        &genericMapping,
                        &grantedAccess,
                        &ntStatus))
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pResume)
    {
        resume = *pResume;
    }

    err = LsaSrvPrivsGetAccountEntries(
                        &resume,
                        PreferredMaxSize,
                        &ppAccounts,
                        &count);
    if (err == ERROR_MORE_DATA ||
        err == ERROR_NO_MORE_ITEMS)
    {
        enumerationStatus = err;
        err = ERROR_SUCCESS;
    }
    else if (err != ERROR_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(err);
    }

    err = LwAllocateMemory(
                        sizeof(pAccountSids[0]) * count,
                        OUT_PPVOID(&pAccountSids));
    BAIL_ON_LSA_ERROR(err);

    for (i = 0; i < count; i++)
    {
        DWORD sidSize = RtlLengthSid(ppAccounts[i]->pSid);

        err = LwAllocateMemory(
                            sidSize,
                            OUT_PPVOID(&pAccountSids[i]));
        BAIL_ON_LSA_ERROR(err);

        ntStatus = RtlCopySid(sidSize,
                              pAccountSids[i],
                              ppAccounts[i]->pSid);
        if (ntStatus)
        {
            err = LwNtStatusToWin32Error(ntStatus);
            BAIL_ON_LSA_ERROR(err);
        }
    }

    if (pResume)
    {
        *pResume = resume;
    }

    *ppAccountSids  = pAccountSids;
    *pCount         = count;

error:
    if (err || ntStatus)
    {
        for (i = 0; i < count; i++)
        {
            LW_SAFE_FREE_MEMORY(pAccountSids[i]);
        }
        LW_SAFE_FREE_MEMORY(pAccountSids);

        if (ppAccountSids)
        {
            *ppAccountSids = NULL;
        }

        if (pCount)
        {
            *pCount = 0;
        }
    }

    for (i = 0; i < count; i++)
    {
        LsaSrvPrivsReleaseAccountEntry(ppAccounts[i]);
    }
    LW_SAFE_FREE_MEMORY(ppAccounts);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    if (err == ERROR_SUCCESS)
    {
        err = enumerationStatus;
    }

    return err;
}


DWORD
LsaSrvPrivsEnumAccountsWithUserRight(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PWSTR UserRight,
    OUT PSID **ppAccountSids,
    OUT PDWORD pCount
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PACCESS_TOKEN accessToken = AccessToken;
    BOOLEAN releaseAccessToken = FALSE;
    ACCESS_MASK accessRights = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                               LSA_ACCESS_VIEW_POLICY_INFO;
    ACCESS_MASK grantedAccess = 0;
    GENERIC_MAPPING genericMapping = {0};
    PSTR pszAccountRight = NULL;
    PLSA_PRIVILEGE pPrivilege = NULL;
    DWORD systemAccessRight = 0;
    DWORD resume = 0;
    DWORD preferredMaxSize = (DWORD)(-1);
    PLSA_ACCOUNT *ppAccounts = NULL;
    DWORD numAccounts = 0;
    DWORD count = 0;
    DWORD i = 0;
    PSID *pAccountSids = NULL;

    if (!accessToken)
    {
        err = LsaSrvPrivsGetAccessTokenFromServerHandle(
                                hServer,
                                &accessToken);
        BAIL_ON_LSA_ERROR(err);

        releaseAccessToken = TRUE;
    }

    if (!RtlAccessCheck(pGlobals->pPrivilegesSecDesc,
                        accessToken,
                        accessRights,
                        0,
                        &genericMapping,
                        &grantedAccess,
                        &ntStatus))
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    //
    // Find out if UserRight is a privilege or system access right name
    //
    err = LwWc16sToMbs(UserRight,
                       &pszAccountRight);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvGetPrivilegeEntryByName(
                        pszAccountRight,
                        &pPrivilege);
    if (err == ERROR_NO_SUCH_PRIVILEGE)
    {
        err = LsaSrvAccountRightNameToSar(
                            UserRight,
                            &systemAccessRight);
    }
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvPrivsGetAccountEntries(
                        &resume,
                        preferredMaxSize,
                        &ppAccounts,
                        &numAccounts);
    BAIL_ON_LSA_ERROR(err);

    //
    // Count the number of accounts to return
    //
    for (i = 0; i < numAccounts; i++)
    {
        if ((pPrivilege &&
             LsaSrvIsPrivilegeAssigned(
                            ppAccounts[i],
                            pPrivilege)) ||
            (systemAccessRight &&
             LsaSrvIsSarAssigned(
                            ppAccounts[i],
                            systemAccessRight)))
        {
            count++;
        }
    }

    err = LwAllocateMemory(
                        sizeof(pAccountSids[0]) * count,
                        OUT_PPVOID(&pAccountSids));
    BAIL_ON_LSA_ERROR(err);

    count = 0;

    //
    // Copy the account SIDs to return
    //
    for (i = 0; i < numAccounts; i++)
    {
        DWORD sidSize = 0; 

        if ((pPrivilege &&
             LsaSrvIsPrivilegeAssigned(
                            ppAccounts[i],
                            pPrivilege)) ||
            (systemAccessRight &&
             LsaSrvIsSarAssigned(
                            ppAccounts[i],
                            systemAccessRight)))
        {
            sidSize = RtlLengthSid(ppAccounts[i]->pSid);

            err = LwAllocateMemory(
                                sidSize,
                                OUT_PPVOID(&pAccountSids[i]));
            BAIL_ON_LSA_ERROR(err);

            ntStatus = RtlCopySid(
                                sidSize,
                                pAccountSids[count++],
                                ppAccounts[i]->pSid);
            if (ntStatus)
            {
                err = LwNtStatusToWin32Error(ntStatus);
                BAIL_ON_LSA_ERROR(err);
            }
        }
    }

    *ppAccountSids  = pAccountSids;
    *pCount         = count;

error:
    if (err || ntStatus)
    {
        for (i = 0; i < count; i++)
        {
            LW_SAFE_FREE_MEMORY(pAccountSids[i]);
        }
        LW_SAFE_FREE_MEMORY(pAccountSids);

        if (ppAccountSids)
        {
            *ppAccountSids = NULL;
        }

        if (pCount)
        {
            *pCount = 0;
        }
    }

    if (releaseAccessToken)
    {
        RtlReleaseAccessToken(&accessToken);
    }

    for (i = 0; i < numAccounts; i++)
    {
        LsaSrvPrivsReleaseAccountEntry(ppAccounts[i]);
    }
    LW_SAFE_FREE_MEMORY(ppAccounts);

    LW_SAFE_FREE_MEMORY(pszAccountRight);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
BOOLEAN
LsaSrvIsPrivilegeAssigned(
    IN PLSA_ACCOUNT pAccount,
    IN PLSA_PRIVILEGE pPrivilege
    )
{
    DWORD i = 0;

    for (i = 0; i < pAccount->NumPrivileges; i++)
    {
        if (RtlEqualLuid(&pAccount->Privileges[i].Luid,
                         &pPrivilege->Luid))
        {
            return TRUE;
        }
    }

    return FALSE;
}


static
BOOLEAN
LsaSrvIsSarAssigned(
    IN PLSA_ACCOUNT pAccount,
    IN DWORD SystemAccessRight
    )
{
    return (pAccount->SystemAccessRights & SystemAccessRight);
}


DWORD
LsaSrvPrivsGetAccountSecurity(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR_RELATIVE *ppSecurityDescRelative,
    OUT PDWORD pSecurityDescRelativeSize
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = pAccountContext->pAccount;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDesc = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescRelative = NULL;
    DWORD securityDescRelativeSize = 0;

    if (!(pAccountContext->grantedAccess & (LSA_ACCOUNT_VIEW |
                                            READ_CONTROL)))
    {
        err = ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(err);
    }

    err = LwAllocateMemory(
                        SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
                        OUT_PPVOID(&pSecurityDesc));
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlCreateSecurityDescriptorAbsolute(
                        pSecurityDesc,
                        SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    LSASRV_PRIVS_RDLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        PSID owner = NULL;
        BOOLEAN defaulted = FALSE;

        ntStatus = RtlGetOwnerSecurityDescriptor(
                            pAccount->pSecurityDesc,
                            &owner,
                            &defaulted);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RtlSetOwnerSecurityDescriptor(
                            pSecurityDesc,
                            owner,
                            defaulted);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
        PSID group = NULL;
        BOOLEAN defaulted = FALSE;

        ntStatus = RtlGetGroupSecurityDescriptor(
                            pAccount->pSecurityDesc,
                            &group,
                            &defaulted);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RtlSetGroupSecurityDescriptor(
                            pSecurityDesc,
                            group,
                            defaulted);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        PACL pDacl = NULL;
        BOOLEAN daclPresent = FALSE;
        BOOLEAN defaulted = FALSE;

        ntStatus = RtlGetDaclSecurityDescriptor(
                            pAccount->pSecurityDesc,
                            &daclPresent,
                            &pDacl,
                            &defaulted);
        BAIL_ON_NT_STATUS(ntStatus);

        if (daclPresent)
        {
            ntStatus = RtlSetDaclSecurityDescriptor(
                                pSecurityDesc,
                                daclPresent,
                                pDacl,
                                defaulted);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        PACL pSacl = NULL;
        BOOLEAN saclPresent = FALSE;
        BOOLEAN defaulted = FALSE;

        ntStatus = RtlGetSaclSecurityDescriptor(
                            pAccount->pSecurityDesc,
                            &saclPresent,
                            &pSacl,
                            &defaulted);
        BAIL_ON_NT_STATUS(ntStatus);

        if (saclPresent)
        {
            ntStatus = RtlSetSaclSecurityDescriptor(
                                pSecurityDesc,
                                saclPresent,
                                pSacl,
                                defaulted);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

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

    *ppSecurityDescRelative    = pSecurityDescRelative;
    *pSecurityDescRelativeSize = securityDescRelativeSize;

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    if (err || ntStatus)
    {
        LW_SAFE_FREE_MEMORY(pSecurityDescRelative);

        if (ppSecurityDescRelative)
        {
            *ppSecurityDescRelative = NULL;
        }

        if (pSecurityDescRelativeSize)
        {
            *pSecurityDescRelativeSize = 0;
        }
    }

    LW_SAFE_FREE_MEMORY(pSecurityDesc);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


DWORD
LsaSrvPrivsSetAccountSecurity(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescRelative,
    IN DWORD SecurityDescRelativeSize
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ACCESS_MASK requiredRights = LSA_ACCOUNT_VIEW |
                                 READ_CONTROL |
                                 WRITE_DAC |
                                 WRITE_OWNER;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = pAccountContext->pAccount;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDesc = NULL;
    DWORD securityDescSize = 0;
    PSID owner = NULL;
    DWORD ownerSize = 0;
    PSID group = NULL;
    DWORD groupSize = 0;
    PACL pDacl = NULL;
    DWORD daclSize = 0;
    PACL pSacl = NULL;
    DWORD saclSize = 0;

    if (!(pAccountContext->grantedAccess & requiredRights))
    {
        err = ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(err);
    }

    ntStatus = RtlSelfRelativeToAbsoluteSD(
                        pSecurityDescRelative,
                        pSecurityDesc,
                        &securityDescSize,
                        pDacl,
                        &daclSize,
                        pSacl,
                        &saclSize,
                        owner,
                        &ownerSize,
                        group,
                        &groupSize);
    if (ntStatus == STATUS_BUFFER_TOO_SMALL)
    {
        ntStatus = STATUS_SUCCESS;
    }
    else if (ntStatus != STATUS_SUCCESS)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (securityDescSize)
    {
        err = LwAllocateMemory(
                            securityDescSize,
                            OUT_PPVOID(&pSecurityDesc));
        BAIL_ON_LSA_ERROR(err);
    }

    if (daclSize)
    {
        err = LwAllocateMemory(
                            daclSize,
                            OUT_PPVOID(&pDacl));
        BAIL_ON_LSA_ERROR(err);
    }

    if (saclSize)
    {
        err = LwAllocateMemory(
                            saclSize,
                            OUT_PPVOID(&pSacl));
        BAIL_ON_LSA_ERROR(err);
    }

    if (ownerSize)
    {
        err = LwAllocateMemory(
                            ownerSize,
                            OUT_PPVOID(&owner));
        BAIL_ON_LSA_ERROR(err);
    }

    if (groupSize)
    {
        err = LwAllocateMemory(
                            groupSize,
                            OUT_PPVOID(&group));
        BAIL_ON_LSA_ERROR(err);
    }

    ntStatus = RtlSelfRelativeToAbsoluteSD(
                        pSecurityDescRelative,
                        pSecurityDesc,
                        &securityDescSize,
                        pDacl,
                        &daclSize,
                        pSacl,
                        &saclSize,
                        owner,
                        &ownerSize,
                        group,
                        &groupSize);
    BAIL_ON_NT_STATUS(ntStatus);

    LSASRV_PRIVS_WRLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        PSID oldOwner = NULL;
        BOOLEAN defaulted = FALSE;

        ntStatus = RtlGetOwnerSecurityDescriptor(
                            pAccount->pSecurityDesc,
                            &oldOwner,
                            &defaulted);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RtlSetOwnerSecurityDescriptor(
                            pAccount->pSecurityDesc,
                            owner,
                            defaulted);
        BAIL_ON_NT_STATUS(ntStatus);

        LW_SAFE_FREE_MEMORY(oldOwner);
        owner = NULL;

        pAccountContext->Dirty = TRUE;
    }

    if (SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
        PSID oldGroup = NULL;
        BOOLEAN defaulted = FALSE;

        ntStatus = RtlGetGroupSecurityDescriptor(
                            pAccount->pSecurityDesc,
                            &oldGroup,
                            &defaulted);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RtlSetGroupSecurityDescriptor(
                            pAccount->pSecurityDesc,
                            group,
                            defaulted);
        BAIL_ON_NT_STATUS(ntStatus);

        LW_SAFE_FREE_MEMORY(oldGroup);
        group = NULL;

        pAccountContext->Dirty = TRUE;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        PACL pOldDacl = NULL;
        BOOLEAN daclPresent = FALSE;
        BOOLEAN defaulted = FALSE;

        ntStatus = RtlGetDaclSecurityDescriptor(
                            pAccount->pSecurityDesc,
                            &daclPresent,
                            &pOldDacl,
                            &defaulted);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RtlSetDaclSecurityDescriptor(
                            pAccount->pSecurityDesc,
                            daclPresent,
                            pDacl,
                            defaulted);
        BAIL_ON_NT_STATUS(ntStatus);

        LW_SAFE_FREE_MEMORY(pOldDacl);
        pDacl = NULL;

        pAccountContext->Dirty = TRUE;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        PACL pOldSacl = NULL;
        BOOLEAN saclPresent = FALSE;
        BOOLEAN defaulted = FALSE;

        ntStatus = RtlGetDaclSecurityDescriptor(
                            pAccount->pSecurityDesc,
                            &saclPresent,
                            &pOldSacl,
                            &defaulted);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RtlSetDaclSecurityDescriptor(
                            pAccount->pSecurityDesc,
                            saclPresent,
                            pSacl,
                            defaulted);
        BAIL_ON_NT_STATUS(ntStatus);

        LW_SAFE_FREE_MEMORY(pOldSacl);
        pSacl = NULL;

        pAccountContext->Dirty = TRUE;
    }

    // Save the changes (if there were any)
    if (pAccountContext->Dirty)
    {
        err = LsaSrvUpdateAccount_inlock(
                      pAccount->pSid,
                      pAccount);
        BAIL_ON_LSA_ERROR(err);

        pAccountContext->Dirty = FALSE;
    }

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    LW_SAFE_FREE_MEMORY(pSecurityDesc);
    LW_SAFE_FREE_MEMORY(owner);
    LW_SAFE_FREE_MEMORY(group);
    LW_SAFE_FREE_MEMORY(pDacl);
    LW_SAFE_FREE_MEMORY(pSacl);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


DWORD
LsaSrvPrivsMarkAccountDeleted(
    IN PLSA_ACCOUNT_CONTEXT pAccountContext
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN locked = FALSE;
    PLSA_ACCOUNT pAccount = pAccountContext->pAccount;

    if (!(pAccountContext->grantedAccess & DELETE))
    {
        err = ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(err);
    }

    LSASRV_PRIVS_WRLOCK_RWLOCK(locked, &pAccount->accountRwLock);
    LsaSrvPrivsMarkAccountEntryDeleted_inlock(pAccount);

error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(locked, &pAccount->accountRwLock);

    return err;
}


static
VOID
LsaSrvPrivsMarkAccountEntryDeleted_inlock(
    PLSA_ACCOUNT pAccount
    )
{
    pAccount->Delete = TRUE;
}
