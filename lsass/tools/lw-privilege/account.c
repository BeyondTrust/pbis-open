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
 *        LSA Privileges, System Access Rights and Account Rights manager
 *        (rpc client utility).
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

static
DWORD
ProcessEnumerateAccounts(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR UserRightName
    );

static
DWORD
ProcessEnumerateAccountUserRights(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR AccountName
    );

static
DWORD
ProcessAddRemoveAccountRights(
    IN PRPC_PARAMETERS pRpcParams,
    IN BOOLEAN Add,
    IN PSTR AccountRights,
    IN BOOLEAN RemoveAll,
    IN PSTR AccountName
    );

static
DWORD
ProcessDeleteAccount(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR AccountName
    );


VOID
ShowUsageAccount(
    PCSTR pszProgramName
    )
{
    fprintf(stdout, "Usage: %s account <command> [options]\n\n", pszProgramName);
    fprintf(stdout, "command:\n");
    fprintf(stdout, "-e, --enum-account-rights <account> - Enumerate account rights of a specified account\n");
    fprintf(stdout, "--enum-accounts - Enumerate all accounts\n");
    fprintf(stdout, "--enum-accounts [account right] - Enumerate all accounts with a specified account right\n");
    fprintf(stdout, "-a, --add <acct right>[,<acct right>] <account> - Add account rights to a specified account\n");
    fprintf(stdout, "--remove <acct right>[,<acct right>] <account> - Remove account rights from a specified account\n");
    fprintf(stdout, "--remove-all <account> - Remove all account rights from a specified account\n");
    fprintf(stdout, "--delete <account> - Delete specified account\n");
}


DWORD
ProcessAccount(
    const DWORD Argc,
    PCSTR* Argv
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD i = 0;
    PCSTR pszArg = NULL;
    RPC_PARAMETERS Params = {0};
    PRIVILEGE_COMMAND Command = PrivilegeDoNothing;
    PSTR userRightName = NULL;
    PSTR accountName = NULL;
    PSTR accountRightName = NULL;
    BOOLEAN removeAll = FALSE;

    err = ProcessRpcParameters(Argc, Argv, &Params);
    BAIL_ON_LSA_ERROR(err);

    for (i = 0; i < Argc; i++)
    {
        pszArg = Argv[i];

        if (strcmp(pszArg, "--enum-accounts") == 0)
        {
            if (i + 1 < Argc)
            {
                pszArg = Argv[++i];
                err = LwAllocateString(pszArg, &userRightName);
                BAIL_ON_LSA_ERROR(err);
            }

            Command = AccountEnumerate;
        }
        else if (((strcmp(pszArg, "-e") == 0) ||
                  (strcmp(pszArg, "--enum-account-rights") == 0)) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &accountName);
            BAIL_ON_LSA_ERROR(err);

            Command = AccountEnumerateUserRights;
        }
        else if (((strcmp(pszArg, "-a") == 0) ||
                  (strcmp(pszArg, "--add") == 0)) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &accountRightName);
            BAIL_ON_LSA_ERROR(err);

            Command = AccountAddAccountRights;
        }
        else if (((strcmp(pszArg, "-r") == 0) ||
                  (strcmp(pszArg, "--remove") == 0)) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &accountRightName);
            BAIL_ON_LSA_ERROR(err);

            Command = AccountRemoveAccountRights;
        }
        else if (strcmp(pszArg, "--remove-all") == 0)
        {
            removeAll = TRUE;
            Command = AccountRemoveAccountRights;
        }
        else if (strcmp(pszArg, "--delete") == 0)
        {
            Command = AccountDelete;
        }
        else if (i + 1 == Argc)
        {
            LW_SAFE_FREE_MEMORY(accountName);

            pszArg = Argv[i];
            err = LwAllocateString(pszArg, &accountName);
            BAIL_ON_LSA_ERROR(err);
        }
    }

    switch (Command)
    {
    case AccountEnumerate:
        err = ProcessEnumerateAccounts(
                                &Params,
                                userRightName);
        BAIL_ON_LSA_ERROR(err);
        break;

    case AccountEnumerateUserRights:
        err = ProcessEnumerateAccountUserRights(
                                &Params,
                                accountName);
        BAIL_ON_LSA_ERROR(err);
        break;

    case AccountAddAccountRights:
        err = ProcessAddRemoveAccountRights(
                                &Params,
                                TRUE,
                                accountRightName,
                                removeAll,
                                accountName);
        BAIL_ON_LSA_ERROR(err);
        break;

    case AccountRemoveAccountRights:
        err = ProcessAddRemoveAccountRights(
                                &Params,
                                FALSE,
                                accountRightName,
                                removeAll,
                                accountName);
        BAIL_ON_LSA_ERROR(err);
        break;

    case AccountDelete:
        err = ProcessDeleteAccount(
                                &Params,
                                accountName);
        BAIL_ON_LSA_ERROR(err);
        break;

    default:
        ShowUsageAccount(Argv[0]);
        break;
    }

error:
    LW_SAFE_FREE_MEMORY(userRightName);
    LW_SAFE_FREE_MEMORY(accountName);
    LW_SAFE_FREE_MEMORY(accountRightName);

    return err;
}


static
DWORD
ProcessEnumerateAccounts(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR UserRightName
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_BINDING hLsa = NULL;
    LW_PIO_CREDS pCreds = NULL;
    WCHAR wszSysName[] = {'\\', '\\', '\0'};
    DWORD policyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                             LSA_ACCESS_VIEW_POLICY_INFO;
    POLICY_HANDLE hPolicy = NULL;
    PWSTR pwszUserRightName = NULL;
    DWORD resume = 0;
    PSID *ppSids = NULL;
    DWORD numSids = 0;
    DWORD prefMaxSize = 64;
    DWORD i = 0;
    SID_ARRAY sids = {0};
    RefDomainList *pDomList = NULL;
    TranslatedName *pTransNames = NULL;
    DWORD count = 0;
    PSTR pszAccountSid = NULL;
    PSTR pszAccountDomain = NULL;
    PSTR pszAccountName = NULL;
    BOOLEAN moreEntries = FALSE;
    
    err = CreateRpcCredentials(pRpcParams,
                               &pCreds);
    BAIL_ON_LSA_ERROR(err);

    err = CreateLsaRpcBinding(pRpcParams,
                              pCreds,
                              &hLsa);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = LsaOpenPolicy2(hLsa,
                              wszSysName,
                              NULL,
                              policyAccessMask,
                              &hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    fprintf(stdout,
            "LSA Accounts");

    do
    {
        moreEntries = FALSE;

        if (UserRightName)
        {
            fprintf(stdout, " with AccountRight = %s:\n", UserRightName);
            fprintf(stdout,
                    "=================================================="
                    "==============================\n");

            err = LwMbsToWc16s(UserRightName,
                               &pwszUserRightName);
            BAIL_ON_LSA_ERROR(err);

            ntStatus = LsaEnumAccountsWithUserRight(
                                       hLsa,
                                       hPolicy,
                                       pwszUserRightName,
                                       &ppSids,
                                       &numSids);
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            fprintf(stdout, ":\n");
            fprintf(stdout,
                    "=================================================="
                    "==============================\n");

            ntStatus = LsaEnumAccounts(hLsa,
                                       hPolicy,
                                       &resume,
                                       &ppSids,
                                       &numSids,
                                       prefMaxSize);
            if (ntStatus == STATUS_MORE_ENTRIES)
            {
                ntStatus = STATUS_SUCCESS;
                moreEntries = TRUE;
            }
            else if (ntStatus != STATUS_SUCCESS)
            {
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }

        err = LwAllocateMemory(
                       sizeof(sids.pSids[0]) * numSids,
                       OUT_PPVOID(&sids.pSids));
        BAIL_ON_LSA_ERROR(err);

        sids.dwNumSids = numSids;

        for (i = 0; i < sids.dwNumSids; i++)
        {
            sids.pSids[i].pSid = ppSids[i];
        }

        ntStatus = LsaLookupSids(hLsa,
                                 hPolicy,
                                 &sids,
                                 &pDomList,
                                 &pTransNames,
                                 LSA_LOOKUP_NAMES_ALL,
                                 &count);
        if (ntStatus == STATUS_SOME_NOT_MAPPED ||
            ntStatus == STATUS_NONE_MAPPED)
        {
            ntStatus = STATUS_SUCCESS;
        }
        else if (ntStatus != STATUS_SUCCESS)
        {
            BAIL_ON_NT_STATUS(ntStatus);
        }

        for (i = 0; i < sids.dwNumSids; i++)
        {
            DWORD domainIndex = 0;

            ntStatus = RtlAllocateCStringFromSid(
                                 &pszAccountSid,
                                 sids.pSids[i].pSid);
            BAIL_ON_NT_STATUS(ntStatus);

            if (pTransNames[i].type == SID_TYPE_USER ||
                pTransNames[i].type == SID_TYPE_DOM_GRP ||
                pTransNames[i].type == SID_TYPE_DOMAIN ||
                pTransNames[i].type == SID_TYPE_ALIAS ||
                pTransNames[i].type == SID_TYPE_WKN_GRP)
            {
                ntStatus = RtlCStringAllocateFromUnicodeString(
                                     &pszAccountName,
                                     &pTransNames[i].name);
                BAIL_ON_NT_STATUS(ntStatus);

                domainIndex = pTransNames[i].sid_index;

                ntStatus = RtlCStringAllocateFromUnicodeString(
                                      &pszAccountDomain,
                                      &pDomList->domains[domainIndex].name);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            if (pszAccountSid)
            {
                fprintf(stdout, "%s ", pszAccountSid);
            }

            if (pszAccountDomain && pszAccountName)
            {
                fprintf(stdout, "(%s\\%s)", pszAccountDomain, pszAccountName);
            }
            else if (pszAccountDomain && !pszAccountName)
            {
                fprintf(stdout, "(%s\\)", pszAccountDomain);
            }
            else if (!pszAccountDomain && pszAccountName)
            {
                fprintf(stdout, "(%s)", pszAccountName);
            }
            else
            {
                fprintf(stdout, "(unknown)");
            }

            fprintf(stdout, "\n");

            RTL_FREE(&pszAccountSid);
            RTL_FREE(&pszAccountDomain);
            RTL_FREE(&pszAccountName);

        }

        if (pTransNames)
        {
            LsaRpcFreeMemory(pTransNames);
            pTransNames = NULL;
        }

        if (pDomList)
        {
            LsaRpcFreeMemory(pDomList);
            pDomList = NULL;
        }

        if (ppSids)
        {
            LsaRpcFreeMemory(ppSids);
            ppSids = NULL;
        }

        LW_SAFE_FREE_MEMORY(sids.pSids);

    } while (moreEntries && ntStatus == STATUS_SUCCESS);

error:
    if (ntStatus || err)
    {
        PCSTR errName = LwNtStatusToName(ntStatus);
        PCSTR errDescription = LwNtStatusToDescription(ntStatus);

        if (ntStatus)
        {
            errName = LwNtStatusToName(ntStatus);
            errDescription = LwNtStatusToDescription(ntStatus);
        }
        else
        {
            errName = LwWin32ErrorToName(err);
            errDescription = LwWin32ErrorToDescription(err);
        }

        fprintf(stderr, "Error: %s (%s)\n",
                LSA_SAFE_LOG_STRING(errName),
                LSA_SAFE_LOG_STRING(errDescription));
    }

    if (hPolicy)
    {
        LsaClose(hLsa, hPolicy);
    }

    if (hLsa)
    {
        LsaFreeBinding(&hLsa);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    LW_SAFE_FREE_MEMORY(pwszUserRightName);
    RTL_FREE(&pszAccountSid);
    RTL_FREE(&pszAccountDomain);
    RTL_FREE(&pszAccountName);

    if (pTransNames)
    {
        LsaRpcFreeMemory(pTransNames);
    }

    if (pDomList)
    {
        LsaRpcFreeMemory(pDomList);
    }

    if (ppSids)
    {
        LsaRpcFreeMemory(ppSids);
    }

    LW_SAFE_FREE_MEMORY(sids.pSids);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
ProcessEnumerateAccountUserRights(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR AccountName
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_BINDING hLsa = NULL;
    LW_PIO_CREDS pCreds = NULL;
    WCHAR wszSysName[] = {'\\', '\\', '\0'};
    DWORD policyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                             LSA_ACCESS_VIEW_POLICY_INFO;
    POLICY_HANDLE hPolicy = NULL;
    PSID pAccountSid = NULL;
    PSTR pszSid = NULL;
    PWSTR *pAccountRights = NULL;
    DWORD numAccountRights = 0;
    DWORD i = 0;
    PSTR pszAccountRight = NULL;
    
    err = CreateRpcCredentials(pRpcParams,
                               &pCreds);
    BAIL_ON_LSA_ERROR(err);

    err = CreateLsaRpcBinding(pRpcParams,
                              pCreds,
                              &hLsa);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = LsaOpenPolicy2(hLsa,
                              wszSysName,
                              NULL,
                              policyAccessMask,
                              &hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    err = ResolveAccountNameToSid(
                          hLsa,
                          AccountName,
                          &pAccountSid);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlAllocateCStringFromSid(
                          &pszSid,
                          pAccountSid);
    BAIL_ON_NT_STATUS(ntStatus);

    fprintf(stdout, "%s Account Rights\n:"
            "=================================================="
            "==============================\n", AccountName);

    ntStatus = LsaEnumAccountRights(
                          hLsa,
                          hPolicy,
                          pAccountSid,
                          &pAccountRights,
                          &numAccountRights);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < numAccountRights; i++)
    {
        err = LwWc16sToMbs(pAccountRights[i],
                           &pszAccountRight);
        BAIL_ON_LSA_ERROR(err);

        fprintf(stdout, "%s\n", pszAccountRight);

        LW_SAFE_FREE_MEMORY(pszAccountRight);
    }

error:
    if (ntStatus || err)
    {
        PCSTR errName = LwNtStatusToName(ntStatus);
        PCSTR errDescription = LwNtStatusToDescription(ntStatus);

        if (ntStatus)
        {
            errName = LwNtStatusToName(ntStatus);
            errDescription = LwNtStatusToDescription(ntStatus);
        }
        else
        {
            errName = LwWin32ErrorToName(err);
            errDescription = LwWin32ErrorToDescription(err);
        }

        fprintf(stderr, "Error: %s (%s)\n",
                LSA_SAFE_LOG_STRING(errName),
                LSA_SAFE_LOG_STRING(errDescription));
    }

    if (hPolicy)
    {
        LsaClose(hLsa, hPolicy);
    }

    if (hLsa)
    {
        LsaFreeBinding(&hLsa);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (pAccountRights)
    {
        LsaRpcFreeMemory(pAccountRights);
    }

    LW_SAFE_FREE_MEMORY(pszAccountRight);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
ProcessAddRemoveAccountRights(
    IN PRPC_PARAMETERS pRpcParams,
    IN BOOLEAN Add,
    IN PSTR AccountRights,
    IN BOOLEAN RemoveAll,
    IN PSTR AccountName
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_BINDING hLsa = NULL;
    LW_PIO_CREDS pCreds = NULL;
    WCHAR wszSysName[] = {'\\', '\\', '\0'};
    DWORD policyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                             LSA_ACCESS_CREATE_SPECIAL_ACCOUNTS;
    POLICY_HANDLE hPolicy = NULL;
    PSID pAccountSid = NULL;
    PSTR *ppszAccountRightNames = NULL;
    DWORD numAccountRightNames = 0;
    PWSTR *ppwszAccountRightNames = NULL;
    DWORD i = 0;
    
    err = CreateRpcCredentials(pRpcParams,
                               &pCreds);
    BAIL_ON_LSA_ERROR(err);

    err = CreateLsaRpcBinding(pRpcParams,
                              pCreds,
                              &hLsa);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = LsaOpenPolicy2(hLsa,
                              wszSysName,
                              NULL,
                              policyAccessMask,
                              &hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    err = ResolveAccountNameToSid(
                          hLsa,
                          AccountName,
                          &pAccountSid);
    BAIL_ON_LSA_ERROR(err);

    if (AccountRights)
    {
        err = GetStringListFromString(
                              AccountRights,
                              SEPARATOR_CHAR,
                              &ppszAccountRightNames,
                              &numAccountRightNames);
        BAIL_ON_LSA_ERROR(err);

        err = LwAllocateMemory(
                       sizeof(ppwszAccountRightNames[0]) * numAccountRightNames,
                       OUT_PPVOID(&ppwszAccountRightNames));
        BAIL_ON_LSA_ERROR(err);

        for (i = 0; i < numAccountRightNames; i++)
        {
            err = LwMbsToWc16s(ppszAccountRightNames[i],
                               &ppwszAccountRightNames[i]);
            BAIL_ON_LSA_ERROR(err);
        }
    }

    if (Add)
    {
        ntStatus = LsaAddAccountRights(
                              hLsa,
                              hPolicy,
                              pAccountSid,
                              ppwszAccountRightNames,
                              numAccountRightNames);
        BAIL_ON_NT_STATUS(ntStatus);

        fprintf(stdout, "Successfully added account rights to %s\n", AccountName);
    }
    else
    {
        ntStatus = LsaRemoveAccountRights(
                              hLsa,
                              hPolicy,
                              pAccountSid,
                              RemoveAll,
                              ppwszAccountRightNames,
                              numAccountRightNames);
        BAIL_ON_NT_STATUS(ntStatus);

        fprintf(stdout, "Successfully removed account rights from %s\n", AccountName);
    }

error:
    if (ntStatus || err)
    {
        PCSTR errName = LwNtStatusToName(ntStatus);
        PCSTR errDescription = LwNtStatusToDescription(ntStatus);

        if (ntStatus)
        {
            errName = LwNtStatusToName(ntStatus);
            errDescription = LwNtStatusToDescription(ntStatus);
        }
        else
        {
            errName = LwWin32ErrorToName(err);
            errDescription = LwWin32ErrorToDescription(err);
        }

        fprintf(stderr, "Error: %s (%s)\n",
                LSA_SAFE_LOG_STRING(errName),
                LSA_SAFE_LOG_STRING(errDescription));
    }

    if (hPolicy)
    {
        LsaClose(hLsa, hPolicy);
    }

    if (hLsa)
    {
        LsaFreeBinding(&hLsa);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    for (i = 0; i < numAccountRightNames; i++)
    {
        LW_SAFE_FREE_MEMORY(ppwszAccountRightNames[i]);
        LW_SAFE_FREE_MEMORY(ppszAccountRightNames[i]);
    }
    LW_SAFE_FREE_MEMORY(ppwszAccountRightNames);
    LW_SAFE_FREE_MEMORY(ppszAccountRightNames);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
ProcessDeleteAccount(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR AccountName
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_BINDING hLsa = NULL;
    LW_PIO_CREDS pCreds = NULL;
    WCHAR wszSysName[] = {'\\', '\\', '\0'};
    DWORD policyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                             LSA_ACCESS_VIEW_POLICY_INFO;
    DWORD accountAccessMask = DELETE;
    POLICY_HANDLE hPolicy = NULL;
    LSAR_ACCOUNT_HANDLE hAccount = NULL;
    PSID pAccountSid = NULL;
    
    err = CreateRpcCredentials(pRpcParams,
                               &pCreds);
    BAIL_ON_LSA_ERROR(err);

    err = CreateLsaRpcBinding(pRpcParams,
                              pCreds,
                              &hLsa);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = LsaOpenPolicy2(hLsa,
                              wszSysName,
                              NULL,
                              policyAccessMask,
                              &hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    err = ResolveAccountNameToSid(
                          hLsa,
                          AccountName,
                          &pAccountSid);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = LsaOpenAccount(
                          hLsa,
                          hPolicy,
                          pAccountSid,
                          accountAccessMask,
                          &hAccount);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaRpcDeleteObject(
                          hLsa,
                          hAccount);
    BAIL_ON_NT_STATUS(ntStatus);

error:
    if (ntStatus || err)
    {
        PCSTR errName = LwNtStatusToName(ntStatus);
        PCSTR errDescription = LwNtStatusToDescription(ntStatus);

        if (ntStatus)
        {
            errName = LwNtStatusToName(ntStatus);
            errDescription = LwNtStatusToDescription(ntStatus);
        }
        else
        {
            errName = LwWin32ErrorToName(err);
            errDescription = LwWin32ErrorToDescription(err);
        }

        fprintf(stderr, "Error: %s (%s)\n",
                LSA_SAFE_LOG_STRING(errName),
                LSA_SAFE_LOG_STRING(errDescription));
    }

    if (hPolicy)
    {
        LsaClose(hLsa, hPolicy);
    }

    if (hLsa)
    {
        LsaFreeBinding(&hLsa);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


