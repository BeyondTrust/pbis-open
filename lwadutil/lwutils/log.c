#include "includes.h"

VOID
LWSetLogHandler(
    HANDLE hLog,
    PFN_LWUTIL_LOG_MESSAGE pfnLogHandler,
    LWUtilLogLevel maxLogLevel
    )
{  
    ghADULog = hLog;
    gpfnADULogHandler = pfnLogHandler;    
    gLWUtilMaxLogLevel = maxLogLevel;
}

VOID
LWResetLogHandler(
    VOID
    )
{   
    ghADULog = (HANDLE)NULL;
    gpfnADULogHandler = NULL;
}

VOID
LWLogMessage(
    LWUtilLogLevel logLevel,
    PCSTR         pszFormat,
    ...
    )
{
    if (gpfnADULogHandler)
    {
        va_list msgList;
        va_start(msgList, pszFormat);

        gpfnADULogHandler(ghADULog, logLevel, pszFormat, msgList);

        va_end(msgList);
    }
}
