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
 *        lwdns.h
 *
 * Abstract:
 *
 *        BeyondTrust DNS (LWDNS)
 *
 *        Public API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LWDNS_H__
#define __LWDNS_H__

#ifndef _WIN32
#include <lw/types.h>
#include <lw/attrs.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef struct sockaddr_in SOCKADDR_IN, *PSOCKADDR_IN;
#ifndef HAVE_HPUX_OS
typedef struct sockaddr_in6 SOCKADDR_IN6, *PSOCKADDR_IN6;
#endif

#endif


#define LWDNS_ERROR_MASK(_e_)           (_e_ & 0xE000)

/*
 * Logging
 */
#define LWDNS_INFO_TAG     "INFO"
#define LWDNS_ERROR_TAG    "ERROR"
#define LWDNS_WARN_TAG     "WARNING"
#define LWDNS_INFO_TAG     "INFO"
#define LWDNS_VERBOSE_TAG  "VERBOSE"
#define LWDNS_DEBUG_TAG    "VERBOSE"

#define LWDNS_LOG_TIME_FORMAT "%Y%m%d%H%M%S"

typedef enum
{
    LWDNS_LOG_LEVEL_ALWAYS = 0,
    LWDNS_LOG_LEVEL_ERROR,
    LWDNS_LOG_LEVEL_WARNING,
    LWDNS_LOG_LEVEL_INFO,
    LWDNS_LOG_LEVEL_VERBOSE,
    LWDNS_LOG_LEVEL_DEBUG
} LWDNSLogLevel;

typedef VOID (*PFN_LWDNS_LOG_MESSAGE)(
                LWDNSLogLevel logLevel,
                PCSTR         pszFormat,
                va_list       args);

typedef struct __LW_NS_INFO
{
    PSTR  pszNSHostName;
    DWORD dwIP;
} LW_NS_INFO, *PLW_NS_INFO;

typedef struct __LW_INTERFACE_INFO
{
#ifndef HAVE_HPUX_OS
    BOOLEAN         bIPV6Enabled;
    struct sockaddr_in6 ipv6Addr;
#endif
    PSTR            pszName;
    struct sockaddr ipAddr;
    DWORD           dwFlags;
} LW_INTERFACE_INFO, *PLW_INTERFACE_INFO;

DWORD
DNSInitialize(
    VOID
    );

VOID
DNSSetLogParameters(
    LWDNSLogLevel maxLogLevel,
    PFN_LWDNS_LOG_MESSAGE pfnLogMessage
    );

VOID
DNSLogMessage(
    PFN_LWDNS_LOG_MESSAGE pfnLogger,
    LWDNSLogLevel         logLevel,
    PCSTR                 pszFormat,
    ...
    );

DWORD
DNSAllocateMemory(
    DWORD  dwSize,
    PVOID* ppMemory
    );

DWORD
DNSReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    );

VOID
DNSFreeMemory(
    PVOID pMemory
    );

DWORD
DNSAllocateString(
    PCSTR pszInputString,
    PSTR* ppszOutputString
    );

VOID
DNSFreeString(
    PSTR pszString
    );

VOID
DNSStrToUpper(
     PSTR pszString
     );

VOID
DNSStrToLower(
     PSTR pszString
     );

DWORD
DNSGetNameServers(
    IN PCSTR pszDomain,
    OUT PSTR* ppszZone,
    OUT PLW_NS_INFO* ppNSInfoList,
    OUT PDWORD pdwNumServers
    );

VOID
DNSFreeNameServerInfoArray(
    PLW_NS_INFO pNSInfoArray,
    DWORD       dwNumInfos
    );

VOID
DNSFreeNameServerInfo(
    PLW_NS_INFO pNSInfo
    );

VOID
DNSFreeNameServerInfoContents(
    PLW_NS_INFO pNSInfo
    );

DWORD
DNSGetNetworkInterfaces(
    PLW_INTERFACE_INFO* ppInterfaceInfoArray,
    PDWORD              pdwNumInterfaces
    );

VOID
DNSFreeNetworkInterfaces(
    PLW_INTERFACE_INFO pInterfaceInfoArray,
    DWORD              dwNumInterfaces
    );

VOID
DNSFreeNetworkInterface(
    PLW_INTERFACE_INFO pInterfaceInfo
    );

VOID
DNSFreeNetworkInterfaceContents(
    PLW_INTERFACE_INFO pInterfaceInfo
    );

DWORD
DNSOpen(
    PCSTR   pszNameServer,
    DWORD   dwType,
    PHANDLE phDNSServer
    );

DWORD
DNSUpdatePtrSecure(
    PSTR *pArgDnsNameArray,
    DWORD dwArgDnsCount,
    PSOCKADDR_IN pAddr,
    PCSTR  pszHostNameFQDN
    );

#ifndef HAVE_HPUX_OS
DWORD
DNSUpdatePtrV6Secure(
    PSOCKADDR_IN6 pAddr,
    PCSTR  pszHostNameFQDN
    );
#endif

DWORD
DNSUpdateSecure(
    HANDLE hDNSServer,
    PCSTR  pszServerName,
    PCSTR  pszDomainName,
    PCSTR  pszHostname,
    DWORD  dwIPV4Count,
#ifndef HAVE_HPUX_OS
    DWORD  dwIPV6Count,
    PSOCKADDR_IN pAddrArray,
    PSOCKADDR_IN6 pAddr6Array);
#else
    PSOCKADDR_IN pAddrArray);
#endif

DWORD
DNSClose(
    HANDLE hDNSServer
    );

DWORD
DNSShutdown();

DWORD
DNSMapHerrno(
    DWORD dwHerrno
    );

#endif /* __LWDNS_H__ */
