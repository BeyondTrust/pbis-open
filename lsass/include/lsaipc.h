/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lsaipc.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS) Interprocess Communication
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __LSAIPC_H__
#define __LSAIPC_H__

#include <lwmsg/lwmsg.h>
#include <lsa/lsa.h>

#define LSA_SERVER_FILENAME    ".lsassd"

/* Opaque type -- actual definition in state_p.h - LSA_SRV_ENUM_STATE */

typedef struct __LSA_IPC_ERROR
{
    DWORD dwError;
    PSTR pszErrorMessage;
} LSA_IPC_ERROR, *PLSA_IPC_ERROR;

typedef struct __LSA_IPC_FIND_NSSARTEFACT_BY_KEY_REQ
{
    LSA_NIS_MAP_QUERY_FLAGS dwFlags;
    DWORD dwInfoLevel;
    PCSTR pszKeyName;
    PCSTR pszMapName;
} LSA_IPC_FIND_NSSARTEFACT_BY_KEY_REQ, *PLSA_IPC_FIND_NSSARTEFACT_BY_KEY_REQ;

typedef struct __LSA_IPC_AUTH_USER_REQ
{
    PCSTR pszLoginName;
    PCSTR pszPassword;
} LSA_IPC_AUTH_USER_REQ, *PLSA_IPC_AUTH_USER_REQ;

typedef struct __LSA_IPC_CHANGE_PASSWORD_REQ
{
    PCSTR pszLoginName;
    PCSTR pszNewPassword;
    PCSTR pszOldPassword;
} LSA_IPC_CHANGE_PASSWORD_REQ, *PLSA_IPC_CHANGE_PASSWORD_REQ;

typedef struct __LSA_IPC_SET_PASSWORD_REQ
{
    PCSTR pszLoginName;
    PCSTR pszNewPassword;
} LSA_IPC_SET_PASSWORD_REQ, *PLSA_IPC_SET_PASSWORD_REQ;

typedef struct __LSA_IPC_CHECK_USER_IN_LIST_REQ
{
    PCSTR pszLoginName;
    PCSTR pszListName;
} LSA_IPC_CHECK_USER_IN_LIST_REQ, *PLSA_IPC_CHECK_USER_IN_LIST_REQ;

typedef struct __LSA_IPC_SET_TRACE_INFO_REQ
{
    PLSA_TRACE_INFO pTraceFlagArray;
    DWORD dwNumFlags;
} LSA_IPC_SET_TRACE_INFO_REQ, *PLSA_IPC_SET_TRACE_INFO_REQ;

typedef struct __LSA_IPC_BEGIN_ENUM_NSSARTEFACT_REQ
{
    DWORD dwInfoLevel;
    DWORD dwMaxNumNSSArtefacts;
    DWORD dwFlags;
    PCSTR pszMapName;
} LSA_IPC_BEGIN_ENUM_NSSARTEFACT_REQ, *PLSA_IPC_BEGIN_ENUM_NSSARTEFACT_REQ;

typedef struct __LSA_IPC_PROVIDER_IO_CONTROL_REQ {
    PCSTR pszProvider;
    DWORD dwIoControlCode;
    DWORD dwDataLen;
    PBYTE pData;
} LSA_IPC_PROVIDER_IO_CONTROL_REQ, *PLSA_IPC_PROVIDER_IO_CONTROL_REQ;

typedef enum _LSA2_IPC_QUERY_TYPE
{
    LSA2_IPC_QUERY_STRINGS,
    LSA2_IPC_QUERY_DWORDS
} LSA2_IPC_QUERY_TYPE;

typedef struct _LSA2_IPC_FIND_OBJECTS_REQ
{
    PCSTR pszTargetProvider;
    LSA_FIND_FLAGS FindFlags;
    LSA_OBJECT_TYPE ObjectType;
    LSA_QUERY_TYPE QueryType;
    LSA2_IPC_QUERY_TYPE IpcQueryType;
    DWORD dwCount;
    LSA_QUERY_LIST QueryList;
} LSA2_IPC_FIND_OBJECTS_REQ, *PLSA2_IPC_FIND_OBJECTS_REQ;

typedef struct _LSA2_IPC_FIND_OBJECTS_RES
{
    DWORD dwCount;
    PLSA_SECURITY_OBJECT* ppObjects;
} LSA2_IPC_FIND_OBJECTS_RES, *PLSA2_IPC_FIND_OBJECTS_RES;

typedef struct _LSA2_IPC_OPEN_ENUM_OBJECTS_REQ
{
    PCSTR pszTargetProvider;
    LSA_FIND_FLAGS FindFlags;
    LSA_OBJECT_TYPE ObjectType;
    PCSTR pszDomainName;
} LSA2_IPC_OPEN_ENUM_OBJECTS_REQ, *PLSA2_IPC_OPEN_ENUM_OBJECTS_REQ;

typedef struct _LSA2_IPC_ENUM_OBJECTS_REQ
{
    HANDLE hEnum;
    DWORD dwMaxObjectsCount;
} LSA2_IPC_ENUM_OBJECTS_REQ, *PLSA2_IPC_ENUM_OBJECTS_REQ;

typedef struct _LSA2_IPC_ENUM_OBJECTS_RES
{
    DWORD dwObjectsCount;
    PLSA_SECURITY_OBJECT* ppObjects;
} LSA2_IPC_ENUM_OBJECTS_RES, *PLSA2_IPC_ENUM_OBJECTS_RES;

typedef struct _LSA2_IPC_OPEN_ENUM_MEMBERS_REQ
{
    PCSTR pszTargetProvider;
    LSA_FIND_FLAGS FindFlags;
    PCSTR pszSid;
} LSA2_IPC_OPEN_ENUM_MEMBERS_REQ, *PLSA2_IPC_OPEN_ENUM_MEMBERS_REQ;

typedef struct _LSA2_IPC_ENUM_MEMBERS_REQ
{
    HANDLE hEnum;
    DWORD dwMaxSidCount;
} LSA2_IPC_ENUM_MEMBERS_REQ, *PLSA2_IPC_ENUM_MEMBERS_REQ;

typedef struct _LSA2_IPC_ENUM_MEMBERS_RES
{
    DWORD dwSidCount;
    PSTR* ppszMemberSids;
} LSA2_IPC_ENUM_MEMBERS_RES, *PLSA2_IPC_ENUM_MEMBERS_RES;

typedef struct _LSA2_IPC_QUERY_MEMBER_OF_REQ
{
    PCSTR pszTargetProvider;
    LSA_FIND_FLAGS FindFlags;
    DWORD dwSidCount;
    PSTR* ppszSids;
} LSA2_IPC_QUERY_MEMBER_OF_REQ, *PLSA2_IPC_QUERY_MEMBER_OF_REQ;

typedef struct _LSA2_IPC_QUERY_MEMBER_OF_RES
{
    DWORD dwGroupSidCount;
    PSTR* ppszGroupSids;
} LSA2_IPC_QUERY_MEMBER_OF_RES, *PLSA2_IPC_QUERY_MEMBER_OF_RES;

typedef struct _LSA2_IPC_MODIFY_USER_REQ
{
    PCSTR pszTargetProvider;
    PLSA_USER_MOD_INFO_2 pUserModInfo;
} LSA2_IPC_MODIFY_USER_REQ, *PLSA2_IPC_MODIFY_USER_REQ;

typedef struct _LSA2_IPC_MODIFY_GROUP_REQ
{
    PCSTR pszTargetProvider;
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo;
} LSA2_IPC_MODIFY_GROUP_REQ, *PLSA2_IPC_MODIFY_GROUP_REQ;

typedef struct _LSA2_IPC_ADD_USER_REQ
{
    PCSTR pszTargetProvider;
    PLSA_USER_ADD_INFO pUserAddInfo;
} LSA2_IPC_ADD_USER_REQ, *PLSA2_IPC_ADD_USER_REQ;

typedef struct _LSA2_IPC_ADD_GROUP_REQ
{
    PCSTR pszTargetProvider;
    PLSA_GROUP_ADD_INFO pGroupAddInfo;
} LSA2_IPC_ADD_GROUP_REQ, *PLSA2_IPC_ADD_GROUP_REQ;

typedef struct _LSA2_IPC_DELETE_OBJECT_REQ
{
    PCSTR pszTargetProvider;
    PCSTR pszSid;
} LSA2_IPC_DELETE_OBJECT_REQ, *PLSA2_IPC_DELETE_OBJECT_REQ;

typedef struct _LSA2_IPC_FIND_GROUP_AND_EXPANDED_MEMBERS_REQ
{
    PCSTR pszTargetProvider;
    LSA_FIND_FLAGS FindFlags;
    LSA_QUERY_TYPE QueryType;
    LSA2_IPC_QUERY_TYPE IpcQueryType;
    LSA_QUERY_ITEM QueryItem;
} LSA2_IPC_FIND_GROUP_AND_EXPANDED_MEMBERS_REQ, *PLSA2_IPC_FIND_GROUP_AND_EXPANDED_MEMBERS_REQ;

typedef struct _LSA2_IPC_FIND_GROUP_AND_EXPANDED_MEMBERS_RES
{
    PLSA_SECURITY_OBJECT pGroup;
    DWORD dwMemberObjectCount;
    PLSA_SECURITY_OBJECT* ppMemberObjects;
} LSA2_IPC_FIND_GROUP_AND_EXPANDED_MEMBERS_RES, *PLSA2_IPC_FIND_GROUP_AND_EXPANDED_MEMBERS_RES;

typedef struct _LSA2_IPC_GET_SMART_CARD_USER_REQ
{
    PCSTR pszSmartcardUser;
} LSA2_IPC_GET_SMART_CARD_USER_REQ, *PLSA2_IPC_GET_SMART_CARD_USER_REQ;

typedef struct _LSA2_IPC_GET_SMART_CARD_USER_RES
{
    PLSA_SECURITY_OBJECT pObject;
    PSTR pszSmartCardReader;
} LSA2_IPC_GET_SMART_CARD_USER_RES, *PLSA2_IPC_GET_SMART_CARD_USER_RES;

typedef struct _LSA_IPC_AUTH_USER_EX_REQ
{
    PCSTR pszTargetProvider;
    LSA_AUTH_USER_PARAMS authUserParams;
} LSA_IPC_AUTH_USER_EX_REQ, *PLSA_IPC_AUTH_USER_EX_REQ;

#define MAP_LWMSG_ERROR(_e_) (LwMapLwmsgStatusToLwError(_e_))
#define MAP_LW_ERROR_IPC(_e_) ((_e_) ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS)

LWMsgProtocolSpec*
LsaIPCGetProtocolSpec(
    void
    );

LWMsgTypeSpec*
LsaIPCGetAuthUserInfoSpec(
    void
    );

DWORD
LsaOpenServer(
    PHANDLE phConnection
    );

DWORD
LsaCloseServer(
    HANDLE hConnection
    );

extern LWMsgTypeSpec gLsaSecurityObjectSpec[];

#endif /*__LSAIPC_H__*/


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
