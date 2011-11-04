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
 *        samr_connect5.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrConnect5 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


NTSTATUS
SamrSrvConnect5(
    /* [in] */ handle_t             hBinding,
    /* [in] */ PCWSTR               pwszSystemName,
    /* [in] */ DWORD                dwAccessMask,
    /* [in] */ DWORD                dwLevelIn,
    /* [in] */ PSAMR_CONNECT_INFO   pInfoIn,
    /* [out] */ PDWORD              pdwLevelOut,
    /* [out] */ PSAMR_CONNECT_INFO  pInfoOut,
    /* [out] */ CONNECT_HANDLE     *hConn
    )
{
    const DWORD dwConnectVersion = 5;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    PCONNECT_CONTEXT pConnCtx = NULL;
    SAMR_CONNECT_INFO Info;
    DWORD dwLevel = 0;

    ntStatus = SamrSrvConnectInternal(hBinding,
                                      pwszSystemName,
                                      dwAccessMask,
                                      dwConnectVersion,
                                      dwLevelIn,
                                      pInfoIn,
                                      &dwLevel,
                                      &Info,
                                      &pConnCtx);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *pdwLevelOut = dwLevel;
    *pInfoOut    = Info;
    *hConn       = (CONNECT_HANDLE)pConnCtx;

cleanup:
    return ntStatus;

error:
    if (pConnCtx)
    {
        SamrSrvConnectContextFree(pConnCtx);
    }

    *pdwLevelOut = 1;
    memset(pInfoOut, 0, sizeof(*pInfoOut));

    *hConn = NULL;
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
