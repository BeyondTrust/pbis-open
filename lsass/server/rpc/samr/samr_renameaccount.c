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
 *        samr_renameaccount.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrSrvRenameAccount function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvRenameAccount(
    IN  ACCOUNT_HANDLE   hAccount,
    IN  UNICODE_STRING  *pAccountName
    )
{
    const wchar_t wszAccountDnFmt[] = L"CN=%ws,%ws";

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    HANDLE hDirectory = NULL;
    PWSTR pwszOldDn = NULL;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrCommonName[] = DS_ATTR_COMMON_NAME;
    size_t sParentDnLen = 0;
    DWORD dwNewDnLen = 0;
    PWSTR pwszNewDn = NULL;
    size_t sSamAccountNameLen = 0;
    PWSTR pwszSamAccountName = NULL;
    PWSTR pwszCommonName = NULL;
    DWORD i = 0;

    enum AttrValueIndex {
        ATTR_VAL_IDX_DN = 0,
        ATTR_VAL_IDX_SAM_ACCOUNT_NAME,
        ATTR_VAL_IDX_COMMON_NAME,
        ATTR_VAL_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {   /* ATTR_VAL_IDX_DN */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_SAM_ACCOUNT_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_COMMON_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        }
    };

    DIRECTORY_MOD ModDn = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrDn,
        1,
        &AttrValues[ATTR_VAL_IDX_DN]
    };

    DIRECTORY_MOD ModSamAccountName = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrSamAccountName,
        1,
        &AttrValues[ATTR_VAL_IDX_SAM_ACCOUNT_NAME]
    };

    DIRECTORY_MOD ModCommonName = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrCommonName,
        1,
        &AttrValues[ATTR_VAL_IDX_COMMON_NAME]
    };

    DIRECTORY_MOD Mods[ATTR_VAL_IDX_SENTINEL + 1];
    memset(&Mods, 0, sizeof(Mods));

    pAcctCtx = (PACCOUNT_CONTEXT)hAccount;
    pDomCtx  = pAcctCtx->pDomCtx;
    pConnCtx = pDomCtx->pConnCtx;

    if (pAcctCtx == NULL ||
        pAcctCtx->Type != SamrContextAccount)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    hDirectory = pConnCtx->hDirectory;
    pwszOldDn  = pAcctCtx->pwszDn;

    dwError = LwAllocateWc16StringFromUnicodeString(
                               &pwszSamAccountName,
                               pAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16StringFromUnicodeString(
                               &pwszCommonName,
                               pAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszSamAccountName, &sSamAccountNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pDomCtx->pwszDn, &sParentDnLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwNewDnLen = sSamAccountNameLen +
                 sParentDnLen +
                 (sizeof(wszAccountDnFmt)/sizeof(wszAccountDnFmt[0]));

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwNewDnLen,
                               OUT_PPVOID(&pwszNewDn));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszNewDn, dwNewDnLen, wszAccountDnFmt,
                    pwszSamAccountName,
                    pDomCtx->pwszDn) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    AttrValues[ATTR_VAL_IDX_DN].data.pwszStringValue
        = pwszNewDn;
    AttrValues[ATTR_VAL_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue
        = pwszSamAccountName;
    AttrValues[ATTR_VAL_IDX_COMMON_NAME].data.pwszStringValue
        = pwszCommonName;

    Mods[i++] = ModDn;
    Mods[i++] = ModSamAccountName;
    Mods[i++] = ModCommonName;

    dwError = DirectoryModifyObject(hDirectory,
                                    pwszOldDn,
                                    Mods);
    BAIL_ON_LSA_ERROR(dwError);

    LW_SAFE_FREE_MEMORY(pAcctCtx->pwszDn);
    LW_SAFE_FREE_MEMORY(pAcctCtx->pwszName);
    pwszOldDn          = NULL;
    pAcctCtx->pwszDn   = pwszNewDn;
    pAcctCtx->pwszName = pwszSamAccountName;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszCommonName);

    /*
     * Free pwszNewDn and pwszSamAccountName only if the rename
     * failed and therefore pAcctCtx fields haven't been set
     */
    if (pwszNewDn != pAcctCtx->pwszDn)
    {
        LW_SAFE_FREE_MEMORY(pwszNewDn);
    }

    if (pwszSamAccountName != pAcctCtx->pwszName)
    {
        LW_SAFE_FREE_MEMORY(pwszSamAccountName);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwNtStatusToWin32Error(dwError);
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
