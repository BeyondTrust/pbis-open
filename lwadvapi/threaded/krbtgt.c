/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 *        Likewise Advanced API (lwadvapi)
 * 
 *        Kerberos 5 runtime environment
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak <rszczesniak@likewisesoftware.com>
 *
 */

#include "includes.h"


// 12 hours in seconds
#define LW_KRB5_DEFAULT_TKT_LIFE (12 * 60 * 60)


static
DWORD
LwKrb5GetTgtImpl(
    PCSTR             pszUserPrincipal,
    PCSTR             pszPassword,
    PCSTR             pszCcPath,
    PDWORD            pdwGoodUntilTime,
    krb5_preauthtype  *pPreauthTypes,
    DWORD             dwNumPreauthTypes,
    krb5_prompter_fct prompter,
    void              *prompter_data
    );
 
static
krb5_error_code 
cbKrb5Prompter(
    krb5_context ctx, 
    void *data, 
    const char *name,
    const char *banner, 
    int num_prompts, 
    krb5_prompt prompts[]
    );

DWORD
LwKrb5GetTgt(
    PCSTR  pszUserPrincipal,
    PCSTR  pszPassword,
    PCSTR  pszCcPath,
    PDWORD pdwGoodUntilTime
    )
{
    // Always try sending PA-ENC-TIMESTAMP. This saves a round trip because
    // otherwise Windows will return a KRB5KDC_ERR_PREAUTH_REQUIRED reply.
    //
    // Worse still, if the user is marked as preauth not required, and no
    // preauth is given, the TGT will not contain a PAC. If the TGT is used
    // without a PAC, it will not have the same permissions since Windows uses
    // the PAC for group membership checks.
    krb5_preauthtype pPreauthTypes[] = { KRB5_PADATA_ENC_TIMESTAMP };

    return LwKrb5GetTgtImpl(
                pszUserPrincipal,
                pszPassword,
                pszCcPath,
                pdwGoodUntilTime,
                pPreauthTypes,
                sizeof(pPreauthTypes) / sizeof(pPreauthTypes[0]),
                NULL,
                NULL);
}

DWORD
LwKrb5GetTgtWithSmartCard(
    PCSTR  pszUserPrincipal,
    PCSTR  pszPassword,
    PCSTR  pszCcPath,
    PDWORD pdwGoodUntilTime
    )
{
    // Only try the pkinit preauthentication types.
    krb5_preauthtype pPreauthTypes[] = {
        KRB5_PADATA_PK_AS_REQ_OLD,
        KRB5_PADATA_PK_AS_REP_OLD,
        KRB5_PADATA_PK_AS_REQ,
        KRB5_PADATA_PK_AS_REP,
    };
    void *pvPrompterData = &pszPassword;

    return LwKrb5GetTgtImpl(
                pszUserPrincipal,
                NULL,
                pszCcPath,
                pdwGoodUntilTime,
                pPreauthTypes,
                sizeof(pPreauthTypes) / sizeof(pPreauthTypes[0]),
                cbKrb5Prompter,
                pvPrompterData
                );
}

DWORD
LwKrb5DestroyCache(
    IN PCSTR pszCcPath
    )
{
    DWORD dwError = 0;
    krb5_error_code krbError = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;

    krbError = krb5_init_context(&ctx);
    dwError = LW_KRB5_ERROR_TO_LW_ERROR(krbError, ctx);
    BAIL_ON_LW_ERROR(dwError);

    krbError = krb5_cc_resolve(ctx, pszCcPath, &cc);
    if (KRB5_FCC_NOFILE == krbError)
    {
        krbError = 0;
        goto error;
    }
    dwError = LW_KRB5_ERROR_TO_LW_ERROR(krbError, ctx);
    BAIL_ON_LW_ERROR(dwError);

    krbError = krb5_cc_destroy(ctx, cc);
    // This always frees the cc reference, even on error.
    cc = NULL;
    if (KRB5_FCC_NOFILE == krbError)
    {
        dwError = 0;
        goto error;
    }
    dwError = LW_KRB5_ERROR_TO_LW_ERROR(krbError, ctx);
    BAIL_ON_LW_ERROR(dwError);

error:
    if (cc)
    {
        // ctx must be valid.
        krb5_cc_close(ctx, cc);
    }

    if (ctx)
    {
       krb5_free_context(ctx);
    }

    return dwError;
}

static
DWORD
LwKrb5GetTgtImpl(
    PCSTR             pszUserPrincipal,
    PCSTR             pszPassword,
    PCSTR             pszCcPath,
    PDWORD            pdwGoodUntilTime,
    krb5_preauthtype  *pPreauthTypes,
    DWORD             dwNumPreauthTypes,
    krb5_prompter_fct prompter,
    void              *prompter_data
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;
    krb5_get_init_creds_opt opts;
    krb5_principal client = NULL;
    krb5_creds creds = {0};
    PSTR pszPass = NULL;
    PSTR pszUPN = NULL;
    PSTR pszRealmIdx = NULL;
    BOOLEAN bUnlockExistingClientLock = FALSE;
    PWSTR pwszPass = NULL;

    dwError = LwAllocateString(
                    pszUserPrincipal,
                    &pszUPN);
    BAIL_ON_LW_ERROR(dwError);

    if ((pszRealmIdx = strchr(pszUPN, '@')) == NULL) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    LwStrToUpper(++pszRealmIdx);

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    krb5_get_init_creds_opt_init(&opts);
    krb5_get_init_creds_opt_set_tkt_life(&opts, LW_KRB5_DEFAULT_TKT_LIFE);
    krb5_get_init_creds_opt_set_forwardable(&opts, TRUE);
    krb5_get_init_creds_opt_set_preauth_list(
                &opts,
                pPreauthTypes,
                dwNumPreauthTypes);

    if (LW_IS_NULL_OR_EMPTY_STR(pszCcPath)) {
        ret = krb5_cc_default(ctx, &cc);
        BAIL_ON_KRB_ERROR(ctx, ret);
                
    } else {
        ret = krb5_cc_resolve(ctx, pszCcPath, &cc);
        BAIL_ON_KRB_ERROR(ctx, ret);
    }

    ret = krb5_parse_name(ctx, pszUPN, &client);
    BAIL_ON_KRB_ERROR(ctx, ret);
 
    if (!LW_IS_NULL_OR_EMPTY_STR(pszPassword)) {
        dwError = LwAllocateString(pszPassword, &pszPass);
        BAIL_ON_LW_ERROR(dwError);

        // The converted string is not used, but the error code is.
        // krb5_get_init_creds_will return EINVAL if it cannot convert the name
        // from UTF8 to UCS2. By pretesting the string first, we know it is
        // convertable.
        dwError = LwMbsToWc16s(pszPass, &pwszPass);
        BAIL_ON_LW_ERROR(dwError);
    }

    ret = krb5_get_init_creds_password(ctx, &creds, client, pszPass,
                                       prompter, prompter_data, 0, NULL,
                                       &opts);
    BAIL_ON_KRB_ERROR(ctx, ret);

    dwError = pthread_mutex_lock(&gLwKrb5State.ExistingClientLock);
    BAIL_ON_LW_ERROR(dwError);
    bUnlockExistingClientLock = TRUE;
    
    /* Blow away the old credentials cache so that the old TGT is removed.
     * Otherwise, the new TGT will be stored at the end of the credentials
     * cache, and kerberos will use the old TGT instead.
     *
     * See bug 6908.
     */
    ret = krb5_cc_initialize(ctx, cc, client);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_store_cred(ctx, cc, &creds);
    BAIL_ON_KRB_ERROR(ctx, ret);
    
    if (pdwGoodUntilTime)
    {
    	*pdwGoodUntilTime = creds.times.endtime;
    }

cleanup:

    if (bUnlockExistingClientLock)
    {
        pthread_mutex_unlock(&gLwKrb5State.ExistingClientLock);
    }

    if (ctx) {
        
        if (client)
        {
            krb5_free_principal(ctx, client);
        }
        
        if (cc)
        {
            krb5_cc_close(ctx, cc);
        }

        krb5_free_cred_contents(ctx, &creds);
        
        krb5_free_context(ctx);
    }

    LW_SAFE_FREE_STRING(pszUPN);
    if (pwszPass)
    {
        size_t len;
        if (!LwWc16sLen(pwszPass, &len))
        {
            memset(pwszPass, 0, len * sizeof(WCHAR));
        }
        LW_SAFE_FREE_MEMORY(pwszPass);
    }
    LW_SECURE_FREE_STRING(pszPass);

    return dwError;
    
error:

    if (pdwGoodUntilTime)
    {
    	*pdwGoodUntilTime = 0;
    }

    goto cleanup;
}
 
static
krb5_error_code 
cbKrb5Prompter(
    krb5_context ctx, 
    void *data, 
    const char *name,
    const char *banner, 
    int num_prompts, 
    krb5_prompt prompts[]
    )
{
        krb5_error_code ret = 0;
        int cb = 0;
        const char **ppszPIN = (const char **) data;

        if (num_prompts != 1)
        {
                LW_RTL_LOG_ERROR("cbKrb5Prompter: num_prompts invalid (%d != 1)",
                        num_prompts);
                ret = KRB5KRB_ERR_GENERIC;
                goto error;
        }

        LW_RTL_LOG_ERROR("cbKrb5Prompter(%s, %s): %s", name, banner,
                prompts[0].prompt);

        if (!strncmp(prompts[0].prompt, "Password for ", 13))
        {
                LW_RTL_LOG_DEBUG("cbKrb5Prompter: prompted for password rather than PIN");
                goto error;
        }

        if (ppszPIN == NULL || *ppszPIN == NULL)
        {
                /*
                 * No PIN means we were called once before, which
                 * means the preauth attempt failed (bad PIN, bad
                 * card, etc).
                 */
                LW_RTL_LOG_DEBUG("cbKrb5Prompter: no saved PIN");
                goto error;
        }

        cb = strlen(*ppszPIN);
        if (cb > prompts[0].reply->length)
        {
                LW_RTL_LOG_ERROR("cbKrb5Prompter: No room for PIN in reply buffer (%ld < %ld)",
                        (long) prompts[0].reply->length, (long) cb);
                goto error;
        }

        LW_RTL_LOG_DEBUG("cbKrb5Prompter: returning PIN");
        memcpy(prompts[0].reply->data, *ppszPIN, cb+1);
        prompts[0].reply->length = cb;
        *ppszPIN = NULL;

cleanup:
        return ret;

error:
        if (prompts && prompts[0].reply)
        {
            prompts[0].reply->length = 0;

            if (prompts[0].reply->data)
            {
                prompts[0].reply->data[0] = '\0';
            }
        }

        ret = KRB5_PREAUTH_FAILED;
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
