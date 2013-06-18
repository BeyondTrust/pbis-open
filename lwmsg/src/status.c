/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        status.c
 *
 * Abstract:
 *
 *        Status codes
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "status-private.h"
#include "util-private.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

typedef struct
{
    LWMsgStatus status;
    const char* symbol;
    const char* message;
} StatusInfo;

#define STATUS_INFO(st, msg) [st] = { st, #st, msg }

static const StatusInfo status_info[] =
{
    STATUS_INFO(LWMSG_STATUS_SUCCESS, "Success"),
    STATUS_INFO(LWMSG_STATUS_ERROR, "Error"),
    STATUS_INFO(LWMSG_STATUS_AGAIN, "Retry"),
    STATUS_INFO(LWMSG_STATUS_MEMORY, "Out of memory"),
    STATUS_INFO(LWMSG_STATUS_MALFORMED, "Malformed type or message"),
    STATUS_INFO(LWMSG_STATUS_EOF, "End of file"),
    STATUS_INFO(LWMSG_STATUS_NOT_FOUND, "Item not found"),
    STATUS_INFO(LWMSG_STATUS_UNIMPLEMENTED, "Unimplemented"),
    STATUS_INFO(LWMSG_STATUS_INVALID_PARAMETER, "Invalid parameter"),
    STATUS_INFO(LWMSG_STATUS_INVALID_STATE, "Invalid state"),
    STATUS_INFO(LWMSG_STATUS_OVERFLOW, "Arithmetic overflow"),
    STATUS_INFO(LWMSG_STATUS_UNDERFLOW, "Arithmetic underflow"),
    STATUS_INFO(LWMSG_STATUS_SYSTEM, "Unhandled system error"),
    STATUS_INFO(LWMSG_STATUS_TIMEOUT, "Operation timed out"),
    STATUS_INFO(LWMSG_STATUS_SECURITY, "Security violation"),
    STATUS_INFO(LWMSG_STATUS_CANCELLED, "Operation cancelled"),
    STATUS_INFO(LWMSG_STATUS_FILE_NOT_FOUND, "File not found"),
    STATUS_INFO(LWMSG_STATUS_CONNECTION_REFUSED, "Connection refused"),
    STATUS_INFO(LWMSG_STATUS_PEER_CLOSE, "Connection closed by peer"),
    STATUS_INFO(LWMSG_STATUS_PEER_RESET, "Connection reset by peer"),
    STATUS_INFO(LWMSG_STATUS_PEER_ABORT, "Connection aborted by peer"),
    STATUS_INFO(LWMSG_STATUS_SESSION_LOST, "Session lost"),
    STATUS_INFO(LWMSG_STATUS_UNSUPPORTED, "Unsupported operation"),
    STATUS_INFO(LWMSG_STATUS_INVALID_HANDLE, "Invalid handle"),
    STATUS_INFO(LWMSG_STATUS_BUSY, "Conflicting operation already in progress"),
    STATUS_INFO(LWMSG_STATUS_PENDING, "Operating is pending"),
    STATUS_INFO(LWMSG_STATUS_RESOURCE_LIMIT, "System resource limit reached")
};

LWMsgStatus
lwmsg_status_map_errno(
    int err
    )
{
    switch (err)
    {
    case 0:
        return LWMSG_STATUS_SUCCESS;
    case EINVAL:
        return LWMSG_STATUS_INVALID_PARAMETER;
    case EMFILE:
    case ENFILE:
    case ENOBUFS:
    case ENOMEM:
        return LWMSG_STATUS_RESOURCE_LIMIT;
    case EBUSY:
        return LWMSG_STATUS_BUSY;
    case ENOENT:
        return LWMSG_STATUS_FILE_NOT_FOUND;
    case ECONNREFUSED:
        return LWMSG_STATUS_CONNECTION_REFUSED;
    case ECONNRESET:
        return LWMSG_STATUS_PEER_RESET;
    case ECONNABORTED:
        return LWMSG_STATUS_PEER_ABORT;
    case ETIMEDOUT:
        return LWMSG_STATUS_TIMEOUT;
    default:
        return LWMSG_STATUS_SYSTEM;
    }
}

const char*
lwmsg_status_name(
    LWMsgStatus status
    )
{
    if (status < (sizeof(status_info) / sizeof(*status_info)) && status_info[status].symbol)
    {
        return status_info[status].symbol;
    }
    else
    {
        return "unknown";
    }
}
