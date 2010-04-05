/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Memory Utilities
 *
 */


#include "cltr-base.h"

DWORD
CltrAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    )
{
    DWORD dwError = 0;
    PVOID pMemory = NULL;

#ifdef _WIN32
    pMemory = LocalAlloc(LPTR, dwSize);
    if (!pMemory) {
         dwError = GetLastError();
         BAIL_ON_CLTR_ERROR(dwError);
    }
#else
    pMemory = malloc(dwSize);
    if (!pMemory) {
         dwError = errno;
         BAIL_ON_CLTR_ERROR(dwError);
    }
    else
    {
        memset(pMemory, 0, dwSize);
    }
#endif

    *ppMemory = pMemory;
    return(dwError);

error:

    *ppMemory = NULL;
    return(dwError);

}
DWORD
CltrReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    size_t sSize
    )
{
    DWORD dwError = 0;
    PVOID pNewMemory = NULL;

#ifdef _WIN32
    pNewMemory = LocalReAlloc(pMemory, sSize, LPTR);
    if (!pNewMemory) {
         dwError = GetLastError();

         if (dwError == 8)
         {
             pNewMemory = LocalAlloc(LPTR, sSize);
             if (!pNewMemory)
             {
                 dwError = GetLastError();
                 BAIL_ON_CLTR_ERROR(dwError);
             }
             else
             {
                 dwError = 0;
                 CopyMemory(pNewMemory, pMemory, LocalSize(pMemory));
             }
         }
         else
             BAIL_ON_CLTR_ERROR(dwError);
    }
#else
    pNewMemory = realloc(pMemory, sSize);
    if (!pNewMemory) {
        dwError = errno;
         BAIL_ON_CLTR_ERROR(dwError);
    }
#endif

    *ppNewMemory = pNewMemory;
    return(dwError);

error:

    *ppNewMemory = NULL;
    return(dwError);
    
}


void
CltrFreeMemory(
    PVOID pMemory
    )
{
#ifdef _WIN32
    LocalFree(pMemory);
#else
    free(pMemory);
#endif
    return;
}


DWORD
CltrAllocateString(
    PCWSTR  pszInputString,
    PWCHAR* ppszOutputString
    )
{
#ifdef _WIN32
    DWORD dwError = 0;
    DWORD dwLen = 0;
    WCHAR * pszOutputString = NULL;

    if (!pszInputString || !*pszInputString){
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    dwLen = (DWORD)(sizeof(WCHAR)*(wcslen(pszInputString)+1));

    dwError = CltrAllocateMemory(dwLen, (PVOID *)&pszOutputString);
    BAIL_ON_CLTR_ERROR(dwError);

    wcscpy(pszOutputString, pszInputString);

error:

    *ppszOutputString = pszOutputString;

    return(dwError);
#else
    return RtlWC16StringDuplicate(
            ppszOutputString,
            pszInputString);
#endif
}

void
CltrFreeString(
    PWSTR pszString
    )
{
    if (pszString) {
        CltrFreeMemory(pszString);
    }
}

void
CltrFreeStringArray(
    PWSTR * ppStringArray,
    DWORD dwCount
    )
{
    DWORD i;

    if ( ppStringArray ) {
        for (i = 0; i < dwCount; i++)
        {
            if (ppStringArray[i]) {
                CltrFreeString(ppStringArray[i]);
            }
        }

        CltrFreeMemory(ppStringArray);
    }

    return;
}

#ifdef _WIN32
 void __RPC_FAR * __RPC_USER midl_user_allocate(size_t cBytes)
{
    DWORD dwError = 0;
    void __RPC_FAR * pMemory = NULL;

    dwError = CltrAllocateMemory((DWORD)cBytes, &pMemory);
    if (dwError) {
        return NULL;
    }else {
        return pMemory;
    }

}

 void __RPC_USER midl_user_free(void __RPC_FAR * p)
{
    CltrFreeMemory(p);
    
}
#endif
