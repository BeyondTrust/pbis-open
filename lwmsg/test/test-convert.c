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
 *        test-convert.c
 *
 * Abstract:
 *
 *        Primitive type conversion unit tests
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#include <moonunit/interface.h>

#include "convert-private.h"
#include "type-private.h"

MU_TEST(convert, neu16_to_neu32)
{
    uint16_t in = 42;
    uint32_t out;

    lwmsg_convert_integer(&in, sizeof(in), LWMSG_NATIVE_ENDIAN,
                          &out, sizeof(out), LWMSG_NATIVE_ENDIAN,
                          LWMSG_UNSIGNED);

    MU_ASSERT(in == out);
}

MU_TEST(convert, nes16_to_nes32)
{
    uint16_t in = -42;
    uint32_t out;

    lwmsg_convert_integer(&in, sizeof(in), LWMSG_NATIVE_ENDIAN,
                          &out, sizeof(out), LWMSG_NATIVE_ENDIAN,
                          LWMSG_UNSIGNED);

    MU_ASSERT(in == out);
}

MU_TEST(convert, nes16_to_beu32)
{
    int16_t in = -42;
    uint32_t out;
    unsigned char* out_bytes = (unsigned char*) &out;

    lwmsg_convert_integer(&in, sizeof(in), LWMSG_NATIVE_ENDIAN,
                          &out, sizeof(out), LWMSG_BIG_ENDIAN,
                          LWMSG_UNSIGNED);
    
    MU_ASSERT(out_bytes[0] == 0x00 &&
              out_bytes[1] == 0x00 &&
              out_bytes[2] == 0xff &&
              out_bytes[3] == 0xd6);
}

MU_TEST(convert, leu32_to_nes16_overflow)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    uint32_t in;
    int16_t out;
    unsigned char* in_bytes = (unsigned char*) &in;

    in_bytes[0] = 0xd6;
    in_bytes[1] = 0xff;
    in_bytes[2] = 0x00;
    in_bytes[3] = 0x00;

    status = lwmsg_convert_integer(&in, sizeof(in), LWMSG_LITTLE_ENDIAN,
                                   &out, sizeof(out), LWMSG_NATIVE_ENDIAN,
                                   LWMSG_SIGNED);
    
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, status, LWMSG_STATUS_OVERFLOW);
}

MU_TEST(convert, beu8_to_nes32)
{
    uint8_t in;
    int32_t out = 0xFFFFFF;
    unsigned char* in_bytes = (unsigned char*) &in;

    in_bytes[0] = 0x02;

    lwmsg_convert_integer(&in, sizeof(in), LWMSG_BIG_ENDIAN,
                          &out, sizeof(out), LWMSG_NATIVE_ENDIAN,
                          LWMSG_SIGNED);

    MU_ASSERT(out == 2);
}

MU_TEST(convert, nes16_to_nes8_overflow)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int16_t in = 128;
    int8_t out = 0;

    status = lwmsg_convert_integer(&in, sizeof(in), LWMSG_NATIVE_ENDIAN,
                                   &out, sizeof(out), LWMSG_NATIVE_ENDIAN,
                                   LWMSG_SIGNED);
    
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, status, LWMSG_STATUS_OVERFLOW);
}

MU_TEST(convert, nes16_to_nes8_overflow2)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int16_t in = -129;
    int8_t out = 0;

    status = lwmsg_convert_integer(&in, sizeof(in), LWMSG_NATIVE_ENDIAN,
                                   &out, sizeof(out), LWMSG_NATIVE_ENDIAN,
                                   LWMSG_SIGNED);
    
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, status, LWMSG_STATUS_OVERFLOW);
}
