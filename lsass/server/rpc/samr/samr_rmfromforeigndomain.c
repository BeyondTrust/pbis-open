/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        samr_rmfromforeigndomain.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrRemoveFromForeignDomain function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvRemoveMemberFromForeignDomain(
    /* [in] */ handle_t hBinding,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ PSID pSid
    )
{
    const DWORD dwAccessMask = ALIAS_ACCESS_REMOVE_MEMBER;
    const wchar_t wszFilterFmt[] = L"%ws=%d AND %ws='%ws'";

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    HANDLE hDirectory = NULL;
    DWORD i = 0;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrDomainName[] = DS_ATTR_DOMAIN;
    PWSTR pwszDomainName = NULL;
    DWORD dwFilterLen = 0;
    PWSTR pwszFilter = NULL;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    DWORD dwObjectClass = DS_OBJECT_CLASS_LOCAL_GROUP;
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwEntriesNum = 0;
    PWSTR pwszAliasSid = NULL;
    PSID pAliasSid = NULL;
    DWORD dwRid = 0;
    ACCOUNT_HANDLE hAlias = NULL;
    PACCOUNT_CONTEXT pAcctCtx = NULL;

    PWSTR wszAttributes[] = {
        wszAttrObjectSid,
        NULL
    };

    pDomCtx = (PDOMAIN_CONTEXT)hDomain;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pConnCtx       = pDomCtx->pConnCtx;
    pwszDomainName = pDomCtx->pwszDomainName;
    hDirectory     = pConnCtx->hDirectory;

    dwFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrDomainName)/sizeof(WCHAR)) - 1) +
                  (wc16slen(pwszDomainName) + 1) +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    dwError = LwAllocateMemory(dwFilterLen * sizeof(*pwszFilter),
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                    wszAttrObjectClass,
                    dwObjectClass,
                    wszAttrDomainName,
                    pwszDomainName) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = DirectorySearch(hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntries,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Attempt to remove the SID from any alias it might be a member of
     */
    for (i = 0; i < dwEntriesNum; i++)
    {
        PDIRECTORY_ENTRY pEntry = &(pEntries[i]);

        dwError = DirectoryGetEntryAttrValueByName(
                                     pEntry,
                                     wszAttrObjectSid,
                                     DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                     &pwszAliasSid);
        BAIL_ON_LSA_ERROR(dwError);

        ntStatus = RtlAllocateSidFromWC16String(&pAliasSid,
                                                pwszAliasSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwRid = pAliasSid->SubAuthority[pAliasSid->SubAuthorityCount - 1];

        ntStatus = SamrSrvOpenAccount(hBinding,
                                      hDomain,
                                      dwAccessMask,
                                      dwRid,
                                      DS_OBJECT_CLASS_LOCAL_GROUP,
                                      &hAlias);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = SamrSrvDeleteAliasMember(hBinding,
                                            hAlias,
                                            pSid);
        if (ntStatus == STATUS_NO_SUCH_MEMBER ||
            ntStatus == STATUS_MEMBER_NOT_IN_ALIAS)
        {
            /* Member is not in this alias so juts ignore it
               and try another one */
            ntStatus = STATUS_SUCCESS;
        }
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = SamrSrvClose(hBinding, &hAlias);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        /*
         * Release the account handle. Normally DCE/RPC runtime
         * would take care of that but this time the handle wasn't
         * returned as [context_handle] argument
         */
        pAcctCtx = (PACCOUNT_CONTEXT)hAlias;
        hAlias   = NULL;

        ACCOUNT_HANDLE_rundown(pAcctCtx);
        RTL_FREE(&pAcctCtx);

        RTL_FREE(&pAliasSid);
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwEntriesNum);
    }

    if (hAlias)
    {
        pAcctCtx = (PACCOUNT_CONTEXT)hAlias;
        hAlias   = NULL;

        ACCOUNT_HANDLE_rundown(pAcctCtx);
        RTL_FREE(&pAcctCtx);
    }

    RTL_FREE(&pAliasSid);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
