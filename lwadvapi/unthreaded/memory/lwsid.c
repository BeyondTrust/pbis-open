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
 *        lwsid.c
 *
 * Abstract:
 *
 *        SID handling functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


DWORD
LwAllocateWellKnownSid(
    IN  WELL_KNOWN_SID_TYPE  WellKnownSidType,
    IN  OPTIONAL PSID        pDomainOrComputerSid,
    OUT PSID                *ppSid,
    OUT OPTIONAL PDWORD      pdwSidSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwSidSize = 0;
    PSID pSid = NULL;
    DWORD dwMaxSidSize = RtlLengthRequiredSid(SID_MAX_SUB_AUTHORITIES);

    if (pDomainOrComputerSid)
    {
        /*
         * If there's domain or computer SID passed it is
         * likely the caller just wants to append a RID
         */
        dwSidSize = RtlLengthSid(pDomainOrComputerSid);
        dwSidSize += sizeof(pSid->SubAuthority[0]);
    }

    do
    {
        dwError = LwReallocMemory(pSid,
                                  OUT_PPVOID(&pSid),
                                  dwSidSize);
        BAIL_ON_LW_ERROR(dwError);

        ntStatus = RtlCreateWellKnownSid(WellKnownSidType,
                                         pDomainOrComputerSid,
                                         pSid,
                                         &dwSidSize);
        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_BUFFER_TOO_SMALL)
        {
            goto error;
        }
    }
    while (ntStatus == STATUS_BUFFER_TOO_SMALL &&
           dwSidSize < dwMaxSidSize);

    if (pdwSidSize)
    {
        *pdwSidSize = dwSidSize;
    }

    *ppSid      = pSid;

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pSid);

    if (pdwSidSize)
    {
        *pdwSidSize = 0;
    }

    *ppSid = NULL;
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
