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
 *        convert.c
 *
 * Abstract:
 *
 *        Primitive type conversion API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_CONVERT_PRIVATE_H__
#define __LWMSG_CONVERT_PRIVATE_H__

#include "config.h"
#include <lwmsg/type.h>
#include <lwmsg/status.h>
#include <inttypes.h>

#ifdef WORDS_BIGENDIAN
#    define LWMSG_NATIVE_ENDIAN LWMSG_BIG_ENDIAN
#else
#    define LWMSG_NATIVE_ENDIAN LWMSG_LITTLE_ENDIAN
#endif

LWMsgStatus
lwmsg_convert_integer(
    void* in,
    size_t in_size,
    LWMsgByteOrder in_order,
    void* out,
    size_t out_size,
    LWMsgByteOrder out_order,
    LWMsgSignage signage
    );

uint32_t
lwmsg_convert_uint32(
    uint32_t in,
    LWMsgByteOrder in_order,
    LWMsgByteOrder out_order
    );

uint16_t
lwmsg_convert_uint16(
    uint16_t in,
    LWMsgByteOrder in_order,
    LWMsgByteOrder out_order
    );

#endif
