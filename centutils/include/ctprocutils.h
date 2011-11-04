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
