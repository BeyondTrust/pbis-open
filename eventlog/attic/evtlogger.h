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
#ifndef __EVTLOGGER_H_
#define __EVTLOGGER_H_

#include "evtbase.h"

/*
 * Log levels
 */
#define LOG_LEVEL_ALWAYS  0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_VERBOSE 4
#define LOG_LEVEL_DEBUG 5


#ifdef _WIN32

extern DWORD gLogLevel;
extern FILE* gLogFD;

#define LOG_PRINT(...) ((gLogFD == NULL) ? fprintf(stdout, __VA_ARGS__) : fprintf(gLogFD, __VA_ARGS__))

#define EVT_LOG_ALWAYS(...)                     \
    LOG_PRINT(__VA_ARGS__);

#define EVT_LOG_ERROR(...)                      \
    if (gLogLevel >= LOG_LEVEL_ERROR) {    \
        LOG_PRINT(__VA_ARGS__);  \
    }

#define EVT_LOG_WARNING(...)                    \
    if (gLogLevel >= LOG_LEVEL_WARNING) {  \
       LOG_PRINT(__VA_ARGS__);\
    }

#define EVT_LOG_INFO(...)                       \
    if (gLogLevel >= LOG_LEVEL_INFO)    {  \
        LOG_PRINT(__VA_ARGS__);   \
    }

#define EVT_LOG_VERBOSE(...)                    \
    if (gLogLevel >= LOG_LEVEL_VERBOSE) {  \
        LOG_PRINT(__VA_ARGS__);\
    }

#define EVT_LOG_DEBUG(...)                    \
    if (gLogLevel >= LOG_LEVEL_VERBOSE) {  \
        LOG_PRINT(__VA_ARGS__);\
    }


#else

/*
 * Logging targets
 */
#define LOG_TO_SYSLOG 0
#define LOG_TO_FILE   1
#define LOG_TO_CONSOLE 2
#define LOG_DISABLED  3

typedef struct _LOGFILEINFO {
    CHAR szLogPath[PATH_MAX+1];
    FILE* logHandle;
} LOGFILEINFO, *PLOGFILEINFO;

typedef struct _SYSLOGINFO {
    CHAR szIdentifier[PATH_MAX+1];
    DWORD dwOption;
    DWORD dwFacility;
} SYSLOGINFO, *PSYSLOGINFO;

typedef struct _LOGINFO {
    pthread_mutex_t lock;
    DWORD dwLogLevel;
    DWORD logTarget;
    union _logdata {
        LOGFILEINFO logfile;
        SYSLOGINFO syslog;
    } data;
    BOOLEAN  bLoggingInitiated;
} LOGINFO, *PLOGINFO;

void
evt_log_message(
    DWORD dwLogLevel,
    PSTR pszFormat, ...
    );


DWORD
evt_init_logging_to_syslog(
    DWORD dwLogLevel,
    PSTR  pszIdentifier,
    DWORD dwOption,
    DWORD dwFacility
    );



extern LOGINFO gLogInfo;

#define EVT_LOG_ALWAYS(szFmt...)                     \
    evt_log_message(LOG_LEVEL_ALWAYS, ## szFmt);

#define EVT_LOG_ERROR(szFmt...)                      \
    if (gLogInfo.dwLogLevel >= LOG_LEVEL_ERROR) {    \
        evt_log_message(LOG_LEVEL_ERROR, ## szFmt);  \
    }

#define EVT_LOG_WARNING(szFmt...)                    \
    if (gLogInfo.dwLogLevel >= LOG_LEVEL_WARNING) {  \
        evt_log_message(LOG_LEVEL_WARNING, ## szFmt);\
    }

#define EVT_LOG_INFO(szFmt...)                       \
    if (gLogInfo.dwLogLevel >= LOG_LEVEL_INFO)    {  \
        evt_log_message(LOG_LEVEL_INFO, ## szFmt);   \
    }

#define EVT_LOG_VERBOSE(szFmt...)                    \
    if (gLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {  \
        evt_log_message(LOG_LEVEL_VERBOSE, ## szFmt);\
    }

#define EVT_LOG_DEBUG(szFmt...)                    \
    if (gLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {  \
        evt_log_message(LOG_LEVEL_VERBOSE, ## szFmt);\
    }
#endif //non-_WIN32

DWORD
evt_set_log_level(
	DWORD dwLogLevel
	);

DWORD
evt_init_logging_to_file(
    DWORD dwLogLevel,
    PSTR  pszLogFilePath
    );

void
evt_close_log();

#endif /*__EVTLOGGER_H__*/
