/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        credcontext.c
 *
 * Abstract:
 *
 *       Credential Context API
 *
 * Author: Glenn Curtis (glennc@likewisesoftware.com)
 *         Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "includes.h"

DWORD
ADUBuildCredContext(
    PCSTR              pszDomain,
    PCSTR              pszUserUPN,
    PADU_CRED_CONTEXT* ppCredContext
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    PADU_CRED_CONTEXT pCredContext = NULL;

    dwError = LWAllocateMemory(
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
        dwError = ADUKrb5GetUserCachePath(
                        pszUserUPN,
                        &pCredContext->pszCachePath);
        BAIL_ON_MAC_ERROR(dwError);
    }

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

        LWFreeMemory(pCredContext->pszCachePath);
    }

    LW_SAFE_FREE_STRING(pCredContext->pszPrincipalName);

    LWFreeMemory(pCredContext);
}
