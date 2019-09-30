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
 *        BeyondTrust Task Client
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

