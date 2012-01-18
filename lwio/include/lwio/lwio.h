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
 *        lwio.h
 *
 * Abstract:
 *
 *        Likewise I/O Subsystem (LWIO)
 *
 *        Public API
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 */
#ifndef __LWIO_H__
#define __LWIO_H__

#include <stdarg.h>
#include <lw/base.h>
#include <lwio/io-types.h>

typedef unsigned char uchar8_t;

#define NUL  ((uchar8_t) 0)
#define WNUL ((wchar16_t) 0)

typedef LW_NTSTATUS SMB_ERROR;

#define LWIO_ERROR_SUCCESS                      LW_STATUS_SUCCESS
#define LWIO_ERROR_INVALID_CACHE_PATH           LW_STATUS_INVALID_PARAMETER
#define LWIO_ERROR_INVALID_CONFIG_PATH          LW_STATUS_INVALID_PARAMETER
#define LWIO_ERROR_INVALID_PREFIX_PATH          LW_STATUS_INVALID_PARAMETER
#define LWIO_ERROR_INSUFFICIENT_BUFFER          LW_STATUS_BUFFER_TOO_SMALL
#define LWIO_ERROR_OUT_OF_MEMORY                LW_STATUS_INSUFFICIENT_RESOURCES
#define LWIO_ERROR_NOT_IMPLEMENTED              LW_STATUS_NOT_IMPLEMENTED
#define LWIO_ERROR_REGEX_COMPILE_FAILED         LW_STATUS_INVALID_PARAMETER
#define LWIO_ERROR_INTERNAL                     LW_STATUS_UNSUCCESSFUL
#define LWIO_ERROR_INVALID_PARAMETER            LW_STATUS_INVALID_PARAMETER
#define LWIO_ERROR_INVALID_CONFIG               LW_STATUS_DATA_ERROR
#define LWIO_ERROR_NULL_BUFFER                  LW_STATUS_INVALID_PARAMETER
#define LWIO_ERROR_INVALID_LOG_LEVEL            LW_STATUS_INVALID_PARAMETER
#define LWIO_ERROR_LWMSG_ERROR                  LW_STATUS_UNSUCCESSFUL
#define LWIO_ERROR_MALFORMED_REQUEST            LW_STATUS_INVALID_NETWORK_RESPONSE
#define LWIO_ERROR_LWMSG_EOF                    LW_STATUS_END_OF_FILE
#define LWIO_ERROR_NO_SUCH_ITEM                 LW_STATUS_NOT_FOUND
#define LWIO_ERROR_OVERFLOW                     LW_STATUS_INTEGER_OVERFLOW
#define LWIO_ERROR_UNDERFLOW                    LW_STATUS_FLOAT_UNDERFLOW
#define LWIO_ERROR_SYSTEM                       LW_STATUS_UNSUCCESSFUL
#define LWIO_ERROR_SERVER_UNREACHABLE           LW_STATUS_HOST_UNREACHABLE
#define LWIO_ERROR_STRING_CONV_FAILED           LW_STATUS_INVALID_PARAMETER
#define LWIO_ERROR_PASSWORD_EXPIRED             LW_STATUS_PASSWORD_EXPIRED
#define LWIO_ERROR_PASSWORD_MISMATCH            LW_STATUS_WRONG_PASSWORD
#define LWIO_ERROR_CLOCK_SKEW                   LW_STATUS_TIME_DIFFERENCE_AT_DC
#define LWIO_ERROR_KRB5_NO_KEYS_FOUND           LW_STATUS_NOT_FOUND
#define LWIO_ERROR_KRB5_CALL_FAILED             LW_STATUS_UNSUCCESSFUL
#define LWIO_ERROR_NO_BIT_AVAILABLE             LW_STATUS_UNSUCCESSFUL
#define LWIO_ERROR_INVALID_HANDLE               LW_STATUS_INVALID_HANDLE
#define LWIO_ERROR_OUT_OF_HANDLES               LW_STATUS_INSUFFICIENT_RESOURCES
#define LWIO_ERROR_GSS                          LW_STATUS_UNSUCCESSFUL
#define LWIO_ERROR_HOST_NOT_FOUND               LW_STATUS_NOT_FOUND
#define LWIO_ERROR_INVALID_VFS_PROVIDER         LW_STATUS_INVALID_ARGUMENT
#define LWIO_ERROR_DATA_ERROR                   LW_STATUS_DATA_ERROR
#define LWIO_ERROR_LOGON_FAILURE                LW_STATUS_LOGON_FAILURE

/*
 * Logging
 */
typedef enum
{
    LWIO_LOG_LEVEL_ALWAYS = LW_RTL_LOG_LEVEL_ALWAYS,
    LWIO_LOG_LEVEL_ERROR = LW_RTL_LOG_LEVEL_ERROR,
    LWIO_LOG_LEVEL_WARNING = LW_RTL_LOG_LEVEL_WARNING,
    LWIO_LOG_LEVEL_INFO = LW_RTL_LOG_LEVEL_INFO,
    LWIO_LOG_LEVEL_VERBOSE = LW_RTL_LOG_LEVEL_VERBOSE,
    LWIO_LOG_LEVEL_DEBUG = LW_RTL_LOG_LEVEL_DEBUG,
    LWIO_LOG_LEVEL_TRACE = LW_RTL_LOG_LEVEL_TRACE
} LWIO_LOG_LEVEL;

typedef enum
{
    LWIO_DRIVER_STATE_UNKNOWN,
    LWIO_DRIVER_STATE_UNLOADED,
    LWIO_DRIVER_STATE_LOADED
} LWIO_DRIVER_STATE, *PLWIO_DRIVER_STATE;

LW_NTSTATUS
LwIoShutdown(
    VOID
    );

LW_NTSTATUS
LwIoQueryStateDriver(
    LW_PWSTR pwszDriverName,
    PLWIO_DRIVER_STATE pState
    );

LW_NTSTATUS
LwIoLoadDriver(
    LW_PWSTR pwszDriverName
    );

LW_NTSTATUS
LwIoUnloadDriver(
    LW_PWSTR pwszDriverName
    );

LW_NTSTATUS
LwIoGetPid(
    pid_t* pPid
    );

LW_NTSTATUS
LwIoCreatePlainCredsW(
    LW_PCWSTR pwszUsername,
    LW_PCWSTR pwszDomain,
    LW_PCWSTR pwszPassword,
    LW_PIO_CREDS* ppCreds
    );

LW_NTSTATUS
LwIoCreatePlainCredsA(
    PCSTR pszUsername,
    PCSTR pszDomain,
    PCSTR pszPassword,
    LW_PIO_CREDS* ppCreds
    );

LW_NTSTATUS
LwIoCreateKrb5CredsW(
    PCWSTR pwszPrincipal,
    PCWSTR pwszCachePath,
    LW_PIO_CREDS* ppCreds
    );

LW_NTSTATUS
LwIoCreateKrb5CredsA(
    PCSTR pszPrincipal,
    PCSTR pszCachePath,
    LW_PIO_CREDS* ppCreds
    );

BOOLEAN
LwIoCompareCreds(
    LW_PIO_CREDS pCreds1,
    LW_PIO_CREDS pCreds2
    );

LW_NTSTATUS
LwIoCopyCreds(
    LW_PIO_CREDS pCreds,
    LW_PIO_CREDS * ppCopy
    );

VOID
LwIoDeleteCreds(
    LW_PIO_CREDS pCreds
    );

LW_NTSTATUS
LwIoSetThreadCreds(
    LW_PIO_CREDS pToken
    );

LW_NTSTATUS
LwIoGetThreadCreds(
    LW_PIO_CREDS* ppToken
    );

LW_NTSTATUS
LwIoSetPathCreds(
    IN LW_PUNICODE_STRING PathPrefix,
    IN OPTIONAL LW_PIO_CREDS pCreds
    );

LW_NTSTATUS
LwIoGetActiveCreds(
    IN OPTIONAL LW_PUNICODE_STRING PathPrefix,
    OUT LW_PIO_CREDS* ppToken
    );

LW_NTSTATUS
LwIoUncPathToInternalPath(
    LW_PCWSTR pwszUncPath,
    LW_PWSTR* ppwszInternalPath
    );

#include <lwio/ntfileapi.h>
#include <lwio/smbfileapi.h>
#include <lwio/srvshareapi.h>

#endif /* __LWIO_H__ */
