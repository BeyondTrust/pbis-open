/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#ifndef __BAIL_H__
#define __BAIL_H__

#include <lw/rtllog.h>

#define BAIL_ON_LW_ERROR(dwError) \
    do { \
        if (dwError) \
        { \
            LW_RTL_LOG_DEBUG("[%s() %s:%d] Error code: %d (symbol: %s)", __FUNCTION__, __FILE__, __LINE__, dwError, LW_SAFE_LOG_STRING(LwWin32ErrorToName(dwError))); \
            goto error; \
        } \
    } while (0)

#define BAIL_ON_LDAP_ERROR(dwError) \
    do { \
        if (dwError) \
        { \
            dwError = LwMapLdapErrorToLwError(dwError); \
            LW_RTL_LOG_DEBUG("[%s() %s:%d] Ldap error code: %d", __FUNCTION__, __FILE__, __LINE__, dwError); \
            goto error; \
        } \
    } while (0)

#define LW_BAIL_ON_INVALID_STRING(pszParam) \
    do { \
        if (LW_IS_NULL_OR_EMPTY_STR(pszParam)) \
        { \
           dwError = LW_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LW_ERROR(dwError); \
        } \
    } while (0)

#define LW_BAIL_ON_INVALID_HANDLE(hParam) \
    do { \
        if (!hParam) \
        { \
           dwError = LW_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LW_ERROR(dwError); \
        } \
    } while (0)

#define LW_BAIL_ON_INVALID_POINTER(p) \
    do { \
        if (!p) \
        { \
           dwError = LW_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LW_ERROR(dwError); \
        } \
    } while (0)

#define BAIL_ON_KRB_ERROR(ctx, ret) \
    do { \
        if (ret) \
        { \
           (dwError) = LwTranslateKrb5Error(ctx, ret, __FUNCTION__, __FILE__, __LINE__); \
           goto error; \
        } \
    } while (0)

#endif /* __BAIL_H__ */
