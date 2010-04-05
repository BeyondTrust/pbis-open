/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Server API
 *
 */
#ifndef __EVTSERVER_H__
#define __EVTSERVER_H__

DWORD
CltrRegisterForRPC(
    PWSTR pszServiceName,
    RPC_BINDING_VECTOR ** ppServerBinding
    );

DWORD
CltrListenForRPC();

DWORD
CltrUnregisterForRPC(
    RPC_BINDING_VECTOR * pServerBinding
    ); 

#endif /* __EVTSERVER_H__ */
