#include "includes.h"

DWORD gLogLevel = LOG_LEVEL_VERBOSE;
FILE* gBasicLogStreamFD = NULL;

LOGINFO gLWMGMTLogInfo =
{
    PTHREAD_MUTEX_INITIALIZER,
    LOG_LEVEL_ERROR,
    LOG_DISABLED,
    {{"", NULL}},
    0
};


