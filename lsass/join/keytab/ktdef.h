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
 *        ktdef.h
 *
 * Abstract:
 *
 *        Kerberos 5 keytab management library
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 * 
 */
#ifndef __KTDEF_H__
#define __KTDEF_H__

typedef enum
{
    KRB5_InMemory_Cache,
    KRB5_File_Cache
} Krb5CacheType;

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef WIN32
#define PATH_SEPARATOR_STR "/"
#else
#define PATH_SEPARATOR_STR "\\"
#endif

#ifndef DWORD_MAX

#ifdef  UINT_MAX
#define DWORD_MAX   UINT_MAX
#else
#define DWORD_MAX   4294967295U
#endif

#endif

#ifndef UINT32_MAX
#define UINT32_MAX UINT_MAX
#endif

#if defined(__sparc__) || defined(__ppc__)

#ifndef uint32_t
#define u_int32_t uint32_t
#endif

#ifndef uint16_t
#define u_int16_t uint16_t
#endif

#ifndef uint8_t
#define u_int8_t  uint8_t
#endif

#endif

#ifndef SOCKET_DEFINED
typedef int             SOCKET;
#define SOCKET_DEFINED 1
#endif

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

#ifndef WIN32

#define BAIL_ON_KT_ERROR(dwError)                 \
    if (dwError) {                                \
        goto error;                               \
    }

#define BAIL_WITH_KT_ERROR(_newerror_)            \
    dwError = (_newerror_);                       \
    BAIL_ON_KT_ERROR(dwError);

#define BAIL_IF_NO_MEMORY(ptr)                    \
    if (ptr == NULL) {                            \
        BAIL_WITH_KT_ERROR(KT_STATUS_OUT_OF_MEMORY); \
    }

#define BAIL_ON_INVALID_STRING(pszParam)          \
    if (IsNullOrEmptyString(pszParam)) {          \
        dwError = KT_STATUS_INVALID_PARAMETER;    \
        BAIL_ON_KT_ERROR(dwError);                \
    }

#define BAIL_ON_INVALID_HANDLE(hParam)            \
    if (hParam == (HANDLE)NULL) {                 \
        dwError = KT_STATUS_INVALID_PARAMETER;    \
        BAIL_ON_KT_ERROR(dwError);                \
    }

#define BAIL_ON_INVALID_POINTER(p)                \
    if (NULL == p) {                              \
        dwError = KT_STATUS_INVALID_PARAMETER;    \
        BAIL_ON_KT_ERROR(dwError);                \
    }


/* 
 * TODO: sort out error reporting
 */

#define BAIL_ON_LDAP_ERROR(lderr)                         \
    if (lderr) {                                          \
        switch (lderr) {                                  \
        default:                                          \
            dwError = KT_STATUS_LDAP_ERROR;               \
        }                                                 \
        goto error;                                       \
    }

#ifdef WIN32

#define BAIL_ON_SEC_ERROR(dwMajorStatus)                        \
    if ((dwMajorStatus!= SEC_E_OK) &&                           \
        (dwMajorStatus != SEC_I_CONTINUE_NEEDED)) {             \
        dwError = KT_STATUS_GSS_CALL_FAILED;                    \
        goto error;                                             \
    }

#else

#define BAIL_ON_SEC_ERROR(dwMajorStatus)                        \
    if ((dwMajorStatus!= GSS_S_COMPLETE) &&                     \
        (dwMajorStatus != GSS_S_CONTINUE_NEEDED)) {             \
        dwError = KT_STATUS_GSS_CALL_FAILED;                    \
        goto error;                                             \
    }

#endif /* WIN32 */

#define BAIL_ON_KRB5_ERROR(ctx, ret)                      \
    if (ret) {                                            \
        switch (ret) {                                    \
        case ENOENT:                                      \
            dwError = ret;                                \
            break;                                        \
        case KRB5_LIBOS_BADPWDMATCH:                      \
            dwError = KT_STATUS_KRB5_PASSWORD_MISMATCH;   \
            break;                                        \
        case KRB5KDC_ERR_KEY_EXP:                         \
            dwError = KT_STATUS_KRB5_PASSWORD_EXPIRED;    \
            break;                                        \
        case KRB5KRB_AP_ERR_SKEW:                         \
            dwError = KT_STATUS_KRB5_CLOCK_SKEW;          \
            break;                                        \
        default:                                          \
            dwError = KT_STATUS_KRB5_ERROR;               \
        }                                                 \
        goto error;                                       \
    }

#endif

#define KT_SAFE_FREE_STRING(str)  \
        do {                      \
           if (str) {             \
              KtFreeString(str);  \
              (str) = NULL;       \
           }                      \
        } while(0);

#define KT_SAFE_CLEAR_FREE_STRING(str)        \
        do {                                  \
           if (str) {                         \
              if (*str) {                     \
                 memset(str, 0, strlen(str)); \
              }                               \
              KtFreeString(str);              \
              (str) = NULL;                   \
           }                                  \
        } while(0);

#define KT_SAFE_FREE_MEMORY(mem)  \
        do {                      \
           if (mem) {             \
              KtFreeMemory(mem);  \
              (mem) = NULL;       \
           }                      \
        } while(0);

#define KT_SAFE_FREE_STRING_ARRAY(ppszArray)                \
        do {                                                \
           if (ppszArray) {                                 \
               KtFreeNullTerminatedStringArray(ppszArray);  \
               (ppszArray) = NULL;                          \
           }                                                \
        } while (0);

#define IsNullOrEmptyString(str) (!(str) || !(*(str)))

#endif /* __KTDEF_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
