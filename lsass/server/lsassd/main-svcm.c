/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "config.h"
#include "lsassd.h"
#include "lwnet.h"
#include "lw/base.h"
#include "lwdscache.h"
#include "lsasrvutils.h"

#ifdef ENABLE_STATIC_PROVIDERS
#ifdef ENABLE_AD
extern DWORD LsaInitializeProvider_ActiveDirectory(PCSTR*, PLSA_PROVIDER_FUNCTION_TABLE*);
#endif
#ifdef ENABLE_LOCAL
extern DWORD LsaInitializeProvider_Local(PCSTR*, PLSA_PROVIDER_FUNCTION_TABLE*);
#endif

static LSA_STATIC_PROVIDER gStaticProviders[] =
{
#ifdef ENABLE_AD
    { LSA_PROVIDER_TAG_AD, LsaInitializeProvider_ActiveDirectory },
#endif
#ifdef ENABLE_LOCAL
    { LSA_PROVIDER_TAG_LOCAL, LsaInitializeProvider_Local },
#endif
    { 0 }
};
#endif // ENABLE_STATIC_PROVIDERS

NTSTATUS
LsaSvcmInit(
    PCWSTR pServiceName,
    PLW_SVCM_INSTANCE pInstance
    )
{
    return STATUS_SUCCESS;
}

VOID
LsaSvcmDestroy(
    PLW_SVCM_INSTANCE pInstance
    )
{
    return;
}


NTSTATUS
LsaSvcmStart(
    PLW_SVCM_INSTANCE pInstance,
    ULONG ArgCount,
    PWSTR* ppArgs,
    ULONG FdCount,
    int* pFds
    )
{
    DWORD dwError = 0;

    dwError = LsaSrvSetDefaults();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaInitTracing_r();
    BAIL_ON_LSA_ERROR(dwError);

    // Test system to see if dependent configuration tasks are completed prior to starting our process.
    dwError = LsaSrvStartupPreCheck();
    BAIL_ON_LSA_ERROR(dwError);

#ifdef ENABLE_EVENTLOG
    dwError = LsaSrvStartEventLoggingThread();
    BAIL_ON_LSA_ERROR(dwError);
#endif

    /* Start NTLM IPC server before we initialize providers
       because the providers might end up attempting to use
       NTLM via gss-api */
    dwError = NtlmSrvStartListenThread();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvInitialize();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvStartListenThread();
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogProcessStartedEvent();

cleanup:

    return LwWin32ErrorToNtStatus(dwError);

error:

    goto cleanup;
}

NTSTATUS
LsaSvcmStop(
    PLW_SVCM_INSTANCE pInstance
    )
{   LsaSrvStopListenThread();
    NtlmSrvStopListenThread();
    LsaSrvApiShutdown();
    NtlmClientIpcShutdown();
    LSA_LOG_INFO("LSA Service exiting...");
#ifdef ENABLE_EVENTLOG
    LsaSrvStopEventLoggingThread();
#endif
    LsaShutdownTracing_r();

    return STATUS_SUCCESS;
}

NTSTATUS
LsaSvcmRefresh(
    PLW_SVCM_INSTANCE pInstance
    )
{
    DWORD dwError = 0;
    HANDLE hServer = NULL;

    LSA_LOG_VERBOSE("Refreshing configuration");

    dwError = LsaSrvOpenServer(
                getuid(),
                getgid(),
                getpid(),
                &hServer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvRefreshConfiguration(hServer);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_INFO("Refreshed configuration successfully");

cleanup:

    if (hServer != NULL)
    {
        LsaSrvCloseServer(hServer);
    }

    return LwWin32ErrorToNtStatus(dwError);

error:

    LSA_LOG_ERROR("Failed to refresh configuration. [Error code:%u]", dwError);

    goto cleanup;
}

static LW_SVCM_MODULE gService =
{
    .Size = sizeof(gService),
    .Init = LsaSvcmInit,
    .Destroy = LsaSvcmDestroy,
    .Start = LsaSvcmStart,
    .Stop = LsaSvcmStop,
    .Refresh = LsaSvcmRefresh
};

#define SVCM_ENTRY_POINT LW_RTL_SVCM_ENTRY_POINT_NAME(lsass)

PLW_SVCM_MODULE
SVCM_ENTRY_POINT(
    VOID
    )
{
    return &gService;
}

DWORD
LsaSrvStartupPreCheck(
    VOID
    )
{
    DWORD dwError = 0;
#ifdef __LWI_DARWIN__
    PSTR  pszHostname = NULL;
    int  iter = 0;

    // Make sure that the local hostname has been setup by the system
    for (iter = 0; iter < STARTUP_PRE_CHECK_WAIT; iter++)
    {
        LW_SAFE_FREE_STRING(pszHostname);
        dwError = LsaDnsGetHostInfo(&pszHostname);
        BAIL_ON_LSA_ERROR(dwError);

        if (!strcasecmp(pszHostname, "localhost"))
        {
            sleep(10);
        }
        else
        {
            /* Hostname now looks correct */
            LSA_LOG_INFO("LSA Process start up check for hostname complete [hostname:%s]", pszHostname);
            break;
        }
    }

    if (iter >= STARTUP_PRE_CHECK_WAIT)
    {
        dwError = LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK;
        LSA_LOG_ERROR("LSA start up pre-check failed to get updated hostname after %u seconds of waiting [Code:%u]",
                      STARTUP_PRE_CHECK_WAIT*10,
                      dwError);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Now that we are running, we need to flush the DirectoryService process of any negative cache entries
    dwError = LsaSrvFlushSystemCache();
    BAIL_ON_LSA_ERROR(dwError);

error:

    LW_SAFE_FREE_STRING(pszHostname);
#endif

    return dwError;
}

DWORD
LsaSrvSetDefaults(
    VOID
    )
{
    DWORD dwError = 0;

    strcpy(gpServerInfo->szCachePath, CACHEDIR);
    strcpy(gpServerInfo->szPrefixPath, PREFIXDIR);

    return (dwError);
}

DWORD
LsaSrvInitialize(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = LsaInitCacheFolders();
    BAIL_ON_LSA_ERROR(dwError);

#ifdef ENABLE_STATIC_PROVIDERS
    dwError = LsaSrvApiInit(gStaticProviders);
#else
    dwError = LsaSrvApiInit(NULL);
#endif
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaInitCacheFolders(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    BOOLEAN bExists = FALSE;

    dwError = LsaSrvGetCachePath(&pszCachePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckDirectoryExists(
                        pszCachePath,
                        &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists) {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = LsaCreateDirectory(pszCachePath, cacheDirMode);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszCachePath);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    if (LW_IS_NULL_OR_EMPTY_STR(gpServerInfo->szCachePath)) {
      dwError = LW_ERROR_INVALID_CACHE_PATH;
      BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateString(gpServerInfo->szCachePath, &pszPath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:

    LSA_UNLOCK_SERVERINFO(bInLock);

    return dwError;

 error:

    LW_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

DWORD
LsaSrvGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    if (LW_IS_NULL_OR_EMPTY_STR(gpServerInfo->szPrefixPath)) {
      dwError = LW_ERROR_INVALID_PREFIX_PATH;
      BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateString(gpServerInfo->szPrefixPath, &pszPath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:

    LSA_UNLOCK_SERVERINFO(bInLock);

    return dwError;

 error:

    LW_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

VOID
LsaSrvLogProcessStartedEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise authentication service was started.");
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_INFO_SERVICE_STARTED,
            SERVICE_EVENT_CATEGORY,
            pszDescription,
            NULL);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
LsaSrvLogProcessStoppedEvent(
    DWORD dwExitCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise authentication service was stopped");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwExitCode,
                         &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwExitCode)
    {
        LsaSrvLogServiceFailureEvent(
                LSASS_EVENT_ERROR_SERVICE_STOPPED,
                SERVICE_EVENT_CATEGORY,
                pszDescription,
                pszData);
    }
    else
    {
        LsaSrvLogServiceSuccessEvent(
                LSASS_EVENT_INFO_SERVICE_STOPPED,
                SERVICE_EVENT_CATEGORY,
                pszDescription,
                pszData);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}

VOID
LsaSrvLogProcessFailureEvent(
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise authentication service stopped running due to an error");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceFailureEvent(
            LSASS_EVENT_ERROR_SERVICE_START_FAILURE,
            SERVICE_EVENT_CATEGORY,
            pszDescription,
            pszData);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}

