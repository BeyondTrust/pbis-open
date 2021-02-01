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
