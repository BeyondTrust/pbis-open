/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "../includes.h"


DWORD
ADUBuildCredContext(
    PCSTR              pszDomain,
    PCSTR              pszUserUPN,
    BOOLEAN            bOnline,
    PADU_CRED_CONTEXT* ppCredContext
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    PADU_CRED_CONTEXT pCredContext = NULL;

    dwError = LwAllocateMemory(
                    sizeof(ADU_CRED_CONTEXT),
                    (PVOID*)&pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    if (IsNullOrEmptyString(pszUserUPN))
    {
        /* Set default credentials to the machine's */
        dwError = ADUInitKrb5(pszDomain);
        BAIL_ON_MAC_ERROR(dwError);

        dwError = ADUKrb5GetSystemCachePath(&pCredContext->pszCachePath);
        BAIL_ON_MAC_ERROR(dwError);
    }
    else
    {
        dwError = ADUKrb5GetUserCachePathAndSID(
                        pszUserUPN,
                        &pCredContext->pszCachePath,
                        &pCredContext->pszHomeDirPath,
                        &pCredContext->pszSID,
                        &pCredContext->uid);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (bOnline)
    {
        dwError = ADUKrb5GetPrincipalName(
                        pCredContext->pszCachePath,
                        &pCredContext->pszPrincipalName);
        BAIL_ON_MAC_ERROR(dwError);

        status = LwIoCreateKrb5CredsA(
                        pCredContext->pszPrincipalName,
                        pCredContext->pszCachePath,
                        &pCredContext->pAccessToken);
        if (status != STATUS_SUCCESS)
        {
            // TODO: Map NTSTATUS to winerror
            dwError = status;
        }
        BAIL_ON_MAC_ERROR(dwError);
    }

    *ppCredContext = pCredContext;

cleanup:

    return dwError;

error:

    *ppCredContext = NULL;

    if (pCredContext)
    {
        ADUFreeCredContext(pCredContext);
    }

    goto cleanup;
}

DWORD
ADUActivateCredContext(
    PADU_CRED_CONTEXT pCredContext
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = LwIoSetThreadCreds(pCredContext->pAccessToken);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    return dwError;

error:

    // TODO: Map ntstatus to winerror
    dwError = ntStatus;

    goto cleanup;
}

DWORD
ADUDeactivateCredContext(
    PADU_CRED_CONTEXT pCredContext
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = LwIoSetThreadCreds(NULL);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    return dwError;

error:

    // TODO: Map ntstatus to winerror
    dwError = ntStatus;

    goto cleanup;
}

VOID
ADUFreeCredContext(
    PADU_CRED_CONTEXT pCredContext
    )
{
    if (pCredContext->pAccessToken)
    {
        LwIoDeleteCreds(pCredContext->pAccessToken);
    }

    if (pCredContext->pszCachePath)
    {
        if (pCredContext->bDestroyCachePath)
        {
            ADUKerb5DestroyCache(pCredContext->pszCachePath);
        }

        LwFreeMemory(pCredContext->pszCachePath);
    }

    LW_SAFE_FREE_STRING(pCredContext->pszHomeDirPath);
    LW_SAFE_FREE_STRING(pCredContext->pszPrincipalName);
    LW_SAFE_FREE_STRING(pCredContext->pszSID);

    LwFreeMemory(pCredContext);
}
