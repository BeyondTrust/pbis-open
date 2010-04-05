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

#ifndef __DJPROCUTILS_H__
#define __DJPROCUTILS_H__

CENTERROR
DJKillProcess(
    PPROCINFO pProcInfo
    );

void
FreeProcInfo(
    PPROCINFO pProcInfo
    );

CENTERROR
DJSpawnProcess(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    PPROCINFO* ppProcInfo
    );

CENTERROR
DJSpawnProcessWithFds(
    PCSTR pszCommand,
    PSTR* ppszArgs,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    );

CENTERROR
DJSpawnProcessSilent(
    PCSTR pszCommand,
    PSTR* ppArgs,
    PPROCINFO* ppProcInfo
    );

CENTERROR
DJSpawnProcessOutputToFile(
    PCSTR pszCommand,
    PSTR* ppArgs,
    PCSTR file,
    PPROCINFO* ppProcInfo
    );

CENTERROR
DJSpawnProcessWithEnvironment(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    const PSTR* ppszEnv,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    );

CENTERROR
DJReadData(
    PPROCINFO pProcInfo,
    PPROCBUFFER pProcBuffer
    );

CENTERROR
DJTimedReadData(
    PPROCINFO pProcInfo,
    PPROCBUFFER pProcBuffer,
    DWORD dwTimeoutSecs,
    PBOOLEAN pbTimedout
    );

CENTERROR
DJWriteData(
    DWORD dwFd,
    PSTR pszBuf,
    DWORD dwLen
    );

CENTERROR
DJGetProcessStatus(
    PPROCINFO pProcInfo,
    PLONG plstatus
    );

#endif /* __DJPROCUTILS_H__ */
