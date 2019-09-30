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
 *        samr_setsecurity.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrSetSecurity function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvSetSecurity(
    IN  handle_t                          hBinding,
    IN  void                             *hObject,
    IN  DWORD                             dwSecurityInfo,
    IN  PSAMR_SECURITY_DESCRIPTOR_BUFFER  pSecDescBuf
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
    CHAR szAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrSecDesc[] = DS_ATTR_SECURITY_DESCRIPTOR;
    PSTR pszDn = NULL;
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwNumEntries = 0;
    PDIRECTORY_ENTRY pObjectEntry = NULL;
    POCTET_STRING pSecDescBlob = NULL;
    GENERIC_MAPPING GenericMapping = {0};
    PSECURITY_DESCRIPTOR_RELATIVE pCurrentSecDesc = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescChange = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pNewSecDesc = NULL;
    DWORD dwNewSecDescLength = 0;
    OCTET_STRING NewSecDescBlob = {0};
    DWORD iMod = 0;

    PWSTR wszAttributes[] = {
        wszAttrSecDesc,
        NULL
    };

    enum AttrValueIndex {
        ATTR_VAL_IDX_SEC_DESC = 0,
        ATTR_VAL_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {   /* ATTR_VAL_IDX_SEC_DESC */
            .Type = DIRECTORY_ATTR_TYPE_OCTET_STREAM,
            .data.pOctetString = NULL
        }
    };

    DIRECTORY_MOD ModSecDesc = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrSecDesc,
        1,
        &AttrValues[ATTR_VAL_IDX_SEC_DESC]
    };

    DIRECTORY_MOD Mods[ATTR_VAL_IDX_SENTINEL + 1];
    memset(&Mods, 0, sizeof(Mods));

    /*
     * Only setting security descriptor in an account is allowed
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
     * 3) WRITE_DAC right is required to change a DACL in security descriptor
     * 4) WRITE_OWNER right is require to change owner in security descriptor
     */
    if (!(pAcctCtx->dwAccessGranted & READ_CONTROL) ||
        dwSecurityInfo & SACL_SECURITY_INFORMATION ||
        ((dwSecurityInfo & DACL_SECURITY_INFORMATION) &&
         !(pAcctCtx->dwAccessGranted & WRITE_DAC)) ||
        ((dwSecurityInfo & OWNER_SECURITY_INFORMATION) &&
         !(pAcctCtx->dwAccessGranted & WRITE_OWNER)))
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

    pCurrentSecDesc  = (PSECURITY_DESCRIPTOR_RELATIVE)pSecDescBlob->pBytes;
    pSecDescChange   = (PSECURITY_DESCRIPTOR_RELATIVE)pSecDescBuf->pBuffer;

    ntStatus = RtlSetSecurityDescriptorInfo(
                              (SECURITY_INFORMATION)dwSecurityInfo,
                              pSecDescChange,
                              pCurrentSecDesc,
                              NULL,
                              &dwNewSecDescLength,
                              &GenericMapping);
    if (ntStatus == STATUS_BUFFER_TOO_SMALL)
    {
        dwError = LwAllocateMemory(dwNewSecDescLength,
                                   OUT_PPVOID(&pNewSecDesc));
        BAIL_ON_LSA_ERROR(dwError);

        ntStatus = RtlSetSecurityDescriptorInfo(
                                  (SECURITY_INFORMATION)dwSecurityInfo,
                                  pSecDescChange,
                                  pCurrentSecDesc,
                                  pNewSecDesc,
                                  &dwNewSecDescLength,
                                  &GenericMapping);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }
    else
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    NewSecDescBlob.pBytes     = (PBYTE)pNewSecDesc;
    NewSecDescBlob.ulNumBytes = dwNewSecDescLength;

    AttrValues[ATTR_VAL_IDX_SEC_DESC].data.pOctetString = &NewSecDescBlob;
    Mods[iMod++] = ModSecDesc;

    dwError = DirectoryModifyObject(hDirectory,
                                    pwszDn,
                                    Mods);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    LW_SAFE_FREE_MEMORY(pszDn);
    LW_SAFE_FREE_MEMORY(pwszFilter);
    LW_SAFE_FREE_MEMORY(pNewSecDesc);

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
