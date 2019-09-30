/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
