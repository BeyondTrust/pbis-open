/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        api.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        LSA Server API (Private Header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "config.h"

#include "lsasystem.h"
#include <lsa/lsa.h>
#include <lwmsg/lwmsg.h>

#include <dce/rpc.h>
#include <dce/dcethread.h>
#include <lw/base.h>
#include <reg/reg.h>

#ifdef HAVE_EVENTLOG_H
#include <eventlog.h>
#endif

#include "lsadef.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwsecurityidentifier.h"
#include "lsautils.h"
#include "lsaregdef.h"
#include "lwmapsecurity-lsass.h"
#include <lwhash.h>

#include "lsasrvutils.h"
#include "lsaserver.h"
#include <lsa/provider.h>
#include "lsarpcsrv.h"
#include "rpcctl.h"
#include "directory.h"

#include "structs_p.h"
#include "auth_p.h"
#include "auth_provider_p.h"
#include "rpc_server_p.h"
#include "event_p.h"
#include "externs_p.h"
#include "session_p.h"
#include "state_p.h"
#include "metrics_p.h"
#include "status_p.h"
#include "config_p.h"

#include "lsasrvapi.h"
#include "lsasrvapi2.h"
#include "lsasrvprivilege.h"
#include "lsasrvprivilege-ipc.h"

#include "lsaipc-common.h"
#include "lsaipc.h"

#include "ipc_error_p.h"
#include "externs_p.h"

#include <lsarpcsrv.h>

#include "rpcctl-register.h"
#include "externs.h"

VOID
LsaSrvInitializeLock(
     PLSA_SRV_RWLOCK pLock
     );

VOID
LsaSrvAcquireRead(
     PLSA_SRV_RWLOCK pLock
     );

VOID
LsaSrvAcquireWrite(
     PLSA_SRV_RWLOCK pLock
     );

VOID
LsaSrvReleaseRead(
     PLSA_SRV_RWLOCK pLock
     );

VOID
LsaSrvReleaseWrite(
     PLSA_SRV_RWLOCK pLock
     );

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
