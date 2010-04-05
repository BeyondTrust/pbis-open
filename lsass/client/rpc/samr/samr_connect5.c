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
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrConnect5 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrConnect5(
    IN  handle_t         hSamrBinding,
    IN  PCWSTR           pwszSysName,
    IN  UINT32           AccessMask,
    IN  UINT32           LevelIn,
    IN  SamrConnectInfo *pInfoIn,
    IN  PUINT32          pLevelOut,
    OUT SamrConnectInfo *pInfoOut,
    OUT CONNECT_HANDLE  *phConn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WCHAR wszDefaultSysName[] = SAMR_DEFAULT_SYSNAME;
    PWSTR pwszSystemName = NULL;
    UINT32 SystemNameLen = 0;
    CONNECT_HANDLE hConn = NULL;
    UINT32 Level = 0;
    SamrConnectInfo Info;

    BAIL_ON_INVALID_PTR(hSamrBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pInfoIn, ntStatus);
    BAIL_ON_INVALID_PTR(pLevelOut, ntStatus);
    BAIL_ON_INVALID_PTR(pInfoOut, ntStatus);
    BAIL_ON_INVALID_PTR(phConn, ntStatus);

    memset(&Info, 0, sizeof(Info));

    pwszSystemName = wc16sdup((pwszSysName) ?
                              pwszSysName : &(wszDefaultSysName[0]));
    BAIL_ON_NULL_PTR(pwszSystemName, ntStatus);

    SystemNameLen = (UINT32) wc16slen(pwszSystemName) + 1;

    DCERPC_CALL(ntStatus, cli_SamrConnect5(hSamrBinding,
                                           SystemNameLen,
                                           pwszSystemName,
                                           AccessMask,
                                           LevelIn,
                                           pInfoIn,
                                           &Level,
                                        &Info,
                                        &hConn));
    BAIL_ON_NT_STATUS(ntStatus);

    *pLevelOut = Level;
    *pInfoOut  = Info;
    *phConn    = hConn;

cleanup:
    SAFE_FREE(pwszSystemName);

    return ntStatus;

error:
    if (pInfoOut)
    {
        memset(pInfoOut, 0, sizeof(*pInfoOut));
    }

    *phConn    = NULL;
    *pLevelOut = 0;

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
