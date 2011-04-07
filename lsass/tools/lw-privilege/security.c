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
 *        security.c
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
ProcessGetAccountSecurityDescriptor(
    IN PRPC_PARAMETERS pRpcParams,
    IN OPTIONAL SECURITY_INFORMATION securityInfo,
    IN PSTR pszAccountName
    );

static
DWORD
ProcessSetAccountSecurityDescriptor(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR pszSecurityDesc,
    IN PSTR pszAccountName
    );


VOID
ShowUsageSecurity(
    PCSTR pszProgramName
    )
{
    fprintf(stdout, "Usage: %s security <command> [options]\n\n", pszProgramName);
    fprintf(stdout, "command:\n");
    fprintf(stdout, "-g, --get - Get lsa account security descriptor\n");
    fprintf(stdout, "-s, --set <secdesc> - Set lsa account security descriptor\n");
    fprintf(stdout, "options:\n");
    fprintf(stdout, "--owner - Query owner\n");
    fprintf(stdout, "--group - Query group\n");
    fprintf(stdout, "--dacl  - Query DACL\n");
    fprintf(stdout, "--sacl  - Query SACL\n");
}


DWORD
ProcessSecurity(
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
    PSTR pszSecurityDesc = NULL;
    SECURITY_INFORMATION securityInfo = 0;

    err = ProcessRpcParameters(Argc, Argv, &Params);
    BAIL_ON_LSA_ERROR(err);

    for (i = 0; i < Argc; i++)
    {
        pszArg = Argv[i];

        if ((strcmp(pszArg, "-g") == 0) ||
            (strcmp(pszArg, "--get") == 0))
        {
            Command = SecurityGetAccountDescriptor;
        }
        if (((strcmp(pszArg, "-s") == 0) ||
             (strcmp(pszArg, "--set") == 0)) &&
            (i + 1 <= Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &pszSecurityDesc);
            BAIL_ON_LSA_ERROR(err);

            Command = SecuritySetAccountDescriptor;
        }
        else if (strcmp(pszArg, "--owner") == 0)
        {
            securityInfo |= OWNER_SECURITY_INFORMATION;
        }
        else if (strcmp(pszArg, "--group") == 0)
        {
            securityInfo |= GROUP_SECURITY_INFORMATION;
        }
        else if (strcmp(pszArg, "--dacl") == 0)
        {
            securityInfo |= DACL_SECURITY_INFORMATION;
        }
        else if (strcmp(pszArg, "--sacl") == 0)
        {
            securityInfo |= SACL_SECURITY_INFORMATION;
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
    case SecurityGetAccountDescriptor:
        err = ProcessGetAccountSecurityDescriptor(
                                &Params,
                                securityInfo,
                                accountName);
        BAIL_ON_LSA_ERROR(err);
        break;

    case SecuritySetAccountDescriptor:
        err = ProcessSetAccountSecurityDescriptor(
                                &Params,
                                pszSecurityDesc,
                                accountName);
        BAIL_ON_LSA_ERROR(err);
        break;

    default:
        ShowUsageSecurity(Argv[0]);
        break;
    }

error:
    LW_SAFE_FREE_MEMORY(accountName);
    LW_SAFE_FREE_MEMORY(pszSecurityDesc);

    return err;
}


static
DWORD
ProcessGetAccountSecurityDescriptor(
    IN PRPC_PARAMETERS pRpcParams,
    IN OPTIONAL SECURITY_INFORMATION securityInfo,
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
    DWORD accountAccessMask = LSA_ACCOUNT_VIEW |
                              READ_CONTROL;
    LSAR_ACCOUNT_HANDLE hAccount = NULL;
    DWORD securityInformation = OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION | 
                                DACL_SECURITY_INFORMATION;
                                
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescRelative = NULL;
    DWORD securityDescRelativeSize = 0;
    PSTR pszSecurityDescriptor = NULL;

    err = CreateRpcCredentials(pRpcParams,
                               &pCreds);
    BAIL_ON_LSA_ERROR(err);

    err = CreateLsaRpcBinding(pRpcParams,
                              pCreds,
                              &hLsa);
    BAIL_ON_LSA_ERROR(err);

    //
    // Query selected security information if provided or
    // Owner,Group and DACL otherwise.
    //
    if (securityInfo)
    {
        securityInformation = securityInfo;
    }

    // If no account name is passed then query security descriptor
    // of the policy server
    if (pszAccountName)
    {
        err = ResolveAccountNameToSid(
                              hLsa,
                              pszAccountName,
                              &pAccountSid);
        BAIL_ON_LSA_ERROR(err);
    }
    else
    {
        // More access rights are going to be needed if we want
        // to query lsa policy security descriptor
        policyAccessMask |= MAXIMUM_ALLOWED;
    }

    ntStatus = LsaOpenPolicy2(hLsa,
                              wszSysName,
                              NULL,
                              policyAccessMask,
                              &hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pAccountSid)
    {
        ntStatus = LsaOpenAccount(hLsa,
                                  hPolicy,
                                  pAccountSid,
                                  accountAccessMask,
                                  &hAccount);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LsaQuerySecurity(
                                  hLsa,
                                  hAccount,
                                  securityInformation,
                                  &pSecurityDescRelative,
                                  &securityDescRelativeSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = LsaQuerySecurity(
                                  hLsa,
                                  hPolicy,
                                  securityInformation,
                                  &pSecurityDescRelative,
                                  &securityDescRelativeSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = RtlAllocateSddlCStringFromSecurityDescriptor(
                              &pszSecurityDescriptor,
                              pSecurityDescRelative,
                              SDDL_REVISION,
                              securityInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pAccountSid)
    {
        fprintf(stdout, "Account: %s\n", pszAccountName);
    }
    else
    {
        fprintf(stdout, "LSA Policy:\n");
    }
    fprintf(stdout,
            "=================================================="
            "==============================\n");
    fprintf(stdout, "Security descriptor: %s\n", pszSecurityDescriptor);

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

    if (pSecurityDescRelative)
    {
        LsaRpcFreeMemory(pSecurityDescRelative);
    }

    RTL_FREE(&pszSecurityDescriptor);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
ProcessSetAccountSecurityDescriptor(
    IN PRPC_PARAMETERS pRpcParams,
    IN PSTR pszSecurityDesc,
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
    DWORD accountAccessMask = LSA_ACCOUNT_VIEW |
                              READ_CONTROL |
                              WRITE_DAC |
                              WRITE_OWNER;
    LSAR_ACCOUNT_HANDLE hAccount = NULL;
    SECURITY_INFORMATION securityInformation = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescRelative = NULL;
    DWORD securityDescRelativeSize = 0;
    PSTR pszSecurityDescriptor = NULL;

    err = CreateRpcCredentials(pRpcParams,
                               &pCreds);
    BAIL_ON_LSA_ERROR(err);

    err = CreateLsaRpcBinding(pRpcParams,
                              pCreds,
                              &hLsa);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlAllocateSecurityDescriptorFromSddlCString(
                        &pSecurityDescRelative,
                        &securityDescRelativeSize,
                        pszSecurityDesc,
                        SDDL_REVISION);
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

    ntStatus = RtlAllocateSecurityDescriptorFromSddlCString(
                        &pSecurityDescRelative,
                        &securityDescRelativeSize,
                        pszSecurityDesc,
                        SDDL_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlGetSecurityInformationFromSddlCString(
                        pszSecurityDesc,
                        &securityInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    // If no account name is passed then query security descriptor
    // of the policy server
    if (pszAccountName)
    {
        err = ResolveAccountNameToSid(
                              hLsa,
                              pszAccountName,
                              &pAccountSid);
        BAIL_ON_LSA_ERROR(err);
    }
    else
    {
        // More access rights are going to be needed if we want
        // to query lsa policy security descriptor
        policyAccessMask |= MAXIMUM_ALLOWED;
    }

    ntStatus = LsaOpenPolicy2(hLsa,
                              wszSysName,
                              NULL,
                              policyAccessMask,
                              &hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pAccountSid)
    {
        ntStatus = LsaOpenAccount(hLsa,
                                  hPolicy,
                                  pAccountSid,
                                  accountAccessMask,
                                  &hAccount);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LsaSetSecurity(
                                  hLsa,
                                  hAccount,
                                  securityInformation,
                                  pSecurityDescRelative,
                                  securityDescRelativeSize);
        BAIL_ON_NT_STATUS(ntStatus);

        LW_SAFE_FREE_MEMORY(pSecurityDescRelative);
        securityDescRelativeSize = 0;

        ntStatus = LsaQuerySecurity(
                                  hLsa,
                                  hPolicy,
                                  securityInformation,
                                  &pSecurityDescRelative,
                                  &securityDescRelativeSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = LsaSetSecurity(
                                  hLsa,
                                  hPolicy,
                                  securityInformation,
                                  pSecurityDescRelative,
                                  securityDescRelativeSize);
        BAIL_ON_NT_STATUS(ntStatus);

        LW_SAFE_FREE_MEMORY(pSecurityDescRelative);
        securityDescRelativeSize = 0;

        ntStatus = LsaQuerySecurity(
                                  hLsa,
                                  hPolicy,
                                  securityInformation,
                                  &pSecurityDescRelative,
                                  &securityDescRelativeSize);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = RtlAllocateSddlCStringFromSecurityDescriptor(
                              &pszSecurityDescriptor,
                              pSecurityDescRelative,
                              SDDL_REVISION,
                              securityInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pAccountSid)
    {
        fprintf(stdout, "Account: %s\n", pszAccountName);
    }
    else
    {
        fprintf(stdout, "LSA Policy:\n");
    }
    fprintf(stdout,
            "=================================================="
            "==============================\n");
    fprintf(stdout, "Updated security descriptor: %s\n", pszSecurityDescriptor);

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

    if (pSecurityDescRelative)
    {
        LsaRpcFreeMemory(pSecurityDescRelative);
    }

    RTL_FREE(&pszSecurityDescriptor);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}
