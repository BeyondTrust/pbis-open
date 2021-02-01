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
