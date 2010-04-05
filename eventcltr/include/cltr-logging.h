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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        cltr-logging.h
 *
 * Abstract:
 *
 *        Event Collector
 *        Logging interface
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#ifndef __CLTR_LOGGING_H__
#define __CLTR_LOGGING_H__

typedef struct _CLTR_LOG CLTR_LOG, *PCLTR_LOG;

typedef enum
{
    CLTR_LOG_LEVEL_ALWAYS = 0,
    CLTR_LOG_LEVEL_ERROR,
    CLTR_LOG_LEVEL_WARNING,
    CLTR_LOG_LEVEL_INFO,
    CLTR_LOG_LEVEL_VERBOSE,
    CLTR_LOG_LEVEL_DEBUG
} CltrLogLevel;

DWORD
CltrOpenSplitFileLogEx(
    IN FILE *pInfoFile,
    IN FILE *pErrorFile,
    OUT PCLTR_LOG* ppLog
    );

DWORD
CltrOpenSplitFileLog(
    IN PCSTR       pszInfoPath,
    IN PCSTR       pszErrorPath,
    OUT PCLTR_LOG* ppLog
    );

DWORD
CltrOpenSingleFileLogEx(
    IN FILE *pFile,
    OUT PCLTR_LOG* ppLog
    );

DWORD
CltrOpenFileLog(
    IN PCSTR       pszFilePath,
    OUT PCLTR_LOG* ppLog
    );

DWORD
CltrCloseLog(
    IN PCLTR_LOG pLog
    );

DWORD
CltrWriteToLogV(
    IN PCLTR_LOG pLog,
    IN CltrLogLevel level,
    IN PCSTR pszFormat,
    IN va_list args
    );

DWORD
CltrOpenSyslog(
    IN PCSTR       pszIdentifier,
    IN DWORD       dwOptions,
    IN DWORD       dwFacility,
    OUT PCLTR_LOG* ppLog
    );

DWORD
CltrSetGlobalLog(
    IN PCLTR_LOG pLog,
    IN CltrLogLevel level
    );

DWORD
CltrCloseGlobalLog();

DWORD
CltrLogMessage(
    CltrLogLevel level,
    PCSTR pszFormat,
    ...
    );

#define CLTR_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define CLTR_LOG_ALWAYS(szFmt, ...) \
    CltrLogMessage(CLTR_LOG_LEVEL_ALWAYS, szFmt, ## __VA_ARGS__)

#define CLTR_LOG_ERROR(szFmt, ...) \
    CltrLogMessage(CLTR_LOG_LEVEL_ERROR, szFmt, ## __VA_ARGS__)

#define CLTR_LOG_WARNING(szFmt, ...) \
    CltrLogMessage(CLTR_LOG_LEVEL_WARNING, szFmt, ## __VA_ARGS__)

#define CLTR_LOG_INFO(szFmt, ...) \
    CltrLogMessage(CLTR_LOG_LEVEL_INFO, szFmt, ## __VA_ARGS__)

#define CLTR_LOG_VERBOSE(szFmt, ...) \
    CltrLogMessage(CLTR_LOG_LEVEL_VERBOSE, szFmt, ## __VA_ARGS__)

#define CLTR_LOG_DEBUG(szFmt, ...) \
    CltrLogMessage(CLTR_LOG_LEVEL_DEBUG, szFmt, ## __VA_ARGS__)


#endif /* __CLTR_LOGGING_H__ */
