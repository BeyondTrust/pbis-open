/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        rsys-def.h
 *
 * Abstract:
 *
 *        Reaper for syslog
 *
 *        Common definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __RSYS_DEF_H__
#define __RSYS_DEF_H__

#ifndef WIN32

#define BAIL_ON_RSYS_ERROR(dwError) \
    if (dwError) {                    \
       RSYS_LOG_DEBUG("Error in %s at %s:%d [code: %d]", __FUNCTION__, __FILE__, __LINE__, dwError); \
       goto error;                    \
    }

#endif

#define NO_BAIL_ON_RSYS_ERROR(str) \
    if (dwError) {                    \
       RSYS_LOG_DEBUG("Error in %s at %s:%d [%s]", __FUNCTION__, __FILE__, __LINE__, str); \
    }

#define BAIL_ON_NON_LWREG_ERROR(dwError) \
        if (!(40700 <= dwError && dwError <= 41200)) {  \
           BAIL_ON_RSYS_ERROR(dwError);            \
        }


#define RSYS_SAFE_CLEAR_FREE_STRING(str)    \
        do {                                  \
           if (str) {                         \
              if (*str) {                     \
                 memset(str, 0, strlen(str)); \
              }                               \
              RtlCStringFree(str);           \
              (str) = NULL;                   \
           }                                  \
        } while(0);

#define RSYS_SAFE_FREE_MEMORY(mem) \
        do {                        \
           if (mem) {               \
              RtlMemoryFree(mem); \
              (mem) = NULL;         \
           }                        \
        } while(0);

#define RSYS_SAFE_FREE_STRING_ARRAY(ppszArray)               \
        do {                                                  \
           if (ppszArray) {                                   \
               RSysFreeNullTerminatedStringArray(ppszArray); \
               (ppszArray) = NULL;                            \
           }                                                  \
        } while (0);

#define IsNullOrEmptyString(str) (!(str) || !(*(str)))

#define RSYS_SERVER_FILENAME    ".reapsysld"

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

#if defined(HAVE_SOCKLEN_T) && defined(GETSOCKNAME_TAKES_SOCKLEN_T)
#    define SOCKLEN_T socklen_t
#else
#    define SOCKLEN_T int
#endif

#define RSYS_MODE_BITS_URWX_GRX_ORX (S_IRWXU | S_IRGRP | S_IXGRP |S_IROTH | S_IXOTH)
#define RSYS_MODE_BITS_URW_GR_OR    (S_IRUSR | S_IWUSR | S_IRGRP |S_IROTH)

#define RSYS_SECONDS_IN_MINUTE (60)
#define RSYS_SECONDS_IN_HOUR   (60 * RSYS_SECONDS_IN_MINUTE)
#define RSYS_SECONDS_IN_DAY    (24 * RSYS_SECONDS_IN_HOUR)

#define RSYS_MILLISECONDS_IN_SECOND (1000)
#define RSYS_MICROSECONDS_IN_MILLISECOND (1000)
#define RSYS_MICROSECONDS_IN_SECOND ((RSYS_MICROSECONDS_IN_MILLISECOND)*(RSYS_MILLISECONDS_IN_SECOND))
#define RSYS_NANOSECONDS_IN_MILLISECOND (1000 * RSYS_MICROSECONDS_IN_MILLISECOND);

#define BAIL_ON_INVALID_STRING(pszParam)             \
        if (IsNullOrEmptyString(pszParam)) {         \
           dwError = EINVAL; \
           BAIL_ON_RSYS_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_HANDLE(hParam)               \
        if (hParam == (HANDLE)NULL) {                \
           dwError = EINVAL; \
           BAIL_ON_RSYS_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_POINTER(p)                   \
        if (NULL == p) {                             \
           dwError = EINVAL; \
           BAIL_ON_RSYS_ERROR(dwError);            \
        }

#endif /* __RSYS_DEF_H__ */

