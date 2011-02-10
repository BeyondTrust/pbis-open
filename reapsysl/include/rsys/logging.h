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
 *        logging.h
 *
 * Abstract:
 *
 *        Reaper for syslog
 * 
 *        Active Directory Site API
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 * 
 */
#ifndef __UNAMED__LOGGING_H__
#define __UNAMED__LOGGING_H__

/*
 * Logging
 */
typedef enum
{
    RSYS_LOG_LEVEL_ALWAYS = 0,
    RSYS_LOG_LEVEL_ERROR,
    RSYS_LOG_LEVEL_WARNING,
    RSYS_LOG_LEVEL_INFO,
    RSYS_LOG_LEVEL_VERBOSE,
    RSYS_LOG_LEVEL_DEBUG
} RSysLogLevel;

typedef enum
{
    RSYS_LOG_TARGET_DISABLED = 0,
    RSYS_LOG_TARGET_CONSOLE,
    RSYS_LOG_TARGET_FILE,
    RSYS_LOG_TARGET_SYSLOG
} RSysLogTarget;

typedef struct __RSYS_LOG_INFO {
    RSysLogLevel  maxAllowedLogLevel;
    RSysLogTarget logTarget;
    PSTR         pszPath;
} RSYS_LOG_INFO, *PRSYS_LOG_INFO;

DWORD
RSysSetLogLevel(
    HANDLE hUnnamedConnection,
    RSysLogLevel logLevel
    );

DWORD
RSysSetLogInfo(
    HANDLE hUnnamedConnection,
    PRSYS_LOG_INFO pLogInfo
    );

DWORD
RSysGetLogInfo(
    HANDLE hUnnamedConnection,
    PRSYS_LOG_INFO* ppLogInfo
    );

VOID
RSysFreeLogInfo(
    PRSYS_LOG_INFO pLogInfo
    );

#endif /* __UNAMED__LOGGING_H__ */
