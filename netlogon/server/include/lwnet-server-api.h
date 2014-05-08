/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        lwnet-server-api.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        Server API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#ifndef __LWNETSRVAPI_H__
#define __LWNETSRVAPI_H__

#include "lwnet.h"

#define LWNET_RESOLVE_HOST_NONE 0
#define LWNET_RESOLVE_HOST_DNS 1
#define LWNET_RESOLVE_HOST_NETBIOS 2
#define LWNET_RESOLVE_HOST_WINS 4

DWORD
LWNetSrvApiInit(
    VOID
    );

DWORD
LWNetSrvApiShutdown(
    VOID
    );

DWORD
LWNetSrvOpenServer(
    uid_t peerUID,
    gid_t peerGID,
    PHANDLE phServer
    );

void
LWNetSrvCloseServer(
    HANDLE hServer
    );

DWORD
LWNetSrvGetDCName(
    IN PCSTR pszServerName,
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN OPTIONAL PCSTR pszPrimaryDomain,
    IN DWORD dwDsFlags,
    IN DWORD dwBlackListCount,
    IN PSTR* ppszAddressBlackList,
    OUT PLWNET_DC_INFO* ppDcInfo
    );

DWORD
LWNetSrvGetDCList(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PLWNET_DC_ADDRESS* ppDcList,
    OUT PDWORD pdwDcCount
    );

DWORD
LWNetSrvGetDCTime(
    IN PCSTR pszDomainFQDN,
    OUT PLWNET_UNIX_TIME_T pDCTime
    );

DWORD
LWNetSrvGetDomainController(
    IN PCSTR pszDomainFQDN,
    OUT PSTR* ppszDomainControllerFQDN
    );


// Netlogon service event types
#define SERVICE_EVENT_CATEGORY      "Service"
#define NETWORK_EVENT_CATEGORY      "Network"

// Netlogon service events
#define LWNET_EVENT_INFO_SERVICE_STARTED                             1000
#define LWNET_EVENT_ERROR_SERVICE_START_FAILURE                      1001
#define LWNET_EVENT_INFO_SERVICE_STOPPED                             1002
#define LWNET_EVENT_ERROR_SERVICE_STOPPED                            1003
#define LWNET_EVENT_INFO_SERVICE_CONFIGURATION_CHANGED               1004

#define LWNB_NETBIOS_FLAGS_RESOLVE_WORKSTATION 0x0001
#define LWNB_NETBIOS_FLAGS_RESOLVE_DC 0x0002
#define LWNB_NETBIOS_FLAGS_RESOLVE_FILE_SERVICE 0x0004
#define LWNB_NETBIOS_FLAGS_MODE_BROADCAST 0x0008
#define LWNB_NETBIOS_FLAGS_MODE_WINS 0x0010
#define LWNB_NETBIOS_FLAGS_MODE_HYBRID \
    (LWNB_NETBIOS_FLAGS_MODE_WINS|LWNB_NETBIOS_FLAGS_MODE_BROADCAST)


DWORD
LWNetSrvLogInformationEvent(
    DWORD dwEventID,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

DWORD
LWNetSrvLogWarningEvent(
    DWORD dwEventID,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

DWORD
LWNetSrvLogErrorEvent(
    DWORD dwEventID,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );


DWORD 
LWNetSrvStartNetBios(
    VOID
    );


DWORD LWNetNbResolveName(
    IN PSTR pszHostName,
    IN UINT16 flags,
    OUT struct in_addr **retAddrs,
    OUT PDWORD retAddrsLen
    );

VOID
LWNetNbAddressListFree(
    IN struct in_addr *retAddrs
    );

DWORD
LWNetConfigResolveNameOrder(
    PDWORD *nameOrder,
    PDWORD nameOrderLen
    );

#endif /* __LWNETSRVAPI_H__ */

