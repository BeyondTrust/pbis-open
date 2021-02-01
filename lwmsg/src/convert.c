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
 *        Primitive type conversion API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "config.h"
#include <lwmsg/type.h>
#include <lwmsg/status.h>
#include "util-private.h"
#include "convert-private.h"

#include <stdlib.h>
#include <string.h>

static inline void
swap_memcpy(void* out, const void* in, size_t n)
{
    size_t i;
    unsigned char* in_bytes = (unsigned char*) in;
    unsigned char* out_bytes = (unsigned char*) out;

    for (i = 0; i < n; i++)
    {
        out_bytes[n - 1 - i] = in_bytes[i];
    }
}

LWMsgStatus
lwmsg_convert_integer(
    void* in,
    size_t in_size,
    LWMsgByteOrder in_order,
    void* out,
    size_t out_size,
    LWMsgByteOrder out_order,
    LWMsgSignage signage
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    unsigned char* in_bytes = (unsigned char*) in;
    unsigned char* out_bytes = (unsigned char*) out;

    /* Widening conversion */
    if (in_size <= out_size)
    {
        /* Start of lower order (in_size) bytes in out_bytes */
        unsigned char *out_low_bytes;
        /* Start of higher order (out_size - in_size) bytes in out_bytes */
        unsigned char *out_high_bytes;
        /* Byte that will contain sign bit in out_bytes after copying
           but before sign extension */
        unsigned char *out_sign_byte;

        /* Decide where our lower order, higher order,
           and sign bytes are in out_bytes */
        if (out_order == LWMSG_LITTLE_ENDIAN)
        {
            out_low_bytes = out_bytes;
            out_high_bytes = out_bytes + in_size;
            out_sign_byte = out_low_bytes + in_size - 1;
        }
        else
        {
            out_low_bytes = out_bytes + out_size - in_size;
            out_high_bytes = out_bytes;
            out_sign_byte = out_low_bytes;
        }

        /* Copy into the lower order bytes, swapping order
           if necessary */
        if (in_order == out_order)
        {
            memcpy(out_low_bytes, in_bytes, in_size);
        }
        else
        {
            swap_memcpy(out_low_bytes, in_bytes, in_size);
        }

        /* Perform sign extension or clear high bytes */
        if (signage == LWMSG_SIGNED && *out_sign_byte & 0x80)
        {
            memset(out_high_bytes, 0xFF, out_size - in_size);
        }
        else
        {
            memset(out_high_bytes, 0x00, out_size - in_size);
        }
    }
    /* Truncating conversion */
    else
    {
        /* Lowest order (out_size) bytes of in_bytes */
        unsigned char *in_low_bytes;
        /* Highest order (in_size - out_size) bytes of in_bytes */
        unsigned char *in_high_bytes;
        /* Byte which will contain the sign bit after truncation */
        unsigned char *in_sign_byte;
        /* Expected value of high bytes if no overflow/underflow is to occur */
        unsigned char expected_high_byte;
        unsigned char* i;

        if (in_order == LWMSG_LITTLE_ENDIAN)
        {
            in_low_bytes = in_bytes;
            in_high_bytes = in_bytes + out_size;
            in_sign_byte = in_low_bytes + out_size - 1;
        }
        else
        {
            in_low_bytes = in_bytes + in_size - out_size;
            in_high_bytes = in_bytes;
            in_sign_byte = in_low_bytes;
        }

        /* Decide if conversion will cause overflow/underflow */
        if (signage == LWMSG_SIGNED && *in_sign_byte & 0x80)
        {
            /* Output will end up being negative */
            expected_high_byte = 0xFF;
        }
        else
        {
            /* Output will end up being positive */
            expected_high_byte = 0x00;
        }

        for (i = in_high_bytes; i < (in_order == LWMSG_LITTLE_ENDIAN ? in_bytes + in_size : in_low_bytes); i++)
        {
            if (*i != expected_high_byte)
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_OVERFLOW);
            }
        }

        /* Copy lowest order bytes of in_bytes into out_bytes,
           swapping byte order if necessary */
        if (in_order == out_order)
        {
            memcpy(out_bytes, in_low_bytes, out_size);
        }
        else
        {
            swap_memcpy(out_bytes, in_low_bytes, out_size);
        }
    }

error:

    return status;
}

uint32_t
lwmsg_convert_uint32(
    uint32_t in,
    LWMsgByteOrder in_order,
    LWMsgByteOrder out_order
    )
{
    uint32_t out;

    lwmsg_convert_integer(
        &in,
        sizeof(in),
        in_order,
        &out,
        sizeof(out),
        out_order,
        LWMSG_UNSIGNED);

    return out;
}

uint16_t
lwmsg_convert_uint16(
    uint16_t in,
    LWMsgByteOrder in_order,
    LWMsgByteOrder out_order
    )
{
    uint16_t out;
    
    lwmsg_convert_integer(
        &in,
        sizeof(in),
        in_order,
        &out,
        sizeof(out),
        out_order,
        LWMSG_UNSIGNED);
    
    return out;
}
