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
 *        main.c
 *
 * Abstract:
 *
 *        Client utility program
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

static struct
{
    BOOLEAN bQuiet;
} gState =
{
    .bQuiet = FALSE
};

static
PCSTR
LwSmStateToString(
    LW_SERVICE_STATE state
    )
{
    switch (state)
    {
    case LW_SERVICE_STATE_STOPPED:
        return "stopped";
    case LW_SERVICE_STATE_STARTING:
        return "starting";
    case LW_SERVICE_STATE_RUNNING:
        return "running";
    case LW_SERVICE_STATE_STOPPING:
        return "stopping";
    case LW_SERVICE_STATE_PAUSED:
        return "paused";
    case LW_SERVICE_STATE_DEAD:
        return "dead";
    default:
        return "unknown";
    }
}

static
PCSTR
LwSmHomeToString(
    LW_SERVICE_HOME home
    )
{
    switch (home)
    {
    case LW_SERVICE_HOME_STANDALONE:
        return "standalone";
    case LW_SERVICE_HOME_CONTAINER:
        return "container";
    case LW_SERVICE_HOME_IO_MANAGER:
        return "io";
    case LW_SERVICE_HOME_SERVICE_MANAGER:
        return "sm";
    default:
        return "unknown";
    }
}

static
PCSTR
LwSmTypeToString(
    LW_SERVICE_TYPE type
    )
{
    switch (type)
    {
    case LW_SERVICE_TYPE_LEGACY_EXECUTABLE:
        return "legacy executable";
    case LW_SERVICE_TYPE_EXECUTABLE:
        return "executable";
    case LW_SERVICE_TYPE_MODULE:
        return "module";
    case LW_SERVICE_TYPE_DRIVER:
        return "driver";
    case LW_SERVICE_TYPE_STUB:
        return "stub";
    default:
        return "unknown";
    }
}

static
PCSTR
LwSmLogLevelToString(
    LW_SM_LOG_LEVEL level
    )
{
    switch (level)
    {
    case LW_SM_LOG_LEVEL_DEFAULT:
        return "default";
    case LW_SM_LOG_LEVEL_ALWAYS:
        return "ALWAYS";
    case LW_SM_LOG_LEVEL_ERROR:
        return "ERROR";
    case LW_SM_LOG_LEVEL_WARNING:
        return "WARNING";
    case LW_SM_LOG_LEVEL_INFO:
        return "INFO";
    case LW_SM_LOG_LEVEL_VERBOSE:
        return "VERBOSE";
    case LW_SM_LOG_LEVEL_DEBUG:
        return "DEBUG";
    case LW_SM_LOG_LEVEL_TRACE:
        return "TRACE";
    default:
        return "UNKNOWN";
    }
}

static
DWORD
LwSmLogLevelNameToLogLevel(
    PCSTR pszName,
    PLW_SM_LOG_LEVEL pLevel
    )
{
    DWORD dwError = 0;

    if (!strcmp(pszName, "-"))
    {
        *pLevel = LW_SM_LOG_LEVEL_DEFAULT;
    }
    else if (!strcasecmp(pszName, "always"))
    {
        *pLevel = LW_SM_LOG_LEVEL_ALWAYS;
    }
    else if (!strcasecmp(pszName, "error"))
    {
        *pLevel = LW_SM_LOG_LEVEL_ERROR;
    }
    else if (!strcasecmp(pszName, "warning"))
    {
        *pLevel = LW_SM_LOG_LEVEL_WARNING;
    }
    else if (!strcasecmp(pszName, "info"))
    {
        *pLevel = LW_SM_LOG_LEVEL_INFO;
    }
    else if (!strcasecmp(pszName, "verbose"))
    {
        *pLevel = LW_SM_LOG_LEVEL_VERBOSE;
    }
    else if (!strcasecmp(pszName, "debug"))
    {
        *pLevel = LW_SM_LOG_LEVEL_DEBUG;
    }
    else if (!strcasecmp(pszName, "trace"))
    {
        *pLevel = LW_SM_LOG_LEVEL_TRACE;
    }
    else
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

error:

    return dwError;
}

static
VOID
LwSmHandleSigint(
    int sig
    )
{
    raise(SIGTERM);
}

static
DWORD
LwSmConfigureSignals(
    VOID
    )
{
    DWORD dwError = 0;
    sigset_t set;
    static int blockSignals[] =
    {
        SIGTERM,
        SIGCHLD,
        SIGHUP,
        -1
    };
    int i = 0;
    struct sigaction intAction;

    memset(&intAction, 0, sizeof(intAction));

    if (sigemptyset(&set) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

    for (i = 0; blockSignals[i] != -1; i++)
    {
        if (sigaddset(&set, blockSignals[i]) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError); 
        }
    }

    dwError = LwMapErrnoToLwError(pthread_sigmask(SIG_SETMASK, &set, NULL));
    BAIL_ON_ERROR(dwError); 

    intAction.sa_handler = LwSmHandleSigint;
    intAction.sa_flags = 0;

    if (sigaction(SIGINT, &intAction, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmWaitSignals(
    int* pSig
    )
{
    DWORD dwError = 0;
    sigset_t set;
    static int waitSignals[] =
    {
        SIGTERM,
        SIGHUP,
        -1
    };
    int sig = -1;
    int i = 0;

    if (sigemptyset(&set) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

    for (i = 0; waitSignals[i] != -1; i++)
    {
        if (sigaddset(&set, waitSignals[i]) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError); 
        }
    }

    for (;;)
    {
        if (sigwait(&set, &sig) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }

        switch (sig)
        {
        case SIGTERM:
        case SIGHUP:
            *pSig = sig;
            goto cleanup;
        default:
            break;
        }
    }

cleanup:

    return dwError;

error:

    goto cleanup;
};

static
DWORD
LwSmWaitForLwsmd(
    void
    )
{
    DWORD dwError = 0;
    int try = 0;
    static const int maxTries = 4;
    static const int interval = 5;
    PWSTR* ppwszServices = NULL;

    do
    {
        dwError = LwSmEnumerateServices(&ppwszServices);

        if (dwError)
        {
            sleep(interval);
        }

        try++;
    } while (dwError != LW_ERROR_SUCCESS && try < maxTries);

    BAIL_ON_ERROR(dwError);

cleanup:

    if (ppwszServices)
    {
        LwSmFreeServiceNameList(ppwszServices);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmList(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszServiceNames = NULL;
    PSTR pszServiceName = NULL;
    LW_SERVICE_STATUS status;
    LW_SERVICE_HANDLE hHandle = NULL;
    size_t i = 0;
    size_t len = 0;
    size_t maxLen = 0;

    dwError = LwSmEnumerateServices(&ppwszServiceNames);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszServiceNames[i]; i++)
    {
        dwError = LwWc16sLen(ppwszServiceNames[i], &len);
        BAIL_ON_ERROR(dwError);

        if (len > maxLen)
        {
            maxLen = len;
        }
    }

    for (i = 0; ppwszServiceNames[i]; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszServiceNames[i], &hHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmQueryServiceStatus(hHandle, &status);
        BAIL_ON_ERROR(dwError);

        dwError = LwWc16sToMbs(ppwszServiceNames[i], &pszServiceName);
        BAIL_ON_ERROR(dwError);
        
        if (!gState.bQuiet)
        {
            printf("%s", pszServiceName);

            dwError = LwWc16sLen(ppwszServiceNames[i], &len);
            BAIL_ON_ERROR(dwError);

            for (; len < maxLen; len++)
            {
                printf(" ");
            }
            printf("    ");

            switch (status.state)
            {
            case LW_SERVICE_STATE_RUNNING:
                BAIL_ON_ERROR(dwError);
                printf("%s (%s: %li)\n",
                       LwSmStateToString(status.state),
                       LwSmHomeToString(status.home),
                       (long) status.pid);
                break;
            default:
                printf("%s\n", LwSmStateToString(status.state));
                break;
            }
        }

        dwError = LwSmReleaseServiceHandle(hHandle);
        hHandle = NULL;
        BAIL_ON_ERROR(dwError);

        LW_SAFE_FREE_MEMORY(pszServiceName);    
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pszServiceName);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    if (ppwszServiceNames)
    {
        LwSmFreeServiceNameList(ppwszServiceNames);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmStartOnly(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    
    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    if (!gState.bQuiet)
    {
        printf("Starting service: %s\n", pArgv[1]);
    }

    dwError = LwSmStartService(hHandle);
    BAIL_ON_ERROR(dwError);
    
cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszServiceName);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    return dwError;

error:

    goto cleanup;
}


static
DWORD
LwSmStart(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    LW_SERVICE_HANDLE hDepHandle = NULL;
    LW_SERVICE_STATUS status = {0};
    PWSTR* ppwszDependencies = NULL;
    PSTR pszTemp = NULL;
    size_t i = 0;

    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmQueryServiceDependencyClosure(hHandle, &ppwszDependencies);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszDependencies[i]; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszDependencies[i], &hDepHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmQueryServiceStatus(hDepHandle, &status);
        BAIL_ON_ERROR(dwError);

        if (status.state != LW_SERVICE_STATE_RUNNING)
        {
            if (!gState.bQuiet)
            {
                dwError = LwWc16sToMbs(ppwszDependencies[i], &pszTemp);
                BAIL_ON_ERROR(dwError);
                
                printf("Starting service dependency: %s\n", pszTemp);
                LW_SAFE_FREE_MEMORY(pszTemp);
            }

            dwError = LwSmStartService(hDepHandle);
            BAIL_ON_ERROR(dwError);
        }

        dwError = LwSmReleaseServiceHandle(hDepHandle);
        hDepHandle = NULL;
        BAIL_ON_ERROR(dwError);
    }

    if (!gState.bQuiet)
    {
        printf("Starting service: %s\n", pArgv[1]);
    }

    dwError = LwSmStartService(hHandle);
    BAIL_ON_ERROR(dwError);
    
cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszServiceName);
    LW_SAFE_FREE_MEMORY(pszTemp);

    if (ppwszDependencies)
    {
        LwSmFreeServiceNameList(ppwszDependencies);
    }

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    if (hDepHandle)
    {
        LwSmReleaseServiceHandle(hDepHandle);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmStopOnly(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    
    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    if (!gState.bQuiet)
    {
        printf("Stopping service: %s\n", pArgv[1]);
    }

    dwError = LwSmStopService(hHandle);
    BAIL_ON_ERROR(dwError);
    
cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszServiceName);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmStop(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    LW_SERVICE_HANDLE hDepHandle = NULL;
    LW_SERVICE_STATUS status = {0};
    PWSTR* ppwszDependencies = NULL;
    PSTR pszTemp = NULL;
    size_t i = 0;

    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmQueryServiceReverseDependencyClosure(hHandle, &ppwszDependencies);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszDependencies[i]; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszDependencies[i], &hDepHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmQueryServiceStatus(hDepHandle, &status);
        BAIL_ON_ERROR(dwError);

        if (status.state != LW_SERVICE_STATE_STOPPED)
        {
            if (!gState.bQuiet)
            {
                dwError = LwWc16sToMbs(ppwszDependencies[i], &pszTemp);
                BAIL_ON_ERROR(dwError);
                
                printf("Stopping service reverse dependency: %s\n", pszTemp);
                LW_SAFE_FREE_MEMORY(pszTemp);
            }

            dwError = LwSmStopService(hDepHandle);
            BAIL_ON_ERROR(dwError);
        }

        dwError = LwSmReleaseServiceHandle(hDepHandle);
        hDepHandle = NULL;
        BAIL_ON_ERROR(dwError);
    }

    if (!gState.bQuiet)
    {
        printf("Stopping service: %s\n", pArgv[1]);
    }

    dwError = LwSmStopService(hHandle);
    BAIL_ON_ERROR(dwError);
    
cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszServiceName);
    LW_SAFE_FREE_MEMORY(pszTemp);

    if (ppwszDependencies)
    {
        LwSmFreeServiceNameList(ppwszDependencies);
    }

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    if (hDepHandle)
    {
        LwSmReleaseServiceHandle(hDepHandle);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmRestart(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    PWSTR* ppwszReverseDeps = NULL;
    PLW_SERVICE_STATUS pStatus = NULL;
    PLW_SERVICE_HANDLE phDepHandles = NULL;
    PWSTR* ppwszDependencies = NULL;
    LW_SERVICE_HANDLE hDepHandle = NULL;
    LW_SERVICE_STATUS status;
    PSTR pszTemp = NULL;
    size_t count = 0;
    size_t i = 0;

    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError); 

    dwError = LwSmQueryServiceReverseDependencyClosure(hHandle, &ppwszReverseDeps);
    BAIL_ON_ERROR(dwError);
 
    count = LwSmStringListLength(ppwszReverseDeps);

    dwError = LwAllocateMemory(sizeof(*pStatus) * count, OUT_PPVOID(&pStatus));
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*phDepHandles) * count, OUT_PPVOID(&phDepHandles));
    BAIL_ON_ERROR(dwError);

    for (i = 0; i < count; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszReverseDeps[i], &phDepHandles[i]);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmQueryServiceStatus(phDepHandles[i], &pStatus[i]);
        BAIL_ON_ERROR(dwError);

        if (pStatus[i].state != LW_SERVICE_STATE_STOPPED)
        {
            if (!gState.bQuiet)
            {
                dwError = LwWc16sToMbs(ppwszReverseDeps[i], &pszTemp);
                BAIL_ON_ERROR(dwError);
                printf("Stopping service reverse dependency: %s\n", pszTemp);
                LW_SAFE_FREE_MEMORY(pszTemp);
            }
            dwError = LwSmStopService(phDepHandles[i]);
            BAIL_ON_ERROR(dwError);
        }
    }

    if (!gState.bQuiet)
    {
        printf("Stopping service: %s\n", pArgv[1]);
    }

    dwError = LwSmStopService(hHandle);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmQueryServiceDependencyClosure(hHandle, &ppwszDependencies);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszDependencies[i]; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszDependencies[i], &hDepHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmQueryServiceStatus(hDepHandle, &status);
        BAIL_ON_ERROR(dwError);

        if (status.state != LW_SERVICE_STATE_RUNNING)
        {
            if (!gState.bQuiet)
            {
                dwError = LwWc16sToMbs(ppwszDependencies[i], &pszTemp);
                BAIL_ON_ERROR(dwError);
                
                printf("Starting service dependency: %s\n", pszTemp);
                LW_SAFE_FREE_MEMORY(pszTemp);
            }

            dwError = LwSmStartService(hDepHandle);
            BAIL_ON_ERROR(dwError);
        }

        dwError = LwSmReleaseServiceHandle(hDepHandle);
        hDepHandle = NULL;
        BAIL_ON_ERROR(dwError);
    }

    if (!gState.bQuiet)
    {
        printf("Starting service: %s\n", pArgv[1]);
    }

    dwError = LwSmStartService(hHandle);
    BAIL_ON_ERROR(dwError);

    for (i = 0; i < count; i++)
    {
        if (pStatus[count - 1 - i].state == LW_SERVICE_STATE_RUNNING)
        {
            if (!gState.bQuiet)
            {
                dwError = LwWc16sToMbs(ppwszReverseDeps[count - 1 - i], &pszTemp);
                BAIL_ON_ERROR(dwError);
                printf("Starting service reverse dependency: %s\n", pszTemp);
                LW_SAFE_FREE_MEMORY(pszTemp);
            }
            dwError = LwSmStartService(phDepHandles[count - 1 - i]);
            BAIL_ON_ERROR(dwError);
        }
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pwszServiceName);
    LW_SAFE_FREE_MEMORY(pStatus);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    if (phDepHandles)
    {
        for (i = 0; i < count; i++)
        {
            if (phDepHandles[i])
            {
                LwSmReleaseServiceHandle(phDepHandles[i]);
            }
        }

        LW_SAFE_FREE_MEMORY(phDepHandles);
    }
    if (ppwszReverseDeps)
    {
        LwSmFreeServiceNameList(ppwszReverseDeps);
        ppwszReverseDeps = NULL;
    }

    if (hDepHandle)
    {
        LwSmReleaseServiceHandle(hDepHandle);
        hDepHandle = NULL;
    }
    if (ppwszDependencies)
    {
        LwSmFreeServiceNameList(ppwszDependencies);
        ppwszDependencies = NULL;
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmDoRefresh(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    
    if (argc > 1)
    {
        dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
        BAIL_ON_ERROR(dwError);
    
        dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
        BAIL_ON_ERROR(dwError);

        if (!gState.bQuiet)
        {
            printf("Refreshing service: %s\n", pArgv[1]);
        }

        dwError = LwSmRefreshService(hHandle);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        if (!gState.bQuiet)
        {
            printf("Refreshing service manager\n");
        }

        dwError = LwSmRefresh();
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pwszServiceName);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmInfo(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    PLW_SERVICE_INFO pInfo = NULL;
    PSTR pszTemp = NULL;
    size_t i = 0;

    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmQueryServiceInfo(hHandle, &pInfo);
    BAIL_ON_ERROR(dwError);
    
    printf("Service: %s\n", pArgv[1]);

    dwError = LwWc16sToMbs(pInfo->pwszDescription, &pszTemp);
    BAIL_ON_ERROR(dwError);

    printf("Description: %s\n", pszTemp);
    LW_SAFE_FREE_MEMORY(pszTemp);

    printf("Type: %s\n", LwSmTypeToString(pInfo->type));
    printf("Autostart: %s\n", pInfo->bAutostart ? "yes" : "no");

    dwError = LwWc16sToMbs(pInfo->pwszPath, &pszTemp);
    BAIL_ON_ERROR(dwError);

    printf("Path: %s\n", pszTemp);
    LW_SAFE_FREE_MEMORY(pszTemp);

    printf("Arguments:");

    for (i = 0; pInfo->ppwszArgs[i]; i++)
    {
         dwError = LwWc16sToMbs(pInfo->ppwszArgs[i], &pszTemp);
         BAIL_ON_ERROR(dwError);

         printf(" '%s'", pszTemp);

         LW_SAFE_FREE_MEMORY(pszTemp);
    }

    printf("\n");

    printf("Environment:");

    for (i = 0; pInfo->ppwszEnv[i]; i++)
    {
         dwError = LwWc16sToMbs(pInfo->ppwszEnv[i], &pszTemp);
         BAIL_ON_ERROR(dwError);

         printf(" '%s'", pszTemp);

         LW_SAFE_FREE_MEMORY(pszTemp);
    }

    printf("\n");

    printf("Dependencies:");

    for (i = 0; pInfo->ppwszDependencies[i]; i++)
    {
         dwError = LwWc16sToMbs(pInfo->ppwszDependencies[i], &pszTemp);
         BAIL_ON_ERROR(dwError);

         printf(" %s", pszTemp);

         LW_SAFE_FREE_MEMORY(pszTemp);
    }

    printf("\n");

    dwError = LwWc16sToMbs(pInfo->pwszGroup, &pszTemp);
    BAIL_ON_ERROR(dwError);

    printf("Service Group: %s\n", pszTemp);
    LW_SAFE_FREE_MEMORY(pszTemp);

    if (pInfo->dwFdLimit)
    {
        printf("File descriptor limit: %lu\n", (unsigned long) pInfo->dwFdLimit);
    }
    else
    {
        printf("File descriptor limit: inherit\n");
    }
    if (pInfo->dwCoreSize)
    {
        printf("Core dump size limit: %lu\n", (unsigned long) pInfo->dwCoreSize);
    }
    else
    {
        printf("Core dump size limit: inherit\n");
    }

cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszServiceName);
    LW_SAFE_FREE_MEMORY(pszTemp);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmStatus(
    int argc,
    char** pArgv,
    int* pRet
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    LW_SERVICE_STATUS status = {0};

    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmQueryServiceStatus(hHandle, &status);
    BAIL_ON_ERROR(dwError);
    
    if (!gState.bQuiet)
    {
        switch (status.state)
        {
        case LW_SERVICE_STATE_RUNNING:
            printf("%s (%s: %li)\n",
                   LwSmStateToString(status.state),
                   LwSmHomeToString(status.home),
                   (long) status.pid);
            break;
        default:
            printf("%s\n", LwSmStateToString(status.state));
            break;
        }
    }

    *pRet = status.state;

cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszServiceName);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmAutostart(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR *ppwszAllServices = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    PLW_SERVICE_INFO pInfo = NULL;
    size_t i = 0;
    PWSTR *ppwszDependencies = NULL;
    LW_SERVICE_HANDLE hDepHandle = NULL;
    LW_SERVICE_STATUS status = {0};
    PSTR pszTemp = NULL;
    size_t j = 0;

    dwError = LwSmEnumerateServices(&ppwszAllServices);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszAllServices[i]; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszAllServices[i], &hHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmQueryServiceInfo(hHandle, &pInfo);
        BAIL_ON_ERROR(dwError);

        if (pInfo->bAutostart)
        {
            dwError = LwSmQueryServiceDependencyClosure(
                            hHandle,
                            &ppwszDependencies);
            BAIL_ON_ERROR(dwError);

            for (j = 0; ppwszDependencies[j]; j++)
            {
                dwError = LwSmAcquireServiceHandle(
                                ppwszDependencies[j],
                                &hDepHandle);
                BAIL_ON_ERROR(dwError);

                dwError = LwSmQueryServiceStatus(hDepHandle, &status);
                BAIL_ON_ERROR(dwError);

                if (status.state != LW_SERVICE_STATE_RUNNING)
                {
                    if (!gState.bQuiet)
                    {
                        dwError = LwWc16sToMbs(ppwszDependencies[j], &pszTemp);
                        BAIL_ON_ERROR(dwError);

                        printf("Starting service dependency: %s\n", pszTemp);
                        LW_SAFE_FREE_MEMORY(pszTemp);
                    }

                    dwError = LwSmStartService(hDepHandle);
                    BAIL_ON_ERROR(dwError);
                }

                dwError = LwSmReleaseServiceHandle(hDepHandle);
                hDepHandle = NULL;
                BAIL_ON_ERROR(dwError);
            }

            if (ppwszDependencies)
            {
                LwSmFreeServiceNameList(ppwszDependencies);
                ppwszDependencies = NULL;
            }

            if (!gState.bQuiet)
            {
                dwError = LwWc16sToMbs(ppwszAllServices[i], &pszTemp);
                BAIL_ON_ERROR(dwError);

                printf("Starting service: %s\n", pszTemp);
                LW_SAFE_FREE_MEMORY(pszTemp);
            }

            dwError = LwSmStartService(hHandle);
            BAIL_ON_ERROR(dwError);
        }

        dwError = LwSmReleaseServiceHandle(hHandle);
        hHandle = NULL;
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
        hHandle = NULL;
    }

    if (ppwszAllServices)
    {
        LwSmFreeStringList(ppwszAllServices);
        ppwszAllServices = NULL;
    }

    if (ppwszDependencies)
    {
        LwSmFreeServiceNameList(ppwszDependencies);
        ppwszDependencies = NULL;
    }

    if (hDepHandle)
    {
        LwSmReleaseServiceHandle(hDepHandle);
        hDepHandle = NULL;
    }

    LW_SAFE_FREE_MEMORY(pszTemp);

    return dwError;

error:

    goto cleanup;
}

static
PVOID
LwSmWaitThread(
    PVOID pData
    )
{
    DWORD dwError = 0;
    LW_SERVICE_HANDLE hHandle = pData;
    LW_SERVICE_STATUS status = {0};
    
    while (status.state != LW_SERVICE_STATE_STOPPED &&
           status.state != LW_SERVICE_STATE_DEAD)
    {
        dwError = LwSmWaitService(hHandle, status.state, &status.state);
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    kill(getpid(), SIGHUP);

    return NULL;

error:

    goto cleanup;
}

static
DWORD
LwSmProxy(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    LW_SERVICE_HANDLE hHandle = NULL;
    PWSTR pwszServiceName = NULL;
    pthread_t waitThread;
    int sig = 0;

    dwError = LwSmWaitForLwsmd();
    BAIL_ON_ERROR(dwError);

    dwError = LwSmConfigureSignals();
    BAIL_ON_ERROR(dwError);

    dwError = LwSmStart(argc, pArgv);
    BAIL_ON_ERROR(dwError);

    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwMapErrnoToLwError(pthread_create(
                                      &waitThread,
                                      NULL,
                                      LwSmWaitThread,
                                      hHandle));
    BAIL_ON_ERROR(dwError);

    dwError = LwMapErrnoToLwError(pthread_detach(waitThread));
    BAIL_ON_ERROR(dwError);
    
    if (!gState.bQuiet)
    {
        printf("Proxying for service: %s\n", pArgv[1]);
    }

    dwError = LwSmWaitSignals(&sig);
    BAIL_ON_ERROR(dwError);

    switch (sig)
    {
    case SIGTERM:
        dwError = LwSmStop(argc, pArgv);
        BAIL_ON_ERROR(dwError);
        break;
    default:
        break;
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pwszServiceName);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmFindServiceWithPid(
    pid_t pid,
    PLW_SERVICE_HANDLE phHandle
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszServiceNames = NULL;
    DWORD i = 0;
    PLW_SERVICE_INFO pInfo = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    LW_SERVICE_STATUS status = {0};

    dwError = LwSmEnumerateServices(&ppwszServiceNames);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszServiceNames[i]; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszServiceNames[i], &hHandle);
        BAIL_ON_ERROR(dwError);
    
        dwError = LwSmQueryServiceStatus(hHandle, &status);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmQueryServiceInfo(hHandle, &pInfo);
        BAIL_ON_ERROR(dwError);

        if (status.pid == pid && pInfo->type != LW_SERVICE_TYPE_DRIVER)
        {
            *phHandle = hHandle;
            hHandle = NULL;
            goto cleanup;
        }

        LwSmReleaseServiceHandle(hHandle);
        hHandle = NULL;
        LwSmFreeServiceInfo(pInfo);
        pInfo = NULL;
    }

    dwError = LW_ERROR_INVALID_PARAMETER;
    BAIL_ON_ERROR(dwError);

cleanup:

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    if (ppwszServiceNames)
    {
        LwSmFreeServiceNameList(ppwszServiceNames);
    }

    if (pInfo)
    {
        LwSmFreeServiceInfo(pInfo);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmGdb(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    LW_SERVICE_STATUS status = {0};
    PLW_SERVICE_INFO pInfo = NULL;
    PSTR pszExecutablePath = NULL;
    PSTR pszPid = NULL;

    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmQueryServiceStatus(hHandle, &status);
    BAIL_ON_ERROR(dwError);

    if (status.state != LW_SERVICE_STATE_RUNNING)
    {
        dwError = LwSmStart(argc, pArgv);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmQueryServiceStatus(hHandle, &status);
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwSmQueryServiceInfo(hHandle, &pInfo);
    BAIL_ON_ERROR(dwError);

    if (pInfo->type == LW_SERVICE_TYPE_DRIVER)
    {
        /* Find non-driver service so we can figure out what to attach to */
        LwSmReleaseServiceHandle(hHandle);
        hHandle = NULL;
        LwSmFreeServiceInfo(pInfo);
        pInfo = NULL;

        dwError = LwSmFindServiceWithPid(status.pid, &hHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmQueryServiceInfo(hHandle, &pInfo);
        if (dwError == LW_ERROR_INVALID_PARAMETER)
        {
            printf("Service type is not supported\n");
        }
        BAIL_ON_ERROR(dwError);
    }

    switch (pInfo->type)
    {
    case LW_SERVICE_TYPE_EXECUTABLE:
    case LW_SERVICE_TYPE_LEGACY_EXECUTABLE:
        dwError = LwWc16sToMbs(pInfo->pwszPath, &pszExecutablePath);
        BAIL_ON_ERROR(dwError);
        break;
    case LW_SERVICE_TYPE_MODULE:
        dwError = LwAllocateString(SBINDIR "/lwsmd", &pszExecutablePath);
        BAIL_ON_ERROR(dwError);
        break;
    default:
        printf("Service type is not supported\n");
        break;
    }

    dwError = LwAllocateStringPrintf(&pszPid, "%lu", (unsigned long) status.pid);
    BAIL_ON_ERROR(dwError);
    
    if (execlp("gdb", "gdb", pszExecutablePath, pszPid, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

error:

    LW_SAFE_FREE_MEMORY(pszPid);
    LW_SAFE_FREE_MEMORY(pszExecutablePath);

    if (pInfo)
    {
        LwSmFreeServiceInfo(pInfo);
    }

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    return dwError;
}

static
DWORD
LwSmSetLog(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    LW_SERVICE_HANDLE hHandle = NULL;
    LW_SM_LOGGER_TYPE type = 0;
    PSTR pFacility = NULL;
    PSTR pszTarget = NULL;
    PWSTR pServiceName = NULL;

    if (argc < 4)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    if (strcmp(pArgv[1], "-"))
    {
        dwError = LwMbsToWc16s(pArgv[1], &pServiceName);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmAcquireServiceHandle(pServiceName, &hHandle);
        BAIL_ON_ERROR(dwError);
    }

    if (strcmp(pArgv[2], "-"))
    {
        pFacility = pArgv[2];
    }

    if (!strcasecmp(pArgv[3], "-"))
    {
        type = LW_SM_LOGGER_DEFAULT;
    }
    else if (!strcasecmp(pArgv[3], "none"))
    {
        type = LW_SM_LOGGER_NONE;
    }
    else if (!strcasecmp(pArgv[3], "file"))
    {
        if (argc < 5)
        {     
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_ERROR(dwError);
        }
        type = LW_SM_LOGGER_FILE;
        pszTarget = pArgv[4];
    }
    else if (!strcasecmp(pArgv[3], "syslog"))
    {
        type = LW_SM_LOGGER_SYSLOG;
        pszTarget = NULL;
    }
    else
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwSmSetServiceLogTarget(hHandle, pFacility, type, pszTarget);
    BAIL_ON_ERROR(dwError);

error:

    return dwError;
}

static
VOID
PrintLogInfo(
    PCSTR pFacility,
    LW_SM_LOGGER_TYPE type,
    PCSTR pTarget,
    LW_SM_LOG_LEVEL level
    )
{
    PCSTR pszLoggerName = NULL;

    switch (type)
    {
    case LW_SM_LOGGER_NONE:
        pszLoggerName = "none";
        break;
    case LW_SM_LOGGER_FILE:
        pszLoggerName = "file";
        break;
    case LW_SM_LOGGER_SYSLOG:
        pszLoggerName = "syslog";
        break;
    case LW_SM_LOGGER_DEFAULT:
        pszLoggerName = "default";
        break;
    }

    if (pTarget)
    {
        printf("%s: %s %s at %s\n", pFacility, pszLoggerName, pTarget, LwSmLogLevelToString(level));
    }
    else
    {
        printf("%s: %s at %s\n", pFacility, pszLoggerName, LwSmLogLevelToString(level));
    }
}

static
DWORD
LwSmGetLog(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    LW_SM_LOGGER_TYPE type = 0;
    LW_SM_LOG_LEVEL level = 0;
    PSTR pszTarget = NULL;
    PSTR pFacility = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    PWSTR pServiceName = NULL;
    PWSTR* ppFacilities = NULL;
    DWORD index = 0;

    if (argc < 2)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    if (strcmp(pArgv[1], "-"))
    {
        dwError = LwMbsToWc16s(pArgv[1], &pServiceName);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmAcquireServiceHandle(pServiceName, &hHandle);
        BAIL_ON_ERROR(dwError);
    }

    if (argc == 2)
    {
        dwError = LwSmGetServiceLogState(hHandle, NULL, &type, &pszTarget, &level);
        BAIL_ON_ERROR(dwError);

        PrintLogInfo("<default>", type, pszTarget, level);

        if (pszTarget)
        {
            LwSmFreeLogTarget(pszTarget);
            pszTarget = NULL;
        }

        dwError = LwSmEnumerateServiceLogFacilities(hHandle, &ppFacilities);
        BAIL_ON_ERROR(dwError);

        for (index = 0; ppFacilities[index]; index++)
        {
            dwError = LwWc16sToMbs(ppFacilities[index], &pFacility);
            BAIL_ON_ERROR(dwError);

            dwError = LwSmGetServiceLogState(hHandle, pFacility, &type, &pszTarget, &level);
            BAIL_ON_ERROR(dwError);

            PrintLogInfo(pFacility, type, pszTarget, level);

            if (pszTarget)
            {
                LwSmFreeLogTarget(pszTarget);
                pszTarget = NULL;
            }
        }
    }
    else if (argc == 3)
    {
        if (strcmp(pArgv[2], "-"))
        {
            dwError = LwAllocateString(pArgv[2], &pFacility);
            BAIL_ON_ERROR(dwError);
        }

        dwError = LwSmGetServiceLogState(hHandle, pFacility, &type, &pszTarget, &level);
        BAIL_ON_ERROR(dwError);

        PrintLogInfo(pFacility, type, pszTarget, level);
    }

error:

    LW_SAFE_FREE_MEMORY(pFacility);

    if (pszTarget)
    {
        LwSmFreeLogTarget(pszTarget);
    }

    if (ppFacilities)
    {
        LwSmFreeLogFacilityList(ppFacilities);
    }

    return dwError;
}

static
DWORD
LwSmCmdSetLogLevel(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    LW_SM_LOG_LEVEL level = 0;
    PSTR pFacility = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    PWSTR pServiceName = NULL;

    if (argc < 4)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    if (strcmp(pArgv[1], "-"))
    {
        dwError = LwMbsToWc16s(pArgv[1], &pServiceName);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmAcquireServiceHandle(pServiceName, &hHandle);
        BAIL_ON_ERROR(dwError);
    }

    if (strcmp(pArgv[2], "-"))
    {
        pFacility = pArgv[2];
    }

    dwError = LwSmLogLevelNameToLogLevel(pArgv[3], &level);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmSetServiceLogLevel(hHandle, pFacility, level);
    BAIL_ON_ERROR(dwError);

error:

    return dwError;
}

static
DWORD
LwSmCmdSettings(
    VOID
    )
{
    DWORD dwError = ERROR_SUCCESS;
    BOOLEAN bWatchdog = FALSE;

    dwError = LwSmGetGlobal(LW_SM_GLOBAL_SETTING_WATCHDOG, &bWatchdog);
    BAIL_ON_ERROR(dwError);

    printf("watchdog: %s\n", bWatchdog ? "on" : "off");

error:

    return dwError;
}

static
DWORD
ParseBoolean(
    PCSTR pStr,
    PBOOLEAN pValue
    )
{
    if (!strcmp(pStr, "true") ||
        !strcmp(pStr, "on"))
    {
        *pValue = TRUE;
    }
    else if (!strcmp(pStr, "false") ||
             !strcmp(pStr, "off"))
    {
        *pValue = FALSE;
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;
}

static
DWORD
LwSmCmdSet(
    int argc,
    char** ppArgv
    )
{
    DWORD dwError = ERROR_SUCCESS;
    BOOLEAN booleanValue = FALSE;

    if (argc != 3)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    if (!strcmp(ppArgv[1], "watchdog"))
    {
        dwError = ParseBoolean(ppArgv[2], &booleanValue);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmSetGlobal(LW_SM_GLOBAL_SETTING_WATCHDOG, booleanValue);
        BAIL_ON_ERROR(dwError);
    }

error:

    return dwError;
}

static
VOID
LogTapper(
    PLW_TASK pTask,
    PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LONG64* pllTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int FifoFd = *(int*) pContext;
    siginfo_t info = {0};
    char buffer[2048] = {0};
    ssize_t count = 0;

    if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        status = STATUS_CANCELLED;
        BAIL_ON_ERROR(status);
    }
    else if (WakeMask & LW_TASK_EVENT_INIT)
    {
        status = LwRtlSetTaskUnixSignal(pTask, SIGINT, TRUE);
        BAIL_ON_ERROR(status);

        status = LwRtlSetTaskUnixSignal(pTask, SIGTERM, TRUE);
        BAIL_ON_ERROR(status);

        status = LwRtlSetTaskFd(pTask, FifoFd, LW_TASK_EVENT_FD_READABLE);
        BAIL_ON_ERROR(status);
    }
    else if (WakeMask & LW_TASK_EVENT_UNIX_SIGNAL)
    {
        while (LwRtlNextTaskUnixSignal(pTask, &info))
        {
            if (info.si_signo == SIGINT || info.si_signo == SIGTERM)
            {
                status = STATUS_CANCELLED;
                BAIL_ON_ERROR(status);
            }
        }
    }
    else if (WakeMask & LW_TASK_EVENT_FD_READABLE)
    {
        do
        {
            do
            {
                count = read(FifoFd, buffer, sizeof(buffer));
            } while (count < 0 && errno == EINTR);

            if (count == 0)
            {
                status = STATUS_END_OF_FILE;
                BAIL_ON_ERROR(status);
            }
            else if (count > 0)
            {
                count = write(1, buffer, count);
                if (count < 0)
                {
                    status = LwErrnoToNtStatus(errno);
                    BAIL_ON_ERROR(status);
                }
            }
            else if (errno != EAGAIN)
            {
                status = LwErrnoToNtStatus(errno);
                BAIL_ON_ERROR(status);
            }
        } while (count > 0);
    }

    *pWaitMask = LW_TASK_EVENT_FD_READABLE | LW_TASK_EVENT_UNIX_SIGNAL;

cleanup:

    return;

error:

    *pWaitMask = LW_TASK_EVENT_COMPLETE;
    LwRtlSetTaskFd(pTask, FifoFd, 0);
    LwRtlExitMain(status);

    goto cleanup;
}

static
DWORD
LwSmCmdTapLog(
    int argc,
    char** pArgv
    )
{
    DWORD error = ERROR_SUCCESS;
    PLW_THREAD_POOL pPool = NULL;
    PLW_TASK pTask = NULL;
    BOOLEAN bResetLogger = FALSE;
    BOOLEAN bRmPipe = FALSE;
    LW_SM_LOGGER_TYPE oldLogger = 0;
    PSTR pOldTarget = NULL;
    LW_SM_LOG_LEVEL oldLevel = 0;
    LW_SM_LOG_LEVEL newLevel = 0;
    PSTR pFacility = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    PWSTR pServiceName = NULL;
    PSTR pFifo = NULL;
    int FifoFd = -1;

    if (argc < 4)
    {
        error = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(error);
    }

    if (strcmp(pArgv[1], "-"))
    {
        error = LwMbsToWc16s(pArgv[1], &pServiceName);
        BAIL_ON_ERROR(error);

        error = LwSmAcquireServiceHandle(pServiceName, &hHandle);
        BAIL_ON_ERROR(error);
    }

    if (strcmp(pArgv[2], "-"))
    {
        pFacility = pArgv[2];
    }

    error = LwSmLogLevelNameToLogLevel(pArgv[3], &newLevel);
    BAIL_ON_ERROR(error);

    error = LwSmGetServiceLogState(hHandle, pFacility, &oldLogger, &pOldTarget, &oldLevel);
    BAIL_ON_ERROR(error);

    error = LwAllocateStringPrintf(&pFifo, "/tmp/.lwsm-log-tap-%lu", (unsigned long) getpid());
    BAIL_ON_ERROR(error);

    LwRtlBlockSignals();
    if (mknod(pFifo, S_IRUSR | S_IWUSR | S_IFIFO, 0) < 0)
    {
        error = LwErrnoToWin32Error(errno);
        BAIL_ON_ERROR(error);
    }
    bRmPipe = TRUE;

    if ((FifoFd = open(pFifo, O_RDONLY | O_NONBLOCK)) < 0)
    {
        error = LwErrnoToWin32Error(errno);
        BAIL_ON_ERROR(error);
    }

    if (fcntl(FifoFd, F_SETFL, O_NONBLOCK) < 0)
    {
        error = LwErrnoToWin32Error(errno);
        BAIL_ON_ERROR(error);
    }

    error = LwSmSetServiceLogTarget(hHandle, pFacility, LW_SM_LOGGER_FILE, pFifo);
    BAIL_ON_ERROR(error);
    bResetLogger = TRUE;

    error = LwSmSetServiceLogLevel(hHandle, pFacility, newLevel);
    BAIL_ON_ERROR(error);

    error = LwNtStatusToWin32Error(LwRtlCreateThreadPool(&pPool, NULL));
    BAIL_ON_ERROR(error);

    error = LwNtStatusToWin32Error(LwRtlCreateTask(pPool, &pTask, NULL, LogTapper, &FifoFd));
    BAIL_ON_ERROR(error);

    LwRtlWakeTask(pTask);

    error = LwNtStatusToWin32Error(LwRtlMain());
    BAIL_ON_ERROR(error);

error:

    if (pTask)
    {
        LwRtlCancelTask(pTask);
        LwRtlWaitTask(pTask);
        LwRtlReleaseTask(&pTask);
    }

    LwRtlFreeThreadPool(&pPool);

    if (bResetLogger)
    {
        error = LwSmSetServiceLogLevel(hHandle, pFacility, oldLevel);
        BAIL_ON_ERROR(error);

        error = LwSmSetServiceLogTarget(hHandle, pFacility, oldLogger, pOldTarget);
        BAIL_ON_ERROR(error);
    }

    if (pOldTarget)
    {
        LwSmFreeLogTarget(pOldTarget);
    }

    if (FifoFd >= 0)
    {
        close(FifoFd);
    }

    if (bRmPipe)
    {
        unlink(pFifo);
    }

    LW_SAFE_FREE_MEMORY(pFifo);

    return error;
}

static
DWORD
LwSmUsage(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;

    printf("Usage: %s [ options ... ] <command> ...\n\n", pArgv[0]);
    printf("Service commands:\n"
           "    list                       List all known services and their status\n"
           "    autostart                  Start all services configured for autostart\n"
           "    start-only <service>       Start a service\n"
           "    start <service>            Start a service and all dependencies\n"
           "    stop-only <service>        Stop a service\n"
           "    stop <service>             Stop a service and all running dependents\n"
           "    restart <service>          Restart a service and all running dependents\n"
           "    refresh <service>          Refresh service's configuration\n"
           "    proxy <service>            Act as a proxy process for a service\n"
           "    info <service>             Get information about a service\n"
           "    status <service>           Get the status of a service\n"
           "    get-log <service> [ <facility> ]\n"
           "                               List logging state given service and optional facility\n"
           "    set-log-target <service> <facility> <type> [ <target> ]\n"
           "                               Set log target for a given service and facility\n"
           "    set-log-level <service> <facility> <level>\n"
           "                               Set log level for a given service and facility\n"
           "    tap-log <service> <facility> <level>\n"
           "                               Temporarily redirect logging for the given service and\n"
           "                               facility to stdout with the given log level\n"
           "    gdb <service>              Attach gdb to the specified running service\n\n");
    printf("Maintenance commands:\n"
           "    shutdown                   Shutdown service manager\n"
           "    settings                   List global settings and values \n"
           "    set <setting> <value>      Change global setting\n\n");
    printf("Options:\n"
           "    -q, --quiet                Suppress console output\n"
           "    -h, --help                 Show usage information\n\n");

    return dwError;
}

int
main(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    CHAR szErrorMessage[2048];
    int ret = 0;
    int i = 0;
    PCSTR pszErrorName = NULL;

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(pArgv[i], "-h") ||
            !strcmp(pArgv[i], "--help"))
        {
            dwError = LwSmUsage(argc, pArgv);
            goto error;
        }
        if (!strcmp(pArgv[i], "-q") ||
            !strcmp(pArgv[i], "--quiet"))
        {
            gState.bQuiet = TRUE;
        }
        else if (!strcmp(pArgv[i], "list"))
        {
            dwError = LwSmList(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "start-only"))
        {
            dwError = LwSmStartOnly(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "start"))
        {
            dwError = LwSmStart(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "stop-only"))
        {
            dwError = LwSmStopOnly(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "stop"))
        {
            dwError = LwSmStop(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "info"))
        {
            dwError = LwSmInfo(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "status"))
        {
            dwError = LwSmStatus(argc-i, pArgv+i, &ret);
            goto error;
        }
        else if (!strcmp(pArgv[i], "refresh"))
        {
            dwError = LwSmDoRefresh(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "restart"))
        {
            dwError = LwSmRestart(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "proxy"))
        {
            dwError = LwSmProxy(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "gdb"))
        {
            dwError = LwSmGdb(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "set-log-target"))
        {
            dwError = LwSmSetLog(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "get-log"))
        {
            dwError = LwSmGetLog(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "set-log-level"))
        {
            dwError = LwSmCmdSetLogLevel(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "tap-log"))
        {
            dwError = LwSmCmdTapLog(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "autostart"))
        {
            dwError = LwSmAutostart(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "shutdown"))
        {
            dwError = LwSmShutdown();
            goto error;
        }
        else if (!strcmp(pArgv[i], "settings"))
        {
            dwError = LwSmCmdSettings();
            goto error;
        }
        else if (!strcmp(pArgv[i], "set"))
        {
            dwError = LwSmCmdSet(argc - 1, pArgv + 1);
            goto error;
        }
        else
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_ERROR(dwError);
        }
    }

    dwError = LwSmUsage(argc, pArgv);
    BAIL_ON_ERROR(dwError);

error:

    if (dwError)
    {
        memset(szErrorMessage, 0, sizeof(szErrorMessage));
        LwGetErrorString(dwError, szErrorMessage, sizeof(szErrorMessage) - 1);
        pszErrorName = LwWin32ExtErrorToName(dwError);

        if (!gState.bQuiet)
        {
            printf("Error: %s (%lu)\n", pszErrorName ? pszErrorName : "UNKNOWN", (unsigned long) dwError);
            printf("%s\n", szErrorMessage);
        }

        return 1;
    }
    else
    {
        return ret;
    }
}
