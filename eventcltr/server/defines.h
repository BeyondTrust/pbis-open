
#define EVENTLOG_DB_FILENAME  "LWCollector.db"
#define EVENTLOG_LOG_FILENAME  L"LWCollector.log"


#define ENTER_RW_READER_LOCK EnterCriticalSection(&g_dbLock)
#define LEAVE_RW_READER_LOCK LeaveCriticalSection(&g_dbLock)

#define ENTER_RW_WRITER_LOCK EnterCriticalSection(&g_dbLock)
#define LEAVE_RW_WRITER_LOCK LeaveCriticalSection(&g_dbLock)
