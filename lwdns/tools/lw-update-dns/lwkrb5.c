/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "includes.h"

static
DWORD
DNSKrb5GetTGTFromKeytab(
    PCSTR  pszUserName,
    PCSTR  pszPassword,
    PCSTR  pszCachePath,
    PDWORD pdwGoodUntilTime
    );

static
DWORD
LsaKrb5SetDefaultCachePath(
    PCSTR pszCachePath,
    PSTR* ppszOrigCachePath
    );

static
DWORD
DNSKrb5DestroyCache(
    PCSTR pszCachePath
    );

DWORD
DNSKrb5Init(
    PCSTR pszAccountName,
    PCSTR pszDomain
    )
{
    DWORD dwError = 0;
    PSTR  pszUsername = NULL;

    if (!gbKrb5Initialized)
    {
        DWORD i = 0;
        DWORD j = 0;
        DWORD dwLength = strlen(pszAccountName) + strlen(pszDomain) + 2;

        dwError = DNSAllocateMemory(
                        dwLength,
                        (PVOID*)&pszUsername);
        BAIL_ON_LWDNS_ERROR(dwError);

        for (i = 0; i < strlen(pszAccountName); i++)
        {
            pszUsername[i] = toupper((int)pszAccountName[i]);
        }
        pszUsername[i++] = '@';
        for (; j  < strlen(pszDomain); j++)
        {
             pszUsername[i++] = toupper((int)pszDomain[j]);
        }

        dwError = DNSKrb5GetTGTFromKeytab(
                        pszUsername,
                        NULL,
                        gpszKrb5CachePath,
                        NULL);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = LsaKrb5SetDefaultCachePath(
                        gpszKrb5CachePath,
                        NULL);
        BAIL_ON_LWDNS_ERROR(dwError);

        gbKrb5Initialized = TRUE;
    }

cleanup:

    LWDNS_SAFE_FREE_STRING(pszUsername);

    return dwError;

error:

    goto cleanup;
}

DWORD
DNSKrb5Shutdown(
    VOID
    )
{
    DWORD dwError = 0;

    if (gbKrb5Initialized)
    {
        dwError = DNSKrb5DestroyCache(gpszKrb5CachePath);
        BAIL_ON_LWDNS_ERROR(dwError);

        gbKrb5Initialized = FALSE;
    }

error:

    return dwError;
}

static
DWORD
DNSKrb5GetTGTFromKeytab(
    PCSTR  pszUserName,
    PCSTR  pszPassword,
    PCSTR  pszCachePath,
    PDWORD pdwGoodUntilTime
    )
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_creds creds = { 0 };
    krb5_ccache cc = NULL;
    krb5_keytab keytab = 0;
    krb5_principal client_principal = NULL;
    krb5_get_init_creds_opt opts;

    ret = krb5_init_context(&ctx);
    BAIL_ON_LWDNS_KRB_ERROR(ctx, ret);

    ret = krb5_parse_name(ctx, pszUserName, &client_principal);
    BAIL_ON_LWDNS_KRB_ERROR(ctx, ret);

    /* use krb5_cc_resolve to get an alternate cache */
    ret = krb5_cc_resolve(ctx, pszCachePath, &cc);
    BAIL_ON_LWDNS_KRB_ERROR(ctx, ret);

    ret = krb5_kt_default(ctx, &keytab);
    BAIL_ON_LWDNS_KRB_ERROR(ctx, ret);

    krb5_get_init_creds_opt_init(&opts);
    krb5_get_init_creds_opt_set_forwardable(&opts, TRUE);

    ret = krb5_get_init_creds_keytab(
                    ctx,
                    &creds,
                    client_principal,
                    keytab,
                    0,    /* start time     */
                    NULL, /* in_tkt_service */
                    &opts  /* options        */
                    );
    BAIL_ON_LWDNS_KRB_ERROR(ctx, ret);

    ret = krb5_cc_initialize(ctx, cc, client_principal);
    BAIL_ON_LWDNS_KRB_ERROR(ctx, ret);

    ret = krb5_cc_store_cred(ctx, cc, &creds);
    BAIL_ON_LWDNS_KRB_ERROR(ctx, ret);

    if (pdwGoodUntilTime)
    {
        *pdwGoodUntilTime = creds.times.endtime;
    }

error:

    if (creds.client == client_principal) {
        creds.client = NULL;
    }

    if (ctx) {
        if (client_principal) {
            krb5_free_principal(ctx, client_principal);
        }

        if (keytab) {
            krb5_kt_close(ctx, keytab);
        }

        if (cc) {
            krb5_cc_close(ctx, cc);
        }

        krb5_free_cred_contents(ctx, &creds);

        krb5_free_context(ctx);
    }

    return(dwError);
}

static
DWORD
LsaKrb5SetDefaultCachePath(
    PCSTR pszCachePath,
    PSTR* ppszOrigCachePath
    )
{
    DWORD dwError       = 0;
    DWORD dwMajorStatus = 0;
    DWORD dwMinorStatus = 0;
    PSTR  pszOrigCachePath = NULL;

    // Set the default for gss
    dwMajorStatus = gss_krb5_ccache_name(
                            (OM_uint32 *)&dwMinorStatus,
                            pszCachePath,
                            (ppszOrigCachePath) ? (const char**)&pszOrigCachePath : NULL);
    BAIL_ON_SEC_ERROR(dwMajorStatus);

    if (ppszOrigCachePath) {
        if (!IsNullOrEmptyString(pszOrigCachePath)) {
            dwError = DNSAllocateString(pszOrigCachePath, ppszOrigCachePath);
            BAIL_ON_LWDNS_ERROR(dwError);
        } else {
            *ppszOrigCachePath = NULL;
        }
    }

cleanup:

    return dwError;

sec_error:
error:

    if (ppszOrigCachePath) {
        *ppszOrigCachePath = NULL;
    }

    goto cleanup;
}

static
DWORD
DNSKrb5DestroyCache(
    PCSTR pszCachePath
    )
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;

    ret = krb5_init_context(&ctx);
    BAIL_ON_LWDNS_KRB_ERROR(ctx, ret);

    /* use krb5_cc_resolve to get an alternate cache */
    ret = krb5_cc_resolve(ctx, pszCachePath, &cc);
    BAIL_ON_LWDNS_KRB_ERROR(ctx, ret);

    ret = krb5_cc_destroy(ctx, cc);
    if (ret != 0) {
        if (ret != KRB5_FCC_NOFILE) {
            BAIL_ON_LWDNS_KRB_ERROR(ctx, ret);
        } else {
            ret = 0;
        }
    }

error:

    if (ctx)
    {
       krb5_free_context(ctx);
    }

    return(dwError);
}
