/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        fileentry.c
 *
 * Abstract:
 *
 *        Registry File Backend
 *
 *
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "api.h"

DWORD
FileCreateKeyEx(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey,
    DWORD Reserved,
    PWSTR pClass,
    DWORD dwOptions,
    REGSAM samDesired,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHKEY phkResult,
    PDWORD pdwDisposition
    )
{
    DWORD dwError = 0;


    return dwError;
}


DWORD
FileCloseKey(
    HANDLE Handle,
    HKEY hKey
    )
{
    DWORD dwError = 0;


    return dwError;
}

DWORD
FileDeleteKey(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
FileDeleteKeyValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey,
    PCWSTR pValueName
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
FileDeleteTree(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
FileDeleteValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pValueName
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
FileEnumKeyEx(
    HANDLE Handle,
    HKEY hKey,
    DWORD dwIndex,
    PWSTR* ppName,
    PDWORD pcName,
    PDWORD pReserved,
    PWSTR pClass,
    PDWORD pcClass,
    PFILETIME pftLastWriteTime
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
FileEnumValue(
    HANDLE Handle,
    HKEY hKey,
    DWORD dwIndex,
    PWSTR pValueName,
    PDWORD pcchValueName,
    PDWORD pReserved,
    PDWORD pType,
    PBYTE pData,
    PDWORD pcbData
    )
{
    DWORD dwError = 0;

}

DWORD
FileGetValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey,
    PCWSTR pValue,
    DWORD dwFlags,
    PDWORD pdwType,
    PVOID pvData,
    PDWORD pcbData
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
FileOpenKeyEx(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
FileQueryInfoKey(
    HANDLE Handle,
    HKEY hKey,
    PWSTR pClass,
    PDWORD pcClass,
    PDWORD pReserved,
    PDWORD pcSubKeys,
    PDWORD pcMaxSubKeyLen,
    PDWORD pcMaxClassLen,
    PDWORD pcValues,
    PDWORD pcMaxValueNameLen,
    PDWORD pcMaxValueLen,
    PDWORD pcbSecurityDescriptor,
    PFILETIME pftLastWriteTime
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
FileQueryMultipleValues(
    IN HANDLE Handle,
    IN HKEY hKey,
    OUT PVALENT* ppVal_list,
    IN DWORD num_vals,
    OUT PWSTR* ppValue,
    OUT PDWORD pdwTotalsize
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
FileQueryValueEx(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pValueName,
    PDWORD pReserved,
    PDWORD pType,
    PBYTE pData,
    PDWORD pcbData
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
FileSetValueEx(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE *pData,
    DWORD cbData
    )
{
    DWORD dwError = 0;

    return dwError;
}
