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
 *        Likewise Task Service (LWTASK)
 *
 *        Share Migration Test Driver
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include <config.h>
#include <lwtasksystem.h>

#include <lwdef.h>
#include <lwerror.h>

#include <lwtaskdef.h>
#include <lwtaskutils.h>
#include <lwtasklog_r.h>

#include <lwmigrate.h>

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
    BOOLEAN bShutdown = FALSE;
    PLW_SHARE_MIGRATION_CONTEXT pContext = NULL;

    if (argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")))
    {
        ShowUsage();
        goto cleanup;
    }

    if (argc < 5)
    {
        dwError = ERROR_BAD_ARGUMENTS;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    dwError = LwTaskMigrateInit();
    BAIL_ON_LW_TASK_ERROR(dwError);

    bShutdown = TRUE;

    dwError = LwTaskMigrateCreateContext(
                    argv[3], /* User name   */
                    argv[4], /* Password    */
                    &pContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskMigrateShareA(
                    pContext,
                    argv[1], /* Server name */
                    argv[2], /* Share name  */
                    0        /* Flags       */
                    );
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    if (pContext)
    {
        LwTaskMigrateCloseContext(pContext);
    }

    if (bShutdown)
    {
        LwTaskMigrateShutdown();
    }

    return dwError;

error:

    switch (dwError)
    {
        case ERROR_BAD_ARGUMENTS:

            ShowUsage();

            break;

        default:

            fprintf(stderr, "Error migrating share. Code %u\n", dwError);

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
    fprintf(stderr,
            "Usage: lwmigrate <server> <share> <user> <password>\n");
}
