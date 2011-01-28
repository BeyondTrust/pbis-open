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
