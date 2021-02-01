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
 *        cachedb.h
 *
 * Abstract:
 *
 *        BeyondTrust Netlogon (LWNET)
 * 
 *        Caching for BeyondTrust Netlogon
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#ifndef __CACHEDB_H__
#define __CACHEDB_H__

#include <lwnet-utils.h>

//
// Default DB location
//

#define NETLOGON_DB_DIR LWNET_CACHE_DIR "/db"
#define NETLOGON_DB     NETLOGON_DB_DIR "/netlogon-cache.filedb"

//
// Types for low-level API
//

// ISSUE-2008/07/01-dalmeida -- These should only be here if we do export
typedef DWORD LWNET_CACHE_DB_QUERY_TYPE;

#define LWNET_CACHE_DB_QUERY_TYPE_DC  0
#define LWNET_CACHE_DB_QUERY_TYPE_GC  1
#define LWNET_CACHE_DB_QUERY_TYPE_PDC 2

typedef struct _LWNET_CACHE_DB_ENTRY {
    PSTR pszDnsDomainName;
    PSTR pszSiteName;
    LWNET_CACHE_DB_QUERY_TYPE QueryType;
    LWNET_UNIX_TIME_T LastDiscovered;
    LWNET_UNIX_TIME_T LastPinged;

    // If set, this entry was a result of having to back
    // off from the standard result set to get a writable DC.
    BOOLEAN IsBackoffToWritableDc;
    LWNET_UNIX_TIME_T LastBackoffToWritableDc;

    LWNET_DC_INFO DcInfo;
} LWNET_CACHE_DB_ENTRY, *PLWNET_CACHE_DB_ENTRY;


typedef struct _LWNET_CACHE_REGISTRY_VALUE
{
    PSTR pszValueName;
    DWORD dwType;
    void* pValue;
    DWORD dwValueLen;
} LWNET_CACHE_REGISTRY_VALUE, *PLWNET_CACHE_REGISTRY_VALUE;

// ISSUE-2008/07/01-dalmeida -- simplify declaraion of handle type
typedef struct _LWNET_CACHE_DB_HANDLE_DATA  LWNET_CACHE_DB_HANDLE_DATA, *PLWNET_CACHE_DB_HANDLE_DATA;
typedef PLWNET_CACHE_DB_HANDLE_DATA LWNET_CACHE_DB_HANDLE, *PLWNET_CACHE_DB_HANDLE;


//
// Low-level API (for tools)
//
DWORD
LWNetCacheDbOpen(
    IN PCSTR Path,
    IN BOOLEAN bIsWrite,
    OUT PLWNET_CACHE_DB_HANDLE pDbHandle
    );

VOID
LWNetCacheDbClose(
    IN OUT PLWNET_CACHE_DB_HANDLE pDbHandle
    );

DWORD
LWNetCacheDbSetup(
    IN LWNET_CACHE_DB_HANDLE DbHandle
    );

DWORD
LWNetCacheDbQuery(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT PLWNET_UNIX_TIME_T LastDiscovered,
    OUT PLWNET_UNIX_TIME_T LastPinged,
    OUT PBOOLEAN IsBackoffToWritableDc,
    OUT PLWNET_UNIX_TIME_T LastBackoffToWritableDc
    );

DWORD
LWNetCacheDbScavenge(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN LWNET_UNIX_TIME_T PositiveCacheAge,
    IN LWNET_UNIX_TIME_T NegativeCacheAge
    );

DWORD
LWNetCacheDbExport(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    OUT PLWNET_CACHE_DB_ENTRY* ppEntries,
    OUT PDWORD pdwCount
    );

//
// High-level API for server
//
DWORD
LWNetCacheInitialize(
    );

VOID
LWNetCacheCleanup(
    );

DWORD
LWNetCacheQuery(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT PLWNET_UNIX_TIME_T LastDiscovered,
    OUT PLWNET_UNIX_TIME_T LastPinged,
    OUT PBOOLEAN IsBackoffToWritableDc,
    OUT PLWNET_UNIX_TIME_T LastBackoffToWritableDc
    );

DWORD
LWNetCacheUpdate(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    IN LWNET_UNIX_TIME_T LastDiscovered,
    IN LWNET_UNIX_TIME_T LastPinged,
    IN BOOLEAN IsBackoffToWritableDc,
    IN OPTIONAL LWNET_UNIX_TIME_T LastBackoffToWritableDc,
    IN PLWNET_DC_INFO pDcInfo
    );

DWORD
LWNetCacheScavenge(
    IN LWNET_UNIX_TIME_T PositiveCacheAge,
    IN LWNET_UNIX_TIME_T NegativeCacheAge
    );

#endif /* __CACHEDB_H__ */
