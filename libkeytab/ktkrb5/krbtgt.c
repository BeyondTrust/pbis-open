/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        krbtgt.c
 *
 * Abstract:
 *
 *        Kerberos 5 keytab management library
 * 
 *        Credentials handling
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 *
 */

#include "includes.h"

#define KRB5_DEFAULT_OPTIONS  0
#define KRB5_DEFAULT_TKT_LIFE 60*60*12 /* 12 hours */


DWORD
KtKrb5GetTgt(
    PCSTR pszUserPrincipal,
    PCSTR pszPassword,
    PCSTR pszCcPath
    )
{
    DWORD dwError = KT_STATUS_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;
    krb5_get_init_creds_opt opts;
    krb5_principal client;
    krb5_creds creds = {0};
    PSTR pszPass = NULL;
    PSTR pszUPN = NULL;
    PSTR pszRealmIdx = NULL;

    dwError = KtAllocateString(pszUserPrincipal, &pszUPN);
    BAIL_ON_KT_ERROR(dwError);

    if ((pszRealmIdx = index(pszUPN, '@')) == NULL) {
        dwError = KT_STATUS_INVALID_PARAMETER;
        BAIL_ON_KT_ERROR(dwError);
    }

    /* Ensure realm name is uppercased */
    KtStrToUpper(++pszRealmIdx);

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    krb5_get_init_creds_opt_init(&opts);
    krb5_get_init_creds_opt_set_tkt_life(&opts, KRB5_DEFAULT_TKT_LIFE);
    krb5_get_init_creds_opt_set_forwardable(&opts, TRUE);

    if (IsNullOrEmptyString(pszCcPath)) {
        ret = krb5_cc_default(ctx, &cc);
        BAIL_ON_KRB5_ERROR(ctx, ret);
                
    } else {
        ret = krb5_cc_resolve(ctx, pszCcPath, &cc);
        BAIL_ON_KRB5_ERROR(ctx, ret);
    }

    ret = krb5_parse_name(ctx, pszUPN, &client);
    BAIL_ON_KRB5_ERROR(ctx, ret);
 
    ret = krb5_cc_initialize(ctx, cc, client);
    BAIL_ON_KRB5_ERROR(ctx, ret);
    
    if (!IsNullOrEmptyString(pszPassword)) {
        dwError = KtAllocateString(pszPassword, &pszPass);
        BAIL_ON_KT_ERROR(dwError);
    }

    ret = krb5_get_init_creds_password(ctx, &creds, client, pszPass, NULL,
                                       NULL, 0, NULL, &opts);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    if (ret == KRB5KRB_AP_ERR_SKEW) {
        //dwError = KtSyncTimeToDC(pszRealm);
        //BAIL_ON_KT_ERROR(dwError);

        ret = krb5_get_init_creds_password(ctx, &creds, client, pszPass, NULL,
                                           NULL, 0, NULL, &opts);
        BAIL_ON_KRB5_ERROR(ctx, ret);

    } else {
        BAIL_ON_KRB5_ERROR(ctx, ret);
    }

    ret = krb5_cc_store_cred(ctx, cc, &creds);
    BAIL_ON_KRB5_ERROR(ctx, ret);

error:
    if (ctx) {
        if (client) {
            krb5_free_principal(ctx, client);
        }

        if (cc) {
            krb5_cc_close(ctx, cc);
        }

        krb5_free_cred_contents(ctx, &creds);
        krb5_free_context(ctx);
    }

    KT_SAFE_FREE_STRING(pszUPN);
    KT_SAFE_CLEAR_FREE_STRING(pszPass);

    return dwError;
}


DWORD
KtKrb5GetTgs(
    PCSTR pszCliPrincipal,
    PCSTR pszSvcPrincipal,
    PCSTR pszCcPath
    )
{
    DWORD dwError = KT_STATUS_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;
    krb5_flags opts = 0;
    krb5_principal client, service;
    krb5_creds tgs_req = {0};
    krb5_creds* tgs_rep = NULL;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    if (IsNullOrEmptyString(pszCcPath)) {
        ret = krb5_cc_default(ctx, &cc);
        BAIL_ON_KRB5_ERROR(ctx, ret);
                
    } else {
        ret = krb5_cc_resolve(ctx, pszCcPath, &cc);
        BAIL_ON_KRB5_ERROR(ctx, ret);
    }

    ret = krb5_parse_name(ctx, pszCliPrincipal, &client);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    ret = krb5_parse_name(ctx, pszSvcPrincipal, &service);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    tgs_req.client        = client;
    tgs_req.server        = service;
    tgs_req.times.endtime = time(NULL) + KRB5_DEFAULT_TKT_LIFE;

    ret = krb5_get_credentials(ctx, opts, cc, &tgs_req, &tgs_rep);
    BAIL_ON_KRB5_ERROR(ctx, ret);

cleanup:

    if (ctx)
    {

        if (client) {
            krb5_free_principal(ctx, client);
        }

        if (service) {
            krb5_free_principal(ctx, service);
        }

        if (tgs_rep) {
            krb5_free_creds(ctx, tgs_rep);
        }

        if (cc) {
            krb5_cc_close(ctx, cc);
        }

        krb5_free_context(ctx);
    }

    return dwError;

error:

    goto cleanup;
}


DWORD
KtKrb5SetupLoginCreds(
    PCSTR         pszUsername,
    PCSTR         pszPassword,
    PCSTR         pszCachePath
    )
{
    DWORD dwError = KT_STATUS_SUCCESS;

    dwError = KtKrb5GetTgt(pszUsername, pszPassword, pszCachePath);
    BAIL_ON_KT_ERROR(dwError);

error:
    return dwError;
}


DWORD
KtKrb5GetServiceTicketForUser(
    uid_t         uid,
    PCSTR         pszUsername,
    PCSTR         pszServername,
    PCSTR         pszDomain,
    Krb5CacheType cacheType
    )
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    krb5_context    ctx = NULL;
    krb5_ccache     cc  = NULL;
    krb5_creds      in_creds = {0};
    krb5_creds*     pCreds = NULL;
    krb5_principal  user_principal = NULL;
    krb5_principal  server_principal = NULL;
    PSTR            pszCachePath = NULL;
    PSTR            pszTargetName = NULL;
    
    BAIL_ON_INVALID_STRING(pszUsername);
    BAIL_ON_INVALID_STRING(pszServername);

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB5_ERROR(ctx, ret);
    
    dwError = KtKrb5GetUserCachePath(
                    uid,
                    cacheType,
                    &pszCachePath);
    BAIL_ON_KT_ERROR(dwError);
            
    if (IsNullOrEmptyString(pszCachePath)) {
                
        ret = krb5_cc_default(ctx, &cc);
        BAIL_ON_KRB5_ERROR(ctx, ret);
                
    } else {
                
        ret = krb5_cc_resolve(ctx, pszCachePath, &cc);
        BAIL_ON_KRB5_ERROR(ctx, ret);
                
    }
    
    ret = krb5_parse_name(ctx, pszUsername, &user_principal);
    BAIL_ON_KRB5_ERROR(ctx, ret);
    
    dwError = KtAllocateStringPrintf(
                    &pszTargetName,
                    "%s$@%s",
                    pszServername,
                    pszDomain);
    BAIL_ON_KT_ERROR(dwError);
    
    KtStrToUpper(pszTargetName);

    ret = krb5_parse_name(ctx, pszTargetName, &server_principal);
    BAIL_ON_KRB5_ERROR(ctx, ret);
    
    ret = krb5_copy_principal(ctx, user_principal, &in_creds.client);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    ret = krb5_copy_principal(ctx, server_principal, &in_creds.server);
    BAIL_ON_KRB5_ERROR(ctx, ret);
    
    ret = krb5_get_credentials(
                ctx,
                0,
                cc,
                &in_creds,
                &pCreds);
    BAIL_ON_KRB5_ERROR(ctx, ret);
    
cleanup:

    if (ctx) {
    
        if (user_principal) {
            krb5_free_principal(ctx, user_principal);
        }
        
        if (server_principal) {
            krb5_free_principal(ctx, server_principal);
        }
    
        if (cc) {
            krb5_cc_close(ctx, cc);
        }
    
        krb5_free_cred_contents(ctx, &in_creds);
        
        if (pCreds) {
            krb5_free_creds(ctx, pCreds);
        }
    
        krb5_free_context(ctx);
    }

    KT_SAFE_FREE_STRING(pszCachePath);
    KT_SAFE_FREE_STRING(pszTargetName);
    
    return dwError;
    
error:

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
