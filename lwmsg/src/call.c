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

