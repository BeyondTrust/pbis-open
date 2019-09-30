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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        prototypes.h
 *
 * Abstract:
 *
 *        BeyondTrust Task Service (LWTASK)
 *
 *        Utilities
 *
 *        Function prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

// consolelog.c

DWORD
LwTaskOpenConsoleLog(
    LW_TASK_LOG_LEVEL maxAllowedLogLevel,
    PHANDLE           phLog
    );

DWORD
LwTaskCloseConsoleLog(
    HANDLE hLog
    );

VOID
LwTaskFreeConsoleLogInfo(
    PLW_TASK_CONSOLE_LOG pConsoleLog
    );

// filelog.c

DWORD
LwTaskOpenFileLog(
    PCSTR       pszFilePath,
    LW_TASK_LOG_LEVEL maxAllowedLogLevel,
    PHANDLE     phLog
    );

DWORD
LwTaskGetFileLogInfo(
    HANDLE             hLog,
    PLW_TASK_LOG_INFO* ppLogInfo
    );

DWORD
LwTaskCloseFileLog(
    HANDLE hLog
    );

VOID
LwTaskFreeFileLogInfo(
    PLW_TASK_FILE_LOG pFileLog
    );

// logger.c

DWORD
LwTaskSetupLogging(
    HANDLE                  hLog,
    LW_TASK_LOG_LEVEL       maxAllowedLogLevel,
    PFN_LW_TASK_LOG_MESSAGE pfnLogger
    );

VOID
LwTaskResetLogging(
    VOID
    );

// lwtaskloginfo.c

VOID
LwTaskFreeLogInfo(
    PLW_TASK_LOG_INFO pLogInfo
    );

// sysfuncs.c

#if !HAVE_DECL_ISBLANK
int isblank(int c);
#endif

void
lwtask_vsyslog(
    int priority,
    const char *format,
    va_list ap
    );

#if defined(__LWI_AIX__) || defined(__LWI_HP_UX__)

#if !defined(HAVE_RPL_MALLOC)

void*
rpl_malloc(
    size_t n
    );

#endif /* ! HAVE_RPL_MALLOC */

#if !defined(HAVE_RPL_REALLOC)

void*
rpl_realloc(
    void* buf,
    size_t n
    );

#endif /* ! HAVE_RPL_REALLOC */

#endif /* defined(__LWI_AIX__) || defined(__LWI_HP_UX__) */

// syslog.c

DWORD
LwTaskOpenSyslog(
    PCSTR             pszIdentifier,
    LW_TASK_LOG_LEVEL maxAllowedLogLevel,
    DWORD             dwOptions,
    DWORD             dwFacility,
    PHANDLE           phLog
    );

VOID
LwTaskSetSyslogMask(
    LW_TASK_LOG_LEVEL maxLogLevel
    );

DWORD
LwTaskCloseSyslog(
    HANDLE hLog
    );

VOID
LwTaskFreeSysLogInfo(
    PLW_TASK_SYS_LOG pSysLog
    );
