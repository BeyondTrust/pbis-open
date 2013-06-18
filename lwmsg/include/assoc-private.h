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
 *        assoc-private.h
 *
 * Abstract:
 *
 *        Association API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_ASSOC_PRIVATE_H__
#define __LWMSG_ASSOC_PRIVATE_H__

#include <lwmsg/assoc.h>
#include "session-private.h"
#include "context-private.h"
#include "call-private.h"

/**
 * @internal
 * @brief Association implementation structure
 *
 * Describes the concrete implementation of an association.
 */
typedef struct LWMsgAssocClass
{
    /** 
     * @ingroup assoc_impl
     * @brief Constructor method
     *
     * This method performs initialization of implementation-specific state for a newly-created association.
     *
     * @param[in] assoc the association being constructed
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_memory
     * @lwmsg_endstatus
     */
    LWMsgStatus (*construct)(LWMsgAssoc* assoc);
    /**
     * @ingroup assoc_impl
     * @brief Destructor
     *
     * This method performs teardown of private state for an association which is being deleted.
     *
     * @param[in] assoc the association being deleted
     */
    void (*destruct)(LWMsgAssoc* assoc);
    /**
     * @ingroup assoc_impl
     * @brief Message send method
     *
     * This method performs a message send operation.
     *
     * @param[in] assoc the association
     * @param[in] message the message to send
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{TIMEOUT, operation timed out}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*send_msg)(LWMsgAssoc* assoc, LWMsgMessage* message);
    /**
     * @ingroup assoc_impl
     * @brief Message receive method
     *
     * This method performs a message receive operation.
     *
     * @param[in] assoc the association
     * @param[out] message the received message
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{TIMEOUT, operation timed out}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*recv_msg)(LWMsgAssoc* assoc, LWMsgMessage* message);
    /**
     * @ingroup assoc_impl
     * @brief Association close method
     *
     * This method performs logic to shut down an association, e.g.
     * notifying the remote peer.  As opposed to the destructor, this method may
     * fail or time out without successfully completing.
     *
     * @param[in] assoc the association
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{TIMEOUT, operation timed out}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*close)(LWMsgAssoc* assoc);
    /**
     * @ingroup assoc_impl
     * @brief Association reset method
     *
     * This method performs logic to reset an association, e.g.
     * notifying the remote peer and resetting internal state.
     *
     * @param[in] assoc the association
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{TIMEOUT, operation timed out}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*reset)(LWMsgAssoc* assoc);
    LWMsgStatus (*finish)(LWMsgAssoc* assoc, LWMsgMessage** message);
    LWMsgStatus (*set_nonblock)(LWMsgAssoc* assoc, LWMsgBool nonblock);

    /**
     * @ingroup assoc_impl
     * @brief Peer session manager ID access method
     *
     * This method retrieves the session handle for the association.
     *
     * @param[in] assoc the association
     * @param[out] session the retrieved session handle
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_STATE, the session handle in not available in the current state}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*get_session)(LWMsgAssoc* assoc, LWMsgSession** session);
    /**
     * @ingroup assoc_impl
     * @brief Get association state
     *
     * This method returns the current state of the association.
     *
     * @param[in] assoc the association
     * @return the current state
     */
    LWMsgAssocState (*get_state)(LWMsgAssoc* assoc);

    /**
     * @ingroup assoc_impl
     * @brief Set timeout
     * 
     * This method sets a timeout that should be used for subsequent operations
     *
     * @param[in] assoc the association
     * @param[in] type the type of timeout
     * @param[in] value the value of the timeout, or NULL for no timeout
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{UNSUPPORTED, the association does not support the specified timeout type}
     * @lwmsg_etc{implementation-specific error}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*set_timeout)(
        LWMsgAssoc* assoc,
        LWMsgTimeout type,
        LWMsgTime* value
        );

    LWMsgStatus
    (*connect_peer)(
        LWMsgAssoc* assoc,
        LWMsgSession* session
        );

    LWMsgStatus
    (*accept_peer)(
        LWMsgAssoc* assoc,
        LWMsgSession* session
        );
} LWMsgAssocClass;

typedef struct AssocCall
{
    LWMsgCall base;
    LWMsgBool in_use;
} AssocCall;

struct LWMsgAssoc
{
    LWMsgContext context;
    LWMsgAssocClass* aclass;
    LWMsgProtocol* prot;
    void* construct_data;
    AssocCall call;
};

#define ASSOC_RAISE_ERROR(_assoc_, ...) BAIL_ON_ERROR(status = RAISE(&(_assoc_)->context, __VA_ARGS__))

/**
 * @ingroup assoc_impl
 * @brief Create a new association
 *
 * Creates a new association with the specified implementation and protocol.
 * 
 * @param[in] context an optional context
 * @param[in] prot the protocol understood by the association
 * @param[in] aclass the implementation structure for the new association
 * @param[out] assoc the created association
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_new(
    const LWMsgContext* context,
    LWMsgProtocol* prot,
    LWMsgAssocClass* aclass,
    size_t size,
    LWMsgAssoc** assoc
    );

LWMsgStatus
lwmsg_assoc_call_init(
    AssocCall* call
    );

LWMsgStatus
lwmsg_assoc_session_new(
    LWMsgAssoc* assoc,
    LWMsgSession** out_session
    );

#endif
