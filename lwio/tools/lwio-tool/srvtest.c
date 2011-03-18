/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        srvtest.c
 *
 * Abstract:
 *
 *        LW IO Tool SRV Test Module
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "lwio-tool.h"
#include <lwio/srvtransportapi.h>

NTSTATUS
SrvTestMain(
    IN OUT PLW_PARSE_ARGS pParseArgs,
    OUT PSTR* ppszUsageError
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PSTR pszUsageError = NULL;
    PCSTR pszCommand = NULL;
    PCSTR pszSubCommand = NULL;

    pszCommand = LwParseArgsNext(pParseArgs);
    if (!pszCommand)
    {
        status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Missing command.\n");
        assert(!status && pszUsageError);
        GOTO_CLEANUP_EE(EE);
    }

    if (!strcmp(pszCommand, "transport"))
    {
        pszSubCommand = LwParseArgsNext(pParseArgs);
        if (!pszSubCommand)
        {
            status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Missing sub-command.\n");
            assert(!status && pszUsageError);
            GOTO_CLEANUP_EE(EE);
        }

        if (!strcmp(pszSubCommand, "query"))
        {
            BOOLEAN isStarted = FALSE;

            if (LwParseArgsNext(pParseArgs))
            {
                status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Too many arguments.\n");
                assert(!status && pszUsageError);
                GOTO_CLEANUP_EE(EE);
            }

            printf("Querying SRV transport.\n");

            status = LwIoSrvTransportQuery(&isStarted);
            GOTO_CLEANUP_ON_STATUS_EE(status, EE);

            printf("SRV transport is %s.\n", isStarted ? "started" : "stopped");
        }
        else if (!strcmp(pszSubCommand, "start"))
        {
            if (LwParseArgsNext(pParseArgs))
            {
                status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Too many arguments.\n");
                assert(!status && pszUsageError);
                GOTO_CLEANUP_EE(EE);
            }

            printf("Starting SRV transport.\n");

            status = LwIoSrvTransportStart();
            GOTO_CLEANUP_ON_STATUS_EE(status, EE);

            printf("SRV transport started.\n");
        }
        else if (!strcmp(pszSubCommand, "stop"))
        {
            BOOLEAN isForce = FALSE;
            PCSTR pszOption = LwParseArgsNext(pParseArgs);
            if (pszOption)
            {
                if (!strcmp(pszOption, "-f") ||
                    !strcmp(pszOption, "--force"))
                {
                    isForce = TRUE;
                }
            }

            if (LwParseArgsNext(pParseArgs))
            {
                status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Too many arguments.\n");
                assert(!status && pszUsageError);
                GOTO_CLEANUP_EE(EE);
            }

            printf("Stopping SRV transport.\n");

            status = LwIoSrvTransportStop(isForce);
            GOTO_CLEANUP_ON_STATUS_EE(status, EE);

            printf("SRV transport stopped.\n");
        }
        else
        {
            status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Invalid command '%s'\n", pszSubCommand);
            assert(!status);
            GOTO_CLEANUP_EE(EE);
        }
    }
    else
    {
        status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Invalid command '%s'\n", pszCommand);
        assert(!status);
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (status)
    {
        printf("An error occurred: status = 0x%08x (%s)\n", status, LwNtStatusToName(status));
    }

    if (pszUsageError)
    {
        status = STATUS_INVALID_PARAMETER;
    }

    *ppszUsageError = pszUsageError;

    LOG_LEAVE_IF_STATUS_EE(status, EE);

    return status;
}
