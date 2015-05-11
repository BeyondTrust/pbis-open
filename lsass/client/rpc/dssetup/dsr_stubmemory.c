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

#include "includes.h"


static
VOID
DsrCleanStubDsRoleInfo(
    PDSR_ROLE_INFO  pInfo,
    WORD            swLevel
    )
{
    unsigned32 rpcStatus = 0;

    switch (swLevel)
    {
    case DS_ROLE_BASIC_INFORMATION:
        rpc_sm_client_free(pInfo->Basic.pwszDomain, &rpcStatus);
        rpc_sm_client_free(pInfo->Basic.pwszDnsDomain, &rpcStatus);
        rpc_sm_client_free(pInfo->Basic.pwszForest, &rpcStatus);
        break;

    case DS_ROLE_UPGRADE_STATUS:
    case DS_ROLE_OP_STATUS:
    default:
        break;
    }
}


VOID
DsrFreeStubDsRoleInfo(
    PDSR_ROLE_INFO  pInfo,
    WORD            swLevel
    )
{
    unsigned32 rpcStatus = 0;

    DsrCleanStubDsRoleInfo(pInfo, swLevel);
    rpc_sm_client_free(pInfo, &rpcStatus);
}
