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
 *        Likewise Task Client
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

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
    DWORD dwError = 0;

    if (argc < 2)
    {
        dwError = ERROR_BAD_ARGUMENTS;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    if (!strcmp(argv[1], "log"))
    {
        dwError = LwTaskHandleLogRequest(argc-2, &argv[2]);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }
    else if (!strcmp(argv[1], "task"))
    {
        dwError = LwTaskHandleTaskRequest(argc-2, &argv[2]);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }
    else
    {
        dwError = ERROR_BAD_ARGUMENTS;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    switch (dwError)
    {
        case ERROR_BAD_ARGUMENTS:

            ShowUsage();

            break;

        default:

            fprintf(stderr, "Failed to execute command (error: %u)\n", dwError);

            break;
    }

    goto cleanup;
}

static
VOID
ShowUsage(
    VOID
    )
{
    printf("Usage: lwtask-cli {log, task} <arguments>\n");
    printf("Arguments:\n");
    printf("log  get-info \n"
    	   "     set-level {warning, error, info, verbose, debug}\n\n"
           "task list   <task type>\n"
           "     add    <task type> <arguments>\n"
    	   "     del    <task id>\n"
    	   "     exec   <task id> <arguments>\n"
    	   "     schema <task type>\n"
    	   "     status <task id>\n\n"
    	   "Available task types: {migrate-share}\n");
}

