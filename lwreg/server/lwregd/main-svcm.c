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
 *        main.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */
#include "registryd.h"
#include <lw/svcm.h>

NTSTATUS
RegSvcmInit(
    PCWSTR pServiceName,
    PLW_SVCM_INSTANCE pInstance
    )
{
    return STATUS_SUCCESS;
}

VOID
RegSvcmDestroy(
    PLW_SVCM_INSTANCE pInstance
    )
{
    return;
}


NTSTATUS
RegSvcmStart(
    PLW_SVCM_INSTANCE pInstance,
    ULONG ArgCount,
    PWSTR* ppArgs,
    ULONG FdCount,
    int* pFds
    )
{
    DWORD dwError = 0;

    dwError = RegSrvSetDefaults();
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSrvInitialize();
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSrvStartListenThread();
    BAIL_ON_REG_ERROR(dwError);

cleanup:

    return LwWin32ErrorToNtStatus(dwError);

error:

    REG_LOG_ERROR("REG Process exiting due to error [Code:%d]", dwError);

    goto cleanup;
}

NTSTATUS
RegSvcmStop(
    PLW_SVCM_INSTANCE pInstance
    )
{
    REG_LOG_VERBOSE("Reg main cleaning up");

    RegSrvStopListenThread();

    RegSrvApiShutdown();

    REG_LOG_INFO("REG Service exiting...");

    return STATUS_SUCCESS;
}

static LW_SVCM_MODULE gService =
{
    .Size = sizeof(gService),
    .Init = RegSvcmInit,
    .Destroy = RegSvcmDestroy,
    .Start = RegSvcmStart,
    .Stop = RegSvcmStop
};

#define SVCM_ENTRY_POINT LW_RTL_SVCM_ENTRY_POINT_NAME(lwreg)

PLW_SVCM_MODULE
SVCM_ENTRY_POINT(
    VOID
    )
{
    return &gService;
}

DWORD
RegSrvSetDefaults(
    VOID
    )
{
    DWORD dwError = 0;

    strncpy(gpLwregServerInfo->szCachePath,
            CACHEDIR,
            sizeof(gpLwregServerInfo->szCachePath)-1);

    strncpy(gpLwregServerInfo->szPrefixPath,
            PREFIXDIR,
            sizeof(gpLwregServerInfo->szPrefixPath)-1);

    return (dwError);
}

DWORD
RegSrvInitialize(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = RegInitCacheFolders();
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSrvApiInit();
    BAIL_ON_REG_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
RegInitCacheFolders(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    BOOLEAN bExists = FALSE;

    dwError = RegSrvGetCachePath(&pszCachePath);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegCheckDirectoryExists(
        pszCachePath,
        &bExists);

    BAIL_ON_REG_ERROR(dwError);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = RegCreateDirectory(pszCachePath, cacheDirMode);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:

    LWREG_SAFE_FREE_STRING(pszCachePath);

    return dwError;

error:

    goto cleanup;
}

DWORD
RegSrvGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;

    REG_LOCK_SERVERINFO(bInLock);

    if (IsNullOrEmptyString(gpLwregServerInfo->szCachePath))
    {
        dwError = LWREG_ERROR_INVALID_CACHE_PATH;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LwRtlCStringDuplicate(&pszPath, gpLwregServerInfo->szCachePath);
    BAIL_ON_REG_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:

    REG_UNLOCK_SERVERINFO(bInLock);

    return dwError;

 error:

    LWREG_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

DWORD
RegSrvGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;

    REG_LOCK_SERVERINFO(bInLock);

    if (IsNullOrEmptyString(gpLwregServerInfo->szPrefixPath))
    {
        dwError = LWREG_ERROR_INVALID_PREFIX_PATH;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LwRtlCStringDuplicate(&pszPath, gpLwregServerInfo->szPrefixPath);
    BAIL_ON_REG_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:

    REG_UNLOCK_SERVERINFO(bInLock);

    return dwError;

 error:

    LWREG_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

