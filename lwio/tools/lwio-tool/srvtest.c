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
