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
LsaSrvPrivsAccountRightNameToSar(
    IN PWSTR AccessRightName,
    OUT PDWORD pAccessRight
    );



DWORD
LsaSrvPrivsEnumPrivilegesSids(
    IN HANDLE hProvider,
    IN PSTR *ppszSids,
    IN DWORD NumSids,
    OUT PLUID_AND_ATTRIBUTES *ppPrivileges,
    OUT PDWORD pNumPrivileges
    )
{
    DWORD err = ERROR_SUCCESS;

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
    PPRIVILEGE_SET pPrivilegeSet = NULL;
    DWORD systemAccessRights = 0;
    
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

    err = LsaSrvResolveNamesToPrivilegesAndSar(
                        hProvider,
                        accessToken,
                        ppwszAccountRights,
                        NumAccountRights,
                        &pPrivilegeSet,
                        &systemAccessRights);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvPrivsAddPrivilegesToAccount(
                        hProvider,
                        accountContext,
                        pPrivilegeSet);
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
        if (err == ERROR_NO_SUCH_PRIVILEGE)
        {
            err = LsaSrvPrivsAccountRightNameToSar(
                                pAccountRights[i],
                                &systemAccess);
            BAIL_ON_LSA_ERROR(err);

            systemAccessRights |= systemAccess;
        }
        else if (err != ERROR_SUCCESS)
        {
            BAIL_ON_LSA_ERROR(err);
        }

        pPrivilegeSet->Privilege[numPrivileges++].Luid = privilegeValue;
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
    PPRIVILEGE_SET pPrivilegeSet = NULL;
    DWORD systemAccessRights = 0;

    err = LsaSrvPrivsOpenAccount(
                            hProvider,
                            accessToken,
                            AccountSid,
                            accessRights,
                            &accountContext);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvResolveNamesToPrivilegesAndSar(
                        hProvider,
                        accessToken,
                        ppwszAccountRights,
                        NumAccountRights,
                        &pPrivilegeSet,
                        &systemAccessRights);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvPrivsRemovePrivilegesFromAccount(
                        hProvider,
                        accountContext,
                        RemoveAll,
                        pPrivilegeSet);
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
    OUT PWSTR **ppwszAccountRights,
    OUT PDWORD pNumAccountRights
    )
{
    DWORD err = ERROR_SUCCESS;

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
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN PPRIVILEGE_SET pPrivilegeSet
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN accountLocked = FALSE;
    PLSA_ACCOUNT pAccount = pAccountContext->pAccount;
    DWORD i = 0;
    DWORD j = 0;
    BOOLEAN found = FALSE;

    LSASRV_PRIVS_WRLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    for (i = 0; i < pPrivilegeSet->PrivilegeCount; i++)
    {
        for (j = 0; !found && j < pAccount->NumPrivileges; j++)
        {
            found = RtlEqualLuid(
                         &pPrivilegeSet->Privilege[i].Luid,
                         &pAccount->Privileges[j].Luid);
        }

        if (!found)
        {
            pAccount->Privileges[pAccount->NumPrivileges++].Luid =
                pPrivilegeSet->Privilege[i].Luid;
        }
    }

    err = LsaSrvUpdateAccount(pAccount->pSid,
                              pAccount);
    BAIL_ON_LSA_ERROR(err);
                   
error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    return err;
}


DWORD
LsaSrvPrivsRemovePrivilegesFromAccount(
    IN HANDLE hProvider,
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

    err = LsaSrvUpdateAccount(pAccount->pSid,
                              pAccount);
    BAIL_ON_LSA_ERROR(err);
                   
error:
    LSASRV_PRIVS_UNLOCK_RWLOCK(accountLocked, &pAccount->accountRwLock);

    return err;
}


static
DWORD
LsaSrvPrivsAccountRightNameToSar(
    IN PWSTR AccessRightName,
    OUT PDWORD pAccessRight
    )
{
    DWORD err = ERROR_SUCCESS;

    *pAccessRight = 0;

    return err;
}
