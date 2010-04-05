/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#include "includes.h"

DWORD
LWCaptureOutput(
    PCSTR command,
    PSTR* output
    )
{
    return LWCaptureOutputWithStderr(command, FALSE, output);
}

DWORD
LWCaptureOutputWithStderr(
    PCSTR command,
    BOOLEAN captureStderr,
    PSTR* output
    )
{
    PCSTR args[] = { "/bin/sh", "-c", command, (char*) NULL };
    return LWCaptureOutputWithStderrEx( args[0], args, captureStderr, output, NULL);
}

DWORD
LWCaptureOutputWithStderrEx(
    PCSTR   command,
    PCSTR*  ppszArgs,
    BOOLEAN captureStderr,
    PSTR*   output,
    int *   exitCode
    )
{
    DWORD dwError = 0;
    unsigned int buffer_size = 1024;
    unsigned int read_size, write_size;
    int out[2];
    int pid, status;
    PSTR tempOutput = NULL;

    if(output != NULL)
        *output = NULL;
    
    if (pipe(out))
    {
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);   
    }
    
    pid = fork();
    
    if (pid < 0)
    {
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);     
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
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);     
    }
    
    dwError = LWAllocateMemory(buffer_size, (PVOID*) &tempOutput);
    BAIL_ON_LWUTIL_ERROR(dwError);
    
    write_size = 0;
    
    while ((read_size = read(out[0], tempOutput + write_size, buffer_size - write_size)) > 0)
    {
        write_size += read_size;
        if (write_size == buffer_size)
        {
            buffer_size *= 2;
            dwError = LWReallocMemory(tempOutput, (PVOID*) &tempOutput, buffer_size);
            BAIL_ON_LWUTIL_ERROR(dwError);
        }
    }
    
    if (read_size < 0)
    {
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError); 
    }
    
    if (close(out[0]))
    {
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError); 
    }
    
    if (waitpid(pid, &status, 0) != pid)
    {
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    if(output != NULL)
    {
        *output = tempOutput;
        tempOutput = NULL;
    }
    
    if(exitCode != NULL)
    {
        *exitCode = WEXITSTATUS(status);
    }
    else if (status)
    {
        dwError = LWUTIL_ERROR_IPC_FAILED;
        BAIL_ON_LWUTIL_ERROR(dwError); 
    }
    
cleanup:

    if (tempOutput)
        LWFreeMemory(tempOutput);
        
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWRunCommand(
    PCSTR command
    )
{
    DWORD dwError = 0;
    
    int code = system(command);
    
    if (code < 0)
    {
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);   
    }
    else if (code > 0)
    {
        dwError = LWUTIL_ERROR_IPC_FAILED;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWSpawnProcessWithFds(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    )
{
    return LWSpawnProcessWithEnvironment(pszCommand, ppszArgs, NULL, dwFdIn, dwFdOut, dwFdErr, ppProcInfo);
}

DWORD
LWSpawnProcessWithEnvironment(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    const PSTR* ppszEnv,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    )
{
    DWORD dwError = 0;
    PPROCINFO pProcInfo = NULL;
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
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    maxfd = rlm.rlim_max;


    if (dwFdIn >= 0)
    {
        fd0[0] = dup(dwFdIn);
    } else if (pipe(fd0) < 0) {
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    if (dwFdOut >= 0)
    {
        fd1[1] = dup(dwFdOut);
    } else if (pipe(fd1) < 0) {
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    if (dwFdErr >= 0)
    {
        fd2[1] = dup(dwFdErr);
    } else if (pipe(fd2) < 0) {
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    if ((pid = fork()) < 0) {
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    if (pid > 0) {
        /* parent */
        if (fd0[0] >= 0) close(fd0[0]); fd0[0] = -1;
        if (fd1[1] >= 0) close(fd1[1]); fd1[1] = -1;
        if (fd2[1] >= 0) close(fd2[1]); fd2[1] = -1;

        dwError = LWAllocateMemory(sizeof(PROCINFO), (PVOID*)&pProcInfo);
        BAIL_ON_LWUTIL_ERROR(dwError);

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

cleanup:

    if (pProcInfo)
        LWFreeProcInfo(pProcInfo);

    for(iFd = 0; iFd < 2; iFd++)
        if (fd0[iFd] >= 0)
            close(fd0[iFd]);

    for(iFd = 0; iFd < 2; iFd++)
        if (fd1[iFd] >= 0)
            close(fd1[iFd]);

    for(iFd = 0; iFd < 2; iFd++)
        if (fd2[iFd] >= 0)
            close(fd2[iFd]);

    return dwError;
    
error:

    goto cleanup;
}

void
LWFreeProcInfo(
    PPROCINFO pProcInfo
    )
{
    LONG status = 0;

    if(pProcInfo == NULL)
        return;

    if (pProcInfo->pid != 0) {
        LWGetExitStatus(pProcInfo, &status);
    }

    if (pProcInfo->fdin >= 0)
        close(pProcInfo->fdin);
    if (pProcInfo->fdout >= 0)
        close(pProcInfo->fdout);
    if (pProcInfo->fderr >= 0)
        close(pProcInfo->fderr);

    LWFreeMemory(pProcInfo);
}

DWORD
LWGetExitStatus(
    PPROCINFO pProcInfo,
    PLONG plstatus
    )
{
    DWORD dwError = 0;
    int status = 0;

    while(1)
    {
        if (waitpid(pProcInfo->pid, &status, 0) < 0) {
            if (errno == EINTR)
                continue;
            dwError = errno;
            BAIL_ON_LWUTIL_ERROR(dwError);
        }
        else
            break;
    }

    if (WIFEXITED(status)) {
        *plstatus = WEXITSTATUS(status);
    } else {
        dwError = LWUTIL_ERROR_NO_PROC_STATUS;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

cleanup:

    /* Mark this PROCINFO as unused */
    pProcInfo->pid = 0;

    return dwError;
    
error:

    goto cleanup;
}

