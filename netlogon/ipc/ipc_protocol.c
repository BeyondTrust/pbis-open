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
 *        Likewise Netlogon
 *
 *        Active Directory Site API
 *
 * Authors:
 *          Brian koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "includes.h"

static LWMsgTypeSpec gLWNetIpcErrorSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_ERROR),
    LWMSG_MEMBER_UINT32(LWNET_IPC_ERROR, dwError),
    LWMSG_MEMBER_PSTR(LWNET_IPC_ERROR, pszErrorMessage),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIpcLogInfoSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_LOG_INFO),
    LWMSG_MEMBER_UINT32(LWNET_IPC_LOG_INFO, LogLevel),
    LWMSG_MEMBER_UINT32(LWNET_IPC_LOG_INFO, LogTarget),
    LWMSG_MEMBER_PSTR(LWNET_IPC_LOG_INFO, pszLogPath),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIpcConstStringSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_CONST_STRING),
    LWMSG_MEMBER_PSTR(LWNET_IPC_CONST_STRING, pszString),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIpcStringSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_STRING),
    LWMSG_MEMBER_PSTR(LWNET_IPC_STRING, pszString),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIpcTimeSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_TIME),
    LWMSG_MEMBER_INT64(LWNET_IPC_TIME, Time),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIpcGetDcSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_GET_DC),
    LWMSG_MEMBER_PSTR(LWNET_IPC_GET_DC, pszServerFQDN),
    LWMSG_MEMBER_PSTR(LWNET_IPC_GET_DC, pszDomainFQDN),
    LWMSG_MEMBER_PSTR(LWNET_IPC_GET_DC, pszSiteName),
    LWMSG_MEMBER_PSTR(LWNET_IPC_GET_DC, pszPrimaryDomain),
    LWMSG_MEMBER_UINT32(LWNET_IPC_GET_DC, dwFlags),
    LWMSG_MEMBER_UINT32(LWNET_IPC_GET_DC, dwBlackListCount),
    LWMSG_MEMBER_POINTER_BEGIN(LWNET_IPC_GET_DC, ppszAddressBlackList),
    LWMSG_PSTR,
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(LWNET_IPC_GET_DC, dwBlackListCount),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetDcInfoSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_DC_INFO),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwPingTime),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwDomainControllerAddressType),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwFlags),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwVersion),
    LWMSG_MEMBER_UINT16(LWNET_DC_INFO, wLMToken),
    LWMSG_MEMBER_UINT16(LWNET_DC_INFO, wNTToken),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDomainControllerName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDomainControllerAddress),
    LWMSG_MEMBER_ARRAY_BEGIN(LWNET_DC_INFO, pucDomainGUID),
    LWMSG_UINT8(UCHAR),
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
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetDcAddressSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_DC_ADDRESS),
    LWMSG_MEMBER_PSTR(LWNET_DC_ADDRESS, pszDomainControllerName),
    LWMSG_MEMBER_PSTR(LWNET_DC_ADDRESS, pszDomainControllerAddress),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLWNetIpcDcListSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_IPC_DC_LIST),
    LWMSG_MEMBER_UINT32(LWNET_IPC_DC_LIST, dwDcCount),
    LWMSG_MEMBER_POINTER_BEGIN(LWNET_IPC_DC_LIST, pDcList),
    LWMSG_TYPESPEC(gLWNetDcAddressSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(LWNET_IPC_DC_LIST, dwDcCount),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgProtocolSpec gLWNetIPCSpec[] =
{
    LWMSG_MESSAGE(LWNET_R_ERROR, gLWNetIpcErrorSpec), // LWNET_IPC_ERROR
    LWMSG_MESSAGE(LWNET_Q_SET_LOG_LEVEL, gLWNetIpcLogInfoSpec), // LWNET_IPC_LOG_INFO
    LWMSG_MESSAGE(LWNET_R_SET_LOG_LEVEL, NULL), // NULL
    LWMSG_MESSAGE(LWNET_Q_GET_LOG_INFO, NULL), // NULL
    LWMSG_MESSAGE(LWNET_R_GET_LOG_INFO, gLWNetIpcLogInfoSpec), // LWNET_IPC_LOG_INFO
    LWMSG_MESSAGE(LWNET_Q_GET_DC_TIME, gLWNetIpcConstStringSpec), // LWNET_IPC_CONST_STRING
    LWMSG_MESSAGE(LWNET_R_GET_DC_TIME, gLWNetIpcTimeSpec), // LWNET_IPC_TIME
    LWMSG_MESSAGE(LWNET_Q_GET_DC_NAME, gLWNetIpcGetDcSpec), // LWNET_IPC_GET_DC
    LWMSG_MESSAGE(LWNET_R_GET_DC_NAME, gLWNetDcInfoSpec), // LWNET_DC_INFO
    LWMSG_MESSAGE(LWNET_Q_GET_DOMAIN_CONTROLLER, gLWNetIpcConstStringSpec), // LWNET_IPC_CONST_STRING
    LWMSG_MESSAGE(LWNET_R_GET_DOMAIN_CONTROLLER, gLWNetIpcStringSpec), // LWNET_IPC_STRING
    LWMSG_MESSAGE(LWNET_Q_GET_CURRENT_DOMAIN, NULL), // NULL
    LWMSG_MESSAGE(LWNET_R_GET_CURRENT_DOMAIN, gLWNetIpcStringSpec), // LWNET_IPC_STRING
    LWMSG_MESSAGE(LWNET_Q_GET_DC_LIST, gLWNetIpcGetDcSpec), // LWNET_IPC_GET_DC
    LWMSG_MESSAGE(LWNET_R_GET_DC_LIST, gLWNetIpcDcListSpec), // LWNET_IPC_DC_LIST
    LWMSG_PROTOCOL_END
};

LWMsgProtocolSpec*
LWNetIPCGetProtocolSpec(
    void
    )
{
    return gLWNetIPCSpec;
}
