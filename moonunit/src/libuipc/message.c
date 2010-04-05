/*
 * Copyright (c) 2007, Brian Koropoff
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Moonunit project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRIAN KOROPOFF ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL BRIAN KOROPOFF BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE
#include <uipc/ipc.h>
#include <uipc/wire.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>

#define PAYLOAD_BUFFER_SIZE (1024)

struct __uipc_message
{
    uipc_message_type type;
    void* payload;
    uipc_typeinfo* payload_type;
    uipc_packet* packet;
};

struct __uipc_handle
{
    bool readable, writeable;
	int socket;
};

static
uipc_packet*
packet_from_message(uipc_message* message)
{
    uipc_packet* packet = malloc(sizeof (uipc_packet_header) + 
                                 sizeof (uipc_packet_message) +
                                 PAYLOAD_BUFFER_SIZE);
    unsigned long payload_length;

    if (!packet)
        return NULL;

    packet->header.type = PACKET_MESSAGE;
    packet->u.message.type = message->type;

    payload_length = uipc_marshal_payload(packet->u.message.payload, PAYLOAD_BUFFER_SIZE,
                                          message->payload, message->payload_type);
    
    if (payload_length > PAYLOAD_BUFFER_SIZE)
    {
        packet = realloc(packet, 
                         sizeof(uipc_packet_header) +
                         sizeof(uipc_packet_message) +
                         payload_length);
        payload_length = uipc_marshal_payload(packet->u.message.payload, payload_length,
                                              message->payload, message->payload_type);
    }

    packet->u.message.length = payload_length;
    packet->header.length = 
        sizeof(uipc_packet_header) +
        sizeof(uipc_packet_message) +
        payload_length;

    return packet;
}

static uipc_message* 
message_from_packet(uipc_packet* packet)
{
    uipc_message* message = malloc(sizeof(uipc_message));
	
    if (!message)
        return NULL;

    message->type = packet->u.message.type;
    message->packet = packet;
    message->payload = NULL;
    message->payload_type = NULL;

    return message;
}

uipc_handle* 
uipc_attach(int socket)
{
    uipc_handle* handle = malloc(sizeof (uipc_handle));

    if (!handle)
        return NULL;
	
    handle->socket = socket;
    handle->readable = handle->writeable = true;

    return handle;
}

uipc_status
uipc_recv_async(uipc_handle* handle, uipc_message** message)
{
    uipc_packet* packet = NULL;
    uipc_status result;

    if (!handle->readable)
        return UIPC_EOF;

    result = uipc_packet_recv(handle->socket, &packet);
        
    if (result == UIPC_EOF)
    {
        handle->readable = false;
        return UIPC_EOF;
    }
    else if (result != UIPC_SUCCESS)
    {
        return result;
    }
    else
    {
        switch (packet->header.type)
        {
        case PACKET_MESSAGE:
        {
            *message = message_from_packet(packet);
            
            if (!*message)
            {
                free(packet);
                handle->readable = false;
                return UIPC_NOMEM;
            }
            return UIPC_SUCCESS;
        }
        default:
            return UIPC_ERROR;
        }
    }    
}

uipc_status
uipc_recv(uipc_handle* handle, uipc_message** message, uipc_time* abs)
{
    uipc_status result = UIPC_SUCCESS;
    
    do
    {
        result = uipc_packet_available(handle->socket, abs);
    } while (result == UIPC_RETRY);
    
    if (result != UIPC_SUCCESS)
        return result;
    
	return uipc_recv_async(handle, message);
}

uipc_status
uipc_send_async(uipc_handle* handle, uipc_message* message)
{
    uipc_status result = UIPC_SUCCESS;
    uipc_packet* packet = NULL;

    if (!handle->writeable)
    {
        result = UIPC_EOF;
        goto cleanup;
    }

    packet = packet_from_message(message);

    result = uipc_packet_send(handle->socket, packet);
        
    if (result == UIPC_EOF)
    {
        handle->writeable = false;
        goto cleanup;
    }
    else if (result != UIPC_SUCCESS)
    {
        goto cleanup;
    }

cleanup:
    if (packet)
        free(packet);

    return result;
}

uipc_status
uipc_send(uipc_handle* handle, uipc_message* message, uipc_time* abs)
{
    uipc_status result = UIPC_SUCCESS;
    
    do
    {
        result = uipc_packet_sendable(handle->socket, abs);
    } while (result == UIPC_RETRY);
    
    if (result != UIPC_SUCCESS)
        return result;
    
	return uipc_send_async(handle, message);
}

uipc_status
uipc_detach(uipc_handle* handle)
{
    uipc_status result = UIPC_SUCCESS;

    if (!handle)
        return UIPC_ERROR;
    
    free(handle);
    
    return result;
}

uipc_status
uipc_close(uipc_handle* handle)
{
    uipc_status result = UIPC_SUCCESS;

    if (!handle)
        return UIPC_ERROR;
    
    close(handle->socket);
    free(handle);
    
    return result;
}

uipc_message* 
uipc_msg_new(uipc_message_type type)
{
    uipc_message* message = malloc(sizeof (uipc_message));
	
    if (!message)
        return NULL;

    message->type = type;
    message->payload = NULL;
    message->packet = NULL;

	return message;
}

void
uipc_msg_free(uipc_message* message)
{
    if (message->packet)
        free((void*) message->packet);
    free(message);
}

uipc_message_type
uipc_msg_get_type(uipc_message* message)
{
    return message->type;
}

void*
uipc_msg_get_payload(uipc_message* message, uipc_typeinfo* info)
{
    if (message->packet)
    {
        void* object;
        uipc_unmarshal_payload(&object, message->packet->u.message.payload, info); 
        return object;
    }
    else
    {
        return NULL;
    }
}

void
uipc_msg_free_payload(void* payload, uipc_typeinfo* info)
{
    uipc_free_object(payload, info);
}

void
uipc_msg_set_payload(uipc_message* message, const void* payload, uipc_typeinfo* info)
{
    message->payload = (void*) payload;
    message->payload_type = info;
}
