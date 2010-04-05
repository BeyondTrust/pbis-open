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

#include "includes.h"


NET_API_STATUS
NetrShareEnum(
    IN  PNFSSVC_CONTEXT pContext,
    IN  PCWSTR   pwszServername,
    IN  DWORD    dwLevel,
    OUT PBYTE   *ppBuffer,
    IN  DWORD    dwMaxLen,
    OUT PDWORD   pdwNumEntries,
    OUT PDWORD   pdwTotalEntries,
    OUT PDWORD   pdwResume
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    NET_API_STATUS EnumStatus = ERROR_SUCCESS;
    nfssvc_NetShareCtr ctr;
    nfssvc_NetShareCtr0 ctr0;
    nfssvc_NetShareCtr1 ctr1;
    nfssvc_NetShareCtr2 ctr2;
    nfssvc_NetShareCtr501 ctr501;
    nfssvc_NetShareCtr502 ctr502;
    PWSTR pwszServer = NULL;
    PBYTE pBuffer = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwReturnedLevel = dwLevel;

    BAIL_ON_INVALID_PTR(pContext, status);
    BAIL_ON_INVALID_PTR(pContext->hBinding, status);
    BAIL_ON_INVALID_PTR(ppBuffer, status);
    BAIL_ON_INVALID_PTR(pdwNumEntries, status);
    BAIL_ON_INVALID_PTR(pdwTotalEntries, status);

    memset(&ctr, 0, sizeof(ctr));
    memset(&ctr0, 0, sizeof(ctr0));
    memset(&ctr1, 0, sizeof(ctr1));
    memset(&ctr2, 0, sizeof(ctr2));
    memset(&ctr501, 0, sizeof(ctr501));
    memset(&ctr502, 0, sizeof(ctr502));

    if (pwszServername)
    {
        status = LwAllocateWc16String(&pwszServer,
                                      pwszServername);
        BAIL_ON_WIN_ERROR(status);
    }

    switch (dwLevel) {
    case 0:
        ctr.ctr0 = &ctr0;
        break;

    case 1:
        ctr.ctr1 = &ctr1;
        break;

    case 2:
        ctr.ctr2 = &ctr2;
        break;

    case 501:
        ctr.ctr501 = &ctr501;
        break;

    case 502:
        ctr.ctr502 = &ctr502;
        break;
    }

    DCERPC_CALL(status,
                _NetrShareEnum(pContext->hBinding,
                               pwszServer,
                               &dwReturnedLevel,
                               &ctr,
                               dwMaxLen,
                               &dwTotalEntries,
                               pdwResume));

    /* Preserve returned status code */
    EnumStatus = status;

    if (dwReturnedLevel != dwLevel)
    {
        status = ERROR_BAD_NET_RESP;
    }

    if (status != ERROR_SUCCESS &&
        status != ERROR_MORE_DATA)
    {
        BAIL_ON_WIN_ERROR(status);
    }

    status = NfsSvcCopyNetShareCtr(dwLevel,
                                   &ctr,
                                   &dwNumEntries,
                                   &pBuffer);
    BAIL_ON_WIN_ERROR(status);

    *pdwNumEntries   = dwNumEntries;
    *pdwTotalEntries = dwTotalEntries;
    *ppBuffer        = pBuffer;

cleanup:
    NfsSvcClearNetShareCtr(dwLevel, &ctr);

    if (status == ERROR_SUCCESS &&
        EnumStatus != ERROR_SUCCESS)
    {
        status = EnumStatus;
    }

    return status;

error:
    *pdwNumEntries   = 0;
    *pdwTotalEntries = 0;
    *pdwResume       = 0;
    *ppBuffer        = NULL;

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
