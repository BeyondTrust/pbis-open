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
 *        Tool to refresh SMB Configuration
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lwiosys.h"
#include "lwio/lwio.h"
#include "lwiodef.h"
#include "lwioutils.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

VOID
ParseArgs(
    int    argc,
    char*  argv[]
    );

VOID
ShowUsage();

DWORD
MapErrorCode(
    DWORD dwError
    );

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PIO_CONTEXT pContext = NULL;

    if (geteuid() != 0) {
        fprintf(stderr, "This program requires super-user privileges.\n");
        dwError = EACCES;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    ParseArgs(argc, argv);

    dwError = LwIoOpenContext(&pContext);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = LwIoRefreshConfiguration((HANDLE) pContext);
    BAIL_ON_LWIO_ERROR(dwError);

cleanup:

    if (pContext != NULL)
    {
        LwIoCloseContext(pContext);
    }

    LwIoShutdown();

    return (dwError);

error:

    dwError = MapErrorCode(dwError);

    fprintf(
        stderr,
        "Failed to query status from SMB service.  Error code %u (%s).\n%s\n",
        dwError,
        LW_PRINTF_STRING(LwNtStatusToName(dwError)),
        LW_PRINTF_STRING(LwNtStatusToDescription(dwError)));

    goto cleanup;
}

VOID
ParseArgs(
    int    argc,
    char*  argv[]
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0
    } ParseMode;

    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }

        switch (parseMode)
        {
            case PARSE_MODE_OPEN:

                if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
                {
                    ShowUsage();
                    exit(0);
                }
                else
                {
                    ShowUsage();
                    exit(1);
                }

                break;
        }

    } while (iArg < argc);
}

void
ShowUsage()
{
    printf("Usage: lwio-refresh\n");
}

DWORD
MapErrorCode(
    DWORD dwError
    )
{
    DWORD dwError2 = dwError;

    switch (dwError)
    {
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:

            dwError2 = LWIO_ERROR_SERVER_UNREACHABLE;

            break;

        default:

            break;
    }

    return dwError2;
}
