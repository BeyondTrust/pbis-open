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
 *        Bootstrap service logic
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

static SM_BOOTSTRAP_SERVICE gRegistryService =
{
    .pszName = "lwreg",
    .type = LW_SERVICE_TYPE_MODULE,
    .pszPath = LIBDIR "/lw-svcm/lwreg" MOD_EXT,
    .ppszArgs =
    {
        "--syslog",
        NULL
    }
};

static PSM_BOOTSTRAP_SERVICE gBootstrapServices[] =
{
    &gRegistryService,
    NULL
};

DWORD
LwSmBootstrap(
    VOID
    )
{
    DWORD dwError = 0;
    PLW_SERVICE_INFO pInfo = NULL;
    PSM_BOOTSTRAP_SERVICE pService = NULL;
    PSM_TABLE_ENTRY pEntry = NULL;
    size_t i = 0;
    size_t j = 0;
    size_t len = 0;

    SM_LOG_VERBOSE("Bootstrapping");

    for (i = 0; gBootstrapServices[i]; i++)
    {
        pService = gBootstrapServices[i];

        dwError = LwAllocateMemory(sizeof(*pInfo), OUT_PPVOID(&pInfo));
        BAIL_ON_ERROR(dwError);

        pInfo->type = pService->type;

        pInfo->bAutostart = 0;

        dwError = LwMbsToWc16s(pService->pszName, &pInfo->pwszName);
        BAIL_ON_ERROR(dwError);

        dwError = LwMbsToWc16s(pService->pszPath, &pInfo->pwszPath);
        BAIL_ON_ERROR(dwError);

#ifdef SERVICE_DIRECT
        dwError = LwMbsToWc16s("direct", &pInfo->pwszGroup);
#else
        dwError = LwMbsToWc16s(pService->pszName, &pInfo->pwszGroup);
#endif
        BAIL_ON_ERROR(dwError);

        dwError = LwMbsToWc16s("Bootstrap service", &pInfo->pwszDescription);
        BAIL_ON_ERROR(dwError);

        for (len = 0; pService->ppszArgs[len]; len++);

        dwError = LwAllocateMemory(
            (len + 1) * sizeof(*pInfo->ppwszArgs),
            OUT_PPVOID(&pInfo->ppwszArgs));
        BAIL_ON_ERROR(dwError);

        for (j = 0; j < len; j++)
        {
            dwError = LwMbsToWc16s(pService->ppszArgs[j], &pInfo->ppwszArgs[j]);
            BAIL_ON_ERROR(dwError);
        }

        /* Create empty Env list. */
        dwError = LwAllocateMemory(
            1 * sizeof(*pInfo->ppwszEnv), OUT_PPVOID(&pInfo->ppwszEnv));
        BAIL_ON_ERROR(dwError);

        pInfo->ppwszEnv[0] = NULL;

        dwError = LwAllocateMemory(
            1 * sizeof(*pInfo->ppwszDependencies),
            OUT_PPVOID(&pInfo->ppwszDependencies));
        BAIL_ON_ERROR(dwError);

        dwError = LwSmTableAddEntry(pInfo, &pEntry);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmTableStartEntry(pEntry);
        if (dwError)
        {
            SM_LOG_ERROR("Could not start bootstrap service: %s",
                         LwWin32ExtErrorToName(dwError));
        }
        BAIL_ON_ERROR(dwError);

        LwSmTableReleaseEntry(pEntry);
        pEntry = NULL;

        LwSmCommonFreeServiceInfo(pInfo);
        pInfo = NULL;
    }

cleanup:

    if (pEntry)
    {
        LwSmTableReleaseEntry(pEntry);
    }

    if (pInfo)
    {
        LwSmCommonFreeServiceInfo(pInfo);
    }

    return dwError;

error:

    goto cleanup;
}
