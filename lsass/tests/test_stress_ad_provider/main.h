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
 *        main.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Test Program for stress testing AD Provider
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __MAIN_H__
#define __MAIN_H__

VOID
ShowUsage(
    VOID
    );

DWORD
LADSInitGlobals(
    VOID
    );

PSTR
LADSGetProgramName(
    PSTR pszFullProgramPath
    );

DWORD
LADSShutdownGlobals(
    VOID
    );

DWORD
LADSParseArgs(
    int argc,
    char* argv[],
    PSTR* ppszConfigFilePath,
    LsaLogLevel* pMaxAllowedLogLevel
    );

DWORD
LADSBlockSelectedSignals(
    VOID
    );

DWORD
LADSHandleSignals(
    VOID
    );

DWORD
LADSInitAuthProvider(
    PCSTR pszConfigFilePath,
    PTEST_AUTH_PROVIDER pProvider
    );

DWORD
LADSValidateProvider(
    PTEST_AUTH_PROVIDER pProvider
    );

DWORD
LADSStartWorkers(
    VOID
    );

DWORD
LADSStopWorkers(
    VOID
    );

DWORD
LADSShutdownAuthProvider(
    PTEST_AUTH_PROVIDER pProvider
    );

VOID
LADSStopProcess(
    VOID
    );

BOOLEAN
LADSProcessShouldStop(
    VOID
    );

#endif /* __MAIN_H__ */
