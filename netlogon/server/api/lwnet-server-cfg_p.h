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
 *        lwnet-cfg.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#ifndef __LWNET_SERVER_CFG_H__
#define __LWNET_SERVER_CFG_H__


DWORD
LWNetSrvReadRegistry(
   );

PCSTR
LWNetConfigGetPluginPath(
    VOID
    );

DWORD
LWNetConfigGetPingAgainTimeoutSeconds(
    VOID
    );

DWORD
LWNetConfigGetNegativeCacheTimeoutSeconds(
    VOID
    );

DWORD
LWNetConfigGetWritableRediscoveryTimeoutSeconds(
    VOID
    );

DWORD
LWNetConfigGetWritableTimestampMinimumChangeSeconds(
    VOID
    );

DWORD
LWNetConfigGetCLdapMaximumConnections(
    VOID
    );

DWORD
LWNetConfigGetCLdapSearchTimeoutSeconds(
    VOID
    );

DWORD
LWNetConfigGetCLdapSingleConnectionTimeoutSeconds(
    VOID
    );

DWORD
LWNetConfigIsNetBiosEnabled(
    VOID
    );

DWORD
LWNetConfigIsNetBiosUdpTimeout(
    VOID
    );

VOID
LwNetConfigGetWinsServers(
    PSTR *primaryServer,
    PSTR *secondaryServer
    );


#endif /* __LWNET_SERVER_CFG_H__ */
