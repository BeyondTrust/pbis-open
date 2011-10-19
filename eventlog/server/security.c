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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 *
 * Eventlog server security wrapper
 *
 */

#include "includes.h"
#include <gssapi/gssapi.h>


DWORD
LWIGetGSSSecurityContextInfo(
    gss_ctx_id_t gss_ctx,
    PSTR *pszClientName
    )
{
    DWORD dwError = EVT_ERROR_SUCCESS;
    int gss_rc = 0;
    OM_uint32 minor_status = 0;
    gss_name_t src = GSS_C_NO_NAME;
    gss_buffer_desc src_name = GSS_C_EMPTY_BUFFER;
    gss_OID src_type = GSS_C_NULL_OID;

    PSTR pszClient = NULL;

    /* Fetch security context information to make it available
       on the server side (e.g. for security checks) */
    gss_rc = gss_inquire_context(&minor_status,
                                 gss_ctx,
                                 &src,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
    if (gss_rc == GSS_S_COMPLETE) {

        /*
         * Get calling principal name
         */
        gss_rc = gss_display_name(&minor_status, src, &src_name, &src_type);
        if (gss_rc != GSS_S_COMPLETE) {
            /*
             * TODO: error handling
             */

        }

        dwError = EVTStrndup(src_name.value, src_name.length, &pszClient);
        BAIL_ON_EVT_ERROR(dwError);

    } else {
        /* error handling */
    }

    *pszClientName = pszClient;

cleanup:

    gss_release_buffer(&minor_status, &src_name);

    if (src)
    {
        gss_release_name(&minor_status, &src);
    }

    return dwError;

error:
    EVT_SAFE_FREE_STRING(pszClient);
    *pszClientName = NULL;

    goto cleanup;
}


DWORD
LWICheckGSSSecurity(
    gss_ctx_id_t    gss_ctx,
    PEVTALLOWEDDATA pAllowedData
    )
{
    DWORD dwError = EVT_ERROR_SUCCESS;
    PSTR pszClientName = NULL;
    PSTR pszServerName = NULL;

    // We require LSASS in order to parse the configuration data.
    // If we were not able to do so earlier, try it again here.

    if ( pAllowedData->configData && !pAllowedData->pAllowedTo )
    {
        dwError = EVTAccessGetData(
                      pAllowedData->configData,
                      &pAllowedData->pAllowedTo);
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = LWIGetGSSSecurityContextInfo(gss_ctx,
                                           &pszClientName);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAccessCheckData(
                  pszClientName,
                  pAllowedData->pAllowedTo);
    BAIL_ON_EVT_ERROR(dwError);

cleanup:
    EVT_SAFE_FREE_STRING(pszClientName);
    EVT_SAFE_FREE_STRING(pszServerName);

    return dwError;

error:
    goto cleanup;
}


DWORD
LWICheckSecurity(
    handle_t        hBindingHandle,
    PEVTALLOWEDDATA pAllowedData
    )
{
    DWORD dwError = EVT_ERROR_SUCCESS;
    volatile unsigned32 rpcError;
    unsigned char* pszStringBinding = NULL;
    unsigned char* pszProtocol = NULL;
    DWORD dwAuthnProtocol = 0;
    PVOID pMechCtx = NULL;
    unsigned32 uid = -1, gid = -1;
    rpc_transport_info_handle_t info = NULL;

    TRY
    {
        /*
         * This is expected to fail for local rpc.
         */
        rpc_binding_inq_security_context(
            hBindingHandle,
            (unsigned32*)&dwAuthnProtocol,
            &pMechCtx,
            (unsigned32*)&rpcError);
    }
    CATCH_ALL
    ENDTRY;

    TRY
    {
        rpc_binding_to_string_binding(
            hBindingHandle,
            &pszStringBinding,
            (unsigned32*)&rpcError);
    }
    CATCH_ALL
    ENDTRY;

    BAIL_ON_DCE_ERROR(dwError, rpcError);

    TRY
    {
        rpc_string_binding_parse(
            pszStringBinding,
            NULL,
            &pszProtocol,
            NULL,
            NULL,
            NULL,
            (unsigned32*)&rpcError);
    }
    CATCH_ALL
    ENDTRY;

    BAIL_ON_DCE_ERROR(dwError, rpcError);

    switch(dwAuthnProtocol)
    {
    case rpc_c_authn_gss_negotiate:
        dwError = LWICheckGSSSecurity(
                      (gss_ctx_id_t)pMechCtx,
                      pAllowedData);
        break;
        
    default:
        /* Always allow in local rpc */
        if (!strcmp((char*) pszProtocol, "ncalrpc"))
        {
            rpc_binding_inq_transport_info(hBindingHandle, &info, (unsigned32*) &rpcError);
            BAIL_ON_DCE_ERROR(dwError, rpcError);

            rpc_lrpc_transport_info_inq_peer_eid(info, &uid, &gid);

            if (uid == 0)
            {
                dwError = EVT_ERROR_SUCCESS;
            }
            else
            {
                dwError = EVT_ERROR_ACCESS_DENIED;
            }
        }
        else
        {
            dwError = EVT_ERROR_ACCESS_DENIED;
        }
        break;
    }

error:
    
    if (pszStringBinding)
    {
        rpc_string_free(&pszStringBinding, (unsigned32*)&rpcError);
    }

    if (pszProtocol)
    {
        rpc_string_free(&pszProtocol, (unsigned32*)&rpcError);
    }

    return dwError;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
