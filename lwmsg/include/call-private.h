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
