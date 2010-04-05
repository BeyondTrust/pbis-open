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

#ifndef __GPA_EXEC_H__
#define __GPA_EXEC_H__

#include "includes.h"

/**
 * @defgroup GPAExec Unix program interaction
 */
/*@{*/

typedef struct __GPAPROCINFO
{
    pid_t pid;
    int   fdin;
    int   fdout;
    int   fderr;
} GPAPROCINFO, *PGPAPROCINFO;


typedef struct _GPAStackFrame
{
    const char* file;
    unsigned int line;
    struct _GPAStackFrame *down;
} GPAStackFrame;

typedef struct _GPAException
{
    CENTERROR code;
    CENTERROR subcode;
    char* shortMsg;
    char* longMsg;
    GPAStackFrame stack;
} GPAException;

#define GPA_RAISE_EX(dest, code, short_msg, __ARGS__...)	\
    GPARaiseEx(dest, code, __FILE__, __LINE__,		\
	      short_msg, ## __ARGS__)			\

#define GPA_RAISE(dest, code)			\
    GPARaiseEx(dest, code, __FILE__, __LINE__,	\
	      NULL, NULL)			\

#define GPA_CLEANUP_CTERR(dest, err)		\
    do						\
    {						\
	CENTERROR _err = (err);			\
	if (_err)				\
	{					\
	    GPA_RAISE(dest, _err);		\
	    goto cleanup;			\
	}					\
    } while (0)					\

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
 * @see GPAEscapeString
 * @param command @in the Unix command to execute
 * @param output @out the dynamically-allocated buffer containing
 * the output of the command
 * @errcode
 * @canfail
 */
CENTERROR
GPACaptureOutput(
    PCSTR command,
    PSTR* output
    );

/**
 * @brief Run the specified unix command and only show the output if it fails
 *
 * Runs the specified Unix command and captures the output. An exception is
 * returned if the command exits with a non-zero exit code. The output is only
 * returned in the exception, otherwise the command runs silently.
 * @see GPAEscapeString
 * @param command @in the Unix command to execute
 * @errcode
 * @canfail
 */
void GPACaptureOutputToExc(
    PCSTR command,
    GPAException **exc
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
 * @see GPAEscapeString
 * @param command @in the Unix command to execute
 * @param captureStderr @in whether to capture stderr with stdout or to let it output to the caller's stderr.
 * @param output @out the dynamically-allocated buffer containing
 * the output of the command
 * @errcode
 * @canfail
 */
CENTERROR
GPACaptureOutputWithStderr(
    PCSTR command,
    BOOLEAN captureStderr,
    PSTR* output
    );

CENTERROR
GPACaptureOutputWithStderrEx(
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
 * This command returns CENTERROR_COMMAND_FAILED if the program returns a non-
 * zero exit code.
 *
 * @see GPAEscapeString
 * @param command @in the Unix command to execute
 * @errcode
 * @canfail
 */
CENTERROR
GPARunCommand(
    PCSTR command
    );

CENTERROR
GPASpawnProcessWithFds(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PGPAPROCINFO* ppProcInfo
    );

CENTERROR
GPASpawnProcessWithEnvironment(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    const PSTR* ppszEnv,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PGPAPROCINFO* ppProcInfo
    );

void
GPAFreeProcInfo(
    PGPAPROCINFO pProcInfo
    );

CENTERROR
GPAGetExitStatus(
    PGPAPROCINFO pProcInfo,
    PLONG plstatus
    );
    
/*@}*/

#endif
