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
 *        lwnet-utils.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        System utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __LWNET_UTILS_H__
#define __LWNET_UTILS_H__

#include <reg/lwreg.h>
#include <lw/rtllog.h>

typedef struct __DLINKEDLIST
{
    PVOID pItem;
    
    struct __DLINKEDLIST * pNext;
    
    struct __DLINKEDLIST * pPrev;
    
} DLINKEDLIST, *PDLINKEDLIST;

typedef VOID (*PFN_DLINKEDLIST_FUNC)(PVOID pData, PVOID pUserData);

typedef DWORD (*PFN_LWNET_FOREACH_STACK_ITEM)(PVOID pItem, PVOID pUserData);

typedef struct __LWNET_STACK
{
    PVOID pItem;
    
    struct __LWNET_STACK * pNext;
    
} LWNET_STACK, *PLWNET_STACK;

//defined flags in dwOptions
#define LWNET_CFG_OPTION_STRIP_SECTION          0x00000001
#define LWNET_CFG_OPTION_STRIP_NAME_VALUE_PAIR  0x00000002
#define LWNET_CFG_OPTION_STRIP_ALL (LWNET_CFG_OPTION_STRIP_SECTION |      \
                                     LWNET_CFG_OPTION_STRIP_NAME_VALUE_PAIR)

// This standardizes the width to 64 bits.  This is useful for
/// writing to files and such.

// This is in seconds (or milliseconds, microseconds, nanoseconds) since
// Jan 1, 1970.
#ifndef LWNET_UNIX_TIME_T_DEFINED
typedef int64_t LWNET_UNIX_TIME_T, *PLWNET_UNIX_TIME_T;
#define LWNET_UNIX_TIME_T_DEFINED
#endif
typedef int64_t LWNET_UNIX_MS_TIME_T, *PLWNET_UNIX_MS_TIME_T;
typedef int64_t LWNET_UNIX_US_TIME_T, *PLWNET_UNIX_US_TIME_T;
typedef int64_t LWNET_UNIX_NS_TIME_T, *PLWNET_UNIX_NS_TIME_T;

// This is in 100ns units from Jan 1, 1601:
typedef int64_t LWNET_WINDOWS_TIME_T, *PLWNET_WINDOWS_TIME_T;

/*
 * Configuration
 */
#define BAIL_ON_NON_LWREG_ERROR(dwError) \
    if (!(40700 <= dwError && dwError <= 41200)) {\
        BAIL_ON_LWNET_ERROR(dwError); \
    }

typedef struct _DNS_SERVER_INFO
{
    PSTR pszName;
    PSTR pszAddress;
} DNS_SERVER_INFO, *PDNS_SERVER_INFO;

//
// Logging
//

#define _LWNET_LOG_AT(Level, ...) LW_RTL_LOG_AT_LEVEL(Level, "netlogon", __VA_ARGS__)
#define LWNET_LOG_ALWAYS(...) _LWNET_LOG_AT(LW_RTL_LOG_LEVEL_ALWAYS, __VA_ARGS__)
#define LWNET_LOG_ERROR(...) _LWNET_LOG_AT(LW_RTL_LOG_LEVEL_ERROR, __VA_ARGS__)
#define LWNET_LOG_WARNING(...) _LWNET_LOG_AT(LW_RTL_LOG_LEVEL_WARNING, __VA_ARGS__)
#define LWNET_LOG_INFO(...) _LWNET_LOG_AT(LW_RTL_LOG_LEVEL_INFO, __VA_ARGS__)
#define LWNET_LOG_VERBOSE(...) _LWNET_LOG_AT(LW_RTL_LOG_LEVEL_VERBOSE, __VA_ARGS__)
#define LWNET_LOG_DEBUG(...) _LWNET_LOG_AT(LW_RTL_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LWNET_LOG_TRACE(...) _LWNET_LOG_AT(LW_RTL_LOG_LEVEL_TRACE, __VA_ARGS__)

#define LWNET_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

//
// DNS resolver library utils
//
extern pthread_mutex_t gLwnetResolverLock;

DWORD
LWNetAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    );

DWORD
LWNetReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    );


void
LWNetFreeMemory(
    PVOID pMemory
    );


DWORD
LWNetAllocateString(
    PCSTR pszInputString, 
    PSTR *ppszOutputString
    );


void
LWNetFreeString(
    PSTR pszString
    );

void
LWNetFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    );

void
LWNetFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    );

DWORD
LWNetDLinkedList(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

DWORD
LWNetDLinkedListAppend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

BOOLEAN
LWNetDLinkedListDelete(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

VOID
LWNetDLinkedListForEach(
    PDLINKEDLIST          pList,
    PFN_DLINKEDLIST_FUNC pFunc,
    PVOID                pUserData
    );

VOID
LWNetDLinkedListFree(
    PDLINKEDLIST pList
    );

DWORD
LWNetStackPush(
    PVOID pItem,
    PLWNET_STACK* ppStack
    );

PVOID
LWNetStackPop(
    PLWNET_STACK* ppStack
    );

PVOID
LWNetStackPeek(
    PLWNET_STACK pStack
    );

DWORD
LWNetStackForeach(
    PLWNET_STACK pStack,
    PFN_LWNET_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    );

PLWNET_STACK
LWNetStackReverse(
    PLWNET_STACK pStack
    );

VOID
LWNetStackFree(
    PLWNET_STACK pStack
    );

DWORD
LWNetDnsGetHostInfo(
    OUT OPTIONAL PSTR* ppszHostname,
    OUT OPTIONAL PSTR* ppszDomain
    );

// TODO: Add corresponding free call for this!!!
DWORD
LWNetDnsGetNameServerList(
    OUT PSTR** pppszNameServerList,
    OUT PDWORD pdwNumServers
    );

DWORD
LWNetDnsSrvQuery(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwFlags,
    OUT PDNS_SERVER_INFO* ppServerArray,
    OUT PDWORD pdwServerCount
    );

DWORD
LWNetReadNextLine(
    FILE* fp,
    PSTR *output,
    PBOOLEAN pbEndOfFile
    );

DWORD
LWNetGetPrefixDirPath(
    PSTR* ppszPath
    );

DWORD
LWNetGetLibDirPath(
    PSTR* ppszPath
    );

#if !HAVE_DECL_ISBLANK && !defined(isblank)
#define isblank(c) ((c) == '\t' || (c) == ' ')
#endif

int
LWNetStrError(
    int errnum,
    char *pszBuf,
    size_t buflen
    );

//Convert to seconds string of form ##s, ##m, ##h, or ##d
//where s,m,h,d = seconds, minutes, hours, days.
DWORD
LWNetParseDateString(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    );

DWORD
LWNetSetSystemTime(
    IN LWNET_UNIX_TIME_T Time
    );

DWORD
LWNetGetSystemTime(
    OUT PLWNET_UNIX_TIME_T pTime
    );

DWORD
LWNetGetSystemTimeInMs(
    OUT PLWNET_UNIX_MS_TIME_T pTime
    );

DWORD
LWNetGetSystemTimeInUs(
    OUT PLWNET_UNIX_US_TIME_T pTime
    );

DWORD
LWNetGetSystemTimeInNs(
    OUT PLWNET_UNIX_NS_TIME_T pTime
    );

DWORD
LWNetTimeInMsToTimespec(
    IN LWNET_UNIX_MS_TIME_T Time,
    OUT struct timespec* Timespec
    );

DWORD
LWNetSleepInMs(
    IN LWNET_UNIX_MS_TIME_T Time
    );

DWORD
LWNetCrackLdapTime(
    IN PCSTR pszStrTime,
    OUT struct tm* pTm
    );

#endif /* __LWNET_UTILS_H__ */


