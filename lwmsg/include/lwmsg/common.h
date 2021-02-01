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
 *        common.h
 *
 * Abstract:
 *
 *        Common definitions (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_COMMON_H__
#define __LWMSG_COMMON_H__

#include <inttypes.h>

/**
 * @file common.h Common definitions
 * @brief Common definitions
 */

/**
 * @defgroup public Public APIs
 *
 * These APIs are intended for direct consumption by client applications and
 * encompass the most common use cases.
 */

/**
 * @internal
 * @defgroup private Internal APIs
 *
 * These APIs are used internally within lwmsg, are not publically visible, and
 * are not API or ABI stable.
 */

/**
 * @defgroup common Common definitions
 * @ingroup public
 * @brief Definitions of common types
 *
 * Defines various common types which are used through <tt>LWMsg</tt>.
 */

/* @{ */

/**
 * @brief Boolean type
 *
 * Represents a boolean true/false value
 */
typedef enum LWMsgBool
{
    /** False */
    LWMSG_FALSE = 0,
    /** True */
    LWMSG_TRUE = 1
} LWMsgBool;

/**
 * @brief Endpoint type
 *
 * Represents the type of a client or server endpoint
 */
typedef enum LWMsgEndpointType
{
    LWMSG_ENDPOINT_NONE,
    /** Machine-local (UNIX domain socket) */
    LWMSG_ENDPOINT_LOCAL,
    /** Direct (same process) */
    LWMSG_ENDPOINT_DIRECT,
    /** Socket pair (same user) */
    LWMSG_ENDPOINT_PAIR
} LWMsgEndpointType;

/* @} */

#endif
