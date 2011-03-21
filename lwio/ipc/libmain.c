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

#include "includes.h"

DWORD
LwIoIPCMapLWMsgStatus(
    LWMsgStatus status
    )
{
    DWORD dwError = 0;

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        dwError = LWIO_ERROR_SUCCESS;
        break;
    case LWMSG_STATUS_ERROR:
        dwError = LWIO_ERROR_LWMSG_ERROR;
        break;
    case LWMSG_STATUS_AGAIN:
        dwError = STATUS_RETRY;
        break;
    case LWMSG_STATUS_MEMORY:
        dwError = LWIO_ERROR_OUT_OF_MEMORY;
        break;
    case LWMSG_STATUS_MALFORMED:
        dwError = LWIO_ERROR_MALFORMED_REQUEST;
        break;
    case LWMSG_STATUS_EOF:
        dwError = LWIO_ERROR_LWMSG_EOF;
        break;
    case LWMSG_STATUS_NOT_FOUND:
        dwError = LWIO_ERROR_NO_SUCH_ITEM;
        break;
    case LWMSG_STATUS_UNIMPLEMENTED:
        dwError = LWIO_ERROR_NOT_IMPLEMENTED;
        break;
    case LWMSG_STATUS_INVALID_PARAMETER:
        dwError = LWIO_ERROR_INVALID_PARAMETER;
        break;
    case LWMSG_STATUS_OVERFLOW:
        dwError = LWIO_ERROR_OVERFLOW;
        break;
    case LWMSG_STATUS_UNDERFLOW:
        dwError = LWIO_ERROR_UNDERFLOW;
        break;
    case LWMSG_STATUS_SYSTEM:
        dwError = LWIO_ERROR_SYSTEM;
        break;
    case LWMSG_STATUS_TIMEOUT:
        dwError = STATUS_IO_TIMEOUT;
        break;
    case LWMSG_STATUS_SECURITY:
        dwError = STATUS_ACCESS_DENIED;
        break;
    case LWMSG_STATUS_CANCELLED:
        dwError = STATUS_CANCELLED;
        break;
    case LWMSG_STATUS_FILE_NOT_FOUND:
        dwError = STATUS_NOT_FOUND;
        break;
    default:
        LWIO_LOG_ERROR("Failed to map lwmsg error [%d]", status);
        dwError = LWIO_ERROR_LWMSG_ERROR;
        break;
    }

    return dwError;
}
