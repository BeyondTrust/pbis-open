/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog RPC Memory utilities
 *
 */

#include "includes.h"

DWORD
RPCAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    )
{
    DWORD dwError = 0;
    PVOID pMemory = NULL;

    pMemory = RPC_SS_ALLOCATE(dwSize);
    if (!pMemory){
        dwError = ENOMEM;
        *ppMemory = NULL;
    }else {
        memset(pMemory,0, dwSize);
        *ppMemory = pMemory;
    }
    return (dwError);
}

void
RPCFreeMemory(
    PVOID pMemory
    )
{
    RPC_SS_FREE(pMemory);
}


DWORD
RPCAllocateString(
    PSTR  pszInputString,
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
    dwError = RPCAllocateMemory(dwLen+1, (PVOID *)&pszOutputString);
    BAIL_ON_LWMGMT_ERROR(dwError);

    strcpy(pszOutputString, pszInputString);

error:

    *ppszOutputString = pszOutputString;

    return(dwError);
}

void
RPCFreeString(
    PSTR pszString
    )
{
    if (pszString) {
        RPCFreeMemory(pszString);
    }
}

