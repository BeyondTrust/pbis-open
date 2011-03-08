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

NTSTATUS
MemDbOpen(
    OUT PREG_DB_HANDLE phDb
    );
