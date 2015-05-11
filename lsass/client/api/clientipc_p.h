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
 *        clientipc_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Private Header (Library)
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __CLIENTIPC_P_H__
#define __CLIENTIPC_P_H__

#include <lwmsg/lwmsg.h>

typedef struct __LSA_CLIENT_CONNECTION_CONTEXT
{
    LWMsgProtocol* pProtocol;
    LWMsgAssoc* pAssoc;
    LWMsgSession* pSession;
} LSA_CLIENT_CONNECTION_CONTEXT, *PLSA_CLIENT_CONNECTION_CONTEXT;

DWORD
LsaIpcAcquireCall(
    HANDLE hServer,
    LWMsgCall** ppCall
    );

DWORD
LsaIpcUnregisterHandle(
    LWMsgCall* pCall,
    PVOID pHandle
    );
    
DWORD
LsaTransactAddGroup2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_GROUP_ADD_INFO pGroupAddInfo
    );

DWORD
LsaTransactDeleteObject(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PCSTR pszSid
    );

DWORD
LsaTransactAddUser2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_USER_ADD_INFO pUserAddInfo2
    );

DWORD
LsaTransactAuthenticateUserPam(
    HANDLE hServer,
    LSA_AUTH_USER_PAM_PARAMS* pParams,
    PLSA_AUTH_USER_PAM_INFO* ppPamAuthInfo
    );

DWORD
LsaTransactAuthenticateUserEx(
    IN HANDLE hServer,
    IN PCSTR pszTargetProvider,
    IN LSA_AUTH_USER_PARAMS* pParams,
    OUT PLSA_AUTH_USER_INFO* ppUserInfo
    );

DWORD
LsaTransactValidateUser(
    HANDLE hServer,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    );

DWORD
LsaTransactChangePassword(
    HANDLE hServer,
    PCSTR  pszLoginName,
    PCSTR  pszNewPassword,
    PCSTR  pszOldPassword
    );

DWORD
LsaTransactSetPassword(
    HANDLE hServer,
    PCSTR  pszLoginName,
    PCSTR  pszNewPassword
    );

DWORD
LsaTransactModifyUser2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_USER_MOD_INFO_2 pUserModInfo
    );

DWORD
LsaTransactModifyGroup2(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo
    );

DWORD
LsaTransactProviderIoControl(
    IN HANDLE  hServer,
    IN PCSTR   pszProvider,
    IN DWORD   dwIoControlCode,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    );

DWORD
LsaTransactFindObjects(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
LsaTransactOpenEnumObjects(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    );

DWORD
LsaTransactEnumObjects(
    IN HANDLE hLsa,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
LsaTransactOpenEnumMembers(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    );

DWORD
LsaTransactEnumMembers(
    IN HANDLE hLsa,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PSTR** pppszMember
    );

DWORD
LsaTransactQueryMemberOf(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    );

LW_DWORD
LsaTransactFindGroupAndExpandedMembers(
    LW_IN LW_HANDLE hLsa,
    LW_PCSTR pszTargetProvider,
    LW_IN LSA_FIND_FLAGS FindFlags,
    LW_IN LSA_QUERY_TYPE QueryType,
    LW_IN LSA_QUERY_ITEM QueryItem,
    LW_OUT PLSA_SECURITY_OBJECT* pGroupObject,
    LW_OUT LW_PDWORD pdwMemberObjectCount,
    LW_OUT PLSA_SECURITY_OBJECT** pppMemberObjects
    );

DWORD
LsaTransactCloseEnum(
    IN HANDLE hLsa,
    IN OUT HANDLE hEnum
    );

DWORD
LsaTransactGetSmartCardUserObject(
    HANDLE hServer,
    PLSA_SECURITY_OBJECT* ppObject,
    PSTR* ppszSmartCardReader
    );

DWORD
LsaTransactGetStatus(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    OUT PLSASTATUS* ppLsaStatus
    );

#endif /* __CLIENTIPC_P_H__ */

