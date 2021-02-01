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



