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
            char _errorString[256]; \
            LwGetErrorString(_error, _errorString, sizeof(_errorString)); \
            BAIL("Error code: %d (symbol: %s/%s)" _format, _error, \
                    LW_SAFE_LOG_STRING(LwWin32ErrorToName(_error)), \
                    _errorString , ## __VA_ARGS__); \
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

#define BAIL_ON_LDAP_ERROR(_ldap_error) \
    do { \
        if ((_ldapError) != 0) \
        { \
            _BAIL_WITH_LW_ERROR(LwMapLdapErrorToLwError(_ldapError), \
                    ": LDAP error %d", _ldap_error); \
        } \
    } while (0)

extern DWORD LwSSLErrorToLwError(
                unsigned long ssl_error
                );

#define BAIL_WITH_SSL_ERROR(_ssl_error) \
    do { \
        char _errorString[256]; \
        ERR_error_string_n(_ssl_error, _errorString, sizeof(_errorString)); \
        _BAIL_WITH_LW_ERROR(LwSSLErrorToLwError(_ssl_error), \
                ": OpenSSL error %x (%s)", _ssl_error, _errorString); \
    } while(0)

#define BAIL_ON_SSL_ERROR(_expr) \
    do { \
        if (_expr) { \
            BAIL_WITH_SSL_ERROR(ERR_get_error()); \
        } \
    } while(0)

#define BAIL_ON_SSL_CTX_ERROR(_expr, _ctx) \
    do { \
        if (_expr) { \
            BAIL_WITH_SSL_ERROR(X509_STORE_CTX_get_error(_ctx)); \
        } \
    } while(0)

#endif /* __LWAUTOENROLL_BAIL_H__ */
