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

#ifndef __CT_EXEC_H__
#define __CT_EXEC_H__

#include <lw/attrs.h>
#include <lwexc.h>

/**
 * @defgroup CTExec Unix program interaction
 */
/*@{*/

typedef struct __PROCINFO
{
    pid_t pid;
    int   fdin;
    int   fdout;
    int   fderr;
} PROCINFO, *PPROCINFO;


LW_BEGIN_EXTERN_C

/**
 * @brief Capture output of a Unix command
 *
 * Runs the specified Unix command and captures output
 * to a buffer.  The buffer is dynamically allocated and
 * freeing it becomes the responsibility of the caller.
 * The command is passed to the standard Unix shell
 * (/bin/sh), which is reponsible for parsing and executing
 * it; shell features such as pipelines may be used, but
 * care must be taken to properly escape commands. The caller may need to
 * free the output buffer even if the function fails.
 * @see CTEscapeString
 * @param command @in the Unix command to execute
 * @param output @out the dynamically-allocated buffer containing
 * the output of the command
 * @errcode
 * @canfail
 */

DWORD
CTCaptureOutput(
    PCSTR command,
    PSTR* output
    );

/**
 * @brief Run the specified unix command and only show the output if it fails
 *
 * Runs the specified Unix command and captures the output. An exception is
 * returned if the command exits with a non-zero exit code. The output is only
 * returned in the exception, otherwise the command runs silently.
 * @see CTEscapeString
 * @param command @in the Unix command to execute
 * @errcode
 * @canfail
 */
void CTCaptureOutputToExc(
    PCSTR command,
    LWException **exc
    );

/**
 * @brief Capture output of a Unix command
 *
 * Runs the specified Unix command and captures output
 * to a buffer.  The buffer is dynamically allocated and
 * freeing it becomes the responsibility of the caller.
 * The command is passed to the standard Unix shell
 * (/bin/sh), which is reponsible for parsing and executing
 * it; shell features such as pipelines may be used, but
 * care must be taken to properly escape commands. The caller may need to
 * free the output buffer even if the function fails.
 * @see CTEscapeString
 * @param command @in the Unix command to execute
 * @param captureStderr @in whether to capture stderr with stdout or to let it output to the caller's stderr.
 * @param output @out the dynamically-allocated buffer containing
 * the output of the command
 * @errcode
 * @canfail
 */
DWORD
CTCaptureOutputWithStderr(
    PCSTR command,
    BOOLEAN captureStderr,
    PSTR* output
    );

DWORD
CTCaptureOutputWithStderrEx(
    PCSTR command,
    PCSTR* ppszArgs,
    BOOLEAN captureStderr,
    PSTR* output,
    int* exitCode
    );

/**
 * @brief Run a command
 *
 * Runs the specified Unix command and waits for it to
 * complete.  The command is passed to the standard Unix shell
 * (/bin/sh), which is responsible for parsing and executing
 * it; shell features such as pipelines may be used, but
 * care must be taken to properly escape commands.
 *
 * This command returns ERROR_BAD_COMMAND if the program returns a non-
 * zero exit code.
 *
 * @see CTEscapeString
 * @param command @in the Unix command to execute
 * @errcode
 * @canfail
 */
DWORD
CTRunCommand(
    PCSTR command
    );

DWORD
CTSpawnProcessWithFds(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    );

DWORD
CTSpawnProcessWithEnvironment(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    const PSTR* ppszEnv,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    );

void
CTFreeProcInfo(
    PPROCINFO pProcInfo
    );

DWORD
CTGetExitStatus(
    PPROCINFO pProcInfo,
    PLONG plstatus
    );

LW_END_EXTERN_C

    
/*@}*/

#endif
