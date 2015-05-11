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
 * Copyright (C) Likewise Software. All rights reserved.
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


