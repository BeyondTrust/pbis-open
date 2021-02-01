/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwnet-server-api.h
 *
 * Abstract:
 *
 *        BeyondTrust Site Manager
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
LWNetSrvRefreshConfiguration(
     HANDLE hServer
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

