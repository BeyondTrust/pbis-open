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
 *        protocol-private.h
 *
 * Abstract:
 *
 *        Protocol specification and construction API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_PROTOCOL_PRIVATE_H__
#define __LWMSG_PROTOCOL_PRIVATE_H__

#include <lwmsg/message.h>
#include <lwmsg/protocol.h>
#include <lwmsg/type.h>
#include <lwmsg/data.h>

#include "context-private.h"
#include "type-private.h"

struct LWMsgProtocol
{
    LWMsgContext* context;
    /* Number of message types in this protocol */
    size_t num_types;
    /* Pointers to protocol spec entries indexed by message tag */
    LWMsgProtocolSpec** types;
    LWMsgMemoryList specmem;
};

typedef struct LWMsgProtocolMessageRep
{
    LWMsgTag tag;
    LWMsgTypeRep* type;
    char* name;
} LWMsgProtocolMessageRep;

typedef struct LWMsgProtocolRep
{
    uint16_t message_count;
    LWMsgProtocolMessageRep* messages;
} LWMsgProtocolRep;

LWMsgStatus
lwmsg_protocol_get_protocol_rep(
    LWMsgProtocol* prot,
    LWMsgProtocolRep** rep
    );

LWMsgStatus
lwmsg_protocol_add_protocol_rep(
    LWMsgProtocol* prot,
    LWMsgProtocolRep* rep
    );

LWMsgStatus
lwmsg_protocol_is_protocol_rep_compatible(
    LWMsgProtocol* prot,
    LWMsgProtocolRep* rep
    );

void
lwmsg_protocol_free_protocol_rep(
    LWMsgProtocol* prot,
    LWMsgProtocolRep* rep
    );

LWMsgStatus
lwmsg_protocol_print(
    LWMsgProtocol* prot,
    unsigned int indent,
    LWMsgBuffer* buffer
    );

LWMsgStatus
lwmsg_protocol_print_alloc(
    LWMsgProtocol* prot,
    unsigned int indent,
    char** text
    );

extern LWMsgTypeSpec* lwmsg_protocol_rep_spec;

#endif
