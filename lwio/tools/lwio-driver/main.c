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
 *        LWIO client utility program
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 */

#include "config.h"
#include <lwio/lwio.h>
#include <stdio.h>
#include "lwioutils.h"

static
VOID
Usage(
    IN PCSTR pszProgramName
    )
{
    printf("Usage: %s [options] <command> arg1 ...\n\n"
           "Commands:\n\n"
           "    status <driver>\n"
           "    load <driver>\n"
           "    unload <driver>\n",
           pszProgramName);
}

static
NTSTATUS
Status(
    int argc,
    PSTR* ppszArgv
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszDriverName = NULL;
    LWIO_DRIVER_STATE driverState = 0;

    status = LwRtlWC16StringAllocateFromCString(&pwszDriverName, ppszArgv[1]);
    BAIL_ON_NT_STATUS(status);
    
    status = LwIoQueryStateDriver(pwszDriverName, &driverState);
    BAIL_ON_NT_STATUS(status);

    switch (driverState)
    {
    case LWIO_DRIVER_STATE_UNLOADED:
        printf("Unloaded\n");
        break;
    case LWIO_DRIVER_STATE_LOADED:
        printf("Loaded\n");
        break;
    default:
        printf("Unknown\n");
        break;
    }

cleanup:

    LWIO_SAFE_FREE_MEMORY(pwszDriverName);

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
Load(
    PCSTR pszDriverName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszDriverName = NULL;

    status = LwRtlWC16StringAllocateFromCString(&pwszDriverName, pszDriverName);
    BAIL_ON_NT_STATUS(status);
    
    status = LwIoLoadDriver(pwszDriverName);

    if (status)
    {
        printf("Could not load driver: %s\n", LwNtStatusToName(status));
    }
    else
    {
        printf("Driver [%s] loaded successfully\n", pszDriverName);
    }
    BAIL_ON_NT_STATUS(status);

cleanup:

    LWIO_SAFE_FREE_MEMORY(pwszDriverName);

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
Unload(
    PCSTR pszDriverName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszDriverName = NULL;

    status = LwRtlWC16StringAllocateFromCString(&pwszDriverName, pszDriverName);
    BAIL_ON_NT_STATUS(status);
    
    status = LwIoUnloadDriver(pwszDriverName);

    if (status)
    {
        printf("Could not unload driver: %s\n", LwNtStatusToName(status));
    }
    else
    {
        printf("Driver [%s] unloaded successfully\n", pszDriverName);
    }
    BAIL_ON_NT_STATUS(status);

cleanup:

    LWIO_SAFE_FREE_MEMORY(pwszDriverName);

    return status;

error:

    goto cleanup;
}

int
main(
    int argc,
    char** ppszArgv
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (argc >= 2)
    {
        if (LwRtlCStringIsEqual(ppszArgv[1], "status", TRUE))
        {
            status = Status(argc - 1, ppszArgv + 1);
            BAIL_ON_NT_STATUS(status);
        }
        else if (LwRtlCStringIsEqual(ppszArgv[1], "load", TRUE))
        {
            int iArg = 2;

            for (; iArg < argc; iArg++)
            {
                status = Load(ppszArgv[iArg]);
                BAIL_ON_NT_STATUS(status);
            }
        }
        else if (LwRtlCStringIsEqual(ppszArgv[1], "unload", TRUE))
        {
            int iArg = 2;

            for (; iArg < argc; iArg++)
            {
                status = Unload(ppszArgv[iArg]);
                BAIL_ON_NT_STATUS(status);
            }
        }
        else
        {
            Usage(ppszArgv[0]);
        }
    }
    else
    {
        Usage(ppszArgv[0]);
    }

cleanup:

    return status ? 1 : 0;

error:

    goto cleanup;
}
