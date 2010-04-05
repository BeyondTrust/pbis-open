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
NetrShareAdd(
    IN  PNFSSVC_CONTEXT pContext,
    IN  PCWSTR   pwszServername,
    IN  DWORD    dwLevel,
    IN  PBYTE    pBuffer,
    OUT PDWORD   pdwParmErr
    )
{
    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    nfssvc_NetShareInfo Info;
    DWORD dwParmErr = 0;
    PWSTR pwszServer = NULL;

    BAIL_ON_INVALID_PTR(pContext, ntStatus);
    BAIL_ON_INVALID_PTR(pContext->hBinding, ntStatus);

    if (pwszServername)
    {
        err = LwAllocateWc16String(&pwszServer, pwszServername);
        BAIL_ON_WIN_ERROR(err);
    }

    memset(&Info, 0, sizeof(Info));

    switch (dwLevel) {
    case 2:
        Info.info2 = (PSHARE_INFO_2)pBuffer;
        break;

    case 502:
        Info.info502 = (PSHARE_INFO_502)pBuffer;
        break;

    default:
        err = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(err);
        break;
    }

    DCERPC_CALL(err, _NetrShareAdd(pContext->hBinding,
                                   pwszServer,
                                   dwLevel,
                                   Info,
                                   &dwParmErr));
    BAIL_ON_WIN_ERROR(err);

    if (pdwParmErr)
    {
        *pdwParmErr = dwParmErr;
    }

cleanup:
    NFSSVC_SAFE_FREE(pwszServer);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;

error:
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
