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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        common.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Join to Active Directory
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "includes.h"

#define BAIL_ON_KRB_ERROR(ctx, ret) \
    do { \
        if (ret) \
        { \
           (dwError) = LwTranslateKrb5Error(ctx, ret, __FUNCTION__, __FILE__, __LINE__); \
           goto error; \
        } \
    } while (0)

typedef struct _LSA_CREDS_FREE_INFO
{
    BOOLEAN bKrbCreds;
    krb5_context ctx;
    krb5_ccache cc;
    PSTR pszRestoreCache;

    LW_PIO_CREDS pRestoreCreds;
} LSA_CREDS_FREE_INFO;

DWORD
LsaSetSMBAnonymousCreds(
    OUT PLSA_CREDS_FREE_INFO* ppFreeInfo
    )
{
    DWORD dwError = 0;
    LW_PIO_CREDS pNewCreds = NULL;
    LW_PIO_CREDS pOldCreds = NULL;
    PLSA_CREDS_FREE_INFO pFreeInfo = NULL;

    BAIL_ON_INVALID_POINTER(ppFreeInfo);

    dwError = LwIoCreatePlainCredsA(
        "",
        "",
        "",
        &pNewCreds);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pFreeInfo), (PVOID*)&pFreeInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwIoGetThreadCreds(&pOldCreds);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwIoSetThreadCreds(pNewCreds);
    BAIL_ON_LSA_ERROR(dwError);

    pFreeInfo->pRestoreCreds = pOldCreds;
    pFreeInfo->bKrbCreds = FALSE;
    pOldCreds = NULL;

cleanup:
    *ppFreeInfo = pFreeInfo;
    if (pOldCreds != NULL)
    {
        LwIoDeleteCreds(pOldCreds);
    }

    if (pNewCreds != NULL)
    {
        LwIoDeleteCreds(pNewCreds);
    }
    return dwError;

error:
    if (pFreeInfo)
    {
        LwFreeMemory(pFreeInfo);
        pFreeInfo = NULL;
    }
    goto cleanup;
}

DWORD
LsaSetSMBCreds(
    IN PCSTR pszUserPrincipalName,
    IN PCSTR pszPassword,
    IN BOOLEAN bSetDefaultCachePath,
    OUT PLSA_CREDS_FREE_INFO* ppFreeInfo
    )
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    PSTR pszNewCachePath = NULL;
    PCSTR  pszCacheName = NULL;
    PCSTR  pszCacheType = NULL;
    krb5_context ctx = 0;
    krb5_ccache cc = 0;
    LW_PIO_CREDS pNewCreds = NULL;
    LW_PIO_CREDS pOldCreds = NULL;
    PLSA_CREDS_FREE_INFO pFreeInfo = NULL;
    PSTR pszOldCachePath = NULL;
    BOOLEAN bSwitchedPath = FALSE;

    BAIL_ON_INVALID_POINTER(ppFreeInfo);
    BAIL_ON_INVALID_STRING(pszUserPrincipalName);

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* Generates a new filed based credentials cache in /tmp. The file will
     * be owned by root and only accessible by root.
     */
    ret = krb5_cc_new_unique(
            ctx,
            "FILE",
            "hint",
            &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    pszCacheType = krb5_cc_get_type(ctx, cc);
    pszCacheName = krb5_cc_get_name(ctx, cc);
    dwError = LwAllocateStringPrintf(&pszNewCachePath, "%s:%s", pszCacheType, pszCacheName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwKrb5GetTgt(
                pszUserPrincipalName,
                pszPassword,
                pszNewCachePath,
                NULL);
    if (dwError == LW_ERROR_KRB5KDC_ERR_C_PRINCIPAL_UNKNOWN)
    {
        dwError = LW_ERROR_INVALID_ACCOUNT;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (bSetDefaultCachePath)
    {
        LSA_LOG_DEBUG("Switching default credentials path for new access token"); 
        dwError = LwKrb5SetThreadDefaultCachePath(
                  pszNewCachePath,
                  &pszOldCachePath);
        BAIL_ON_LSA_ERROR(dwError);
        bSwitchedPath = TRUE;
    }

    dwError = LwIoCreateKrb5CredsA(
        pszUserPrincipalName,
        pszNewCachePath,
        &pNewCreds);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pFreeInfo), (PVOID*)&pFreeInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwIoGetThreadCreds(&pOldCreds);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwIoSetThreadCreds(pNewCreds);
    BAIL_ON_LSA_ERROR(dwError);

    pFreeInfo->ctx = ctx;
    pFreeInfo->cc = cc;
    pFreeInfo->pRestoreCreds = pOldCreds;
    pFreeInfo->pszRestoreCache = pszOldCachePath;
    pFreeInfo->bKrbCreds = TRUE;
    pOldCreds = NULL;

cleanup:
    *ppFreeInfo = pFreeInfo;
    if (pOldCreds != NULL)
    {
        LwIoDeleteCreds(pOldCreds);
    }

    if (pNewCreds != NULL)
    {
        LwIoDeleteCreds(pNewCreds);
    }
    LW_SAFE_FREE_STRING(pszNewCachePath);

    return dwError;

error:
    if (ctx != NULL)
    {
        if (cc != NULL)
        {
            krb5_cc_destroy(ctx, cc);
        }
        krb5_free_context(ctx);
    }

    if (pFreeInfo)
    {
        LwFreeMemory(pFreeInfo);
        pFreeInfo = NULL;
    }
    if (bSwitchedPath)
    {
        LwKrb5SetThreadDefaultCachePath(
                  pszOldCachePath,
                  NULL);
        LW_SAFE_FREE_STRING(pszOldCachePath);
    }

    goto cleanup;
}

void
LsaFreeSMBCreds(
    IN OUT PLSA_CREDS_FREE_INFO* ppFreeInfo
    )
{
    PLSA_CREDS_FREE_INFO pFreeInfo = *ppFreeInfo;

    if (!pFreeInfo)
    {
        goto cleanup;
    }

    LwIoSetThreadCreds(pFreeInfo->pRestoreCreds);
    if (pFreeInfo->pRestoreCreds != NULL)
    {
        LwIoDeleteCreds(pFreeInfo->pRestoreCreds);
    }
    
    if (pFreeInfo->bKrbCreds)
    {
        LwKrb5SetThreadDefaultCachePath(
                      pFreeInfo->pszRestoreCache,
                      NULL);
        LW_SAFE_FREE_STRING(pFreeInfo->pszRestoreCache);

        if (pFreeInfo->ctx != NULL)
        {
            if (pFreeInfo->cc != NULL)
            {
                krb5_cc_destroy(pFreeInfo->ctx, pFreeInfo->cc);
            }
            krb5_free_context(pFreeInfo->ctx);
        }
    }

    LwFreeMemory(pFreeInfo);

    *ppFreeInfo = NULL;

cleanup:
    return;
}
