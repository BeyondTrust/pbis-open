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
NetrSessionEnum(
    PNFSSVC_CONTEXT pContext,
    const wchar16_t *servername,
    const wchar16_t *unc_client_name,
    const wchar16_t *username,
    UINT32 level,
    UINT8 **bufptr,
    UINT32 prefmaxlen,
    UINT32 *entriesread,
    UINT32 *totalentries,
    UINT32 *resume_handle
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    NET_API_STATUS memerr = ERROR_SUCCESS;
    nfssvc_NetSessCtr ctr;
    nfssvc_NetSessCtr0 ctr0;
    nfssvc_NetSessCtr1 ctr1;
    nfssvc_NetSessCtr2 ctr2;
    nfssvc_NetSessCtr10 ctr10;
    nfssvc_NetSessCtr502 ctr502;
    UINT32 l = level;

    BAIL_ON_INVALID_PTR(pContext, status);
    BAIL_ON_INVALID_PTR(pContext->hBinding, status);
    BAIL_ON_INVALID_PTR(bufptr, status);
    BAIL_ON_INVALID_PTR(entriesread, status);
    BAIL_ON_INVALID_PTR(totalentries, status);

    memset(&ctr, 0, sizeof(ctr));
    memset(&ctr0, 0, sizeof(ctr0));
    memset(&ctr1, 0, sizeof(ctr1));
    memset(&ctr2, 0, sizeof(ctr2));
    memset(&ctr10, 0, sizeof(ctr10));
    memset(&ctr502, 0, sizeof(ctr502));

    *entriesread = 0;
    *bufptr = NULL;

    switch (level) {
    case 0:
        ctr.ctr0 = &ctr0;
        break;
    case 1:
        ctr.ctr1 = &ctr1;
        break;
    case 2:
        ctr.ctr2 = &ctr2;
        break;
    case 10:
        ctr.ctr10 = &ctr10;
        break;
    case 502:
        ctr.ctr502 = &ctr502;
        break;
    }

    DCERPC_CALL(status,
                _NetrSessionEnum(pContext->hBinding,
                                 (wchar16_t *)servername,
                                 (wchar16_t *)unc_client_name,
                                 (wchar16_t *)username,
                                 &l, &ctr,
                                 prefmaxlen, totalentries,
                                 resume_handle));

    if (l != level) {
        status = ERROR_BAD_NET_RESP;
        BAIL_ON_WIN_ERROR(status);
    }

    memerr = NfsSvcCopyNetSessCtr(l, &ctr, entriesread, bufptr);
    BAIL_ON_WIN_ERROR(memerr);

cleanup:
    switch (level) {
    case 0:
        if (ctr.ctr0 == &ctr0) {
            ctr.ctr0 = NULL;
        }
        break;
    case 1:
        if (ctr.ctr1 == &ctr1) {
            ctr.ctr1 = NULL;
        }
        break;
    case 2:
        if (ctr.ctr2 == &ctr2) {
            ctr.ctr2 = NULL;
        }
        break;
    case 10:
        if (ctr.ctr10 == &ctr10) {
            ctr.ctr10 = NULL;
        }
        break;
    case 502:
        if (ctr.ctr502 == &ctr502) {
            ctr.ctr502 = NULL;
        }
        break;
    }
    NfsSvcClearNetSessCtr(l, &ctr);

    return status;

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
