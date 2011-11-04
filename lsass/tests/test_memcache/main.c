#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwsecurityidentifier.h"
#include "lsautils.h"

typedef struct __USERINFO
{
    LSA_CACHE_ENTRY entry;
    DWORD uid;
    DWORD gid;
    PSTR pszUpn;
    PSTR pszSid;
} USERINFO, *PUSERINFO;

typedef enum
{
    USERINFO_UID,
    USERINFO_GID,
    USERINFO_UPN,
    USERINFO_SID,
} USERINFO_KEY;

static PUSERINFO
userinfo_create(DWORD uid)
{
    PUSERINFO pUser = calloc(1, sizeof(*pUser));

    pUser->uid = uid;
    pUser->gid = ~uid;
    
    LwAllocateStringPrintf(&pUser->pszUpn, "user%.4lu@domain.com", (unsigned long) uid);
    LwAllocateStringPrintf(&pUser->pszSid, "S-NOT-A-REAL-SID-%.4lu", (unsigned long) uid);
    
    return pUser;
}

static DWORD
generic_hash(unsigned char* pKey, DWORD dwSize)
{
    DWORD dwIndex;
    DWORD dwHash = 0;

    for (dwIndex = 0; dwIndex < dwSize; dwIndex++)
    {
        dwHash += pKey[dwIndex];
        dwHash += (dwHash << 10);
        dwHash ^= (dwHash >> 6);
    }

    dwHash += (dwHash << 3);
    dwHash ^= (dwHash >> 11);
    dwHash += (dwHash << 15);

    return dwHash;
}

static PVOID
userinfo_getkey(void* pEntry, DWORD dwIndex, void* data)
{
    PUSERINFO pUser = (PUSERINFO) pEntry;

    switch ((USERINFO_KEY) dwIndex)
    {
    case USERINFO_UID:
        return &pUser->uid;
    case USERINFO_GID:
        return &pUser->gid;
    case USERINFO_SID:
        return pUser->pszSid;
    case USERINFO_UPN:
        return pUser->pszUpn;
    }

    return NULL;
}

static DWORD
userinfo_hash(void* pKey, DWORD dwIndex, void* data)
{
    switch ((USERINFO_KEY) dwIndex)
    {
    case USERINFO_UID:
    case USERINFO_GID:
        return *(DWORD*) pKey;
    case USERINFO_SID:
    case USERINFO_UPN:
        return generic_hash(pKey, strlen((const char*) pKey));
    }

    return 0;
}

static BOOLEAN
userinfo_equal(void* pKey1, void* pKey2, DWORD dwIndex, void* data)
{
    switch ((USERINFO_KEY) dwIndex)
    {
    case USERINFO_UID:
    case USERINFO_GID:
        return *(DWORD*) pKey1 == *(DWORD*) pKey2;
    case USERINFO_SID:
    case USERINFO_UPN:
        return !strcmp((const char *) pKey1, (const char*) pKey2);
    }

    return FALSE;
}

static DWORD
userinfo_miss(void* pKey, DWORD dwIndex, void* data, PVOID* ppEntry)
{
    DWORD uid = 0;

    switch ((USERINFO_KEY) dwIndex)
    {
    case USERINFO_UID:
        uid = *(DWORD*) pKey;
        break;
    case USERINFO_GID:
        uid = ~*(DWORD*) pKey;
        break;
    case USERINFO_SID:
        uid = atoi(((char*) pKey) + strlen("S-NOT-A-REAL-SID-"));
        break;
    case USERINFO_UPN:
        uid = atoi(((char*) pKey) + strlen("user"));
        break;        
    }

    *ppEntry = userinfo_create(uid);

    return 0;
}

static DWORD
userinfo_kick(PVOID pEntry, void* data)
{
    free(pEntry);

    return 0;
}

#define BUCKETS 1333
#define USERS 1000

int main(int argc, char** argv)
{
    PLSA_CACHE pCache = NULL;
    DWORD i;
    PUSERINFO pUser1 = NULL;
    PUSERINFO pUser2 = NULL;
    char pszUserName[256];

    LsaCacheNew(
        4,
        BUCKETS,
        userinfo_hash,
        userinfo_equal,
        userinfo_getkey,
        userinfo_miss,
        userinfo_kick,
        NULL,
        &pCache
        );

    for (i = 1; i <= USERS; i++)
    {
        sprintf(pszUserName, "user%.4lu@domain.com", (unsigned long) i);
        LsaCacheLookup(pCache, pszUserName, USERINFO_UPN, (PVOID*) (PVOID) &pUser1);
        LsaCacheLookup(pCache, &i, USERINFO_UID, (PVOID*) (PVOID) &pUser2);

        if (pUser1 != pUser2 || pUser1->uid != i || strcmp(pUser1->pszUpn, pszUserName))
        {
            abort();
        }
    }

    for (i = 1; i <= USERS; i++)
    {
        DWORD gid = ~i;
        LsaCacheLookup(pCache, &gid, USERINFO_GID, (PVOID*) (PVOID) &pUser1);

        if (pUser1->gid != gid)
        {
            abort();
        }
    }

    printf("Cache collisions: %lu (%f per key)\n", (unsigned long) pCache->dwNumCollisions, (double) pCache->dwNumCollisions / 4.0);
    printf("Cache hash misses: %lu\n", (unsigned long) pCache->dwNumHashMisses - USERS);
    printf("Cache full misses: %lu\n", (unsigned long) pCache->dwNumFullMisses - USERS);
    printf("Cache kicks: %lu\n", (unsigned long) pCache->dwNumKicks);
    printf("Cache load: %f\n", (double) pCache->dwNumUsedBuckets / (4.0 * BUCKETS));

    return 0;
}
