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
    .uShutdownTimeout = DEFAULT_SHUTDOWN_TIMEOUT_SECONDS,
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
    char * debugBootstrap = NULL;
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

        debugBootstrap = getenv("PBIS_DEBUG_BOOTSTRAP");

        pInfo->DefaultLogLevel = (debugBootstrap) ? LW_SM_LOG_LEVEL_DEBUG : LW_SM_LOG_LEVEL_DEFAULT;

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

        pInfo->uShutdownTimeout = pService->uShutdownTimeout;

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
