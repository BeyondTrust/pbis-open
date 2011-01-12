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

#define BAIL_ON_LW_ERROR(_error) \
    do { \
        if ((_error) != 0) { \
            char _errorString[256]; \
            LwGetErrorString(_error, _errorString, sizeof(_errorString)); \
            BAIL("Error code: %d (symbol: %s/%s)", _error, \
                    LW_SAFE_LOG_STRING(LwWin32ErrorToName(_error)), \
                    _errorString); \
        } \
    } while (0)

#define BAIL_ON_LDAP_ERROR(_errorVar, _ldapError) \
    do { \
        if ((_ldapError) != 0) \
        { \
            char _errorString[256]; \
            _errorVar = LwMapLdapErrorToLwError(_ldapError); \
            LwGetErrorString(_errorVar, _errorString, sizeof(_errorString)); \
            BAIL("Ldap error code: %d (LW error %d, symbol %s/%s)", \
                    _ldapError, _errorVar, \
                    LW_SAFE_LOG_STRING(LwWin32ErrortoName(_errorVar)), \
                    _errorString); \
            goto cleanup; \
        } \
    } while (0)

#endif /* __LWAUTOENROLL_BAIL_H__ */
