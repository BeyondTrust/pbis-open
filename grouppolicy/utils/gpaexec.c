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

#include "includes.h"

CENTERROR
GPACaptureOutput(
    PCSTR command,
    PSTR* output
    )
{
    return GPACaptureOutputWithStderr(command, FALSE, output);
}

CENTERROR
GPACaptureOutputWithStderr(
    PCSTR command,
    BOOLEAN captureStderr,
    PSTR* output
    )
{
    return GPAShell("/bin/sh -c %command >%output 2>&%redirect_stderr",
                   GPASHELL_STRING(command, command),
                   GPASHELL_BUFFER(output, output),
                   GPASHELL_INTEGER(redirect_stderr, captureStderr ? 1 : 2));
}

CENTERROR
GPACaptureOutputWithStderrEx(
    PCSTR command,
    PCSTR* ppszArgs,
    BOOLEAN captureStderr,
    PSTR* output,
    int *exitCode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    unsigned int buffer_size = 1024;
    unsigned int read_size, write_size;
    int out[2];
    int pid, status;
    PSTR tempOutput = NULL;

    if(output != NULL)
        *output = NULL;
    
    if (pipe(out))
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);   
    }
    
    pid = fork();
    
    if (pid < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);     
    }
    else if (pid == 0)
    {
        // Child process
        if (dup2(out[1], STDOUT_FILENO) < 0)
            abort();
        if (captureStderr && dup2(out[1], STDERR_FILENO) < 0)
            abort();
        if (close(out[0]))
            abort();
        if (close(out[1]))
            abort();
        execvp(command, (char **)ppszArgs);
    }
    
    if (close(out[1]))
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);     
    }
    
    ceError = LwAllocateMemory(buffer_size, (PVOID*) &tempOutput);
    BAIL_ON_GPA_ERROR(ceError);
    
    write_size = 0;
    
    while ((read_size = read(out[0], tempOutput + write_size, buffer_size - write_size)) > 0)
    {
        write_size += read_size;
        if (write_size == buffer_size)
        {
            buffer_size *= 2;
            ceError = LwReallocMemory(tempOutput, (PVOID*) &tempOutput, buffer_size);
            BAIL_ON_GPA_ERROR(ceError);
        }
    }
    
    if (read_size < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError); 
    }
    
    if (close(out[0]))
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError); 
    }
    
    if (waitpid(pid, &status, 0) != pid)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    if(output != NULL)
    {
        *output = tempOutput;
        tempOutput = NULL;
    }
    
    if(exitCode != NULL)
        *exitCode = WEXITSTATUS(status);
    else if (status)
    {
        ceError = CENTERROR_COMMAND_FAILED;
        BAIL_ON_GPA_ERROR(ceError); 
    }
    
error:   
    LW_SAFE_FREE_MEMORY(tempOutput);
    return ceError;
}

CENTERROR
GPARunCommand(
    PCSTR command
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    int code = system(command);
    
    if (code < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);   
    }
    else if (code > 0)
    {
        ceError = CENTERROR_COMMAND_FAILED;
        BAIL_ON_GPA_ERROR(ceError);
    }
    
error:   
    return ceError;
}

CENTERROR
GPASpawnProcessWithFds(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PGPAPROCINFO* ppProcInfo
    )
{
    return GPASpawnProcessWithEnvironment(pszCommand, ppszArgs, NULL, dwFdIn, dwFdOut, dwFdErr, ppProcInfo);
}

CENTERROR
GPASpawnProcessWithEnvironment(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    const PSTR* ppszEnv,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PGPAPROCINFO* ppProcInfo
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPAPROCINFO pProcInfo = NULL;
    int pid = -1;
    int iFd = 0;
    int fd0[2] = {-1, -1};
    int fd1[2] = {-1, -1};
    int fd2[2] = {-1, -1};
    int maxfd = 0;
    struct rlimit rlm;

    *ppProcInfo = 0;

    memset(&rlm, 0, sizeof(struct rlimit));

    if (getrlimit(RLIMIT_NOFILE, &rlm) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    maxfd = rlm.rlim_max;


    if (dwFdIn >= 0)
    {
        fd0[0] = dup(dwFdIn);
    } else if (pipe(fd0) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    if (dwFdOut >= 0)
    {
        fd1[1] = dup(dwFdOut);
    } else if (pipe(fd1) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    if (dwFdErr >= 0)
    {
        fd2[1] = dup(dwFdErr);
    } else if (pipe(fd2) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    if ((pid = fork()) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    if (pid > 0) {
        /* parent */
        if (fd0[0] >= 0) close(fd0[0]); fd0[0] = -1;
        if (fd1[1] >= 0) close(fd1[1]); fd1[1] = -1;
        if (fd2[1] >= 0) close(fd2[1]); fd2[1] = -1;

        ceError = LwAllocateMemory(sizeof(GPAPROCINFO), (PVOID*)&pProcInfo);
        BAIL_ON_GPA_ERROR(ceError);

        pProcInfo->pid = pid;
        pProcInfo->fdin = fd0[1];
        pProcInfo->fdout = fd1[0];
        pProcInfo->fderr = fd2[0];

        *ppProcInfo = pProcInfo;
        pProcInfo = NULL;

    }

    if (pid == 0) {

        /* child -- must exit/abort and never bail via goto */
        if (fd0[1] >= 0) close(fd0[1]); fd0[1] = -1;
        if (fd1[0] >= 0) close(fd1[0]); fd1[0] = -1;
        if (fd2[0] >= 0) close(fd2[0]); fd2[0] = -1;

        if (fd0[0] != STDIN_FILENO) {
            if (dup2(fd0[0], STDIN_FILENO) != STDIN_FILENO) {
                abort();
            }
            close(fd0[0]); fd0[0] = -1;
        }

        if (fd1[1] != STDOUT_FILENO) {
            if (dup2(fd1[1], STDOUT_FILENO) != STDOUT_FILENO) {
                abort();
            }
            close(fd1[1]); fd1[1] = -1;
        }

        if (fd2[1] != STDERR_FILENO) {
            if (dup2(fd2[1], STDERR_FILENO) != STDERR_FILENO) {
                abort();
            }
            close(fd2[1]); fd2[1] = -1;
        }

        for (iFd = STDERR_FILENO+1; iFd < 20; iFd++)
            close(iFd);

        if (ppszEnv)
            execve(pszCommand, (char **)ppszArgs, (char **)ppszEnv);
        else
            execvp(pszCommand, (char **)ppszArgs);
        _exit(127);
    }

    return ceError;

error:

    if (pProcInfo)
        GPAFreeProcInfo(pProcInfo);

    for(iFd = 0; iFd < 2; iFd++)
        if (fd0[iFd] >= 0)
            close(fd0[iFd]);

    for(iFd = 0; iFd < 2; iFd++)
        if (fd1[iFd] >= 0)
            close(fd1[iFd]);

    for(iFd = 0; iFd < 2; iFd++)
        if (fd2[iFd] >= 0)
            close(fd2[iFd]);

    return ceError;
}

void
GPAFreeProcInfo(
    PGPAPROCINFO pProcInfo
    )
{
    LONG status = 0;

    if(pProcInfo == NULL)
        return;

    if (pProcInfo->pid != 0) {
        GPAGetExitStatus(pProcInfo, &status);
    }

    if (pProcInfo->fdin >= 0)
        close(pProcInfo->fdin);
    if (pProcInfo->fdout >= 0)
        close(pProcInfo->fdout);
    if (pProcInfo->fderr >= 0)
        close(pProcInfo->fderr);

    LwFreeMemory(pProcInfo);
}

CENTERROR
GPAGetExitStatus(
    PGPAPROCINFO pProcInfo,
    PLONG plstatus
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int status = 0;

    while(1)
    {
        if (waitpid(pProcInfo->pid, &status, 0) < 0) {
            if (errno == EINTR)
                continue;
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_GPA_ERROR(ceError);
        }
        else
            break;
    }

    if (WIFEXITED(status)) {
        *plstatus = WEXITSTATUS(status);
    } else {
        BAIL_ON_GPA_ERROR(CENTERROR_ABNORMAL_TERMINATION);
    }

error:

    /* Mark this PROCINFO as unused */
    pProcInfo->pid = 0;

    return ceError;
}

static GPAException successExc =
{
    .code = CENTERROR_SUCCESS,
    .shortMsg = "Success",
    .longMsg = "The operation succeeded without error.",
    .stack =
    {
	NULL,
	0,
	NULL
    }
};

static GPAException memExc =
{
    .code = CENTERROR_OUT_OF_MEMORY,
    .shortMsg = "Out of memory",
    .longMsg = "A memory allocation failed due to insufficient system resources.",
    .stack =
    {
	NULL,
	0,
	NULL
    }
};

static GPAException*
CreateException(
    CENTERROR code,
    const char* file,
    unsigned int line,
    char* shortMsg,
    char* longMsg
    )
{
    GPAException* exc;

    switch (code)
    {
    case CENTERROR_SUCCESS:
	return &successExc;
    case CENTERROR_OUT_OF_MEMORY:
	return &memExc;
    default:
	exc = malloc(sizeof(*exc));
	if (!exc)
	    return &memExc;
	exc->code = code;
        exc->subcode = 0;
	exc->stack.file = file;
	exc->stack.line = line;
	exc->stack.down = NULL;
	exc->shortMsg = shortMsg;
	exc->longMsg = longMsg;

	return exc;
    }
}

void
GPARaiseEx(
    GPAException** dest,
    CENTERROR code,
    const char* file,
    unsigned int line,
    const char* _shortMsg,
    const char* fmt,
    ...
    )
{
    if (dest)
    {
	CENTERROR ceError;
	char* shortMsg;
	char* longMsg;
	va_list ap;
	
	va_start(ap, fmt);
	
	if (!_shortMsg)
	{
	    _shortMsg = CTErrorDescription(code);
	}
        if (!_shortMsg)
        {
            _shortMsg = "Undocumented exception";
        }

	if (!fmt)
	{
	    fmt = CTErrorHelp(code);
	}
        if (!fmt)
        {
            fmt = "An undocumented exception has occurred. Please contact Likewise technical support and use the error code to identify this exception.";
        }

	if (_shortMsg)
	{
	    if ((ceError = LwAllocateString(_shortMsg, &shortMsg)))
	    {
		*dest = CreateException(ceError, __FILE__, __LINE__, NULL, NULL);
		return;
	    }
	}
	else
	{
	    shortMsg = NULL;
	}
	    
	if (fmt)
	{
	    if ((ceError = LwAllocateStringPrintfV(&longMsg, fmt, ap)))
	    {
		LwFreeString(shortMsg);
		*dest = CreateException(ceError, __FILE__, __LINE__, NULL, NULL);
		return;
	    }
	}
	else
	{
	    longMsg = NULL;
	}

	*dest = CreateException(code, file, line, shortMsg, longMsg);
    }
}


void GPACaptureOutputToExc(
    PCSTR command,
    GPAException **exc
    )
{
    PSTR output = NULL;
    CENTERROR ceError = GPACaptureOutputWithStderr(command, TRUE, &output);

    if(ceError == CENTERROR_COMMAND_FAILED)
    {
        PSTR showOutput = output;
        if(showOutput == NULL)
            showOutput = "";
        GPA_RAISE_EX(exc, ceError, "Command failed", "%s", showOutput);
        goto cleanup;
    }
    GPA_CLEANUP_CTERR(exc, ceError);

cleanup:
    LW_SAFE_FREE_STRING(output);
}
