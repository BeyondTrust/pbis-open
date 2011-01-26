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
 *        ipc_protocol.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process communication API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#include "ipc.h"

static LWMsgTypeSpec gLsaAdIPCStringSpec[] =
{
    LWMSG_PSTR,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaAdIPCEnumUsersFromCacheReqSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ),
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ, pszResume),
    LWMSG_MEMBER_UINT32(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ, dwMaxNumUsers),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaAdIPCEnumUsersFromCacheRespSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP),
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP, pszResume),
    LWMSG_MEMBER_UINT32(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP, dwNumUsers),
    LWMSG_MEMBER_POINTER_BEGIN(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP, ppObjects),
    LWMSG_POINTER(LWMSG_TYPESPEC(gLsaSecurityObjectSpec)),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_POINTER_END,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_LENGTH_MEMBER(LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP, dwNumUsers),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaAdIPCEnumGroupsFromCacheReqSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ),
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ, pszResume),
    LWMSG_MEMBER_UINT32(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ, dwMaxNumGroups),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaAdIPCEnumGroupsFromCacheRespSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP),
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP, pszResume),
    LWMSG_MEMBER_UINT32(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP, dwNumGroups),
    LWMSG_MEMBER_POINTER_BEGIN(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP, ppObjects),
    LWMSG_POINTER(LWMSG_TYPESPEC(gLsaSecurityObjectSpec)),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_POINTER_END,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_LENGTH_MEMBER(LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP, dwNumGroups),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaAdIPCJoinDomainReqSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_AD_IPC_JOIN_DOMAIN_REQ),
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_JOIN_DOMAIN_REQ, pszHostname), LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_JOIN_DOMAIN_REQ, pszHostDnsDomain), LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_JOIN_DOMAIN_REQ, pszDomain), LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_JOIN_DOMAIN_REQ, pszOU),
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_JOIN_DOMAIN_REQ, pszUsername), LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_JOIN_DOMAIN_REQ, pszPassword), LWMSG_ATTR_NOT_NULL, LWMSG_ATTR_SENSITIVE,
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_JOIN_DOMAIN_REQ, pszOSName), LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_JOIN_DOMAIN_REQ, pszOSVersion), LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_JOIN_DOMAIN_REQ, pszOSServicePack), LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_UINT32(LSA_AD_IPC_JOIN_DOMAIN_REQ, dwFlags),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaAdIPCLeaveDomainReqSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_AD_IPC_LEAVE_DOMAIN_REQ),
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_LEAVE_DOMAIN_REQ, pszUsername),
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_LEAVE_DOMAIN_REQ, pszPassword), LWMSG_ATTR_SENSITIVE,
    LWMSG_MEMBER_PSTR(LSA_AD_IPC_LEAVE_DOMAIN_REQ, pszDomain),
    LWMSG_MEMBER_UINT32(LSA_AD_IPC_LEAVE_DOMAIN_REQ, dwFlags),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaAdIPCGetJoinedDomainsRespSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_AD_IPC_GET_JOINED_DOMAINS_RESP),
    LWMSG_MEMBER_UINT32(LSA_AD_IPC_GET_JOINED_DOMAINS_RESP, dwObjectsCount),
    LWMSG_MEMBER_POINTER_BEGIN(LSA_AD_IPC_GET_JOINED_DOMAINS_RESP, ppszDomains),
    LWMSG_PSTR,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_POINTER_END,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_LENGTH_MEMBER(LSA_AD_IPC_GET_JOINED_DOMAINS_RESP, dwObjectsCount),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaAdIPCGetMachineAccountInfoSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_MACHINE_ACCOUNT_INFO_A),
    LWMSG_MEMBER_PSTR(LSA_MACHINE_ACCOUNT_INFO_A, DnsDomainName),
    LWMSG_MEMBER_PSTR(LSA_MACHINE_ACCOUNT_INFO_A, NetbiosDomainName),
    LWMSG_MEMBER_PSTR(LSA_MACHINE_ACCOUNT_INFO_A, DomainSid),
    LWMSG_MEMBER_PSTR(LSA_MACHINE_ACCOUNT_INFO_A, SamAccountName),
    LWMSG_MEMBER_UINT32(LSA_MACHINE_ACCOUNT_INFO_A, AccountFlags),
    LWMSG_MEMBER_UINT32(LSA_MACHINE_ACCOUNT_INFO_A, KeyVersionNumber),
    LWMSG_MEMBER_PSTR(LSA_MACHINE_ACCOUNT_INFO_A, Fqdn),
    LWMSG_MEMBER_INT64(LSA_MACHINE_ACCOUNT_INFO_A, LastChangeTime),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaAdIPCGetMachinePasswordInfoSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_MACHINE_PASSWORD_INFO_A),
    LWMSG_MEMBER_TYPESPEC(LSA_MACHINE_PASSWORD_INFO_A, Account, gLsaAdIPCGetMachineAccountInfoSpec),
    LWMSG_MEMBER_PSTR(LSA_MACHINE_PASSWORD_INFO_A, Password),
    LWMSG_ATTR_SENSITIVE,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec*
LsaAdIPCGetStringSpec(
    VOID
    )
{
    return gLsaAdIPCStringSpec;
}

LWMsgTypeSpec*
LsaAdIPCGetEnumUsersFromCacheReqSpec(
    void
    )
{
    return gLsaAdIPCEnumUsersFromCacheReqSpec;
}

LWMsgTypeSpec*
LsaAdIPCGetEnumUsersFromCacheRespSpec(
    void
    )
{
    return gLsaAdIPCEnumUsersFromCacheRespSpec;
}

LWMsgTypeSpec*
LsaAdIPCGetEnumGroupsFromCacheReqSpec(
    void
    )
{
    return gLsaAdIPCEnumGroupsFromCacheReqSpec;
}

LWMsgTypeSpec*
LsaAdIPCGetEnumGroupsFromCacheRespSpec(
    void
    )
{
    return gLsaAdIPCEnumGroupsFromCacheRespSpec;
}

LWMsgTypeSpec*
LsaAdIPCGetJoinDomainReqSpec(
    void
    )
{
    return gLsaAdIPCJoinDomainReqSpec;
}

LWMsgTypeSpec*
LsaAdIPCGetLeaveDomainReqSpec(
    void
    )
{
    return gLsaAdIPCLeaveDomainReqSpec;
}

LWMsgTypeSpec*
LsaAdIPCGetJoinedDomainsRespSpec(
    void
    )
{
    return gLsaAdIPCGetJoinedDomainsRespSpec;
}

LWMsgTypeSpec*
LsaAdIPCGetMachineAccountInfoSpec(
    VOID
    )
{
    return gLsaAdIPCGetMachineAccountInfoSpec;
}

LWMsgTypeSpec*
LsaAdIPCGetMachinePasswordInfoSpec(
    VOID
    )
{
    return gLsaAdIPCGetMachinePasswordInfoSpec;
}

static
LWMsgStatus
LsaAdIPCAllocate(
    IN size_t Size,
    OUT PVOID* ppMemory,
    IN PVOID pContext
    )
{
    DWORD dwError = LwAllocateMemory((DWORD)Size, ppMemory);
    return dwError ? LWMSG_STATUS_MEMORY : LWMSG_STATUS_SUCCESS;
}

static
VOID
LsaAdIPCFree(
    IN PVOID pMemory,
    IN PVOID pContext
    )
{
    LwFreeMemory(pMemory);
}

VOID
LsaAdIPCSetMemoryFunctions(
    IN LWMsgContext* pContext
    )
{
    lwmsg_context_set_memory_functions(pContext, LsaAdIPCAllocate, LsaAdIPCFree, NULL, NULL);
}
