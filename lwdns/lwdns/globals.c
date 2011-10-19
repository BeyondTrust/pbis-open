#include "includes.h"

pthread_mutex_t       gLogLock = PTHREAD_MUTEX_INITIALIZER;

LWDNSLogLevel         gLWDNSMaxLogLevel = LWDNS_LOG_LEVEL_ERROR;

PFN_LWDNS_LOG_MESSAGE gpfnLWDNSLogger = NULL;
