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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        krb5.c
 *
 * Abstract:
 *
 *        Event forwarder from eventlogd to collector service
 *
 *        Kerberos ticket and credentials cache helper functions
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#include "includes.h"

DWORD
EfdTranslateKrb5Error(
    krb5_context ctx,
    krb5_error_code krbError,
    PCSTR pszFile,
    DWORD dwLine
    )
{
    DWORD dwError = EFD_ERROR_SUCCESS;
    PCSTR pszKrb5Error = NULL;

    if (ctx)
    {
        pszKrb5Error = krb5_get_error_message(ctx, krbError);
    }
    if (pszKrb5Error)
    {
        EFD_LOG_ERROR("KRB5 Error at %s:%d: [Code:%d] [Message: %s]",
                pszFile,
                dwLine,
                krbError,
                pszKrb5Error);
    }
    else
    {
        EFD_LOG_ERROR("KRB5 Error at %s:%d [Code:%d]",
                pszFile,
                dwLine,
                krbError);
    }

    dwError = STATUS_ENCRYPTION_FAILED;

    if (pszKrb5Error)
    {
        krb5_free_error_message(ctx, pszKrb5Error);
    }
    return dwError;
}

DWORD
EfdSrvRefreshTgt(
    krb5_context ctx,
    krb5_ccache cc,
    PCSTR pszMachPrincipal,
    PCWSTR pwszMachinePassword
    )
{
    DWORD dwError = 0;
    krb5_get_init_creds_opt opts;
    krb5_principal client = NULL;
    krb5_creds creds = {0};
    PSTR pszPass = NULL;
    krb5_error_code ret = 0;

    krb5_get_init_creds_opt_init(&opts);
    krb5_get_init_creds_opt_set_forwardable(&opts, TRUE);

    dwError = RtlCStringAllocateFromWC16String(
                    &pszPass,
                    pwszMachinePassword);
    BAIL_ON_EFD_ERROR(dwError);

    ret = krb5_parse_name(ctx, pszMachPrincipal, &client);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_get_init_creds_password(ctx, &creds, client, pszPass, NULL,
                                       NULL, 0, NULL, &opts);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_initialize(ctx, cc, client);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_store_cred(ctx, cc, &creds);
    BAIL_ON_KRB_ERROR(ctx, ret);

cleanup:
    RtlCStringFree(&pszPass);
    if (client)
    {
        krb5_free_principal(ctx, client);
    }
    krb5_free_cred_contents(ctx, &creds);
    return dwError;

error:
    goto cleanup;
}

DWORD
EfdSrvSetupCredCache(
    LW_PIO_CREDS* ppAccessToken
    )
{
    DWORD dwError = 0;
    PLWPS_PASSWORD_INFO pMachineAcctInfo = NULL;
    HANDLE hPasswordStore = (HANDLE)NULL;
    PWSTR pwszMachPrincipal = NULL;
    PWSTR pwszTgtPrincipal = NULL;
    PSTR pszMachPrincipal = NULL;
    PSTR pszTgtPrincipal = NULL;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;
    const char* szCachePath = "FILE:/var/lib/likewise/krb5cc_efd";
    const WCHAR wszCachePath[] = {'F','I','L','E',':','/','v','a','r','/','l','i','b','/','l','i','k','e','w','i','s','e','/','k','r','b','5','c','c','_','e','f','d',0};
    krb5_creds tgtSearch = {0};
    krb5_creds *pTgtFound = NULL;
    DWORD dwMajorStatus = 0;
    DWORD dwMinorStatus = 0;
    // Do not free
    PSTR pszCacheAssign = NULL;
    PCSTR pszCacheEnv = NULL;
    LW_PIO_CREDS pAccessToken = NULL;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    dwError = LwpsOpenPasswordStore(
                    LWPS_PASSWORD_STORE_DEFAULT,
                    &hPasswordStore);
    BAIL_ON_EFD_ERROR(dwError);
    
    dwError = LwpsGetPasswordByCurrentHostName(
                    hPasswordStore,
                    &pMachineAcctInfo);
    BAIL_ON_EFD_ERROR(dwError);

    dwError = LwRtlWC16StringAllocatePrintfW(
                    &pwszMachPrincipal,
                    L"%ws@%ws",
                    pMachineAcctInfo->pwszMachineAccount,
                    pMachineAcctInfo->pwszDnsDomainName);
    BAIL_ON_EFD_ERROR(dwError);

    dwError = RtlCStringAllocateFromWC16String(
                    &pszMachPrincipal,
                    pwszMachPrincipal);
    BAIL_ON_EFD_ERROR(dwError);

    ret = krb5_cc_resolve(ctx, szCachePath, &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_parse_name(ctx, pszMachPrincipal, &tgtSearch.client);
    BAIL_ON_KRB_ERROR(ctx, ret);

    dwError = LwRtlWC16StringAllocatePrintfW(
                    &pwszTgtPrincipal,
                    L"krbtgt/%ws@%ws",
                    pMachineAcctInfo->pwszDnsDomainName,
                    pMachineAcctInfo->pwszDnsDomainName);
    BAIL_ON_EFD_ERROR(dwError);

    dwError = RtlCStringAllocateFromWC16String(
                    &pszTgtPrincipal,
                    pwszTgtPrincipal);
    BAIL_ON_EFD_ERROR(dwError);

    ret = krb5_parse_name(ctx, pszTgtPrincipal, &tgtSearch.server);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_get_credentials(
                ctx,
                0,
                cc,
                &tgtSearch,
                &pTgtFound);
    if (ret == KRB5_FCC_NOFILE /* file does not exist */ ||
        ret == KRB5KRB_AP_ERR_TKT_EXPIRED /* the ticket expired */ ||
        ret == KRB5KDC_ERR_NEVER_VALID /* the ticket is about to expire? */ ||
        ret == KRB5_NO_TKT_IN_RLM /* cache is for the wrong principal */ )
    {
        dwError = EfdSrvRefreshTgt(
                    ctx,
                    cc,
                    pszMachPrincipal,
                    pMachineAcctInfo->pwszMachinePassword);
        BAIL_ON_EFD_ERROR(dwError);
    }
    else
    {
        BAIL_ON_KRB_ERROR(ctx, ret);
    }

    pszCacheEnv = getenv("KRB5CCNAME");
    if (!pszCacheEnv || strcmp(pszCacheEnv, szCachePath))
    {
        dwMajorStatus = gss_krb5_ccache_name(
                            (OM_uint32 *)&dwMinorStatus,
                            szCachePath,
                            NULL);
        BAIL_ON_SEC_ERROR(dwMajorStatus);

        dwError = LwRtlCStringAllocatePrintf(
                        &pszCacheAssign,
                        "KRB5CCNAME=%s",
                        szCachePath);
        BAIL_ON_EFD_ERROR(dwError);
        // Yes, this leaks memory, but there is no clean way to avoid that
        if (putenv(pszCacheAssign) < 0)
        {
            dwError = errno;
            BAIL_ON_EFD_ERROR(dwError);
        }
        pszCacheAssign = NULL;
    }

    dwError = LwIoCreateKrb5CredsW(
                pwszMachPrincipal,
                wszCachePath,
                &pAccessToken);
    BAIL_ON_EFD_ERROR(dwError);

    if (ppAccessToken)
    {
        *ppAccessToken = pAccessToken;
        pAccessToken = NULL;
    }

cleanup:
    RtlCStringFree(&pszCacheAssign);
    RtlWC16StringFree(&pwszTgtPrincipal);
    RtlWC16StringFree(&pwszMachPrincipal);
    RtlCStringFree(&pszTgtPrincipal);
    RtlCStringFree(&pszMachPrincipal);
    if (pAccessToken != NULL)
    {
        LwIoDeleteCreds(pAccessToken);
    }
    if (pMachineAcctInfo)
    {
        LwpsFreePasswordInfo(hPasswordStore, pMachineAcctInfo);
    }

    if (hPasswordStore != (HANDLE)NULL)
    {
       LwpsClosePasswordStore(hPasswordStore);
    }
    if (ctx)
    {
        if (pTgtFound)
        {
            krb5_free_creds(ctx, pTgtFound);
        }
        krb5_free_cred_contents(ctx, &tgtSearch);
        if (cc)
        {
            krb5_cc_close(ctx, cc);
        }
        krb5_free_context(ctx);
    }
    return dwError;

error:
    goto cleanup;
}
