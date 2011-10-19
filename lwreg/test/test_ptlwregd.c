/*
 * Copyright Likewise Software    2004-2009
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        test_ptlwregd.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Multi-threaded lwregd test client
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */

#define TEST_ADD        0
#define TEST_DELETE     1
#define TEST_GET_KEY    2
#define TEST_GET_VALUE  3


#define LWREGD_MAX_THEADS 2
#define LWREGD_MAX_ITERATIONS 10


#include "includes.h"
#include "rsutils.h"

typedef struct _PTLWREGD_CONTEXT
{
    HANDLE hReg;
    pthread_t thread;
    PSTR pszKeyPath;
    PSTR pszKeyNamePrefix;
    DWORD dwRange;
    DWORD dwIterations;
    DWORD dwOperation;
} PTLWREGD_CONTEXT, *PPTLWREGD_CONTEXT;


DWORD
ThreadTestAddKey(
    HANDLE hReg,
    PSTR pszKeyPath,
    PSTR pszKeyNamePrefix,
    DWORD dwRange,
    DWORD dwIterations)
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    DWORD dwKeyNum = 0;
    PSTR pszKeyName = NULL;
    PSTR pszSubKeyPath = NULL;

    for (dwCount=0; dwCount<dwIterations; dwCount++)
    {
        printf("ThreadTestAddKey: %d %s\\%s\n",
               dwCount,
               pszKeyPath,
               pszKeyNamePrefix);
        for (dwKeyNum=0; dwKeyNum<dwRange; dwKeyNum++)
        {
            dwError = RegCStringAllocatePrintf(
                      &pszKeyName,
                      "%s-%d",
                      pszKeyNamePrefix,
                      dwKeyNum);
            BAIL_ON_REG_ERROR(dwError);

            dwError = RegShellUtilAddKey(hReg, NULL, pszKeyPath, pszKeyName, FALSE);
            BAIL_ON_REG_ERROR(dwError);

            printf("    >>ThreadTestAddKey: %d %s\\%s\n",
                   dwCount,
                   pszKeyPath,
                   pszKeyName);

            dwError = RegCStringAllocatePrintf(
                      &pszSubKeyPath,
                      "%s\\%s",
                      pszKeyPath,
                      pszKeyName);
            BAIL_ON_REG_ERROR(dwError);

            dwError = RegShellUtilAddKey(hReg, NULL, pszSubKeyPath, pszKeyName, FALSE);
            BAIL_ON_REG_ERROR(dwError);

            printf("    >>ThreadTestAddKey: %d %s\\%s\n",
                   dwCount,
                   pszSubKeyPath,
                   pszKeyName);
        }
    }

cleanup:
    LWREG_SAFE_FREE_STRING(pszKeyName);
    LWREG_SAFE_FREE_STRING(pszSubKeyPath);
    return dwError;

error:
    if (dwError)
    {
        RegPrintError("ThreadTestAddKey", dwError);
    }
    goto cleanup;
}

DWORD
ThreadTestGetKeys(
    HANDLE hReg,
    PSTR pszKeyPath,
    PSTR pszKeyNamePrefix,
    DWORD dwRange,
    DWORD dwIterations)
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    DWORD dwKeyNum = 0;
    PSTR pszKeyName = NULL;
    LW_WCHAR **ppSubKeys = NULL;
    DWORD dwRetSubKeyCount = 0;
    int i = 0;

    for (dwCount=0; dwCount<dwIterations; dwCount++)
    {
        printf("ThreadTestGetKeys: %d %s\\%s\n",
               dwCount,
               pszKeyPath,
               pszKeyNamePrefix);
        for (dwKeyNum=0; dwKeyNum<dwRange; dwKeyNum++)
        {
            dwError = RegCStringAllocatePrintf(
                      &pszKeyName,
                      "%s-%d",
                      pszKeyNamePrefix,
                      dwKeyNum);
            BAIL_ON_REG_ERROR(dwError);

            dwError = RegShellUtilGetKeys(hReg, NULL, pszKeyPath, pszKeyName, &ppSubKeys, &dwRetSubKeyCount);
            BAIL_ON_REG_ERROR(dwError);

            printf("    >>ThreadTestGetKeys: %d %s\\%s has %d subkeys\n",
                   dwCount,
                   pszKeyPath,
                   pszKeyName,
                   dwRetSubKeyCount);
        }
    }

cleanup:
    for (i = 0; i < dwRetSubKeyCount; i++)
    {
        LWREG_SAFE_FREE_MEMORY(ppSubKeys[i]);
    }
    LWREG_SAFE_FREE_MEMORY(ppSubKeys);
    LWREG_SAFE_FREE_STRING(pszKeyName);

    return dwError;

error:
    if (dwError)
    {
        RegPrintError("ThreadTestGetKeys", dwError);
    }

    goto cleanup;
}

DWORD
ThreadTestGetValues(
    HANDLE hReg,
    PSTR pszKeyPath,
    PSTR pszKeyNamePrefix,
    DWORD dwRange,
    DWORD dwIterations)
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    DWORD dwKeyNum = 0;
    PSTR pszKeyName = NULL;
    PREGSHELL_UTIL_VALUE valueArray = NULL;
    DWORD dwValueArrayLen = 0;
    int i = 0;

    for (dwCount=0; dwCount<dwIterations; dwCount++)
    {
        printf("ThreadTestGetValues: %d %s\\%s\n",
               dwCount,
               pszKeyPath,
               pszKeyNamePrefix);
        for (dwKeyNum=0; dwKeyNum<dwRange; dwKeyNum++)
        {
            dwError = RegCStringAllocatePrintf(
                      &pszKeyName,
                      "%s-%d",
                      pszKeyNamePrefix,
                      dwKeyNum);
            BAIL_ON_REG_ERROR(dwError);

            dwError = RegShellUtilGetValues(hReg, NULL, pszKeyPath, pszKeyName, &valueArray, &dwValueArrayLen);
            BAIL_ON_REG_ERROR(dwError);

            printf("    >>ThreadTestGetValues: %d %s\\%s has %d value\n",
                   dwCount,
                   pszKeyPath,
                   pszKeyName,
                   dwValueArrayLen);
        }
    }

cleanup:
    for (i=0; i<dwValueArrayLen; i++)
    {
        LWREG_SAFE_FREE_MEMORY(valueArray[i].pValueName);
        LWREG_SAFE_FREE_MEMORY(valueArray[i].pData);
    }
    LWREG_SAFE_FREE_MEMORY(valueArray);
    LWREG_SAFE_FREE_STRING(pszKeyName);

    return dwError;

error:
    if (dwError)
    {
        RegPrintError("ThreadTestGetValues", dwError);
    }
    goto cleanup;
}

DWORD
ThreadTestDeleteKey(
    HANDLE hReg,
    PSTR pszKeyPath,
    PSTR pszKeyNamePrefix,
    DWORD dwRange,
    DWORD dwIterations)
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    DWORD dwKeyNum = 0;
    PSTR pszKeyName = NULL;
    PSTR pszSubKeyPath = NULL;

    for (dwCount=0; dwCount<dwIterations; dwCount++)
    {
        printf("ThreadTestDeleteKey: %d %s\\%s\n",
               dwCount,
               pszKeyPath,
               pszKeyNamePrefix);
        for (dwKeyNum=0; dwKeyNum<dwRange; dwKeyNum++)
        {
            dwError = RegCStringAllocatePrintf(
                      &pszKeyName,
                      "%s-%d",
                      pszKeyNamePrefix,
                      dwKeyNum);
            BAIL_ON_REG_ERROR(dwError);

            dwError = RegCStringAllocatePrintf(
                      &pszSubKeyPath,
                      "%s\\%s",
                      pszKeyPath,
                      pszKeyName);
            BAIL_ON_REG_ERROR(dwError);

            dwError = RegShellUtilDeleteKey(hReg, NULL, pszSubKeyPath, pszKeyName);
            BAIL_ON_REG_ERROR(dwError);

            printf("    >>ThreadTestDeleteKey: %d %s\\%s\n",
                   dwCount,
                   pszSubKeyPath,
                   pszKeyName);

            dwError = RegShellUtilDeleteKey(hReg, NULL, pszKeyPath, pszKeyName);
            BAIL_ON_REG_ERROR(dwError);

            printf("    >>ThreadTestDeleteKey: %d %s\\%s\n",
                   dwCount,
                   pszKeyPath,
                   pszKeyName);
        }
    }

cleanup:
    LWREG_SAFE_FREE_STRING(pszKeyName);
    LWREG_SAFE_FREE_STRING(pszSubKeyPath);
    return dwError;

error:
    if (dwError == LWREG_ERROR_FAILED_DELETE_HAS_SUBKEY ||
        dwError == LWREG_ERROR_KEY_IS_ACTIVE ||
        dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = 0;
    }

    if (dwError)
    {
        RegPrintError("ThreadTestDeleteKey", dwError);
    }
    goto cleanup;
}


void *
ThreadTestPtKey(
    void *pctx)
{
    DWORD dwError = 0;
    PPTLWREGD_CONTEXT context = (PPTLWREGD_CONTEXT) pctx;
    PSTR pszOperation = NULL;

    switch (context->dwOperation)
        {
        case TEST_ADD:
            pszOperation = "AddKey";

            break;

        case TEST_DELETE:
            pszOperation = "DeleteKey";

            break;

        case TEST_GET_KEY:
            pszOperation = "GetKeys";

            break;

        case TEST_GET_VALUE:
            pszOperation = "GetValues";
            break;

        default:
            pszOperation = "Undefined";
        }


    printf("ThreadTestPt%sKey: starting %s\\%s\n",
           pszOperation,
           context->pszKeyPath,
           context->pszKeyNamePrefix);


    switch (context->dwOperation)
    {
    case TEST_ADD:
        dwError = ThreadTestAddKey(
                      context->hReg,
                      context->pszKeyPath,
                      context->pszKeyNamePrefix,
                      context->dwRange,
                      context->dwIterations);
        break;

    case TEST_DELETE:
        dwError = ThreadTestDeleteKey(
                      context->hReg,
                      context->pszKeyPath,
                      context->pszKeyNamePrefix,
                      context->dwRange,
                      context->dwIterations);
        break;

    case TEST_GET_KEY:
        dwError = ThreadTestGetKeys(
                     context->hReg,
                     context->pszKeyPath,
                     context->pszKeyNamePrefix,
                     context->dwRange,
                     context->dwIterations);
        break;

    case TEST_GET_VALUE:
        dwError = ThreadTestGetValues(
                     context->hReg,
                     context->pszKeyPath,
                     context->pszKeyNamePrefix,
                     context->dwRange,
                     context->dwIterations);
        pszOperation = "GetValues";
        break;

    default:
        pszOperation = "Undefined";
    }
    BAIL_ON_REG_ERROR(dwError);

    printf("ThreadTestPt%sKey: %s\\%s done.\n",
           pszOperation,
           context->pszKeyPath,
           context->pszKeyNamePrefix);
cleanup:
    return NULL;

error:
    goto cleanup;
}

DWORD
ThreadTestPtFree(
    PPTLWREGD_CONTEXT pCtx)
{
    DWORD dwError = 0;
    BAIL_ON_INVALID_HANDLE(pCtx);

    RegCloseServer(pCtx->hReg);
    RegMemoryFree(pCtx->pszKeyNamePrefix);
    RegMemoryFree(pCtx->pszKeyPath);
    RegMemoryFree(pCtx);
cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
ThreadTestPtInit(
    PSTR pszKeyPath,
    PSTR pszKeyNamePrefix,
    DWORD dwKeyNameSuffix,
    DWORD dwIterations,
    DWORD dwOperation,
    PPTLWREGD_CONTEXT *ppRetCtx)
{
    PPTLWREGD_CONTEXT pCtx = NULL;
    DWORD dwError = 0;
    HANDLE hReg = NULL;

    dwError = RegAllocateMemory(sizeof(*pCtx), (PVOID*)&pCtx);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenServer(&hReg);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegCStringDuplicate(&pCtx->pszKeyPath, pszKeyPath);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegAllocateMemory(strlen(pszKeyNamePrefix) + 11, (PVOID*)&pCtx->pszKeyNamePrefix);
    BAIL_ON_REG_ERROR(dwError);

    sprintf(pCtx->pszKeyNamePrefix, "%s%d", pszKeyNamePrefix, dwKeyNameSuffix);

    pCtx->hReg = hReg;
    pCtx->dwRange = 1000;
    pCtx->dwIterations = dwIterations;
    pCtx->dwOperation = dwOperation;

    *ppRetCtx = pCtx;

cleanup:
    return dwError;

error:
    goto cleanup;
}


int main(int argc, char *argv[])
{
    PPTLWREGD_CONTEXT ctxAdd[LWREGD_MAX_THEADS] = {0};
    PPTLWREGD_CONTEXT ctxDel[LWREGD_MAX_THEADS] = {0};
    PPTLWREGD_CONTEXT ctxGetKeys[LWREGD_MAX_THEADS] = {0};
    PPTLWREGD_CONTEXT ctxGetValues[LWREGD_MAX_THEADS] = {0};

    DWORD dwError = 0;

    int sts = 0;
    DWORD i = 0;

    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        ThreadTestPtInit("thread_tests",
                         "TestKey",
                         i,
                         LWREGD_MAX_ITERATIONS,
                         TEST_ADD,
                         &ctxAdd[i]);
    }

    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        ThreadTestPtInit("thread_tests",
                         "TestKey",
                         i,
                         LWREGD_MAX_ITERATIONS,
                         TEST_DELETE,
                         &ctxDel[i]);
    }

    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        ThreadTestPtInit("thread_tests",
                         "TestKey",
                         i,
                         LWREGD_MAX_ITERATIONS,
                         TEST_GET_KEY,
                         &ctxGetKeys[i]);
    }

    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        ThreadTestPtInit("thread_tests",
                         "TestKey",
                         i,
                         LWREGD_MAX_ITERATIONS,
                         TEST_GET_VALUE,
                         &ctxGetValues[i]);
    }

    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        sts = pthread_create(&ctxAdd[i]->thread,
                             NULL,
                             ThreadTestPtKey,
                             ctxAdd[i]);
        if (sts == -1)
        {
            printf("pthread_create: Error ThreadTestPtAddkey(ctxAdd[%d])\n", i);
            return 1;
        }
    }
    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        sts = pthread_create(&ctxDel[i]->thread,
                             NULL,
                             ThreadTestPtKey,
                             ctxDel[i]);
        if (sts == -1)
        {
            printf("pthread_create: Error ThreadTestPtDeletekey(ctxDel[%d])\n", i);
            return 1;
        }
    }
    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        sts = pthread_create(&ctxGetKeys[i]->thread,
                             NULL,
                             ThreadTestPtKey,
                             ctxGetKeys[i]);
        if (sts == -1)
        {
            printf("pthread_create: Error ThreadTestPtGetKeys(ctxGetKeys[%d])\n", i);
            return 1;
        }
    }
    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        sts = pthread_create(&ctxGetValues[i]->thread,
                             NULL,
                             ThreadTestPtKey,
                             ctxGetValues[i]);
        if (sts == -1)
        {
            printf("pthread_create: Error ThreadTestPtGetValues(ctxGetValues[%d])\n", i);
            return 1;
        }
    }


    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        pthread_join(ctxAdd[i]->thread, NULL);
        pthread_join(ctxDel[i]->thread, NULL);
        pthread_join(ctxGetKeys[i]->thread, NULL);
        pthread_join(ctxGetValues[i]->thread, NULL);
    }
    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        ThreadTestPtFree(ctxAdd[i]);
        ThreadTestPtFree(ctxDel[i]);
        ThreadTestPtFree(ctxGetKeys[i]);
        ThreadTestPtFree(ctxGetValues[i]);
    }
    return dwError;
}
