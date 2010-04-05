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

#ifndef __UIPC_IPC_H__
#define __UIPC_IPC_H__

#include <uipc/marshal.h>
#include <uipc/status.h>
#include <uipc/time.h>

#include <stdlib.h>

struct __uipc_handle;
typedef struct __uipc_handle uipc_handle;

struct __uipc_message;
typedef struct __uipc_message uipc_message;

typedef unsigned int uipc_message_type;

uipc_handle* uipc_attach(int socket);
uipc_status uipc_recv_async(uipc_handle* handle, uipc_message** message);
uipc_status uipc_recv(uipc_handle* handle, uipc_message** message, uipc_time* abs);
uipc_status uipc_send_async(uipc_handle* handle, uipc_message* message);
uipc_status uipc_send(uipc_handle* handle, uipc_message* message, uipc_time* abs);
uipc_status uipc_detach(uipc_handle* handle);
uipc_status uipc_close(uipc_handle* handle);

uipc_message* uipc_msg_new(uipc_message_type type);
void uipc_msg_free(uipc_message* message);
uipc_message_type uipc_msg_get_type(uipc_message* message);
void* uipc_msg_get_payload(uipc_message* message, uipc_typeinfo* info);
void uipc_msg_set_payload(uipc_message* message, const void* payload, uipc_typeinfo* info);
void uipc_msg_free_payload(void* payload, uipc_typeinfo* info);

#endif
