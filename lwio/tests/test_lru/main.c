/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2010
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
 *        Likewise IO (LWIO)
 *
 *        Test Program for LRU
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lwstr.h"
#include "lwiosys.h"
#include "lwio/lwio.h"
#include "lwiodef.h"
#include "lwioutils.h"
#include "lwiolru.h"

#define TEST_LRU_RUN(func)                                                  \
{                                                                           \
    NTSTATUS ntStatus = STATUS_SUCCESS;                                     \
    printf("Running test %s...\n", #func);                                  \
    SetUp();                                                                \
    ntStatus = func();                                                      \
    if (ntStatus != STATUS_SUCCESS)                                         \
    {                                                                       \
        printf("ERROR running %s: %d\n", #func, ntStatus);                  \
    }                                                                       \
    TearDown();                                                             \
}

#define TEST_ASSERT_STATUS_SUCCESS(status)                                  \
    {                                                                       \
        NTSTATUS ntStatus = status;                                         \
        if (!NT_SUCCESS(ntStatus))                                          \
        {                                                                   \
            printf("STATUS ERROR %d in %s, file %s, line %d\n",             \
                ntStatus, __FUNCTION__, __FILE__, __LINE__);                \
            BAIL_ON_NT_STATUS(ntStatus);                                    \
        }                                                                   \
    }

#define TEST_ASSERT(status)                                                 \
    {                                                                       \
        if (!(status))                                                      \
        {                                                                   \
            printf("ASSERT FAILURE in %s, file %s, line %d\n",              \
                __FUNCTION__, __FILE__, __LINE__);                          \
            BAIL_ON_NT_STATUS(STATUS_UNSUCCESSFUL);                         \
        }                                                                   \
    }

#define LRU_SIZE                    5
#define LRU_HASH_SIZE               10
#define LRU_PATH_HASH_MULTIPLIER    31

PLWIO_LRU pLru = NULL;

static
void
ShowUsage()
{
    printf("Usage: test-lru\n");
}

static
int
ParseArgs(
    int argc,
    char* argv[]
    )
{
    int iArg = 1;
    PSTR pszArg = NULL;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }

        if ((strcmp(pszArg, "--help") == 0) || (strcmp(pszArg, "-h") == 0))
        {
            ShowUsage();
            exit(0);
        }
    } while (iArg < argc);

    return 0;
}

static
ULONG
LruHash(
    PCVOID pszPath
    )
{
    ULONG KeyResult = 0;
    PCSTR pszPathname = (PCSTR)pszPath;
    PCSTR pszChar = NULL;

    for(pszChar = pszPathname; pszChar && *pszChar; pszChar++)
    {
        KeyResult = (LRU_PATH_HASH_MULTIPLIER * KeyResult) + 
                    (ULONG)tolower(*pszChar);
    }

    return KeyResult;
}

static
VOID
LruFree(
    LWIO_LRU_ENTRY entry
    )
{
    PSTR pszKey = (PSTR)entry.pKey;
    PSTR pszValue = (PSTR)entry.pValue;

    LwRtlCStringFree(&pszKey);
    LwRtlCStringFree(&pszValue);
}

static
LONG
LruComparator(
    PCVOID pKey1,
    PCVOID pKey2
    )
{
    PSTR pszPath1 = (PSTR)pKey1;
    PSTR pszPath2 = (PSTR)pKey2;

    return strcasecmp(pszPath1, pszPath2);
}

static
VOID
SetUp(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    
    ntStatus = LwioLruCreate(LRU_SIZE, 
                             LRU_HASH_SIZE, 
                             LruComparator, 
                             LruHash, 
                             LruFree, 
                             &pLru);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    if (!NT_SUCCESS(ntStatus))
    {
        printf("ERROR in SetUp: %d, aborting...\n", ntStatus);
        abort();
    }
}

static
VOID
TearDown(
    VOID
    )
{
    LwioLruSafeFree(&pLru);
}

static
NTSTATUS
TestInsertRemove(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR k1, k2, k3, k4, k5, k6;
    PSTR v1, v2, v3, v4, v5, v6;
    PLWIO_LRU_ITERATOR it = NULL;
    PLWIO_LRU_ENTRY pEntry = NULL;
    char* answerKey[] = {"k5", "k3", "k1", "k4", "k2"};
    char* answerValue[] = {"v5", "v3", "v1", "v4", "v2"};
    ULONG i = 0;
    PSTR pValue = NULL;

    TEST_ASSERT_STATUS_SUCCESS(SMBAllocateString("k1", &k1));
    TEST_ASSERT_STATUS_SUCCESS(SMBAllocateString("k2", &k2));
    TEST_ASSERT_STATUS_SUCCESS(SMBAllocateString("k3", &k3));
    TEST_ASSERT_STATUS_SUCCESS(SMBAllocateString("k4", &k4));
    TEST_ASSERT_STATUS_SUCCESS(SMBAllocateString("k5", &k5));
    TEST_ASSERT_STATUS_SUCCESS(SMBAllocateString("k6", &k6));

    TEST_ASSERT_STATUS_SUCCESS(SMBAllocateString("v1", &v1));
    TEST_ASSERT_STATUS_SUCCESS(SMBAllocateString("v2", &v2));
    TEST_ASSERT_STATUS_SUCCESS(SMBAllocateString("v3", &v3));
    TEST_ASSERT_STATUS_SUCCESS(SMBAllocateString("v4", &v4));
    TEST_ASSERT_STATUS_SUCCESS(SMBAllocateString("v5", &v5));
    TEST_ASSERT_STATUS_SUCCESS(SMBAllocateString("v6", &v6));

    TEST_ASSERT_STATUS_SUCCESS(LwioLruSetValue(pLru, k1, v1));
    TEST_ASSERT_STATUS_SUCCESS(LwioLruSetValue(pLru, k2, v2));
    TEST_ASSERT_STATUS_SUCCESS(LwioLruSetValue(pLru, k3, v3));
    TEST_ASSERT_STATUS_SUCCESS(LwioLruSetValue(pLru, k4, v4));
    TEST_ASSERT_STATUS_SUCCESS(LwioLruSetValue(pLru, k5, v5));

    TEST_ASSERT(LwioLruHasValue(pLru, k1));
    TEST_ASSERT(LwioLruHasValue(pLru, k3));
    TEST_ASSERT(LwioLruHasValue(pLru, k5));

    TEST_ASSERT_STATUS_SUCCESS(LwioLruIteratorAllocate(pLru, &it));
    while ((pEntry = LwioLruNext(it)))
    {
        TEST_ASSERT(!strcmp(answerKey[i], (PSTR)pEntry->pKey));
        TEST_ASSERT(!strcmp(answerValue[i], (PSTR)pEntry->pValue));
        ++i;
    }
    LwioLruIteratorSafeFree(&it);

    TEST_ASSERT(LwioLruSize(pLru) == 5);
    TEST_ASSERT(LwioLruIsEmpty(pLru) == FALSE);

    TEST_ASSERT_STATUS_SUCCESS(LwioLruSetValue(pLru, k6, v6));

    TEST_ASSERT(LwioLruSize(pLru) == 5);

    ntStatus = LwioLruGetValue(pLru, "k2", (PVOID*)&pValue);
    TEST_ASSERT(ntStatus == STATUS_NOT_FOUND);
    ntStatus = STATUS_SUCCESS;

    TEST_ASSERT_STATUS_SUCCESS(LwioLruGetValue(pLru, k6, (PVOID*)&pValue));
    TEST_ASSERT(!strcmp("v6", pValue));

    TEST_ASSERT_STATUS_SUCCESS(LwioLruRemove(pLru, k6));
    TEST_ASSERT(LwioLruSize(pLru) == 4);

error:

    return ntStatus;
}

static
NTSTATUS
TestExists(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
TestIterator(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
TestErrors(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

int
main(
    int argc,
    char* argv[]
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = ParseArgs(argc, argv);
    BAIL_ON_NT_STATUS(ntStatus);

    TEST_LRU_RUN(TestInsertRemove);
    TEST_LRU_RUN(TestExists);
    TEST_LRU_RUN(TestIterator);
    TEST_LRU_RUN(TestErrors);

cleanup:

    return ntStatus;

error:

    printf("ERROR: %d\n", ntStatus);

    goto cleanup;
}
