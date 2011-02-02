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
 *        swab.h
 *
 * Abstract:
 *
 *        Swap Bytes for Endian Conversions
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#ifndef __LWBASE_SWAB_H__
#define __LWBASE_SWAB_H__

//
// _LW_SWAB<N> - Swap Bytes for N-bit Values
//

#define _LW_SWAB16(Value) \
    ((((LW_USHORT)(Value) & 0xFF00) >> 8) | \
     (((LW_USHORT)(Value) & 0x00FF) << 8))

#define _LW_SWAB32(Value) \
    ((((LW_ULONG)(Value) & 0xFF000000) >> 24) | \
    (((LW_ULONG)(Value) & 0x00FF0000) >>  8) | \
    (((LW_ULONG)(Value) & 0x0000FF00) <<  8) | \
     (((LW_ULONG)(Value) & 0x000000FF) << 24))

#if 0
#define _LW_SWAB32(Value) \
    (((LW_ULONG)(_LW_SWAB16(((LW_ULONG)(Value) & 0xFFFF0000) >> 16))) | \
     (((LW_ULONG)_LW_SWAB16(((LW_ULONG)(Value) & 0x0000FFFF))) <<  16))
#endif

#define _LW_SWAB64(Value) \
    (((LW_ULONG64)(_LW_SWAB32(((LW_ULONG64)(Value) & 0xFFFFFFFF00000000LL) >> 32))) | \
     (((LW_ULONG64)_LW_SWAB32(((LW_ULONG64)(Value) & 0x00000000FFFFFFFFLL))) << 32))

#if !defined(LW_BIG_ENDIAN) && !defined(LW_LITTLE_ENDIAN)
#error One of LW_BIG_ENDIAN or LW_LITTLE_ENDIAN must be defined to use this header
#endif

#if defined(LW_BIG_ENDIAN)
#define LW_HTOL16(x) _LW_SWAB16(x)
#define LW_HTOL32(x) _LW_SWAB32(x)
#define LW_HTOL64(x) _LW_SWAB64(x)

#define LW_LTOH16(x) _LW_SWAB16(x)
#define LW_LTOH32(x) _LW_SWAB32(x)
#define LW_LTOH64(x) _LW_SWAB64(x)

#define LW_HTOB16(x) (x)
#define LW_HTOB32(x) (x)
#define LW_HTOB64(x) (x)

#define LW_BTOH16(x) (x)
#define LW_BTOH32(x) (x)
#define LW_BTOH64(x) (x)
#else
#define LW_HTOL16(x) (x)
#define LW_HTOL32(x) (x)
#define LW_HTOL64(x) (x)

#define LW_LTOH16(x) (x)
#define LW_LTOH32(x) (x)
#define LW_LTOH64(x) (x)

#define LW_HTOB16(x) _LW_SWAB16(x)
#define LW_HTOB32(x) _LW_SWAB32(x)
#define LW_HTOB64(x) _LW_SWAB64(x)

#define LW_BTOH16(x) _LW_SWAB16(x)
#define LW_BTOH32(x) _LW_SWAB32(x)
#define LW_BTOH64(x) _LW_SWAB64(x)
#endif

// No-ops, but for readability.
#define LW_HTOL8(x) (x)
#define LW_LTOH8(x) (x)
#define LW_HTOB8(x) (x)
#define LW_BTOH8(x) (x)

#define LW_HTOL8_INPLACE(x) ((x) = LW_HTOL8(x))
#define LW_HTOL16_INPLACE(x) ((x) = LW_HTOL16(x))
#define LW_HTOL32_INPLACE(x) ((x) = LW_HTOL32(x))
#define LW_HTOL64_INPLACE(x) ((x) = LW_HTOL64(x))

#define LW_LTOH8_INPLACE(x) ((x) = LW_LTOH8(x))
#define LW_LTOH16_INPLACE(x) ((x) = LW_LTOH16(x))
#define LW_LTOH32_INPLACE(x) ((x) = LW_LTOH32(x))
#define LW_LTOH64_INPLACE(x) ((x) = LW_LTOH64(x))

#define LW_HTOB8_INPLACE(x) ((x) = LW_HTOB8(x))
#define LW_HTOB16_INPLACE(x) ((x) = LW_HTOB16(x))
#define LW_HTOB32_INPLACE(x) ((x) = LW_HTOB32(x))
#define LW_HTOB64_INPLACE(x) ((x) = LW_HTOB64(x))

#define LW_BTOH8_INPLACE(x) ((x) = LW_BTOH8(x))
#define LW_BTOH16_INPLACE(x) ((x) = LW_BTOH16(x))
#define LW_BTOH32_INPLACE(x) ((x) = LW_BTOH32(x))
#define LW_BTOH64_INPLACE(x) ((x) = LW_BTOH64(x))

#endif /* __LWBASE_SWAB_H__ */
