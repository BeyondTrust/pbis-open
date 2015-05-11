/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
