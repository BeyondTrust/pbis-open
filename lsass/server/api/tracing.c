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
 *        tracing.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Tracing API (Private Header)
 *
 *        Thread Safe
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#include "api.h"

PLSA_BIT_VECTOR gpTraceFlags = NULL;
pthread_mutex_t gTraceLock = PTHREAD_MUTEX_INITIALIZER;

DWORD
LsaTraceInitialize(
    VOID
    )
{
    DWORD dwError = 0;
    PLSA_BIT_VECTOR pTraceVector = NULL;

    dwError = LsaBitVectorCreate(
                    LSA_TRACE_FLAG_SENTINEL,
                    &pTraceVector);
    BAIL_ON_LSA_ERROR(dwError);

    if (gpTraceFlags)
    {
        LsaBitVectorFree(gpTraceFlags);
    }

    gpTraceFlags = pTraceVector;

cleanup:

    return dwError;

error:

    if (pTraceVector)
    {
        LsaBitVectorFree(pTraceVector);
    }

    goto cleanup;
}

BOOLEAN
LsaTraceIsFlagSet(
    DWORD dwTraceFlag
    )
{
    BOOLEAN bResult = FALSE;

    if (gpTraceFlags &&
        dwTraceFlag &&
        LsaBitVectorIsSet(gpTraceFlags, dwTraceFlag))
    {
        bResult = TRUE;
    }

    return bResult;
}

BOOLEAN
LsaTraceIsAllowed(
    DWORD dwTraceFlags[],
    DWORD dwNumFlags
    )
{
    BOOLEAN bResult = FALSE;
    DWORD   iFlag = 0;

    if (gpTraceFlags)
    {
        for (; !bResult && (iFlag < dwNumFlags); iFlag++)
        {
            if (LsaTraceIsFlagSet(dwTraceFlags[iFlag]))
            {
                bResult = TRUE;
            }
        }
    }

    return bResult;
}

DWORD
LsaTraceSetFlag(
    DWORD dwTraceFlag
    )
{
    DWORD dwError = 0;

    if (!gpTraceFlags)
    {
        dwError = LW_ERROR_TRACE_NOT_INITIALIZED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaBitVectorSetBit(
                    gpTraceFlags,
                    dwTraceFlag);

error:

    return dwError;
}

DWORD
LsaTraceUnsetFlag(
    DWORD dwTraceFlag
    )
{
    DWORD dwError = 0;

    if (!gpTraceFlags)
    {
        dwError = LW_ERROR_TRACE_NOT_INITIALIZED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaBitVectorUnsetBit(
                    gpTraceFlags,
                    dwTraceFlag);

error:

    return dwError;
}

VOID
LsaTraceShutdown(
    VOID
    )
{
    if (gpTraceFlags)
    {
        LsaBitVectorFree(gpTraceFlags);
        gpTraceFlags = NULL;
    }
}

DWORD
LsaInitTracing_r(
    VOID
    )
{
    DWORD dwError = 0;

    LSA_LOCK_TRACER;

    dwError = LsaTraceInitialize();

    LSA_UNLOCK_TRACER;

    return dwError;
}

DWORD
LsaTraceSetFlag_r(
    DWORD   dwTraceFlag,
    BOOLEAN bStatus
    )
{
    DWORD dwError = 0;

    LSA_LOCK_TRACER;

    if (bStatus)
    {
        dwError = LsaTraceSetFlag(dwTraceFlag);
    }
    else
    {
        dwError = LsaTraceUnsetFlag(dwTraceFlag);
    }

    LSA_UNLOCK_TRACER;

    return dwError;
}

DWORD
LsaTraceGetInfo_r(
    DWORD    dwTraceFlag,
    PBOOLEAN pbStatus
    )
{
    LSA_LOCK_TRACER;

    *pbStatus = LsaTraceIsFlagSet(dwTraceFlag);

    LSA_UNLOCK_TRACER;

    return 0;
}

VOID
LsaShutdownTracing_r(
    VOID
    )
{
    LsaTraceShutdown();
}

