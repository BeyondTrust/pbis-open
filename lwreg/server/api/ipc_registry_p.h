/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
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

