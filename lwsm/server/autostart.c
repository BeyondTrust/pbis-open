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
 * Module Name:
 *
 *        bootstrap.c
 *
 * Abstract:
 *
 *        Autostart logic
 *
 */

#include "includes.h"

static
DWORD
StartService(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    PSM_TABLE_ENTRY pDep = NULL;
    PWSTR *ppwszDeps = NULL;
    size_t i = 0;

    dwError = LwSmTableGetEntryDependencyClosure(pEntry, &ppwszDeps);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszDeps[i]; i++)
    {
        dwError = LwSmTableGetEntry(ppwszDeps[i], &pDep);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmTableStartEntry(pDep);
        if (dwError)
        {
            SM_LOG_ERROR("Could not autostart fixme: %s",
                         /* fixme : service name */
                         LwWin32ExtErrorToName(dwError));
        }
        BAIL_ON_ERROR(dwError);


        LwSmTableReleaseEntry(pDep);
        pDep = NULL;
    }

    dwError = LwSmTableStartEntry(pEntry);
    BAIL_ON_ERROR(dwError);

cleanup:

    if (pDep)
    {
        LwSmTableReleaseEntry(pDep);
        pDep = NULL;
    }

    if (ppwszDeps)
    {
        LwSmFreeStringList(ppwszDeps);
        ppwszDeps = NULL;
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmAutostartServices(
    VOID
    )
{
    DWORD dwError = 0;
    PWSTR *ppwszAllServices = NULL;
    PSM_TABLE_ENTRY pEntry = NULL;
    size_t i = 0;

    dwError = LwSmTableEnumerateEntries(&ppwszAllServices);

    for (i = 0; ppwszAllServices[i]; i++)
    {
        dwError = LwSmTableGetEntry(ppwszAllServices[i], &pEntry);
        BAIL_ON_ERROR(dwError);

        if (pEntry->pInfo->bAutostart)
        {
            StartService(pEntry);
        }

        LwSmTableReleaseEntry(pEntry);
        pEntry = NULL;
    }

cleanup:

    if (pEntry)
    {
        LwSmTableReleaseEntry(pEntry);
        pEntry = NULL;
    }

    if (ppwszAllServices)
    {
        LwSmFreeStringList(ppwszAllServices);
        ppwszAllServices = NULL;
    }

    return dwError;

error:

    goto cleanup;
}
