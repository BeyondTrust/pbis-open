/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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
