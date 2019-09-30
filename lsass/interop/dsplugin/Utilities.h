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

#ifndef __Utilities_h__
#define __Utilities_h__ 1

#include "LWIStruct.h"

#ifdef __cplusplus
extern "C" {
#endif

long
LWCaptureOutput(
    char* pszCommand,
    char** ppszOutput
    );

void LogMessageV(const char *Format, va_list Args);
/*
 * Will log a message to either /var/log/opendirectoryd.log or /var/log/syslog.
 * If /var/lib/pbis/lwedsplugin.syslog is present then logging goes to syslog, 
 * otherwise if opendirectorylogging is set to debug it will go to opendirectoryd.log
 */
void LogMessage(const char *Format, ...);
void LogBuffer(void* Buffer, int Length);
const char* StateToString(unsigned long State);
const char* TypeToString(unsigned long Type);
const char* MacErrorToString(long MacError);

BOOLEAN
LWIsUserInLocalGroup(
    char* pszUsername,
    const char* pszGroupname
    );

long
LWRemoveUserFromLocalGroup(
    char* pszUsername,
    const char* pszGroupName
    );

long
LWAddUserToLocalGroup(
    char* pszUsername,
    const char* pszGroupName
    );

#define BOOL_STRING(x) ((x) ? "Y" : "N")

#define SAFE_LOG_STR(s) ((s)?(s):"(null)")

#define _LOG(prefix, format, ...)                                       \
    LogMessage("[LWEDS] %s: %s(): " format, prefix, __FUNCTION__, ##__VA_ARGS__)

#define LOG(format, ...)                        \
    _LOG("     ", format, ##__VA_ARGS__)

#define TRY_CRASH() \
    ((*(volatile char*)0) = 0)

#define NOP() \
    (0)

#define LOG_FATAL(format, ...) \
    do { \
        _LOG("FATAL", format, ##__VA_ARGS__); \
        TRY_CRASH(); \
    } while (0)

#define LOG_ERROR(format, ...)                  \
    _LOG("ERROR", format, ##__VA_ARGS__)

#define LOG_ENTER(format, ...)                  \
    _LOG("ENTER", format, ##__VA_ARGS__)

#define LOG_PARAM(format, ...)                  \
    _LOG("PARAM", format, ##__VA_ARGS__)

#define LOG_LEAVE(format, ...)                  \
    _LOG("LEAVE", format, ##__VA_ARGS__)

#define LOG_BUFFER(buffer, length) \
    LogBuffer(buffer, length)

#define GOTO_CLEANUP() \
    do { goto cleanup; } while (0)

#define GOTO_CLEANUP_EE(EE) \
    do { (EE) = __LINE__; goto cleanup; } while (0)

#define _LOG_GOTO_CLEANUP_ON_MACERROR(macError)                         \
    LOG_ERROR("Error %d (%s) at %s:%d", macError, SAFE_LOG_STR(MacErrorToString(macError)), __FILE__, __LINE__)

#define GOTO_CLEANUP_ON_MACERROR_EE(macError, EE)       \
    do {                                                \
        if (macError) {                                 \
            _LOG_GOTO_CLEANUP_ON_MACERROR(macError);    \
            GOTO_CLEANUP_EE(EE);                        \
        }                                               \
    } while (0)

#define GOTO_CLEANUP_ON_MACERROR(macError)              \
    do {                                                \
        if (macError) {                                 \
            _LOG_GOTO_CLEANUP_ON_MACERROR(macError);    \
            GOTO_CLEANUP();                             \
        }                                               \
    } while (0)

#define FlagOn(var, flag) \
            ((var) & (flag))

#ifdef __cplusplus
}
#endif

#endif
