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
 *        lsarpcsrv.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef __LSARPCSRV_H__
#define __LSARPCSRV_H__

#include "lsautils.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwsecurityidentifier.h"


typedef DWORD (*PFNSTARTRPCSRV)(void);

typedef DWORD (*PFNSTOPRPCSRV)(void);
    

typedef struct rpcsrv_function_table {
    PFNSTARTRPCSRV          pfnStart;
    PFNSTOPRPCSRV           pfnStop;
} LSA_RPCSRV_FUNCTION_TABLE, *PLSA_RPCSRV_FUNCTION_TABLE;


#define LSA_SYMBOL_NAME_INITIALIZE_RPCSRV    "LsaInitializeRpcSrv"

typedef DWORD (*PFNINITIALIZERPCSRV)(
    PSTR* ppszRpcSrvName,
    PLSA_RPCSRV_FUNCTION_TABLE* ppFnTable
    );

#define LSA_SYMBOL_NAME_SHUTDOWN_RPCSRV      "LsaShutdownRpcSrv"

typedef DWORD (*PFNSHUTDOWNRPCSRV)(
    PCSTR pszProviderName,
    PLSA_RPCSRV_FUNCTION_TABLE pFnTable
    );


#define BAIL_ON_DCERPC_ERROR(st)                            \
    if ((st) != rpc_s_ok) {                                 \
        LSA_LOG_DEBUG("DCE/RPC error at %s:%d [0x%08x]\n",  \
                      __FILE__, __LINE__, (st));            \
        dwError = LW_ERROR_DCERPC_ERROR;                   \
        goto error;                                         \
    }
        

#endif /* __LSARPCSRV_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
