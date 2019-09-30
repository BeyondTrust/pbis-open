/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        LMcreds.h
 *
 * Abstract:
 *
 *        Network Management API (a.k.a. LanMan API) rpc client library
 *
 *        Session credentials handling functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NetCreateKrb5CredentialsW(
    PWSTR              pwszPrincipal,
    PWSTR              pwszCache,
    NET_CREDS_HANDLE  *phCreds
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    LW_PIO_CREDS pCreds = NULL;

    BAIL_ON_INVALID_PTR(pwszPrincipal, err);
    BAIL_ON_INVALID_PTR(pwszCache, err);
    BAIL_ON_INVALID_PTR(phCreds, err);

    status = LwIoCreateKrb5CredsW(pwszPrincipal,
                                  pwszCache,
                                  &pCreds);
    BAIL_ON_NT_STATUS(status);

    *phCreds = pCreds;

cleanup:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(status);
    }

    return err;

error:
    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    goto cleanup;
}


NET_API_STATUS
NetCreateKrb5CredentialsA(
    PSTR               pszPrincipal,
    PSTR               pszCache,
    NET_CREDS_HANDLE  *phCreds
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    LW_PIO_CREDS pCreds = NULL;

    BAIL_ON_INVALID_PTR(pszPrincipal, err);
    BAIL_ON_INVALID_PTR(pszCache, err);
    BAIL_ON_INVALID_PTR(phCreds, err);

    status = LwIoCreateKrb5CredsA(pszPrincipal,
                                  pszCache,
                                  &pCreds);
    BAIL_ON_NT_STATUS(status);

    *phCreds = (NET_CREDS_HANDLE)pCreds;

cleanup:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(status);
    }

    return err;

error:
    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    goto cleanup;
}


NET_API_STATUS
NetCreateNtlmCredentialsW(
    PWSTR              pwszUsername,
    PWSTR              pwszPassword,
    PWSTR              pwszDomainName,
    DWORD              dwFlags,
    NET_CREDS_HANDLE  *phCreds
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    LW_PIO_CREDS pCreds = NULL;

    BAIL_ON_INVALID_PTR(pwszUsername, err);
    BAIL_ON_INVALID_PTR(pwszPassword, err);
    BAIL_ON_INVALID_PTR(pwszDomainName, err);
    BAIL_ON_INVALID_PTR(phCreds, err);

    status = LwIoCreatePlainCredsW(pwszUsername,
                                   pwszDomainName,
                                   pwszPassword,
                                   &pCreds);
    BAIL_ON_NT_STATUS(status);

    *phCreds = pCreds;

cleanup:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(status);
    }

    return err;

error:
    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    goto cleanup;
}


NET_API_STATUS
NetCreateNtlmCredentialsA(
    PSTR               pszUsername,
    PSTR               pszPassword,
    PSTR               pszDomainName,
    DWORD              dwFlags,
    NET_CREDS_HANDLE  *phCreds
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    LW_PIO_CREDS pCreds = NULL;

    BAIL_ON_INVALID_PTR(pszUsername, err);
    BAIL_ON_INVALID_PTR(pszPassword, err);
    BAIL_ON_INVALID_PTR(pszDomainName, err);
    BAIL_ON_INVALID_PTR(phCreds, err);

    status = LwIoCreatePlainCredsA(pszUsername,
                                   pszDomainName,
                                   pszPassword,
                                   &pCreds);
    BAIL_ON_NT_STATUS(status);

    *phCreds = pCreds;

cleanup:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(status);
    }

    return err;

error:
    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    goto cleanup;
}


NET_API_STATUS
NetSetCredentials(
    NET_CREDS_HANDLE hCreds
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    LW_PIO_CREDS pCreds = (LW_PIO_CREDS)hCreds;

    status = LwIoSetThreadCreds(pCreds);
    BAIL_ON_NT_STATUS(status);

cleanup:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(status);
    }

    return err;

error:
    goto cleanup;
}


VOID
NetDeleteCredentials(
    NET_CREDS_HANDLE *phCreds
    )
{
    LW_PIO_CREDS pCreds = NULL;

    if (phCreds == NULL) return;

    pCreds = (LW_PIO_CREDS)(*phCreds);
    LwIoDeleteCreds(pCreds);

    *phCreds = NULL;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
