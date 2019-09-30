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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        ipcio.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Inter-process communication API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */
#include "ipc.h"

DWORD
RegWriteData(
    DWORD dwFd,
    PSTR pszBuf,
    DWORD dwLen
    )
{
    DWORD dwError = 0;
    DWORD dwRemaining = dwLen;
    PSTR  pStr = pszBuf;

    while (dwRemaining > 0) {

        int nWritten = write(dwFd, pStr, dwRemaining);

        if (nWritten < 0)
        {
            if (errno != EAGAIN && errno != EINTR)
            {
                dwError = errno;
                BAIL_ON_REG_ERROR(dwError);
            }
        }
        else
        {
            dwRemaining -= nWritten;
            pStr += nWritten;
        }
    }

error:

    return (dwError);
}

DWORD
RegReadData(
    DWORD  dwFd,
    PSTR   pszBuf,
    DWORD  dwBytesToRead,
    PDWORD pdwBytesRead
    )
{
    DWORD dwError = 0;
    DWORD dwTotalBytesRead = 0;
    DWORD dwBytesLeftToRead = 0;
    int maxfd;
    fd_set read_fd_set;
    int select_status;
    short bConnectionClosed = 0;

    struct timeval timeout;

    PSTR pCurPos = pszBuf;

    dwBytesLeftToRead = dwBytesToRead;

    FD_ZERO(&read_fd_set);
    while (!bConnectionClosed && (dwTotalBytesRead < dwBytesToRead)) {

          FD_SET(dwFd, &read_fd_set);

          maxfd = dwFd + 1;

          timeout.tv_sec = 5;
          timeout.tv_usec = 0;

          select_status = select(maxfd,
                                 &read_fd_set,
                                 NULL /* write_fds */,
                                 NULL /* except_fds */,
                                 &timeout);
          if (select_status < 0) {

             dwError = errno;
             BAIL_ON_REG_ERROR(dwError);

          } else if (select_status == 0) {

            /* timed out */

          } else {

             if (FD_ISSET(dwFd, &read_fd_set)) {

                int nBytesRead = read(dwFd, pCurPos, dwBytesLeftToRead);
                if (nBytesRead < 0) {

                   if (errno != EAGAIN && errno != EINTR) {
                      dwError = errno;
                      BAIL_ON_REG_ERROR(dwError);
                   }

                } else if (nBytesRead == 0) {

                  bConnectionClosed = 1;
                  dwError = ENOENT;

                } else {

                  dwTotalBytesRead += nBytesRead;
                  pCurPos += nBytesRead;
                  dwBytesLeftToRead -= nBytesRead;
                }

             } else {

               bConnectionClosed = 1;
               dwError = ENOENT;

             }
         }
    }

  error:

    *pdwBytesRead = dwTotalBytesRead;

    return (dwError);
}


