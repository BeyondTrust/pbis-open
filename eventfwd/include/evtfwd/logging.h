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
 *        Unnamed Description
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
    EFD_LOG_LEVEL_ALWAYS = 0,
    EFD_LOG_LEVEL_ERROR,
    EFD_LOG_LEVEL_WARNING,
    EFD_LOG_LEVEL_INFO,
    EFD_LOG_LEVEL_VERBOSE,
    EFD_LOG_LEVEL_DEBUG
} EfdLogLevel;

typedef enum
{
    EFD_LOG_TARGET_DISABLED = 0,
    EFD_LOG_TARGET_CONSOLE,
    EFD_LOG_TARGET_FILE,
    EFD_LOG_TARGET_SYSLOG
} EfdLogTarget;

typedef struct __EFD_LOG_INFO {
    EfdLogLevel  maxAllowedLogLevel;
    EfdLogTarget logTarget;
    PSTR         pszPath;
} EFD_LOG_INFO, *PEFD_LOG_INFO;

DWORD
EfdSetLogLevel(
    HANDLE hUnnamedConnection,
    EfdLogLevel logLevel
    );

DWORD
EfdSetLogInfo(
    HANDLE hUnnamedConnection,
    PEFD_LOG_INFO pLogInfo
    );

DWORD
EfdGetLogInfo(
    HANDLE hUnnamedConnection,
    PEFD_LOG_INFO* ppLogInfo
    );

VOID
EfdFreeLogInfo(
    PEFD_LOG_INFO pLogInfo
    );

#endif /* __UNAMED__LOGGING_H__ */
