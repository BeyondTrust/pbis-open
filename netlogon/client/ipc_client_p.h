/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        ipc_client_p.h
 *  
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Private Header (Library)
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __IPC_CLIENT_P_H__
#define __IPC_CLIENT_P_H__

#include <lwmsg/lwmsg.h>

typedef struct __LWNET_CLIENT_CONNECTION_CONTEXT
{
    LWMsgProtocol* pProtocol;
    LWMsgPeer* pClient;
    LWMsgSession* pSession;
} LWNET_CLIENT_CONNECTION_CONTEXT, *PLWNET_CLIENT_CONNECTION_CONTEXT;

DWORD
LWNetAcquireCall(
    HANDLE hConnection,
    LWMsgCall** ppCall
    );

DWORD
LWNetOpenServer(
    PHANDLE phConnection
    );

DWORD
LWNetCloseServer(
    HANDLE hConnection
    );

DWORD
LWNetTransactGetDCName(
    HANDLE hConnection,
    PCSTR pszServerFQDN,
    PCSTR pszDomainFQDN,
    PCSTR pszSiteName,
    PCSTR pszPrimaryDomain,
    DWORD dwFlags,
    DWORD dwBlackListCount,
    PSTR* ppszAddressBlackList,
    PLWNET_DC_INFO* ppDCInfo
    );

DWORD
LWNetTransactGetDCList(
    IN HANDLE hConnection,
    IN PCSTR pszDomainFQDN,
    IN PCSTR pszSiteName,
    IN DWORD dwFlags,
    OUT PLWNET_DC_ADDRESS* ppDcList,
    OUT LW_PDWORD pdwDcCount
    );

DWORD
LWNetTransactGetDCTime(
    HANDLE hConnection,
    PCSTR pszDomainFQDN,
    PLWNET_UNIX_TIME_T pDCTime
    );

DWORD
LWNetTransactGetDomainController(
    HANDLE hConnection,
    PCSTR pszDomainFQDN,
    PSTR* ppszDomainControllerFQDN
    );

DWORD
LWNetTransactResolveName(
    LW_IN HANDLE hConnection,
    LW_IN LW_PCWSTR pcwszHostName,
    LW_OUT LW_PWSTR *pwszCanonName,
    LW_OUT PLWNET_RESOLVE_ADDR **pppAddressList,
    LW_OUT PDWORD pdwAddressListLen
    );

#endif /* __IPC_CLIENT_P_H__ */
