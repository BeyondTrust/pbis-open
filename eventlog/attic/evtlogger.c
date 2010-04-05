/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog logging utilities
 *
 */
#include "evtlogger.h"


#ifdef _WIN32


DWORD gLogLevel = LOG_LEVEL_VERBOSE;
FILE* gLogFD = NULL;

DWORD
evt_set_log_level(
    DWORD dwLogLevel
    )
{
	gLogLevel = dwLogLevel;
	if(gLogFD == NULL)
	{
		gLogFD = stdout;
	}
	return 0;
}


DWORD
evt_init_logging_to_file(
    DWORD dwLogLevel,
    PSTR  pszLogFilePath
    )
{
	DWORD dwError = 0;
	if(IsNullOrEmptyString(pszLogFilePath)) 
	{
		gLogFD = stdout;
		return dwError;
	}
	
	dwError = fopen_s(&gLogFD, pszLogFilePath, "w");
	BAIL_ON_EVT_ERROR(dwError);

	if(gLogFD == NULL)
	{
		gLogFD = stdout;
		return -1;
	}
	else 
	{
		return dwError;
	}

error:
	gLogFD = stdout;
	return dwError;

}



void
evt_close_log()
{
	fflush(stdout);
}



#else

static const PSTR ERROR_TAG   = "ERROR";
static const PSTR WARN_TAG    = "WARNING";
static const PSTR INFO_TAG    = "INFO";
static const PSTR VERBOSE_TAG = "VERBOSE";

static const PSTR LOG_TIME_FORMAT = "%Y%m%d%H%M%S";

LOGINFO gLogInfo =
{
    PTHREAD_MUTEX_INITIALIZER,
    LOG_LEVEL_ERROR,
    LOG_DISABLED,
    {"", NULL},
    0
};

#define EVT__LOCK_LOGGER   pthread_mutex_lock(&gLogInfo.lock)
#define EVT__UNLOCK_LOGGER pthread_mutex_unlock(&gLogInfo.lock)

DWORD
evt_validate_log_level(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    if (dwLogLevel < 1) {
        dwError = EINVAL;
    }

    return (dwError);
}

void
evt_set_syslogmask(
    DWORD dwLogLevel
    )
{
    DWORD dwSysLogLevel;

    switch (dwLogLevel)
    {
        case LOG_LEVEL_ALWAYS:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }
        case LOG_LEVEL_ERROR:
        {
            dwSysLogLevel = LOG_UPTO(LOG_ERR);
            break;
        }

        case LOG_LEVEL_WARNING:
        {
            dwSysLogLevel = LOG_UPTO(LOG_WARNING);
            break;
        }

        case LOG_LEVEL_INFO:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }

        default:
        {
            dwSysLogLevel = LOG_UPTO(LOG_INFO);
            break;
        }
    }

    setlogmask(dwSysLogLevel);
}

DWORD
evt_init_logging_to_syslog(
    DWORD dwLogLevel,
    PSTR  pszIdentifier,
    DWORD dwOption,
    DWORD dwFacility
    )
{
    DWORD dwError = 0;
    
    EVT__LOCK_LOGGER;

    dwError = evt_validate_log_level(dwLogLevel);
    BAIL_ON_EVT_ERROR(dwError);

    gLogInfo.logTarget = LOG_TO_SYSLOG;

    strncpy(gLogInfo.data.syslog.szIdentifier, pszIdentifier, PATH_MAX);
    *(gLogInfo.data.syslog.szIdentifier+PATH_MAX) = '\0';

    gLogInfo.data.syslog.dwOption = dwOption;
    gLogInfo.data.syslog.dwFacility = dwFacility;

    openlog(pszIdentifier, dwOption, dwFacility);

    evt_set_syslogmask(dwLogLevel);

    gLogInfo.bLoggingInitiated = 1;

    EVT__UNLOCK_LOGGER;

    return (dwError);

  error:

    EVT__UNLOCK_LOGGER;
    
    return (dwError);
}

DWORD
evt_init_logging_to_file(
			 DWORD dwLogLevel,
			 PSTR  pszLogFilePath
			 )
{
  DWORD dwError = 0;
  /*
  
  EVT__LOCK_LOGGER;
  
  if(IsNullOrEmptyString(pszLogFilePath)) 
    {
      gLogInfo.logTarget = LOG_TO_CONSOLE;
      gLogInfo.data.logfile.szLogPath[0] = '\0';
      gLogInfo.data.logfile.logHandle = stdout;
    }
  else 
    {
      gLogInfo.logTarget = LOG_TO_FILE;
      strcpy(gLogInfo.data.logfile.szLogPath, pszLogFilePath);
      
      gLogInfo.data.logfile.logHandle = NULL;
      if (gLogInfo.data.logfile.szLogPath[0] != '\0') {
	gLogInfo.data.logfile.logHandle = freopen(gLogInfo.data.logfile.szLogPath, "w", stderr);
	if (gLogInfo.data.logfile.logHandle == NULL) {
	  dwError = errno;
	  BAIL_ON_EVT_ERROR(dwError);
	}
      }
    }  
  
  dwError = evt_validate_log_level(dwLogLevel);
  BAIL_ON_EVT_ERROR(dwError);
  
  gLogInfo.dwLogLevel = dwLogLevel;
  
  gLogInfo.bLoggingInitiated = 1;
  
  EVT__UNLOCK_LOGGER;
  
  return (dwError);
  
 error:
  
  if (gLogInfo.data.logfile.logHandle != NULL) {
      fclose(gLogInfo.data.logfile.logHandle);
      gLogInfo.data.logfile.logHandle = NULL;
  }
  
  EVT__UNLOCK_LOGGER;
  
  */

  return (dwError);
}

void
evt_close_log()
{
    EVT__LOCK_LOGGER;

    if (!gLogInfo.bLoggingInitiated) {
        EVT__UNLOCK_LOGGER;
        return;
    }

    switch (gLogInfo.logTarget)
    {
        case LOG_TO_SYSLOG:
            {
                /* close connection to syslog */
                closelog();
            }
            break;
        case LOG_TO_FILE:
            {
                if (gLogInfo.data.logfile.logHandle != NULL) {
                    fclose(gLogInfo.data.logfile.logHandle);
                    gLogInfo.data.logfile.logHandle = NULL;
                }
            }
            break;
    }

    EVT__UNLOCK_LOGGER;
}

void
evt_log_to_file_mt_unsafe(
    DWORD dwLogLevel,
    PSTR pszFormat,
    va_list msgList
    )
{
    PSTR pszEntryType = NULL;
    time_t currentTime;
    struct tm tmp;
    char timeBuf[1024];
    FILE* pTarget = NULL;

    switch (dwLogLevel)
    {
        case LOG_LEVEL_ALWAYS:
        {
            pszEntryType = INFO_TAG;
            pTarget = stderr;
            break;
        }
        case LOG_LEVEL_ERROR:
        {
            pszEntryType = ERROR_TAG;
            pTarget = stderr;
            break;
        }

        case LOG_LEVEL_WARNING:
        {
            pszEntryType = WARN_TAG;
            pTarget = stderr;
            break;
        }

        case LOG_LEVEL_INFO:
        {
            pszEntryType = INFO_TAG;
            pTarget = stderr;
            break;
        }

        default:
        {
            pszEntryType = VERBOSE_TAG;
            pTarget = stderr;
            break;
        }
    }

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), LOG_TIME_FORMAT, &tmp);

    fprintf(pTarget, "%s:0x%x:%s:", timeBuf, (long)pthread_self(), pszEntryType);
    vfprintf(pTarget, pszFormat, msgList);
    fprintf(pTarget, "\n", msgList);
    fflush(pTarget);
}

void
evt_log_to_syslog_mt_unsafe(
    DWORD dwLogLevel,
    PSTR pszFormat,
    va_list msgList
    )
{
    switch (dwLogLevel)
    {
        case LOG_LEVEL_ALWAYS:
        {
            vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
        case LOG_LEVEL_ERROR:
        {
            vsyslog(LOG_ERR, pszFormat, msgList);
            break;
        }

        case LOG_LEVEL_WARNING:
        {
            vsyslog(LOG_WARNING, pszFormat, msgList);
            break;
        }

        case LOG_LEVEL_INFO:
        {
            vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }

        default:
        {
            vsyslog(LOG_INFO, pszFormat, msgList);
            break;
        }
    }
}

void
evt_log_message(
    DWORD dwLogLevel,
    PSTR pszFormat,...
    )
{
  /*
    va_list argp;
  
    EVT__LOCK_LOGGER;

    if ( !gLogInfo.bLoggingInitiated ||
         gLogInfo.logTarget == LOG_DISABLED ) {
        EVT__UNLOCK_LOGGER;
        return;
    }

    if (gLogInfo.dwLogLevel < dwLogLevel) {
        EVT__UNLOCK_LOGGER;
        return;
    }

    va_start(argp, pszFormat);

    switch (gLogInfo.logTarget)
    {
        case LOG_TO_SYSLOG:
        {
            evt_log_to_syslog_mt_unsafe(dwLogLevel, pszFormat, argp);
            break;
        }
        case LOG_TO_FILE:
        {
            evt_log_to_file_mt_unsafe(dwLogLevel, pszFormat, argp);
            break;
        }
    }

    va_end(argp);

    EVT__UNLOCK_LOGGER;
  */
}

DWORD
evt_set_log_level(
    DWORD dwLogLevel
    )
{
    DWORD dwError = 0;

    EVT__LOCK_LOGGER;

    dwError = evt_validate_log_level(dwLogLevel);
    BAIL_ON_EVT_ERROR(dwError);

    gLogInfo.dwLogLevel = dwLogLevel;

    EVT__UNLOCK_LOGGER;

    return (dwError);
    
error:

    EVT__UNLOCK_LOGGER;

    return (dwError);
}

#endif //not _WIN32
