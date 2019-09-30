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
 *        samr_queryaliasinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrQueryAliasInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
SamrFillAliasInfo1(
    PDIRECTORY_ENTRY pEntry,
    DWORD dwNumMembers,
    AliasInfo *pInfo
    );


static
NTSTATUS
SamrFillAliasInfo2(
    PDIRECTORY_ENTRY pEntry,
    AliasInfo *pInfo
    );


static
NTSTATUS
SamrFillAliasInfo3(
    PDIRECTORY_ENTRY pEntry,
    AliasInfo *pInfo
    );


NTSTATUS
SamrSrvQueryAliasInfo(
    /* [in] */ handle_t hBinding,
    /* [in] */ ACCOUNT_HANDLE hAlias,
    /* [in] */ UINT16 level,
    /* [out] */ AliasInfo **info
    )
{
    PCSTR filterFormat = "%s=%Q";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    CHAR szAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    DWORD dwScope = 0;
    PSTR pszDn = NULL;
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    PDIRECTORY_ENTRY pMemberEntry = NULL;
    DWORD dwNumMembers = 0;
    AliasInfo *pAliasInfo = NULL;

    PWSTR wszAttributesLevel1[] = {
        wszAttrSamAccountName,
        wszAttrDescription,
        NULL
    };

    PWSTR wszAttributesLevel2[] = {
        wszAttrSamAccountName,
        NULL
    };

    PWSTR wszAttributesLevel3[] = {
        wszAttrDescription,
        NULL
    };

    PWSTR *pwszAttributes[] = {
        wszAttributesLevel1,
        wszAttributesLevel2,
        wszAttributesLevel3
    };

    PWSTR wszMemberAttributes[] = {
        wszAttrDn,
        NULL
    };

    pAcctCtx = (PACCOUNT_CONTEXT)hAlias;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!(pAcctCtx->dwAccessGranted & ALIAS_ACCESS_LOOKUP_INFO))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pDomCtx  = pAcctCtx->pDomCtx;
    pConnCtx = pDomCtx->pConnCtx;

    pwszBase = pDomCtx->pwszDn;

    dwError = LwWc16sToMbs(pAcctCtx->pwszDn,
                           &pszDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryAllocateWC16StringFilterPrintf(
                            &pwszFilter,
                            filterFormat,
                            szAttrDn, pszDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              pwszAttributes[level - 1],
                              FALSE,
                              &pEntry,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwEntriesNum == 0)
    {
        ntStatus = STATUS_INVALID_HANDLE;

    }
    else if (dwEntriesNum > 1)
    {
        ntStatus = STATUS_INTERNAL_ERROR;
    }

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (level == ALIAS_INFO_ALL)
    {
        dwError = DirectoryGetGroupMembers(pConnCtx->hDirectory,
                                           pAcctCtx->pwszDn,
                                           wszMemberAttributes,
                                           &pMemberEntry,
                                           &dwNumMembers);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ntStatus = SamrSrvAllocateMemory((void**)&pAliasInfo,
                                     sizeof(*pAliasInfo));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    switch (level)
    {
    case ALIAS_INFO_ALL:
        ntStatus = SamrFillAliasInfo1(pEntry, dwNumMembers, pAliasInfo);
        break;

    case ALIAS_INFO_NAME:
        ntStatus = SamrFillAliasInfo2(pEntry, pAliasInfo);
        break;

    case ALIAS_INFO_DESCRIPTION:
        ntStatus = SamrFillAliasInfo3(pEntry, pAliasInfo);
        break;

    default:
        ntStatus = STATUS_INVALID_INFO_CLASS;
    }
    
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *info = pAliasInfo;
    
cleanup:
    LW_SAFE_FREE_MEMORY(pszDn);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntry)
    {
        DirectoryFreeEntries(pEntry, dwEntriesNum);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pAliasInfo)
    {
        SamrSrvFreeMemory(pAliasInfo);
    }

    *info = NULL;
    goto cleanup;
}


static
NTSTATUS
SamrFillAliasInfo1(
    PDIRECTORY_ENTRY pEntry,
    DWORD dwNumMembers,
    AliasInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    AliasInfoAll *pInfoAll = NULL;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    PWSTR pwszName = NULL;
    PWSTR pwszDescription = NULL;

    pInfoAll = &(pInfo->all);

    /* name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrSrvInitUnicodeString(&pInfoAll->name, pwszName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* num_members */
    pInfoAll->num_members = dwNumMembers;

    /* description */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDescription,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDescription);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrSrvInitUnicodeString(&pInfoAll->description, pwszDescription);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    memset(pInfo, 0, sizeof(*pInfo));
    goto cleanup;
}


static
NTSTATUS
SamrFillAliasInfo2(
    PDIRECTORY_ENTRY pEntry,
    AliasInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrSamrAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    PWSTR pwszName = NULL;

    /* name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamrAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrSrvInitUnicodeString(&pInfo->name, pwszName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    memset(pInfo, 0, sizeof(*pInfo));
    goto cleanup;
}


static
NTSTATUS
SamrFillAliasInfo3(
    PDIRECTORY_ENTRY pEntry,
    AliasInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    PWSTR pwszDescription = NULL;

    /* name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDescription,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDescription);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrSrvInitUnicodeString(&pInfo->description, pwszDescription);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    memset(pInfo, 0, sizeof(*pInfo));
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
