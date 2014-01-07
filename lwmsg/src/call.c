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
 *        call.c
 *
 * Abstract:
 *
 *        Call handle API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#include "call-private.h"
#include "util-private.h"

LWMsgStatus
lwmsg_call_dispatch(
    LWMsgCall* call,
    const LWMsgParams* in,
    LWMsgParams* out,
    LWMsgCompleteFunction complete,
    void* data
    )
{
    return call->vtbl->dispatch(call, in, out, complete, data);
}

void
lwmsg_call_pend(
    LWMsgCall* call,
    LWMsgCancelFunction cancel,
    void* data
    )
{
    call->vtbl->pend(call, cancel, data);
}

void
lwmsg_call_complete(
    LWMsgCall* call,
    LWMsgStatus status
    )
{
    call->vtbl->complete(call, status);
}

void
lwmsg_call_cancel(
    LWMsgCall* call
    )
{
    call->vtbl->cancel(call);
}

LWMsgStatus
lwmsg_call_wait(
    LWMsgCall* call
    )
{
    return call->vtbl->wait(call);
}

void
lwmsg_call_release(
    LWMsgCall* call
    )
{
    if (call)
    {
        call->vtbl->release(call);
    }
}

LWMsgStatus
lwmsg_call_destroy_params(
    LWMsgCall* call,
    LWMsgParams* params
    )
{
    return call->vtbl->destroy_params(call, params);
}

LWMsgSession*
lwmsg_call_get_session(
    LWMsgCall* call
    )
{
    return call->vtbl->get_session(call);
}

void
lwmsg_call_set_user_data(
    LWMsgCall* call,
    void* data
    )
{
    call->user_data = data;
}

void*
lwmsg_call_get_user_data(
    LWMsgCall* call
    )
{
    return call->user_data;
}

LWMsgCallDirection
lwmsg_call_get_direction(
    LWMsgCall* call
    )
{
    return call->is_outgoing ? LWMSG_CALL_OUTGOING : LWMSG_CALL_INCOMING;
}

