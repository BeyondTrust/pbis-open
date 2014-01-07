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
 *        lwnet.h
 *
 * Abstract:
 *
 *        Likewise Netlogon
 * 
 *        Active Directory Site API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#ifndef __LWNET_H__
#define __LWNET_H__

#ifndef _WIN32

#include <lw/types.h>
#include <lw/attrs.h>

#ifndef LWNET_UNIX_TIME_T_DEFINED
// This standardizes the time width to 64 bits.  This is useful for
// writing to files and such w/o worrying about any different sized time_t.
// It is seconds (or milliseconds, microseconds, nanoseconds) since
// the "Unix epoch" (Jan 1, 1970).  Note that negative values represent
// times before the "Unix epoch".
typedef LW_INT64 LWNET_UNIX_TIME_T, *PLWNET_UNIX_TIME_T;
#define LWNET_UNIX_TIME_T_DEFINED
#endif

#endif

#define LWNET_KRB5_CONF_DIRNAME LWNET_CACHE_DIR
#define LWNET_KRB5_CONF_BASENAME "krb5-affinity.conf"
#define LWNET_KRB5_CONF_PATH LWNET_KRB5_CONF_DIRNAME "/" LWNET_KRB5_CONF_BASENAME

#define LWNET_API

//Standard GUID's are 16 bytes long.
#define LWNET_GUID_SIZE 16

//used in LWNET_DC_INFO::ulDomainControllerAddressType
#define DS_INET_ADDRESS 23
#define DS_NETBIOS_ADDRESS 24

//used in LWNET_DC_INFO::Flags
#define DS_PDC_FLAG            0x00000001    //DC is a PDC of a domain
#define DS_BIT1_RESERVED_FLAG  0x00000002    //reserved: should always be 0
#define DS_GC_FLAG             0x00000004    //DC contains GC of a forest
#define DS_LDAP_FLAG           0x00000008    //DC supports an LDAP server
#define DS_DS_FLAG             0x00000010    //DC supports a DS
#define DS_KDC_FLAG            0x00000020    //DC is running a KDC
#define DS_TIMESERV_FLAG       0x00000040    //DC is running the time service
#define DS_CLOSEST_FLAG        0x00000080    //DC is the closest one to the client.
#define DS_WRITABLE_FLAG       0x00000100    //DC has a writable DS
#define DS_GOOD_TIMESERV_FLAG  0x00000200    //DC is running time service and has clock hardware 
#define DS_NDNC_FLAG           0x00000400    //Non-Domain NC
#define DS_PING_FLAGS          0x0000FFFF    //bitmask of flags returned on ping

#define DS_DNS_CONTROLLER_FLAG 0x20000000    //DomainControllerName is a DNS name
#define DS_DNS_DOMAIN_FLAG     0x40000000    //DomainName is a DNS name
#define DS_DNS_FOREST_FLAG     0x80000000    //DnsForestName is a DNS name

#define LWNET_SUPPORTED_DS_OUTPUT_FLAGS   (DS_PDC_FLAG       | \
                                          DS_GC_FLAG        | \
                                          DS_DS_FLAG        | \
                                          DS_KDC_FLAG       | \
                                          DS_TIMESERV_FLAG  | \
                                          DS_CLOSEST_FLAG   | \
                                          DS_WRITABLE_FLAG | \
                                          DS_GOOD_TIMESERV_FLAG)

#define LWNET_UNSUPPORTED_DS_OUTPUT_FLAGS   (DS_NDNC_FLAG    | \
                                            DS_DNS_CONTROLLER_FLAG | \
                                            DS_DNS_DOMAIN_FLAG  | \
                                            DS_DNS_FOREST_FLAG)


//used in DsGetDcName 'Flags' input parameter
#define DS_FORCE_REDISCOVERY            0x00000001

#define DS_DIRECTORY_SERVICE_REQUIRED   0x00000010
#define DS_DIRECTORY_SERVICE_PREFERRED  0x00000020
#define DS_GC_SERVER_REQUIRED           0x00000040
#define DS_PDC_REQUIRED                 0x00000080
#define DS_BACKGROUND_ONLY              0x00000100
#define DS_IP_REQUIRED                  0x00000200
#define DS_KDC_REQUIRED                 0x00000400
#define DS_TIMESERV_REQUIRED            0x00000800
#define DS_WRITABLE_REQUIRED            0x00001000
#define DS_GOOD_TIMESERV_REQUIRED       0x00002000
#define DS_AVOID_SELF                   0x00004000
#define DS_ONLY_LDAP_NEEDED             0x00008000

#define DS_IS_FLAT_NAME                 0x00010000
#define DS_IS_DNS_NAME                  0x00020000

#define DS_RETURN_DNS_NAME              0x40000000
#define DS_RETURN_FLAT_NAME             0x80000000

#define LWNET_SUPPORTED_DS_INPUT_FLAGS    (DS_FORCE_REDISCOVERY           | \
                                          DS_DIRECTORY_SERVICE_REQUIRED   | \
                                          DS_GC_SERVER_REQUIRED           | \
                                          DS_PDC_REQUIRED                 | \
                                          DS_BACKGROUND_ONLY              | \
                                          DS_KDC_REQUIRED                 | \
                                          DS_TIMESERV_REQUIRED            | \
                                          DS_WRITABLE_REQUIRED            | \
                                          DS_GOOD_TIMESERV_REQUIRED       | \
                                          DS_AVOID_SELF)
                                          


#define LWNET_UNSUPPORTED_DS_INPUT_FLAGS   (DS_DIRECTORY_SERVICE_PREFERRED  | \
                                           DS_IP_REQUIRED                   | \
                                           DS_ONLY_LDAP_NEEDED              | \
                                           DS_IS_FLAT_NAME                  | \
                                           DS_IS_DNS_NAME                   | \
                                           DS_RETURN_DNS_NAME               | \
                                           DS_RETURN_FLAT_NAME)

#define _LWNET_MAKE_SAFE_FREE(Pointer, FreeFunction) \
    do { \
        if (Pointer) \
        { \
            (FreeFunction)(Pointer); \
            (Pointer) = NULL; \
        } \
    } while (0)

#define LWNET_SAFE_FREE_DC_INFO(pDcInfo) \
    _LWNET_MAKE_SAFE_FREE(pDcInfo, LWNetFreeDCInfo)

typedef struct _LWNET_DC_INFO
{
    LW_DWORD dwPingTime;
    LW_DWORD dwDomainControllerAddressType;
    LW_DWORD dwFlags;
    LW_DWORD dwVersion;
    LW_WORD wLMToken;
    LW_WORD wNTToken;
    LW_PSTR pszDomainControllerName;
    LW_PSTR pszDomainControllerAddress;
    LW_UCHAR pucDomainGUID[LWNET_GUID_SIZE];
    LW_PSTR pszNetBIOSDomainName;
    LW_PSTR pszFullyQualifiedDomainName;
    LW_PSTR pszDnsForestName;
    LW_PSTR pszDCSiteName;
    LW_PSTR pszClientSiteName;
    LW_PSTR pszNetBIOSHostName;
    LW_PSTR pszUserName;
} LWNET_DC_INFO, *PLWNET_DC_INFO;

typedef struct _LWNET_DC_ADDRESS {
    LW_PSTR pszDomainControllerName;
    LW_PSTR pszDomainControllerAddress;
} LWNET_DC_ADDRESS, *PLWNET_DC_ADDRESS;


typedef enum LWNET_ADDR_TYPE
{
    LWNET_IP_ADDR_V4 = 1,
    LWNET_IP_ADDR_V6
} LWNET_ADDR_TYPE;


typedef union LWNET_ADDR
{
    BYTE Ip4Addr[4];
    BYTE Ip6Addr[16];
} LWNET_ADDR, *PLWNET_ADDR;


typedef struct LWNET_RESOLVE_ADDR
{
    LWNET_ADDR_TYPE AddressType;
    LWNET_ADDR Address;
} LWNET_RESOLVE_ADDR, *PLWNET_RESOLVE_ADDR;


LW_BEGIN_EXTERN_C

LWNET_API
LW_DWORD
LWNetGetDCName(
    LW_IN LW_PCSTR pszServerFQDN,
    LW_IN LW_PCSTR pszDomainFQDN,
    LW_IN LW_PCSTR pszSiteName,
    LW_IN LW_DWORD dwFlags,
    LW_OUT PLWNET_DC_INFO* ppDCInfo
    );

LWNET_API
LW_DWORD
LWNetGetDCNameExt(
    LW_PCSTR pszServerFQDN,
    LW_PCSTR pszDomainFQDN,
    LW_PCSTR pszSiteName,
    LW_PCSTR pszPrimaryDomain,
    LW_DWORD dwFlags,
    LW_IN LW_DWORD dwBlackListCount,
    LW_IN LW_OPTIONAL LW_PSTR* ppszAddressBlackList,
    PLWNET_DC_INFO* ppDCInfo
    );

LWNET_API
LW_DWORD
LWNetGetDCNameWithBlacklist(
    LW_IN LW_PCSTR pszServerFQDN,
    LW_IN LW_PCSTR pszDomainFQDN,
    LW_IN LW_PCSTR pszSiteName,
    LW_IN LW_DWORD dwFlags,
    LW_IN LW_DWORD dwBlackListCount,
    LW_IN LW_OPTIONAL LW_PSTR* ppszAddressBlackList,
    LW_OUT PLWNET_DC_INFO* ppDCInfo
    );

LWNET_API
LW_DWORD
LWNetGetDCList(
    LW_IN LW_PCSTR pszDomainFQDN,
    LW_IN LW_PCSTR pszSiteName,
    LW_IN LW_DWORD dwFlags,
    LW_OUT PLWNET_DC_ADDRESS* ppDcList,
    LW_OUT LW_PDWORD pdwDcCount
    );

LWNET_API
LW_DWORD
LWNetGetDomainController(
    LW_IN LW_PCSTR pszDomainFQDN,
    LW_OUT LW_PSTR* ppszDomainControllerFQDN
    );

LWNET_API
LW_DWORD
LWNetGetDCTime(
    LW_IN LW_PCSTR pszDomainFQDN,
    LW_OUT PLWNET_UNIX_TIME_T pDCTime
    );

LWNET_API
LW_DWORD
LWNetExtendEnvironmentForKrb5Affinity(
    LW_IN LW_BOOLEAN bNoDefault
    );

LWNET_API
LW_VOID
LWNetFreeDCInfo(
    LW_IN LW_OUT PLWNET_DC_INFO pDCInfo
    );

LWNET_API
LW_VOID
LWNetFreeDCList(
    LW_IN LW_OUT PLWNET_DC_ADDRESS pDcList,
    LW_IN LW_DWORD dwDcCount
    );

LWNET_API
LW_VOID
LWNetFreeString(
    LW_IN LW_OUT LW_PSTR pszString
    );

LWNET_API
size_t
LWNetGetErrorString(
    LW_IN LW_DWORD dwErrorCode,
    LW_OUT LW_PSTR pszBuffer,
    LW_IN size_t stBufSize
    );

//
// Log levels
//

typedef LW_DWORD LWNET_LOG_LEVEL, *PLWNET_LOG_LEVEL;

#define LWNET_LOG_LEVEL_ALWAYS  LW_RTL_LOG_LEVEL_ALWAYS
#define LWNET_LOG_LEVEL_ERROR   LW_RTL_LOG_LEVEL_ERROR
#define LWNET_LOG_LEVEL_WARNING LW_RTL_LOG_LEVEL_WARNING
#define LWNET_LOG_LEVEL_INFO    LW_RTL_LOG_LEVEL_INFO
#define LWNET_LOG_LEVEL_VERBOSE LW_RTL_LOG_LEVEL_VERBOSE
#define LWNET_LOG_LEVEL_DEBUG   LW_RTL_LOG_LEVEL_DEBUG
#define LWNET_LOG_LEVEL_TRACE   LW_RTL_LOG_LEVEL_TRACE

LWNET_API
LW_DWORD
LWNetResolveName(
    LW_IN LW_PCWSTR pcwszHostName,
    LW_OUT LW_OPTIONAL LW_PWSTR *ppwszCanonName,
    LW_OUT PLWNET_RESOLVE_ADDR **pppAddressList,
    LW_OUT PDWORD pdwAddressListLen
    );

LWNET_API
LW_DWORD
LWNetResolveNameFree(
    LW_IN LW_OPTIONAL LW_PWSTR pwszCanonName,
    LW_IN PLWNET_RESOLVE_ADDR *ppAddressList,
    LW_IN DWORD dwAddressListLen
    );

LW_END_EXTERN_C


#endif /* __LWNET_H__ */
