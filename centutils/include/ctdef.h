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

#ifndef __CTDEF_H__
#define __CTDEF_H__

#ifndef WIN32
#include <lw/types.h>
#include <lw/attrs.h>
#endif

/*
  Standard integer-related types and constants come from standard system
  headers.

  Standard integer types:
  - {u,}int{8,16,32,64}_t

  Standard MAX/MIN constants:
  - INT{8,16,32,64}_{MIN,MAX}
  - UINT{8,16,32,64}_MAX
*/

#ifndef CT_MAX
#define CT_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef CT_MIN
#define CT_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

typedef struct __DBLBYTE {
    BYTE b1;
    BYTE b2;
} DBLBYTE, *PDBLBYTE;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define GOTO_CLEANUP() \
    do { goto cleanup; } while (0)

#define GOTO_CLEANUP_EE(EE) \
    do { (EE) = __LINE__; goto cleanup; } while (0)

#define GOTO_CLEANUP_ON_DWORD(ceError) \
    do { if (ceError) goto cleanup; } while (0)

#define GOTO_CLEANUP_ON_DWORD_EE(ceError, EE) \
    do { if (ceError) { (EE) = __LINE__; goto cleanup; } } while (0)

/* Deprecated -- please use GOTO_CLEANUP versions */
#define CLEANUP_ON_DWORD(ceError) GOTO_CLEANUP_ON_DWORD(ceError)
#define CLEANUP_ON_DWORD_EE(ceError, EE) GOTO_CLEANUP_ON_DWORD_EE(ceError, EE)

#ifndef WIN32
#define BAIL_ON_CENTERIS_ERROR(__ceError__) \
    do { \
        if ((__ceError__) != 0) { \
            goto error; \
        } \
    } while (0)

#endif

#ifndef IsNullOrEmptyString

#define IsNullOrEmptyString(pszStr)             \
    (pszStr == NULL || *pszStr == '\0')

#endif

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

