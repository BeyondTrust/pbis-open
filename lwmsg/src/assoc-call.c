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
 *        assoc-call.c
 *
 * Abstract:
 *
 *        Association API
 *        Trivial call API implementation
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#include "assoc-private.h"
#include "util-private.h"

static
void
lwmsg_assoc_call_release(
    LWMsgCall* call
    )
{
    ((AssocCall*) call)->in_use = LWMSG_FALSE;
}

static
LWMsgStatus
lwmsg_assoc_call_dispatch(
    LWMsgCall* call,
    const LWMsgParams* in,
    LWMsgParams* out,
    LWMsgCompleteFunction complete,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool retry = LWMSG_FALSE;
    LWMsgAssoc* assoc = LWMSG_OBJECT_FROM_MEMBER(call, LWMsgAssoc, call);
    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;
    LWMsgSession* session = NULL;

    if (complete)
    {
        /* Async completion not yet supported */
        BAIL_ON_ERROR(status = LWMSG_STATUS_UNIMPLEMENTED);
    }

    request.tag = in->tag;
    request.data = in->data;
    
    do
    {
        status = lwmsg_assoc_send_message(assoc, &request);
        if (status == LWMSG_STATUS_SUCCESS)
        {
            status = lwmsg_assoc_recv_message(assoc, &response);
        }

        if (!retry && 
            (status == LWMSG_STATUS_PEER_RESET ||
             status == LWMSG_STATUS_PEER_CLOSE))
        {
            BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));
            BAIL_ON_ERROR(status = lwmsg_assoc_reset(assoc));
            BAIL_ON_ERROR(status = lwmsg_assoc_connect(assoc, session));
            status = LWMSG_STATUS_AGAIN;
            retry = LWMSG_TRUE;
        }
    } while (status == LWMSG_STATUS_AGAIN);
    
    BAIL_ON_ERROR(status);
    BAIL_ON_ERROR(status = response.status);
    
    out->tag = response.tag;
    out->data = response.data;
    
done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
lwmsg_assoc_call_destroy_params(
    LWMsgCall* call,
    LWMsgParams* params
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = LWMSG_OBJECT_FROM_MEMBER(call, LWMsgAssoc, call);
    LWMsgMessage message = LWMSG_MESSAGE_INITIALIZER;

    message.tag = params->tag;
    message.data = params->data;
    
    BAIL_ON_ERROR(status = lwmsg_assoc_destroy_message(assoc, &message));

    params->tag = message.tag;
    params->data = message.data;
    
error:

    return status;
}

static
LWMsgSession*
lwmsg_assoc_call_get_session(
    LWMsgCall* call
    )
{
    LWMsgAssoc* assoc = LWMSG_OBJECT_FROM_MEMBER(call, LWMsgAssoc, call);
    LWMsgSession* session = NULL;

    LWMSG_ASSERT_SUCCESS(lwmsg_assoc_get_session(assoc, &session));

    return session;
}

static LWMsgCallClass assoc_call_vtbl =
{
    .release = lwmsg_assoc_call_release,
    .dispatch = lwmsg_assoc_call_dispatch,
    .destroy_params = lwmsg_assoc_call_destroy_params,
    .get_session = lwmsg_assoc_call_get_session
};

LWMsgStatus
lwmsg_assoc_call_init(
    AssocCall* call
    )
{
    call->base.vtbl = &assoc_call_vtbl;

    return LWMSG_STATUS_SUCCESS;
}
