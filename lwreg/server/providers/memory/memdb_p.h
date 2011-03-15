#include "includes.h"

struct _REG_DB_CONNECTION;
typedef struct _REG_DB_CONNECTION *REG_DB_HANDLE;
typedef REG_DB_HANDLE *PREG_DB_HANDLE;


typedef struct _REGMEM_NODE *MEM_REG_STORE_HANDLE;
typedef struct _REGMEM_NODE **PMEM_REG_STORE_HANDLE;


typedef struct _REG_DB_CONNECTION
{
    MEM_REG_STORE_HANDLE pMemReg;
    pthread_rwlock_t lock;
} REG_DB_CONNECTION, *PREG_DB_CONNECTION;


#if 0
typedef struct __REG_KEY_HANDLE
{
        ACCESS_MASK AccessGranted;
        PREG_KEY_CONTEXT pKey;
} REG_KEY_HANDLE, *PREG_KEY_HANDLE;

#endif


/*
 * Definition of structure found in context server/include/regserver.h as
 * an incomplete type: REG_KEY_HANDLE, *PREG_KEY_HANDLE;
 * struct __REG_KEY_HANDLE
 */
typedef struct __REG_KEY_CONTEXT
{
    MEM_REG_STORE_HANDLE hKey;
} REG_KEY_CONTEXT;


NTSTATUS
MemDbOpen(
    OUT PREG_DB_HANDLE phDb
    );

NTSTATUS
MemDbClose(
    IN PREG_DB_HANDLE phDb
    );


NTSTATUS
MemDbOpenKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszFullKeyPath,
    OUT OPTIONAL MEM_REG_STORE_HANDLE *pRegKey
    );


NTSTATUS
MemDbCreateKeyEx(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pcwszSubKey,
    IN DWORD dwReserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescLength,
    OUT PMEM_REG_STORE_HANDLE phSubKey,
    OUT OPTIONAL PDWORD pdwDisposition
    );

