/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        net_localgroupgetinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Network Management API, aka LanMan API (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <wchar.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/utsname.h>

#include <wc16str.h>
#include <wc16printf.h>
#include <gssapi/gssapi.h>
#include <keytab.h>
#include <dce/rpc.h>
#include <dce/smb.h>
#include <lw/ntstatus.h>
#include <lw/winerror.h>
#include <lwldap-error.h>
#include <lwmem.h>
#include <lwstr.h>
#include <lwtime.h>
#include <openssl/md4.h>
#include <openssl/md5.h>
#include <openssl/rc4.h>
#include <openssl/rand.h>
#include <openssl/des.h>
#include <ldap.h>
#include <lwps/lwps.h>
#include <lwio/lwio.h>

#include <lwrpc/types.h>
#include <lwrpc/unicodestring.h>
#include <lwrpc/samr.h>
#include <lwrpc/lsa.h>
#include <lwrpc/netlogon.h>
#include <lwrpc/allocate.h>
#include <lwrpc/memptr.h>
#include <lwrpc/sidhelper.h>
#include <lwrpc/LM.h>

#include <random.h>
#include <crypto.h>
#include <md5.h>
#include <rc4.h>
#include <des.h>

#include "net_connection.h"
#include "net_user.h"
#include "net_util.h"
#include "net_memory.h"
#include "net_hostinfo.h"
#include "net_info.h"
#include "net_userinfo.h"
#include "net_groupinfo.h"
#include "net_memberinfo.h"
#include "net_getdcname.h"
#include "net_crypto.h"
#include "ldaputil.h"
#include "joinlocal.h"
#include "unjoinlocal.h"
#include "machinepassword.h"
#include "externs.h"



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
