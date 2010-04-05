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
LWMGMTGetRpcError(
    dcethread_exc* exCatch,
    DWORD dwLWMGMTError
    )
{
    DWORD dwError = 0;

#ifdef _WIN32
    dwError = dwLWMGMTError;
#else
    dwError = dcethread_exc_getstatus(exCatch);
    if(!dwError)
    {
        dwError = dwLWMGMTError;
    }
#endif //!_WIN32

    return dwError;
}

