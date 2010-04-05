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
 *        status-private.h
 *
 * Abstract:
 *
 *        Status codes (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_STATUS_PRIVATE_H__
#define __LWMSG_STATUS_PRIVATE_H__

#include <stdarg.h>

#include <lwmsg/status.h>

typedef struct LWMsgErrorContext
{
    LWMsgStatus status;
    char* message;
} LWMsgErrorContext;

const char*
lwmsg_error_name(
    LWMsgStatus status
    );

LWMsgStatus
lwmsg_error_map_errno(
    int err
    );

void
lwmsg_error_clear(
    LWMsgErrorContext* context
    );

LWMsgStatus
lwmsg_error_raise_str(
    LWMsgErrorContext* context,
    LWMsgStatus status,
    const char* message
    );
       
LWMsgStatus
lwmsg_error_raise_take_str(
    LWMsgErrorContext* context,
    LWMsgStatus status,
    char* message
    );

LWMsgStatus
lwmsg_error_raise_errno(
    LWMsgErrorContext* context,
    int err
    );
           
LWMsgStatus
lwmsg_error_raise_v(
    LWMsgErrorContext* context,
    LWMsgStatus status,
    const char* fmt,
    va_list ap
    );

LWMsgStatus
lwmsg_error_raise(
    LWMsgErrorContext* context,
    LWMsgStatus status,
    const char* fmt,
    ...
    );
                      
const char*
lwmsg_error_message(
    LWMsgStatus status,
    LWMsgErrorContext* context
    );

LWMsgStatus
lwmsg_error_propagate(
    LWMsgErrorContext* from_context,
    LWMsgErrorContext* to_context,
    LWMsgStatus status
    );

#define RAISE(ec, expr, ...) (lwmsg_error_raise((ec), (expr), __VA_ARGS__))

#endif
