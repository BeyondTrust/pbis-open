/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        utils.h
 *
 * Abstract:
 *
 *        Likewise I/O Subsystem (LWIO)
 *
 *        Public API
 *
 * Author: Evgeny Popovich (epopovich@likewise.com)
 */
#ifndef __LWIO_DDK_UTILS_H__
#define __LWIO_DDK_UTILS_H__

/*
 * Logging
 */

#define _LWIO_LOG_TIME_STAMP_PREFIX_SIZE 128

extern HANDLE               ghLwioLog;
extern LWIO_LOG_LEVEL       gLwioMaxLogLevel;
extern PFN_LWIO_LOG_MESSAGE gpfnLwioLogger;
extern BOOLEAN              gbLwioLogDoNanoSecondTime;
extern CHAR                 gszLwioLogTimeStampPrefix[_LWIO_LOG_TIME_STAMP_PREFIX_SIZE];

VOID
LwioLogMessage(
    PFN_LWIO_LOG_MESSAGE pfnLogger,
    HANDLE hLog,
    LWIO_LOG_LEVEL logLevel,
    PCSTR  pszFormat,
    ...
    );

// Logger lock must be held
PSTR
_LwioLogGetTimeStampPrefix(
    VOID
    );

#if defined(LW_SUPPORT_NANOSECOND_TIMESTAMP)
static
inline
PSTR
_LwioLogGetTimeStampPrefixIf(
    VOID
    )
{
    if (gbLwioLogDoNanoSecondTime)
    {
        return _LwioLogGetTimeStampPrefix();
    }
    else
    {
        gszLwioLogTimeStampPrefix[0] = 0;
        return gszLwioLogTimeStampPrefix;
    }
}
#define _LWIO_LOG_PREFIX_NS(Format) \
    "%s" Format, _LwioLogGetTimeStampPrefixIf()
#else
#define _LWIO_LOG_PREFIX_NS(Format) \
    Format
#endif

#if defined(LW_ENABLE_THREADS)

extern pthread_mutex_t gLwioLogLock;

#define LWIO_LOCK_LOGGER   pthread_mutex_lock(&gLwioLogLock)
#define LWIO_UNLOCK_LOGGER pthread_mutex_unlock(&gLwioLogLock)

#define _LWIO_LOG_PREFIX_THREAD(Format) \
    _LWIO_LOG_PREFIX_NS("0x%lx:" Format), ((unsigned long)pthread_self())

#else

#define LWIO_LOCK_LOGGER
#define LWIO_UNLOCK_LOGGER

#define _LWIO_LOG_PREFIX_THREAD(Format) \
    _LWIO_LOG_PREFIX_NS(Format)

#endif

#define _LWIO_LOG_PREFIX_LOCATION(Format, Function, File, Line) \
    _LWIO_LOG_PREFIX_THREAD("[%s() %s:%d] " Format), \
    (Function), \
    (File), \
    (Line)

#define _LWIO_LOG_WITH_THREAD(Level, Format, ...) \
    _LWIO_LOG_MESSAGE(Level, \
                      _LWIO_LOG_PREFIX_THREAD(Format), \
                      ## __VA_ARGS__)

#define _LWIO_LOG_WITH_LOCATION(Level, Format, Function, File, Line, ...) \
    _LWIO_LOG_MESSAGE(Level, \
                      _LWIO_LOG_PREFIX_LOCATION(Format, Function, File, Line), \
                      ## __VA_ARGS__)

#define _LWIO_LOG_WITH_DEBUG(Level, Format, ...) \
    _LWIO_LOG_WITH_LOCATION(Level, Format, \
                            __FUNCTION__, __FILE__, __LINE__, \
                            ## __VA_ARGS__)

#define _LWIO_LOG_MESSAGE(Level, Format, ...) \
    LwioLogMessage(gpfnLwioLogger, ghLwioLog, Level, Format, ## __VA_ARGS__)

#define _LWIO_LOG_IF(Level, Format, ...)                     \
    do {                                                    \
        LWIO_LOCK_LOGGER;                                    \
        if (gpfnLwioLogger && (gLwioMaxLogLevel >= (Level)))  \
        {                                                   \
            if (gLwioMaxLogLevel >= LWIO_LOG_LEVEL_DEBUG)     \
            {                                               \
                _LWIO_LOG_WITH_DEBUG(Level, Format, ## __VA_ARGS__); \
            }                                               \
            else                                            \
            {                                               \
                _LWIO_LOG_WITH_THREAD(Level, Format, ## __VA_ARGS__); \
            }                                               \
        }                                                   \
        LWIO_UNLOCK_LOGGER;                                  \
    } while (0)

#define _LWIO_LOG_IF_CUSTOM(Level, Format, ...)                   \
    do {                                                          \
        LWIO_LOCK_LOGGER;                                         \
        if (gpfnLwioLogger)                                       \
        {                                                         \
            _LWIO_LOG_WITH_THREAD(Level, Format, ## __VA_ARGS__); \
        }                                                         \
        LWIO_UNLOCK_LOGGER;                                       \
    } while (0)

#define LWIO_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define LWIO_LOG_ALWAYS_CUSTOM(logLevel, szFmt, ...) \
    _LWIO_LOG_IF_CUSTOM(logLevel, szFmt, ## __VA_ARGS__)

#define LWIO_LOG_ALWAYS(szFmt, ...) \
    _LWIO_LOG_IF(LWIO_LOG_LEVEL_ALWAYS, szFmt, ## __VA_ARGS__)

#define LWIO_LOG_ERROR(szFmt, ...) \
    _LWIO_LOG_IF(LWIO_LOG_LEVEL_ERROR, szFmt, ## __VA_ARGS__)

#define LWIO_LOG_WARNING(szFmt, ...) \
    _LWIO_LOG_IF(LWIO_LOG_LEVEL_WARNING, szFmt, ## __VA_ARGS__)

#define LWIO_LOG_INFO(szFmt, ...) \
    _LWIO_LOG_IF(LWIO_LOG_LEVEL_INFO, szFmt, ## __VA_ARGS__)

#define LWIO_LOG_VERBOSE(szFmt, ...) \
    _LWIO_LOG_IF(LWIO_LOG_LEVEL_VERBOSE, szFmt, ## __VA_ARGS__)

#define LWIO_LOG_DEBUG(szFmt, ...) \
    _LWIO_LOG_IF(LWIO_LOG_LEVEL_DEBUG, szFmt, ## __VA_ARGS__)

#define LWIO_LOG_TRACE(szFmt, ...) \
    _LWIO_LOG_IF(LWIO_LOG_LEVEL_TRACE, szFmt, ## __VA_ARGS__)


/*
 * Registry
 */

typedef struct __LWIO_CONFIG_REG LWIO_CONFIG_REG, *PLWIO_CONFIG_REG;

typedef enum
{
    LwIoTypeString,
    LwIoTypeMultiString,
    LwIoTypeDword,
    LwIoTypeBoolean,
    LwIoTypeChar,
    LwIoTypeEnum
} LWIO_CONFIG_TYPE;

typedef struct __LWIO_CONFIG_TABLE
{
    PCSTR   pszName;
    BOOLEAN bUsePolicy;
    LWIO_CONFIG_TYPE Type;
    DWORD dwMin;
    DWORD dwMax;
    const PCSTR *ppszEnumNames;
    PVOID pValue;
} LWIO_CONFIG_TABLE, *PLWIO_CONFIG_TABLE;

NTSTATUS
LwIoProcessConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLWIO_CONFIG_TABLE pConfig,
    DWORD dwConfigEntries,
    BOOLEAN bIgnoreNotFound
    );

NTSTATUS
LwIoOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLWIO_CONFIG_REG *ppReg
    );

VOID
LwIoCloseConfig(
    PLWIO_CONFIG_REG pReg
    );

NTSTATUS
LwIoReadConfigString(
    PLWIO_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue
    );

NTSTATUS
LwIoReadConfigMultiString(
    PLWIO_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    **pppszValue
    );

VOID
LwIoMultiStringFree(
    PSTR *ppszValue
    );

NTSTATUS
LwIoReadConfigDword(
    PLWIO_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD   dwMin,
    DWORD   dwMax,
    PDWORD pdwValue
    );

NTSTATUS
LwIoReadConfigBoolean(
    PLWIO_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    );

NTSTATUS
LwIoReadConfigEnum(
    PLWIO_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD dwMin,
    DWORD dwMax,
    const PCSTR *ppszEnumNames,
    PDWORD pdwValue
    );

#endif /* __LWIO_DDK_UTILS_H__ */
