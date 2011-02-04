/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        Local Provider Inter-process communication API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "ipc.h"


static LWMsgTypeSpec gLsaLuidAndAttributesSpec[] =
{
    LWMSG_STRUCT_BEGIN(LUID_AND_ATTRIBUTES),
    LWMSG_MEMBER_STRUCT_BEGIN(LUID_AND_ATTRIBUTES, Luid),
    LWMSG_MEMBER_UINT32(LUID, LowPart),
    LWMSG_MEMBER_UINT32(LUID, HighPart),
    LWMSG_STRUCT_END,
    LWMSG_MEMBER_UINT32(LUID_AND_ATTRIBUTES, Attributes),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaPrivilegeSetSpec[] =
{
    LWMSG_STRUCT_BEGIN(PRIVILEGE_SET),
    LWMSG_MEMBER_UINT32(PRIVILEGE_SET, PrivilegeCount),
    LWMSG_MEMBER_UINT32(PRIVILEGE_SET, Control),
    LWMSG_MEMBER_POINTER_BEGIN(PRIVILEGE_SET, Privilege),
    LWMSG_TYPESPEC(gLsaLuidAndAttributesSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(PRIVILEGE_SET, PrivilegeCount),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaLocalIPCEnumPrivilegesSidsReqSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_LOCAL_IPC_ENUM_PRIVILEGES_SIDS_REQ),
    LWMSG_MEMBER_UINT32(LSA_LOCAL_IPC_ENUM_PRIVILEGES_SIDS_REQ, NumSids),
    LWMSG_MEMBER_POINTER_BEGIN(LSA_LOCAL_IPC_ENUM_PRIVILEGES_SIDS_REQ, ppszSids),
    LWMSG_PSTR,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_POINTER_END,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_LENGTH_MEMBER(LSA_LOCAL_IPC_ENUM_PRIVILEGES_SIDS_REQ, NumSids),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaLocalIPCEnumPrivilegesSidsRespSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_LOCAL_IPC_ENUM_PRIVILEGES_SIDS_RESP),
    LWMSG_TYPESPEC(gLsaPrivilegeSetSpec),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec*
LsaLocalIpcGetEnumPrivilegesSidsReqSpec(
    VOID
    )
{
    return gLsaLocalIPCEnumPrivilegesSidsReqSpec;
}

LWMsgTypeSpec*
LsaLocalIpcGetEnumPrivilegesSidsRespSpec(
    VOID
    )
{
    return gLsaLocalIPCEnumPrivilegesSidsRespSpec;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
