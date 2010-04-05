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
NetrShareSetInfo(
    IN  PNFSSVC_CONTEXT pContext,
    IN  PCWSTR          pwszServername,
    IN  PCWSTR          pwszNetname,
    IN  DWORD           dwLevel,
    IN  PBYTE           pBuffer,
    OUT PDWORD          pdwParmErr
    )
{
    NET_API_STATUS err = ERROR_SUCCESS;
    PWSTR pwszServer = NULL;
    PWSTR pwszShare = NULL;
    nfssvc_NetShareInfo Info;
    PSHARE_INFO_502 pInfo502 = NULL;
    SHARE_INFO_502_I Info502i = {0};
    PSHARE_INFO_1501 pInfo1501 = NULL;
    SHARE_INFO_1501_I Info1501i = {0};
    PVOID pSecDescBuffer = NULL;

    BAIL_ON_INVALID_PTR(pContext, err);
    BAIL_ON_INVALID_PTR(pContext->hBinding, err);
    BAIL_ON_INVALID_PTR(pwszNetname, err);

    memset(&Info, 0, sizeof(Info));
    memset(&Info502i, 0, sizeof(Info502i));
    memset(&Info1501i, 0, sizeof(Info1501i));

    switch (dwLevel)
    {
    case 1:
        Info.info1 = (PSHARE_INFO_1)pBuffer;
        break;

    case 2:
        Info.info2 = (PSHARE_INFO_2)pBuffer;
        break;

    case 502:
        pInfo502 = (PSHARE_INFO_502)pBuffer;
        if (pInfo502)
        {
            if ((pInfo502->shi502_security_descriptor && !pInfo502->shi502_reserved) ||
                (!pInfo502->shi502_security_descriptor && pInfo502->shi502_reserved))
            {
                err = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN_ERROR(err);
            }

            Info502i.shi502_netname             = pInfo502->shi502_netname;
            Info502i.shi502_type                = pInfo502->shi502_type;
            Info502i.shi502_remark              = pInfo502->shi502_remark;
            Info502i.shi502_permissions         = pInfo502->shi502_permissions;
            Info502i.shi502_max_uses            = pInfo502->shi502_max_uses;
            Info502i.shi502_current_uses        = pInfo502->shi502_current_uses;
            Info502i.shi502_path                = pInfo502->shi502_path;
            Info502i.shi502_password            = pInfo502->shi502_password;
            Info502i.shi502_reserved            = pInfo502->shi502_reserved;
            Info502i.shi502_security_descriptor = pInfo502->shi502_security_descriptor;

            Info.info502 = &Info502i;
        }
        break;

    case 1004:
        Info.info1004 = (PSHARE_INFO_1004)pBuffer;
        break;

    case 1005:
        Info.info1005 = (PSHARE_INFO_1005)pBuffer;
        break;

    case 1006:
        Info.info1006 = (PSHARE_INFO_1006)pBuffer;
        break;

    case 1501:
        pInfo1501 = (PSHARE_INFO_1501)pBuffer;
        if (pInfo1501)
        {
            if ((pInfo1501->shi1501_security_descriptor && !pInfo1501->shi1501_reserved) ||
                (!pInfo1501->shi1501_security_descriptor && pInfo1501->shi1501_reserved))
            {
                err = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN_ERROR(err);
            }

            Info1501i.shi1501_reserved            = pInfo1501->shi1501_reserved;
            Info1501i.shi1501_security_descriptor = pInfo1501->shi1501_security_descriptor;

            Info.info1501 = &Info1501i;
        }
        break;
    }

    DCERPC_CALL(err,
                _NetrShareSetInfo(pContext->hBinding,
                                  pwszServername,
                                  pwszNetname,
                                  dwLevel,
                                  Info,
                                  pdwParmErr));
    BAIL_ON_WIN_ERROR(err);

cleanup:
    LW_SAFE_FREE_MEMORY(pSecDescBuffer);

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
