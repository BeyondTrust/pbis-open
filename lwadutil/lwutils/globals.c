#include "includes.h"

HANDLE ghADULog = (HANDLE)NULL;

PFN_LWUTIL_LOG_MESSAGE gpfnADULogHandler = NULL;

LWUtilLogLevel          gLWUtilMaxLogLevel = LWUTIL_LOG_LEVEL_ERROR;
