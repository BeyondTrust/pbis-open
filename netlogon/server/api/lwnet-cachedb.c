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
 *        lwnet-cachedb.c
 *
 * Abstract:
 *
 *        Caching for Likewise Netlogon
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#include "includes.h"

#define ENABLE_CACHEDB_DEBUG 0

#define FILEDB_FORMAT_TYPE "LFLT"
#define FILEDB_FORMAT_VERSION 2

struct _LWNET_CACHE_DB_HANDLE_DATA {
    PDLINKEDLIST pCacheList;
    // This RW lock helps us to ensure that we don't stomp
    // ourselves while giving up good parallel access.
    // Note, however, that SQLite might still return busy errors
    // if some other process is trying to poke at the database
    // (which might happen with database debugging or maintenance tools).
    pthread_rwlock_t Lock;
    pthread_rwlock_t* pLock;
};

static LWNET_CACHE_DB_HANDLE gDbHandle;
#define LWNET_NETLOGON_REGISTRY_KEY "Services\\netlogon"
#define LWNET_CACHE_REGISTRY_KEY "cachedb"

LWMsgTypeSpec gLWNetCacheEntrySpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_CACHE_DB_ENTRY),
    LWMSG_MEMBER_PSTR(LWNET_CACHE_DB_ENTRY, pszDnsDomainName),
    LWMSG_MEMBER_PSTR(LWNET_CACHE_DB_ENTRY, pszSiteName),
    LWMSG_MEMBER_UINT32(LWNET_CACHE_DB_ENTRY, QueryType),
    LWMSG_MEMBER_INT64(LWNET_CACHE_DB_ENTRY, LastDiscovered),
    LWMSG_MEMBER_INT64(LWNET_CACHE_DB_ENTRY, LastPinged),
    LWMSG_MEMBER_UINT8(LWNET_CACHE_DB_ENTRY, IsBackoffToWritableDc),
    LWMSG_MEMBER_INT64(LWNET_CACHE_DB_ENTRY, LastBackoffToWritableDc),
    LWMSG_MEMBER_STRUCT_BEGIN(LWNET_CACHE_DB_ENTRY, DcInfo),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwPingTime),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwDomainControllerAddressType),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwFlags),
    LWMSG_MEMBER_UINT16(LWNET_DC_INFO, wLMToken),
    LWMSG_MEMBER_UINT16(LWNET_DC_INFO, wNTToken),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDomainControllerName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDomainControllerAddress),
    LWMSG_MEMBER_ARRAY_BEGIN(LWNET_DC_INFO, pucDomainGUID),
    LWMSG_UINT8(char),
    LWMSG_ARRAY_END,
    LWMSG_ATTR_LENGTH_STATIC(LWNET_GUID_SIZE),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszNetBIOSDomainName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszFullyQualifiedDomainName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDnsForestName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDCSiteName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszClientSiteName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszNetBIOSHostName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszUserName),
    LWMSG_STRUCT_END,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

// ISSUE-2008/07/01-dalmeida -- For now, use exlusive locking as we need to
// verify actual thread safety wrt things like error strings and such.
#define RW_LOCK_ACQUIRE_READ(Lock) \
    pthread_rwlock_wrlock(Lock)

#define RW_LOCK_RELEASE_READ(Lock) \
    pthread_rwlock_unlock(Lock)

#define RW_LOCK_ACQUIRE_WRITE(Lock) \
    pthread_rwlock_wrlock(Lock)

#define RW_LOCK_RELEASE_WRITE(Lock) \
    pthread_rwlock_unlock(Lock)

#if ENABLE_CACHEDB_DEBUG
static
VOID
DebugEntry(
    IN PLWNET_CACHE_DB_ENTRY pEntry
    );

#define DEBUG_ENTRY(pEntry) DebugEntry(pEntry)
#else
#define DEBUG_ENTRY(pEntry)
#endif

static
DWORD
LWNetCacheDbReadFromRegistry(
    LWNET_CACHE_DB_HANDLE dbHandle
    );

static
DWORD
LWNetCacheDbWriteToRegistry(
    PDLINKEDLIST pCacheList
    );

static
VOID
LWNetCacheDbForEachEntryDestroy(
    IN PVOID pData,
    IN PVOID pContext
    );

static
VOID
LWNetCacheDbEntryFree(
    IN OUT PLWNET_CACHE_DB_ENTRY pEntry
    );

static
DWORD
LWNetCacheDbUpdate(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN LWNET_CACHE_DB_QUERY_TYPE QueryType,
    IN LWNET_UNIX_TIME_T LastDiscovered,
    IN LWNET_UNIX_TIME_T LastPinged,
    IN BOOLEAN IsBackoffToWritableDc,
    IN OPTIONAL LWNET_UNIX_TIME_T LastBackoffToWritableDc,
    IN PLWNET_DC_INFO pDcInfo
    );

#if ENABLE_CACHEDB_DEBUG
static
VOID
DebugEntry(
    IN PLWNET_CACHE_DB_ENTRY pEntry
    )
{
    struct tm ldTimeFields = { 0 };
    struct tm lpTimeFields = { 0 };
    struct tm lwTimeFields = { 0 };
    time_t ld = (time_t) pEntry->LastDiscovered;
    time_t lp = (time_t) pEntry->LastPinged;
    time_t lw = (time_t) pEntry->LastBackoffToWritableDc;

    localtime_r(&ld, &ldTimeFields);
    localtime_r(&lp, &lpTimeFields);
    localtime_r(&lw, &lwTimeFields);

    LWNET_LOG_DEBUG("dns=%s, site=%s, type=%u, "
            "ld=%04d%02d%02d-%02d:%02d:%02d, "
            "lp=%04d%02d%02d-%02d:%02d:%02d, "
            "iw=%c, "
            "lw=%04d%02d%02d-%02d:%02d:%02d",
            LWNET_SAFE_LOG_STRING(pEntry->pszDnsDomainName),
            LWNET_SAFE_LOG_STRING(pEntry->pszSiteName),
            pEntry->QueryType,
            ldTimeFields.tm_year,
            ldTimeFields.tm_mon + 1,
            ldTimeFields.tm_mday,
            ldTimeFields.tm_hour,
            ldTimeFields.tm_min,
            ldTimeFields.tm_sec,
            lpTimeFields.tm_year,
            lpTimeFields.tm_mon + 1,
            lpTimeFields.tm_mday,
            lpTimeFields.tm_hour,
            lpTimeFields.tm_min,
            lpTimeFields.tm_sec,
            pEntry->IsBackoffToWritableDc ? 'Y' : 'N',
            lwTimeFields.tm_year,
            lwTimeFields.tm_mon + 1,
            lwTimeFields.tm_mday,
            lwTimeFields.tm_hour,
            lwTimeFields.tm_min,
            lwTimeFields.tm_sec);
    LWNET_LOG_DEBUG("hnb=%s, hdns=%s, haddr=%s (%u), flags=0x%08x, "
            "dcsite=%s, clisite=%s, ddns=%s, dnb=%s, forest=%s, user=%s, "
            "ver=%u, lm=%u, nt=%u, ping=%u, "
            "guid=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
            LWNET_SAFE_LOG_STRING(pEntry->DcInfo.pszNetBIOSHostName),
            LWNET_SAFE_LOG_STRING(pEntry->DcInfo.pszDomainControllerName),
            LWNET_SAFE_LOG_STRING(pEntry->DcInfo.pszDomainControllerAddress),
            pEntry->DcInfo.dwDomainControllerAddressType,
            pEntry->DcInfo.dwFlags,
            LWNET_SAFE_LOG_STRING(pEntry->DcInfo.pszDCSiteName),
            LWNET_SAFE_LOG_STRING(pEntry->DcInfo.pszClientSiteName),
            LWNET_SAFE_LOG_STRING(pEntry->DcInfo.pszFullyQualifiedDomainName),
            LWNET_SAFE_LOG_STRING(pEntry->DcInfo.pszNetBIOSDomainName),
            LWNET_SAFE_LOG_STRING(pEntry->DcInfo.pszDnsForestName),
            LWNET_SAFE_LOG_STRING(pEntry->DcInfo.pszUserName),
            pEntry->DcInfo.dwVersion,
            pEntry->DcInfo.wLMToken,
            pEntry->DcInfo.wNTToken,
            pEntry->DcInfo.dwPingTime,
            pEntry->DcInfo.pucDomainGUID[0],
            pEntry->DcInfo.pucDomainGUID[1],
            pEntry->DcInfo.pucDomainGUID[2],
            pEntry->DcInfo.pucDomainGUID[3],
            pEntry->DcInfo.pucDomainGUID[4],
            pEntry->DcInfo.pucDomainGUID[5],
            pEntry->DcInfo.pucDomainGUID[6],
            pEntry->DcInfo.pucDomainGUID[7],
            pEntry->DcInfo.pucDomainGUID[8],
            pEntry->DcInfo.pucDomainGUID[9],
            pEntry->DcInfo.pucDomainGUID[10],
            pEntry->DcInfo.pucDomainGUID[11],
            pEntry->DcInfo.pucDomainGUID[12],
            pEntry->DcInfo.pucDomainGUID[13],
            pEntry->DcInfo.pucDomainGUID[14],
            pEntry->DcInfo.pucDomainGUID[15]);
}
#endif

DWORD
LWNetCacheDbOpen(
    IN PCSTR Path,
    IN BOOLEAN bIsWrite,
    OUT PLWNET_CACHE_DB_HANDLE pDbHandle
    )
{
    DWORD dwError = 0;
    LONG lError = 0;
    LWNET_CACHE_DB_HANDLE dbHandle = NULL;

    dwError = LWNetAllocateMemory(sizeof(*dbHandle), (PVOID *)&dbHandle);
    BAIL_ON_LWNET_ERROR(dwError);

    lError = pthread_rwlock_init(&dbHandle->Lock, NULL);
    dwError = LwMapErrnoToLwError(lError);
    BAIL_ON_LWNET_ERROR(dwError);

    dbHandle->pLock = &dbHandle->Lock;

    dwError = LWNetCacheDbReadFromRegistry(dbHandle);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    if (dwError)
    {
        LWNetCacheDbClose(&dbHandle);
    }    
    *pDbHandle = dbHandle;
    return dwError;
}

VOID
LWNetCacheDbClose(
    IN OUT PLWNET_CACHE_DB_HANDLE pDbHandle
    )
{
    LWNET_CACHE_DB_HANDLE dbHandle = *pDbHandle;

    if (dbHandle)
    {
        if (dbHandle->pCacheList)
        {
            LWNetCacheDbWriteToRegistry(dbHandle->pCacheList);
            LWNetDLinkedListForEach(
                dbHandle->pCacheList,
                LWNetCacheDbForEachEntryDestroy,
                NULL);
            LWNetDLinkedListFree(dbHandle->pCacheList);
        }
        if (dbHandle->pLock)
        {
            pthread_rwlock_destroy(dbHandle->pLock);
        }

        LWNET_SAFE_FREE_MEMORY(dbHandle);

        *pDbHandle = NULL;
    }
}

static
VOID
LWNetCacheDbForEachEntryDestroy(
    IN PVOID pData,
    IN PVOID pContext
    )
{
    PLWNET_CACHE_DB_ENTRY pEntry = (PLWNET_CACHE_DB_ENTRY)pData;
    LWNetCacheDbEntryFree(pEntry);
}

static
VOID
LWNetCacheDbEntryFree(
    IN OUT PLWNET_CACHE_DB_ENTRY pEntry
    )
{
    if (pEntry)
    {
        LWNET_SAFE_FREE_STRING(pEntry->pszDnsDomainName);
        LWNET_SAFE_FREE_STRING(pEntry->pszSiteName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszDomainControllerName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszDomainControllerAddress);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszNetBIOSDomainName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszFullyQualifiedDomainName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszDnsForestName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszDCSiteName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszClientSiteName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszNetBIOSHostName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszUserName);
        LWNET_SAFE_FREE_MEMORY(pEntry);
    }
}


DWORD
LWNetCacheDbRegistryReadString(
    HANDLE hReg,
    HKEY hKey,
    PSTR pszValueName,
    PSTR *ppszRetString)
{
    DWORD dwError = 0;
    DWORD dwValueLen = 0;
    PSTR pszRetString = NULL;

    dwError = RegGetValueA(
                  hReg,
                  hKey,
                  NULL,
                  pszValueName,
                  REG_SZ,
                  NULL,
                  NULL,
                  &dwValueLen);
    if (dwError == 0 && dwValueLen > 0)
    {
        dwError = LwAllocateMemory(
                      (dwValueLen+1) * sizeof(CHAR),
                      (PVOID) &pszRetString);
        BAIL_ON_LWNET_ERROR(dwError);
        dwError = RegGetValueA(
                      hReg,
                      hKey,
                      NULL,
                      pszValueName,
                      REG_SZ,
                      NULL,
                      pszRetString,
                      &dwValueLen);
 
    }
    *ppszRetString = pszRetString;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LWNetCacheDbRegistryReadValue(
    HANDLE hReg,
    HKEY hKey,
    PSTR pszValueName,
    DWORD regType,
    PVOID *ppValue,
    DWORD dwValueLen)
{
    DWORD dwError = 0;
    DWORD dwValue = 0;
    DWORD dwRetValueLen = 0;
    PWORD pwValue = (PWORD) ppValue;
    PDWORD pdwValue = (PDWORD) ppValue;
    PBYTE *ppData = (PBYTE *) ppValue;

    switch (regType)
    {
        case REG_SZ:
            dwError = LWNetCacheDbRegistryReadString(
                         hReg,
                         hKey,
                         pszValueName,
                         (PSTR *) ppValue);
            BAIL_ON_LWNET_ERROR(dwError);
            break;

        case REG_DWORD:
            dwRetValueLen = sizeof(DWORD);
            dwError = RegGetValueA(
                          hReg,
                          hKey,
                          NULL,
                          pszValueName,
                          REG_DWORD,
                          NULL,
                          &dwValue,
                          &dwRetValueLen);
            BAIL_ON_LWNET_ERROR(dwError);
            if (dwValueLen == sizeof(WORD))
            {
                *pwValue = dwValue;
            }
            else
            {
                *pdwValue = dwValue;
            }
            break;

        case REG_BINARY:
            dwRetValueLen = dwValueLen;
            dwError = RegGetValueA(
                          hReg,
                          hKey,
                          NULL,
                          pszValueName,
                          REG_BINARY,
                          NULL,
                          ppData,
                          &dwRetValueLen);
            BAIL_ON_LWNET_ERROR(dwError);
            break;
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}


DWORD
LWNetCacheDbRegistryInitValueArray(
    PLWNET_CACHE_DB_ENTRY pEntry,
    PLWNET_CACHE_REGISTRY_VALUE *ppValueArray,
    PDWORD pdwArrayEntries)
{
    DWORD dwError = 0;
    PLWNET_CACHE_REGISTRY_VALUE pValueArray = NULL;
    LWNET_CACHE_REGISTRY_VALUE valueArray[] = 
    {
        {
            "DnsDomainName",
            REG_SZ,
            &pEntry->pszDnsDomainName,
            0
        },
        {
            "SiteName",
            REG_SZ,
            &pEntry->pszSiteName,
            0
        },
        {
            "QueryType",
            REG_DWORD,
            &pEntry->QueryType,
            0
        },
        {
            "LastDiscovered",
            REG_BINARY,
            &pEntry->LastDiscovered,
            sizeof(pEntry->LastDiscovered)
        },
        {
            "LastPinged",
            REG_BINARY,
            &pEntry->LastPinged,
            sizeof(pEntry->LastPinged)
        },
        {
            "IsBackoffToWritableDc",
            REG_DWORD,
            &pEntry->IsBackoffToWritableDc,
            0
        },

        {
            "DcInfo-PingTime",
            REG_DWORD,
            &pEntry->DcInfo.dwPingTime,
            0
        },
        {
            "DcInfo-DomainControllerAddressType",
            REG_DWORD,
            &pEntry->DcInfo.dwDomainControllerAddressType,
            0
        },
        {
            "DcInfo-Flags",
            REG_DWORD,
            &pEntry->DcInfo.dwFlags,
            0
        },
        {
            "DcInfo-Version",
            REG_DWORD,
            &pEntry->DcInfo.dwVersion,
            0
        },
        {
            "DcInfo-LMToken",
            REG_DWORD,
            &pEntry->DcInfo.wLMToken,
            sizeof(pEntry->DcInfo.wLMToken)
        },
        {
            "DcInfo-NTToken",
            REG_DWORD,
            &pEntry->DcInfo.wNTToken,
            sizeof(pEntry->DcInfo.wNTToken)
        },
        {
            "DcInfo-DomainControllerName",
            REG_SZ,
            &pEntry->DcInfo.pszDomainControllerName,
            0
        },
        {
            "DcInfo-DomainControllerAddress",
            REG_SZ,
            &pEntry->DcInfo.pszDomainControllerAddress,
            0
        },
        {
            "DcInfo-DomainGUID",
            REG_BINARY,
            &pEntry->DcInfo.pucDomainGUID,
            LWNET_GUID_SIZE
        },
        {
            "DcInfo-NetBIOSDomainName",
            REG_SZ,
            &pEntry->DcInfo.pszNetBIOSDomainName,
            0
        },
        {
            "DcInfo-FullyQualifiedDomainName",
            REG_SZ,
            &pEntry->DcInfo.pszFullyQualifiedDomainName,
            0
        },
        {
            "DcInfo-DnsForestName",
            REG_SZ,
            &pEntry->DcInfo.pszDnsForestName,
            0
        },
        {
            "DcInfo-DCSiteName",
            REG_SZ,
            &pEntry->DcInfo.pszDCSiteName,
            0
        },
        {
            "DcInfo-ClientSiteName",
            REG_SZ,
            &pEntry->DcInfo.pszClientSiteName,
            0
        },
        {
            "DcInfo-NetBIOSHostName",
            REG_SZ,
            &pEntry->DcInfo.pszNetBIOSHostName,
            0
        },
        {
            "DcInfo-UserName",
            REG_SZ,
            &pEntry->DcInfo.pszUserName,
            0
        },
    };

    dwError = LwAllocateMemory(sizeof(valueArray), (PVOID) &pValueArray);
    BAIL_ON_LWNET_ERROR(dwError);

    memcpy(pValueArray, valueArray, sizeof(valueArray));
    *ppValueArray = pValueArray;
    *pdwArrayEntries = sizeof(valueArray)/sizeof(LWNET_CACHE_REGISTRY_VALUE);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LWNetCacheDbRegistryReadValues(
    HANDLE hReg,
    HKEY pKey,
    PLWNET_CACHE_DB_ENTRY pEntry)
{

    DWORD dwError = 0;
    DWORD index = 0;
    DWORD dwArrayEntries = 0;

    PLWNET_CACHE_REGISTRY_VALUE valueArray = NULL;

    dwError = LWNetCacheDbRegistryInitValueArray(
                  pEntry,
                  &valueArray,
                  &dwArrayEntries);
    BAIL_ON_LWNET_ERROR(dwError);

    for (index=0; index<dwArrayEntries; index++)
    {
        dwError = LWNetCacheDbRegistryReadValue(
                      hReg,
                      pKey,
                      valueArray[index].pszValueName,
                      valueArray[index].dwType,
                      valueArray[index].pValue,
                      valueArray[index].dwValueLen);
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:
    LWNET_SAFE_FREE_MEMORY(valueArray);
    return dwError;

error:
    goto cleanup;
}


static
DWORD
LWNetCacheDbReadFromRegistry(
    LWNET_CACHE_DB_HANDLE dbHandle
    )
{
    DWORD dwError = 0;
    HANDLE hReg = NULL;
    LW_WCHAR **ppSubKeys = NULL;
    PSTR pszError = NULL;
    DWORD dwSubKeyCount = 0;
    DWORD i = 0;
    HKEY pNetLogonKey = NULL;
    LWNET_CACHE_DB_ENTRY cacheEntry;

    memset(&cacheEntry, 0, sizeof(cacheEntry));

    /* Open connection to registry */
    dwError = RegOpenServer(&hReg);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = RegUtilIsValidKey(
                  hReg,
                  HKEY_THIS_MACHINE,
                  LWNET_NETLOGON_REGISTRY_KEY "\\" LWNET_CACHE_REGISTRY_KEY);

    /* cachedb entry does not exist until netlogond is stopped, so this
     * is not an error when this entry is not found.
     */
    if (dwError)
    {
        dwError = 0;
        goto cleanup;
    }

    dwError = RegUtilGetKeys(
                  hReg,
                  HKEY_THIS_MACHINE,
                  LWNET_NETLOGON_REGISTRY_KEY,
                  LWNET_CACHE_REGISTRY_KEY,
                  &ppSubKeys,
                  &dwSubKeyCount);
    BAIL_ON_LWNET_ERROR(dwError);
    for (i=0; i<dwSubKeyCount; i++)
    {
        dwError = RegOpenKeyExW(
                      hReg,
                      NULL,
                      ppSubKeys[i],
                      0,
                      KEY_READ,
                      &pNetLogonKey);
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetCacheDbRegistryReadValues(
                      hReg,
                      pNetLogonKey,
                      &cacheEntry);
        if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
        {
            LwWc16sToMbs(ppSubKeys[i],
                         &pszError);
            if (pszError)
            {
                LWNET_LOG_WARNING("Warning: invalid/incomplete registry key "
                                  "'%s'", pszError);
                LWNET_SAFE_FREE_MEMORY(pszError);
            }
            dwError = 0;
            continue;
        }
        BAIL_ON_LWNET_ERROR(dwError);
        RegCloseKey(hReg, pNetLogonKey);
        pNetLogonKey = NULL;

        dwError = LWNetCacheDbUpdate(
                      dbHandle,
                      cacheEntry.pszDnsDomainName,
                      cacheEntry.pszSiteName,
                      cacheEntry.QueryType,
                      cacheEntry.LastDiscovered,
                      cacheEntry.LastPinged,
                      cacheEntry.IsBackoffToWritableDc,
                      cacheEntry.LastBackoffToWritableDc,
                      &cacheEntry.DcInfo);
        BAIL_ON_LWNET_ERROR(dwError);
        memset(&cacheEntry, 0, sizeof(cacheEntry));
    }
cleanup:
    if (hReg)
    {
        if (pNetLogonKey)
        {
            RegCloseKey(hReg, pNetLogonKey);
        }
        RegCloseServer(hReg);
    }
    return dwError;

error:
    goto cleanup;
}


DWORD
LWNetCacheDbRegistryWriteValue(
    HANDLE hReg,
    PSTR pszSubKey,
    PSTR pszValueName,
    DWORD dwType,
    PVOID pValue,
    DWORD dwValueLen)
{
    DWORD dwError = 0;
    DWORD dwDataLen = 0;
    DWORD dwData = 0;
    PVOID pData = NULL;
    PSTR pszValue = NULL;

    switch (dwType)
    {
        case REG_SZ:
            pszValue = *((PSTR *) pValue);
            if (pszValue)
            {
                dwDataLen = strlen(pszValue);
                pData = pszValue;
            }
            else
            {
                pszValue = "";
                dwDataLen = 0;
                pData = (PVOID) pszValue;
            }
            break;

        case REG_DWORD:
        default:
            if (dwValueLen == sizeof(WORD))
            {
                dwData = *((WORD *) pValue);
            }
            else
            {
                dwData = *((DWORD *) pValue);
            }
            pData = (PVOID) &dwData;
            dwDataLen = sizeof(DWORD);
            break;

        case REG_BINARY:
            dwDataLen = dwValueLen;
            pData = (PVOID) pValue;
            break;
    }

    dwError = RegUtilSetValue(
                  hReg,
                  HKEY_THIS_MACHINE,
                  LWNET_NETLOGON_REGISTRY_KEY "\\" LWNET_CACHE_REGISTRY_KEY,
                  pszSubKey,
                  pszValueName,
                  dwType,
                  pData,
                  dwDataLen);
    BAIL_ON_LWNET_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LWNetCacheDbRegistryWriteValues(
    HANDLE hReg,
    PSTR pszSubKey,
    PLWNET_CACHE_DB_ENTRY pEntry)
{

    DWORD dwError = 0;
    DWORD index = 0;
    DWORD dwArrayEntries = 0;
    PLWNET_CACHE_REGISTRY_VALUE valueArray = NULL;

    dwError = LWNetCacheDbRegistryInitValueArray(
                  pEntry,
                  &valueArray,
                  &dwArrayEntries);
    BAIL_ON_LWNET_ERROR(dwError);
              
    for (index=0; index<dwArrayEntries; index++)
    {
        dwError = LWNetCacheDbRegistryWriteValue(
                      hReg,
                      pszSubKey,
                      valueArray[index].pszValueName,
                      valueArray[index].dwType,
                      valueArray[index].pValue,
                      valueArray[index].dwValueLen);
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:
    LWNET_SAFE_FREE_MEMORY(valueArray);
    return dwError;

error:
    goto cleanup;
}


static
DWORD
LWNetCacheDbWriteToRegistry(
    PDLINKEDLIST pCacheList
    )
{
    HANDLE hReg = NULL;
    HKEY pRootKey = NULL;
    DWORD dwError = 0;
    PSTR pszNewCacheKey = NULL;
    PVOID pData = NULL;
    PLWNET_CACHE_DB_ENTRY pCacheDbEntry = NULL;

    /* Open connection to registry */
    dwError = RegOpenServer(&hReg);
    BAIL_ON_LWNET_ERROR(dwError);

    /* Don't care if this fails, just remove the old cache if it exists */
    RegUtilDeleteTree(hReg,
                      HKEY_THIS_MACHINE,
                      LWNET_NETLOGON_REGISTRY_KEY,
                      LWNET_CACHE_REGISTRY_KEY);
                      
    dwError = RegUtilAddKey(
                  hReg,
                  HKEY_THIS_MACHINE,
                  LWNET_NETLOGON_REGISTRY_KEY,
                  LWNET_CACHE_REGISTRY_KEY);
    BAIL_ON_LWNET_ERROR(dwError);

    /*
     * Create new keys for every cached entry, and fill in the values under
     * each key
     */
    while (pCacheList)
    {
        /* Create new registry subkey for each cached entry */
        pCacheDbEntry = (PLWNET_CACHE_DB_ENTRY) pCacheList->pItem;

        dwError = LwAllocateStringPrintf(
                       &pszNewCacheKey, 
                      "%s%s%s-%d",
                       pCacheDbEntry->pszDnsDomainName ?
                           pCacheDbEntry->pszDnsDomainName : "",
                       pCacheDbEntry->pszSiteName ? "-" : "",
                       pCacheDbEntry->pszSiteName ?
                           pCacheDbEntry->pszSiteName : "",
                       (int) pCacheDbEntry->QueryType);
        BAIL_ON_LWNET_ERROR(dwError);
        
        dwError = RegUtilAddKey(
                      hReg,
                      HKEY_THIS_MACHINE,
                      LWNET_NETLOGON_REGISTRY_KEY "\\" LWNET_CACHE_REGISTRY_KEY,
                      pszNewCacheKey);
        BAIL_ON_LWNET_ERROR(dwError);

        /* Add to registry the current entries */
        dwError = LWNetCacheDbRegistryWriteValues(
                      hReg,
                      pszNewCacheKey,
                      pCacheDbEntry);
        BAIL_ON_LWNET_ERROR(dwError);
        LWNET_SAFE_FREE_MEMORY(pszNewCacheKey);

        pCacheList = pCacheList->pNext;
    }

cleanup:
    if (pRootKey)
    {
        RegCloseKey(hReg, pRootKey);
    }
    RegCloseServer(hReg);
    LWNET_SAFE_FREE_MEMORY(pData);
    LWNET_SAFE_FREE_MEMORY(pszNewCacheKey);

    return dwError;

error:
    LWNET_LOG_ERROR("Failed to save cache %s [%d]",
                    HKEY_THIS_MACHINE "\\" LWNET_CACHE_REGISTRY_KEY,
                    dwError);

    goto cleanup;
}


static
LWNET_CACHE_DB_QUERY_TYPE
LWNetCacheDbQueryToQueryType(
    IN DWORD dwDsFlags
    )
{
    LWNET_CACHE_DB_QUERY_TYPE result = LWNET_CACHE_DB_QUERY_TYPE_DC;
    if (dwDsFlags & DS_PDC_REQUIRED)
    {
        result = LWNET_CACHE_DB_QUERY_TYPE_PDC;
    }
    if (dwDsFlags & DS_GC_SERVER_REQUIRED)
    {
        result = LWNET_CACHE_DB_QUERY_TYPE_GC;
    }
    return result;
}

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
    )
{
    DWORD dwError = 0;
    LWNET_CACHE_DB_QUERY_TYPE queryType = 0;
    BOOLEAN isAcquired = FALSE;
    PLWNET_DC_INFO pDcInfo = NULL;
    LWNET_UNIX_TIME_T lastDiscovered = 0;
    LWNET_UNIX_TIME_T lastPinged = 0;
    BOOLEAN isBackoffToWritableDc = FALSE;
    LWNET_UNIX_TIME_T lastBackoffToWritableDc = 0;
    PSTR pszDnsDomainNameLower = NULL;
    PSTR pszSiteNameLower = NULL;
    PLWNET_CACHE_DB_ENTRY pEntry = NULL;
    PDLINKEDLIST pListEntry = NULL;

    if (pszDnsDomainName)
    {
        dwError = LWNetAllocateString(
                      pszDnsDomainName,
                      &pszDnsDomainNameLower);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    else
    {
        LWNET_LOG_DEBUG("Cached entry not found: %s, %s, %u",
                        "",
                        pszSiteNameLower ? pszSiteNameLower : "",
                        queryType);
        goto error;
    }

    LwStrToLower(pszDnsDomainNameLower);

    if (pszSiteName)
    {
        dwError = LWNetAllocateString(
                      pszSiteName,
                      &pszSiteNameLower);
        BAIL_ON_LWNET_ERROR(dwError);

        LwStrToLower(pszSiteNameLower);
    }

    queryType = LWNetCacheDbQueryToQueryType(dwDsFlags);

    RW_LOCK_ACQUIRE_READ(DbHandle->pLock);
    isAcquired = TRUE;

    for (pListEntry = DbHandle->pCacheList ;
         pListEntry ;
         pListEntry = pListEntry->pNext)
    {
        pEntry = (PLWNET_CACHE_DB_ENTRY)pListEntry->pItem;

        if ( pEntry->QueryType == queryType &&
             !strcmp(pEntry->pszDnsDomainName, pszDnsDomainNameLower))
        {
            if (!pEntry->pszSiteName && !pszSiteNameLower)
            {
                break;
            }
            if (pEntry->pszSiteName && pszSiteNameLower &&
                !strcmp(pEntry->pszSiteName, pszSiteNameLower))
            {
                break;
            }
        }
    }

    if (!pListEntry)
    {
        LWNET_LOG_DEBUG("Cached entry not found: %s, %s, %u",
                        pszDnsDomainNameLower,
                        pszSiteNameLower ? pszSiteNameLower : "",
                        queryType);
        goto error;
    }

    dwError = LWNetAllocateMemory(sizeof(*pDcInfo), (PVOID*)&pDcInfo);
    BAIL_ON_LWNET_ERROR(dwError);

    lastDiscovered = pEntry->LastDiscovered;
    lastPinged = pEntry->LastPinged;
    isBackoffToWritableDc = pEntry->IsBackoffToWritableDc;
    lastBackoffToWritableDc = pEntry->LastBackoffToWritableDc;

    pDcInfo->dwPingTime = pEntry->DcInfo.dwPingTime;
    pDcInfo->dwDomainControllerAddressType = pEntry->DcInfo.dwDomainControllerAddressType;
    pDcInfo->dwFlags = pEntry->DcInfo.dwFlags;
    pDcInfo->dwVersion = pEntry->DcInfo.dwVersion;
    pDcInfo->wLMToken = pEntry->DcInfo.wLMToken;
    pDcInfo->wNTToken = pEntry->DcInfo.wNTToken;

    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszDomainControllerName,
                  &pDcInfo->pszDomainControllerName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszDomainControllerAddress,
                  &pDcInfo->pszDomainControllerAddress);
    BAIL_ON_LWNET_ERROR(dwError);

    memcpy(pDcInfo->pucDomainGUID,
           pEntry->DcInfo.pucDomainGUID,
           LWNET_GUID_SIZE);

    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszNetBIOSDomainName,
                  &pDcInfo->pszNetBIOSDomainName);
    BAIL_ON_LWNET_ERROR(dwError);
           
    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszFullyQualifiedDomainName,
                  &pDcInfo->pszFullyQualifiedDomainName);
    BAIL_ON_LWNET_ERROR(dwError);
           
    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszDnsForestName,
                  &pDcInfo->pszDnsForestName);
    BAIL_ON_LWNET_ERROR(dwError);
           
    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszDCSiteName,
                  &pDcInfo->pszDCSiteName);
    BAIL_ON_LWNET_ERROR(dwError);
           
    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszClientSiteName,
                  &pDcInfo->pszClientSiteName);
    BAIL_ON_LWNET_ERROR(dwError);
           
    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszNetBIOSHostName,
                  &pDcInfo->pszNetBIOSHostName);
    BAIL_ON_LWNET_ERROR(dwError);
           
    if (pEntry->DcInfo.pszUserName)
    {
        dwError = LWNetAllocateString(
                      pEntry->DcInfo.pszUserName,
                      &pDcInfo->pszUserName);
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:
    if (isAcquired)
    {
        RW_LOCK_RELEASE_READ(DbHandle->pLock);
    }
    if (dwError)
    {
        LWNET_SAFE_FREE_DC_INFO(pDcInfo);
        lastPinged = 0;
        lastDiscovered = 0;
        isBackoffToWritableDc = FALSE;
        lastBackoffToWritableDc = 0;
    }

    LWNET_SAFE_FREE_STRING(pszDnsDomainNameLower);
    LWNET_SAFE_FREE_STRING(pszSiteNameLower);

    *ppDcInfo = pDcInfo;
    *LastDiscovered = lastDiscovered;
    *LastPinged = lastPinged;
    *IsBackoffToWritableDc = isBackoffToWritableDc;
    *LastBackoffToWritableDc = lastBackoffToWritableDc;

    return dwError;
}

static
DWORD
LWNetCacheDbUpdate(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN LWNET_CACHE_DB_QUERY_TYPE QueryType,
    IN LWNET_UNIX_TIME_T LastDiscovered,
    IN LWNET_UNIX_TIME_T LastPinged,
    IN BOOLEAN IsBackoffToWritableDc,
    IN OPTIONAL LWNET_UNIX_TIME_T LastBackoffToWritableDc,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    DWORD dwError = 0;
    PLWNET_CACHE_DB_ENTRY pNewEntry = NULL;
    PLWNET_CACHE_DB_ENTRY pOldEntry = NULL;
    PDLINKEDLIST pOldListEntry = NULL;
    BOOLEAN isAcquired = FALSE;

    dwError = LWNetAllocateMemory(sizeof(*pNewEntry), (PVOID *)&pNewEntry);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pszDnsDomainName,
                  &pNewEntry->pszDnsDomainName);
    BAIL_ON_LWNET_ERROR(dwError);

    LwStrToLower(pNewEntry->pszDnsDomainName);

    if (pszSiteName)
    {
        dwError = LWNetAllocateString(
                      pszSiteName,
                      &pNewEntry->pszSiteName);
        BAIL_ON_LWNET_ERROR(dwError);

        LwStrToLower(pNewEntry->pszSiteName);
    }

    pNewEntry->QueryType = QueryType;

    pNewEntry->LastDiscovered = LastDiscovered;
    pNewEntry->LastPinged = LastPinged;
    pNewEntry->IsBackoffToWritableDc = IsBackoffToWritableDc;
    pNewEntry->LastBackoffToWritableDc = IsBackoffToWritableDc ? LastBackoffToWritableDc : 0;
    pNewEntry->DcInfo.dwPingTime = pDcInfo->dwPingTime;
    pNewEntry->DcInfo.dwDomainControllerAddressType = pDcInfo->dwDomainControllerAddressType;
    pNewEntry->DcInfo.dwFlags = pDcInfo->dwFlags;
    pNewEntry->DcInfo.dwVersion = pDcInfo->dwVersion;
    pNewEntry->DcInfo.wLMToken = pDcInfo->wLMToken;
    pNewEntry->DcInfo.wNTToken = pDcInfo->wNTToken;

    dwError = LWNetAllocateString(
                  pDcInfo->pszDomainControllerName,
                  &pNewEntry->DcInfo.pszDomainControllerName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pDcInfo->pszDomainControllerAddress,
                  &pNewEntry->DcInfo.pszDomainControllerAddress);
    BAIL_ON_LWNET_ERROR(dwError);

    memcpy(pNewEntry->DcInfo.pucDomainGUID,
           pDcInfo->pucDomainGUID,
           LWNET_GUID_SIZE);

    dwError = LWNetAllocateString(
                  pDcInfo->pszNetBIOSDomainName,
                  &pNewEntry->DcInfo.pszNetBIOSDomainName);
    BAIL_ON_LWNET_ERROR(dwError);
           
    dwError = LWNetAllocateString(
                  pDcInfo->pszFullyQualifiedDomainName,
                  &pNewEntry->DcInfo.pszFullyQualifiedDomainName);
    BAIL_ON_LWNET_ERROR(dwError);
           
    dwError = LWNetAllocateString(
                  pDcInfo->pszDnsForestName,
                  &pNewEntry->DcInfo.pszDnsForestName);
    BAIL_ON_LWNET_ERROR(dwError);
           
    dwError = LWNetAllocateString(
                  pDcInfo->pszDCSiteName,
                  &pNewEntry->DcInfo.pszDCSiteName);
    BAIL_ON_LWNET_ERROR(dwError);
           
    dwError = LWNetAllocateString(
                  pDcInfo->pszClientSiteName,
                  &pNewEntry->DcInfo.pszClientSiteName);
    BAIL_ON_LWNET_ERROR(dwError);
           
    dwError = LWNetAllocateString(
                  pDcInfo->pszNetBIOSHostName,
                  &pNewEntry->DcInfo.pszNetBIOSHostName);
    BAIL_ON_LWNET_ERROR(dwError);
           
    dwError = LWNetAllocateString(
	          pDcInfo->pszUserName ? pDcInfo->pszUserName : "",
		  &pNewEntry->DcInfo.pszUserName);
    BAIL_ON_LWNET_ERROR(dwError);

    RW_LOCK_ACQUIRE_WRITE(DbHandle->pLock);
    isAcquired = TRUE;

    for (pOldListEntry = DbHandle->pCacheList ;
         pOldListEntry ;
         pOldListEntry = pOldListEntry->pNext)
    {
        pOldEntry = (PLWNET_CACHE_DB_ENTRY)pOldListEntry->pItem;

        if (pOldEntry->QueryType == pNewEntry->QueryType &&
             !strcmp(pOldEntry->pszDnsDomainName, pNewEntry->pszDnsDomainName))
        {
            if (!pOldEntry->pszSiteName && !pNewEntry->pszSiteName)
            {
                break;
            }
            if (pOldEntry->pszSiteName && pNewEntry->pszSiteName &&
                !strcmp(pOldEntry->pszSiteName, pNewEntry->pszSiteName))
            {
                break;
            }
        }
    }

    if (pOldListEntry)
    {
        LWNetDLinkedListDelete(
            &DbHandle->pCacheList,
            pOldEntry);

        LWNetCacheDbEntryFree(pOldEntry);
    }

    dwError = LWNetDLinkedListAppend(
                  &DbHandle->pCacheList,
                  pNewEntry);
    BAIL_ON_LWNET_ERROR(dwError);

    DEBUG_ENTRY(pNewEntry);

    pNewEntry = NULL;

error:
    if (isAcquired)
    {
        RW_LOCK_RELEASE_WRITE(DbHandle->pLock);
    }

    LWNetCacheDbEntryFree(pNewEntry);

    return dwError;
}

DWORD
LWNetCacheDbScavenge(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN LWNET_UNIX_TIME_T PositiveCacheAge,
    IN LWNET_UNIX_TIME_T NegativeCacheAge
    )
{
    DWORD dwError = 0;
    LWNET_UNIX_TIME_T now = 0;
    LWNET_UNIX_TIME_T positiveTimeLimit = 0;
    PLWNET_CACHE_DB_ENTRY pEntry = NULL;
    PDLINKEDLIST pListEntry = NULL;
    PDLINKEDLIST pNextListEntry = NULL;
    BOOLEAN isAcquired = FALSE;

    dwError = LWNetGetSystemTime(&now);
    positiveTimeLimit = now + PositiveCacheAge;

    RW_LOCK_ACQUIRE_WRITE(DbHandle->pLock);
    isAcquired = TRUE;

    pListEntry = DbHandle->pCacheList;
    while(pListEntry)
    {
        pNextListEntry = pListEntry->pNext;
        pEntry = pListEntry->pItem;

        if (pEntry->LastPinged < positiveTimeLimit)
        {
            LWNetDLinkedListDelete(
                &DbHandle->pCacheList,
                pEntry);

            LWNetCacheDbEntryFree(pEntry);
        }

        pListEntry = pNextListEntry;
    }

    if (isAcquired)
    {
        RW_LOCK_RELEASE_WRITE(DbHandle->pLock);
    }

    return dwError;
}

DWORD
LWNetCacheDbExport(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    OUT PLWNET_CACHE_DB_ENTRY* ppEntries,
    OUT PDWORD pdwCount
    )
{
#if 1
    return ERROR_CALL_NOT_IMPLEMENTED;
#else
    DWORD dwError = 0;
    PLWNET_CACHE_DB_ENTRY pEntries = NULL;
    DWORD dwCount = 0;
    PDLINKEDLIST pListEntry = NULL;
    DWORD i = 0;

    RW_LOCK_ACQUIRE_READ(DbHandle->pLock);
    
    for (pListEntry = DbHandle->pCacheList;
         pListEntry;
         pListEntry = pListEntry->pNext)
    {
        dwCount++;
    }

    dwError = LWNetAllocateMemory(sizeof(*pEntries) * dwCount, OUT_PPVOID(&pEntries));
    BAIL_ON_LWNET_ERROR(dwError);

    i = 0;
    for (pListEntry = DbHandle->pCacheList;
         pListEntry;
         pListEntry = pListEntry->pNext)
    {
        PLWNET_CACHE_DB_ENTRY pEntry = (PLWNET_CACHE_DB_ENTRY) pListEntry->pItem;
        PLWNET_CACHE_DB_ENTRY pNewEntry = &pEntries[i];

        dwError = LWNetAllocateString(
                        pEntry->pszDnsDomainName,
                        &pNewEntry->pszDnsDomainName);
        BAIL_ON_LWNET_ERROR(dwError);

        if (pEntry->pszSiteName)
        {
            dwError = LWNetAllocateString(
                            pEntry->pszSiteName,
                            &pNewEntry->pszSiteName);
            BAIL_ON_LWNET_ERROR(dwError);
        }

        pNewEntry->QueryType = pEntry->QueryType;

        pNewEntry->LastDiscovered = pEntry->LastDiscovered;
        pNewEntry->LastPinged = pEntry->LastPinged;
        pNewEntry->IsBackoffToWritableDc = pEntry->IsBackoffToWritableDc;
        pNewEntry->LastBackoffToWritableDc = pEntry->LastBackoffToWritableDc;
        pNewEntry->DcInfo.dwPingTime = pEntry->DcInfo.dwPingTime;
        pNewEntry->DcInfo.dwDomainControllerAddressType = pEntry->DcInfo.dwDomainControllerAddressType;
        pNewEntry->DcInfo.dwFlags = pEntry->DcInfo.dwFlags;
        pNewEntry->DcInfo.dwVersion = pEntry->DcInfo.dwVersion;
        pNewEntry->DcInfo.wLMToken = pEntry->DcInfo.wLMToken;
        pNewEntry->DcInfo.wNTToken = pEntry->DcInfo.wNTToken;

        dwError = LWNetAllocateString(
                        pEntry->DcInfo.pszDomainControllerName,
                        &pNewEntry->DcInfo.pszDomainControllerName);
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetAllocateString(
                        pEntry->DcInfo.pszDomainControllerAddress,
                        &pNewEntry->DcInfo.pszDomainControllerAddress);
        BAIL_ON_LWNET_ERROR(dwError);

        memcpy(pNewEntry->DcInfo.pucDomainGUID,
               pEntry->DcInfo.pucDomainGUID,
               LWNET_GUID_SIZE);

        dwError = LWNetAllocateString(
                        pEntry->DcInfo.pszNetBIOSDomainName,
                        &pNewEntry->DcInfo.pszNetBIOSDomainName);
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetAllocateString(
                        pEntry->DcInfo.pszFullyQualifiedDomainName,
                        &pNewEntry->DcInfo.pszFullyQualifiedDomainName);
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetAllocateString(
                        pEntry->DcInfo.pszDnsForestName,
                        &pNewEntry->DcInfo.pszDnsForestName);
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetAllocateString(
                        pEntry->DcInfo.pszDCSiteName,
                        &pNewEntry->DcInfo.pszDCSiteName);
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetAllocateString(
                        pEntry->DcInfo.pszClientSiteName,
                        &pNewEntry->DcInfo.pszClientSiteName);
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetAllocateString(
                        pEntry->DcInfo.pszNetBIOSHostName,
                        &pNewEntry->DcInfo.pszNetBIOSHostName);
        BAIL_ON_LWNET_ERROR(dwError);

        if (pEntry->DcInfo.pszUserName)
        {
            dwError = LWNetAllocateString(
                            pEntry->DcInfo.pszUserName,
                            &pNewEntry->DcInfo.pszUserName);
            BAIL_ON_LWNET_ERROR(dwError);
        }

        i++;
    }

cleanup:
    RW_LOCK_RELEASE_READ(DbHandle->pLock);

    *ppEntries = pEntries;
    *pdwCount = dwCount;

    return dwError;

error:
    // TODO: Add free function.
    pEntries = NULL;
    dwCount = 0;
    goto cleanup;
#endif
}

DWORD
LWNetCacheInitialize(
    )
{
    DWORD dwError = 0;

    dwError = LWNetCacheDbOpen(NETLOGON_DB, TRUE, &gDbHandle);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    if (dwError)
    {
        LWNetCacheCleanup();
    }
    return dwError;
}

VOID
LWNetCacheCleanup(
    )
{
    LWNetCacheDbClose(&gDbHandle);
}

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
    )
{
    return LWNetCacheDbQuery(gDbHandle,
                             pszDnsDomainName, pszSiteName, dwDsFlags,
                             ppDcInfo, LastDiscovered, LastPinged,
                             IsBackoffToWritableDc, LastBackoffToWritableDc);
}

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
    )
{
    DWORD dwError = 0;
    LWNET_CACHE_DB_QUERY_TYPE QueryType = LWNetCacheDbQueryToQueryType(dwDsFlags);

    dwError = LWNetCacheDbUpdate(
                    gDbHandle,
                    pszDnsDomainName,
                    pszSiteName,
                    QueryType,
                    LastDiscovered,
                    LastPinged,
                    IsBackoffToWritableDc,
                    LastBackoffToWritableDc,
                    pDcInfo);
    BAIL_ON_LWNET_ERROR(dwError);

    if (pDcInfo->pszDCSiteName &&
        (!pszSiteName || strcasecmp(pszSiteName, pDcInfo->pszDCSiteName)))
    {
        dwError = LWNetCacheDbUpdate(
                        gDbHandle,
                        pszDnsDomainName,
                        pDcInfo->pszDCSiteName,
                        QueryType,
                        LastDiscovered,
                        LastPinged,
                        IsBackoffToWritableDc,
                        LastBackoffToWritableDc,
                        pDcInfo);
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:
    return dwError;
}

DWORD
LWNetCacheScavenge(
    IN LWNET_UNIX_TIME_T PositiveCacheAge,
    IN LWNET_UNIX_TIME_T NegativeCacheAge
    )
{
    return LWNetCacheDbScavenge(gDbHandle, PositiveCacheAge, NegativeCacheAge);
}
