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

#ifndef __CTPROCUTILS_H__
#define __CTPROCUTILS_H__


LW_BEGIN_EXTERN_C

DWORD
CTMatchProgramToPID(
    PCSTR pszProgramName,
    pid_t    pid
    );

/* Find the pid(s) of processes whose executable machines programName. The program name does not include the path. Processes whose root directory does not match the current root directory are not returned. The caller should set count to the size of pid buffer. This function will set pid to the number of matching processes.
 *
 * If more programs match than there is space for in the buffer, as many processes as possible will be filled in. count will be set to how many processes matched, not how many were stored.
 *
 * If count is NULL, then it is assumed that the pid buffer has space for exactly 1 pid. In this case, if at least one pid is found, the first one is stored in the buffer. If no matching pids are found, DWORD_NO_SUCH_PROCESS is returned. When count is not NULL, it is not considered an error when no processes match.
 *
 * pid may be NULL. This is only useful when count is also NULL, in which case the error code can be used to determine whether a process is running or not without actually finding the pid.
 *
 * (uid_t)-1 is treated as wildcard for the owner parameter, and all uids will be matched in this case.
 *
 * Please check the implementation of this function before using it. It may not be implemented for your platform. In this case, it will return DWORD_NOT_IMPLEMENTED.
 */
DWORD
CTGetPidOf(
    PCSTR programName,
    uid_t owner,
    pid_t *pid,
    size_t *count
    );

/* Like CTGetPidOf, except the entire command line (program name plus
 * arguments) can be searched by setting cmdLine.
 *
 * Searching via programFilename may require root access.
 */
DWORD
CTGetPidOfCmdLine(
    PCSTR programName,
    PCSTR programFilename,
    PCSTR cmdLine,
    uid_t owner,
    pid_t *pid,
    size_t *count
    );

DWORD
CTIsProgramRunning(
	PCSTR pszPidFile,
	PCSTR pszProgramName,
	pid_t *pPid,
    PBOOLEAN pbRunning
    );
	
DWORD
CTSendSignal(
	pid_t pid,
	int sig
    );

LW_END_EXTERN_C

	
#endif /* __CTPROCUTILS_H__ */
