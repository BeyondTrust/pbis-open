/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lwnet_p.h
 *
 * Abstract:
 *
 *        BeyondTrust Netlogon
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

// CLDAP searches of individual domain controllers are
// started in batches of 1/5 the maximum number of
// connections or the minimum number set here.  The
// short poll timeout controls how long to wait between
// batches.  This prevents the entire set of connections
// from being started in cases where there is a fast
// response.
#define LWNET_CLDAP_SHORT_POLL_TIMEOUT_MILLISECONDS 100
#define LWNET_CLDAP_MINIMUM_INCREMENTAL_CONNECTIONS 20

// These settings can be overridden by configuration.
// This timeout is the default for both the entire
// search and individual domain controller searches.
// The maximum number of connections controls how
// many domain controllers can be searched 
// simultaneously.
#define LWNET_CLDAP_DEFAULT_TIMEOUT_SECONDS 15
#define LWNET_CLDAP_DEFAULT_MAXIMUM_CONNECTIONS 100

typedef struct _LWNET_CLDAP_CONNECTION_CONTEXT {
    HANDLE hDirectory;
    int fd;
    int msgid;
    LWNET_UNIX_MS_TIME_T StartTime;
    PDNS_SERVER_INFO pServerInfo;
} LWNET_CLDAP_CONNECTION_CONTEXT, *PLWNET_CLDAP_CONNECTION_CONTEXT;

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

VOID
LWNetStopNetBios(
    );

DWORD
LWNetGetPreferredDcList(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PDNS_SERVER_INFO* ppServerArray,
    OUT PDWORD pdwServerCount
    );

BOOLEAN
LWNetIsUpdateKrb5AffinityEnabled(
    IN DWORD DsFlags,
    IN PCSTR SiteName,
    IN PLWNET_DC_INFO pDcInfo
    );

#endif /* __LWNET_P_H__ */
