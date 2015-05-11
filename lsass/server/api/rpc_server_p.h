/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        rpc_server_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _RPC_SERVER_P_H_
#define _RPC_SERVER_P_H_

typedef struct lsa_rpc_server {
    PSTR                        pszSrvLibPath;
    PSTR                        pszName;
    PVOID                       phLib;
    PFNSHUTDOWNRPCSRV           pfnShutdownSrv;
    PLSA_RPCSRV_FUNCTION_TABLE  pfnTable;
    struct lsa_rpc_server       *pNext;
} LSA_RPC_SERVER, *PLSA_RPC_SERVER;


DWORD
LsaCheckInvalidRpcServer(
    PVOID pSymbol,
    PCSTR pszLibPath
    );


DWORD
LsaSrvInitRpcServer(
    PLSA_RPC_SERVER pRpc
    );


DWORD
LsaSrvInitRpcServers(
    VOID
    );


void
LsaStartRpcServers(
    PLSA_RPC_SERVER pRpcServerList
    );


void
LsaStopRpcServers(
    PLSA_RPC_SERVER pRpcServerList
    );


DWORD
LsaValidateRpcServer(
    PLSA_RPC_SERVER pRpc
    );


DWORD
LsaRpcReadRegistry(
    PLSA_RPC_SERVER *ppRpcSrvList
    );


VOID
LsaSrvAppendRpcServerList(
    PLSA_RPC_SERVER pRpcServer,
    PLSA_RPC_SERVER *ppRpcServerList
    );


void
LsaSrvFreeRpcServer(
    PLSA_RPC_SERVER pSrv
    );


void
LsaSrvFreeRpcServerList(
    PLSA_RPC_SERVER pRpcServerList
    );


void
LsaSrvFreeRpcServerListWithoutStopping(
    PLSA_RPC_SERVER pRpcServerList
    );


VOID
LsaSrvFreeRpcServers(
    VOID
    );


#endif /* _RPC_SERVER_P_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
