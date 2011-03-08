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
 *        common.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        LSA Privileges, System Access Rights and Account Rights manager
 *        (rpc client utility).
 *        Common routines.
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


DWORD
ProcessRpcParameters(
    IN const DWORD Argc,
    IN PCSTR* Argv,
    OUT PRPC_PARAMETERS pParams
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD i = 0;
    PCSTR pszArg = NULL;
    PSTR pszHost = NULL;
    PSTR pszBindingString = NULL;
    PSTR pszKrb5Principal = NULL;
    PSTR pszKrb5Cache = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszUserName = NULL;
    PSTR pszPassword = NULL;

    for (i = 0; i < Argc; i++)
    {
        pszArg = Argv[i];

        if (((strcmp(pszArg, "-h") == 0) ||
             (strcmp(pszArg, "--host") == 0)) &&
            (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &pszHost);
            BAIL_ON_LSA_ERROR(err);
        }
        else if (((strcmp(pszArg, "-b") == 0) ||
                  (strcmp(pszArg, "--binding-string") == 0)) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &pszBindingString);
            BAIL_ON_LSA_ERROR(err);
        }
        else if (((strcmp(pszArg, "-r") == 0) ||
                  (strcmp(pszArg, "--krb5-principal") == 0)) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &pszKrb5Principal);
            BAIL_ON_LSA_ERROR(err);
        }
        else if (((strcmp(pszArg, "-c") == 0) ||
                  (strcmp(pszArg, "--krb5-cache") == 0)) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &pszKrb5Cache);
            BAIL_ON_LSA_ERROR(err);
        }
        else if (((strcmp(pszArg, "-d") == 0) ||
                  (strcmp(pszArg, "--domain") == 0)) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &pszDomainName);
            BAIL_ON_LSA_ERROR(err);
        }
        else if (((strcmp(pszArg, "-u") == 0) ||
                  (strcmp(pszArg, "--username") == 0)) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &pszUserName);
            BAIL_ON_LSA_ERROR(err);
        }
        else if (((strcmp(pszArg, "-p") == 0) ||
                  (strcmp(pszArg, "--password") == 0)) &&
                 (i + 1 < Argc))
        {
            pszArg = Argv[++i];
            err = LwAllocateString(pszArg, &pszPassword);
            BAIL_ON_LSA_ERROR(err);
        }
    }

    pParams->pszHost          = pszHost;
    pParams->pszBindingString = pszBindingString;

    if (pszKrb5Principal)
    {
        pParams->Credentials = CredsKrb5;

        pParams->Creds.Krb5.pszPrincipal = pszKrb5Principal;
        pParams->Creds.Krb5.pszCache     = pszKrb5Cache;
    }
    else if (pszUserName)
    {
        pParams->Credentials = CredsNtlm;

        pParams->Creds.Ntlm.pszDomainName = pszDomainName;
        pParams->Creds.Ntlm.pszUserName   = pszUserName;
        pParams->Creds.Ntlm.pszPassword   = pszPassword;
    }

error:
    return err;
}


VOID
ReleaseRpcParameters(
    PRPC_PARAMETERS pParams
    )
{
    LW_SAFE_FREE_MEMORY(pParams->pszHost);
    LW_SAFE_FREE_MEMORY(pParams->pszBindingString);

    switch (pParams->Credentials)
    {
    case CredsKrb5:
        LW_SAFE_FREE_MEMORY(pParams->Creds.Krb5.pszPrincipal);
        LW_SAFE_FREE_MEMORY(pParams->Creds.Krb5.pszCache);
        break;

    case CredsNtlm:
        LW_SAFE_FREE_MEMORY(pParams->Creds.Ntlm.pszDomainName);
        LW_SAFE_FREE_MEMORY(pParams->Creds.Ntlm.pszUserName);
        LW_SAFE_FREE_MEMORY(pParams->Creds.Ntlm.pszPassword);
        break;

    default:
        break;
    }
}


DWORD
CreateRpcCredentials(
    IN PRPC_PARAMETERS pRpcParams,
    OUT LW_PIO_CREDS *ppCreds
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LW_PIO_CREDS pCreds = NULL;

    if (pRpcParams->Credentials == CredsNtlm)
    {
        ntStatus = LwIoCreatePlainCredsA(
                          pRpcParams->Creds.Ntlm.pszUserName,
                          pRpcParams->Creds.Ntlm.pszDomainName,
                          pRpcParams->Creds.Ntlm.pszPassword,
                          &pCreds);
        if (ntStatus)
        {
            fprintf(stderr, "Failed to create NTLM credentials\n");
        }
    }
    else if (pRpcParams->Credentials == CredsKrb5)
    {
        ntStatus = LwIoCreateKrb5CredsA(
                          pRpcParams->Creds.Krb5.pszPrincipal,
                          pRpcParams->Creds.Krb5.pszCache,
                          &pCreds);
        if (ntStatus)
        {
            fprintf(stderr, "Failed to create KRB5 credentials\n");
        }
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppCreds = pCreds;

error:
    if (err)
    {
        if (pCreds)
        {
            LwIoDeleteCreds(pCreds);
        }

        *ppCreds = NULL;
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


DWORD
CreateLsaRpcBinding(
    IN PRPC_PARAMETERS pRpcParams,
    IN LW_PIO_CREDS pCreds,
    OUT LSA_BINDING *phLsa
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszHost = NULL;
    PWSTR pwszBindingString = NULL;
    LSA_BINDING hLsa = NULL;

    if (pRpcParams->pszBindingString)
    {
        err = LwMbsToWc16s(pRpcParams->pszBindingString,
                           &pwszBindingString);
        BAIL_ON_LSA_ERROR(err);

        ntStatus = LsaInitBindingFromBindingString(
                               &hLsa,
                               pwszBindingString,
                               pCreds);
    }
    else
    {
        if (pRpcParams->pszHost)
        {
            err = LwMbsToWc16s(pRpcParams->pszHost,
                               &pwszHost);
            BAIL_ON_LSA_ERROR(err);
        }

        ntStatus = LsaInitBindingDefault(
                               &hLsa,
                               pwszHost,
                               pCreds);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *phLsa = hLsa;

error:
    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


DWORD
ResolveAccountNameToSid(
    IN LSA_BINDING hLsa,
    IN PSTR pszAccountName,
    OUT PSID *ppAccountSid
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pAccountSid = NULL;
    WCHAR wszSysName[] = {'\\', '\\', '\0'};
    DWORD policyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    POLICY_HANDLE hPolicy = NULL;
    PWSTR pwszAccountName = NULL;
    RefDomainList *pDomainList = NULL;
    TranslatedSid3 *pTransSid = NULL;
    DWORD count = 0;

    ntStatus = RtlAllocateSidFromCString(
                          &pAccountSid,
                          pszAccountName);
    if (ntStatus == STATUS_INVALID_SID)
    {
        ntStatus = LsaOpenPolicy2(hLsa,
                                  wszSysName,
                                  NULL,
                                  policyAccessMask,
                                  &hPolicy);
        BAIL_ON_NT_STATUS(ntStatus);

        err = LwMbsToWc16s(pszAccountName,
                           &pwszAccountName);
        BAIL_ON_LSA_ERROR(err);

        ntStatus = LsaLookupNames3(hLsa,
                                   hPolicy,
                                   1,
                                   &pwszAccountName,
                                   &pDomainList,
                                   &pTransSid,
                                   LSA_LOOKUP_NAMES_ALL,
                                   &count);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RtlDuplicateSid(&pAccountSid,
                                   pTransSid[0].sid);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppAccountSid = pAccountSid;

error:
    if (err)
    {
        if (ppAccountSid)
        {
            *ppAccountSid = NULL;
        }
        RTL_FREE(&pAccountSid);
    }

    if (hPolicy)
    {
        LsaClose(hLsa, hPolicy);
    }

    LW_SAFE_FREE_MEMORY(pwszAccountName);

    if (pDomainList)
    {
        LsaRpcFreeMemory(pDomainList);
    }

    if (pTransSid)
    {
        LsaRpcFreeMemory(pTransSid);
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


DWORD
GetStringListFromString(
    PSTR pszStr,
    CHAR Separator,
    PSTR **pppszList,
    PDWORD pNumElements
    )
{
    DWORD err = ERROR_SUCCESS;
    PSTR *ppszList = NULL;
    PSTR pszString = NULL;
    DWORD i = 0;
    size_t stringLen = 0;
    DWORD numElements = 1;
    PSTR pszElement = NULL;

    err = LwAllocateString(pszStr, &pszString);
    BAIL_ON_LSA_ERROR(err);

    stringLen = strlen(pszString);
    for (i = 0; i < stringLen; i++)
    {
        if (pszString[i] == Separator)
        {
            numElements++;
        }
    }

    err = LwAllocateMemory(sizeof(PSTR) * numElements,
                               OUT_PPVOID(&ppszList));
    BAIL_ON_LSA_ERROR(err);

    pszElement  = pszString;
    numElements = 0;

    for (i = 0; i < stringLen; i++)
    {
        if (pszString[i] != Separator)
        {
            continue;
        }

        pszString[i] = '\0';

        err = LwAllocateString(pszElement,
                                   &(ppszList[numElements++]));
        BAIL_ON_LSA_ERROR(err);

        pszElement = &(pszString[i + 1]);
    }

    // Copy the last element (it doesn't end with the separator)
    err = LwAllocateString(pszElement,
                               &(ppszList[numElements++]));
    BAIL_ON_LSA_ERROR(err);

    *pppszList    = ppszList;
    *pNumElements = numElements;

error:
    if (err)
    {
        for (i = 0; ppszList && i < numElements; i++)
        {
            LW_SAFE_FREE_MEMORY(ppszList[i]);
        }
        LW_SAFE_FREE_MEMORY(ppszList);

        *pppszList    = NULL;
        *pNumElements = 0;
    }

    LW_SAFE_FREE_MEMORY(pszString);

    return err;
}


DWORD
GetLuidListFromString(
    IN PSTR pszStr,
    PLUID *ppLuidList,
    PDWORD pNumElements
    )
{
    DWORD err = ERROR_SUCCESS;
    PSTR *ppszList = NULL;
    DWORD numElements = 0;
    PLUID pLuidList = NULL;
    DWORD i = 0;

    err = GetStringListFromString(
                        pszStr,
                        LUID_SEPARATOR_CHAR,
                        &ppszList,
                        &numElements);
    BAIL_ON_LSA_ERROR(err);

    err = LwAllocateMemory(
                        sizeof(pLuidList[0]) * numElements,
                        OUT_PPVOID(&pLuidList));
    BAIL_ON_LSA_ERROR(err);

    for (i = 0; i < numElements; i++)
    {
        if (sscanf(ppszList[i],
                   "{%d,%u}",
                   &pLuidList[i].HighPart,
                   &pLuidList[i].LowPart) != 2)
        {
            err = ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(err);
        }
    }

    *ppLuidList = pLuidList;
    *pNumElements = numElements;

error:
    for (i = 0; i < numElements; i++)
    {
        LW_SAFE_FREE_MEMORY(ppszList[i]);
    }
    LW_SAFE_FREE_MEMORY(ppszList);

    return err;
}
