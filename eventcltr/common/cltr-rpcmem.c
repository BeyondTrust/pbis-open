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

#include "cltr-base.h"

#ifdef _WIN32

#include <Rpc.h>

/* As mentioned in msdn "The Rpcss Memory Management Package is obsolete. It is recommended 
 * that midl_user_allocate and midl_user_free are used in its place."
 * http://msdn.microsoft.com/en-us/library/aa378480(VS.85).aspx 
 * Hence using midl_user_allocate and midl_user_free
 */

//#define RPC_SS_ALLOCATE(dwSize) RpcSsAllocate(dwSize)
//#define RPC_SS_FREE(node) RpcSsFree(node)

#define RPC_SS_ALLOCATE(dwSize) midl_user_allocate(dwSize)

#define RPC_SS_FREE(node) midl_user_free(node)

#else

#define RPC_SS_ALLOCATE(dwSzie) rpc_ss_allocate(dwSize)

#define RPC_SS_FREE(node) rpc_ss_free(node)

#endif

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
    PCWSTR  pszInputString,
    PWSTR* ppszOutputString
    )
{
    DWORD dwError = 0;
    DWORD dwLen = 0;
    WCHAR * pszOutputString = NULL;

    if (!pszInputString){
        dwError = EINVAL;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    dwLen = (DWORD)(sizeof(WCHAR)*(wc16slen(pszInputString)+1));

    dwError = RPCAllocateMemory(dwLen, (PVOID *)&pszOutputString);
    BAIL_ON_CLTR_ERROR(dwError);

    wc16scpy(pszOutputString,pszInputString);
    
error:

    *ppszOutputString = pszOutputString;

    return(dwError);
}

void
RPCFreeString(
    PWSTR pszString
    )
{
    if (pszString) {
        RPCFreeMemory(pszString);
    }
}

