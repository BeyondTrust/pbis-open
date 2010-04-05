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
#include "includes.h"

#if defined(WORDS_BIGENDIAN)
#  define UCS2_NATIVE "UCS-2BE"
#else
#  define UCS2_NATIVE "UCS-2LE"
#endif

#define LWMSG_MEMBER_PWSTR(_type, _field)           \
    LWMSG_MEMBER_POINTER_BEGIN(_type, _field),      \
    LWMSG_UINT16(wchar16_t),                        \
    LWMSG_POINTER_END,                              \
    LWMSG_ATTR_ZERO_TERMINATED,                     \
    LWMSG_ATTR_ENCODING(UCS2_NATIVE)

LWMsgTypeSpec gPstoreFileDBMachineAcctInfoSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWPS_FILE_DB_MACHINE_ACCT_INFO),
    LWMSG_MEMBER_PWSTR(LWPS_FILE_DB_MACHINE_ACCT_INFO, pwszDomainSID),
    LWMSG_MEMBER_PWSTR(LWPS_FILE_DB_MACHINE_ACCT_INFO, pwszDomainName),
    LWMSG_MEMBER_PWSTR(LWPS_FILE_DB_MACHINE_ACCT_INFO, pwszDomainDnsName),
    LWMSG_MEMBER_PWSTR(LWPS_FILE_DB_MACHINE_ACCT_INFO, pwszHostName),
    LWMSG_MEMBER_PWSTR(LWPS_FILE_DB_MACHINE_ACCT_INFO, pwszHostDnsDomain),
    LWMSG_MEMBER_PWSTR(LWPS_FILE_DB_MACHINE_ACCT_INFO, pwszMachineAccountName),
    LWMSG_MEMBER_PWSTR(LWPS_FILE_DB_MACHINE_ACCT_INFO, pwszMachineAccountPassword),
    LWMSG_MEMBER_UINT64(LWPS_FILE_DB_MACHINE_ACCT_INFO, pwdCreationTimestamp),
    LWMSG_MEMBER_UINT64(LWPS_FILE_DB_MACHINE_ACCT_INFO, pwdClientModifyTimestamp),
    LWMSG_MEMBER_UINT32(LWPS_FILE_DB_MACHINE_ACCT_INFO, dwSchannelType),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec*
FileDB_GetMachineAcctInfoSpec(
    void
    )
{
    return gPstoreFileDBMachineAcctInfoSpec;
}

DWORD
FileDB_MapLwmsgStatus(
    LWMsgStatus status
    )
{
    switch (status)
    {
    default:
        return LWPS_ERROR_INTERNAL;
    case LWMSG_STATUS_SUCCESS:
        return LWPS_ERROR_SUCCESS;
    case LWMSG_STATUS_ERROR:
        return LWPS_ERROR_INTERNAL;
    case LWMSG_STATUS_MEMORY:
        return LWPS_ERROR_OUT_OF_MEMORY;
    case LWMSG_STATUS_MALFORMED:
    case LWMSG_STATUS_OVERFLOW:
    case LWMSG_STATUS_UNDERFLOW:
    case LWMSG_STATUS_EOF:
        return LWPS_ERROR_INVALID_MESSAGE;
    case LWMSG_STATUS_INVALID_PARAMETER:
        return LWPS_ERROR_INVALID_PARAMETER;
    case LWMSG_STATUS_INVALID_STATE:
        return EINVAL;
    case LWMSG_STATUS_UNIMPLEMENTED:
        return LWPS_ERROR_NOT_IMPLEMENTED;
    case LWMSG_STATUS_SYSTEM:
        return LWPS_ERROR_INTERNAL;
    case LWMSG_STATUS_SECURITY:
        return EACCES;
    case LWMSG_STATUS_CANCELLED:
        return EINTR;
    case LWMSG_STATUS_FILE_NOT_FOUND:
        return ENOENT;
    case LWMSG_STATUS_CONNECTION_REFUSED:
        return ECONNREFUSED;
    case LWMSG_STATUS_PEER_RESET:
        return ECONNRESET;
    case LWMSG_STATUS_PEER_ABORT:
        return ECONNABORTED;
    case LWMSG_STATUS_PEER_CLOSE:
        return EPIPE;
    case LWMSG_STATUS_SESSION_LOST:
        return EPIPE;
    }
}
