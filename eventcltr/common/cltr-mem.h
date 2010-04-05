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
#ifndef __EVTMEM_H__
#define __EVTMEM_H__

#define CLTR_SAFE_FREE_MEMORY(mem) \
        do {                      \
           if (mem) {             \
              CltrFreeMemory(mem); \
           }                      \
        } while(0);

DWORD
CltrAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    );

DWORD
CltrReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    size_t sSize
    );


void
CltrFreeMemory(
    PVOID pMemory
    );


DWORD
CltrAllocateString(
    PCWSTR pszInputString, 
    PWCHAR *ppszOutputString
    );


void
CltrFreeString(
    WCHAR * pszString
    );

void
CltrFreeStringArray(
    PWSTR * ppStringArray,
    DWORD dwCount
    );

#ifdef _WIN32
 void __RPC_FAR * __RPC_USER midl_user_allocate(size_t cBytes);

 void __RPC_USER midl_user_free(void __RPC_FAR * p);
#endif

#endif /* __EVTMEM_H__ */
