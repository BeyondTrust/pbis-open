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
 *        call.h
 *
 * Abstract:
 *
 *        Call handle interface
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#ifndef __LWMSG_CALL_H__
#define __LWMSG_CALL_H__

#include <lwmsg/message.h>
#include <lwmsg/status.h>
#include <lwmsg/context.h>
#include <lwmsg/session.h>

/**
 * @file call.h
 * @brief Call discipline API
 */

/**
 * @defgroup call Call discipline
 * @ingroup public
 * @brief Abstract call mechanism with asynchronous support
 */

/*@{*/

/**
 * @brief Call parameters
 *
 * Represents the parameters or return values of a call
 */
typedef struct LWMsgParams
{
    /**
     * @brief Data tag
     *
     * Indicates type of call/return data according to call protocol
     */
    LWMsgTag tag;
    /**
     * @brief Data values
     *
     * Contains actual call/return data
     */
    void* data;
} LWMsgParams;

/**
 * @brief Call handle
 *
 * Tracks a call through its life cycle.
 */
typedef struct LWMsgCall LWMsgCall;

/**
 * @brief Call direction
 *
 * The direction of a call.
 */
typedef enum LWMsgCallDirection
{
    LWMSG_CALL_INCOMING,
    LWMSG_CALL_OUTGOING
} LWMsgCallDirection;

/**
 * @brief Call completion callback
 *
 * A caller-supplied function which is invoked when an asynchronous
 * call is completed by the callee.  A completion function must not
 * block.
 *
 * @param[in] call the call handle
 * @param[in] status the status code the call completed with
 * @param[in] data a user data pointer
 */
typedef void
(*LWMsgCompleteFunction) (
    LWMsgCall* call,
    LWMsgStatus status,
    void* data
    );

/**
 * @brief Call cancellation function
 *
 * A callee-supplied function which is invoked when an asynchronous
 * operation is cancelled by the caller.  A cancellation function
 * must not block.
 *
 * @param[in] call the call handle
 * @param[in] data a user data pointer
 */
typedef void
(*LWMsgCancelFunction) (
    LWMsgCall* call,
    void* data
    );

/**
 * @brief Destroy parameters
 *
 * Destroys the contents of a parameters structure that was
 * previously filled in by the given call handle.  If the
 * parameters structure contains no data (e.g. tag is
 * #LWMSG_TAG_INVALID), this function will take no action
 * and return #LWMSG_STATUS_SUCCESS.  Therefore, you may always
 * safely use this function on a #LWMsgParams structure that
 * has been correctly initialized (e.g. with #LWMSG_PARAMS_INITIALIZER)
 * to free any data it might contain.
 * 
 * @param[in] call the call handle
 * @param[in,out] params the parameters structure
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{MALFORMED, the parameter data was not well-formed and could not be fully destroyed}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_call_destroy_params(
    LWMsgCall* call,
    LWMsgParams* params
    );

/**
 * @brief Dispatch call
 *
 * Dispatches a call.  If complete is provided, the callee may opt
 * to complete the call asynchronously, in which case it will be
 * invoked at completion time in an arbitrary thread.
 *
 * @param[in,out] call the call handle
 * @param[in] input the input parameters
 * @param[out] output the output parameters
 * @param[in] complete an optional completion callback
 * @param[in] data a user data pointer to be passed to the completion callback
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{PENDING, the call will be completed asynchronously by
 * invoking the provided completion function}
 * @lwmsg_etc{call-specific error}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_call_dispatch(
    LWMsgCall* call,
    const LWMsgParams* input,
    LWMsgParams* output,
    LWMsgCompleteFunction complete,
    void* data
    );

/**
 * @brief Mark call as pending
 *
 * Marks the given call as pending asychronous completion.  This
 * function must only be used by callees.  It signifies the callee's
 * intent to complete the call asynchronously with #lwmsg_call_complete().
 * The callee must also cause #LWMSG_STATUS_PENDING to be returned from
 * #lwmsg_call_dispatch().
 *
 * @param[in,out] call the call handle
 * @param[in] cancel a mandatory call cancellation function
 * @param[in] data a data pointer to pass to the cancellation function
 */
void
lwmsg_call_pend(
    LWMsgCall* call,
    LWMsgCancelFunction cancel,
    void* data
    );

/**
 * @brief Complete a pending call
 *
 * Completes the given call with the provided status code.  This function
 * must only be used by callees, and only after marking the call as pending
 * with #lwmsg_call_pend().
 *
 * @param[in,out] call the call handle
 * @param[in] status the call return status
 */
void
lwmsg_call_complete(
    LWMsgCall* call,
    LWMsgStatus status
    );

/**
 * @brief Cancel a pending call
 *
 * Cancels the given call.  The function must only be used by callers, and
 * only after #lwmsg_call_dispatch() has returned #LWMSG_STATUS_PENDING for the
 * call.  When successfully cancelled, the completion function passed to
 * #lwmsg_call_dispatch() will be invoked with a status of #LWMSG_STATUS_CANCELLED.
 *
 * @param[in,out] call the call handle
 */
void
lwmsg_call_cancel(
    LWMsgCall* call
    );

/**
 * @brief Wait for pending call
 *
 * Waits for the given pending call to complete and returns the status it
 * completed with.
 * @param[in] call the call handle
 * @return the status the call completed with
 */
LWMsgStatus
lwmsg_call_wait(
    LWMsgCall* call
    );

/**
 * @brief Release call handle
 *
 * Releases the given call handle.  This function should be used by the caller
 * after it is finished with the call.  A pending call may be released before
 * it completes.
 *
 * @param[in,out] call the call handle
 */
void
lwmsg_call_release(
    LWMsgCall* call
    );

/**
 * @brief Get session for call
 *
 * Returns the #LWMsgSession associated with the given call.
 * This function may be used by both callers and callees.
 *
 * @param[in] call the call handle
 * @return the session for the call
 */
LWMsgSession*
lwmsg_call_get_session(
    LWMsgCall* call
    );

/**
 * @brief Get call direction
 *
 * Returns whether a call is incoming or outgoing.
 *
 * @param[in] call the call handle
 * @return the call type
 */
LWMsgCallDirection
lwmsg_call_get_direction(
    LWMsgCall* call
    );

/**
 * @brief Set user data
 *
 * Sets an arbitrary user data pointer on a call handle,
 * which may be used for any purpose.
 *
 * @param[in,out] call the call handle
 * @param[in] data the data pointer
 */
void
lwmsg_call_set_user_data(
    LWMsgCall* call,
    void* data
    );

/**
 * @brief Get user data
 *
 * Gets the user data pointer previously set with
 * #lwmsg_call_set_user_data().
 *
 * @param[in] call the call handle
 * @return the user data pointer
 */
void*
lwmsg_call_get_user_data(
    LWMsgCall* call
    );

/**
 * @brief Static initializer for #LWMsgParams
 *
 * A constant which may be used to initialize automatic variables
 * of #LWMsgParams to sane default values
 *
 * @hideinitializer
 */
#define LWMSG_PARAMS_INITIALIZER { LWMSG_TAG_INVALID, NULL }

/*@}*/

#endif
