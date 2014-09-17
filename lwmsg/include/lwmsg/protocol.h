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
 *        protocol.h
 *
 * Abstract:
 *
 *        Protocol specification and construction API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_PROTOCOL_H__
#define __LWMSG_PROTOCOL_H__

#include <lwmsg/status.h>
#include <lwmsg/context.h>
#include <lwmsg/type.h>
#include <lwmsg/buffer.h>
#include <lwmsg/message.h>

/**
 * @file protocol.h
 * @brief Protocol API
 */

/**
 * @defgroup protocol Protocols
 * @ingroup public
 * @brief Describe messages and message contents
 *
 * Protocols consist of an enumerated set of message tags.
 * Each message tag has an associated marshaller type
 * which describes the layout of the payload for that message.
 * Protocols fully specify the messages available to peers
 * communicating through an association.
 *
 * A protocol object includes one or more protocol specifications,
 * which are simple statically-initialized C arrays.
 */

/*@{*/

/**
 * @brief A protocol object
 *
 * An opaque protocol object suitable for creating associations
 */
typedef struct LWMsgProtocol LWMsgProtocol;

/**
 * @brief Protocol specification structure
 *
 * Defines the messages and payload types available to a protocol.
 * You should initialize a static array of this structure in your source code
 * using #LWMSG_MESSAGE() and #LWMSG_PROTOCOL_END.  The result will be suitable
 * to pass to #lwmsg_protocol_add_protocol_spec().  Consider the following example:
 *
 * @code
 * enum FooMessageType
 * {
 *     FOO_REQUEST_BAR = 1,
 *     FOO_REPLY_BAR = 2,
 *     FOO_REQUEST_BAZ = 3,
 *     FOO_REPLY_BAZ = 4
 * };
 *
 * static LWMsgProtocolSpec foo_spec[] =
 * {
 *     LWMSG_MESSAGE(FOO_REQUEST_BAR, foo_request_bar_spec),
 *     LWMSG_MESSAGE(FOO_REPLY_BAR, foo_reply_bar_spec),
 *     LWMSG_MESSAGE(FOO_REQUEST_BAZ, foo_request_baz_spec),
 *     LWMSG_MESSAGE(FOO_REPLY_BAZ, foo_reply_baz_spec),
 *     LWMSG_PROTOCOL_END
 * };
 * @endcode
 * 
 * This example assumes the existence of the marshaller type specifications 
 * <tt>foo_request_bar_spec</tt>, <tt>foo_request_baz_spec</tt>,
 * <tt>foo_reply_bar_spec</tt>, and <tt>foo_reply_baz_spec</tt>.  See @ref types
 * for more information on specifying marshaller types.
 */
typedef struct LWMsgProtocolSpec
#ifndef DOXYGEN
{
    unsigned int tag;
    LWMsgTypeSpec* type;
    const char* tag_name;
        
}
#endif
const LWMsgProtocolSpec;

/**
 * @brief Get marshaller type by message tag
 *
 * Gets the marshaller type associated with a given message tag.
 * The retrieved type may be passed directly to the marshaller to
 * marshal or unmarshal message payloads of the given type.
 *
 * @param[in] prot the protocol
 * @param[in] tag the message tag
 * @param[out] out_type the marshaller type
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{NOT_FOUND, no such message tag exists in the specified protocol}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_protocol_get_message_type(
    LWMsgProtocol* prot,
    LWMsgTag tag,
    LWMsgTypeSpec** out_type
    );

/**
 * @brief Get name of message tag
 *
 * Gets the symbolic name of the given message tag.
 *
 * @param[in] prot the protocol
 * @param[in] tag the message tag
 * @param[out] name the symbolic name of the tag
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{NOT_FOUND, no such message tag exists in the specified protocol}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_protocol_get_message_name(
    LWMsgProtocol* prot,
    LWMsgTag tag,
    const char** name
    );

/**
 * @brief Create a new protocol object
 *
 * Creates a new protocol object with no known messages.
 * Messages must be added with lwmsg_protocol_add_protocol_spec().
 *
 * @param[in] context a marshalling context, or <tt>NULL</tt> for default settings
 * @param[out] prot the created protocol
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_protocol_new(
    LWMsgContext* context,
    LWMsgProtocol** prot
    );

/**
 * @brief Add messages from a protocol specification
 *
 * Adds all messages in the specified protocol specification
 * to the specified protocol object.  This may be performed
 * multiple times to aggregate several protocol specifications.
 *
 * @param[in,out] prot the protocol object
 * @param[in] spec the protocol specification
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{MALFORMED, an error was detected in the protocol specification or one of the type specifications}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_protocol_add_protocol_spec(
    LWMsgProtocol* prot,
    LWMsgProtocolSpec* spec
    );

/**
 * @brief Delete a protocol object
 *
 * Deletes the specified protocol object.  It is the caller's responsibility
 * to ensure than no users of the object remain.
 *
 * @param[in,out] prot the protocol object to delete
 */
void
lwmsg_protocol_delete(
    LWMsgProtocol* prot
    );

/**
 * @ingroup protocol
 * @brief Specify a message tag and type
 *
 * This macro is used in the construction of protocol
 * specifications.  It declares a message by its
 * integer tag and associated marshaller type
 * specification.
 *
 * @param tag the integer identifier for the message
 * @param spec the marshaller type specifier that describes the message payload
 * @hideinitializer
 */
#define LWMSG_MESSAGE(tag, spec) \
    { (tag), (spec), #tag }

/**
 * @ingroup protocol
 * @brief Mark end of protocol specification
 *
 * This macro marks the end of a protocol specification.  All
 * protocol specifications must end with this macro.
 * @hideinitializer
 */
#define LWMSG_PROTOCOL_END { -1, NULL }

/*@}*/

#endif
