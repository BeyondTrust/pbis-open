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
 *        lsaserver.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Server
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LSASERVER_H__
#define __LSASERVER_H__

typedef struct __LSASERVERCONNECTIONCONTEXT {
    int    fd;
    uid_t  peerUID;
    gid_t  peerGID;
    short  bConnected;
    HANDLE hServer;
} LSASERVERCONNECTIONCONTEXT, *PLSASERVERCONNECTIONCONTEXT;

#define LSA_SAFE_FREE_CONNECTION_CONTEXT(pContext) \
	if (pContext)  {                               \
       LsaSrvFreeContext(pContext);                \
       pContext = NULL;                            \
    }

DWORD
LsaSrvOpenConnection(
    int     fd,
    uid_t   uid,
    gid_t   gid,
    PHANDLE phConnection
    );

void
LsaSrvCloseConnection(
    HANDLE hConnection
    );

void
LsaSrvHandleConnection(
    HANDLE hConnection
    );

LWMsgDispatchSpec*
LsaSrvGetDispatchSpec(
    void
    );

void
LsaSrvIpcDestructSession(
    LWMsgSecurityToken* pToken,
    void* pSessionData
    );

LWMsgStatus
LsaSrvIpcConstructSession(
    LWMsgSecurityToken* pToken,
    void* pData,
    void** ppSessionData
    );

#endif /* __LSASERVER_H__ */

