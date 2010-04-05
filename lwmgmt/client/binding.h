/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Client RPC Binding
 *
 */
#ifndef __LWMGMTBINDING_H__
#define __LWMGMTBINDING_H__

DWORD
LWMGMTCreateRpcBinding(
    const char * hostname,
    char**       ppszBindingString,
    handle_t *   binding
    );

DWORD
LWMGMTFreeRpcBinding(
    handle_t binding,
    char * pszBindingString
    );

#endif /* __LWMGMTBINDING_H__ */

