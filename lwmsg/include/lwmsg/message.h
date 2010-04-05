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
 *        message.h
 *
 * Abstract:
 *
 *        Message structure and functions
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_MESSAGE_H__
#define __LWMSG_MESSAGE_H__

#include <lwmsg/status.h>
#include <lwmsg/common.h>

/**
 * @file message.h
 * @brief Message structure and functions
 */

/**
 * @brief Message tag
 *
 * Identifies the type of data contained within a message
 * in the context of a particular protocol.  Values less
 * than 0 are reserved for internal use by LWMsg.
 */
typedef int16_t LWMsgTag;

/**
 * @brief Message cookie
 *
 * Allows "request" and "response" messages to be
 * correlated when multiple requests might be outstanding
 * simultaneously.
 */
typedef uint16_t LWMsgCookie;

typedef enum LWMsgMessageFlags
{
    LWMSG_MESSAGE_FLAG_CONTROL   = 0x1,
    LWMSG_MESSAGE_FLAG_SYNTHETIC = 0x2,
    LWMSG_MESSAGE_FLAG_REPLY     = 0x4
} LWMsgMessageFlags;

/**
 * @brief Invalid message tag
 *
 * A message with this tag is invalid and contains
 * no data.
 * @hideinitializer
 */
#define LWMSG_TAG_INVALID ((LWMsgTag) -1)

/**
 * @brief Message structure
 *
 * Encapsulates all the elements of a message
 * in a single structure.
 */
typedef struct LWMsgMessage
{
    LWMsgMessageFlags flags;
    /** 
     * @brief Status code
     *
     * Indicates the overall success of the message.
     */
    LWMsgStatus status;
    /**
     * @brief Cookie
     *
     * An arbitrary cookie value.  Primarily used for correlating
     * request-response message pairs.
     */
    LWMsgCookie cookie;
    /** 
     * @brief Tag
     *
     * Indicates the meaning and data type of the message.  A given
     * tag is meaningful in the context of a given #LWMsgProtocol
     *
     */
    LWMsgTag tag;
#ifdef LWMSG_DISABLE_DEPRECATED
    void* data;
#else
#ifndef DOXYGEN
    union
    {
        void* object;
#endif
        /**
         * @brief Data payload
         * 
         * The data content of the message.  Its type is determined
         * by the tag and the associated #LWMsgProtocol.
         */
        void* data;
#ifndef DOXYGEN
    };
#endif
#endif
#ifndef DOXYGEN
    unsigned long reserved1;
#endif
} LWMsgMessage;


/**
 * @brief Message static initializer
 *
 * An #LWMsgMessage structure may be statically initialized
 * with this value in lieu of explicit initialization of the
 * various fields.
 * @hideinitializer
 */
#ifdef LWMSG_DISABLE_DEPRECATED
#define LWMSG_MESSAGE_INITIALIZER \
    {0, LWMSG_STATUS_SUCCESS, 0, LWMSG_TAG_INVALID, NULL, 0}
#else
#define LWMSG_MESSAGE_INITIALIZER \
    {0, LWMSG_STATUS_SUCCESS, 0, LWMSG_TAG_INVALID, {.data = NULL}, 0}
#endif

/**
 * @brief Initialize an #LWMsgMessage
 *
 * Initializes an #LWMsgMessage structure to reasonable default values:
 * 
 * - Status is #LWMSG_STATUS_SUCCESS
 * - Cookie is 0
 * - Tag is #LWMSG_TAG_INVALID
 * - Data is NULL
 *
 * @param message the message to initialize
 */
void
lwmsg_message_init(
    LWMsgMessage* message
    );

#endif /* __LWMSG_MESSAGE_H__ */
