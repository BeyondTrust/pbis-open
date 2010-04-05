#include "stdafx.h"

DWORD
CamAllocateMemory(
    size_t sSize,
    LPVOID* ppMemory
    )
{
    DWORD dwError = 0;
    PVOID pMemory = NULL;

    pMemory = LocalAlloc(LPTR, sSize);
    if (!pMemory)
	{
        dwError = GetLastError();;
        *ppMemory = NULL;    
	}
	else
	{     
        *ppMemory = pMemory;
    }

    return dwError;
}

VOID
CamFreeMemory(
	LPVOID pMemory
	)
{
	LocalFree(pMemory);
}

DWORD
CamAllocateString(
	LPWSTR pwszInputString,
	LPWSTR* ppwszOutputString
	)
{
	DWORD dwError = 0;	    
    size_t sLen = 0;
    PWSTR pwszOutputString = NULL;

    if (!pwszInputString)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    sLen = wcslen(pwszInputString);

    dwError = CamAllocateMemory(sizeof(pwszOutputString[0]) * (sLen + 1),
                               (PVOID)&pwszOutputString);
    BAIL_ON_ERROR(dwError);

    if (sLen)
    {
        wcsncpy_s(pwszOutputString, sLen + 1, pwszInputString, sLen);		
    }

    *ppwszOutputString = pwszOutputString;

cleanup:
    return dwError;

error:
    CAM_SAFE_FREE_MEMORY(pwszOutputString);

    *ppwszOutputString = NULL;
    goto cleanup;

	return dwError;
}


VOID
CamFreeString(
	LPWSTR pszString
	)
{
	CamFreeMemory(pszString);
	return;
}


void __RPC_USER midl_user_free(void __RPC_FAR * p)
{
    CamFreeMemory(p);
}


void __RPC_FAR * __RPC_USER midl_user_allocate(size_t cBytes)
{
	DWORD dwError = 0;
	LPVOID pMemory = NULL;
	dwError =  CamAllocateMemory(cBytes, &pMemory);
	if (dwError) {
		return NULL;
	}
	return pMemory;
}
