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
 *        cache.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        User Lookup and Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "adclient.h"

DWORD
LsaAdEmptyCache(
    IN HANDLE hLsaConnection,
    IN OPTIONAL PCSTR pszDomainName,
    IN BOOLEAN bForceOfflineDelete
    )
{
    DWORD dwError = 0;
    PSTR pszTargetProvider = NULL;

    if (geteuid() != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszDomainName)
    {
        dwError = LwAllocateStringPrintf(
                      &pszTargetProvider,
                      "%s:%s:%d",
                      LSA_PROVIDER_TAG_AD,
                      pszDomainName,
                      bForceOfflineDelete);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateStringPrintf(
                      &pszTargetProvider,
                      "%s::%d",
                      LSA_PROVIDER_TAG_AD,                 
                      bForceOfflineDelete);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderIoControl(
                  hLsaConnection,
                  pszTargetProvider,
                  LSA_AD_IO_EMPTYCACHE,
                  0,
                  NULL,
                  NULL,
                  NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszTargetProvider);

    return dwError;

error:

    goto cleanup;
}
