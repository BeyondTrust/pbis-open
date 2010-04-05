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
 *        ipc_protocol.c
 *
 * Abstract:
 *
 *        Event forwarder from eventlogd to collector service
 *
 *        Active Directory Site API
 *
 * Authors:
 *          Brian koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "includes.h"

static LWMsgTypeSpec gEfdIPCErrorSpec[] =
{
    LWMSG_STRUCT_BEGIN(EFD_IPC_ERROR),
    LWMSG_MEMBER_UINT32(EFD_IPC_ERROR, dwError),
    LWMSG_MEMBER_PSTR(EFD_IPC_ERROR, pszErrorMessage),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gEfdIPCLogInfoSpec[] =
{
    LWMSG_STRUCT_BEGIN(EFD_LOG_INFO),
    LWMSG_MEMBER_UINT8(EFD_LOG_INFO, maxAllowedLogLevel),
    LWMSG_MEMBER_UINT8(EFD_LOG_INFO, logTarget),
    LWMSG_MEMBER_PSTR(EFD_LOG_INFO, pszPath),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gEfdIPCSetLogInfoReqSpec[] =
{
    LWMSG_POINTER_BEGIN,
    LWMSG_TYPESPEC(gEfdIPCLogInfoSpec),
    LWMSG_POINTER_END,
    LWMSG_TYPE_END
};

static LWMsgProtocolSpec gEfdIPCSpec[] =
{
    LWMSG_MESSAGE(EFD_Q_SET_LOGINFO, gEfdIPCSetLogInfoReqSpec),
    LWMSG_MESSAGE(EFD_R_SET_LOGINFO_SUCCESS, NULL),
    LWMSG_MESSAGE(EFD_R_SET_LOGINFO_FAILURE, gEfdIPCErrorSpec),
    LWMSG_MESSAGE(EFD_Q_GET_LOGINFO, NULL),
    LWMSG_MESSAGE(EFD_R_GET_LOGINFO_SUCCESS, gEfdIPCLogInfoSpec),
    LWMSG_MESSAGE(EFD_R_GET_LOGINFO_FAILURE, gEfdIPCErrorSpec),
    LWMSG_PROTOCOL_END
};

LWMsgProtocolSpec*
EfdIPCGetProtocolSpec(
    void
    )
{
    return gEfdIPCSpec;
}
