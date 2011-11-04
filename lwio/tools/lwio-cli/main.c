/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        LWIO client interface program
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
DWORD
ParseArgs(
    int      argc,
    char*    argv[],
    PBOOLEAN pbGetStats,
    PBOOLEAN pbResetStats
    );

static
VOID
ShowUsage(
    VOID
    );

int
main(
    int   argc,
    char* argv[]
    )
{
    DWORD   dwError = 0;
    BOOLEAN bShowStats = FALSE;
    BOOLEAN bResetStats = FALSE;

    dwError = ParseArgs(argc, argv, &bShowStats, &bResetStats);
    BAIL_ON_LWIO_ERROR(dwError);

    if (bShowStats || bResetStats)
    {
        dwError = ProcessServerStats(bShowStats, bResetStats);
        BAIL_ON_LWIO_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ParseArgs(
    int      argc,
    char*    argv[],
    PBOOLEAN pbGetStats,
    PBOOLEAN pbResetStats
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    typedef enum
    {
        PARSE_MODE_OPEN = 0
    } ParseMode;

    int       iArg       = 1;
    ParseMode parseMode  = PARSE_MODE_OPEN;
    BOOLEAN   bGetStats   = FALSE;
    BOOLEAN   bResetStats = FALSE;

    for (iArg = 1; iArg < argc; iArg++)
    {
        PSTR pszArg = argv[iArg];

        switch (parseMode)
        {
            case PARSE_MODE_OPEN:

                if (!strcasecmp(pszArg, "-h") || !strcasecmp(pszArg, "--help"))
                {
                    ShowUsage();
                    exit(0);
                }
                else if (!strcasecmp(pszArg, "--get-stats"))
                {
                    bGetStats = TRUE;
                }
                else if (!strcasecmp(pszArg, "--reset-stats"))
                {
                    bResetStats = TRUE;
                }
                else
                {
                    ShowUsage();
                    fprintf(stderr, "Error: Invalid option \n");
                    exit(1);
                }
                break;

            default:

                ShowUsage();
                exit(0);

                break;
        }
    }

    *pbGetStats = bGetStats;
    *pbResetStats = bResetStats;

    return ntStatus;
}

static
VOID
ShowUsage(
    VOID
    )
{
    printf("Usage: lwio-cli <arguments> ...\n\n"
           "Arguments:\n"
           "    --get-stats \n"
           "    --reset-stats \n");
}



