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
 *        krb5.c
 *
 * Abstract:
 *
 *        BeyondTrust Task System (LWTASK)
 *
 *        Setup Kerberos Credentials
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

DWORD
LwTaskAcquireCredsW(
    PWSTR           pwszUsername,  /* IN     */
    PWSTR           pwszPassword,  /* IN     */
    PLW_TASK_CREDS* ppCreds        /* IN OUT */
    )
{
    DWORD dwError = 0;
    PSTR  pszUsername = NULL;
    PSTR  pszPassword = NULL;
    PLW_TASK_CREDS pCreds = NULL;

    dwError = LwWc16sToMbs(pwszUsername, &pszUsername);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszPassword, &pszPassword);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskAcquireCredsA(pszUsername, pszPassword, &pCreds);
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppCreds = pCreds;

cleanup:

    LW_SAFE_FREE_MEMORY(pszUsername);
    LW_SAFE_FREE_MEMORY(pszPassword);

    return dwError;

error:

    *ppCreds = NULL;

    if (pCreds)
    {
        LwTaskFreeCreds(pCreds);
    }

    goto cleanup;
}

DWORD
LwTaskAcquireCredsA(
    PCSTR           pszUsername,  /* IN     */
    PCSTR           pszPassword,  /* IN     */
    PLW_TASK_CREDS* ppCreds       /* IN OUT */
    )
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    PSTR   pszNewCachePath = NULL;
    PLW_TASK_CREDS pCreds = NULL;

    BAIL_ON_INVALID_POINTER(ppCreds);
    BAIL_ON_INVALID_STRING(pszUsername);

    dwError = LwAllocateMemory(sizeof(*pCreds), (PVOID*)&pCreds);
    BAIL_ON_LW_TASK_ERROR(dwError);

    ret = krb5_init_context(&pCreds->ctx);
    BAIL_ON_KRB_ERROR(pCreds->ctx, ret);

    /* Generates a new filed based credentials cache in /tmp.
     * The file will be owned by root and only accessible by root.
     */
    ret = krb5_cc_new_unique(pCreds->ctx, "FILE", "hint", &pCreds->cc);
    BAIL_ON_KRB_ERROR(pCreds->ctx, ret);

    dwError = LwAllocateStringPrintf(
                    &pszNewCachePath,
                    "%s:%s",
                    krb5_cc_get_type(pCreds->ctx, pCreds->cc),
                    krb5_cc_get_name(pCreds->ctx, pCreds->cc));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwKrb5GetTgt(pszUsername, pszPassword, pszNewCachePath, NULL);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwKrb5SetDefaultCachePath(
                    pszNewCachePath,
                    &pCreds->pszRestoreCache);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwIoCreateKrb5CredsA(
                    pszUsername,
                    pszNewCachePath,
                    &pCreds->pKrb5Creds);
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppCreds = pCreds;

cleanup:

    LW_SAFE_FREE_STRING(pszNewCachePath);

    return dwError;

error:

    *ppCreds = NULL;

    if (pCreds)
    {
        LwTaskFreeCreds(pCreds);
    }

    goto cleanup;
}

VOID
LwTaskFreeCreds(
    PLW_TASK_CREDS pCreds /* IN OUT */
    )
{
    if (pCreds->pKrb5Creds != NULL)
    {
        LwIoDeleteCreds(pCreds->pKrb5Creds);
    }

    if (pCreds->pszRestoreCache)
    {
        LwKrb5SetDefaultCachePath(pCreds->pszRestoreCache, NULL);

        LwFreeString(pCreds->pszRestoreCache);
    }

    if (pCreds->ctx != NULL)
    {
        if (pCreds->cc != NULL)
        {
            krb5_cc_destroy(pCreds->ctx, pCreds->cc);
        }

        krb5_free_context(pCreds->ctx);
    }

    LwFreeMemory(pCreds);
}
