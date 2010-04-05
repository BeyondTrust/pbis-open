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

#ifndef __UIPC_WIRE_H__
#define __UIPC_WIRE_H__

#include <uipc/status.h>
#include <uipc/ipc.h>
#include <uipc/time.h>

typedef struct uipc_packet_header
{
	enum
	{
		PACKET_MESSAGE, PACKET_ACK
	} type;
	unsigned long length;
} uipc_packet_header;

typedef struct uipc_packet_message
{
	uipc_message_type type;
	unsigned long length;
    char payload[];
} uipc_packet_message;

typedef struct uipc_packet
{
	uipc_packet_header header;
	union
	{
		uipc_packet_message message;
	} u;
} uipc_packet;

uipc_status uipc_packet_send(int socket, uipc_packet* packet);
uipc_status uipc_packet_recv(int socket, uipc_packet** packet);
uipc_status uipc_packet_available(int socket, uipc_time* abs);
uipc_status uipc_packet_sendable(int socket, uipc_time* abs);

#endif
