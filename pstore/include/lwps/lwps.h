/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        lwps.h
 *
 * Abstract:
 *
 *        Likewise Password Store API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *           Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __LWPS_H__
#define __LWPS_H__

#ifndef _WIN32

#include <lw/types.h>
#include <lw/attrs.h>

#endif

/* ERRORS */
#define LWPS_ERROR_SUCCESS                   0x0000
#define LWPS_ERROR_INVALID_CACHE_PATH        0x4000 // 16384
#define LWPS_ERROR_INVALID_CONFIG_PATH       0x4001 // 16385
#define LWPS_ERROR_INVALID_PREFIX_PATH       0x4002 // 16386
#define LWPS_ERROR_INSUFFICIENT_BUFFER       0x4003 // 16387
#define LWPS_ERROR_OUT_OF_MEMORY             0x4004 // 16388
#define LWPS_ERROR_DATA_ERROR                0x4005 // 16389
#define LWPS_ERROR_NOT_IMPLEMENTED           0x4006 // 16390
#define LWPS_ERROR_REGEX_COMPILE_FAILED      0x4007 // 16391
#define LWPS_ERROR_INTERNAL                  0x4008 // 16392
#define LWPS_ERROR_UNEXPECTED_DB_RESULT      0x4009 // 16393
#define LWPS_ERROR_INVALID_PARAMETER         0x400A // 16394
#define LWPS_ERROR_INVALID_SID_REVISION      0x400B // 16395
#define LWPS_ERROR_LOAD_LIBRARY_FAILED       0x400C // 16396
#define LWPS_ERROR_LOOKUP_SYMBOL_FAILED      0x400D // 16397
#define LWPS_ERROR_INVALID_CONFIG            0x400E // 16398
#define LWPS_ERROR_UNEXPECTED_TOKEN          0x400F // 16399
#define LWPS_ERROR_STRING_CONV_FAILED        0x4010 // 16400
#define LWPS_ERROR_QUERY_CREATION_FAILED     0x4011 // 16401
#define LWPS_ERROR_NOT_SUPPORTED             0x4012 // 16402
#define LWPS_ERROR_NO_SUCH_PROVIDER          0x4013 // 16403
#define LWPS_ERROR_INVALID_PROVIDER          0x4014 // 16404
#define LWPS_ERROR_INVALID_SID               0x4015 // 16405
#define LWPS_ERROR_INVALID_ACCOUNT           0x4016 // 16406
#define LWPS_ERROR_INVALID_HANDLE            0x4017 // 16407
#define LWPS_ERROR_DB_RECORD_NOT_FOUND       0x4018 // 16408
#define LWPS_ERROR_INVALID_MESSAGE           0x4019 // 16409
#define LWPS_ERROR_ACCESS_DENIED             0x401A // 16410
#define LWPS_ERROR_SENTINEL                  0x401B // 16411

#define LWPS_ERROR_MASK(_e_)             (_e_ & 0x4000)

typedef struct __LWPS_PASSWORD_INFO
{
    wchar16_t* pwszDomainName;
    wchar16_t* pwszDnsDomainName;
    wchar16_t* pwszSID;
    wchar16_t* pwszHostname;
    wchar16_t* pwszHostDnsDomain;
    wchar16_t* pwszMachineAccount;
    wchar16_t* pwszMachinePassword;
    time_t     last_change_time;
    DWORD      dwSchannelType;

} LWPS_PASSWORD_INFO, *PLWPS_PASSWORD_INFO;

typedef enum
{
    LWPS_PASSWORD_STORE_UNKNOWN = 0,
    LWPS_PASSWORD_STORE_DEFAULT,
    LWPS_PASSWORD_STORE_SQLDB,
    LWPS_PASSWORD_STORE_TDB,
    LWPS_PASSWORD_STORE_FILEDB,
    LWPS_PASSWORD_STORE_REGDB
} LwpsPasswordStoreType;

DWORD
LwpsOpenPasswordStore(
    LwpsPasswordStoreType storeType,
    PHANDLE hStore
    );

DWORD
LwpsGetPasswordByHostName(
    HANDLE hStore,
    PCSTR  pszHostname,
    PLWPS_PASSWORD_INFO* ppInfo
    );

DWORD
LwpsGetPasswordByCurrentHostName(
    HANDLE hStore,
    PLWPS_PASSWORD_INFO* ppInfo
    );

DWORD
LwpsGetPasswordByDomainName(
    HANDLE hStore,
    PCSTR  pszDomainName,
    PLWPS_PASSWORD_INFO* ppInfo
    );

DWORD
LwpsGetHostListByDomainName(
    HANDLE hStore,
    PCSTR  pszDomainName,
    PSTR **pppszHostnames,
    DWORD *pdwNumEntries
    );

DWORD
LwpsWritePasswordToAllStores(
    PLWPS_PASSWORD_INFO pInfo
    );

DWORD
LwpsDeleteEntriesInAllStores(
    VOID
    );

DWORD
LwpsDeleteHostInAllStores(
    PCSTR pszDomainName
    );

VOID
LwpsFreePasswordInfo(
    HANDLE hStore,
    PLWPS_PASSWORD_INFO pInfo
    );

VOID
LwpsFreeHostList(
    PSTR *ppszHostnames,
    DWORD dwNumEntries
    );

DWORD
LwpsClosePasswordStore(
    HANDLE hStore
    );

BOOLEAN
LwpsIsLwpsError(
    DWORD dwErrorCode
    );

size_t
LwpsGetErrorString(
    DWORD  dwErrorCode,
    PSTR   pszBuffer,
    size_t stBufSize
    );

typedef enum
{
    LWPS_LOG_LEVEL_ALWAYS = 0,
    LWPS_LOG_LEVEL_ERROR,
    LWPS_LOG_LEVEL_WARNING,
    LWPS_LOG_LEVEL_INFO,
    LWPS_LOG_LEVEL_VERBOSE,
    LWPS_LOG_LEVEL_DEBUG
} LwpsLogLevel;

typedef VOID (*PLWPS_LOG_CALLBACK)(LwpsLogLevel level, PVOID pUserData, PCSTR pszMessage);

DWORD
LwpsSetLogFunction(
    IN LwpsLogLevel maxLevel,
    IN PLWPS_LOG_CALLBACK pCallback,
    IN PVOID pUserData
    );

DWORD
LwpsLogMessage(
    IN LwpsLogLevel level,
    IN PCSTR pszFormat,
    ...
    );

#endif /* __LWPS_H__ */
