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
 *        rpcmacros.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Common private macros for rpc client library
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#ifndef _RPC_MACROS_H_
#define _RPC_MACROS_H_

#define BAIL_ON_WIN_ERROR(err)    BAIL_ON_LSA_ERROR(err)

#define BAIL_ON_RPC_STATUS(st)    \
    if ((st) != RPC_S_OK) {       \
        LSA_LOG_DEBUG("RPC Error at %s:%d [code: %X]", \
                      __FILE__, __LINE__, (st));  \
        goto error;               \
    }

#define BAIL_ON_NULL_PTR(p, status)              \
    if ((p) == NULL) {                           \
        status = STATUS_INSUFFICIENT_RESOURCES;  \
        LSA_LOG_DEBUG("Error at %s:%d [code: %X]", \
                      __FILE__, __LINE__, (status));  \
        goto error;                              \
    }

#define BAIL_ON_INVALID_PTR(p, status)           \
    if ((p) == NULL) {                           \
        status = STATUS_INVALID_PARAMETER;       \
        LSA_LOG_DEBUG("Error at %s:%d [code: %X]", \
                      __FILE__, __LINE__, (status));  \
        goto error;                              \
    }

#define DCERPC_CALL(status, fn_call)                                 \
    do {                                                             \
        DCETHREAD_TRY                                                \
        {                                                            \
            (status) = fn_call;                                      \
        }                                                            \
        DCETHREAD_CATCH_ALL(dceexc)                                  \
        {                                                            \
            status = LwRpcStatusToNtStatus(dceexc->match.value);     \
            LSA_LOG_DEBUG("Converted DCERPC code 0x%08X to NTSTATUS 0x%08x", \
                          dceexc->match.value,                       \
                          status);                                   \
        }                                                            \
        DCETHREAD_ENDTRY;                                            \
    } while (0);

#define DCERPC_CALL_WINERR(winerr, fn_call)                          \
    do {                                                             \
        NTSTATUS ntstat;                                             \
                                                                     \
        DCETHREAD_TRY                                                \
        {                                                            \
            ntstat = STATUS_SUCCESS;                                 \
            (winerr) = fn_call;                                      \
        }                                                            \
        DCETHREAD_CATCH_ALL(dceexc)                                  \
        {                                                            \
            ntstat = LwRpcStatusToNtStatus(dceexc->match.value);     \
            winerr = LwNtStatusToWin32Error(ntstat);                 \
            LSA_LOG_DEBUG("Converted DCERPC code 0x%08X to WINERR %d", \
                          dceexc->match.value,                       \
                          winerr);                                   \
        }                                                            \
        DCETHREAD_ENDTRY;                                            \
    } while (0);


#endif /* _RPC_MACROS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
