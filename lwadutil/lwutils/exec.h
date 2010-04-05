/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#ifndef __CT_EXEC_H__
#define __CT_EXEC_H__

/**
 * @defgroup LWExec Unix program interaction
 */
/*@{*/

typedef struct __PROCINFO
{
    pid_t pid;
    int   fdin;
    int   fdout;
    int   fderr;
} PROCINFO, *PPROCINFO;

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
 * @see LWEscapeString
 * @param command @in the Unix command to execute
 * @param output @out the dynamically-allocated buffer containing
 * the output of the command
 * @errcode
 * @canfail
 */
DWORD
LWCaptureOutput(
    PCSTR command,
    PSTR* output
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
 * @see LWEscapeString
 * @param command @in the Unix command to execute
 * @param captureStderr @in whether to capture stderr with stdout or to let it output to the caller's stderr.
 * @param output @out the dynamically-allocated buffer containing
 * the output of the command
 * @errcode
 * @canfail
 */
DWORD
LWCaptureOutputWithStderr(
    PCSTR command,
    BOOLEAN captureStderr,
    PSTR* output
    );

DWORD
LWCaptureOutputWithStderrEx(
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
 * This command returns eIPCSendError if the program returns a non-
 * zero exit code.
 *
 * @see LWEscapeString
 * @param command @in the Unix command to execute
 * @errcode
 * @canfail
 */
DWORD
LWRunCommand(
    PCSTR command
    );

DWORD
LWSpawnProcessWithFds(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    );

DWORD
LWSpawnProcessWithEnvironment(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    const PSTR* ppszEnv,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    );

VOID
LWFreeProcInfo(
    PPROCINFO pProcInfo
    );

DWORD
LWGetExitStatus(
    PPROCINFO pProcInfo,
    PLONG plstatus
    );
    
/*@}*/

#endif
