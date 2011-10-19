/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Test Program for exercising SMB Client API
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lsmbsys.h"
#include "lwio/lwio.h"
#include "smbdef.h"
#include "smbutils.h"

static
int
ParseArgs(
    int argc,
    char* argv[],
    PSTR* ppszPipename
    );

static
void
ShowUsage(
    VOID
    );

#define BUFFER_SIZE 512

#define LWIO_ERROR_PIPE_BUSY 231L
#define LWIO_ERROR_MORE_DATA 234L

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSTR pszShare = NULL;
    PSTR pszPipename = NULL;
    HANDLE hPipe = (HANDLE)NULL;
    BOOLEAN bSuccess = FALSE;
    wchar16_t wszPipe[1024];
    wchar16_t wszClientMessage[256];
    wchar16_t wszBuf[BUFFER_SIZE+1];
    PCSTR pszDefaultClientMessage = "Message from LWIO Client";
    DWORD cbRead = 0;
    DWORD dwMode = 0;

    dwError = ParseArgs(argc, argv, &pszPipename);
    BAIL_ON_LWIO_ERROR(dwError);

    if (!mbstowc16s(&wszPipe[0],
                    pszPipename,
                    sizeof(wszPipe)))
    {
        dwError = LWIO_ERROR_INTERNAL;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    if (!mbstowc16s(&wszClientMessage[0],
                    pszDefaultClientMessage,
                    sizeof(wszClientMessage)))
    {
        dwError = LWIO_ERROR_INTERNAL;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    while(1)
    {
        hPipe = CreateFile(
                    wszPipe, // pipe name
                    (GENERIC_READ | GENERIC_WRITE), // access
                    0,        // don't share
                    NULL,     // default security attributes
                    OPEN_EXISTING, // don't create
                    0, // default attributes
                    NULL); // no template file

        if (hPipe != (HANDLE)NULL)
        {
            break;
        }

        if (GetLastError() == LWIO_ERROR_PIPE_BUSY)
        {
            printf("Failed to open pipe\n");
            goto cleanup;
        }

        if (!WaitNamedPipe(wszPipe, 20000))
        {
            printf("Timed out waiting for pipe\n");
            goto cleanup;
        }
    }

    // After the pipe connected, switch to message read mode
    dwMode = PIPE_READMODE_MESSAGE;
    /*bSuccess = SetNamedPipeHandleState(
                    hPipe,
                    &dwMode,
                    NULL,
                    NULL);
    if (!bSuccess)
    {
        printf("SetNamedPipeHandleState failed\n");

        dwError = GetLastError();
        BAIL_ON_LWIO_ERROR(dwError);
    }
    */

    bSuccess = TransactNamedPipe(
                    hPipe,
                    wszClientMessage,
                    wc16slen(wszClientMessage) * sizeof(wchar16_t) + sizeof(WNUL),
                    wszBuf,
                    BUFFER_SIZE * sizeof(wchar16_t),
                    &cbRead,
                    NULL);
    if (!bSuccess && GetLastError() != LWIO_ERROR_MORE_DATA)
    {
        printf("TransactNamedPipe failed\n");
        dwError = GetLastError();
        BAIL_ON_LWIO_ERROR(dwError);
    }

    do
    {
        memset(wszBuf, 0, sizeof(wszBuf));

        bSuccess = ReadFile(
                        hPipe,
                        wszBuf,
                        BUFFER_SIZE * sizeof(wchar16_t),
                        &cbRead,
                        NULL);
        if (!bSuccess && GetLastError() != LWIO_ERROR_MORE_DATA)
        {
            break;
        }

        // print the data
        if (cbRead)
        {
            assert(cbRead <= BUFFER_SIZE);

            printfw16("%S\n", wszBuf);
        }

    } while (!bSuccess);

cleanup:

    if (hPipe != (HANDLE)NULL)
    {
        CloseHandle(hPipe);
    }

    LWIO_SAFE_FREE_STRING(pszShare);
    LWIO_SAFE_FREE_STRING(pszPipename);

    return (dwError);

error:

    goto cleanup;
}


static
int
ParseArgs(
    int argc,
    char* argv[],
    PSTR* ppszPipename
    )
{
    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR  pszPipename = NULL;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }

        if ((strcmp(pszArg, "--help") == 0) || (strcmp(pszArg, "-h") == 0))
        {
            ShowUsage();
            exit(0);
        }
        else
        {
            LWIO_SAFE_FREE_STRING(pszPipename);

            dwError = SMBAllocateString(
                            pszArg,
                            &pszPipename);
            BAIL_ON_LWIO_ERROR(dwError);
        }

    } while (iArg < argc);

    if (IsNullOrEmptyString(pszPipename))
    {
        fprintf(stderr, "\nPlease specify a valid pipe name\n\n");
        ShowUsage();
        exit(1);
    }

    *ppszPipename = pszPipename;

cleanup:

    return dwError;

error:

    *ppszPipename = NULL;

    LWIO_SAFE_FREE_STRING(pszPipename);

    goto cleanup;
}

static
void
ShowUsage(
    VOID
    )
{
    printf("Usage: test-npclient_transactnp <pipe name>\n");
    printf("Example: test-npclient_transactnp \\\\testsrv.centeris.com\\pipe\\testpipe\n");
}

