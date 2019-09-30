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
 *        acceptsecctxt.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
