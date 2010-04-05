#ifndef __GPLDAPDEF_H__
#define __GPLDAPDEF_H__

#ifndef WIN32
typedef gss_ctx_id_t CtxtHandle, *PCtxtHandle;
#endif

#ifdef WIN32

#define BAIL_ON_SEC_ERROR(dwMajorStatus)                        \
    if ((dwMajorStatus!= SEC_E_OK) &&                           \
        (dwMajorStatus != SEC_I_CONTINUE_NEEDED)) {             \
        GPA_LOG_ERROR("GSS API Error: %d", dwMajorStatus);      \
        ceError = CENTERROR_GP_GSS_CALL_FAILED;                 \
        goto error;                                             \
    }

#else

#ifndef BAIL_ON_SEC_ERROR

#define BAIL_ON_SEC_ERROR(dwMajorStatus)                        \
    if ((dwMajorStatus!= GSS_S_COMPLETE) &&                     \
        (dwMajorStatus != GSS_S_CONTINUE_NEEDED)) {             \
        GPA_LOG_ERROR("GSS API Error: %d", dwMajorStatus);      \
        ceError = CENTERROR_GP_GSS_CALL_FAILED;                 \
        goto error;                                             \
    }

#endif

#ifndef BAIL_ON_KRB_ERROR

#define BAIL_ON_KRB_ERROR(ctx, ret)                                                      \
    if (ret) {                                                                           \
        if (ctx)  {                                                                      \
            PCSTR pszKrb5Error = krb5_get_error_message(ctx, ret);                        \
            if (pszKrb5Error) {                                                          \
                GPA_LOG_ERROR("KRB5 Error at %s:%d: %s",__FILE__,__LINE__,pszKrb5Error); \
                krb5_free_error_message(ctx, pszKrb5Error);                              \
            }                                                                            \
        } else {                                                                         \
            GPA_LOG_ERROR("KRB5 Error at %s:%d [Code:%d]",__FILE__,__LINE__,ret);        \
        }                                                                                \
        ceError = CENTERROR_GP_KRB5_CALL_FAILED;                                         \
        goto error;                                                                      \
    }

#endif

#endif /* WIN32 */

#endif /* __GPLDAPDEF_H__ */

