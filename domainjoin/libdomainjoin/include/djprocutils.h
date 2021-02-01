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

#ifndef __DJPROCUTILS_H__
#define __DJPROCUTILS_H__

void
FreeProcInfo(
    PPROCINFO pProcInfo
    );

DWORD
DJSpawnProcess(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    PPROCINFO* ppProcInfo
    );

DWORD
DJSpawnProcessWithFds(
    PCSTR pszCommand,
    PSTR* ppszArgs,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    );

DWORD
DJSpawnProcessSilent(
    PCSTR pszCommand,
    PSTR* ppArgs,
    PPROCINFO* ppProcInfo
    );

DWORD
DJSpawnProcessOutputToFile(
    PCSTR pszCommand,
    PSTR* ppArgs,
    PCSTR file,
    PPROCINFO* ppProcInfo
    );

DWORD
DJSpawnProcessWithEnvironment(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    const PSTR* ppszEnv,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    );

DWORD
DJReadData(
    PPROCINFO pProcInfo,
    PPROCBUFFER pProcBuffer
    );

DWORD
DJTimedReadData(
    PPROCINFO pProcInfo,
    PPROCBUFFER pProcBuffer,
    DWORD dwTimeoutSecs,
    PBOOLEAN pbTimedout
    );

DWORD
DJWriteData(
    DWORD dwFd,
    PSTR pszBuf,
    DWORD dwLen
    );

DWORD
DJGetProcessStatus(
    PPROCINFO pProcInfo,
    PLONG plstatus
    );

#endif /* __DJPROCUTILS_H__ */
