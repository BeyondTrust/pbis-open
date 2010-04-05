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

#include "includes.h"

DWORD
LWMGMTAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    )
{
    DWORD dwError = 0;
    PVOID pMemory = NULL;

    pMemory = malloc(dwSize);
    if (!pMemory){
        dwError = ENOMEM;
        *ppMemory = NULL;
    }else {
        memset(pMemory,0, dwSize);
        *ppMemory = pMemory;
    }
    return (dwError);
}

DWORD
LWMGMTReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    )
{
    DWORD dwError = 0;
    PVOID pNewMemory = NULL;

    if (pMemory == NULL) {
        pNewMemory = malloc(dwSize);
        memset(pNewMemory, 0, dwSize);
    }else {
        pNewMemory = realloc(pMemory, dwSize);
    }
    if (!pNewMemory){
        dwError = ENOMEM;
        *ppNewMemory = NULL;
    }else {
        *ppNewMemory = pNewMemory;
    }

    return(dwError);
}


void
LWMGMTFreeMemory(
    PVOID pMemory
    )
{
    free(pMemory);
}


DWORD
LWMGMTAllocateString(
    PCSTR  pszInputString,
    PSTR* ppszOutputString
    )
{
    DWORD dwError = 0;
    DWORD dwLen = 0;
    char * pszOutputString = NULL;

    if (!pszInputString || !*pszInputString){
        dwError = EINVAL;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    dwLen = (DWORD)strlen(pszInputString);
    dwError = LWMGMTAllocateMemory(dwLen+1, (PVOID *)&pszOutputString);
    BAIL_ON_LWMGMT_ERROR(dwError);

    strcpy(pszOutputString, pszInputString);

error:

    *ppszOutputString = pszOutputString;

    return(dwError);
}

void
LWMGMTFreeString(
    PSTR pszString
    )
{
    if (pszString) {
        LWMGMTFreeMemory(pszString);
    }
}

void
LWMGMTFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    )
{
    DWORD i;

    if ( ppStringArray ) {
        for (i = 0; i < dwCount; i++)
        {
            if (ppStringArray[i]) {
                LWMGMTFreeString(ppStringArray[i]);
            }
        }

        LWMGMTFreeMemory(ppStringArray);
    }

    return;
}

#ifdef _WIN32
 void __RPC_FAR * __RPC_USER midl_user_allocate(size_t cBytes)
{
    return((void __RPC_FAR *) malloc(cBytes));
}

 void __RPC_USER midl_user_free(void __RPC_FAR * p)
{
    free(p);
}
#endif
