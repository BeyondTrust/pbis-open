/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        acceptsecctxt.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        AcceptSecurityContext client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "client.h"

DWORD
NtlmClientAcceptSecurityContext(
    IN PNTLM_CRED_HANDLE phCredential,
    IN OUT PNTLM_CONTEXT_HANDLE phContext,
    IN PSecBufferDesc pInput,
    IN DWORD fContextReq,
    IN DWORD TargetDataRep,
    IN OUT PNTLM_CONTEXT_HANDLE phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT PDWORD  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    TimeStamp tsTimeStamp = 0;
    DWORD fContextAttr = 0;

    BAIL_ON_INVALID_POINTER(phContext);
    BAIL_ON_INVALID_POINTER(phNewContext);

    if(ptsTimeStamp)
    {
        memset(ptsTimeStamp, 0, sizeof(TimeStamp));
    }

    if(pfContextAttr)
    {
        *pfContextAttr = 0;
    }

    dwError = NtlmTransactAcceptSecurityContext(
        phCredential ? *phCredential : NULL,
        *phContext,
        pInput,
        fContextReq,
        TargetDataRep,
        phNewContext,
        pOutput,
        &fContextAttr,
        &tsTimeStamp);

    if (dwError != LW_WARNING_CONTINUE_NEEDED)
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if(ptsTimeStamp)
    {
        *ptsTimeStamp = tsTimeStamp;
    }
    if(pfContextAttr)
    {
        *pfContextAttr = fContextAttr;
    }

    return(dwError);
error:
    // we may not want to clear the IN OUT params on error
    if(phContext)
    {
        *phContext = NULL;
    }
    if(phNewContext)
    {
        *phNewContext = NULL;
    }
    if(pOutput)
    {
        memset(pOutput, 0, sizeof(SecBufferDesc));
    }

    goto cleanup;
}
