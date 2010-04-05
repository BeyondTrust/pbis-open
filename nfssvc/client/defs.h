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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        macros.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Common private macros for rpc client library
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#ifndef _DEFS_H_
#define _DEFS_H_

#define NFSSVC_DEFAULT_PROT_SEQ   "ncacn_np"
#define NFSSVC_DEFAULT_ENDPOINT   "\\pipe\\nfssvc"
#define NFSSVC_LOCAL_ENDPOINT     "/var/lib/likewise/rpc/nfssvc"

#define DCERPC_CALL(status, fn_call)                               \
    do {                                                           \
        NTSTATUS ntStatus = STATUS_SUCCESS;                        \
        dcethread_exc *dceexc;                                     \
                                                                   \
        DCETHREAD_TRY                                              \
        {                                                          \
            dceexc = NULL;                                         \
            (status) = fn_call;                                    \
        }                                                          \
        DCETHREAD_CATCH_ALL(dceexc)                                \
        {                                                          \
            ntStatus = LwRpcStatusToNtStatus(dceexc->match.value); \
            (status) = LwNtStatusToWin32Error(ntStatus);           \
        }                                                          \
        DCETHREAD_ENDTRY;                                          \
    } while (0);

#endif /* _DEFS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
