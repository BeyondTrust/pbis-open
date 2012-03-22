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
 *        samr_openaccount.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrOpenAccount function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvOpenAccount(
    IN  handle_t         hBinding,
    IN  DOMAIN_HANDLE    hDomain,
    IN  DWORD            dwAccessMask,
    IN  DWORD            dwRid,
    IN  DWORD            dwObjectClass,
    OUT ACCOUNT_HANDLE  *phAccount
    )
{
    const ULONG ulSubAuthCount = 5;
    const wchar_t wszFilterFmt[] = L"(%ws=%u AND %ws='%ws')";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    HANDLE hDirectory = NULL;
    PWSTR pwszBaseDn = NULL;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrSecurityDescriptor[] = DS_ATTR_SECURITY_DESCRIPTOR;
    DWORD dwScope = 0;
    ULONG ulSidLength = 0;
    PSID pSid = NULL;
    size_t sAccountSidStrLen = 0;
    PWSTR pwszAccountSid = NULL;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PSID pDomainSid = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    GENERIC_MAPPING GenericMapping = {0};
    DWORD dwAccessGranted = 0;
    PWSTR pwszName = NULL;
    PWSTR pwszAccountName = NULL;
    PWSTR pwszDn = NULL;
    PWSTR pwszAccountDn = NULL;
    PWSTR pwszSid = NULL;
    PSID pAccountSid = NULL;
    DWORD dwAccountRid = 0;
    
    PWSTR wszAttributes[] = {
        wszAttrDn,
        wszAttrSamAccountName,
        wszAttrObjectSid,
        wszAttrSecurityDescriptor,
        NULL
    };

    pDomCtx  = (PDOMAIN_CONTEXT)hDomain;
    pConnCtx = pDomCtx->pConnCtx;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!(pDomCtx->dwAccessGranted & DOMAIN_ACCESS_OPEN_ACCOUNT))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    hDirectory = pDomCtx->pConnCtx->hDirectory;
    pwszBaseDn = pDomCtx->pwszDn;
    pDomainSid = pDomCtx->pDomainSid;

    dwError = LwAllocateMemory(sizeof(*pAcctCtx),
                                OUT_PPVOID(&pAcctCtx));
    BAIL_ON_LSA_ERROR(dwError);

    ulSidLength = RtlLengthRequiredSid(ulSubAuthCount);
    dwError = LwAllocateMemory(ulSidLength,
                                OUT_PPVOID(&pSid));
    BAIL_ON_NTSTATUS_ERROR(dwError);

    ntStatus = RtlCopySid(ulSidLength, pSid, pDomainSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlAppendRidSid(ulSidLength, pSid, dwRid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlAllocateWC16StringFromSid(&pwszAccountSid,
                                            pSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwWc16sLen(pwszAccountSid, &sAccountSidStrLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwFilterLen = ((sizeof(wszAttrObjectClass) / sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrObjectSid) / sizeof(WCHAR)) - 1) +
                  sAccountSidStrLen +
                  (sizeof(wszFilterFmt) / sizeof(wszFilterFmt[0]));

    dwError = LwAllocateMemory(dwFilterLen * sizeof(WCHAR),
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                    wszAttrObjectClass, dwObjectClass,
                    wszAttrObjectSid, pwszAccountSid) < 0)
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
                              &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        ntStatus = STATUS_NO_SUCH_USER;

    }
    else if (dwNumEntries > 1)
    {
        ntStatus = STATUS_INTERNAL_ERROR;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Perform an access check using domain object security descriptor
     */
    dwError = DirectoryGetEntrySecurityDescriptor(
                              pEntry,
                              &pSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    if (!RtlAccessCheck(pSecDesc,
                        pConnCtx->pUserToken,
                        dwAccessMask,
                        pAcctCtx->dwAccessGranted,
                        &GenericMapping,
                        &dwAccessGranted,
                        &ntStatus))
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Get account name
     */
    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrSamAccountName,
                              DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                              &pwszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszAccountName,
                                   pwszName);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Get account SID
     */
    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrObjectSid,
                              DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                              &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlAllocateSidFromWC16String(
                              &pAccountSid,
                              pwszSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwAccountRid = pAccountSid->SubAuthority[pAccountSid->SubAuthorityCount - 1];

    /*
     * Get domain object DN
     */
    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrDn,
                              DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                              &pwszDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszAccountDn,
                                   pwszDn);
    BAIL_ON_LSA_ERROR(dwError);

    pAcctCtx->Type             = SamrContextAccount;
    pAcctCtx->refcount         = 1;
    pAcctCtx->dwAccessGranted  = dwAccessGranted;
    pAcctCtx->pwszDn           = pwszAccountDn;
    pAcctCtx->pwszName         = pwszAccountName;
    pAcctCtx->pSid             = pAccountSid;
    pAcctCtx->dwRid            = dwAccountRid;

    pAcctCtx->pDomCtx          = pDomCtx;
    InterlockedIncrement(&pDomCtx->refcount);

    *phAccount = (ACCOUNT_HANDLE)pAcctCtx;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszFilter);
    LW_SAFE_FREE_MEMORY(pSid);

    if (pwszAccountSid)
    {
        RTL_FREE(&pwszAccountSid);
    }

    if (pEntry)
    {
        DirectoryFreeEntries(pEntry, dwNumEntries);
    }

    DirectoryFreeEntrySecurityDescriptor(&pSecDesc);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pAcctCtx)
    {
        SamrSrvAccountContextFree(pAcctCtx);
    }

    *phAccount = NULL;
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
