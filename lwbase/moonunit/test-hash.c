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
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 */

#include <moonunit/moonunit.h>
#include <lw/base.h>
#include <lw/rtlgoto.h>

#define MU_ASSERT_STATUS_SUCCESS(status) \
    MU_ASSERT(STATUS_SUCCESS == (status))

#define NUM_STRINGS 1024
#define TABLE_SIZE 17

typedef struct _STR_NODE
{
    PSTR pszString;
    BOOLEAN bInserted;
    LW_HASHTABLE_NODE Node;
} STR_NODE, *PSTR_NODE;

static
PCVOID
StrNodeGetKey(
    PLW_HASHTABLE_NODE pNode,
    PVOID pUnused
    )
{
    PSTR_NODE pStrNode = LW_STRUCT_FROM_FIELD(pNode, STR_NODE, Node);

    return pStrNode->pszString;
}

static
VOID
StrNodeFree(
    PLW_HASHTABLE_NODE pNode,
    PVOID pUnused
    )
{
    PSTR_NODE pStrNode = LW_STRUCT_FROM_FIELD(pNode, STR_NODE, Node);

    RTL_FREE(&pStrNode->pszString);
    pStrNode->bInserted = FALSE;
}

static
VOID
StrPairFree(
    PLW_HASHMAP_PAIR pPair,
    PVOID pUnused
    )
{
    RTL_FREE(&pPair->pKey);
    *(PBOOLEAN) pPair->pValue = FALSE;
}

MU_TEST(Hash, ExerciseTable)
{
    PLW_HASHTABLE pTable = NULL;
    STR_NODE strings[NUM_STRINGS];
    ULONG ulIndex = 0;
    PLW_HASHTABLE_NODE pNode = NULL;
    LW_HASHTABLE_ITER iter = LW_HASHTABLE_ITER_INIT;

    MU_ASSERT_STATUS_SUCCESS(
        LwRtlCreateHashTable(
            &pTable,
            StrNodeGetKey,
            LwRtlHashDigestPstr,
            LwRtlHashEqualPstr,
            NULL,
            TABLE_SIZE));

    /* Allocate and insert unique strings */
    for (ulIndex = 0; ulIndex < NUM_STRINGS; ulIndex++)
    {
        MU_ASSERT_STATUS_SUCCESS(
            LwRtlCStringAllocatePrintf(
                &strings[ulIndex].pszString,
                "%lu",
                (unsigned long) ulIndex));

        LwRtlHashTableResizeAndInsert(pTable, &strings[ulIndex].Node, &pNode);
        MU_ASSERT_EQUAL(MU_TYPE_POINTER, pNode, NULL);

        strings[ulIndex].bInserted = TRUE;
    }

    /* Node count should be equal to NUM_STRINGS */
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, LwRtlHashTableGetCount(pTable), NUM_STRINGS);
    /* Table should have been transparently resized */
    MU_ASSERT(LwRtlHashTableGetSize(pTable) > TABLE_SIZE);

    /* Remove even entries */
    for (ulIndex = 0; ulIndex < NUM_STRINGS; ulIndex += 2)
    {
        MU_ASSERT_STATUS_SUCCESS(LwRtlHashTableRemove(pTable, &strings[ulIndex].Node));
        /* Assert that the node is no longer found */
        MU_ASSERT_EQUAL(
            MU_TYPE_INTEGER,
            LwRtlHashTableFindKey(pTable, &pNode, strings[ulIndex].pszString),
            STATUS_NOT_FOUND);

        strings[ulIndex].bInserted = FALSE;
    }

    /* Node count should be half of NUM_STRINGS */
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, LwRtlHashTableGetCount(pTable), NUM_STRINGS / 2);

    /* Attempt to look up each remaining entry */
    for (ulIndex = 1; ulIndex < NUM_STRINGS; ulIndex += 2)
    {
        MU_ASSERT_STATUS_SUCCESS(LwRtlHashTableFindKey(pTable, &pNode, strings[ulIndex].pszString));
        MU_ASSERT_EQUAL(MU_TYPE_POINTER, pNode, &strings[ulIndex].Node);
        MU_ASSERT(((PSTR_NODE) LW_STRUCT_FROM_FIELD(pNode, STR_NODE, Node))->bInserted);
    }

    /* Iterate over table and remove remaining entries */
    while ((pNode = LwRtlHashTableIterate(pTable, &iter)))
    {
        MU_ASSERT_STATUS_SUCCESS(LwRtlHashTableRemove(pTable, pNode));
        MU_ASSERT(((PSTR_NODE) LW_STRUCT_FROM_FIELD(pNode, STR_NODE, Node))->bInserted);
        ((PSTR_NODE) LW_STRUCT_FROM_FIELD(pNode, STR_NODE, Node))->bInserted = FALSE;
    }

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, LwRtlHashTableGetCount(pTable), 0);

    /* Ensure everything was marked as removed */
    for (ulIndex = 0; ulIndex < NUM_STRINGS; ulIndex++)
    {
        MU_ASSERT(!strings[ulIndex].bInserted);
    }

    /* Re-insert everything */
    for (ulIndex = 0; ulIndex < NUM_STRINGS; ulIndex++)
    {
        LwRtlHashTableResizeAndInsert(pTable, &strings[ulIndex].Node, &pNode);
        MU_ASSERT_EQUAL(MU_TYPE_POINTER, pNode, NULL);

        strings[ulIndex].bInserted = TRUE;
    }

    /* Node count should be equal to NUM_STRINGS */
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, LwRtlHashTableGetCount(pTable), NUM_STRINGS);

    /* Test clear function */
    LwRtlHashTableClear(pTable, StrNodeFree, NULL);

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, LwRtlHashTableGetCount(pTable), 0);

    /* Ensure everything was marked as removed */
    for (ulIndex = 0; ulIndex < NUM_STRINGS; ulIndex++)
    {
        MU_ASSERT(!strings[ulIndex].pszString);
        MU_ASSERT(!strings[ulIndex].bInserted);
    }

    /* Free table */
    LwRtlFreeHashTable(&pTable);

    MU_ASSERT_EQUAL(MU_TYPE_POINTER, pTable, NULL);
}

MU_TEST(Hash, ExerciseMap)
{
    PLW_HASHMAP pMap = NULL;
    STR_NODE strings[NUM_STRINGS];
    ULONG ulIndex = 0;
    LW_HASHMAP_PAIR pair = {NULL, NULL};
    LW_HASHMAP_PAIR pair2 = {NULL, NULL};
    LW_HASHMAP_ITER iter = LW_HASHMAP_ITER_INIT;
    PBOOLEAN pbInserted = NULL;

    MU_ASSERT_STATUS_SUCCESS(
        LwRtlCreateHashMap(
            &pMap,
            LwRtlHashDigestPstr,
            LwRtlHashEqualPstr,
            NULL));

    /* Allocate and insert unique strings */
    for (ulIndex = 0; ulIndex < NUM_STRINGS; ulIndex++)
    {
        MU_ASSERT_STATUS_SUCCESS(
            LwRtlCStringAllocatePrintf(
                &strings[ulIndex].pszString,
                "%lu",
                (unsigned long) ulIndex));

        MU_ASSERT_STATUS_SUCCESS(
            LwRtlHashMapInsert(
                pMap,
                strings[ulIndex].pszString,
                &strings[ulIndex].bInserted,
                &pair));
        MU_ASSERT_EQUAL(MU_TYPE_POINTER, pair.pKey, NULL);
        MU_ASSERT_EQUAL(MU_TYPE_POINTER, pair.pValue, NULL);

        strings[ulIndex].bInserted = TRUE;
    }

    /* Node count should be equal to NUM_STRINGS */
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, LwRtlHashMapGetCount(pMap), NUM_STRINGS);

    /* Remove even entries */
    for (ulIndex = 0; ulIndex < NUM_STRINGS; ulIndex += 2)
    {
        MU_ASSERT_STATUS_SUCCESS(LwRtlHashMapRemove(pMap, strings[ulIndex].pszString, &pair));
        MU_ASSERT_EQUAL(MU_TYPE_POINTER, pair.pKey, strings[ulIndex].pszString);
        MU_ASSERT_EQUAL(MU_TYPE_POINTER, pair.pValue, &strings[ulIndex].bInserted);
        /* Assert that the node is no longer found */
        MU_ASSERT_EQUAL(
            MU_TYPE_INTEGER,
            LwRtlHashMapFindKey(pMap, OUT_PPVOID(&pbInserted), strings[ulIndex].pszString),
            STATUS_NOT_FOUND);
        strings[ulIndex].bInserted = FALSE;
    }

    /* Node count should be half of NUM_STRINGS */
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, LwRtlHashMapGetCount(pMap), NUM_STRINGS / 2);

    /* Attempt to look up each remaining entry */
    for (ulIndex = 1; ulIndex < NUM_STRINGS; ulIndex += 2)
    {
        MU_ASSERT_STATUS_SUCCESS(LwRtlHashMapFindKey(pMap, OUT_PPVOID(&pbInserted), strings[ulIndex].pszString));
        MU_ASSERT_EQUAL(MU_TYPE_POINTER, pbInserted, &strings[ulIndex].bInserted);
        MU_ASSERT(*pbInserted);
    }

    /* Iterate over table and remove remaining entries */
    while (LwRtlHashMapIterate(pMap, &iter, &pair))
    {
        MU_ASSERT_STATUS_SUCCESS(LwRtlHashMapRemove(pMap, pair.pKey, &pair2));
        MU_ASSERT_EQUAL(MU_TYPE_POINTER, pair.pKey, pair2.pKey);
        MU_ASSERT_EQUAL(MU_TYPE_POINTER, pair.pValue, pair2.pValue);
        pbInserted = (PBOOLEAN) pair.pValue;
        MU_ASSERT(*pbInserted);
        *pbInserted = FALSE;
    }

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, LwRtlHashMapGetCount(pMap), 0);

    /* Ensure everything was marked as removed */
    for (ulIndex = 0; ulIndex < NUM_STRINGS; ulIndex++)
    {
        MU_ASSERT(!strings[ulIndex].bInserted);
    }

    /* Re-insert everything */
    for (ulIndex = 0; ulIndex < NUM_STRINGS; ulIndex++)
    {
        MU_ASSERT_STATUS_SUCCESS(
            LwRtlHashMapInsert(
                pMap,
                strings[ulIndex].pszString,
                &strings[ulIndex].bInserted,
                &pair));
        MU_ASSERT_EQUAL(MU_TYPE_POINTER, pair.pKey, NULL);
        MU_ASSERT_EQUAL(MU_TYPE_POINTER, pair.pValue, NULL);

        strings[ulIndex].bInserted = TRUE;
    }

    /* Node count should be equal to NUM_STRINGS */
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, LwRtlHashMapGetCount(pMap), NUM_STRINGS);

    /* Test clear function */
    LwRtlHashMapClear(pMap, StrPairFree, NULL);

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, LwRtlHashMapGetCount(pMap), 0);

    /* Ensure everything was removed */
    for (ulIndex = 0; ulIndex < NUM_STRINGS; ulIndex++)
    {
        MU_ASSERT(!strings[ulIndex].bInserted);
    }

    LwRtlFreeHashMap(&pMap);

    MU_ASSERT_EQUAL(MU_TYPE_POINTER, pMap, NULL);
}
