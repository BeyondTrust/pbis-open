/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#ifndef __DJLOGGER_H_
#define __DJLOGGER_H_

/*
 * Log levels
 */
#define LOG_LEVEL_ALWAYS  0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_VERBOSE 4

/*
 * Logging targets
 */
#define LOG_TO_FILE   1
#define LOG_DISABLED  3

typedef struct _LOGFILEINFO {
    CHAR szLogPath[PATH_MAX+1];
    FILE* logHandle;
} LOGFILEINFO, *PLOGFILEINFO;

typedef struct _LOGINFO {
    DWORD dwLogLevel;
    DWORD dwLogTarget;
    LOGFILEINFO logfile;
} LOGINFO, *PLOGINFO;

DWORD
dj_set_log_level(
    DWORD dwLogLevel
    );

DWORD
dj_init_logging_to_file(
    DWORD dwLogLevel,
    PSTR  pszLogFilePath
    );

DWORD
dj_init_logging_to_file_handle(
    DWORD dwLogLevel,
    FILE* handle
    );

DWORD
dj_init_logging_to_console(
    DWORD dwLogLevel
    );

DWORD
dj_disable_logging();

void
dj_log_message(
    DWORD dwLogLevel,
    PSTR pszFormat, ...
    );

void
dj_close_log();

DWORD DJLogException(
    DWORD dwLogLevel,
    const LWException *exc);

extern LOGINFO gdjLogInfo;

#define DJ_LOG_ALWAYS(szFmt...)                 \
    dj_log_message(LOG_LEVEL_ALWAYS, ## szFmt);

#define DJ_LOG_ERROR(szFmt...)                              \
    do {                                                    \
        if (gdjLogInfo.dwLogLevel >= LOG_LEVEL_ERROR) {     \
            dj_log_message(LOG_LEVEL_ERROR, ## szFmt);      \
        }                                                   \
    } while(0)

#define DJ_LOG_WARNING(szFmt...)                            \
    do {                                                    \
        if (gdjLogInfo.dwLogLevel >= LOG_LEVEL_WARNING) {   \
            dj_log_message(LOG_LEVEL_WARNING, ## szFmt);    \
        }                                                   \
    } while(0)                                              \

#define DJ_LOG_INFO(szFmt...)                               \
    do {                                                    \
        if (gdjLogInfo.dwLogLevel >= LOG_LEVEL_INFO)    {   \
            dj_log_message(LOG_LEVEL_INFO, ## szFmt);       \
        }                                                   \
    } while(0)

#define DJ_LOG_VERBOSE(szFmt...)                            \
    do {                                                    \
        if (gdjLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {   \
            dj_log_message(LOG_LEVEL_VERBOSE, ## szFmt);    \
        }                                                   \
    } while(0)

#endif /*__DJLOGGER_H__*/
