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
 *        rsys-logging.h
 *
 * Abstract:
 *
 *        Reaper for syslog
 *        Logging interface
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#ifndef __RSYS_LOGGING_H__
#define __RSYS_LOGGING_H__

#include <rsys/logging.h>

typedef struct _RSYS_LOG RSYS_LOG, *PRSYS_LOG;

DWORD
RSysOpenSplitFileLogEx(
    IN FILE *pInfoFile,
    IN FILE *pErrorFile,
    OUT PRSYS_LOG* ppLog
    );

DWORD
RSysOpenSplitFileLog(
    IN PCSTR       pszInfoPath,
    IN PCSTR       pszErrorPath,
    OUT PRSYS_LOG* ppLog
    );

DWORD
RSysOpenSingleFileLogEx(
    IN FILE *pFile,
    OUT PRSYS_LOG* ppLog
    );

DWORD
RSysOpenFileLog(
    IN PCSTR       pszFilePath,
    OUT PRSYS_LOG* ppLog
    );

DWORD
RSysCloseLog(
    IN PRSYS_LOG pLog
    );

DWORD
RSysWriteToLogV(
    IN PRSYS_LOG pLog,
    IN RSysLogLevel level,
    IN PCSTR pszFormat,
    IN va_list args
    );

DWORD
RSysOpenSyslog(
    IN PCSTR       pszIdentifier,
    IN DWORD       dwOptions,
    IN DWORD       dwFacility,
    OUT PRSYS_LOG* ppLog
    );

DWORD
RSysSetGlobalLog(
    IN PRSYS_LOG pLog,
    IN RSysLogLevel level
    );

DWORD
RSysCloseGlobalLog();

DWORD
RSysLogMessage(
    RSysLogLevel level,
    PCSTR pszFormat,
    ...
    );

#define RSYS_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define RSYS_LOG_ALWAYS(szFmt, ...) \
    RSysLogMessage(RSYS_LOG_LEVEL_ALWAYS, szFmt, ## __VA_ARGS__)

#define RSYS_LOG_ERROR(szFmt, ...) \
    RSysLogMessage(RSYS_LOG_LEVEL_ERROR, szFmt, ## __VA_ARGS__)

#define RSYS_LOG_WARNING(szFmt, ...) \
    RSysLogMessage(RSYS_LOG_LEVEL_WARNING, szFmt, ## __VA_ARGS__)

#define RSYS_LOG_INFO(szFmt, ...) \
    RSysLogMessage(RSYS_LOG_LEVEL_INFO, szFmt, ## __VA_ARGS__)

#define RSYS_LOG_VERBOSE(szFmt, ...) \
    RSysLogMessage(RSYS_LOG_LEVEL_VERBOSE, szFmt, ## __VA_ARGS__)

#define RSYS_LOG_DEBUG(szFmt, ...) \
    RSysLogMessage(RSYS_LOG_LEVEL_DEBUG, szFmt, ## __VA_ARGS__)


#endif /* __RSYS_LOGGING_H__ */
