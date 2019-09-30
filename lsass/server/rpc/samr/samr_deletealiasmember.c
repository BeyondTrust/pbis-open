/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        samr_deletealiasmember.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrDeleteAliasMember function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvDeleteAliasMember(
    handle_t IDL_handle,
    ACCOUNT_HANDLE hAlias,
    PSID pSid
    )
{
    const wchar_t wszFilterFmt[] = L"%ws='%ws'";

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszGroupDn = NULL;
    PWSTR pwszSid = NULL;
    size_t sSidStrLen = 0;
    HANDLE hDirectory = NULL;
    PWSTR pwszBaseDn = NULL;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    DWORD dwScope = 0;
    DWORD dwFilterLen = 0;
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;

    PWSTR wszAttributes[] = {
        wszAttrDn,
        wszAttrObjectClass,
        NULL
    };

    pAcctCtx = (PACCOUNT_CONTEXT)hAlias;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!(pAcctCtx->dwAccessGranted & ALIAS_ACCESS_REMOVE_MEMBER))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pDomCtx     = pAcctCtx->pDomCtx;
    pConnCtx    = pDomCtx->pConnCtx;
    hDirectory  = pConnCtx->hDirectory;
    pwszGroupDn = pAcctCtx->pwszDn;

    ntStatus = RtlAllocateWC16StringFromSid(&pwszSid,
                                            pSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwWc16sLen(pwszSid, &sSidStrLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1)) +
                  sSidStrLen +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwFilterLen,
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                    wszAttrObjectSid, pwszSid) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = DirectorySearch(hDirectory,
                              pwszBaseDn,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntry,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwEntriesNum > 1)
    {
        ntStatus = STATUS_INTERNAL_ERROR;
    }
    else if (dwEntriesNum == 0)
    {
        ntStatus = STATUS_NO_SUCH_MEMBER;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = DirectoryRemoveFromGroup(hDirectory,
                                       pwszGroupDn,
                                       pEntry);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pEntry)
    {
        DirectoryFreeEntries(pEntry, dwEntriesNum);
    }

    LW_SAFE_FREE_MEMORY(pwszFilter);
    RTL_FREE(&pwszSid);

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
