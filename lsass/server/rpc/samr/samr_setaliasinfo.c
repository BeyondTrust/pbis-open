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
 *        samr_setaliasinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrSrvSetAliasInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


#define SET_UNICODE_STRING_VALUE(var, idx, mod)                \
    do {                                                       \
        PWSTR pwszValue = NULL;                                \
                                                               \
        ntStatus = SamrSrvGetFromUnicodeString(&pwszValue,     \
                                               &(var));        \
        BAIL_ON_NTSTATUS_ERROR(ntStatus);                      \
                                                               \
        AttrValues[(idx)].data.pwszStringValue = pwszValue;    \
                                                               \
        Mods[i++] = mod;                                       \
    } while (0);


NTSTATUS
SamrSrvSetAliasInfo(
    /* [in] */ handle_t hBinding,
    /* [in] */ ACCOUNT_HANDLE hAlias,
    /* [in] */ UINT16 level,
    /* [in] */ AliasInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    HANDLE hDirectory = NULL;
    PWSTR pwszAccountDn = NULL;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    DWORD i = 0;

    enum AttrValueIndex {
        ATTR_VAL_IDX_DESCRIPTION = 0,
        ATTR_VAL_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        }
    };

    DIRECTORY_MOD ModDescription = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrDescription,
        1,
        &AttrValues[ATTR_VAL_IDX_DESCRIPTION]
    };

    DIRECTORY_MOD Mods[ATTR_VAL_IDX_SENTINEL + 1];
    memset(&Mods, 0, sizeof(Mods));

    pAcctCtx = (PACCOUNT_CONTEXT)hAlias;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!(pAcctCtx->dwAccessGranted & ALIAS_ACCESS_SET_INFO))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pDomCtx       = pAcctCtx->pDomCtx;
    pConnCtx      = pDomCtx->pConnCtx;
    hDirectory    = pConnCtx->hDirectory;
    pwszAccountDn = pAcctCtx->pwszDn;

    switch (level)
    {
    case ALIAS_INFO_ALL:
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
        break;

    case ALIAS_INFO_NAME:
        ntStatus = SamrSrvRenameAccount(hAlias,
                                        &pInfo->name);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        goto cleanup;

    case ALIAS_INFO_DESCRIPTION:
        SET_UNICODE_STRING_VALUE(pInfo->description,
                                 ATTR_VAL_IDX_DESCRIPTION,
                                 ModDescription);
        break;
    }

    dwError = DirectoryModifyObject(hDirectory,
                                    pwszAccountDn,
                                    Mods);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
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
