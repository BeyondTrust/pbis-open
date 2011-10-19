#include "includes.h"

VOID
DNSSetLogParameters(
    LWDNSLogLevel maxLogLevel,
    PFN_LWDNS_LOG_MESSAGE pfnLogMessage
    )
{
    LWDNS_LOCK_LOGGER;

    gLWDNSMaxLogLevel = maxLogLevel;

    gpfnLWDNSLogger = pfnLogMessage;

    LWDNS_UNLOCK_LOGGER;
}

VOID
DNSLogMessage(
    PFN_LWDNS_LOG_MESSAGE pfnLogger,
    LWDNSLogLevel         logLevel,
    PCSTR                 pszFormat,
    ...
    )
{
    va_list msgList;
    va_start(msgList, pszFormat);

    pfnLogger(logLevel, pszFormat, msgList);

    va_end(msgList);
}
