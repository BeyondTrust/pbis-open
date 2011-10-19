/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#ifndef __DEFS_H__
#define __DEFS_H__

typedef gss_ctx_id_t CtxtHandle, *PCtxtHandle;

#define BAIL_ON_SEC_ERROR(dwMajorStatus) \
    if ((dwMajorStatus != GSS_S_COMPLETE)\
            && (dwMajorStatus != GSS_S_CONTINUE_NEEDED)) {\
        goto sec_error; \
    }

#define BAIL_ON_LWIO_KRB_ERROR(ctx, ret)                               \
    if (ret) {                                                        \
        if (ctx)  {                                                   \
            PCSTR pszKrb5Error = krb5_get_error_message(ctx, ret);    \
            if (pszKrb5Error) {                                       \
                LWIO_LOG_ERROR("KRB5 Error at %s:%d: %s",              \
                        __FILE__,                                     \
                        __LINE__,                                     \
                        pszKrb5Error);                                \
                krb5_free_error_message(ctx, pszKrb5Error);           \
            }                                                         \
        } else {                                                      \
            LWIO_LOG_ERROR("KRB5 Error at %s:%d [Code:%d]",            \
                    __FILE__,                                         \
                    __LINE__,                                         \
                    ret);                                             \
        }                                                             \
        if (ret == KRB5KDC_ERR_KEY_EXP) {                             \
            dwError = LWIO_ERROR_PASSWORD_EXPIRED;                     \
        } else if (ret == KRB5_LIBOS_BADPWDMATCH) {                   \
            dwError = LWIO_ERROR_PASSWORD_MISMATCH;                    \
        } else if (ret == KRB5KRB_AP_ERR_SKEW) {                      \
            dwError = LWIO_ERROR_CLOCK_SKEW;                           \
        } else if (ret == ENOENT) {                                   \
            dwError = LWIO_ERROR_KRB5_NO_KEYS_FOUND;                   \
        } else {                                                      \
            dwError = LWIO_ERROR_KRB5_CALL_FAILED;                     \
        }                                                             \
        goto error;                                                   \
    }

typedef enum
{
    SMB_GSS_SEC_CONTEXT_STATE_INITIAL = 0,
    SMB_GSS_SEC_CONTEXT_STATE_NEGOTIATE,
    SMB_GSS_SEC_CONTEXT_STATE_COMPLETE
} SMB_GSS_SEC_CONTEXT_STATE;

#endif
