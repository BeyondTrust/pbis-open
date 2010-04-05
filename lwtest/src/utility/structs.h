#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdio.h>
#include "config.h"

typedef struct _LOG_LEVEL0_DATA
{
    PSTR pszApiName;
    BOOLEAN bResult;
    struct _LOG_LEVEL0_DATA* pNextLog;
} LOG_LEVEL0_DATA, *PLOG_LEVEL0_DATA;

typedef struct LWT_LOG_INFO 
{
     int nLogLevel;
     FILE* fp;
     PSTR pszPath;
} LWT_LOG_INFO, *PLWT_LOG_INFO;

#endif
