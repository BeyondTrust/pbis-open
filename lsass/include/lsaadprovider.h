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
 *        lsaadprovider.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Client API
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
