/* This structure captures the arguments that must be
 * sent to the Group Policy Service
 */
typedef struct {
    /* MT safety */
    CRITICAL_SECTION lock;
    /* Should start as daemon */
    DWORD dwStartAsDaemon;
    /* config file path */
    WCHAR szConfigFilePath[PATH_MAX + 1];
    /* Cache path */
    WCHAR szCachePath[PATH_MAX+1];
    /* Prefix path */
    WCHAR szPrefixPath[PATH_MAX+1];
    /* Process termination flag */
    BOOLEAN  bProcessShouldExit;
    /* Process Exit Code */
    DWORD dwExitCode;
    /* Flag to overrite any existing db   */
    BOOLEAN bReplaceDB;
    /* Maximum number of log file size in KB that is supported */
    DWORD dwMaxLogSize;
    /* Maximum number of records that can be hold*/
    DWORD dwMaxRecords;
    /* Remove the events older than*/
    DWORD dwMaxAge;
    /* Enable/disable remove events needed flag */
    BOOLEAN  bRemoveRecordsAsNeeded;
} CltrSERVERINFO, *PEVTSERVERINFO;

typedef struct _RPC_LWCOLLECTOR_CONNECTION
{
    BOOLEAN bReadAllowed;
    BOOLEAN bWriteAllowed;
    BOOLEAN bDeleteAllowed;
    PWSTR pwszFromAddress;
    PWSTR pwszClientName;
} RPC_LWCOLLECTOR_CONNECTION, *PRPC_LWCOLLECTOR_CONNECTION;