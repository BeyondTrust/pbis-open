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
