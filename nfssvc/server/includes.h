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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * NfsSvc Server
 *
 */
#include <config.h>
#include <nfssvcsys.h>

#include <lw/winerror.h>
#include <lw/ntstatus.h>
#include <wc16str.h>
#include <lwmem.h>
#include <lwstr.h>
#include <lwerror.h>
#include <lwdscache.h>
#include <lw/base.h>
#include <lwio/lwio.h>
#include <lwio/ntfileapi.h>
#include <lwio/lmshare.h>
#include <lwio/lwshareinfo.h>
#include <dce/rpc.h>
#include <compat/dcerpc.h>
#include <lwrpc/lsa.h>
#include <lwnet.h>

#include <lw/nfssvc.h>

#include <nfssvcdefs.h>
#include <nfssvcutils.h>

#include <nfssvclog_r.h>

#include "nfssvc_srv.h"
#include "nfssvc_h.h"

#include "defs.h"
#include "structs.h"
#include "prototypes.h"
#include "marshall.h"

#include "externs.h"


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
