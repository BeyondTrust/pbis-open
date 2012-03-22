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
 *        call-private.h
 *
 * Abstract:
 *
 *        Call handle interface (private)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#ifndef __LWMSG_CALL_PRIVATE_H__
#define __LWMSG_CALL_PRIVATE_H__

#include <lwmsg/call.h>
#include <stdlib.h>

/**
 * @file call-private.h
 * @brief Call API (INTERNAL)
 * @internal
 */

/**
 * @internal
 * @defgroup call_private Call handles
 * @ingroup private
 * @brief Call handle implementation
 */

/*@{*/

/**
 * @internal
 * @brief Call handle vtable
 *
 * Virtual function table for #LWMsgCall structures.
 *
 */
typedef struct LWMsgCallClass
{ 
    /**
     * @brief Release call handle (virtual)
     *
     * Implements the exact semantics of #lwmsg_call_release().
     *
     * This function may be NULL for call handles only intended
     * for use by callees.
     */
    void (*release)(
        LWMsgCall* call
        );
    /**
     * @brief Dispatch call (virtual)
     *
     * Implements the exact semantics of #lwmsg_call_dispatch().
     *
     * This function may be NULL for call handles only intended
     * for use by callees.
     */
    LWMsgStatus (*dispatch)(
        LWMsgCall* call,
        const LWMsgParams* in,
        LWMsgParams* out,
        LWMsgCompleteFunction complete,
        void* data
        );
    /**
     * @brief Mark call as pending (virtual)
     *
     * Implements the exact semantics of #lwmsg_call_pend().
     *
     * This function may be NULL for call handles only intended
     * for use by callers.
     */
    LWMsgStatus (*pend)(
        LWMsgCall* call,
        LWMsgCancelFunction cancel,
        void* data
        );
    /**
     * @brief Mark call as complete (virtual)
     *
     * Implements the exact semantics of #lwmsg_call_complete().
     *
     * This function may be NULL for call handles only intended
     * for use by callers.
     */
    LWMsgStatus (*complete)(
        LWMsgCall* call,
        LWMsgStatus status
        );
    /**
     * @brief Cancel call (virtual)
     *
     * Implements the exact semantics of #lwmsg_call_cancel().
     *
     * This function may be NULL for call handles only intended
     * for use by callees.
     */
    LWMsgStatus (*cancel)(
        LWMsgCall* call
        );

    LWMsgStatus (*wait)(
        LWMsgCall* call
        );
    /**
     * @brief Destroy call parameters (virtual)
     *
     * Implements the exact semantics of #lwmsg_call_destroy_params().
     *
     * This function may be NULL for call handles only intended
     * for use by callees.
     */
    LWMsgStatus (*destroy_params)(
        LWMsgCall* call,
        LWMsgParams* params
        );
    /**
     * @brief Get session for call (virtual)
     *
     * Implements the exact semantics of #lwmsg_call_get_session().
     *
     * This function must be present for all call handles
     */
    LWMsgSession*
    (*get_session)(
        LWMsgCall* call
        );
} LWMsgCallClass;

/**
 * @internal
 * @brief Call handle representation
 *
 * The internal representation of a call handle.  To implement
 * a new type of call handle, create a call handle structure with
 * a #LWMsgCall as the first member and fill in its vtbl field.
 */
struct LWMsgCall
{
    /**
     * @brief Virtual function table pointer
     *
     * A pointer to the virtual function table that implements
     * the call handle operations
     */
    LWMsgCallClass* vtbl;
    unsigned is_outgoing:1;
    void* user_data;
};

/**
 * @internal
 * @brief Cast to call handle
 *
 * Convenience macro to cast to #LWMsgCall *
 */
#define LWMSG_CALL(obj) ((LWMsgCall*) (obj))

/*@}*/

#endif
