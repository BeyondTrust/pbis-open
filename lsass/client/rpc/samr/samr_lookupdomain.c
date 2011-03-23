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
 *        samr_lookupdomain.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrLookupDomain function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrLookupDomain(
    IN  SAMR_BINDING    hBinding,
    IN  CONNECT_HANDLE  hConn,
    IN  PCWSTR          pwszDomainName,
    OUT PSID           *ppSid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    UNICODE_STRING DomainName = {0};
    DWORD dwSidSize = 0;
    PSID pSid = NULL;
    PSID pRetSid = NULL;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(hConn, ntStatus);
    BAIL_ON_INVALID_PTR(pwszDomainName, ntStatus);
    BAIL_ON_INVALID_PTR(ppSid, ntStatus);

    dwError = LwAllocateUnicodeStringFromWc16String(
                                    &DomainName,
                                    pwszDomainName);
    BAIL_ON_WIN_ERROR(dwError);

    DCERPC_CALL(ntStatus, cli_SamrLookupDomain((handle_t)hBinding,
                                               hConn,
                                               &DomainName,
                                               &pSid));
    BAIL_ON_NT_STATUS(ntStatus);

    if (pSid)
    {
        dwSidSize = RtlLengthRequiredSid(pSid->SubAuthorityCount);
        ntStatus = SamrAllocateMemory(OUT_PPVOID(&pRetSid),
                                      dwSidSize);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RtlCopySid(dwSidSize,
                              pRetSid,
                              pSid);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppSid = pRetSid;

cleanup:
    LwFreeUnicodeString(&DomainName);

    if (pSid)
    {
        SamrFreeStubDomSid(pSid);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pRetSid)
    {
        SamrFreeMemory(pRetSid);
    }

    if (ppSid)
    {
        *ppSid = NULL;
    }

    goto cleanup;
}
