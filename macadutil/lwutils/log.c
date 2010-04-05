#include "includes.h"

VOID
LWSetLogHandler(
    HANDLE hLog,
    PFN_MAC_AD_LOG_MESSAGE pfnLogHandler,
    MacADLogLevel maxLogLevel
    )
{  
    ghADULog = hLog;
    gpfnADULogHandler = pfnLogHandler;    
    gMacADMaxLogLevel = maxLogLevel;
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
    MacADLogLevel logLevel,
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
