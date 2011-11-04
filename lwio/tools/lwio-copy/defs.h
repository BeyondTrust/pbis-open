#ifndef __DEFS_H__
#define __DEFS_H__

#define BUFF_SIZE 1024
#define MAX_BUFFER 4096

#define BAIL_ON_NULL_POINTER(p)                    \
        if (NULL == p) {                          \
           status = LWIO_ERROR_INVALID_PARAMETER; \
           goto error;                            \
        }

#define BAIL_ON_KRB_ERROR(ctx, ret)                                   \
    if (ret) {                                                        \
        if (ctx)  {                                                   \
            PCSTR pszKrb5Error = krb5_get_error_message(ctx, ret);    \
            if (pszKrb5Error) {                                       \
                LWIO_LOG_ERROR("KRB5 Error at %s:%d: %s",             \
                        __FILE__,                                     \
                        __LINE__,                                     \
                        pszKrb5Error);                                \
                krb5_free_error_message(ctx, pszKrb5Error);           \
            }                                                         \
        } else {                                                      \
            LWIO_LOG_ERROR("KRB5 Error at %s:%d [Code:%d]",           \
                    __FILE__,                                         \
                    __LINE__,                                         \
                    ret);                                             \
        }                                                             \
        if (ret == KRB5KDC_ERR_KEY_EXP) {                             \
            ntStatus = LWIO_ERROR_PASSWORD_EXPIRED;                   \
        } else if (ret == KRB5_LIBOS_BADPWDMATCH) {                   \
            ntStatus = LWIO_ERROR_PASSWORD_MISMATCH;                  \
        } else if (ret == KRB5KRB_AP_ERR_SKEW) {                      \
            ntStatus = LWIO_ERROR_CLOCK_SKEW;                         \
        } else if (ret == ENOENT) {                                   \
            ntStatus = LWIO_ERROR_KRB5_NO_KEYS_FOUND;                 \
        } else {                                                      \
            ntStatus = LWIO_ERROR_LOGON_FAILURE;                      \
        }                                                             \
        goto error;                                                   \
    }

#endif /* __DEFS_H__ */

