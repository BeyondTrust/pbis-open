#include "includes.h"

CENTERROR
WriteData(
    DWORD dwFd,
    PSTR pszBuf,
    DWORD dwLen
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD dwRemaining = dwLen;
    PSTR pStr = pszBuf;

    while (dwRemaining > 0) {

        int nWritten = write(dwFd, pStr, dwRemaining);
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

CENTERROR
ReadData(
    DWORD  dwFd,
    PSTR   pszBuf,
    DWORD  dwBytesToRead,
    PDWORD pdwBytesRead
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD dwTotalBytesRead = 0;
    DWORD dwBytesLeftToRead = 0;
    int maxfd;
    fd_set read_fd_set;
    int select_status;
    //short bError = 0;
    short bConnectionClosed = 0;
    PSTR pCurPos = pszBuf;
    struct timeval timeout;

    dwBytesLeftToRead = dwBytesToRead;

    while (!bConnectionClosed && (dwTotalBytesRead < dwBytesToRead)) {

        FD_ZERO(&read_fd_set);
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

            if (errno == EINTR)
                continue;

            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);

        } else if (select_status == 0) {

            /* timed out */

        } else {

            if (FD_ISSET(dwFd, &read_fd_set)) {

                int nBytesRead = read(dwFd, pCurPos, dwBytesLeftToRead);
                if (nBytesRead < 0) {

                    if (errno != EAGAIN && errno != EINTR) {
                        ceError = LwMapErrnoToLwError(errno);
                        BAIL_ON_CENTERIS_ERROR(ceError);
                    }

                } else if (nBytesRead == 0) {

                    bConnectionClosed = 1;
                    ceError = CENTERROR_CONNECTION_UNAVAIL;

                } else {

                    dwTotalBytesRead += nBytesRead;
                    pCurPos += nBytesRead;
                    dwBytesLeftToRead -= nBytesRead;
                }

            } else {

                bConnectionClosed = 1;
                ceError = CENTERROR_CONNECTION_UNAVAIL;

            }
        }
    }

error:

    *pdwBytesRead = dwTotalBytesRead;

    return (ceError);
}

