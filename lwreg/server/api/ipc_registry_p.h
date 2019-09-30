/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        ipc_registry_p.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#ifndef __IPC_REGISTRY_P_H__
#define __IPC_REGISTRY_P_H__

LWMsgStatus
RegSrvIpcEnumRootKeysW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcCreateKeyEx(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcCloseKey(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcDeleteKey(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcDeleteKeyValue(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcDeleteTree(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcDeleteValue(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcEnumKeyExW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcEnumValueW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcGetValueW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcOpenKeyExW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcQueryInfoKeyW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcQueryMultipleValues(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcSetValueExW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcSetKeySecurity(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcGetKeySecurity(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcSetValueAttibutesW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcGetValueAttibutesW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

LWMsgStatus
RegSrvIpcDeleteValueAttibutesW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

VOID
RegSrvFreeHandle(
    PVOID pData
    );

#endif /* __IPC_REGISTRY_P_H__ */

