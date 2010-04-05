#include "stdafx.h"

static struct
{
    DWORD dwError;
    LPWSTR pszMessage;
} gCamErrorMap[] =
{
    {
        ERROR_SUCCESS,
        L"No error"
    },
    {
        ERROR_INSUFFICIENT_BUFFER,
        L"The provided buffer is insufficient"
    },
    {
        ERROR_OUTOFMEMORY,
        L"Out of memory"
    },  
    {
    	ERROR_INTERNAL_ERROR,
        L"Internal error"
    },
    {
        ERROR_INVALID_PARAMETER,
        L"Invalid parameter"
    },
    {
        ERROR_INVALID_MESSAGE,
        L"The Inter Process message is invalid"
    },
    {
        ERROR_ACCESS_DENIED,
        L"Incorrect access attempt"
    },
	{
		NERR_UserExists,
		L"Computer Accont already exists"
	}
};

size_t
CamClientGetErrorString(
    DWORD dwError,
	LPWSTR pszBuffer,
    size_t stBufSize
    )
{
    size_t sErrIndex = 0;
    size_t sRequiredLen = 0;

    if (pszBuffer && stBufSize) {
       memset(pszBuffer, 0, stBufSize);
    }

    for (sErrIndex = 0; sErrIndex < sizeof(gCamErrorMap)/sizeof(gCamErrorMap[0]); sErrIndex++)
    {
        if (gCamErrorMap[sErrIndex].dwError == dwError)
        {
            sRequiredLen = wcslen(gCamErrorMap[sErrIndex].pszMessage) + 1;
            if (stBufSize >= sRequiredLen)
            {
                wcscpy(pszBuffer, gCamErrorMap[sErrIndex].pszMessage);
            }
            return sRequiredLen;
        }
    }

    sRequiredLen = wcslen(L"Unknown error") + 1;
    if (stBufSize >= sRequiredLen)
    {
        wcscpy(pszBuffer, L"Unknown error");
    }
    return sRequiredLen;
}














DWORD
CamClientAllocateMemory(
    size_t sSize,
    LPVOID* ppMemory
    )
{
	LPVOID pMemory = NULL;
	DWORD dwError = 0;

	pMemory = LocalAlloc(LPTR, sSize);
	if (!pMemory) {
		dwError = GetLastError();
		BAIL_ON_ERROR(dwError);
	}

	*ppMemory = pMemory;

	return dwError;

error:
	return dwError;
}

VOID
CamClientFreeMemory(
	LPVOID pMemory
	)
{
	LocalFree(pMemory);
}

DWORD
CamClientAllocateString(
	LPWSTR pszString,
	LPWSTR * ppszNewString
	)
{
	DWORD dwError = 0;
	return dwError;
}


VOID
CamClientFreeString(
	LPWSTR pszString
	)
{
	CamClientFreeMemory(pszString);
	return;
}




void __RPC_FAR * __RPC_USER midl_user_allocate(size_t cBytes)
{
	DWORD dwError = 0;
	LPVOID pMemory = NULL;
	dwError =  CamClientAllocateMemory(cBytes, &pMemory);
	if (dwError) {
		return NULL;
	}
	return pMemory;
}

void __RPC_USER midl_user_free(void __RPC_FAR * p)
{
    CamClientFreeMemory(p);
}

