/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        servermain.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __SERVERMAIN_H__
#define __SERVERMAIN_H__

int
main(
    int argc,
    char* argv[]
    );

int
lsassd_main(
    int argc,
    char* argv[]
    );

DWORD
LsaSrvStartupPreCheck(
    VOID
    );

DWORD
LsaSrvVerifyNetLogonStatus(
    VOID
    );

DWORD
LsaSrvVerifyLwIoStatus(
    VOID
    );

DWORD
LsaSrvRaiseMaxFiles(
    DWORD dwMaxFiles
    );

DWORD
LsaSrvSetDefaults(
    VOID
    );

DWORD
LsaSrvParseArgs(
    int argc,
    PSTR argv[],
    PLSASERVERINFO pLsaServerInfo
    );

PSTR
LsaGetProgramName(
    PSTR pszFullProgramPath
    );

VOID
ShowUsage(
    PCSTR pszProgramName
    );

VOID
LsaSrvExitHandler(
    VOID
    );

DWORD
LsaSrvInitialize(
    VOID
    );

DWORD
LsaInitCacheFolders(
    VOID
    );

BOOLEAN
LsaSrvShouldStartAsDaemon(
    VOID
    );

DWORD
LsaSrvStartAsDaemon(
    VOID
    );

DWORD
LsaSrvGetProcessExitCode(
    PDWORD pdwExitCode
    );

VOID
LsaSrvSetProcessExitCode(
    DWORD dwExitCode
    );

DWORD
LsaSrvGetCachePath(
    PSTR* ppszPath
    );

DWORD
LsaSrvGetPrefixPath(
    PSTR* ppszPath
    );

DWORD
LsaSrvInitLogging(
    PCSTR pszProgramName,
    LsaLogTarget* pTarget,
    PHANDLE phLog
    );

DWORD
LsaBlockSelectedSignals(
    VOID
    );

BOOLEAN
LsaSrvShouldProcessExit(
    VOID
    );

VOID
LsaSrvSetProcessToExit(
    BOOLEAN bExit
    );

VOID
LsaSrvLogProcessStartedEvent(
    VOID
    );

VOID
LsaSrvLogProcessStoppedEvent(
    DWORD dwExitCode
    );

VOID
LsaSrvLogProcessFailureEvent(
    DWORD dwErrCode
    );

#endif /* __SERVERMAIN_H__ */

