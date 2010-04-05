/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include <dce/rpcsts.h>
#include <lwrpc/types.h>
#include <lwrpc/errcheck.h>


/* This isn't exactly the facility code, so let's call it error type */
#define ERRTYPE_CODE(v)         ((v) & 0xffff0000)

#define NTSTATUS_ERROR          (0xc0000000)
#define DCERPC_EXCEPTION        (0x16c90000)


int IsNtStatusError(UINT32 v)
{
    return (ERRTYPE_CODE(v) == NTSTATUS_ERROR);
}


int IsDceRpcException(UINT32 v)
{
    return (ERRTYPE_CODE(v) == DCERPC_EXCEPTION);
}


int IsDceRpcConnError(UINT32 v)
{
    int conn_error = 0;

    if (!IsDceRpcException(v)) return 0;

    /* check if returned exception code is one of
       connection related exceptions */
    conn_error = (v == rpc_s_connection_closed ||
                  v == rpc_s_connect_timed_out ||
                  /* Invalid credentials is listed as a connection error
                   * because it may be specific to a given domain controller.
                   * That is, reconnecting to a different DC with the same
                   * credentials may work. */
                  v == rpc_s_invalid_credentials ||
                  v == rpc_s_auth_skew ||
                  v == rpc_s_cannot_connect);
    return conn_error;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
