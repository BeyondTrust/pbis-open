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

typedef const struct
{
    PCSTR protocol;
    PCSTR endpoint;
    BOOLEAN bRegistrationRequired;
} ENDPOINT, *PENDPOINT;

struct _RPC_LW_EVENTLOG_CONNECTION
{
    // This value is initialized to &RpcEvtOpen and checked at the beginning of
    // RPC functions to verify the user did not mix up handle types.
    PVOID pMagic;

    PACCESS_TOKEN pUserToken;
    BOOLEAN ReadAllowed;
    BOOLEAN WriteAllowed;
    BOOLEAN DeleteAllowed;
};

DWORD
EVTRegisterInterface(
    VOID
    );

DWORD
EVTRegisterEndpoint(
    PSTR pszServiceName,
    PENDPOINT pEndpoint
    );

DWORD
EVTListen(
    VOID
    );

BOOLEAN
EVTIsListening(
    VOID
    );

DWORD
EVTStopListen(
    VOID
    );

DWORD
EVTUnregisterAllEndpoints(
    VOID
    );

#endif /* __EVTSERVER_H__ */
