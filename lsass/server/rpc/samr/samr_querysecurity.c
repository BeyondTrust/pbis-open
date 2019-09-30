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
 *        samr_querysecurity.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrQuerySecurity function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvQuerySecurity(
    IN  handle_t                          hBinding,
    IN  void                             *hObject,
    IN  DWORD                             dwSecurityInfo,
    OUT PSAMR_SECURITY_DESCRIPTOR_BUFFER *ppSecDescBuf
    )
{
    PCSTR filterFormat = "%s=%Q";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSAMR_GENERIC_CONTEXT pCtx = (PSAMR_GENERIC_CONTEXT)hObject;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    PWSTR pwszDn = NULL;
    HANDLE hDirectory = NULL;
    PWSTR pwszBaseDn = NULL;
    DWORD dwScope = 0;
    WCHAR wszAttrSecDesc[] = DS_ATTR_SECURITY_DESCRIPTOR;
    CHAR szAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    PSTR pszDn = NULL;
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwNumEntries = 0;
    PDIRECTORY_ENTRY pObjectEntry = NULL;
    POCTET_STRING pSecDescBlob = NULL;
    PSAMR_SECURITY_DESCRIPTOR_BUFFER pSecDescBuf = NULL;

    PWSTR wszAttributes[] = {
        wszAttrSecDesc,
        NULL
    };

    /*
     * Only querying security descriptor on an account is allowed
     */
    if (pCtx->Type == SamrContextAccount)
    {
        pAcctCtx    = (PACCOUNT_CONTEXT)hObject;
        pDomCtx     = pAcctCtx->pDomCtx;
        pwszDn      = pAcctCtx->pwszDn;
        hDirectory  = pDomCtx->pConnCtx->hDirectory;
    }
    else if (pCtx->Type == SamrContextConnect ||
             pCtx->Type == SamrContextDomain)
    {
        ntStatus = STATUS_ACCESS_DENIED;
    }
    else
    {
        ntStatus = STATUS_INVALID_HANDLE;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * 1) READ_CONTROL right is required to read a security descriptor
     * 2) Querying SACL part of the SD is not permitted over rpc
     */
    if (!(pAcctCtx->dwAccessGranted & READ_CONTROL) ||
        dwSecurityInfo & SACL_SECURITY_INFORMATION)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = LwWc16sToMbs(pwszDn, &pszDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryAllocateWC16StringFilterPrintf(
                              &pwszFilter,
                              filterFormat,
                              szAttrDn, pszDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(hDirectory,
                              pwszBaseDn,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntries,
                              &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        ntStatus = STATUS_INVALID_HANDLE;
    }
    else if (dwNumEntries > 1)
    {
        ntStatus = STATUS_INTERNAL_ERROR;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pObjectEntry = &(pEntries[0]);

    dwError = DirectoryGetEntryAttrValueByName(
                              pObjectEntry,
                              wszAttrSecDesc,
                              DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR,
                              &pSecDescBlob);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrSrvAllocateSecDescBuffer(
                              &pSecDescBuf,
                              (SECURITY_INFORMATION)dwSecurityInfo,
                              pSecDescBlob);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppSecDescBuf = pSecDescBuf;

cleanup:
    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    LW_SAFE_FREE_MEMORY(pszDn);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pSecDescBuf)
    {
        if (pSecDescBuf->pBuffer)
        {
            SamrSrvFreeMemory(pSecDescBuf->pBuffer);
        }

        SamrSrvFreeMemory(pSecDescBuf);
    }

    *ppSecDescBuf = NULL;

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
