/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2010
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
    const wchar_t wszFilterFmt[] = L"%ws='%ws'";

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSAMR_GENERIC_CONTEXT pCtx = (PSAMR_GENERIC_CONTEXT)hObject;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    PWSTR pwszDn = NULL;
    HANDLE hDirectory = NULL;
    PWSTR pwszBaseDn = NULL;
    DWORD dwScope = 0;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrSecDesc[] = DS_ATTR_SECURITY_DESCRIPTOR;
    size_t sDnLen = 0;
    DWORD dwFilterLen = 0;
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

    dwError = LwWc16sLen(pwszDn, &sDnLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwFilterLen = ((sizeof(wszAttrDn)/sizeof(wszAttrDn[0])) - 1) +
                  sDnLen +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwFilterLen,
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                    wszAttrDn,
                    pwszDn) < 0)
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
