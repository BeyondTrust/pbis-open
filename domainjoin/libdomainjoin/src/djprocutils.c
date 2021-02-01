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

#include "domainjoin.h"

void
WaitTimeout(int val)
{
}

void
FreeProcInfo(
    PPROCINFO pProcInfo
    )
{
    CTFreeProcInfo(pProcInfo);
}

DWORD
DJSpawnProcessWithEnvironment(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    const PSTR* ppszEnv,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    )
{
    return CTSpawnProcessWithEnvironment(
        pszCommand,
        ppszArgs,
        ppszEnv,
        dwFdIn,
        dwFdOut,
        dwFdErr,
        ppProcInfo
        );
}

DWORD
DJSpawnProcess(
    PCSTR pszCommand,
    const PSTR* ppszArgs,
    PPROCINFO* ppProcInfo
    )
{
    return DJSpawnProcessWithEnvironment(pszCommand, ppszArgs, NULL, -1, -1, -1, ppProcInfo);
}

DWORD
DJSpawnProcessWithFds(
    PCSTR pszCommand,
    PSTR* ppszArgs,
    int dwFdIn,
    int dwFdOut,
    int dwFdErr,
    PPROCINFO* ppProcInfo
    )
{
    return CTSpawnProcessWithFds(pszCommand, ppszArgs, dwFdIn, dwFdOut, dwFdErr, ppProcInfo);
}

DWORD
DJSpawnProcessSilent(
    PCSTR pszCommand,
    PSTR* ppArgs,
    PPROCINFO* ppProcInfo
    )
{
    int dwFdIn = -1, dwFdOut = -1, dwFdErr = -1;
    DWORD ceError = ERROR_SUCCESS;

    dwFdIn = open("/dev/zero", O_RDONLY);

    if (dwFdIn < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    dwFdOut = open("/dev/null", O_WRONLY);

    if (dwFdOut < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    dwFdErr = open("/dev/null", O_WRONLY);

    if (dwFdErr < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = DJSpawnProcessWithFds(pszCommand, ppArgs, dwFdIn, dwFdOut, dwFdErr, ppProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if(dwFdIn != -1)
        close(dwFdIn);
    if(dwFdOut != -1)
        close(dwFdOut);
    if(dwFdErr != -1)
        close(dwFdErr);

    return ceError;
}

DWORD
DJSpawnProcessOutputToFile(
    PCSTR pszCommand,
    PSTR* ppArgs,
    PCSTR file,
    PPROCINFO* ppProcInfo
    )
{
    int dwFdIn = -1, dwFdOut = -1, dwFdErr = -1;
    DWORD ceError = ERROR_SUCCESS;

    dwFdIn = open("/dev/zero", O_RDONLY);

    if (dwFdIn < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    dwFdOut = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );

    if (dwFdOut < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    dwFdErr = open("/dev/null", O_WRONLY);

    if (dwFdErr < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = DJSpawnProcessWithFds(pszCommand, ppArgs, dwFdIn, dwFdOut, dwFdErr, ppProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if(dwFdIn != -1)
        close(dwFdIn);
    if(dwFdOut != -1)
        close(dwFdOut);
    if(dwFdErr != -1)
        close(dwFdErr);

    return ceError;
}

DWORD
DJTimedReadData(
    PPROCINFO pProcInfo,
    PPROCBUFFER pProcBuffer,
    DWORD dwTimeoutSecs,
    PBOOLEAN pbTimedout
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR pszBuf = NULL;
    long lBytesRead = 0;
    int maxfd;
    fd_set read_fd_set;
    int select_status;
    int fd = 0;
    int iFd = 0;

    struct timeval timeout;
    timeout.tv_sec = dwTimeoutSecs;
    timeout.tv_usec = 0;

    pProcBuffer->dwOutBytesRead = 0;
    pProcBuffer->dwErrBytesRead = 0;
    pProcBuffer->bEndOfFile = FALSE;

    FD_ZERO(&read_fd_set);
    while (!pProcBuffer->dwOutBytesRead &&
           !pProcBuffer->dwErrBytesRead &&
           !pProcBuffer->bEndOfFile) {

        if (pProcInfo->fdout >= 0)
        {
            FD_SET(pProcInfo->fdout, &read_fd_set);
        }
        if (pProcInfo->fderr >= 0)
        {
            FD_SET(pProcInfo->fderr, &read_fd_set);
        }

        maxfd = (pProcInfo->fdout > pProcInfo->fderr ? pProcInfo->fdout+1 : pProcInfo->fderr + 1);

        select_status = select(maxfd,
                               &read_fd_set,
                               NULL /* write_fds */,
                               NULL /* except_fds */,
                               &timeout);
        if (select_status < 0) {

            if (errno == EINTR)
                continue;

            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);

        } else if (select_status == 0) {

            /* timed out */

        } else {

            for (iFd = 0; iFd < 2; iFd++) {

                fd = (iFd == 0 ? pProcInfo->fdout : pProcInfo->fderr);
                if (fd < 0)
                {
                    continue;
                }
                pszBuf = (iFd == 0 ? pProcBuffer->szOutBuf : pProcBuffer->szErrBuf);

                if (FD_ISSET(fd, &read_fd_set)) {

                    lBytesRead = read(fd, pszBuf, MAX_PROC_BUF_LEN);
                    if (lBytesRead < 0) {

                        if (errno != EAGAIN && errno != EINTR) {
                            ceError = LwMapErrnoToLwError(errno);
                            BAIL_ON_CENTERIS_ERROR(ceError);
                        }

                    } else if (lBytesRead == 0) {

                        pProcBuffer->bEndOfFile = TRUE;

                    } else {

                        if (iFd == 0)
                            pProcBuffer->dwOutBytesRead = lBytesRead;
                        else
                            pProcBuffer->dwErrBytesRead = lBytesRead;

                    }

                }
            }
        }
    }

error:

    return (ceError);
}

DWORD
DJReadData(
    PPROCINFO pProcInfo,
    PPROCBUFFER pProcBuffer
    )
{
    BOOLEAN bTimedout = FALSE;
    return DJTimedReadData(pProcInfo, pProcBuffer, 5, &bTimedout);
}

DWORD
DJWriteData(
    DWORD dwFd,
    PSTR pszBuf,
    DWORD dwLen
    )
{
    DWORD ceError = ERROR_SUCCESS;
    long nWritten = 0;
    DWORD dwRemaining = 0;
    PSTR pStr;

    dwRemaining = dwLen;
    pStr = pszBuf;
    while (dwRemaining > 0) {

        nWritten = write(dwFd, pStr, dwRemaining);
        if (nWritten < 0)
        {
            if (errno != EAGAIN && errno != EINTR)
            {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
        else
        {
            dwRemaining -= nWritten;
            pStr += nWritten;
        }
    }

error:

    return (ceError);
}

DWORD
DJGetProcessStatus(
    PPROCINFO pProcInfo,
    PLONG plstatus
    )
{
    DWORD ceError = ERROR_SUCCESS;
    int status = 0;

    do {
        if (waitpid(pProcInfo->pid, &status, 0) < 0) {
            if (errno == EINTR)
                continue;
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if (WIFEXITED(status)) {
            *plstatus  = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            DJ_LOG_ERROR("Process [%d] killed by signal %d\n", pProcInfo->pid, WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            DJ_LOG_ERROR("Process [%d] stopped by signal %d\n", pProcInfo->pid, WSTOPSIG(status));
        } else {
            DJ_LOG_ERROR("Process [%d] unknown status 0x%x\n", pProcInfo->pid, status);
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

error:

    return ceError;
}
