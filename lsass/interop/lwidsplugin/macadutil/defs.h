#ifndef __DEFS_H__
#define __DEFS_H__


#define STATIC_PATH_BUFFER_SIZE 256
#define KRB5CCENVVAR "KRB5CCNAME"
#define LWDS_ADMIN_CACHE_DIR "/var/lib/likewise/lwidsplugin"

typedef gss_ctx_id_t CtxtHandle, *PCtxtHandle;

#define BAIL_ON_SEC_ERROR(dwMajorStatus, dwMinorStatus)       \
    if ((dwMajorStatus!= GSS_S_COMPLETE) &&                   \
        (dwMajorStatus != GSS_S_CONTINUE_NEEDED)) {           \
        LOG_ERROR("GSS API Error: Maj:%d Min: %d at %s:%d", dwMajorStatus, dwMinorStatus,  __FILE__, __LINE__); \
        dwError = MAC_AD_ERROR_GSS_API_FAILED;                \
        goto cleanup;                                         \
    }

#define BAIL_ON_KRB_ERROR(ctx, ret)                       \
    if (ret) {                                            \
        LOG_ERROR("KRB5 error at %s:%d. Error code: %d", __FILE__, __LINE__, ret); \
        switch (ret) {                                    \
        case ENOENT:                                      \
            dwError = ret;                                \
            break;                                        \
        case KRB5_LIBOS_BADPWDMATCH:                      \
            dwError = MAC_AD_ERROR_KRB5_PASSWORD_MISMATCH;\
            break;                                        \
        case KRB5KDC_ERR_KEY_EXP:                         \
            dwError = MAC_AD_ERROR_KRB5_PASSWORD_EXPIRED; \
            break;                                        \
        case KRB5KRB_AP_ERR_SKEW:                         \
            dwError = MAC_AD_ERROR_KRB5_CLOCK_SKEW;       \
            break;                                        \
        default:                                          \
            dwError = MAC_AD_ERROR_KRB5_ERROR;            \
        }                                                 \
        goto error;                                       \
    }

#endif /* __DEFS_H__ */

