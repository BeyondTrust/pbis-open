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
 *        lsaadprovider.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS) Client API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __LSAADPROVIDER_H__
#define __LSAADPROVIDER_H__

#include "lsautils.h"
#include <lsa/ad-types.h>

#define LSA_AD_IO_EMPTYCACHE             1
#define LSA_AD_IO_REMOVEUSERBYNAMECACHE  2
#define LSA_AD_IO_REMOVEUSERBYIDCACHE    3
#define LSA_AD_IO_REMOVEGROUPBYNAMECACHE 4
#define LSA_AD_IO_REMOVEGROUPBYIDCACHE   5
#define LSA_AD_IO_ENUMUSERSCACHE         6
#define LSA_AD_IO_ENUMGROUPSCACHE        7
#define LSA_AD_IO_JOINDOMAIN             8
#define LSA_AD_IO_LEAVEDOMAIN            9
#define LSA_AD_IO_SETDEFAULTDOMAIN      10
#define LSA_AD_IO_GETJOINEDDOMAINS      11
#define LSA_AD_IO_GET_MACHINE_ACCOUNT   12
#define LSA_AD_IO_GET_MACHINE_PASSWORD  13
#define LSA_AD_IO_GET_COMPUTER_DN       14


typedef struct __LSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ {
    PCSTR pszResume;
    DWORD dwMaxNumUsers;
} LSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ, *PLSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ;

typedef struct __LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP {
    PSTR pszResume;
    DWORD dwNumUsers;
    PLSA_SECURITY_OBJECT* ppObjects;
} LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP, *PLSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP;

typedef struct __LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ {
    PCSTR pszResume;
    DWORD dwMaxNumGroups;
} LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ, *PLSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ;

typedef struct __LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP {
    PSTR pszResume;
    DWORD dwNumGroups;
    PLSA_SECURITY_OBJECT* ppObjects;
} LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP, *PLSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP;

typedef struct __LSA_AD_IPC_JOIN_DOMAIN_REQ
{
    PCSTR pszHostname;
    PCSTR pszHostDnsDomain;
    PCSTR pszDomain;
    PCSTR pszOU;
    PCSTR pszUsername;
    PCSTR pszPassword;
    PCSTR pszOSName;
    PCSTR pszOSVersion;
    PCSTR pszOSServicePack;
    LSA_NET_JOIN_FLAGS dwFlags;
    LSA_USER_ACCOUNT_CONTROL_FLAGS dwUac;
} LSA_AD_IPC_JOIN_DOMAIN_REQ, *PLSA_AD_IPC_JOIN_DOMAIN_REQ;

typedef struct __LSA_AD_IPC_LEAVE_DOMAIN_REQ
{
    PCSTR pszUsername;
    PCSTR pszPassword;
    PCSTR pszDomain;
    LSA_NET_JOIN_FLAGS dwFlags;
} LSA_AD_IPC_LEAVE_DOMAIN_REQ, *PLSA_AD_IPC_LEAVE_DOMAIN_REQ;

typedef struct __LSA_AD_IPC_GET_JOINED_DOMAINS_RESP
{
    DWORD dwObjectsCount;
    PSTR* ppszDomains;
} LSA_AD_IPC_GET_JOINED_DOMAINS_RESP, *PLSA_AD_IPC_GET_JOINED_DOMAINS_RESP;

LWMsgTypeSpec*
LsaAdIPCGetStringSpec(
    VOID
    );

LWMsgTypeSpec*
LsaAdIPCGetEnumUsersFromCacheReqSpec(
    void
    );

LWMsgTypeSpec*
LsaAdIPCGetEnumUsersFromCacheRespSpec(
    void
    );

LWMsgTypeSpec*
LsaAdIPCGetEnumGroupsFromCacheReqSpec(
    void
    );

LWMsgTypeSpec*
LsaAdIPCGetEnumGroupsFromCacheRespSpec(
    void
    );

LWMsgTypeSpec*
LsaAdIPCGetJoinDomainReqSpec(
    void
    );

LWMsgTypeSpec*
LsaAdIPCGetLeaveDomainReqSpec(
    void
    );

LWMsgTypeSpec*
LsaAdIPCGetJoinedDomainsRespSpec(
    void
    );

LWMsgTypeSpec*
LsaAdIPCGetMachineAccountInfoSpec(
    VOID
    );

LWMsgTypeSpec*
LsaAdIPCGetMachinePasswordInfoSpec(
    VOID
    );

VOID
LsaAdIPCSetMemoryFunctions(
    IN LWMsgContext* pContext
    );

#endif /* __LSAADPROVIDER_H__ */
