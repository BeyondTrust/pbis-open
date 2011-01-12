#ifndef __LWAUTOENROLL_BAIL_H__
#define __LWAUTOENROLL_BAIL_H__

#include <lw/attrs.h>
#include <lw/types.h>

#include <lwdef.h>
#include <lwlogging.h>

#include <errno.h>
#include <string.h>

#define BAIL(_format, ...) \
    do { \
        LW_LOG_DEBUG("[%s() %s:%d] " _format, __FUNCTION__, \
                __FILE__, __LINE__ , ## __VA_ARGS__); \
        goto cleanup; \
    } while (0)

#define _BAIL_ON_LW_ERROR(_error, _format, ...) \
    do { \
        if ((_error) != 0) { \
            BAIL("Error code: %d (%s/%s)" _format, _error, \
                    LW_SAFE_LOG_STRING(LwWin32ExtErrorToName(_error)), \
                    LW_SAFE_LOG_STRING(LwWin32ExtErrorToDescription(_error)) \
                    , ## __VA_ARGS__); \
        } \
    } while (0)

#define BAIL_ON_LW_ERROR(_error, ...) \
    _BAIL_ON_LW_ERROR(_error , ## __VA_ARGS__, "")

#define _BAIL_WITH_LW_ERROR(_error, _format, ...) \
    do { \
        error = _error; \
        _BAIL_ON_LW_ERROR(error, _format , ## __VA_ARGS__); \
    } while(0)

#define BAIL_WITH_LW_ERROR(_error, ...) \
    _BAIL_WITH_LW_ERROR(_error , ## __VA_ARGS__, "")

#define BAIL_ON_UNIX_ERROR(_expr) \
    do { \
        if (_expr) { \
            _BAIL_WITH_LW_ERROR(LwMapErrnoToLwError(errno), \
                    ": UNIX Error %d (%s)", errno, strerror(errno)); \
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
