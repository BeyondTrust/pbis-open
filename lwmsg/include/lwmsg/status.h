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
 *        status.h
 *
 * Abstract:
 *
 *        Status codes (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_STATUS_H__
#define __LWMSG_STATUS_H__

/**
 * @file status.h
 * @brief Status codes and support API
 */

/**
 * @defgroup status Status codes
 * @ingroup public
 * @brief Common status codes used throughout lwmsg
 *
 * lwmsg uses a common set of status codes to indicate errors
 * or exceptional conditions in its functions.
 */

/**
 * @ingroup status
 * @brief A status code
 */
typedef enum
{
    /** Success 
     * @hideinitializer
     */
    LWMSG_STATUS_SUCCESS = 0,
    /** Generic error
     * @hideinitializer
     */
    LWMSG_STATUS_ERROR = 1,
    /** Call is retriable
     * @hideinitializer
     */
    LWMSG_STATUS_AGAIN = 2,
    /** Out of memory
     * @hideinitializer
     */
    LWMSG_STATUS_MEMORY = 3,
    /** Malformed data detected
     * @hideinitializer
     */
    LWMSG_STATUS_MALFORMED = 4,
    /** End of file or stream
     * @hideinitializer
     */
    LWMSG_STATUS_EOF = 5,
    /** Requested item not found
     * @hideinitializer
     */
    LWMSG_STATUS_NOT_FOUND = 6,   
    /** Not yet implemented
     * @hideinitializer
     */
    LWMSG_STATUS_UNIMPLEMENTED = 7,
    /** Invalid parameter
     * @hideinitializer
     */
    LWMSG_STATUS_INVALID_PARAMETER = 8,
    /** Arithmetic overflow
     * @hideinitializer
     */
    LWMSG_STATUS_OVERFLOW = 9,
    /** Arithmetic underflow
     * @hideinitializer
     */
    LWMSG_STATUS_UNDERFLOW = 10,
    /** Unexpected system error
     * @hideinitializer
     */
    LWMSG_STATUS_SYSTEM = 11,
    /** Operation timed out
     * @hideinitializer
     */
    LWMSG_STATUS_TIMEOUT = 12,
    /** Security violation
     * @hideinitializer
     */
    LWMSG_STATUS_SECURITY = 13,
    /** Operation canceled
     * @hideinitializer
     */
    LWMSG_STATUS_CANCELLED = 14,
    /**
     * File not found
     * @hideinitializer
     */
    LWMSG_STATUS_FILE_NOT_FOUND = 15,
    /**
     * Remote server not listening
     * @hideinitializer
     */
    LWMSG_STATUS_CONNECTION_REFUSED = 16,
    /**
     * The requested operation is undefined in the current state
     * @hideinitializer
     */
    LWMSG_STATUS_INVALID_STATE = 17,
    /**
     * Peer reset association
     * @hideinitializer
     */
    LWMSG_STATUS_PEER_RESET = 18,
    /**
     * Peer closed association
     * @hideinitializer
     */
    LWMSG_STATUS_PEER_CLOSE = 19,
    /**
     * Peer aborted association
     * @hideinitializer
     */
    LWMSG_STATUS_PEER_ABORT = 20,
    /**
     * Session with peer was lost
     * @hideinitializer
     */
    LWMSG_STATUS_SESSION_LOST = 21,
    /**
     * Unsupported operation
     * @hideinitializer
     */
    LWMSG_STATUS_UNSUPPORTED = 22,
    /**
     * Invalid handle
     * @hideinitializer
     */
    LWMSG_STATUS_INVALID_HANDLE = 23,
    /**
     * A conflicting operation is already in progress
     * @hideinitializer
     */
    LWMSG_STATUS_BUSY = 24,
    /**
     * Operation is pending completion
     * @hideinitializer
     */
    LWMSG_STATUS_PENDING = 25,
    /**
     * Internal error
     * @hideinitializer
     */
    LWMSG_STATUS_INTERNAL = 26,
    /** 
     * System resource limit encountered
     * @hideinitializer
     */
    LWMSG_STATUS_RESOURCE_LIMIT = 27,
    /**
     * Buffer was not large enough
     * @hideinitializer
     */
    LWMSG_STATUS_BUFFER_TOO_SMALL = 28,
#ifndef DOXYGEN
    LWMSG_STATUS_COUNT
#endif
} LWMsgStatus;

#endif
