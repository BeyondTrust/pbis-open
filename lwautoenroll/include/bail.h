#ifndef __LWAUTOENROLL_BAIL_H__
#define __LWAUTOENROLL_BAIL_H__

#include <lw/attrs.h>
#include <lw/types.h>

#include <lwdef.h>
#include <lwlogging.h>

#define BAIL(_format, ...) \
    do { \
        LW_LOG_DEBUG("[%s() %s:%d] " _format, __FUNCTION__, \
                __FILE__, __LINE__ , ## __VA_ARGS__); \
        goto cleanup; \
    } while (0)

#define _BAIL_ON_LW_ERROR(_error, _format, ...) \
    do { \
        if ((_error) != 0) { \
            char _LWerrorString[256]; \
            LwGetErrorString(_error, _LWerrorString, sizeof(_LWerrorString)); \
            BAIL("Error code: %d (symbol: %s/%s)" _format, _error, \
                    LW_SAFE_LOG_STRING(LwWin32ErrorToName(_error)), \
                    _LWerrorString , ## __VA_ARGS__); \
        } \
    } while (0)

#define BAIL_ON_LW_ERROR(_error)    _BAIL_ON_LW_ERROR(_error, "")

#define _BAIL_WITH_LW_ERROR(_error, _format, ...) \
    do { \
        error = _error; \
        _BAIL_ON_LW_ERROR(error, _format , ## __VA_ARGS__); \
    } while(0)

#define BAIL_WITH_LW_ERROR(_error) \
    _BAIL_WITH_LW_ERROR(_error, "")

#define BAIL_ON_UNIX_ERROR(_expr) \
    do { \
        if (_expr) { \
            _BAIL_WITH_LW_ERROR(LwMapErrnoToLwError(errno), \
                    ": UNIX Error %d (%s)", errno, strerror(errno)); \
        } \
    } while(0)

#define BAIL_ON_LDAP_ERROR(_ldapError) \
    do { \
        if ((_ldapError) != 0) \
        { \
            _BAIL_WITH_LW_ERROR(LwMapLdapErrorToLwError(_ldapError), \
                    ": LDAP error %d", _ldapError); \
        } \
    } while (0)

#define BAIL_WITH_SSL_ERROR(_sslError) \
    do { \
        char _SSLerrorString[256]; \
        ERR_error_string_n(_sslError, _SSLerrorString, \
                sizeof(_SSLerrorString)); \
        _BAIL_WITH_LW_ERROR(LwSSLErrorToLwError(_sslError), \
                ": OpenSSL error %lx (%s)", _sslError, _SSLerrorString); \
    } while(0)

#define BAIL_ON_SSL_ERROR(_expr) \
    do { \
        if (_expr) { \
            unsigned long _sslError = ERR_get_error(); \
            BAIL_WITH_SSL_ERROR(_sslError); \
        } \
    } while(0)

#define BAIL_ON_SSL_CTX_ERROR(_expr, _ctx) \
    do { \
        if (_expr) { \
            BAIL_WITH_SSL_ERROR(X509_STORE_CTX_get_error(_ctx)); \
        } \
    } while(0)

#define BAIL_ON_KRB_ERROR(_krbError, _ctx) \
    do { \
        if (_krbError) { \
            BAIL_WITH_LW_ERROR(LwTranslateKrb5Error(_ctx, _krbError, \
                        __FUNCTION__, __FILE__, __LINE__)); \
        } \
    } while (0)

#endif /* __LWAUTOENROLL_BAIL_H__ */
