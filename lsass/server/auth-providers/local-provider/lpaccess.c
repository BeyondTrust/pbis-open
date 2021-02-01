/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4; -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lpaccess.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Access Check API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
LocalCheckForAddAccess(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAdmin = FALSE;

    dwError = LocalCheckIsAdministrator(hProvider, &bIsAdmin);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bIsAdmin)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
    }

error:

    return dwError;
}

DWORD
LocalCheckForModifyAccess(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAdmin = FALSE;

    dwError = LocalCheckIsAdministrator(hProvider, &bIsAdmin);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bIsAdmin)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
    }

error:

    return dwError;
}

DWORD
LocalCheckForPasswordChangeAccess(
    HANDLE hProvider,
    uid_t  targetUid
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    if (pContext->uid != targetUid)
    {
        BOOLEAN bIsAdmin = FALSE;

        dwError = LocalCheckIsAdministrator(hProvider, &bIsAdmin);
        BAIL_ON_LSA_ERROR(dwError);

        if (!bIsAdmin)
        {
            dwError = LW_ERROR_ACCESS_DENIED;
        }
    }

error:

    return dwError;
}

DWORD
LocalCheckForQueryAccess(
    HANDLE hProvider
    )
{
    return LW_ERROR_SUCCESS;
}

DWORD
LocalCheckForDeleteAccess(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAdmin = FALSE;

    dwError = LocalCheckIsAdministrator(hProvider, &bIsAdmin);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bIsAdmin)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
    }

error:

    return dwError;
}

DWORD
LocalCheckIsAdministrator(
    HANDLE   hProvider,
    PBOOLEAN pbIsAdmin
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    BOOLEAN bIsAdmin = FALSE;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_HANDLE(hProvider);

    pthread_mutex_lock(&pContext->mutex);
    bInLock = TRUE;

    switch (pContext->localAdminState)
    {
        case LOCAL_ADMIN_STATE_NOT_DETERMINED:

            dwError = LocalDirCheckIfAdministrator(
                                hProvider,
                                pContext->uid,
                                &bIsAdmin);
            BAIL_ON_LSA_ERROR(dwError);

            pContext->localAdminState = (bIsAdmin ? LOCAL_ADMIN_STATE_IS_ADMIN :
                                                LOCAL_ADMIN_STATE_IS_NOT_ADMIN);

            break;

        case LOCAL_ADMIN_STATE_IS_ADMIN:

            bIsAdmin = TRUE;

            break;

        case LOCAL_ADMIN_STATE_IS_NOT_ADMIN:

            bIsAdmin = FALSE;

            break;
    }

    *pbIsAdmin = bIsAdmin;

cleanup:

    if (bInLock)
    {
        pthread_mutex_unlock(&pContext->mutex);
    }

    return dwError;

error:

    *pbIsAdmin = FALSE;

    goto cleanup;
}
