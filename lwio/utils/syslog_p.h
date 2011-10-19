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
 *        syslog.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 * 
 *        Syslog Logging API
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */

#ifndef __SYSLOG_P_H__
#define __SYSLOG_P_H__

typedef struct
{ 
    PSTR    pszIdentifier;
    BOOLEAN bOpened;
    DWORD   dwFacility;
    DWORD   dwOptions;
} SMB_SYS_LOG, *PSMB_SYS_LOG;

DWORD
SMBOpenSyslog(
    PCSTR       pszIdentifier,
    LWIO_LOG_LEVEL maxAllowedLogLevel,
    DWORD       dwOptions,
    DWORD       dwFacility,
    PHANDLE     phLog
    );

VOID
SMBSetSyslogMask(
    LWIO_LOG_LEVEL logLevel
    );

VOID
SMBLogToSyslog(
    HANDLE      hLog,
    LWIO_LOG_LEVEL dwLogLevel,
    PCSTR       pszFormat,
    va_list     msgList
    );

DWORD
SMBCloseSyslog(
    HANDLE hLog
    );

VOID
SMBFreeSysLogInfo(
    PSMB_SYS_LOG pSysLog
    );

#endif /* __SYSLOG_P_H__ */
