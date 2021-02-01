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
