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
 *        test_openldap.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Test program
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwsecurityidentifier.h"
#include "lsautils.h"
#include "lsasrvutils.h"
#include "lwldap.h"

#define GOTO_CLEANUP_ON_ERROR(dwError) \
    do { \
        if (dwError) \
        { \
            goto cleanup; \
        } \
    } while (0)

#define GOTO_CLEANUP_ON_ERROR_EE(dwError, EE) \
    do { \
        if (dwError) \
        { \
            (EE) = __LINE__; \
            goto cleanup; \
        } \
    } while (0)

DWORD
GetCurrentTime(
    OUT time_t* pSeconds,
    OUT USEC_T* pMicroSeconds
    )
{
    DWORD dwError = 0;
    time_t seconds = 0;
    USEC_T microSeconds = 0;
    struct timeval now;

    if (gettimeofday(&now, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        GOTO_CLEANUP_ON_ERROR(dwError);
    }

    if (now.tv_sec <= 0)
    {
        dwError = LW_ERROR_INTERNAL;
        GOTO_CLEANUP_ON_ERROR(dwError);
    }

    if (now.tv_usec <= 0)
    {
        dwError = LW_ERROR_INTERNAL;
        GOTO_CLEANUP_ON_ERROR(dwError);
    }

    seconds = now.tv_sec;
    microSeconds = now.tv_usec;

cleanup:
    *pSeconds = seconds;
    *pMicroSeconds = microSeconds;
    return dwError;
}

static pthread_mutex_t LogMutex = PTHREAD_MUTEX_INITIALIZER;

VOID
LogV(
    PCSTR pszFormat,
    va_list args
    )
{
    time_t now = 0;
    USEC_T microSeconds = 0;
    struct tm nowLocal = { 0 };
    char buffer[64];
    FILE* fp = stdout;

    pthread_mutex_lock(&LogMutex);

    GetCurrentTime(&now, &microSeconds);

    localtime_r(&now, &nowLocal);

    strftime(buffer, sizeof(buffer), "%Y/%m/%d-%H:%M:%S", &nowLocal);

    fprintf(fp, "%s.%06d - ", buffer, (int) microSeconds);
    vfprintf(fp, pszFormat, args);
    if (pszFormat && pszFormat[0] && (pszFormat[strlen(pszFormat)-1] != '\n'))
    {
        fprintf(fp, "\n");
    }
    fflush(fp);

    pthread_mutex_unlock(&LogMutex);
}

VOID
Log(
    PCSTR  pszFormat,
    ...
    )
{
    va_list args;
    va_start(args, pszFormat);

    LogV(pszFormat, args);

    va_end(args);
}

#define LOG(Format, ...) \
    Log("[0x%08x] %s() - " Format, \
        (unsigned int)pthread_self(), \
        __FUNCTION__, \
        ## __VA_ARGS__)

#define LOG_TID(Format, Id, ...) \
    LOG("[%d] - " Format, Id, ## __VA_ARGS__)

DWORD
SearchTest(
    IN HANDLE hDirectory,
    IN PCSTR pszScopeDn,
    IN PCSTR pszQuery,
    IN OPTIONAL PDWORD pdwThreadId
    )
{
    DWORD dwError = 0;
    int EE = 0;
    LDAPMessage* pMessage = NULL;
    LDAP* pLd = NULL;
    int count = 0;
    PSTR attributes[] = { "distinguishedName", NULL };
    int tid = pdwThreadId ? (int) *pdwThreadId : -1;

    LOG_TID("Searching scope '%s' for '%s'\n",
            tid, pszScopeDn, pszQuery);

    dwError = LwLdapDirectorySearch(
                    hDirectory,
                    pszScopeDn,
                    LDAP_SCOPE_SUBTREE,
                    pszQuery,
                    attributes,
                    &pMessage);
    GOTO_CLEANUP_ON_ERROR_EE(dwError, EE);

    pLd = LwLdapGetSession(hDirectory);
    count = ldap_count_entries(pLd, pMessage);
    LOG_TID("Got %d entries\n", tid, count);
    if (count < 0)
    {
        dwError = -1;
        GOTO_CLEANUP_ON_ERROR_EE(dwError, EE);
    }

cleanup:
    if (dwError)
    {
        LOG_TID("dwError = %u, EE = %d", tid, dwError, EE);
    }
    return dwError;
}

typedef struct _SEARCH_ARGS {
    PCSTR pszScopeDn;
    PCSTR pszQuery;
} SEARCH_ARGS, *PSEARCH_ARGS;

typedef struct _THREADS_CONTEXT {
    HANDLE hDirectory;
    DWORD dwRepeatCount;
    pthread_mutex_t* pMutex;
    pthread_cond_t* pCond;
} THREADS_CONTEXT, *PTHREADS_CONTEXT;

typedef struct _THREAD_CONTEXT {
    pthread_t Thread;
    pthread_t* pThread;
    DWORD dwId;
    PTHREADS_CONTEXT pContext;
    PSEARCH_ARGS pSearchArgs;
} THREAD_CONTEXT, *PTHREAD_CONTEXT;


void*
SearchTestThread(
    IN void* pContext
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PTHREAD_CONTEXT pThreadContext = (PTHREAD_CONTEXT) pContext;
    DWORD dwIndex = 0;
    HANDLE hDirectory = 0;
    DWORD dwRepeatCount = 0;

    // Just to synchronize thread startup...
    pthread_mutex_lock(pThreadContext->pContext->pMutex);
    hDirectory = pThreadContext->pContext->hDirectory;
    dwRepeatCount = pThreadContext->pContext->dwRepeatCount;
    pthread_mutex_unlock(pThreadContext->pContext->pMutex);

    for (dwIndex = 0; dwIndex < dwRepeatCount; dwIndex++)
    {
        dwError = SearchTest(hDirectory,
                             pThreadContext->pSearchArgs->pszScopeDn,
                             pThreadContext->pSearchArgs->pszQuery,
                             &pThreadContext->dwId);
        GOTO_CLEANUP_ON_ERROR_EE(dwError, EE);
    }

cleanup:
    if (dwError)
    {
        LOG("dwError = %u, EE = %d", dwError, EE);
    }
    return NULL;
}

DWORD
SearchTestThreaded(
    IN HANDLE hDirectory,
    IN DWORD dwRepeatCount,
    IN DWORD dwThreadCount,
    IN DWORD dwSearchArgsCount,
    IN PSEARCH_ARGS pSearchArgs
    )
{
    DWORD dwError = 0;
    int EE = 0;
    THREADS_CONTEXT ctx = { 0 };
    pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t Cond = PTHREAD_COND_INITIALIZER;
    DWORD dwIndex = 0;
    PTHREAD_CONTEXT pThreadContext = NULL;
    DWORD dwSearchIndex = 0;
    BOOLEAN bIsAcquired = FALSE;

    dwError = LwAllocateMemory(sizeof(pThreadContext[0]) * dwThreadCount,
                                (PVOID*) &pThreadContext);
    GOTO_CLEANUP_ON_ERROR_EE(dwError, EE);

    ctx.hDirectory = hDirectory;
    ctx.dwRepeatCount = dwRepeatCount;
    ctx.pMutex = &Mutex;
    ctx.pCond = &Cond;

    pthread_mutex_lock(ctx.pMutex);
    bIsAcquired = TRUE;

    for (dwIndex = 0; dwIndex < dwThreadCount; dwIndex++)
    {
        if (dwSearchIndex >= dwSearchArgsCount)
        {
            dwSearchIndex = 0;
        }

        pThreadContext[dwIndex].dwId = dwIndex + 1;
        pThreadContext[dwIndex].pContext = &ctx;
        pThreadContext[dwIndex].pSearchArgs = &pSearchArgs[dwSearchIndex++];

        dwError = pthread_create(&pThreadContext[dwIndex].Thread, NULL, SearchTestThread, &pThreadContext[dwIndex]);
        GOTO_CLEANUP_ON_ERROR_EE(dwError, EE);

        pThreadContext[dwIndex].pThread = &pThreadContext[dwIndex].Thread;
    }

    pthread_mutex_unlock(ctx.pMutex);
    bIsAcquired = FALSE;

cleanup:
    if (dwError)
    {
        LOG("dwError = %u, EE = %d", dwError, EE);
    }
    if (bIsAcquired)
    {
        pthread_mutex_unlock(ctx.pMutex);
        bIsAcquired = FALSE;
    }
    if (pThreadContext)
    {
        for (dwIndex = 0; dwIndex < dwThreadCount; dwIndex++)
        {
            if (pThreadContext[dwIndex].pThread)
            {
                void* result = NULL;
                pthread_join(pThreadContext[dwIndex].Thread, &result);
            }
        }
        LwFreeMemory(pThreadContext);
    }
    return dwError;
}

void
usage(
    IN PCSTR pszProgram
    )
{
    printf("usage: %s <serverAddress> [<serverFqdn> [<scopeDn> <query>]\n", pszProgram);
    exit(1);
}

int
main(
    IN int argc,
    IN const char* argv[]
    )
{
    DWORD dwError = 0;
    int EE = 0;
    int argIndex = 0;
    PCSTR pszProgram = argv[argIndex++];
    PCSTR pszServerAddress = NULL;
    PCSTR pszServerName = NULL;
    HANDLE hDirectory = 0;
    DWORD dwSearchArgsCount = 0;
    PSEARCH_ARGS pSearchArgs = NULL;
    DWORD dwIndex = 0;
    DWORD dwRepeatCount = 0;
    DWORD dwThreadCount = 0;

    if (argc < 2)
    {
        usage(pszProgram);
    }

    pszServerAddress = argv[argIndex++];
    if (argc > argIndex)
    {
        pszServerName = argv[argIndex++];
    }

    if (argc > argIndex)
    {
        int count = atoi(argv[argIndex++]);
        if (count <= 0)
        {
            printf("Invalid repeat count: %d\n", count);
            usage(pszProgram);
        }
        dwRepeatCount = (DWORD) count;
    }

    if (argc > argIndex)
    {
        int count = atoi(argv[argIndex++]);
        if (count <= 0)
        {
            printf("Invalid thread count: %d\n", count);
            usage(pszProgram);
        }
        dwThreadCount = (DWORD) count;
    }

    if ((argc - argIndex) % 2)
    {
        printf("Invalid # of arguments\n");
        usage(pszProgram);
    }

    dwSearchArgsCount = (argc - argIndex) / 2;
    if (dwSearchArgsCount > 0)
    {
        dwError = LwAllocateMemory(sizeof(*pSearchArgs) * dwSearchArgsCount,
                                    (PVOID*) &pSearchArgs);
        GOTO_CLEANUP_ON_ERROR_EE(dwError, EE);

        for (dwIndex = 0; dwIndex < dwSearchArgsCount; dwIndex++)
        {
            pSearchArgs[dwIndex].pszScopeDn = argv[argIndex++];
            pSearchArgs[dwIndex].pszQuery = argv[argIndex++];
        }
    }

    LOG("Pinging address '%s'\n", pszServerAddress);

    dwError = LwLdapPingTcp(pszServerAddress, 5);
    GOTO_CLEANUP_ON_ERROR_EE(dwError, EE);

    if (!pszServerName)
    {
        goto cleanup;
    }

    LOG("Opening '%s'\n", pszServerName);

    dwError = LwLdapOpenDirectoryServer(
                    pszServerAddress,
                    pszServerName,
                    0,
                    &hDirectory);
    GOTO_CLEANUP_ON_ERROR_EE(dwError, EE);

    if (dwSearchArgsCount > 0)
    {
        dwError = SearchTestThreaded(hDirectory, dwRepeatCount, dwThreadCount, dwSearchArgsCount, pSearchArgs);
        GOTO_CLEANUP_ON_ERROR_EE(dwError, EE);
    }

cleanup:
    if (dwError)
    {
        LOG("dwError = %u (EE = %d)", dwError, EE);
        printf("ERROR\n");
    }
    else
    {
        printf("SUCCESS\n");
    }
    return dwError;
}

