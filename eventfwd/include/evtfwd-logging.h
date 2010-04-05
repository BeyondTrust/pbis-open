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
 *        evtfwd-logging.h
 *
 * Abstract:
 *
 *        Event forwarder from eventlogd to collector service
 *        Logging interface
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#ifndef __EFD_LOGGING_H__
#define __EFD_LOGGING_H__

#include <evtfwd/logging.h>

typedef struct _EFD_LOG EFD_LOG, *PEFD_LOG;

DWORD
EfdOpenSplitFileLogEx(
    IN FILE *pInfoFile,
    IN FILE *pErrorFile,
    OUT PEFD_LOG* ppLog
    );

DWORD
EfdOpenSplitFileLog(
    IN PCSTR       pszInfoPath,
    IN PCSTR       pszErrorPath,
    OUT PEFD_LOG* ppLog
    );

DWORD
EfdOpenSingleFileLogEx(
    IN FILE *pFile,
    OUT PEFD_LOG* ppLog
    );

DWORD
EfdOpenFileLog(
    IN PCSTR       pszFilePath,
    OUT PEFD_LOG* ppLog
    );

DWORD
EfdCloseLog(
    IN PEFD_LOG pLog
    );

DWORD
EfdWriteToLogV(
    IN PEFD_LOG pLog,
    IN EfdLogLevel level,
    IN PCSTR pszFormat,
    IN va_list args
    );

DWORD
EfdOpenSyslog(
    IN PCSTR       pszIdentifier,
    IN DWORD       dwOptions,
    IN DWORD       dwFacility,
    OUT PEFD_LOG* ppLog
    );

DWORD
EfdSetGlobalLog(
    IN PEFD_LOG pLog,
    IN EfdLogLevel level
    );

DWORD
EfdCloseGlobalLog();

DWORD
EfdLogMessage(
    EfdLogLevel level,
    PCSTR pszFormat,
    ...
    );

#define EFD_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define EFD_LOG_ALWAYS(szFmt, ...) \
    EfdLogMessage(EFD_LOG_LEVEL_ALWAYS, szFmt, ## __VA_ARGS__)

#define EFD_LOG_ERROR(szFmt, ...) \
    EfdLogMessage(EFD_LOG_LEVEL_ERROR, szFmt, ## __VA_ARGS__)

#define EFD_LOG_WARNING(szFmt, ...) \
    EfdLogMessage(EFD_LOG_LEVEL_WARNING, szFmt, ## __VA_ARGS__)

#define EFD_LOG_INFO(szFmt, ...) \
    EfdLogMessage(EFD_LOG_LEVEL_INFO, szFmt, ## __VA_ARGS__)

#define EFD_LOG_VERBOSE(szFmt, ...) \
    EfdLogMessage(EFD_LOG_LEVEL_VERBOSE, szFmt, ## __VA_ARGS__)

#define EFD_LOG_DEBUG(szFmt, ...) \
    EfdLogMessage(EFD_LOG_LEVEL_DEBUG, szFmt, ## __VA_ARGS__)


#endif /* __EFD_LOGGING_H__ */
