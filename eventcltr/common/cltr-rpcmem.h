/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog RPC Memory Utilities
 *
 */
#ifndef __EVTRPCMEM_H__
#define __EVTRPCMEM_H__

#ifndef _WIN32
#include <compat/dcerpc.h>
#endif

DWORD
RPCAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    );

void
RPCFreeMemory(
    PVOID pMemory
    );


DWORD
RPCAllocateString(
    PCWSTR pszInputString, 
    PWSTR *ppszOutputString
    );


void
RPCFreeString(
    PWSTR pszString
    );

#endif /* __EVTMEM_H__ */
