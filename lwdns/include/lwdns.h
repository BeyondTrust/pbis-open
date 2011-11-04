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

#define LWDNS_ERROR_SUCCESS             0
#define LWDNS_ERROR_INIT_FAILED         0xE000 // 57344
#define LWDNS_ERROR_RECORD_NOT_FOUND    0xE001 // 57345
#define LWDNS_ERROR_BAD_RESPONSE        0xE002 // 57346
#define LWDNS_ERROR_PASSWORD_EXPIRED    0xE003 // 57347
#define LWDNS_ERROR_PASSWORD_MISMATCH   0xE004 // 57348
#define LWDNS_ERROR_CLOCK_SKEW          0xE005 // 57349
#define LWDNS_ERROR_KRB5_NO_KEYS_FOUND  0xE006 // 57350
#define LWDNS_ERROR_KRB5_CALL_FAILED    0xE007 // 57351
#define LWDNS_ERROR_RCODE_UNKNOWN       0xE008 // 57352
#define LWDNS_ERROR_RCODE_FORMERR       0xE009 // 57353
#define LWDNS_ERROR_RCODE_SERVFAIL      0xE00A // 57354
#define LWDNS_ERROR_RCODE_NXDOMAIN      0xE00B // 57355
#define LWDNS_ERROR_RCODE_NOTIMP        0xE00C // 57356
#define LWDNS_ERROR_RCODE_REFUSED       0xE00D // 57357
#define LWDNS_ERROR_RCODE_YXDOMAIN      0xE00E // 57358
#define LWDNS_ERROR_RCODE_YXRRSET       0xE00F // 57359
#define LWDNS_ERROR_RCODE_NXRRSET       0xE010 // 57360
#define LWDNS_ERROR_RCODE_NOTAUTH       0xE011 // 57361
#define LWDNS_ERROR_RCODE_NOTZONE       0xE012 // 57362
#define LWDNS_ERROR_NO_NAMESERVER       0xE013 // 57363
#define LWDNS_ERROR_NO_SUCH_ZONE        0xE014 // 57364
#define LWDNS_ERROR_NO_RESPONSE         0xE015 // 57365
#define LWDNS_ERROR_UNEXPECTED          0xE016 // 57366
#define LWDNS_ERROR_NO_SUCH_ADDRESS     0xE017 // 57367
#define LWDNS_ERROR_UPDATE_FAILED       0xE018 // 57368
#define LWDNS_ERROR_NO_INTERFACES       0xE019 // 57369
#define LWDNS_ERROR_INVALID_IP_ADDRESS  0xE01A // 57370
#define LWDNS_ERROR_STRING_CONV_FAILED  0xE01B // 57371
#define LWDNS_ERROR_INVALID_PARAMETER   0xE01C // 57372
#define LWDNS_ERROR_SENTINEL            0xE01D // 57373

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

size_t
DNSGetErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    );

DWORD
DNSMapHerrno(
    DWORD dwHerrno
    );

#endif /* __LWDNS_H__ */
