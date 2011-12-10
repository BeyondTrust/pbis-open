/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        lwdns.h
 *
 * Abstract:
 *
 *        Likewise DNS (LWDNS)
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
    PSOCKADDR_IN pAddr,
    PCSTR  pszHostNameFQDN
    );

DWORD
DNSUpdateSecure(
    HANDLE hDNSServer,
    PCSTR  pszServerName,
    PCSTR  pszDomainName,
    PCSTR  pszHostname,
    DWORD  dwNumAddrs,
    PSOCKADDR_IN pAddrArray
    );

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
