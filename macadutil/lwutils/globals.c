#include "includes.h"

HANDLE ghADULog = (HANDLE)NULL;

PFN_MAC_AD_LOG_MESSAGE gpfnADULogHandler = NULL;

MacADLogLevel          gMacADMaxLogLevel = MAC_AD_LOG_LEVEL_ERROR;
