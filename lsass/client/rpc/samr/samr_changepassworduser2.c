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
 *        samr_changepassworduser2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrChangePasswordUser2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrChangePasswordUser2(
    IN  LSA_BINDING  hBinding,
    IN  PCWSTR       pwszHostname,
    IN  PCWSTR       pwszAccount,
    IN  BYTE         ntpass[516],
    IN  BYTE         ntverify[16],
    IN  BYTE         bLmChange,
    IN  BYTE         lmpass[516],
    IN  BYTE         lmverify[16]
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    CryptPassword NtPass;
    CryptPassword LmPass;
    CryptPassword *pLmPass = NULL;
    HashPassword NtVer;
    HashPassword LmVer;
    HashPassword *pLmVer = NULL;
    UNICODE_STRING Server = {0};
    UNICODE_STRING Account = {0};

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszHostname, ntStatus);
    BAIL_ON_INVALID_PTR(pwszAccount, ntStatus);
    BAIL_ON_INVALID_PTR(ntpass, ntStatus);
    BAIL_ON_INVALID_PTR(ntverify, ntStatus);

    memset(&NtPass, 0, sizeof(NtPass));
    memset(&LmPass, 0, sizeof(LmPass));
    memset(&NtVer, 0, sizeof(NtVer));
    memset(&LmVer, 0, sizeof(LmVer));

    dwError = LwAllocateUnicodeStringFromWc16String(
                                    &Server,
                                    pwszHostname);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateUnicodeStringFromWc16String(
                                    &Account,
                                    pwszAccount);
    BAIL_ON_WIN_ERROR(dwError);

    memcpy(NtPass.data, ntpass, sizeof(NtPass.data));
    memcpy(NtVer.data, ntverify, sizeof(NtVer.data));

    if (bLmChange)
    {
        memcpy(LmPass.data, lmpass, sizeof(LmPass.data));
        memcpy(LmVer.data, lmverify, sizeof(LmVer.data));
        pLmPass = &LmPass;
        pLmVer  = &LmVer;

    }
    else
    {
        pLmPass = NULL;
        pLmVer  = NULL;
    }

    DCERPC_CALL(ntStatus,
                cli_SamrChangePasswordUser2((handle_t)hBinding,
                                            &Server,
                                            &Account,
                                            &NtPass,
                                            &NtVer,
                                            bLmChange,
                                            pLmPass,
                                            pLmVer));
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    LwFreeUnicodeString(&Account);
    LwFreeUnicodeString(&Server);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}
