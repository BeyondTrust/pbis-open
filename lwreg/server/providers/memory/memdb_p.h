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

typedef struct _MEMDB_FILE_EXPORT_CTX
{
    FILE *wfp;
    MEM_REG_STORE_HANDLE hKey;
    BOOLEAN bStopThread;
} MEMDB_FILE_EXPORT_CTX, *PMEMDB_FILE_EXPORT_CTX;

typedef struct _MEMDB_IMPORT_FILE_CTX
{
    MEM_REG_STORE_HANDLE hRootKey;
    MEM_REG_STORE_HANDLE hSubKey;
} MEMDB_IMPORT_FILE_CTX;

#define MEMDB_EXPORT_FILE "/var/lib/likewise/db/memprovider.exp"

/*
 * Definition of structure found in context server/include/regserver.h as
 * an incomplete type: REG_KEY_HANDLE, *PREG_KEY_HANDLE;
 * struct __REG_KEY_HANDLE
 */
typedef struct __REG_KEY_CONTEXT
{
    MEM_REG_STORE_HANDLE hKey;
    ACCESS_MASK AccessGranted;
    REG_DB_CONNECTION pConn;
} REG_KEY_CONTEXT;


typedef struct _MEMDB_STACK_ENTRY
{
    PREGMEM_NODE pNode;
    PWSTR pwszSubKeyPrefix;
} MEMDB_STACK_ENTRY, *PMEMDB_STACK_ENTRY;


typedef struct _MEMDB_STACK
{
    PMEMDB_STACK_ENTRY stack;
    DWORD stackSize;
    DWORD stackPtr;
} MEMDB_STACK, *PMEMDB_STACK;


NTSTATUS
MemDbOpen(
    OUT PREG_DB_HANDLE phDb
    );

NTSTATUS
MemDbClose(
    IN REG_DB_HANDLE hDb
    );


NTSTATUS
MemDbOpenKey(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszFullKeyPath,
    OUT OPTIONAL MEM_REG_STORE_HANDLE *pRegKey
    );


NTSTATUS
MemDbCreateKeyEx(
    IN HANDLE Handle,
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

NTSTATUS
MemDbQueryInfoKey(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    /*
     * A pointer to a buffer that receives the user-defined class of the key. 
     * This parameter can be NULL.
     */
    OUT OPTIONAL PWSTR pClass, 
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pdwReserved, /* This parameter is reserved and must be NULL. */
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen, /* implement this later */
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PDWORD pcbSecurityDescriptor,
    OUT OPTIONAL PFILETIME pftLastWriteTime /* implement this later */
    );


NTSTATUS
MemDbEnumKeyEx(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    IN DWORD dwIndex,
    /*buffer to hold keyName*/
    OUT PWSTR pName, 

    /* 
     * When the function returns, the variable receives the number 
     * of characters stored in the buffer,not including the terminating null 
     * character.
     */
    IN OUT PDWORD pcName,

    IN PDWORD pdwReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT PFILETIME pftLastWriteTime
    );


NTSTATUS
MemDbSetValueEx(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD dwReserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    );


NTSTATUS
MemDbGetValue(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );


NTSTATUS
MemDbEnumValue(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    IN DWORD dwIndex,
    OUT PWSTR pValueName, /*buffer hold valueName*/
    IN OUT PDWORD pcchValueName, /*input - buffer pValueName length*/
    IN PDWORD pdwReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,/*buffer hold value content*/
    IN OUT OPTIONAL PDWORD pcbData /*input - buffer pData length*/
    );


NTSTATUS
MemDbSetKeyAcl(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    OUT ULONG secDescLen
    );


NTSTATUS
MemDbGetKeyAcl(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    OUT PULONG pSecDescLen
    );


NTSTATUS
MemDbSetValueAttributes(
    IN HANDLE hRegConnection,
    IN REG_DB_HANDLE hDb,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    );


NTSTATUS
MemDbGetValueAttributes(
    IN HANDLE hRegConnection,
    IN REG_DB_HANDLE hDb,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    );


NTSTATUS
MemDbRecurseRegistry(
    IN HANDLE hRegConnection,
    IN REG_DB_HANDLE hDb,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PVOID (*pfCallback)(MEM_REG_STORE_HANDLE hKey, 
                           PVOID userContext,
                           PWSTR subKeyPrefix),
    IN PVOID userContext
    );


NTSTATUS
MemDbRecurseDepthFirstRegistry(
    IN HANDLE hRegConnection,
    IN REG_DB_HANDLE hDb,
    IN OPTIONAL PCWSTR pwszOptSubKey,
    IN PVOID (*pfCallback)(MEM_REG_STORE_HANDLE hKey, 
                           PVOID userContext,
                           PWSTR pwszSubKeyPrefix),
    IN PVOID userContext
    );

VOID MemDbStartExportToFileThread(VOID);


NTSTATUS
MemDbImportFromFile(
    IN PSTR pszImportFile,
    IN PFN_REG_CALLBACK parseCallback,
    IN HANDLE userContext
    );


DWORD pfImportFile(PREG_PARSE_ITEM pItem, HANDLE userContext);
