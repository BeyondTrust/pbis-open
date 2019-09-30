/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        dsr_memeory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        DsSetup memory allocation manager
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
DsrSrvAllocateMemory(
    void **ppOut,
    DWORD dwSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    void *pOut = NULL;

    pOut = rpc_ss_allocate(dwSize);
    BAIL_ON_NO_MEMORY(pOut);

    memset(pOut, 0, dwSize);

    *ppOut = pOut;

cleanup:
    return ntStatus;

error:
    *ppOut = NULL;
    goto cleanup;
}


void
DsrSrvFreeMemory(
    void *pPtr
    )
{
    rpc_ss_free(pPtr);
}


NTSTATUS
DsrSrvAllocateSidFromWC16String(
    PSID *ppSid,
    PCWSTR pwszSidStr
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pSid = NULL;
    ULONG ulSidSize = 0;
    PSID pSidCopy = NULL;

    ntStatus = RtlAllocateSidFromWC16String(&pSid,
                                            pwszSidStr);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ulSidSize = RtlLengthSid(pSid);
    ntStatus = DsrSrvAllocateMemory((void**)&pSidCopy,
                                    ulSidSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlCopySid(ulSidSize, pSidCopy, pSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppSid = pSidCopy;

cleanup:
    RTL_FREE(&pSid);

    return ntStatus;

error:
    if (pSidCopy)
    {
        DsrSrvFreeMemory(pSidCopy);
    }

    *ppSid = NULL;
    goto cleanup;
}


NTSTATUS
DsrSrvDuplicateSid(
    PSID *ppSidOut,
    PSID pSidIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pSid = NULL;
    ULONG ulSidSize = 0;

    ulSidSize = RtlLengthSid(pSidIn);
    ntStatus = DsrSrvAllocateMemory((void**)&pSid,
                                    ulSidSize);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlCopySid(ulSidSize, pSid, pSidIn);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppSidOut = pSid;

cleanup:
    return ntStatus;

error:
    if (pSid)
    {
        DsrSrvFreeMemory(pSid);
    }

    *ppSidOut = NULL;
    goto cleanup;
}


NTSTATUS
DsrSrvGetFromUnicodeStringEx(
    PWSTR *ppwszOut,
    UNICODE_STRING *pIn
   )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszStr = NULL;

    ntStatus = DsrSrvAllocateMemory((void**)&pwszStr,
                                    (pIn->MaximumLength) * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    wc16sncpy(pwszStr, pIn->Buffer, (pIn->Length / sizeof(WCHAR)));
    *ppwszOut = pwszStr;

cleanup:
    return ntStatus;

error:
    if (pwszStr)
    {
        DsrSrvFreeMemory(pwszStr);
    }

    *ppwszOut = NULL;
    goto cleanup;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
