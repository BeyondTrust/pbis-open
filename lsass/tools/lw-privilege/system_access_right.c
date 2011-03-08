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
 *        system_access_right.c
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
ProcessGetAccountSystemAccessRights(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR pszAccountName
    );


VOID
ShowUsageSystemAccessRights(
    PCSTR pszProgramName
    )
{
    fprintf(stdout, "Usage: %s sar <command> [options]\n\n", pszProgramName);
    fprintf(stdout, "command:\n");
    fprintf(stdout, "-g, --get - Enumerate Privileges available in the system\n");
}


DWORD
ProcessSystemAccessRight(
    const DWORD Argc,
    PCSTR* Argv
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD i = 0;
    PCSTR pszArg = NULL;
    RPC_PARAMETERS Params = {0};
    PRIVILEGE_COMMAND Command = PrivilegeDoNothing;
    PSTR pszAccountName = NULL;

    err = ProcessRpcParameters(Argc, Argv, &Params);
    BAIL_ON_LSA_ERROR(err);

    for (i = 0; i < Argc; i++)
    {
        pszArg = Argv[i];

        if (((strcmp(pszArg, "-g") == 0) ||
             (strcmp(pszArg, "--get") == 0)) &&
            (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &pszAccountName);
            BAIL_ON_LSA_ERROR(err);

            Command = SystemAccessRightGetAccount;
        }
    }

    switch (Command)
    {
    case SystemAccessRightGetAccount:
        err = ProcessGetAccountSystemAccessRights(
                                &Params,
                                pszAccountName);
        BAIL_ON_LSA_ERROR(err);
        break;

    default:
        ShowUsageSystemAccessRights(Argv[0]);
        break;
    }

error:
    LW_SAFE_FREE_MEMORY(pszAccountName);

    return err;
}


static
DWORD
ProcessGetAccountSystemAccessRights(
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
    DWORD policyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS | 
                             LSA_ACCESS_CREATE_PRIVILEGE |
                             LSA_ACCESS_CREATE_SPECIAL_ACCOUNTS;
    POLICY_HANDLE hPolicy = NULL;
    DWORD accountAccessMask = LSA_ACCOUNT_VIEW;
    LSAR_ACCOUNT_HANDLE hAccount = NULL;
    DWORD systemAccess = 0;

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

    ntStatus = LsaGetSystemAccessAccount(
                              hLsa,
                              hAccount,
                              &systemAccess);
    BAIL_ON_NT_STATUS(ntStatus);

    fprintf(stdout,
            "Account: %s:\n"
            "=================================================================="
            "==============\n", pszAccountName);
    fprintf(stdout, "System Access Rights 0x%08x\n", systemAccess);

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

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}
