#include "config.h"
#include "lwioutils.h"
#include "lwiolru.h"
#include "lwiosys.h"
#include <moonunit/moonunit.h>
#include <lw/base.h>

#define MU_ASSERT_STATUS(expr, status) MU_ASSERT_EQUAL(MU_TYPE_INTEGER, (expr), (status))

#define MU_ASSERT_SUCCESS(expr) MU_ASSERT_STATUS((expr), STATUS_SUCCESS)

MU_TEST(BitVector, Test)
{
    PLWIO_BIT_VECTOR pBitVector = NULL;
    DWORD dwNumBits = 100;
    DWORD dwTestBit = 51;
    DWORD iBit = 0;
    DWORD dwUnsetBit = 0;

    MU_ASSERT_SUCCESS(LwioBitVectorCreate(dwNumBits, &pBitVector));

    MU_ASSERT_SUCCESS(LwioBitVectorSetBit(pBitVector, 0));
    MU_ASSERT_STATUS(LwioBitVectorSetBit(pBitVector, dwNumBits), STATUS_INVALID_PARAMETER);
    MU_ASSERT_SUCCESS(LwioBitVectorSetBit(pBitVector, dwTestBit));
    MU_ASSERT(LwioBitVectorIsSet(pBitVector, dwTestBit));
    MU_ASSERT_SUCCESS(LwioBitVectorUnsetBit(pBitVector, dwTestBit));
    MU_ASSERT_SUCCESS(LwioBitVectorUnsetBit(pBitVector, 0));
    MU_ASSERT(!LwioBitVectorIsSet(pBitVector, dwTestBit));
    MU_ASSERT_SUCCESS(LwioBitVectorFirstUnsetBit(pBitVector, &dwUnsetBit));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, dwUnsetBit, 0);

    for (iBit = 0; iBit < dwNumBits; iBit++)
    {
        if (iBit != dwTestBit)
        {
            MU_ASSERT_SUCCESS(LwioBitVectorSetBit(pBitVector, iBit));
        }
    }

    MU_ASSERT_SUCCESS(LwioBitVectorFirstUnsetBit(pBitVector, &dwUnsetBit));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, dwUnsetBit, dwTestBit);
    MU_ASSERT_SUCCESS(LwioBitVectorSetBit(pBitVector, dwTestBit));
    MU_ASSERT_STATUS(LwioBitVectorFirstUnsetBit(pBitVector, &dwUnsetBit), STATUS_NOT_FOUND);
    
    LwioBitVectorFree(pBitVector);
}

#define LRU_SIZE                    5
#define LRU_HASH_SIZE               10
#define LRU_PATH_HASH_MULTIPLIER    31

PLWIO_LRU pLru = NULL;

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

MU_FIXTURE_SETUP(Lru)
{
    MU_ASSERT_SUCCESS(
        LwioLruCreate(LRU_SIZE, 
                      LRU_HASH_SIZE, 
                      LruComparator, 
                      LruHash, 
                      LruFree, 
                      &pLru));
}

MU_FIXTURE_TEARDOWN(Lru)
{
    LwioLruSafeFree(&pLru);
}

MU_TEST(Lru, InsertRemove)
{
    PSTR k1, k2, k3, k4, k5, k6;
    PSTR v1, v2, v3, v4, v5, v6;
    PLWIO_LRU_ITERATOR it = NULL;
    PLWIO_LRU_ENTRY pEntry = NULL;
    char* answerKey[] = {"k5", "k3", "k1", "k4", "k2"};
    char* answerValue[] = {"v5", "v3", "v1", "v4", "v2"};
    ULONG i = 0;
    PSTR pValue = NULL;

    MU_ASSERT_SUCCESS(SMBAllocateString("k1", &k1));
    MU_ASSERT_SUCCESS(SMBAllocateString("k2", &k2));
    MU_ASSERT_SUCCESS(SMBAllocateString("k3", &k3));
    MU_ASSERT_SUCCESS(SMBAllocateString("k4", &k4));
    MU_ASSERT_SUCCESS(SMBAllocateString("k5", &k5));
    MU_ASSERT_SUCCESS(SMBAllocateString("k6", &k6));

    MU_ASSERT_SUCCESS(SMBAllocateString("v1", &v1));
    MU_ASSERT_SUCCESS(SMBAllocateString("v2", &v2));
    MU_ASSERT_SUCCESS(SMBAllocateString("v3", &v3));
    MU_ASSERT_SUCCESS(SMBAllocateString("v4", &v4));
    MU_ASSERT_SUCCESS(SMBAllocateString("v5", &v5));
    MU_ASSERT_SUCCESS(SMBAllocateString("v6", &v6));

    MU_ASSERT_SUCCESS(LwioLruSetValue(pLru, k1, v1));
    MU_ASSERT_SUCCESS(LwioLruSetValue(pLru, k2, v2));
    MU_ASSERT_SUCCESS(LwioLruSetValue(pLru, k3, v3));
    MU_ASSERT_SUCCESS(LwioLruSetValue(pLru, k4, v4));
    MU_ASSERT_SUCCESS(LwioLruSetValue(pLru, k5, v5));

    MU_ASSERT(LwioLruHasValue(pLru, k1));
    MU_ASSERT(LwioLruHasValue(pLru, k3));
    MU_ASSERT(LwioLruHasValue(pLru, k5));

    MU_ASSERT_SUCCESS(LwioLruIteratorAllocate(pLru, &it));
    while ((pEntry = LwioLruNext(it)))
    {
        MU_ASSERT(!strcmp(answerKey[i], (PSTR)pEntry->pKey));
        MU_ASSERT(!strcmp(answerValue[i], (PSTR)pEntry->pValue));
        ++i;
    }
    LwioLruIteratorSafeFree(&it);

    MU_ASSERT(LwioLruSize(pLru) == 5);
    MU_ASSERT(LwioLruIsEmpty(pLru) == FALSE);

    MU_ASSERT_SUCCESS(LwioLruSetValue(pLru, k6, v6));

    MU_ASSERT(LwioLruSize(pLru) == 5);

    MU_ASSERT_STATUS(LwioLruGetValue(pLru, "k2", (PVOID*)&pValue), STATUS_NOT_FOUND);

    MU_ASSERT_SUCCESS(LwioLruGetValue(pLru, k6, (PVOID*)&pValue));
    MU_ASSERT(!strcmp("v6", pValue));

    MU_ASSERT_SUCCESS(LwioLruRemove(pLru, k6));
    MU_ASSERT(LwioLruSize(pLru) == 4);
}
