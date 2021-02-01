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
 *        buffer.h
 *
 * Abstract:
 *
 *        Marshalling buffer API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_BUFFER_H__
#define __LWMSG_BUFFER_H__

#include <stdlib.h>

#include <lwmsg/status.h>

/**
 * @file buffer.h
 * @brief Generic buffer definition
 */

/**
 * @defgroup buffer Data buffers
 * @ingroup public
 * @brief Semi-abstract buffer interface
 */

/*@{*/

/**
 * @brief Generic buffer
 *
 * This structure comprises a generic buffer used as
 * as a source or destination for data streams when performing
 * various operations such as marshalling, printing, etc.  It
 * contains pointers which delimit the start, end, and current
 * position within a contiguous block of memory.  It also
 * contains an optional wrap callback function which is invoked when
 * the cursor reaches the end of the buffer.  This function may
 * reset, refill, or resize the buffer so that the operation
 * can continue.
 */
typedef struct LWMsgBuffer
{
    /** @brief Pointer to base of memory block */
    unsigned char* base;
    /** @brief Pointer one byte past the end of the memory block */
    unsigned char* end;
    /** @brief Pointer to current position in the buffer */
    unsigned char* cursor;
    /**
     * @brief Buffer wrap callback
     * 
     * This optional callback is invoked when the end of the buffer is
     * reached.  It may arbitrarily modify the base, end, and cursor
     * pointers, so long as the condition (end - cursor) >= 1 holds --
     * that is, there must be at least one unused or unread byte in the
     * buffer.
     *
     * The needed parameter specifies the number of bytes the marshaller
     * is immediately attempting to read or write, but it is merely a
     * suggestion.  In the interest of amortizing calls to this function,
     * it is better to make the usable portion of the buffer (end - cursor)
     * a large as possible on each call.  It is safe to return with
     * (end - cursor) < needed, but this is not recommended.
     *
     * A needed value of 0 indicates that the operation in question
     * (e.g. #lwmsg_data_marshal()) is complete, and is an opportunity
     * to perform any final cleanup (e.g. flushing the partially filled
     * buffer to the underlying data stream).
     *
     * @param[in,out] buffer the buffer structure
     * @param[in] needed the number of bytes immediately needed, or 0 if finished
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_memory
     * @lwmsg_etc{implementation-specific error}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*wrap) (struct LWMsgBuffer* buffer, size_t needed);
    /** @brief User data pointer */
    void* data;
} LWMsgBuffer;

/**
 * @brief Print into buffer
 *
 * Prints into an #LWMsgBuffer according to the specified format string and
 * parameters, calling the buffer's wrap function as needed.
 *
 * @param[in,out] buffer the buffer to print into
 * @param[in] fmt the format string
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{EOF, end of buffer reached}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_buffer_print(
    LWMsgBuffer* buffer,
    const char* fmt,
    ...
    );

/*@}*/

#endif
