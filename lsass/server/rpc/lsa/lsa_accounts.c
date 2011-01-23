/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsa_accounts.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Accounts selection functions (by name or SID) for use before
 *        doing lookups in remote lsa or local samr rpc servers
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvParseAccountName(
    PWSTR pwszName,
    PWSTR *ppwszDomainName,
    PWSTR *ppwszAcctName
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszCursor = NULL;
    PWSTR pwszDomainName = NULL;
    DWORD dwDomainNameLen = 0;
    PWSTR pwszAcctName = NULL;
    DWORD dwAcctNameLen = 0;

    pwszCursor = pwszName;

    while ((*pwszCursor) &&
           (*pwszCursor) != (WCHAR)'\\') pwszCursor++;

    if ((*pwszCursor) == (WCHAR)'\\')
    {
        dwDomainNameLen = (DWORD)(pwszCursor - pwszName);
        dwError = LwAllocateMemory((dwDomainNameLen + 1) * sizeof(WCHAR),
                                   OUT_PPVOID(&pwszDomainName));
        BAIL_ON_LSA_ERROR(dwError);

        wc16sncpy(pwszDomainName, pwszName, dwDomainNameLen);
        pwszCursor++;

    }
    else
    {
        pwszCursor = pwszName;
    }

    dwAcctNameLen = wc16slen(pwszCursor);
    dwError = LwAllocateMemory((dwAcctNameLen + 1) * sizeof(WCHAR),
                               OUT_PPVOID(&pwszAcctName));
    BAIL_ON_LSA_ERROR(dwError);

    wc16sncpy(pwszAcctName, pwszCursor, dwAcctNameLen);

    *ppwszDomainName = pwszDomainName;
    *ppwszAcctName   = pwszAcctName;

cleanup:
    if (dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    LW_SAFE_FREE_MEMORY(pwszDomainName);
    LW_SAFE_FREE_MEMORY(pwszAcctName);

    *ppwszDomainName = NULL;
    *ppwszAcctName = NULL;

    goto cleanup;
}


NTSTATUS
LsaSrvSelectAccountsByDomainName(
    PPOLICY_CONTEXT   pPolCtx,
    UNICODE_STRING   *pNames,
    DWORD             dwNumNames,
    PACCOUNT_NAMES   *ppAccountNames
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    ACCOUNT_NAMES DomainAccounts = {0};
    ACCOUNT_NAMES ForDomainAccounts = {0};
    ACCOUNT_NAMES LocalAccounts = {0};
    ACCOUNT_NAMES BuiltinAccounts = {0};
    ACCOUNT_NAMES OtherAccounts = {0};
    DWORD dwDomainNamesNum = 0;
    DWORD dwForDomainNamesNum = 0;
    DWORD dwLocalNamesNum = 0;
    DWORD dwBuiltinNamesNum = 0;
    DWORD dwOtherNamesNum = 0;
    PACCOUNT_NAMES pAccountNames = NULL;
    DWORD dwAccountTypesNum = LSA_ACCOUNT_TYPE_SENTINEL;
    DWORD i = 0;
    PWSTR pwszName = NULL;
    PWSTR pwszDomainName = NULL;
    WCHAR wszBuiltinDomainName[] = LSA_BUILTIN_DOMAIN_NAME;
    PWSTR pwszAcctName = NULL;

    dwError = LwAllocateMemory(sizeof(*pAccountNames) * dwAccountTypesNum,
                               OUT_PPVOID(&pAccountNames));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*DomainAccounts.ppwszNames) * dwNumNames,
                               OUT_PPVOID(&DomainAccounts.ppwszNames));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*DomainAccounts.pdwIndices) * dwNumNames,
                               OUT_PPVOID(&DomainAccounts.pdwIndices));
    BAIL_ON_LSA_ERROR(dwError);

    memset(DomainAccounts.pdwIndices, -1, sizeof(DWORD) * dwNumNames);

    dwError = LwAllocateMemory(
                             sizeof(*ForDomainAccounts.ppwszNames) * dwNumNames,
                             OUT_PPVOID(&ForDomainAccounts.ppwszNames));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                             sizeof(*ForDomainAccounts.pdwIndices) * dwNumNames,
                             OUT_PPVOID(&ForDomainAccounts.pdwIndices));
    BAIL_ON_LSA_ERROR(dwError);

    memset(ForDomainAccounts.pdwIndices, -1, sizeof(DWORD) * dwNumNames);

    dwError = LwAllocateMemory(sizeof(*LocalAccounts.ppwszNames) * dwNumNames,
                               OUT_PPVOID(&LocalAccounts.ppwszNames));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*LocalAccounts.pdwIndices) * dwNumNames,
                               OUT_PPVOID(&LocalAccounts.pdwIndices));
    BAIL_ON_LSA_ERROR(dwError);

    memset(LocalAccounts.pdwIndices, -1, sizeof(DWORD) * dwNumNames);

    dwError = LwAllocateMemory(sizeof(*BuiltinAccounts.ppwszNames) * dwNumNames,
                               OUT_PPVOID(&BuiltinAccounts.ppwszNames));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*BuiltinAccounts.pdwIndices) * dwNumNames,
                               OUT_PPVOID(&BuiltinAccounts.pdwIndices));
    BAIL_ON_LSA_ERROR(dwError);

    memset(BuiltinAccounts.pdwIndices, -1, sizeof(DWORD) * dwNumNames);

    dwError = LwAllocateMemory(sizeof(*OtherAccounts.ppwszNames) * dwNumNames,
                               OUT_PPVOID(&OtherAccounts.ppwszNames));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*OtherAccounts.pdwIndices) * dwNumNames,
                               OUT_PPVOID(&OtherAccounts.pdwIndices));
    BAIL_ON_LSA_ERROR(dwError);

    memset(OtherAccounts.pdwIndices, -1, sizeof(DWORD) * dwNumNames);


    for (i = 0; i < dwNumNames; i++)
    {
        UNICODE_STRING *pName = &(pNames[i]);

        dwError = LwAllocateWc16StringFromUnicodeString(
                                          &pwszName,
                                          pName);
        BAIL_ON_NTSTATUS_ERROR(dwError);

        ntStatus = LsaSrvParseAccountName(pwszName,
                                          &pwszDomainName,
                                          &pwszAcctName);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        if (pwszDomainName &&
            !wc16scasecmp(pwszDomainName, pPolCtx->pwszLocalDomainName))
        {
            /*
             * Local (MACHINE) domain accounts
             */
            ntStatus = LwAllocateWc16String(
                             &(LocalAccounts.ppwszNames[dwLocalNamesNum]),
                             pwszAcctName);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            LocalAccounts.pdwIndices[dwLocalNamesNum++] = i;
            LocalAccounts.dwCount = dwLocalNamesNum;
        }
        else if (pwszDomainName &&
                 !wc16scasecmp(pwszDomainName, wszBuiltinDomainName))
        {
            /*
             * BUILTIN domain accounts
             */
            ntStatus = LwAllocateWc16String(
                             &(BuiltinAccounts.ppwszNames[dwBuiltinNamesNum]),
                             pwszAcctName);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            BuiltinAccounts.pdwIndices[dwBuiltinNamesNum++] = i;
            BuiltinAccounts.dwCount = dwBuiltinNamesNum;
        }
        else if (pwszDomainName &&
                 pPolCtx->pwszDomainName &&
                 !wc16scasecmp(pwszDomainName, pPolCtx->pwszDomainName))
        {
            /*
             * Domain accounts (only if LSASS is a domain member)
             */
            ntStatus = LwAllocateWc16String(
                             &(DomainAccounts.ppwszNames[dwDomainNamesNum]),
                             pwszName);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            DomainAccounts.pdwIndices[dwDomainNamesNum++] = i;
            DomainAccounts.dwCount = dwDomainNamesNum;
        }
        else if (pwszDomainName &&
                 pPolCtx->pwszDomainName)
        {
            /*
             * Foreign (or any other we going to ask other DCs about)
             * domain accounts. Only considered if LSASS is a domain member.
             */
            ntStatus = LwAllocateWc16String(
                           &(ForDomainAccounts.ppwszNames[dwForDomainNamesNum]),
                           pwszName);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            ForDomainAccounts.pdwIndices[dwForDomainNamesNum++] = i;
            ForDomainAccounts.dwCount = dwForDomainNamesNum;
        }
        else
        {
            /*
             * If the account name isn't prepended with a domain name we're going
             * to give the local and bulitin domain a try and then decide.
             */
            ntStatus = LwAllocateWc16String(
                         &(OtherAccounts.ppwszNames[dwOtherNamesNum]),
                         pwszName);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            OtherAccounts.pdwIndices[dwOtherNamesNum++] = i;
            OtherAccounts.dwCount = dwOtherNamesNum;
        }

        LW_SAFE_FREE_MEMORY(pwszName);
        LW_SAFE_FREE_MEMORY(pwszDomainName);
        LW_SAFE_FREE_MEMORY(pwszAcctName);

        pwszName       = NULL;
        pwszDomainName = NULL;
        pwszAcctName   = NULL;
    }

    pAccountNames[LSA_DOMAIN_ACCOUNTS]         = DomainAccounts;
    pAccountNames[LSA_FOREIGN_DOMAIN_ACCOUNTS] = ForDomainAccounts;
    pAccountNames[LSA_LOCAL_DOMAIN_ACCOUNTS]   = LocalAccounts;
    pAccountNames[LSA_BUILTIN_DOMAIN_ACCOUNTS] = BuiltinAccounts;
    pAccountNames[LSA_OTHER_ACCOUNTS]          = OtherAccounts;

    *ppAccountNames = pAccountNames;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(ntStatus);
    }

    return ntStatus;

error:
    LsaSrvFreeAccountNames(pAccountNames);

    goto cleanup;
}


VOID
LsaSrvFreeAccountNames(
    PACCOUNT_NAMES pAccounts
    )
{
    DWORD iType = 0;
    DWORD iTypesNum = LSA_ACCOUNT_TYPE_SENTINEL;
    DWORD iName = 0;

    for (iType = 0; iType < iTypesNum; iType++)
    {
        for (iName = 0;
             iName < pAccounts[iType].dwCount;
             iName++)
       {
            PWSTR pwszName = pAccounts[iType].ppwszNames[iName];
            LW_SAFE_FREE_MEMORY(pwszName);
        }

        LW_SAFE_FREE_MEMORY(pAccounts[iType].ppwszNames);
        LW_SAFE_FREE_MEMORY(pAccounts[iType].pdwIndices);
    }

    LW_SAFE_FREE_MEMORY(pAccounts);
}


NTSTATUS
LsaSrvSelectAccountsByDomainSid(
    PPOLICY_CONTEXT   pPolCtx,
    SID_ARRAY        *pSids,
    DWORD             dwNumSids,
    PACCOUNT_SIDS    *ppAccountSids
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD i = 0;
    PSID pSid = NULL;
    ACCOUNT_SIDS DomainAccounts = {0};
    ACCOUNT_SIDS ForDomainAccounts = {0};
    ACCOUNT_SIDS LocalAccounts = {0};
    ACCOUNT_SIDS BuiltinAccounts = {0};
    DWORD dwDomainSidsNum = 0;
    DWORD dwForDomainSidsNum = 0;
    DWORD dwLocalSidsNum = 0;
    DWORD dwBuiltinSidsNum = 0;
    PACCOUNT_SIDS pAccountSids = NULL;
    DWORD dwAccountTypesNum = LSA_ACCOUNT_TYPE_SENTINEL;
    PSID pBuiltinDomainSid = NULL;

    dwError = LwAllocateMemory(sizeof(*pAccountSids) * dwAccountTypesNum,
                               OUT_PPVOID(&pAccountSids));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(DomainAccounts.ppSids[0]) * dwNumSids,
                               OUT_PPVOID(&DomainAccounts.ppSids));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*DomainAccounts.pdwIndices) * dwNumSids,
                               OUT_PPVOID(&DomainAccounts.pdwIndices));
    BAIL_ON_LSA_ERROR(dwError);

    memset(DomainAccounts.pdwIndices, -1, sizeof(DWORD) * dwNumSids);

    dwError = LwAllocateMemory(sizeof(ForDomainAccounts.ppSids[0]) * dwNumSids,
                               OUT_PPVOID(&ForDomainAccounts.ppSids));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*ForDomainAccounts.pdwIndices) * dwNumSids,
                               OUT_PPVOID(&ForDomainAccounts.pdwIndices));
    BAIL_ON_LSA_ERROR(dwError);

    memset(ForDomainAccounts.pdwIndices, -1, sizeof(DWORD) * dwNumSids);

    dwError = LwAllocateMemory(sizeof(LocalAccounts.ppSids[0]) * dwNumSids,
                               OUT_PPVOID(&LocalAccounts.ppSids));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*LocalAccounts.pdwIndices) * dwNumSids,
                               OUT_PPVOID(&LocalAccounts.pdwIndices));
    BAIL_ON_LSA_ERROR(dwError);

    memset(LocalAccounts.pdwIndices, -1, sizeof(DWORD) * dwNumSids);

    dwError = LwAllocateMemory(sizeof(BuiltinAccounts.ppSids[0]) * dwNumSids,
                               OUT_PPVOID(&BuiltinAccounts.ppSids));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*BuiltinAccounts.pdwIndices) * dwNumSids,
                               OUT_PPVOID(&BuiltinAccounts.pdwIndices));
    BAIL_ON_LSA_ERROR(dwError);

    memset(BuiltinAccounts.pdwIndices, -1, sizeof(DWORD) * dwNumSids);

    dwError = LwAllocateWellKnownSid(WinBuiltinDomainSid,
                                     NULL,
                                     &pBuiltinDomainSid,
                                     NULL);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwNumSids; i++)
    {
        pSid = pSids->pSids[i].pSid;

        if (pPolCtx->pDomainSid &&
            RtlIsPrefixSid(pPolCtx->pDomainSid, pSid))
        {
            /*
             * Domain accounts (only if LSASS is a domain member)
             */
            ntStatus = RtlDuplicateSid(
                                &DomainAccounts.ppSids[dwDomainSidsNum],
                                pSid);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            DomainAccounts.pdwIndices[dwDomainSidsNum++] = i;
            DomainAccounts.dwCount = dwDomainSidsNum;
        }
        else if (RtlIsPrefixSid(pPolCtx->pLocalDomainSid,
                                pSid))
        {
            /*
             * Local (MACHINE) domain accounts
             */
            ntStatus = RtlDuplicateSid(
                                &LocalAccounts.ppSids[dwLocalSidsNum],
                                pSid);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            LocalAccounts.pdwIndices[dwLocalSidsNum++] = i;
            LocalAccounts.dwCount = dwLocalSidsNum;
        }
        else if (RtlIsPrefixSid(pBuiltinDomainSid,
                                pSid))
        {
            /*
             * BUILTIN domain accounts
             */
            ntStatus = RtlDuplicateSid(
                                &BuiltinAccounts.ppSids[dwBuiltinSidsNum],
                                pSid);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            BuiltinAccounts.pdwIndices[dwBuiltinSidsNum++] = i;
            BuiltinAccounts.dwCount = dwBuiltinSidsNum;
        }
        else if (pPolCtx->pDomainSid)
        {
            /*
             * Foreign (or any other we going to ask other DCs about)
             * domain accounts. Only considered if LSASS is a domain member.
             */
            ntStatus = RtlDuplicateSid(
                                &ForDomainAccounts.ppSids[dwForDomainSidsNum],
                                pSid);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            ForDomainAccounts.pdwIndices[dwForDomainSidsNum++] = i;
            ForDomainAccounts.dwCount = dwForDomainSidsNum;
        }
    }

    pAccountSids[LSA_DOMAIN_ACCOUNTS]         = DomainAccounts;
    pAccountSids[LSA_FOREIGN_DOMAIN_ACCOUNTS] = ForDomainAccounts;
    pAccountSids[LSA_LOCAL_DOMAIN_ACCOUNTS]   = LocalAccounts;
    pAccountSids[LSA_BUILTIN_DOMAIN_ACCOUNTS] = BuiltinAccounts;

    *ppAccountSids = pAccountSids;

cleanup:
    LW_SAFE_FREE_MEMORY(pBuiltinDomainSid);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(ntStatus);
    }

    return ntStatus;

error:
    LsaSrvFreeAccountSids(pAccountSids);

    goto cleanup;
}


VOID
LsaSrvFreeAccountSids(
    PACCOUNT_SIDS pAccounts
    )
{
    DWORD iType = 0;
    DWORD iTypesNum = LSA_ACCOUNT_TYPE_SENTINEL;
    DWORD iSid = 0;

    for (iType = 0; iType < iTypesNum; iType++)
    {
        for (iSid = 0;
             iSid < pAccounts[iType].dwCount;
             iSid++)
        {
            PSID pSid = pAccounts[iType].ppSids[iSid];
            RTL_FREE(&pSid);
        }

        LW_SAFE_FREE_MEMORY(pAccounts[iType].ppSids);
        LW_SAFE_FREE_MEMORY(pAccounts[iType].pdwIndices);
    }

    LW_SAFE_FREE_MEMORY(pAccounts);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
