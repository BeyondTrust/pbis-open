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
 *        Likewise Task System (LWTASK)
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
