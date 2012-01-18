#include "includes.h"

#define MEMDB_MAX_EXPORT_TIMEOUT (60*10) // 10 Minutes
#define MEMDB_CHANGED_EXPORT_TIMEOUT 5 // 5 seconds
#define MEMDB_FOREVER_EXPORT_TIMEOUT (30 * 24 * 3600) // 1 month
#define MEMDB_EXPORT_DIR "/var/lib/pbis/db"
#define MEMDB_EXPORT_FILE MEMDB_EXPORT_DIR "/memprovider.exp"

typedef struct _MEMREG_NODE *PMEMREG_NODE;

typedef struct _MEMDB_FILE_EXPORT_CTX
{
    int wfd;
    PMEMREG_NODE hNode;
    BOOLEAN bStopThread;
} MEMDB_FILE_EXPORT_CTX, *PMEMDB_FILE_EXPORT_CTX;


typedef struct _REG_DB_CONNECTION
{
    PMEMREG_NODE pMemReg;
    pthread_t hThread;
    pthread_rwlock_t lock;
    pthread_mutex_t ExportMutex;
    pthread_mutex_t ExportMutexStop;
    pthread_cond_t ExportCond;
    pthread_cond_t ExportCondStop;
    DWORD valueChangeCount;
    PMEMDB_FILE_EXPORT_CTX ExportCtx;
} REG_DB_CONNECTION, *PREG_DB_CONNECTION;

typedef struct _MEMDB_IMPORT_FILE_CTX
{
    PMEMREG_NODE hRootKey;
    PMEMREG_NODE hSubKey;
    HANDLE parseHandle;
    PSTR fileName;
} MEMDB_IMPORT_FILE_CTX, *PMEMDB_IMPORT_FILE_CTX;


/*
 * Definition of structure found in context server/include/regserver.h as
 * an incomplete type: REG_KEY_HANDLE, *PREG_KEY_HANDLE;
 * struct __REG_KEY_HANDLE
 */
typedef struct __REG_KEY_CONTEXT
{
    PMEMREG_NODE hNode;
    ACCESS_MASK AccessGranted;
    PREG_DB_CONNECTION pConn;
} REG_KEY_CONTEXT;


typedef struct _MEMDB_STACK_ENTRY
{
    PMEMREG_NODE pNode;
    PWSTR pwszSubKeyPrefix;
} MEMDB_STACK_ENTRY, *PMEMDB_STACK_ENTRY;


typedef struct _MEMDB_STACK
{
    PMEMDB_STACK_ENTRY stack;
    DWORD stackSize;
    DWORD stackPtr;
    DWORD stackSizeMax;
} MEMDB_STACK, *PMEMDB_STACK;

typedef enum _MEMDB_EXPORT_STATE
{
   MEMDB_EXPORT_START = 1,
   MEMDB_EXPORT_CHECK_CHANGES,
   MEMDB_EXPORT_INIT_TO_INFINITE,
   MEMDB_EXPORT_INIT_TO_SHORT,
   MEMDB_EXPORT_WAIT,
   MEMDB_EXPORT_TEST_CHANGE,
   MEMDB_EXPORT_TEST_MAX_TIMEOUT,
   MEMDB_EXPORT_UPDATE_SHORT_TIMEOUT,
   MEMDB_EXPORT_WRITE_CHANGES,
} MEMDB_EXPORT_STATE;

NTSTATUS
MemDbOpen(
    OUT PMEMREG_NODE *ppDbRoot
    );

NTSTATUS
MemDbClose(
    IN PREG_DB_CONNECTION hDb
    );


NTSTATUS
MemDbOpenKey(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
    IN PCWSTR pwszFullKeyPath,
    IN ACCESS_MASK AccessDesired,
    OUT OPTIONAL PMEMREG_NODE *pRegKey
    );

VOID
MemDbCloseKey(
    IN HKEY hKey
    );

NTSTATUS
MemDbCreateKeyEx(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
    IN PCWSTR pcwszSubKey,
    IN DWORD dwReserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescLength,
    OUT PMEMREG_NODE *pphSubKey,
    OUT OPTIONAL PDWORD pdwDisposition
    );

NTSTATUS
MemDbQueryInfoKey(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
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
    IN PREG_DB_CONNECTION hDb,
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
    IN PREG_DB_CONNECTION hDb,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD dwReserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    );


NTSTATUS
MemDbGetValue(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
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
    IN PREG_DB_CONNECTION hDb,
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
    IN PREG_DB_CONNECTION hDb,
    OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    OUT ULONG secDescLen
    );


NTSTATUS
MemDbGetKeyAcl(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
    OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    OUT PULONG pSecDescLen
    );


NTSTATUS
MemDbSetValueAttributes(
    IN HANDLE hRegConnection,
    IN PREG_DB_CONNECTION hDb,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    );


NTSTATUS
MemDbGetValueAttributes(
    IN HANDLE hRegConnection,
    IN PREG_DB_CONNECTION hDb,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    );


NTSTATUS
MemDbRecurseRegistry(
    IN HANDLE hRegConnection,
    IN PREG_DB_CONNECTION hDb,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PVOID (*pfCallback)(PMEMREG_NODE hNode, 
                           PVOID userContext,
                           PWSTR subKeyPrefix,
                           NTSTATUS *pstatus),
    IN PVOID userContext
    );


NTSTATUS
MemDbRecurseDepthFirstRegistry(
    IN HANDLE hRegConnection,
    IN PREG_DB_CONNECTION hDb,
    IN OPTIONAL PCWSTR pwszOptSubKey,
    IN PVOID (*pfCallback)(PMEMREG_NODE hNode, 
                           PVOID userContext,
                           PWSTR pwszSubKeyPrefix,
                           NTSTATUS *pStatus),
    IN PVOID userContext
    );

NTSTATUS
MemDbStartExportToFileThread(
    VOID);


NTSTATUS
MemDbImportFromFile(
    IN PSTR pszImportFile,
    IN PFN_REG_CALLBACK parseCallback,
    PMEMDB_IMPORT_FILE_CTX userContext
    );


NTSTATUS
MemDbExportToFile(
    PMEMDB_FILE_EXPORT_CTX pExportCtx
    );


VOID
MemDbExportEntryChanged(
    VOID
    );


DWORD 
pfImportFile(
    PREG_PARSE_ITEM pItem,
    HANDLE userContext 
    );

PWSTR
pwstr_wcschr(
    PWSTR pwszHaystack, WCHAR wcNeedle
    );

