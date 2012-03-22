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
