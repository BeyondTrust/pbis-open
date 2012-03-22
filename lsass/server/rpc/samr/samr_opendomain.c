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
 *        samr_opendomain.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrOpenDomain function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvOpenDomain(
    IN  handle_t        hBinding,
    IN  CONNECT_HANDLE  hConn,
    IN  DWORD           dwAccessMask,
    IN  PSID            pSid,
    OUT DOMAIN_HANDLE  *phDomain
    )
{
    wchar_t wszFilter[] = L"(%ws=%u OR %ws=%u) AND %ws='%ws'";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PWSTR pwszBase = NULL;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrCommonName[] = DS_ATTR_COMMON_NAME;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrMinPwdAge[] = DS_ATTR_MIN_PWD_AGE;
    WCHAR wszAttrMaxPwdAge[] = DS_ATTR_MAX_PWD_AGE;
    WCHAR wszAttrMinPwdLength[] = DS_ATTR_MIN_PWD_LENGTH;
    WCHAR wszAttrPwdPromptTime[] = DS_ATTR_PWD_PROMPT_TIME;
    WCHAR wszAttrPwdProperties[] = DS_ATTR_PWD_PROPERTIES;
    WCHAR wszAttrSequenceNumber[] = DS_ATTR_SEQUENCE_NUMBER;
    WCHAR wszAttrSecurityDescriptor[] = DS_ATTR_SECURITY_DESCRIPTOR;
    DWORD dwObjectClassDomain = DS_OBJECT_CLASS_DOMAIN;
    DWORD dwObjectClassBuiltin = DS_OBJECT_CLASS_BUILTIN_DOMAIN;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PWSTR pwszDomainSid = NULL;
    size_t sDomainSidStrLen = 0;
    DWORD dwNumEntries = 0;
    PDIRECTORY_ENTRY pEntry = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    GENERIC_MAPPING GenericMapping = {0};
    DWORD dwAccessGranted = 0;
    PWSTR pwszSid = NULL;
    PSID pDomainSid = NULL;
    PWSTR pwszName = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszDn = NULL;
    PWSTR pwszDomainDn = NULL;
    LONG64 llMinPasswordAge = 0;
    LONG64 llMaxPasswordAge = 0;
    DWORD dwMinPwdLen = 0;
    LONG64 llPasswordPromptTime = 0;
    DWORD dwPwdProperties = 0;

    PWSTR wszAttributes[] = {
        wszAttrCommonName,
        wszAttrObjectSid,
        wszAttrDn,
        wszAttrMinPwdAge,
        wszAttrMaxPwdAge,
        wszAttrMinPwdLength,
        wszAttrPwdPromptTime,
        wszAttrPwdProperties,
        wszAttrSequenceNumber,
        wszAttrSecurityDescriptor,
        NULL
    };

    BAIL_ON_INVALID_PTR(hBinding);
    BAIL_ON_INVALID_PTR(hConn);
    BAIL_ON_INVALID_PTR(pSid);
    BAIL_ON_INVALID_PTR(phDomain);

    pConnCtx = (PCONNECT_CONTEXT)hConn;

    if (pConnCtx == NULL || pConnCtx->Type != SamrContextConnect)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /* Check access rights required */
    if (!(pConnCtx->dwAccessGranted & SAMR_ACCESS_OPEN_DOMAIN))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    ntStatus = RtlAllocateWC16StringFromSid(&pwszDomainSid, pSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwWc16sLen(pwszDomainSid, &sDomainSidStrLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwFilterLen = ((sizeof(wszAttrObjectClass) / sizeof(WCHAR)) - 1) +
                  10 +
                  (sizeof(wszFilter) / sizeof(wszFilter[0])) +
                  ((sizeof(wszAttrObjectClass) / sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrObjectSid) / sizeof(WCHAR)) - 1) +
                  sDomainSidStrLen;

    dwError = LwAllocateMemory(dwFilterLen * sizeof(pwszFilter[0]),
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilter,
                    &wszAttrObjectClass[0],
                    dwObjectClassDomain,
                    &wszAttrObjectClass[0],
                    dwObjectClassBuiltin,
                    &wszAttrObjectSid[0],
                    pwszDomainSid) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntry,
                              &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pDomCtx),
                               OUT_PPVOID(&pDomCtx));
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        ntStatus = STATUS_NO_SUCH_DOMAIN;
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
                        pDomCtx->dwAccessGranted,
                        &GenericMapping,
                        &dwAccessGranted,
                        &ntStatus))
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Get domain attributes:
     */

    /* SID */
    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrObjectSid,
                              DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                              &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlAllocateSidFromWC16String(
                              &pDomainSid,
                              pwszSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* Domain name */
    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrCommonName,
                              DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                              &pwszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszDomainName,
                                   pwszName);
    BAIL_ON_LSA_ERROR(dwError);

    /* Domain object DN */
    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrDn,
                              DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                              &pwszDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszDomainDn,
                                   pwszDn);
    BAIL_ON_LSA_ERROR(dwError);

    /* Min password age */
    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrMinPwdAge,
                              DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                              &llMinPasswordAge);
    BAIL_ON_LSA_ERROR(dwError);

    /* Max password age */
    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrMaxPwdAge,
                              DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                              &llMaxPasswordAge);
    BAIL_ON_LSA_ERROR(dwError);

    /* Min password length */
    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrMinPwdLength,
                              DIRECTORY_ATTR_TYPE_INTEGER,
                              &dwMinPwdLen);
    BAIL_ON_LSA_ERROR(dwError);

    /* Password prompt time */
    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrPwdPromptTime,
                              DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                              &llPasswordPromptTime);
    BAIL_ON_LSA_ERROR(dwError);

    /* Password properties */
    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrPwdProperties,
                              DIRECTORY_ATTR_TYPE_INTEGER,
                              &dwPwdProperties);
    BAIL_ON_LSA_ERROR(dwError);

    pDomCtx->Type                 = SamrContextDomain;
    pDomCtx->refcount             = 1;
    pDomCtx->dwAccessGranted      = dwAccessGranted;
    pDomCtx->pDomainSid           = pDomainSid;
    pDomCtx->pwszDomainName       = pwszDomainName;
    pDomCtx->pwszDn               = pwszDomainDn;
    pDomCtx->ntMinPasswordAge     = llMinPasswordAge;
    pDomCtx->ntMaxPasswordAge     = llMaxPasswordAge;
    pDomCtx->dwMinPasswordLen     = dwMinPwdLen;
    pDomCtx->ntPasswordPromptTime = llPasswordPromptTime;
    pDomCtx->dwPasswordProperties = dwPwdProperties;

    pDomCtx->pConnCtx             = pConnCtx;
    InterlockedIncrement(&pConnCtx->refcount);

    *phDomain = (DOMAIN_HANDLE)pDomCtx;

cleanup:
    if (pwszDomainSid)
    {
        RTL_FREE(&pwszDomainSid);
    }

    LW_SAFE_FREE_MEMORY(pwszFilter);

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
    if (pDomCtx)
    {
        SamrSrvDomainContextFree(pDomCtx);
    }

    *phDomain = NULL;
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
