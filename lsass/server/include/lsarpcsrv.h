/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsarpcsrv.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
