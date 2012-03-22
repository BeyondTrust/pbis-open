/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#ifndef __LWAUTOENROLL_BAIL_H__
#define __LWAUTOENROLL_BAIL_H__

#include <lw/attrs.h>
#include <lw/types.h>

#include <lw/rtllog.h>

#include <errno.h>
#include <string.h>

#define _BAIL_FIRST_ARG(_arg1, ...)     _arg1
#define _BAIL_EXTRA_ARGS(_arg1, ...)    __VA_ARGS__

#define _BAIL_FORMAT_STRING(...)        _BAIL_FIRST_ARG(__VA_ARGS__, "") "%s"
#define _BAIL_FORMAT_ARGS(...)          _BAIL_EXTRA_ARGS(__VA_ARGS__, "")

#define BAIL(_format, ...) \
    do { \
        LW_RTL_LOG_DEBUG("[%s() %s:%d]" _format, __FUNCTION__, \
            __FILE__, __LINE__ , ## __VA_ARGS__); \
        goto cleanup; \
    } while (0)

#define BAIL_WITH_LW_ERROR(_error, ...) \
    do { \
        PCSTR _desc = LwWin32ExtErrorToDescription(_error); \
        error = _error; \
        BAIL(" Error code: %d (%s%s%s)" _BAIL_FORMAT_STRING(__VA_ARGS__), \
            _error, LW_RTL_LOG_SAFE_STRING(LwWin32ExtErrorToName(_error)), \
            (_desc && *_desc != '\0') ? "/" : "", LW_RTL_LOG_SAFE_STRING(_desc) , \
            _BAIL_FORMAT_ARGS(__VA_ARGS__)); \
    } while(0)

#define BAIL_ON_LW_ERROR(_error, ...) \
    do { \
        if ((_error) != LW_ERROR_SUCCESS) { \
            BAIL_WITH_LW_ERROR(error , ## __VA_ARGS__); \
        } \
    } while (0)

#define BAIL_WITH_UNIX_ERROR(_errno, ...) \
    do { \
        BAIL_WITH_LW_ERROR(LwMapErrnoToLwError(_errno), \
            _BAIL_FORMAT_STRING(__VA_ARGS__) ": UNIX Error %d (%s)", \
            _BAIL_FORMAT_ARGS(__VA_ARGS__), _errno, strerror(_errno)); \
    } while(0)

#define BAIL_ON_UNIX_ERROR(_expr, ...) \
    do { \
        if (_expr) { \
            BAIL_WITH_UNIX_ERROR(errno , ## __VA_ARGS__); \
        } \
    } while(0)

#define BAIL_WITH_KRB_ERROR(_krbError, _ctx, ...) \
    do { \
        BAIL_WITH_LW_ERROR(LwTranslateKrb5Error(_ctx, _krbError, \
            __FUNCTION__, __FILE__, __LINE__), \
            _BAIL_FORMAT_STRING(__VA_ARGS__), \
            _BAIL_FORMAT_ARGS(__VA_ARGS__)); \
    } while (0)

#define BAIL_ON_KRB_ERROR(_krbError, _ctx, ...) \
    do { \
        if (_krbError) { \
            BAIL_WITH_KRB_ERROR(_krbError, _ctx , ## __VA_ARGS__); \
        } \
    } while (0)

#endif /* __LWAUTOENROLL_BAIL_H__ */
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
