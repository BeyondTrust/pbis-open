/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Service (Process Utilities)
 *
 */

#include "server.h"
#include "cltr-base.h"

#define DEFAULT_CONFIG_FILE_PATH CONFIGDIR "/centeris-eventlog.conf"

#define CLTR_DEFAULT_MAX_LOG_SIZE    512 //KB
#define CLTR_DEFAULT_MAX_RECORDS     1000
#define CLTR_DEFAULT_MAX_AGE         7 //days

//whether to remove records which have exceeded their lifespan
#define CLTR_DEFAULT_REM_RECORD_AGE  FALSE


#define CLTR_LOCK_SERVERINFO   EnterCriticalSection(&gServerInfo.lock)
#define CLTR_UNLOCK_SERVERINFO LeaveCriticalSection(&gServerInfo.lock)

DWORD
CltrGetConfigPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    CLTR_LOCK_SERVERINFO;

    dwError = CltrAllocateString((PCWSTR)gServerInfo.szConfigFilePath, (PWCHAR*)ppszPath);

    CLTR_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
CltrGetMaxRecords(
    DWORD* pdwMaxRecords
    )
{
    DWORD dwError = 0;

    CLTR_LOCK_SERVERINFO;

    *pdwMaxRecords = gServerInfo.dwMaxRecords;

    CLTR_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
CltrGetMaxAge(
    DWORD* pdwMaxAge
    )
{
    DWORD dwError = 0;

    CLTR_LOCK_SERVERINFO;

    *pdwMaxAge = gServerInfo.dwMaxAge;

    CLTR_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
CltrGetMaxLogSize(
    DWORD* pdwMaxLogSize
    )
{
    DWORD dwError = 0;

    CLTR_LOCK_SERVERINFO;

    *pdwMaxLogSize = gServerInfo.dwMaxLogSize;

    CLTR_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
CltrGetRemoveEventsFlag(
    PBOOLEAN pbRemoveEvents
    )
{
    DWORD dwError = 0;

    CLTR_LOCK_SERVERINFO;

    *pbRemoveEvents = gServerInfo.bRemoveRecordsAsNeeded;

    CLTR_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
CltrGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    CLTR_LOCK_SERVERINFO;

    dwError = CltrAllocateString((PCWSTR)gServerInfo.szPrefixPath, (PWCHAR*)ppszPath);

    CLTR_UNLOCK_SERVERINFO;

    return (dwError);
}


static
DWORD
CltrSetServerDefaults()
{
    DWORD dwError = 0;

    CLTR_LOCK_SERVERINFO;
    
    //memset(gServerInfo.szConfigFilePath, 0, PATH_MAX+1);
    //strncpy(gServerInfo.szConfigFilePath, DEFAULT_CONFIG_FILE_PATH, PATH_MAX);

    //Set config values
    gServerInfo.dwMaxLogSize = CLTR_DEFAULT_MAX_LOG_SIZE;
    gServerInfo.dwMaxRecords =  CLTR_DEFAULT_MAX_RECORDS;
    gServerInfo.dwMaxAge = CLTR_DEFAULT_MAX_AGE;
    gServerInfo.bRemoveRecordsAsNeeded = CLTR_DEFAULT_REM_RECORD_AGE;
    
    CLTR_UNLOCK_SERVERINFO;
    
    return dwError;
}


static
DWORD
CltrInitLogging()
{
    DWORD dwError = 0;
    PCLTR_LOG pLog = NULL;
    PWSTR pwszLogPath = NULL;
    PSTR pszLogPath = NULL;
    CltrLogLevel dwLevel = 0;

    dwError = CltrGetLogPath(&pwszLogPath);                
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrGetLogLevel(&dwLevel);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = LwRtlCStringAllocateFromWC16String(
                    &pszLogPath,
                    pwszLogPath);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrOpenFileLog(
                    pszLogPath,
                    &pLog);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrSetGlobalLog(
                    pLog,
                    dwLevel);
    BAIL_ON_CLTR_ERROR(dwError);

    CLTR_LOG_VERBOSE("Logging initialized");

cleanup:
    LwRtlCStringFree(&pszLogPath);
    return dwError;
    
error:
    if (pLog)
    {
        CltrCloseLog(pLog);
    }

    goto cleanup;
}

static

/* call back functions to get the values from config file */
DWORD
CltrConfigStartSection(
    PCSTR    pszSectionName,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{

    //This callback may not be required,retaining it for future
    CLTR_LOG_VERBOSE("CltrConfigStartSection: SECTION Name=%s", pszSectionName);

    *pbSkipSection = FALSE;
    *pbContinue = TRUE;

    return 0;
}

DWORD
CltrConfigComment(
    PCSTR    pszComment,
    PBOOLEAN pbContinue
    )
{
    //This callback may not be required,retaining it for future
    CLTR_LOG_VERBOSE("CltrConfigComment: %s",
        (IsNullOrEmptyString(pszComment) ? "" : pszComment));

    *pbContinue = TRUE;

    return 0;
}

DWORD
CltrConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PBOOLEAN pbContinue
    )
{

    //strip the white spaces
    CltrStripWhitespace((PWSTR)pszName,1,1);
    CltrStripWhitespace((PWSTR)pszValue,1,1);

    CLTR_LOG_INFO("CltrConfigNameValuePair: NAME=%s, VALUE=%s",
        (IsNullOrEmptyString(pszName) ? "" : pszName),
        (IsNullOrEmptyString(pszValue) ? "" : pszValue));

    if ( !strcmp(pszName, "MaxLogSize") ) {
        CLTR_LOCK_SERVERINFO;
        gServerInfo.dwMaxLogSize = atoi(pszValue);
        CLTR_UNLOCK_SERVERINFO;
    }
    else if ( !strcmp(pszName, "MaxNumOfRecords") ) {
        CLTR_LOCK_SERVERINFO;
        gServerInfo.dwMaxRecords = atoi(pszValue);
        CLTR_UNLOCK_SERVERINFO;
    }
    else if ( !strcmp(pszName, "MaxAge") ) {
        CLTR_LOCK_SERVERINFO;
        gServerInfo.dwMaxAge = atoi(pszValue);
        CLTR_UNLOCK_SERVERINFO;
    }
    else if ( !strcmp(pszName, "RemoveRecordsAsNeeded") ) {
        CLTR_LOCK_SERVERINFO;
        if ( !strcmp(pszValue, "true") ) {
            gServerInfo.bRemoveRecordsAsNeeded = 1;
        }else if ( !strcmp(pszValue, "false") ) {
            gServerInfo.bRemoveRecordsAsNeeded = 0;
        }
        CLTR_UNLOCK_SERVERINFO;
    }

    *pbContinue = TRUE;

    return 0;
}

DWORD
CltrConfigEndSection(
    PCSTR pszSectionName,
    PBOOLEAN pbContinue
    )
{
    //This callback may not be required,retaining it for future
    CLTR_LOG_VERBOSE("CltrConfigEndSection: SECTION Name=%s", pszSectionName);

    *pbContinue = TRUE;

    return 0;
}


static
DWORD
CltrReadEventLogConfigSettings()
{
    DWORD dwError = 0;
    char* pszConfigFilePath = NULL;

    CLTR_LOG_INFO("Read Eventlog configuration settings");

    dwError = CltrGetConfigPath(&pszConfigFilePath);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrParseConfigFile(
                (PCWSTR)pszConfigFilePath,
                (PFNCONFIG_START_SECTION)&CltrConfigStartSection,
                (PFNCONFIG_COMMENT)&CltrConfigComment,
                (PFNCONFIG_NAME_VALUE_PAIR)&CltrConfigNameValuePair,
                (PFNCONFIG_END_SECTION)&CltrConfigEndSection);
    BAIL_ON_CLTR_ERROR(dwError);

    if (pszConfigFilePath) {
        CltrFreeString((PWCHAR)pszConfigFilePath);
        pszConfigFilePath = NULL;
    }

cleanup:

    return dwError;

error:

    if (pszConfigFilePath) {
        CltrFreeString((PWCHAR)pszConfigFilePath);
    }

    goto cleanup;

}



DWORD
CltrServerMain(
    int argc,
    PSTR argv[])
{
    DWORD dwError = 0;
        
    //Initialize mutex
    CLTR_LOG_INFO("Initialize critical section");
    InitializeCriticalSection(&gServerInfo.lock);
    InitializeCriticalSection(&g_dbLock);   //lock for DB read and write 

    //Initalize server defaults
    dwError = CltrSetServerDefaults();
    BAIL_ON_CLTR_ERROR(dwError);

    
#if 0 //Not required so commenting
    dwError = CltrParseArgs(
                    argc,
                    argv,
                    &gServerInfo
                 );
    BAIL_ON_CLTR_ERROR(dwError);
#endif 

    CLTR_LOG_INFO("Initialize logging...");
    dwError = CltrInitLogging();
    BAIL_ON_CLTR_ERROR(dwError);

    //Initialize Data base
    CLTR_LOG_INFO("Initialize Data base");
    dwError = SrvCreateDB(gServerInfo.bReplaceDB);
    BAIL_ON_CLTR_ERROR(dwError);
    
    if (gServerInfo.bReplaceDB) {
        goto error;
    }    
     
    CLTR_LOG_INFO("Reading conf ...");
    //Read the event log information from eventlog-settings.conf
    //dwError = CltrReadEventLogConfigSettings();
    //if (dwError != 0)
    //{
      //  CLTR_LOG_ERROR("Failed to read eventlog config file.  Error code: [%u]\n", dwError);
       // dwError = 0;
    //}

cleanup:
    return dwError;

error:

    CLTR_LOG_ERROR("Eventlog exiting due to error [code:%d]", dwError);
    goto cleanup;
}
