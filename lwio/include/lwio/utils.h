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

extern pthread_mutex_t gLwioLogLock;

#define LWIO_LOCK_LOGGER   pthread_mutex_lock(&gLwioLogLock)
#define LWIO_UNLOCK_LOGGER pthread_mutex_unlock(&gLwioLogLock)

#define LWIO_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define _LWIO_LOG_AT(Level, ...) LW_RTL_LOG_AT_LEVEL(Level, "lwio", __VA_ARGS__)
#define LWIO_LOG_ALWAYS(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_ALWAYS, __VA_ARGS__)
#define LWIO_LOG_ERROR(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_ERROR, __VA_ARGS__)
#define LWIO_LOG_WARNING(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_WARNING, __VA_ARGS__)
#define LWIO_LOG_INFO(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_INFO, __VA_ARGS__)
#define LWIO_LOG_VERBOSE(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_VERBOSE, __VA_ARGS__)
#define LWIO_LOG_DEBUG(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LWIO_LOG_TRACE(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_TRACE, __VA_ARGS__)

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

NTSTATUS
LwIoMultiStringCopy(
    PSTR **pppszNewStrings,
    PCSTR *ppszOriginalStrings
    );

VOID
LwIoMultiStringFree(
    PSTR **pppszValue
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
