/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __CTDEF_H__
#define __CTDEF_H__

#include <lw/types.h>
#include <lw/attrs.h>

/*
  Standard integer-related types and constants come from standard system
  headers.

  Standard integer types:
  - {u,}int{8,16,32,64}_t

  Standard MAX/MIN constants:
  - INT{8,16,32,64}_{MIN,MAX}
  - UINT{8,16,32,64}_MAX
*/

#define CT_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CT_MIN(a, b) (((a) < (b)) ? (a) : (b))

typedef struct __DBLBYTE {
    BYTE b1;
    BYTE b2;
} DBLBYTE, *PDBLBYTE;

#define GOTO_CLEANUP() \
    do { goto cleanup; } while (0)

#define GOTO_CLEANUP_ON_CENTERROR(macError) \
    do { if (macError) goto cleanup; } while (0)

#define GOTO_CLEANUP_ON_CENTERROR_EE(macError, EE) \
    do { if (macError) { (EE) = __LINE__; goto cleanup; } } while (0)

/* Deprecated -- please use GOTO_CLEANUP versions */
#define CLEANUP_ON_CENTERROR(macError) GOTO_CLEANUP_ON_CENTERROR(macError)
#define CLEANUP_ON_CENTERROR_EE(macError, EE) GOTO_CLEANUP_ON_CENTERROR_EE(macError, EE)

#ifndef WIN32
#define BAIL_ON_CENTERIS_ERROR(__macError__) \
    do { \
        if ((__macError__) != 0) { \
            goto error; \
        } \
    } while (0)

#endif

#define IsNullOrEmptyString(pszStr)             \
    (pszStr == NULL || *pszStr == '\0')

#if defined(WORDS_BIGENDIAN)
#define CONVERT_ENDIAN_DWORD(ui32val)           \
    ((ui32val & 0x000000FF) << 24 |             \
     (ui32val & 0x0000FF00) << 8  |             \
     (ui32val & 0x00FF0000) >> 8  |             \
     (ui32val & 0xFF000000) >> 24)

#define CONVERT_ENDIAN_WORD(ui16val)            \
    ((ui16val & 0x00FF) << 8 |                  \
     (ui16val & 0xFF00) >> 8)

#else
#define CONVERT_ENDIAN_DWORD(ui32val) (ui32val)
#define CONVERT_ENDIAN_WORD(ui16val) (ui16val)
#endif

#endif /* __CTDEF_H__ */

