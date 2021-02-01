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
