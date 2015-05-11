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
 *        registry.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        NT Client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#include "client.h"

/* 
 * This is a work-around for the fact we don't exactly know the
 * multibyte size for these paramters. The only way we can determine
 * this correctly is to actually enumerate the key/values and
 * find the maximum multibyte string length. This is a lot of work,
 * which isn't really necessary. These sizes are used for memory
 * allocation purposes normally, so as long as these are at least
 * as big as the longest multibyte value, this is good enough.
 */
#define LW_WC2MBS_FACTOR_MAX 4

REG_API
NTSTATUS
NtRegEnumRootKeysA(
    IN HANDLE hRegConnection,
    OUT PSTR** pppszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    )
{
	NTSTATUS status = 0;
    PWSTR* ppwszRootKeyNames = NULL;
    PSTR* ppszRootKeyNames = NULL;
    DWORD dwNumRootKeys = 0;
    int iCount = 0;

    status = RegTransactEnumRootKeysW(hRegConnection,
                                       &ppwszRootKeyNames,
                                       &dwNumRootKeys);
    BAIL_ON_NT_STATUS(status);

    if (!dwNumRootKeys)
        goto cleanup;

    status = LW_RTL_ALLOCATE(&ppszRootKeyNames, PSTR, sizeof(*ppszRootKeyNames)*dwNumRootKeys);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < dwNumRootKeys; iCount++)
    {
        status = LwRtlCStringAllocateFromWC16String(&ppszRootKeyNames[iCount],
        		                                     ppwszRootKeyNames[iCount]);
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    *pppszRootKeyNames = ppszRootKeyNames;
    *pdwNumRootKeys = dwNumRootKeys;

    if (ppwszRootKeyNames)
    {
        for (iCount=0; iCount<dwNumRootKeys; iCount++)
        {
            LWREG_SAFE_FREE_MEMORY(ppwszRootKeyNames[iCount]);
        }
        ppwszRootKeyNames = NULL;
    }

    return status;

error:
    if (ppszRootKeyNames)
    {
        RegFreeStringArray(ppszRootKeyNames, dwNumRootKeys);
    }

    goto cleanup;
}

REG_API
NTSTATUS
NtRegEnumRootKeysW(
    IN HANDLE hRegConnection,
    OUT PWSTR** pppwszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    )
{
    return RegTransactEnumRootKeysW(hRegConnection,
                                    pppwszRootKeyNames,
                                   pdwNumRootKeys);
}

REG_API
NTSTATUS
NtRegCreateKeyExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCSTR pszSubKey,
    IN DWORD Reserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pSecDescAbs,
    OUT PHKEY phkResult,
    OUT OPTIONAL PDWORD pdwDisposition
    )
{
	NTSTATUS status = 0;
    PWSTR pwszSubKey = NULL;

    if (pszSubKey)
    {
    	status = LwRtlWC16StringAllocateFromCString(&pwszSubKey,
    			                                     pszSubKey);
        BAIL_ON_NT_STATUS(status);
    }

    status = RegTransactCreateKeyExW(
        hRegConnection,
        hKey,
        pwszSubKey,
        Reserved,
        pClass,
        dwOptions,
        AccessDesired,
        pSecDescAbs,
        phkResult,
        pdwDisposition
        );
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);

    return status;

error:
    goto cleanup;
}

REG_API
NTSTATUS
NtRegCreateKeyExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCWSTR pSubKey,
    IN DWORD Reserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pSecDescAbs,
    OUT PHKEY phkResult,
    OUT OPTIONAL PDWORD pdwDisposition
    )
{
    NTSTATUS status = 0;

    BAIL_ON_NT_INVALID_POINTER(pSubKey);

    // This is enforced on the server-side,
    // but performing this check here prevents extra
    // work from being done that will fail on the server side.
    if (wc16slen(pSubKey) > MAX_KEY_LENGTH)
    {
    	status = STATUS_INVALID_BLOCK_LENGTH;
    	BAIL_ON_NT_STATUS(status);
    }

	status = RegTransactCreateKeyExW(
        hRegConnection,
        hKey,
        pSubKey,
        Reserved,
        pClass,
        dwOptions,
        AccessDesired,
        pSecDescAbs,
        phkResult,
        pdwDisposition
        );
	BAIL_ON_NT_STATUS(status);

cleanup:
    return status;

error:
    goto cleanup;
}

REG_API
NTSTATUS
NtRegCloseKey(
    IN HANDLE hRegConnection,
    IN HKEY hKey
    )
{
    return RegTransactCloseKey(
        hRegConnection,
        hKey
        );
}

REG_API
NTSTATUS
NtRegDeleteKeyA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCSTR pszSubKey
    )
{
	NTSTATUS status = 0;
    PWSTR pwszSubKey = NULL;

    BAIL_ON_NT_INVALID_POINTER(pszSubKey);

    status = LwRtlWC16StringAllocateFromCString(&pwszSubKey,
    		                                     pszSubKey);
    BAIL_ON_NT_STATUS(status);

    status = RegTransactDeleteKeyW(
        hRegConnection,
        hKey,
        pwszSubKey);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);

    return status;

error:
    goto cleanup;
}

REG_API
NTSTATUS
NtRegDeleteKeyW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    )
{
    return RegTransactDeleteKeyW(
        hRegConnection,
        hKey,
        pSubKey
        );
}

REG_API
NTSTATUS
NtRegDeleteKeyValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN OPTIONAL PCSTR pszValueName
    )
{
	NTSTATUS status = 0;
    PWSTR pwszSubKey = NULL;
    PWSTR pwszValueName = NULL;

    if (pszSubKey)
    {
        status = LwRtlWC16StringAllocateFromCString(&pwszSubKey,
        		                                     pszSubKey);
        BAIL_ON_NT_STATUS(status);
    }

    if (pszValueName)
    {
        status = LwRtlWC16StringAllocateFromCString(&pwszValueName,
        		                                     pszValueName);
        BAIL_ON_NT_STATUS(status);
    }

    status = RegTransactDeleteKeyValueW(
                           hRegConnection,
                           hKey,
                           pwszSubKey,
                           pwszValueName);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    LWREG_SAFE_FREE_MEMORY(pwszValueName);

    return status;

error:
    goto cleanup;
}

REG_API
NTSTATUS
NtRegDeleteKeyValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
    return RegTransactDeleteKeyValueW(
        hRegConnection,
        hKey,
        pSubKey,
        pValueName
        );
}

REG_API
NTSTATUS
NtRegDeleteTreeA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey
    )
{
	NTSTATUS status = 0;
    PWSTR pwszSubKey = NULL;

    if (pszSubKey)
    {
        status = LwRtlWC16StringAllocateFromCString(&pwszSubKey,
        		                                     pszSubKey);
        BAIL_ON_NT_STATUS(status);
    }

    status = RegTransactDeleteTreeW(
                               hRegConnection,
                               hKey,
                               pwszSubKey);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    return status;

error:
    goto cleanup;
}

REG_API
NTSTATUS
NtRegDeleteTreeW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey
    )
{
    return RegTransactDeleteTreeW(
        hRegConnection,
        hKey,
        pSubKey
        );
}

REG_API
NTSTATUS
NtRegDeleteValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCSTR pszValueName
    )
{
	NTSTATUS status = 0;
    PWSTR pwszValueName = NULL;

    BAIL_ON_NT_INVALID_STRING(pszValueName);

    status = LwRtlWC16StringAllocateFromCString(&pwszValueName,
    		                                    pszValueName);
    BAIL_ON_NT_STATUS(status);

    status = RegTransactDeleteValueW(
        hRegConnection,
        hKey,
        pwszValueName
        );
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszValueName);

    return status;
error:
    goto cleanup;
}

REG_API
NTSTATUS
NtRegDeleteValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCWSTR pValueName
    )
{
    return RegTransactDeleteValueW(
        hRegConnection,
        hKey,
        pValueName
        );
}

REG_API
NTSTATUS
NtRegEnumKeyExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PSTR pszName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT OPTIONAL PSTR pszClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
	NTSTATUS status = 0;
    PWSTR pwszName = NULL;
    PSTR pszKeyName = NULL;
    PWSTR pwszClass = NULL;
    DWORD cName = *pcName;
    DWORD cTempName = *pcName;

    if (cName == 0)
    {
		status = STATUS_BUFFER_TOO_SMALL;
		BAIL_ON_NT_STATUS(status);
    }

    status = LW_RTL_ALLOCATE(&pwszName, WCHAR, sizeof(*pwszName)*(cName));
    BAIL_ON_NT_STATUS(status);

    if (pcClass)
    {
        if (*pcClass == 0)
        {
    		status = STATUS_BUFFER_TOO_SMALL;
    		BAIL_ON_NT_STATUS(status);
        }

        status = LW_RTL_ALLOCATE(&pwszClass, WCHAR, sizeof(*pwszClass)*(*pcClass));
        BAIL_ON_NT_STATUS(status);
    }

	status = RegTransactEnumKeyExW(
        hRegConnection,
        hKey,
        dwIndex,
        pwszName,
        &cTempName,
        pReserved,
        pwszClass,
        pcClass,
        pftLastWriteTime
        );
	BAIL_ON_NT_STATUS(status);

	status = LwRtlCStringAllocateFromWC16String(&pszKeyName, (PCWSTR)pwszName);
	BAIL_ON_NT_STATUS(status);

	if (cName < strlen(pszKeyName))
	{
		status = STATUS_BUFFER_TOO_SMALL;
		BAIL_ON_NT_STATUS(status);
	}

	memcpy((PBYTE)pszName, (PBYTE)pszKeyName, strlen(pszKeyName));
	*pcName = strlen(pszKeyName);

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszName);
    LWREG_SAFE_FREE_MEMORY(pwszClass);
    LWREG_SAFE_FREE_STRING(pszKeyName);

    return status;

error:
    goto cleanup;
}

REG_API
NTSTATUS
NtRegEnumKeyExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PWSTR pName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
    return RegTransactEnumKeyExW(
        hRegConnection,
        hKey,
        dwIndex,
        pName,
        pcName,
        pReserved,
        pClass,
        pcClass,
        pftLastWriteTime
        );
}


REG_API
NTSTATUS
NtRegEnumValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PSTR pszValueName,
    IN OUT PDWORD pcchValueName,
    IN OPTIONAL PDWORD pReserved,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
	NTSTATUS status = 0;
    DWORD dwType = REG_NONE;
    PWSTR pwszValueName = NULL;
    PSTR pszTempValueName = NULL;
    PVOID pTempData = NULL;
    DWORD cTempData = 0;
    DWORD dwValueNameLen = 0;
    PBYTE pValue = NULL;

    if (*pcchValueName == 0)
    {
		status = STATUS_BUFFER_TOO_SMALL;
		BAIL_ON_NT_STATUS(status);
    }

    if (pData && !pcbData)
    {
    	status = STATUS_INVALID_PARAMETER;
    	BAIL_ON_NT_STATUS(status);
    }
    dwValueNameLen = *pcchValueName;

    status = LW_RTL_ALLOCATE(&pwszValueName, WCHAR, sizeof(*pwszValueName)*(dwValueNameLen));
    BAIL_ON_NT_STATUS(status);

    if (pData && pcbData)
    {
    	status = RtlSafeMultiplyULONG(&cTempData, *pcbData, sizeof(WCHAR));
    	BAIL_ON_NT_STATUS(status);

    	if (cTempData)
    	{
            status = LW_RTL_ALLOCATE(&pTempData, VOID, cTempData);
            BAIL_ON_NT_STATUS(status);
    	}
    }

	status = RegTransactEnumValueW(
	        hRegConnection,
	        hKey,
	        dwIndex,
	        pwszValueName,
	        &dwValueNameLen,
	        pReserved,
	        &dwType,
	        pTempData,
	        &cTempData);
	BAIL_ON_NT_STATUS(status);

	if (!cTempData)
	{
		goto done;
	}

    /* Don't really know size of REG_SZ data, so must retrieve it */
    if (dwType == REG_SZ)
    {
        cTempData *= sizeof(WCHAR);
        dwValueNameLen = *pcchValueName;
        LWREG_SAFE_FREE_MEMORY(pTempData);
        status = LW_RTL_ALLOCATE(&pTempData, BYTE, cTempData);
        BAIL_ON_NT_STATUS(status);
    	status = RegTransactEnumValueW(
                    hRegConnection,
                    hKey,
                    dwIndex,
                    pwszValueName,
                    &dwValueNameLen,
                    pReserved,
                    &dwType,
                    pTempData,
                    &cTempData);
        BAIL_ON_NT_STATUS(status);
    }

	if (!pTempData)
	{
		goto done;
	}

	if (REG_SZ == dwType)
	{
		status = LwRtlCStringAllocateFromWC16String((PSTR*)&pValue, (PCWSTR)pTempData);
		BAIL_ON_NT_STATUS(status);

		cTempData = strlen((PSTR)pValue) + 1;
	}
	else if (REG_MULTI_SZ == dwType)
	{
		status = NtRegConvertByteStreamW2A((PBYTE)pTempData,
				                           cTempData,
				                           &pValue,
				                           &cTempData);
		BAIL_ON_NT_STATUS(status);
	}
	else
	{
	    status = LW_RTL_ALLOCATE(&pValue, VOID, cTempData);
	    BAIL_ON_NT_STATUS(status);

		memcpy(pValue, pTempData, cTempData);
	}

    if (pData)
    {
    	memcpy(pData, pValue, cTempData);
    }

done:
    status = LwRtlCStringAllocateFromWC16String(&pszTempValueName,
    		                                     (PCWSTR)pwszValueName);
    BAIL_ON_NT_STATUS(status);

    memcpy((PBYTE)pszValueName, (PBYTE)pszTempValueName, (strlen(pszTempValueName)+1));
    *pcchValueName = strlen(pszTempValueName);

    if (pdwType)
    {
        *pdwType = dwType;
    }

    if (pcbData)
    {
    	*pcbData = cTempData;
    }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszValueName);
    LWREG_SAFE_FREE_MEMORY(pTempData);
    LWREG_SAFE_FREE_MEMORY(pValue);
    LWREG_SAFE_FREE_STRING(pszTempValueName);

    return status;

error:
    if (pdwType)
    {
        *pdwType = REG_NONE;
    }
    if (pcbData)
    {
	    *pcbData = 0;
    }
    *pcchValueName = 0;

    goto cleanup;
}

REG_API
NTSTATUS
NtRegEnumValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pValueName,
    IN OUT PDWORD pcchValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegTransactEnumValueW(
        hRegConnection,
        hKey,
        dwIndex,
        pValueName,
        pcchValueName,
        pReserved,
        pType,
        pData,
        pcbData
        );
}

REG_API
NTSTATUS
NtRegGetValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN OPTIONAL PCSTR pszValueName,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
	NTSTATUS status = 0;
    DWORD dwType = REG_NONE;
    PWSTR pwszSubKey = NULL;
    PWSTR pwszValueName = NULL;
    PVOID pTempData = NULL;
    DWORD cTempData = 0;
    PBYTE pValue = NULL;

    if (pvData && !pcbData)
    {
    	status = STATUS_INVALID_PARAMETER;
    	BAIL_ON_NT_STATUS(status);
    }

    if (pszSubKey)
    {
    	status = LwRtlWC16StringAllocateFromCString(&pwszSubKey, pszSubKey);
    	BAIL_ON_NT_STATUS(status);
    }

    if (pszValueName)
    {
    	status = LwRtlWC16StringAllocateFromCString(&pwszValueName, pszValueName);
    	BAIL_ON_NT_STATUS(status);
    }

    if (pvData && pcbData)
    {
    	cTempData = (*pcbData)*sizeof(WCHAR);
	    status = LW_RTL_ALLOCATE(&pTempData, VOID, cTempData*sizeof(WCHAR));
	    BAIL_ON_NT_STATUS(status);
    }

	status = RegTransactGetValueW(
        hRegConnection,
        hKey,
        pwszSubKey,
        pwszValueName,
        Flags,
        &dwType,
        pTempData,
        &cTempData);
	BAIL_ON_NT_STATUS(status);

    /* Don't really know size of REG_SZ data, so must retrieve it */
    if (dwType == REG_SZ)
    {
        cTempData *= sizeof(WCHAR);
        LWREG_SAFE_FREE_MEMORY(pTempData);
        status = LW_RTL_ALLOCATE(&pTempData, BYTE, cTempData);
        BAIL_ON_NT_STATUS(status);
    	status = RegTransactGetValueW(
                    hRegConnection,
                    hKey,
                    pwszSubKey,
                    pwszValueName,
                    Flags,
                    &dwType,
                    pTempData,
                    &cTempData);
                    BAIL_ON_NT_STATUS(status);
    }

	if (!pTempData)
	{
		goto done;
	}

	if (REG_SZ == dwType)
	{
		status = LwRtlCStringAllocateFromWC16String((PSTR*)&pValue, (PCWSTR)pTempData);
		BAIL_ON_NT_STATUS(status);

		cTempData = strlen((PSTR)pValue) + 1;
	}
	else if (REG_MULTI_SZ == dwType)
	{
		status = NtRegConvertByteStreamW2A((PBYTE)pTempData,
				                           cTempData,
				                           &pValue,
				                           &cTempData);
		BAIL_ON_NT_STATUS(status);
	}
	else
	{
	    status = LW_RTL_ALLOCATE(&pValue, VOID, cTempData);
	    BAIL_ON_NT_STATUS(status);

		memcpy(pValue, pTempData, cTempData);
	}

    if (pvData)
    {
    	memcpy(pvData, pValue, cTempData);
    }

done:
    if (pdwType)
    {
        *pdwType = dwType;
    }

    if (pcbData)
    {
    	*pcbData = cTempData;
    }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    LWREG_SAFE_FREE_MEMORY(pwszValueName);
    LWREG_SAFE_FREE_MEMORY(pTempData);
    LWREG_SAFE_FREE_MEMORY(pValue);

    return status;

error:
    if (pdwType)
    {
        *pdwType = REG_NONE;
    }

    if (pcbData)
    {
	    *pcbData = 0;
    }

    goto cleanup;
}

REG_API
NTSTATUS
NtRegGetValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegTransactGetValueW(
        hRegConnection,
        hKey,
        pSubKey,
        pValue,
        Flags,
        pdwType,
        pvData,
        pcbData
        );
}

REG_API
NTSTATUS
NtRegOpenKeyExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    )
{
	NTSTATUS status = 0;
    PWSTR pwszSubKey = NULL;

    if (pszSubKey)
    {
        status = LwRtlWC16StringAllocateFromCString(&pwszSubKey,
        		                                     pszSubKey);
        BAIL_ON_NT_STATUS(status);
    }

	status = RegTransactOpenKeyExW(
        hRegConnection,
        hKey,
        pwszSubKey,
        ulOptions,
        AccessDesired,
        phkResult);
	BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);

    return status;

error:
    goto cleanup;
}

REG_API
NTSTATUS
NtRegOpenKeyExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    )
{
    return RegTransactOpenKeyExW(
        hRegConnection,
        hKey,
        pSubKey,
        ulOptions,
        AccessDesired,
        phkResult
        );
}

REG_API
NTSTATUS
NtRegQueryInfoKeyA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    OUT OPTIONAL PSTR pszClass,
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen,
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PULONG pulSecDescLen,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
	NTSTATUS status = 0;
    PWSTR pwszClass = NULL;
    DWORD dwIndex = 0;
    DWORD cValues = 0;
    DWORD cbData = 0;
    DWORD cMaxValueLen = 0;
    CHAR valueName[MAX_KEY_LENGTH] = {0};
    DWORD cValueName = MAX_KEY_LENGTH;
    DWORD dwType = 0;

    if (pcClass)
    {
        if (*pcClass == 0)
        {
    		status = STATUS_BUFFER_TOO_SMALL;
    		BAIL_ON_NT_STATUS(status);
        }

	    status = LW_RTL_ALLOCATE(&pwszClass, WCHAR, sizeof(*pwszClass)*(*pcClass));
	    BAIL_ON_NT_STATUS(status);
    }

	status = NtRegQueryInfoKeyW(
        hRegConnection,
        hKey,
        pwszClass,
        pcClass,
        pReserved,
        pcSubKeys,
        pcMaxSubKeyLen,
        pcMaxClassLen,
        &cValues,
        pcMaxValueNameLen,
        NULL,
        pulSecDescLen,
        pftLastWriteTime);
	BAIL_ON_NT_STATUS(status);

    if (pcMaxSubKeyLen)
    {
        *pcMaxSubKeyLen *= LW_WC2MBS_FACTOR_MAX;
    }
    if (pcMaxValueNameLen)
    {
        *pcMaxValueNameLen *= LW_WC2MBS_FACTOR_MAX;
    }

	for (; dwIndex < cValues; dwIndex++)
	{
		memset(valueName, 0, MAX_KEY_LENGTH);
		cValueName = MAX_KEY_LENGTH;
		cbData = 0;

		status = NtRegEnumValueA(hRegConnection,
				                hKey,
				                dwIndex,
				                valueName,
				                &cValueName,
				                NULL,
                                &dwType,
		                        NULL,
		                        &cbData);
		BAIL_ON_NT_STATUS(status);

    	if (REG_MULTI_SZ == dwType || REG_SZ == dwType)
        {
            cbData *= LW_WC2MBS_FACTOR_MAX;
        }
        if (cMaxValueLen < cbData)
		{
			cMaxValueLen = cbData;
		}
	}

	if (pcValues)
	{
		*pcValues = cValues;
	}

	if (pcMaxValueLen)
	{
		*pcMaxValueLen = cMaxValueLen;
	}

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszClass);

    return status;

error:
    if (pcValues)
    {
	    *pcValues = 0;
    }

    if (pcMaxValueLen)
    {
	    *pcMaxValueLen = 0;
    }

    goto cleanup;
}

REG_API
NTSTATUS
NtRegQueryInfoKeyW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen,
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PULONG pulSecDescLen,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
    return RegTransactQueryInfoKeyW(
        hRegConnection,
        hKey,
        pClass,
        pcClass,
        pReserved,
        pcSubKeys,
        pcMaxSubKeyLen,
        pcMaxClassLen,
        pcValues,
        pcMaxValueNameLen,
        pcMaxValueLen,
        pulSecDescLen,
        pftLastWriteTime
        );
}

REG_API
NTSTATUS
NtRegQueryMultipleValues(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    OUT PVALENT val_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValueBuf,
    IN OUT OPTIONAL PDWORD dwTotsize
    )
{
    return RegTransactQueryMultipleValues(
        hRegConnection,
        hKey,
        val_list,
        num_vals,
        pValueBuf,
        dwTotsize
        );
}

REG_API
NTSTATUS
NtRegQueryValueExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
	return NtRegGetValueA(hRegConnection,
			            hKey,
			            NULL,
			            pszValueName,
			            RRF_RT_REG_NONE,
			            pType,
			            pData,
			            pcbData);
}

REG_API
NTSTATUS
NtRegQueryValueExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
	return NtRegGetValueW(hRegConnection,
			            hKey,
			            NULL,
			            pValueName,
			            RRF_RT_REG_NONE,
			            pType,
			            pData,
			            pcbData);
}

REG_API
NTSTATUS
NtRegSetValueExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    )
{
	NTSTATUS status = 0;
    PWSTR pwszValueName = NULL;
    PBYTE   pOutData = NULL;
    DWORD   cbOutDataLen = 0;
    BOOLEAN bIsStrType = FALSE;

    if (pszValueName)
    {
        status = LwRtlWC16StringAllocateFromCString(&pwszValueName,
        		                                     pszValueName);
        BAIL_ON_NT_STATUS(status);
    }

    if (dwType == REG_MULTI_SZ)
    {
        if (!pData)
        {
            pData = (PBYTE) "";
        }
        if (cbData == 0)
        {
            cbData = 1;
        }
    }
    if (pData)
    {
        if (REG_MULTI_SZ == dwType)
        {
                status = NtRegConvertByteStreamA2W((PBYTE)pData,
                                                          cbData,
                                                          &pOutData,
                                                          &cbOutDataLen);
                BAIL_ON_NT_STATUS(status);
 
                bIsStrType = TRUE;
        }
        else if (REG_SZ == dwType)
    	{
            /* Verify correct null termination of input data */
            if (strlen((char *) pData) != (cbData-1))
            {
                status = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(status);
            }

    		status = LwRtlWC16StringAllocateFromCString((PWSTR*)&pOutData,
    				                                     (PCSTR)pData);
    		BAIL_ON_NT_STATUS(status);

            cbOutDataLen = (mbstrlen((const char*) pData)+1) * sizeof(WCHAR);
    		bIsStrType = TRUE;
    	}
    }

    status = RegTransactSetValueExW(
            hRegConnection,
            hKey,
            pwszValueName,
            Reserved,
            dwType,
            bIsStrType ? pOutData : pData,
            bIsStrType ? cbOutDataLen : cbData);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszValueName);
    LWREG_SAFE_FREE_MEMORY(pOutData);

    return status;
error:
    goto cleanup;
}

REG_API
NTSTATUS
NtRegSetValueExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    )
{
    if (dwType == REG_MULTI_SZ)
    {
        if (!pData)
        {
            pData = (PBYTE) "\0\0";
        }
        if (cbData == 0)
        {
            cbData = sizeof(WCHAR);
        }
    }
    return RegTransactSetValueExW(
        hRegConnection,
        hKey,
        pValueName,
        Reserved,
        dwType,
        pData,
        cbData
        );
}

REG_API
NTSTATUS
NtRegSetKeySecurity(
	IN HANDLE hNtRegConnection,
	IN HKEY hKey,
	IN SECURITY_INFORMATION SecurityInformation,
	IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
	IN ULONG pSecDescLen
	)
{
    return RegTransactSetKeySecurity(
    	    hNtRegConnection,
    	    hKey,
    	    SecurityInformation,
    	    pSecDescRel,
    	    pSecDescLen
    	    );
}

REG_API
NTSTATUS
NtRegGetKeySecurity(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN OUT PULONG pulSecDescLen
    )
{
	return RegTransactGetKeySecurity(
	    	    hNtRegConnection,
	    	    hKey,
	    	    SecurityInformation,
	    	    pSecDescRel,
	    	    pulSecDescLen
	    	    );
}


NTSTATUS
NtRegSetValueAttributesA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN PCSTR pszValueName,
    IN PLWREG_VALUE_ATTRIBUTES_A pValueAttributes
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NtRegSetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    )
{
    NTSTATUS status = 0;

    if (!RegValidValueAttributes(pValueAttributes))
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    status = RegTransactSetValueAttributesW(
            hRegConnection,
            hKey,
            pSubKey,
            pValueName,
            pValueAttributes);
    BAIL_ON_NT_STATUS(status);

error:

    return status;
}

NTSTATUS
NtRegGetValueAttributesA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN PCSTR pszValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES_A* ppValueAttributes
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NtRegGetValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    )
{
    return RegTransactGetValueAttributesW(
        hRegConnection,
        hKey,
        pwszSubKey,
        pwszValueName,
        ppCurrentValue,
        ppValueAttributes
        );
}

NTSTATUS
NtRegDeleteValueAttributesA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN PCSTR pszValueName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NtRegDeleteValueAttributesW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName
    )
{
    return RegTransactDeleteValueAttributesW(
            hRegConnection,
            hKey,
            pwszSubKey,
            pwszValueName
            );
}

