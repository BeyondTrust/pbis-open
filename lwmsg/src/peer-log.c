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
 *        peer-log.c
 *
 * Abstract:
 *
 *        Multi-threaded peer API (logging)
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include <config.h>
#include "peer-private.h"
#include "buffer-private.h"
#include "security-private.h"

void
lwmsg_peer_session_string_for_session(
    LWMsgSession* session,
    LWMsgSessionString string
    )
{
    const LWMsgSessionID* rsmid = NULL;

    rsmid = lwmsg_session_get_id(session);
    lwmsg_session_id_to_string(rsmid, string);
}

LWMsgStatus
lwmsg_peer_log_message(
    PeerAssocTask* task,
    LWMsgMessage* message,
    LWMsgBool is_outgoing
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    const char* call_direction = NULL;
    const char* message_direction = NULL;
    const char* message_type = NULL;
    char* msg_text = NULL;
    const char* status_text = NULL;
    LWMsgSessionString sessid = {0};

    if (lwmsg_context_would_log(task->session->peer->context, LWMSG_LOGLEVEL_TRACE))
    {
        lwmsg_peer_session_string_for_session((LWMsgSession*) task->session, sessid);

        if (message->flags & LWMSG_MESSAGE_FLAG_REPLY)
        {
            if (message->flags & LWMSG_MESSAGE_FLAG_SYNTHETIC)
            {
                message_direction = "srs";
            }
            else
            {
                message_direction = "res";
            }
        }
        else
        {
            if (message->flags & LWMSG_MESSAGE_FLAG_SYNTHETIC)
            {
                message_direction = "srq";
            }
            else
            {
                message_direction = "req";
            }
        }

        if (message->flags & LWMSG_MESSAGE_FLAG_CONTROL)
        {
            message_type = "ctrl";
        }
        else
        {
            message_type = "call";
        }

        if (is_outgoing)
        {
            call_direction = "<<";
        }
        else
        {
            call_direction = ">>";
        }

        if (message->status != LWMSG_STATUS_SUCCESS)
        {
            status_text = lwmsg_string_without_prefix(
                lwmsg_status_name(message->status),
                "LWMSG_STATUS_");
        }

        if (message->tag != LWMSG_TAG_INVALID)
        {
            BAIL_ON_ERROR(lwmsg_assoc_print_message_alloc(task->assoc, message, &msg_text));
        }

        if (msg_text)
        {
            if (status_text)
            {
                LWMSG_LOG_TRACE(
                    task->session->peer->context,
                    "(session:%s %s %u) %s %s [%s] %s",
                    sessid,
                    call_direction,
                    message->cookie,
                    message_type,
                    message_direction,
                    status_text,
                    msg_text);
            }
            else
            {
                LWMSG_LOG_TRACE(
                    task->session->peer->context,
                    "(session:%s %s %u) %s %s %s",
                    sessid,
                    call_direction,
                    message->cookie,
                    message_type,
                    message_direction,
                    msg_text);
            }
        }
        else
        {
            if (status_text)
            {
                LWMSG_LOG_TRACE(
                    task->session->peer->context,
                    "(session:%s %s %u) %s %s [%s]",
                    sessid,
                    call_direction,
                    message->cookie,
                    message_type,
                    message_direction,
                    status_text);
            }
            else
            {
                LWMSG_LOG_TRACE(
                    task->session->peer->context,
                    "(session:%s %s %u) %s %s",
                    sessid,
                    call_direction,
                    message->cookie,
                    message_type,
                    message_direction);
            }
        }
    }

cleanup:

    if (msg_text)
    {
        lwmsg_context_free(task->session->peer->context, msg_text);
    }

    return status;

error:

    goto cleanup;
}

LWMsgStatus
lwmsg_peer_log_accept(
    PeerAssocTask* task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSessionString sessid;
    LWMsgBuffer buffer = {.wrap = lwmsg_buffer_realloc_wrap};
    const char* peerid = NULL;
    unsigned char nul = 0;
    LWMsgSecurityToken* token = lwmsg_session_get_peer_security_token((LWMsgSession*) task->session);

    if (lwmsg_context_would_log(task->session->peer->context, LWMSG_LOGLEVEL_VERBOSE))
    {
        lwmsg_peer_session_string_for_session((LWMsgSession*) task->session, sessid);
        if (token)
        {
            BAIL_ON_ERROR(status = lwmsg_security_token_to_string(
                token,
                &buffer));
            BAIL_ON_ERROR(status = lwmsg_buffer_write(&buffer, &nul, 1));
            peerid = (const char*) buffer.base;
        }
        else
        {
            peerid = "<anonymous>";
        }

        LWMSG_LOG_VERBOSE(
            task->session->peer->context,
            "(session:%s) Accepted %s",
            sessid,
            peerid);
    }

error:

    if (buffer.base)
    {
        free(buffer.base);
    }

    return status;
}

LWMsgStatus
lwmsg_peer_log_connect(
    PeerAssocTask* task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSessionString sessid;
    LWMsgBuffer buffer = {.wrap = lwmsg_buffer_realloc_wrap};
    const char* peerid = NULL;
    unsigned char nul = 0;
    LWMsgSecurityToken* token = lwmsg_session_get_peer_security_token((LWMsgSession*) task->session);

    if (lwmsg_context_would_log(task->session->peer->context, LWMSG_LOGLEVEL_VERBOSE))
    {
        lwmsg_peer_session_string_for_session((LWMsgSession*) task->session, sessid);
        if (token)
        {
            BAIL_ON_ERROR(status = lwmsg_security_token_to_string(
                token,
                &buffer));
            BAIL_ON_ERROR(status = lwmsg_buffer_write(&buffer, &nul, 1));
            peerid = (const char*) buffer.base;
        }
        else
        {
            peerid = "<anonymous>";
        }

        LWMSG_LOG_VERBOSE(
            task->session->peer->context,
            "(session:%s) Connected %s",
            sessid,
            peerid);
    }

error:

    if (buffer.base)
    {
        free(buffer.base);
    }

    return status;
}

static
LWMsgStatus
lwmsg_peer_log_call(
    PeerAssocTask* task,
    PeerCall* call,
    LWMsgBuffer* buffer,
    LWMsgBool incoming
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMessage message = LWMSG_MESSAGE_INITIALIZER;
    char* rep = NULL;

    if (incoming)
    {
        BAIL_ON_ERROR(status = lwmsg_buffer_print(
            buffer,
            "    call >> %lu ",
            (unsigned long) call->cookie));
        message.tag = call->params.incoming.in.tag;
        message.data = call->params.incoming.in.data;
    }
    else
    {
        BAIL_ON_ERROR(status = lwmsg_buffer_print(
            buffer,
            "    call << %lu ",
            (unsigned long) call->cookie));
        message.tag = call->params.outgoing.in->tag;
        message.data = call->params.outgoing.in->data;
    }

    BAIL_ON_ERROR(status = lwmsg_assoc_print_message_alloc(
        task->assoc,
        &message,
        &rep));

    BAIL_ON_ERROR(status = lwmsg_buffer_write(
        buffer,
        (unsigned char*) rep,
        strlen(rep)));

error:

    if (rep)
    {
        lwmsg_context_free(task->session->peer->context, rep);
    }

    return status;
}

LWMsgStatus
lwmsg_peer_log_state(
    PeerAssocTask* task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBuffer buffer = {.wrap = lwmsg_buffer_realloc_wrap};
    LWMsgSessionString sess;
    LWMsgHashIter iter = {0};
    PeerCall* call = NULL;
    static const unsigned char nul = '\0';
    LWMsgSecurityToken* token = NULL;

    if (!lwmsg_context_would_log(task->session->peer->context, LWMSG_LOGLEVEL_TRACE))
    {
        goto error;
    }

    lwmsg_session_id_to_string(
        lwmsg_session_get_id((LWMsgSession*) task->session),
        sess);

    token = lwmsg_session_get_peer_security_token((LWMsgSession*) task->session);

    BAIL_ON_ERROR(status = lwmsg_buffer_print(&buffer, "lwmsg session %s:\n", sess));

    if (task->session->endpoint_str)
    {
        BAIL_ON_ERROR(status = lwmsg_buffer_print(
            &buffer,
            "    endpoint: %s\n", task->session->endpoint_str));
    }

    if (token)
    {
        BAIL_ON_ERROR(status = lwmsg_buffer_print(
            &buffer,
            "    %s: ",
            task->session->is_outgoing ? "connected" : "accepted"));
        BAIL_ON_ERROR(status = lwmsg_security_token_to_string(token, &buffer));
        BAIL_ON_ERROR(status = lwmsg_buffer_print(
             &buffer,
             "\n"));
    }

    lwmsg_hash_iter_begin(&task->incoming_calls, &iter);
    while ((call = lwmsg_hash_iter_next(&task->incoming_calls, &iter)))
    {
        BAIL_ON_ERROR(status = lwmsg_peer_log_call(
            task,
            call,
            &buffer,
            LWMSG_TRUE));
        BAIL_ON_ERROR(status = lwmsg_buffer_print(&buffer, "\n"));
    }
    lwmsg_hash_iter_end(&task->incoming_calls, &iter);
    lwmsg_hash_iter_begin(&task->outgoing_calls, &iter);
    while ((call = lwmsg_hash_iter_next(&task->outgoing_calls, &iter)))
    {
        BAIL_ON_ERROR(status = lwmsg_peer_log_call(
            task,
            call,
            &buffer,
            LWMSG_FALSE));
        BAIL_ON_ERROR(status = lwmsg_buffer_print(&buffer, "\n"));
    }
    lwmsg_hash_iter_end(&task->outgoing_calls, &iter);

    BAIL_ON_ERROR(status = lwmsg_buffer_write(&buffer, &nul, 1));

    LWMSG_LOG_TRACE(task->session->peer->context, "%s", buffer.base);

error:

    if (buffer.base)
    {
        free(buffer.base);
    }

    return status;
}
