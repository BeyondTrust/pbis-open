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
 *        privilege.c
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
ProcessEnumeratePrivileges(
    IN PRPC_PARAMETERS pRpcParams
    );

static
DWORD
ProcessLookupPrivilegeValues(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR PrivilegeNames
    );

static
DWORD
ProcessLookupPrivilegeNames(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR PrivilegeValues
    );

static
DWORD
ProcessLookupPrivilegeDescriptions(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR PrivilegeNames
    );

static
DWORD
ProcessEnumerateAccountPrivileges(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR pszAccountName
    );

static
DWORD
ProcessAddRemovePrivilegesAccount(
    IN PRPC_PARAMETERS pRpcParams,
    IN BOOLEAN Add,
    IN PSTR pszAccountName,
    IN BOOLEAN CreateAccount,
    IN PSTR pszPrivileges,
    IN BOOLEAN RemoveAll
    );


VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    fprintf(stdout, "Usage: %s <mode> [options]\n\n", pszProgramName);
    fprintf(stdout, "mode (default: privilege):\n");
    fprintf(stdout, "privilege - Local Privileges mode\n");
    fprintf(stdout, "sar       - System Access Rights mode\n");
    fprintf(stdout, "account   - Account Rights mode\n");
    fprintf(stdout, "security  - Security mode\n");
    fprintf(stdout, "common options:\n");
    fprintf(stdout, "-h, --host           - hostname to call (skip to call the local host)\n");
    fprintf(stdout, "-b, --binding-string - rpc binding string to use instead of the default\n");
    fprintf(stdout, "-r, --krb5-principal - Kerberos 5 principal name (KRB5 authentication)\n");
    fprintf(stdout, "-c, --krb5-cache     - Kerberos 5 credentials cache path (KRB5 authentication)\n");
    fprintf(stdout, "-d, --domain         - Domain name (NTLM authentication)\n");
    fprintf(stdout, "-u, --username       - User name (NTLM authentication)\n");
    fprintf(stdout, "-p, --password       - User password (NTLM authentication)\n");
}


VOID
ShowUsagePrivileges(
    PCSTR pszProgramName
    )
{
    fprintf(stdout, "Usage: %s privilege <command> [options]\n\n", pszProgramName);
    fprintf(stdout, "commands:\n");
    fprintf(stdout, "-e, --enumerate - Enumerate Privileges available in the system\n");
    fprintf(stdout, "-e, --enumerate <account> - Enumerate Privileges assigned to account\n");
    fprintf(stdout, "-a, --add <privilege>[,<privilege] <account> - Add privileges to account\n");
    fprintf(stdout, "--remove <privilege>[,<privilege] <account> - Remove privileges from account\n");
    fprintf(stdout, "--remove-all <account> - Remove all privileges from account\n");
    fprintf(stdout, "--lookup <privilege>[,<privilege] - Lookup privilege values\n");
    fprintf(stdout, "--lookup-names \"<luid>,[<luid>]\" - Lookup privilege names\n");
    fprintf(stdout, "--lookup-descriptions <privilege>[,<privilege>] - Lookup privilege descriptions\n");
    fprintf(stdout, "options:\n");
    fprintf(stdout, "--dont-create - Do not create account if not existing\n");
}


int
main(const int argc, const char* argv[])
{
    DWORD err = ERROR_SUCCESS;
    PCSTR pszMode = NULL;
    DWORD i = 0;

    if (argc <= 1)
    {
        ShowUsage(argv[0]);
        goto error;
    }
    else
    {
        for (i = 1; i < argc; i++)
        {
            pszMode = argv[i];

            if (strcmp(pszMode, "privilege") == 0)
            {
                err = ProcessPrivilege(argc, argv);
                break;
            }
            else if (strcmp(pszMode, "sar") == 0)
            {
                err = ProcessSystemAccessRight(argc, argv);
                break;
            }
            else if (strcmp(pszMode, "security") == 0)
            {
                err = ProcessSecurity(argc, argv);
                break;
            }
            else if (strcmp(pszMode, "account") == 0)
            {
                err = ProcessAccount(argc, argv);
                break;
            }
            else
            {
                // The default is to process the privileges
                err = ProcessPrivilege(argc, argv);
                break;
            }
        }
    }

error:
    return err;
}


DWORD
ProcessPrivilege(
    const DWORD Argc,
    PCSTR* Argv
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD i = 0;
    PCSTR pszArg = NULL;
    RPC_PARAMETERS Params = {0};
    PRIVILEGE_COMMAND Command = PrivilegeDoNothing;
    PSTR accountName = NULL;
    PSTR privilegeNames = NULL;
    PSTR privilegeValues = NULL;
    BOOLEAN createAccount = TRUE;
    BOOLEAN removeAll = FALSE;

    err = ProcessRpcParameters(Argc, Argv, &Params);
    BAIL_ON_LSA_ERROR(err);

    for (i = 0; i < Argc; i++)
    {
        pszArg = Argv[i];

        if (((strcmp(pszArg, "-e") == 0) ||
             (strcmp(pszArg, "--enumerate") == 0)))
        {
            if (i + 1 < Argc)
            {
                pszArg = Argv[++i];
                err = LwAllocateString(pszArg, &accountName);
                BAIL_ON_LSA_ERROR(err);

                Command = PrivilegeEnumerateAccountPrivs;
            }
            else
            {
                Command = PrivilegeEnumeratePrivs;
            }
        }
        else if (((strcmp(pszArg, "-a") == 0) ||
                  (strcmp(pszArg, "--add") == 0)) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &privilegeNames);
            BAIL_ON_LSA_ERROR(err);

            Command = PrivilegeAddAccountPrivs;
        }
        else if ((strcmp(pszArg, "--remove") == 0) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &privilegeNames);
            BAIL_ON_LSA_ERROR(err);

            Command = PrivilegeRemoveAccountPrivs;
        }
        else if (strcmp(pszArg, "--dont-create") == 0)
        {
            createAccount = FALSE;
        }
        else if (strcmp(pszArg, "--remove-all") == 0)
        {
            removeAll = TRUE;

            Command = PrivilegeRemoveAccountPrivs;
        }
        else if ((strcmp(pszArg, "--lookup") == 0) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &privilegeNames);
            BAIL_ON_LSA_ERROR(err);

            Command = PrivilegeLookupValues;
        }
        else if ((strcmp(pszArg, "--lookup-names") == 0) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &privilegeValues);
            BAIL_ON_LSA_ERROR(err);

            Command = PrivilegeLookupNames;
        }
        else if ((strcmp(pszArg, "--lookup-descriptions") == 0) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &privilegeNames);
            BAIL_ON_LSA_ERROR(err);

            Command = PrivilegeLookupDescriptions;
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
    case PrivilegeEnumeratePrivs:
        err = ProcessEnumeratePrivileges(&Params);
        BAIL_ON_LSA_ERROR(err);
        break;

    case PrivilegeLookupValues:
        err = ProcessLookupPrivilegeValues(
                                &Params,
                                privilegeNames);
        BAIL_ON_LSA_ERROR(err);
        break;

    case PrivilegeLookupNames:
        err = ProcessLookupPrivilegeNames(
                                &Params,
                                privilegeValues);
        BAIL_ON_LSA_ERROR(err);
        break;

    case PrivilegeLookupDescriptions:
        err = ProcessLookupPrivilegeDescriptions(
                                &Params,
                                privilegeNames);
        BAIL_ON_LSA_ERROR(err);
        break;

    case PrivilegeEnumerateAccountPrivs:
        err = ProcessEnumerateAccountPrivileges(
                                &Params,
                                accountName);
        BAIL_ON_LSA_ERROR(err);
        break;

    case PrivilegeAddAccountPrivs:
        err = ProcessAddRemovePrivilegesAccount(
                                &Params,
                                TRUE,
                                accountName,
                                createAccount,
                                privilegeNames,
                                removeAll);
        BAIL_ON_LSA_ERROR(err);
        break;

    case PrivilegeRemoveAccountPrivs:
        err = ProcessAddRemovePrivilegesAccount(
                                &Params,
                                FALSE,
                                accountName,
                                createAccount,
                                privilegeNames,
                                removeAll);
        BAIL_ON_LSA_ERROR(err);
        break;

    default:
        ShowUsagePrivileges(Argv[0]);
        break;
    }

error:
    LW_SAFE_FREE_MEMORY(accountName);
    LW_SAFE_FREE_MEMORY(privilegeNames);

    return err;
}


static
DWORD
ProcessEnumeratePrivileges(
    IN PRPC_PARAMETERS pRpcParams
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_BINDING hLsa = NULL;
    LW_PIO_CREDS pCreds = NULL;
    WCHAR wszSysName[] = {'\\', '\\', 0};
    DWORD policyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                             LSA_ACCESS_VIEW_POLICY_INFO;
    POLICY_HANDLE hPolicy = NULL;
    DWORD resume = 0;
    DWORD preferredMaxSize = 64;
    PWSTR *pNames = NULL;
    PLUID pValues = NULL;
    DWORD numPrivileges = 0;
    DWORD i = 0;
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

    fprintf(stdout, "LSA Privileges:\n"
            "=================================================="
            "==============================\n");

    do
    {
        moreEntries = FALSE;

        ntStatus = LsaEnumPrivileges(
                                  hLsa,
                                  hPolicy,
                                  &resume,
                                  preferredMaxSize,
                                  &pNames,
                                  &pValues,
                                  &numPrivileges);
        if (ntStatus == STATUS_MORE_ENTRIES)
        {
            moreEntries = TRUE;
            ntStatus = STATUS_SUCCESS;
        }
        else if (ntStatus != STATUS_SUCCESS)
        {
            BAIL_ON_NT_STATUS(ntStatus);
        }

        for (i = 0; i < numPrivileges; i++)
        {
            PSTR privilegeName = NULL;

            err = LwWc16sToMbs(pNames[i],
                               &privilegeName);
            BAIL_ON_LSA_ERROR(err);

            fprintf(stdout, "{%d,%u} %s\n",
                    pValues[i].HighPart,
                    pValues[i].LowPart,
                    privilegeName);

            LW_SAFE_FREE_MEMORY(privilegeName);
        }

        if (pNames)
        {
            LsaRpcFreeMemory(pNames);
            pNames = NULL;
        }

        if (pValues)
        {
            LsaRpcFreeMemory(pValues);
            pValues = NULL;
        }

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

    if (pNames)
    {
        LsaRpcFreeMemory(pNames);
    }

    if (pValues)
    {
        LsaRpcFreeMemory(pValues);
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
ProcessLookupPrivilegeValues(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR PrivilegeNames
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
    PSTR *pPrivileges = NULL;
    DWORD numPrivileges = 0;
    PWSTR pwszPrivilegeName = NULL;
    LUID privilegeValue = {0};
    DWORD i = 0;

    err = CreateRpcCredentials(pRpcParams,
                               &pCreds);
    BAIL_ON_LSA_ERROR(err);

    err = CreateLsaRpcBinding(pRpcParams,
                              pCreds,
                              &hLsa);
    BAIL_ON_LSA_ERROR(err);

    err = GetStringListFromString(
                             PrivilegeNames,
                             SEPARATOR_CHAR,
                             &pPrivileges,
                             &numPrivileges);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = LsaOpenPolicy2(hLsa,
                              wszSysName,
                              NULL,
                              policyAccessMask,
                              &hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    fprintf(stdout,
            "Privilege values:\n"
            "=================================================================="
            "==============\n");

    for (i = 0; i < numPrivileges; i++)
    {
        err = LwMbsToWc16s(pPrivileges[i],
                           &pwszPrivilegeName);
        BAIL_ON_LSA_ERROR(err);

        ntStatus = LsaLookupPrivilegeValue(
                                  hLsa,
                                  hPolicy,
                                  pwszPrivilegeName,
                                  &privilegeValue);
        BAIL_ON_NT_STATUS(ntStatus);

        fprintf(stdout, "{%d,%u} %s\n",
                privilegeValue.HighPart,
                privilegeValue.LowPart,
                pPrivileges[i]);

        LW_SAFE_FREE_MEMORY(pwszPrivilegeName);
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

    LW_SAFE_FREE_MEMORY(pwszPrivilegeName);

    for (i = 0; i < numPrivileges; i++)
    {
        LW_SAFE_FREE_MEMORY(pPrivileges[i]);
    }
    LW_SAFE_FREE_MEMORY(pPrivileges);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
ProcessLookupPrivilegeNames(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR PrivilegeValues
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
    PLUID pPrivileges = NULL;
    DWORD numPrivileges = 0;
    PWSTR pwszPrivilegeName = NULL;
    PSTR pszPrivilegeName = NULL;
    DWORD i = 0;

    err = CreateRpcCredentials(pRpcParams,
                               &pCreds);
    BAIL_ON_LSA_ERROR(err);

    err = CreateLsaRpcBinding(pRpcParams,
                              pCreds,
                              &hLsa);
    BAIL_ON_LSA_ERROR(err);

    err = GetLuidListFromString(
                             PrivilegeValues,
                             &pPrivileges,
                             &numPrivileges);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = LsaOpenPolicy2(hLsa,
                              wszSysName,
                              NULL,
                              policyAccessMask,
                              &hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    fprintf(stdout,
            "Privilege names:\n"
            "=================================================================="
            "==============\n");

    for (i = 0; i < numPrivileges; i++)
    {
        ntStatus = LsaLookupPrivilegeName(
                                  hLsa,
                                  hPolicy,
                                  &pPrivileges[i],
                                  &pwszPrivilegeName);
        BAIL_ON_NT_STATUS(ntStatus);

        err = LwWc16sToMbs(pwszPrivilegeName,
                           &pszPrivilegeName);
        BAIL_ON_LSA_ERROR(err);

        fprintf(stdout, "{%d,%u} %s\n",
                pPrivileges[i].HighPart,
                pPrivileges[i].LowPart,
                pszPrivilegeName);

        if (pwszPrivilegeName)
        {
            LsaRpcFreeMemory(pwszPrivilegeName);
            pwszPrivilegeName = NULL;
        }

        LW_SAFE_FREE_MEMORY(pszPrivilegeName);
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

    if (pwszPrivilegeName)
    {
        LsaRpcFreeMemory(pwszPrivilegeName);
    }

    LW_SAFE_FREE_MEMORY(pPrivileges);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
ProcessLookupPrivilegeDescriptions(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR PrivilegeNames
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
    PSTR *pPrivileges = NULL;
    DWORD numPrivileges = 0;
    PWSTR privilegeName = NULL;
    SHORT ClientLanguage = 0;
    SHORT ClientSystemLanguage = 0;
    PWSTR pwszPrivilegeDescription = NULL;
    USHORT Language = 0;
    DWORD i = 0;
    PSTR pszPrivilegeDescription = NULL;

    err = CreateRpcCredentials(pRpcParams,
                               &pCreds);
    BAIL_ON_LSA_ERROR(err);

    err = CreateLsaRpcBinding(pRpcParams,
                              pCreds,
                              &hLsa);
    BAIL_ON_LSA_ERROR(err);

    err = GetStringListFromString(
                             PrivilegeNames,
                             SEPARATOR_CHAR,
                             &pPrivileges,
                             &numPrivileges);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = LsaOpenPolicy2(hLsa,
                              wszSysName,
                              NULL,
                              policyAccessMask,
                              &hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    fprintf(stdout,
            "Privilege descriptions:\n"
            "=================================================================="
            "==============\n");

    for (i = 0; i < numPrivileges; i++)
    {
        err = LwMbsToWc16s(pPrivileges[i],
                           &privilegeName);
        BAIL_ON_LSA_ERROR(err);

        ntStatus = LsaLookupPrivilegeDisplayName(
                                  hLsa,
                                  hPolicy,
                                  privilegeName,
                                  ClientLanguage,
                                  ClientSystemLanguage,
                                  &pwszPrivilegeDescription,
                                  &Language);
        BAIL_ON_NT_STATUS(ntStatus);

        err = LwWc16sToMbs(pwszPrivilegeDescription,
                           &pszPrivilegeDescription);
        BAIL_ON_LSA_ERROR(err);

        fprintf(stdout, "%s\n", pszPrivilegeDescription);

        if (pwszPrivilegeDescription)
        {
            LsaRpcFreeMemory(pwszPrivilegeDescription);
            pwszPrivilegeDescription = NULL;
        }

        LW_SAFE_FREE_MEMORY(privilegeName);
        LW_SAFE_FREE_MEMORY(pszPrivilegeDescription);
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

    LW_SAFE_FREE_MEMORY(privilegeName);
    LW_SAFE_FREE_MEMORY(pszPrivilegeDescription);

    if (pwszPrivilegeDescription)
    {
        LsaRpcFreeMemory(pwszPrivilegeDescription);
    }

    for (i = 0; i < numPrivileges; i++)
    {
        LW_SAFE_FREE_MEMORY(pPrivileges[i]);
    }
    LW_SAFE_FREE_MEMORY(pPrivileges);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
ProcessEnumerateAccountPrivileges(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR pszAccountName
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_BINDING hLsa = NULL;
    LW_PIO_CREDS pCreds = NULL;
    PSID pAccountSid = NULL;
    WCHAR wszSysName[] = {'\\', '\\', '\0'};
    DWORD policyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    POLICY_HANDLE hPolicy = NULL;
    DWORD accountAccessMask = LSA_ACCOUNT_VIEW;
    LSAR_ACCOUNT_HANDLE hAccount = NULL;
    PPRIVILEGE_SET pPrivileges = NULL;
    DWORD i = 0;
    PWSTR pwszPrivilegeName = NULL;
    PSTR pszPrivilegeName = NULL;

    err = CreateRpcCredentials(pRpcParams,
                               &pCreds);
    BAIL_ON_LSA_ERROR(err);

    err = CreateLsaRpcBinding(pRpcParams,
                              pCreds,
                              &hLsa);
    BAIL_ON_LSA_ERROR(err);

    err = ResolveAccountNameToSid(
                          hLsa,
                          pszAccountName,
                          &pAccountSid);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = LsaOpenPolicy2(hLsa,
                              wszSysName,
                              NULL,
                              policyAccessMask,
                              &hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaOpenAccount(hLsa,
                              hPolicy,
                              pAccountSid,
                              accountAccessMask,
                              &hAccount);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaEnumPrivilegesAccount(hLsa,
                                        hAccount,
                                        &pPrivileges);
    BAIL_ON_NT_STATUS(ntStatus);

    fprintf(stdout,
            "%s Privileges:\n"
            "=================================================================="
            "==============\n", pszAccountName);

    for (i = 0; i < pPrivileges->PrivilegeCount; i++)
    {
        ntStatus = LsaLookupPrivilegeName(
                              hLsa,
                              hPolicy,
                              &pPrivileges->Privilege[i].Luid,
                              &pwszPrivilegeName);
        BAIL_ON_NT_STATUS(ntStatus);

        err = LwWc16sToMbs(pwszPrivilegeName,
                           &pszPrivilegeName);
        BAIL_ON_LSA_ERROR(err);

        fprintf(stdout, "{%d,%u} %s",
                pPrivileges->Privilege[i].Luid.HighPart,
                pPrivileges->Privilege[i].Luid.LowPart,
                pszPrivilegeName);

        if (pPrivileges->Privilege[i].Attributes & SE_PRIVILEGE_ENABLED)
        {
            fprintf(stdout, "\t\tSE_PRIVILEGE_ENABLED ");
        }
        if (pPrivileges->Privilege[i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)
        {
            fprintf(stdout, "\t\tSE_PRIVILEGE_ENABLED_BY_DEFAULT ");
        }
        fprintf(stdout, "\n");

        if (pwszPrivilegeName)
        {
            LsaRpcFreeMemory(pwszPrivilegeName);
            pwszPrivilegeName = NULL;
        }

        LW_SAFE_FREE_MEMORY(pszPrivilegeName);
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

    if (hAccount)
    {
        LsaClose(hLsa, hAccount);
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

    RTL_FREE(&pAccountSid);

    if (pPrivileges)
    {
        LsaRpcFreeMemory(pPrivileges);
    }

    if (pwszPrivilegeName)
    {
        LsaRpcFreeMemory(pwszPrivilegeName);
    }

    LW_SAFE_FREE_MEMORY(pszPrivilegeName);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
ProcessAddRemovePrivilegesAccount(
    IN PRPC_PARAMETERS pRpcParams,
    IN BOOLEAN Add,
    IN PSTR AccountName,
    IN BOOLEAN CreateAccount,
    IN PSTR PrivilegesString,
    IN BOOLEAN RemoveAll
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_BINDING hLsa = NULL;
    LW_PIO_CREDS pCreds = NULL;
    PSID pAccountSid = NULL;
    WCHAR wszSysName[] = {'\\', '\\', '\0'};
    DWORD policyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS | 
                             LSA_ACCESS_CREATE_PRIVILEGE |
                             LSA_ACCESS_CREATE_SPECIAL_ACCOUNTS;
    POLICY_HANDLE hPolicy = NULL;
    DWORD accountAccessMask = LSA_ACCOUNT_VIEW | 
                              LSA_ACCOUNT_ADJUST_PRIVILEGES;
    LSAR_ACCOUNT_HANDLE hAccount = NULL;
    PSTR *privileges = NULL;
    DWORD numPrivileges = 0;
    DWORD i = 0;
    DWORD privilegeSetSize = 0;
    PWSTR pwszPrivilegeName = NULL;
    PPRIVILEGE_SET pPrivilegeSet = NULL;

    err = CreateRpcCredentials(pRpcParams,
                               &pCreds);
    BAIL_ON_LSA_ERROR(err);

    err = CreateLsaRpcBinding(pRpcParams,
                              pCreds,
                              &hLsa);
    BAIL_ON_LSA_ERROR(err);

    err = ResolveAccountNameToSid(
                          hLsa,
                          AccountName,
                          &pAccountSid);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = LsaOpenPolicy2(hLsa,
                              wszSysName,
                              NULL,
                              policyAccessMask,
                              &hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaOpenAccount(hLsa,
                              hPolicy,
                              pAccountSid,
                              accountAccessMask,
                              &hAccount);
    if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND &&
        CreateAccount)
    {
        ntStatus = LsaCreateAccount(hLsa,
                                    hPolicy,
                                    pAccountSid,
                                    accountAccessMask,
                                    &hAccount);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else if (ntStatus != STATUS_SUCCESS)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (PrivilegesString)
    {
        err = GetStringListFromString(
                             PrivilegesString,
                             SEPARATOR_CHAR,
                             &privileges,
                             &numPrivileges);
        BAIL_ON_LSA_ERROR(err);
    }

    if (numPrivileges)
    {
        privilegeSetSize = RtlLengthRequiredPrivilegeSet(numPrivileges);
        err = LwAllocateMemory(privilegeSetSize,
                               OUT_PPVOID(&pPrivilegeSet));
        BAIL_ON_LSA_ERROR(err);

        for (i = 0; i < numPrivileges; i++)
        {
            LUID privilegeValue = {0};

            err = LwMbsToWc16s(privileges[i],
                               &pwszPrivilegeName);
            BAIL_ON_LSA_ERROR(err);

            ntStatus = LsaLookupPrivilegeValue(
                                  hLsa,
                                  hPolicy,
                                  pwszPrivilegeName,
                                  &privilegeValue);
            BAIL_ON_NT_STATUS(ntStatus);

            pPrivilegeSet->Privilege[i].Luid = privilegeValue;

            LW_SAFE_FREE_MEMORY(pwszPrivilegeName);
        }

        pPrivilegeSet->PrivilegeCount = numPrivileges;
    }

    if (Add)
    {
        ntStatus = LsaAddPrivilegesToAccount(
                             hLsa,
                             hAccount,
                             pPrivilegeSet);
        BAIL_ON_NT_STATUS(ntStatus);

        fprintf(stdout, "Successfully added privileges to %s\n", AccountName);
    }
    else
    {
        ntStatus = LsaRemovePrivilegesFromAccount(
                             hLsa,
                             hAccount,
                             RemoveAll,
                             pPrivilegeSet);
        BAIL_ON_NT_STATUS(ntStatus);

        fprintf(stdout, "Successfully removed privileges from %s\n", AccountName);
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

    if (hAccount)
    {
        LsaClose(hLsa, hAccount);
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

    RTL_FREE(&pAccountSid);
    LW_SAFE_FREE_MEMORY(pwszPrivilegeName);
    LW_SAFE_FREE_MEMORY(pPrivilegeSet);

    if (privileges)
    {
        for (i = 0; i < numPrivileges; i++)
        {
            LW_SAFE_FREE_MEMORY(privileges[i]);
        }
        LW_SAFE_FREE_MEMORY(privileges);
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}
