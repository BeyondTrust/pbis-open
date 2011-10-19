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
 *        lwnet_p.h
 *
 * Abstract:
 *
 *        Likewise Netlogon
 * 
 *        Active Directory Site API (Private Header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#ifndef __LWNET_P_H__
#define __LWNET_P_H__

typedef struct _PACKED_ARRAY
{
    BYTE *pStart;
    BYTE *pCur;
    size_t totalSize;
} PACKED_ARRAY;

#define BYTES_REMAINING(a) (((a).totalSize) - ((unsigned long)((a).pCur) - (unsigned long)((a).pStart)))

#define NETLOGON_LDAP_ATTRIBUTE_NAME "Netlogon"

#define LWNET_CLDAP_DEFAULT_THREAD_COUNT    20
#define LWNET_CLDAP_DEFAULT_TIMEOUT_SECONDS 15

// Invariant: Threads can set pDcInfo and read from pServerArray
//            IFF (dwServerIndex < dwServerCount).
typedef struct _LWNET_CLDAP_THREAD_CONTEXT {
    // Result:
    PLWNET_DC_INFO pDcInfo; // mutable
    // Queue of work items:
    PSTR pszDnsDomainName;
    PDNS_SERVER_INFO pServerArray;
    DWORD dwServerCount;
    DWORD dwServerIndex; // mutable
    DWORD dwDsFlags;
    BOOLEAN bIsDone; // mutable
    BOOLEAN bFailedFindWritable; // mutable
    // Synchronization
    pthread_mutex_t Mutex;
    pthread_cond_t Condition;
    pthread_mutex_t* pMutex;
    pthread_cond_t* pCondition;
    // Threads currently active -- used to figure out
    // last thread so it can signal
    DWORD dwActiveThreadCount; // mutable
    // RefCount
    DWORD dwRefCount; // mutable
} LWNET_CLDAP_THREAD_CONTEXT, *PLWNET_CLDAP_THREAD_CONTEXT;

// The "if" is to simplify code in codepath where the context
// is not fully initialized.
#define LWNET_CLDAP_THREAD_CONTEXT_ACQUIRE(pContext) \
    do { \
        if ((pContext)->pMutex) \
        { \
            pthread_mutex_lock((pContext)->pMutex); \
        } \
    } while (0)
            
#define LWNET_CLDAP_THREAD_CONTEXT_RELEASE(pContext) \
    do { \
        if ((pContext)->pMutex) \
        { \
            pthread_mutex_unlock((pContext)->pMutex); \
        } \
    } while (0)

BOOLEAN
LWNetSrvIsMatchingDcInfo(
    IN PLWNET_DC_INFO pDcInfo,
    IN DWORD dwDsFlags
    );

BOOLEAN
LWNetSrvIsAffinitizableRequestFlags(
    IN DWORD dwDsFlags
    );

DWORD
LWNetSrvPingCLdapArray(
    IN PCSTR pszDnsDomainName,
    IN DWORD dwDsFlags,
    IN PDNS_SERVER_INFO pServerArray,
    IN DWORD dwServerCount,
    IN OPTIONAL DWORD dwThreadCount,
    IN OPTIONAL DWORD dwTimeoutSeconds,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT PBOOLEAN pbFailedFindWritable
    );

VOID
LWNetFilterFromBlackList(
    IN DWORD dwBlackListCount,
    IN PSTR* ppszAddressBlackList,
    IN OUT PDWORD pdwServerCount,
    IN OUT PDNS_SERVER_INFO pServerArray
    );

DWORD
LWNetSrvGetDCNameDiscover(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN OPTIONAL PCSTR pszPrimaryDomain,
    IN DWORD dwDsFlags,
    IN DWORD dwBlackListCount,
    IN PSTR* ppszAddressBlackList,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT OPTIONAL PDNS_SERVER_INFO* ppServerArray,
    OUT OPTIONAL PDWORD pdwServerCount,
    OUT PBOOLEAN pbFailedFindWritable
    );

DWORD
LWNetBuildDCInfo(
    IN PBYTE pBuffer,
    IN DWORD dwBufferSize,
    OUT PLWNET_DC_INFO* ppDCInfo
    );

DWORD
LWNetReadLEDword(
    OUT PDWORD pdwDest,
    IN PACKED_ARRAY* pArray
    );

DWORD
LWNetReadLEWord(
    OUT PWORD pwDest,
    IN PACKED_ARRAY* pArray
    );

DWORD
LWNetReadGUID(
    OUT PBYTE pbtDest,
    IN PACKED_ARRAY* pArray
    );

DWORD
LWNetReadString(
    OUT PSTR *ppszDest,
    IN PACKED_ARRAY *pArray
    );

DWORD
LWNetInitializePlugin(
    IN PCSTR pszPath
    );

VOID
LWNetCleanupPlugin(
    );

DWORD
LWNetGetPreferredDcList(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PDNS_SERVER_INFO* ppServerArray,
    OUT PDWORD pdwServerCount
    );

#endif /* __LWNET_P_H__ */
