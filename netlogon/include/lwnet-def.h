/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwnet-def.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        Common definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __LWNET_DEF_H__
#define __LWNET_DEF_H__

#include <lw/winerror.h>

#ifndef WIN32
#define PATH_SEPARATOR_STR "/"
#else
#define PATH_SEPARATOR_STR "\\"
#endif

#ifndef WIN32

#define BAIL_ON_LWNET_ERROR(dwError) \
    if (dwError) {                    \
       LWNET_LOG_DEBUG("Error: %d", dwError); \
       goto error;                    \
    }

#endif

#define LWNET_SAFE_FREE_STRING(str) \
        do {                         \
           if (str) {                \
              LWNetFreeString(str); \
              (str) = NULL;          \
           }                         \
        } while(0);

#define LWNET_SAFE_CLEAR_FREE_STRING(str)    \
        do {                                  \
           if (str) {                         \
              if (*str) {                     \
                 memset(str, 0, strlen(str)); \
              }                               \
              LWNetFreeString(str);           \
              (str) = NULL;                   \
           }                                  \
        } while(0);

#define LWNET_SAFE_FREE_MEMORY(mem) \
        do {                        \
           if (mem) {               \
              LWNetFreeMemory(mem); \
              (mem) = NULL;         \
           }                        \
        } while(0);

#define LWNET_SAFE_FREE_STRING_ARRAY(ppszArray)               \
        do {                                                  \
           if (ppszArray) {                                   \
               LWNetFreeNullTerminatedStringArray(ppszArray); \
               (ppszArray) = NULL;                            \
           }                                                  \
        } while (0);

#define IsNullOrEmptyString(str) (!(str) || !(*(str)))

#define LWNET_SERVER_FILENAME    ".netlogond"

#define CT_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CT_MIN(a, b) (((a) < (b)) ? (a) : (b))

#define CT_PTR_OFFSET(BasePointer, Pointer) \
    ((int)((char*)(Pointer) - (char*)(BasePointer)))

#define CT_PTR_ADD(Pointer, Offset) \
    ((char*)(Pointer) + Offset)

#define CT_FIELD_OFFSET(Type, Field) \
    ((size_t)(&(((Type*)(0))->Field)))

#define CT_FIELD_RECORD(Pointer, Type, Field) \
    ((Type*)CT_PTR_ADD(Pointer, -((ssize_t)CT_FIELD_OFFSET(Type, Field))))

#define CT_ARRAY_SIZE(StaticArray) \
    (sizeof(StaticArray)/sizeof((StaticArray)[0]))

#ifndef LW_ENDIAN_SWAP16

#define LW_ENDIAN_SWAP16(wX)                     \
        ((((UINT16)(wX) & 0xFF00) >> 8) |        \
         (((UINT16)(wX) & 0x00FF) << 8))

#endif

#ifndef LW_ENDIAN_SWAP32

#define LW_ENDIAN_SWAP32(dwX)                    \
        ((((UINT32)(dwX) & 0xFF000000L) >> 24) | \
         (((UINT32)(dwX) & 0x00FF0000L) >>  8) | \
         (((UINT32)(dwX) & 0x0000FF00L) <<  8) | \
         (((UINT32)(dwX) & 0x000000FFL) << 24))

#endif

#ifndef LW_ENDIAN_SWAP64

#define LW_ENDIAN_SWAP64(llX)         \
   (((UINT64)(LW_ENDIAN_SWAP32(((UINT64)(llX) & 0xFFFFFFFF00000000LL) >> 32))) | \
   (((UINT64)LW_ENDIAN_SWAP32(((UINT64)(llX) & 0x00000000FFFFFFFFLL))) << 32))

#endif

#define LWNET_MODE_BITS_URWX_GRX_ORX (S_IRWXU | S_IRGRP | S_IXGRP |S_IROTH | S_IXOTH)
#define LWNET_MODE_BITS_URW_GR_OR    (S_IRUSR | S_IWUSR | S_IRGRP |S_IROTH)

#define LWNET_SECONDS_IN_MINUTE (60)
#define LWNET_SECONDS_IN_HOUR   (60 * LWNET_SECONDS_IN_MINUTE)
#define LWNET_SECONDS_IN_DAY    (24 * LWNET_SECONDS_IN_HOUR)

#define LWNET_MILLISECONDS_IN_SECOND (1000)
#define LWNET_MICROSECONDS_IN_MILLISECOND (1000)
#define LWNET_MICROSECONDS_IN_SECOND ((LWNET_MICROSECONDS_IN_MILLISECOND)*(LWNET_MILLISECONDS_IN_SECOND))
#define LWNET_NANOSECONDS_IN_MILLISECOND (1000 * LWNET_MICROSECONDS_IN_MILLISECOND);

#define BAIL_ON_INVALID_STRING(pszParam)             \
        if (IsNullOrEmptyString(pszParam)) {         \
           dwError = ERROR_INVALID_PARAMETER; \
           BAIL_ON_LWNET_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_HANDLE(hParam)               \
        if (hParam == (HANDLE)NULL) {                \
           dwError = ERROR_INVALID_PARAMETER; \
           BAIL_ON_LWNET_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_POINTER(p)                   \
        if (NULL == p) {                             \
           dwError = ERROR_INVALID_PARAMETER; \
           BAIL_ON_LWNET_ERROR(dwError);            \
        }

#endif /* __LWNET_DEF_H__ */

