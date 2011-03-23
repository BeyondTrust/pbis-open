/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2008
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
 *        samr_createdomgroup.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrCreateDomGroup function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */
#include "includes.h"


NTSTATUS
SamrCreateDomGroup(
    IN  SAMR_BINDING    hBinding,
    IN  DOMAIN_HANDLE   hDomain,
    IN  PWSTR           pwszGroupName,
    IN  UINT32          AccessMask,
    OUT ACCOUNT_HANDLE *phGroup,
    OUT PUINT32         pRid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    UNICODE_STRING GroupName = {0};
    ACCOUNT_HANDLE hGroup = NULL;
    UINT32 Rid = 0;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pwszGroupName, ntStatus);
    BAIL_ON_INVALID_PTR(phGroup, ntStatus);
    BAIL_ON_INVALID_PTR(pRid, ntStatus);

    dwError = LwAllocateUnicodeStringFromWc16String(
                                    &GroupName,
                                    pwszGroupName);
    BAIL_ON_WIN_ERROR(dwError);

    DCERPC_CALL(ntStatus, cli_SamrCreateDomGroup((handle_t)hBinding,
                                                 hDomain,
                                                 &GroupName,
                                                 AccessMask,
                                                 &hGroup,
                                                 &Rid));
    BAIL_ON_NT_STATUS(ntStatus);

    *phGroup = hGroup;
    *pRid    = Rid;

cleanup:
    LwFreeUnicodeString(&GroupName);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (phGroup)
    {
        *phGroup = NULL;
    }

    if (pRid)
    {
        *pRid = 0;
    }

    goto cleanup;
}
