/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        fileapi.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#include "includes.h"

DWORD
FileProvider_Intialize(
    PREGPROV_PROVIDER_FUNCTION_TABLE* ppFnTable
    )
{
    *ppFnTable = &gRegFileProviderAPITable;

    return LW_ERROR_SUCCESS;
}

VOID
FileProvider_Shutdown(
    PREGPROV_PROVIDER_FUNCTION_TABLE pFnTable
    )
{


}

DWORD
FileOpenRootKey(
    IN HANDLE Handle,
    IN PSTR pszRootKeyName,
    OUT PHKEY phkResult
    )
{
    return LW_ERROR_SUCCESS;
}

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

#if 0
    dwError = RegStoreFindKey(
                    pszKeyName,
                    &dwKeyIndex
                    );
    if (dwError == ERROR_KEY_NOT_FOUND) {

        dwError = RegStoreCreateKey(
                        pszKeyName,
                        &dwKeyIndex
                        );
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegStoreCheckAccess(
                        hAccessToken,
                        dwKeyIndex,
                        samDesired
                        );
    BAIL_ON_ERROR(dwError);

    dwError = RegCreateHandle(
                        pszFullKeyName,
                        dwKeyIndex,
                        samDesired,
                        phkResult
                        );
    BAIL_ON_ERROR(dwError);

error:
#endif

    return dwError;
}

VOID
FileCloseKey(
    HKEY hKey
    )
{

#if 0
    RegFreeHandle(hKey);
#endif

    return;
}

DWORD
FileDeleteKey(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey
    )
{
    DWORD dwError = 0;

#if 0

    dwError = RegBuildFullKeyPathName(
                    hKey,
                    pSubKey,
                    &pszFullKeyName
                    );
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegFindKeyHandles(
                        pszFullKeyName,
                        &pKCB
                        );
    if (dwError == 0) {

        dwError = ERROR_KEY_IN_USE;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegStoreDeleteKey(
                    dwParentIndex,
                    pszSubKey
                    );
    BAIL_ON_REG_ERROR(dwError);
error:
#endif

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
    PWSTR pName,
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

    return dwError;
}

DWORD
FileGetValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey,
    PCWSTR pValue,
    DWORD dwFlags,
    PDWORD pdwType,
    PBYTE pvData,
    PDWORD pcbData
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
FileGetValueA(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey,
    PCWSTR pValue,
    DWORD dwFlags,
    PDWORD pdwType,
    PBYTE pvData,
    PDWORD pcbData
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
FileGetValueW(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey,
    PCWSTR pValue,
    DWORD dwFlags,
    PDWORD pdwType,
    PBYTE pvData,
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
    HANDLE Handle,
    HKEY hKey,
    PVALENT val_list,
    DWORD num_vals,
    PWSTR pValueBuf,
    PDWORD dwTotsize
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
