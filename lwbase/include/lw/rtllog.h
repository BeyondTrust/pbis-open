/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *     rtllog.h
 *
 * Abstract:
 *
 *     RTL Logging
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __RTL_LOG_H__
#define __RTL_LOG_H__

#include <lw/types.h>
#include <lw/attrs.h>

LW_BEGIN_EXTERN_C

//
// The component can optionally define LW_RTL_LOG_COMPONENT to the component
// name that it wants to log before including this header.
//

#ifndef LW_RTL_LOG_COMPONENT
#define LW_RTL_LOG_COMPONENT NULL
#endif


typedef LW_UCHAR LW_RTL_LOG_LEVEL, *PLW_RTL_LOG_LEVEL;

#define LW_RTL_LOG_LEVEL_UNDEFINED  0
#define LW_RTL_LOG_LEVEL_ALWAYS     1
#define LW_RTL_LOG_LEVEL_ERROR      2
#define LW_RTL_LOG_LEVEL_WARNING    3
#define LW_RTL_LOG_LEVEL_INFO       4
#define LW_RTL_LOG_LEVEL_VERBOSE    5
#define LW_RTL_LOG_LEVEL_DEBUG      6
#define LW_RTL_LOG_LEVEL_TRACE      7


typedef LW_VOID (*LW_RTL_LOG_CALLBACK)(
    LW_IN LW_OPTIONAL LW_PVOID Context,
    LW_IN LW_RTL_LOG_LEVEL Level,
    LW_IN LW_OPTIONAL LW_PCSTR ComponentName,
    LW_IN LW_PCSTR FunctionName,
    LW_IN LW_PCSTR FileName,
    LW_IN LW_ULONG LineNumber,
    LW_IN LW_PCSTR Format,
    LW_IN ...
    );
///<
/// The logging callback function.
///
/// This function is reponsible for logging a message.
///
/// @param[in] Context - The optional context #LwRtlLogSetLevel().
/// @param[in] Level - The actual log level of the message.
/// @param[in] ComponentName - The name of the component (if one is defined).
/// @param[in] FunctionName - The name of the function logging the message.
/// @param[in] FileName - The name of the source file logging the message.
/// @param[in] LineNumber - The source code line number logging the message.
/// @param[in] Format - The format string for the message.
/// @param[in] ... - Arguments for the format string.
///
/// @return N/A
///

LW_VOID
LwRtlLogGetCallback(
    LW_OUT LW_OPTIONAL LW_RTL_LOG_CALLBACK *Callback,
    LW_OUT LW_OPTIONAL LW_PVOID *Context
    );
///<
/// Get the logging callback/context.
///
/// @param[out] Callback - If specified, will return the logging callback.
/// @param[out] Context - If specified, will return the logging callback
///     context.
///
/// @return N/A
///

LW_VOID
LwRtlLogSetCallback(
    LW_IN LW_OPTIONAL LW_RTL_LOG_CALLBACK Callback,
    LW_IN LW_OPTIONAL LW_PVOID Context
    );
///<
/// Set the logging callback/context.
///
/// This must be called in a race-free context wrt the logging
/// macros.
///
/// @param[in] Callback - If specified, logging macros will call this
///     callback to log.
/// @param[in] Context - If specified, logging callback will be called
///     with this context.
///
/// @return N/A
///

LW_VOID
LwRtlLogSetLevel(
    LW_IN LW_RTL_LOG_LEVEL Level
    );
///<
/// Set the log level at which to log.
///
/// @param[in] Level - Level at which to log.  Any log messages
///     at a level greater than or equal to this will invoke
///     the logging callback (if one is set).
///
/// @return N/A
///

#define LwRtlLogGetLevel() _LwRtlLogControl.Level
///<
/// Get the current logging level.
///
/// @return LW_RTL_LOG_LEVEL
///


#define LW_RTL_LOG_ALWAYS(Format, ...) \
    _LW_RTL_LOG_IF(LW_RTL_LOG_LEVEL_ALWAYS, Format, ## __VA_ARGS__)

#define LW_RTL_LOG_ERROR(Format, ...) \
    _LW_RTL_LOG_IF(LW_RTL_LOG_LEVEL_ERROR, Format, ## __VA_ARGS__)

#define LW_RTL_LOG_WARNING(Format, ...) \
    _LW_RTL_LOG_IF(LW_RTL_LOG_LEVEL_WARNING, Format, ## __VA_ARGS__)

#define LW_RTL_LOG_INFO(Format, ...) \
    _LW_RTL_LOG_IF(LW_RTL_LOG_LEVEL_INFO, Format, ## __VA_ARGS__)

#define LW_RTL_LOG_VERBOSE(Format, ...) \
    _LW_RTL_LOG_IF(LW_RTL_LOG_LEVEL_VERBOSE,  Format, ## __VA_ARGS__)

#define LW_RTL_LOG_DEBUG(Format, ...) \
    _LW_RTL_LOG_IF(LW_RTL_LOG_LEVEL_DEBUG, Format, ## __VA_ARGS__)

#define LW_RTL_LOG_TRACE(Format, ...) \
    _LW_RTL_LOG_IF(LW_RTL_LOG_LEVEL_TRACE, Format, ## __VA_ARGS__)

#define LW_RTL_LOG_RAW(LogLevel, Component, Function, File, Line, Format, ...) \
    do { \
        if ((_LwRtlLogControl.Level >= (LogLevel)) && _LwRtlLogControl.Callback) \
        { \
            _LwRtlLogControl.Callback( \
                    _LwRtlLogControl.Context, \
                    LogLevel, \
                    Component, \
                    Function, \
                    File, \
                    Line, \
                    Format, \
                    ## __VA_ARGS__); \
        } \
    } while (0)

#define LW_RTL_LOG_AT_LEVEL(LogLevel, Component, Format, ...) \
    LW_RTL_LOG_RAW(LogLevel, Component, __FUNCTION__, __FILE__, __LINE__, Format, ## __VA_ARGS__)

#define LW_RTL_LOG_SAFE_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define LW_RTL_LOG_SAFE_WSTRING(x) \
    ( (x) ? (x) : _LwRtlLogGetEmptyWC16String() )


//
// Do not use any of the below directly
//

#define _LW_RTL_LOG_IF(LogLevel, Format, ...) \
    LW_RTL_LOG_AT_LEVEL(LogLevel, LW_RTL_LOG_COMPONENT, Format, ## __VA_ARGS__)

static
inline
LW_PCWSTR
_LwRtlLogGetEmptyWC16String(
    LW_VOID
    )
{
    static const WCHAR empty = 0;
    return &empty;
}

typedef struct _LW_RTL_LOG_CONTROL {
    LW_RTL_LOG_CALLBACK Callback;
    LW_PVOID Context;
    volatile LW_RTL_LOG_LEVEL Level;
} LW_RTL_LOG_CONTROL, *PLW_RTL_LOG_CONTROL;

extern LW_RTL_LOG_CONTROL _LwRtlLogControl;

LW_END_EXTERN_C

#endif /* __RTL_LOG_H__ */
