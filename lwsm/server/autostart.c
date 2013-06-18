/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
