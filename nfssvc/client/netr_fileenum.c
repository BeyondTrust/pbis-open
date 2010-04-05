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

#include "includes.h"


NET_API_STATUS
NetrFileEnum(
    PNFSSVC_CONTEXT pContext,
    const wchar16_t *servername,
    const wchar16_t *basepath,
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
    nfssvc_NetFileCtr ctr;
    nfssvc_NetFileCtr2 ctr2;
    nfssvc_NetFileCtr3 ctr3;
    UINT32 l = level;

    BAIL_ON_INVALID_PTR(pContext, status);
    BAIL_ON_INVALID_PTR(pContext->hBinding, status);
    BAIL_ON_INVALID_PTR(bufptr, status);
    BAIL_ON_INVALID_PTR(entriesread, status);
    BAIL_ON_INVALID_PTR(totalentries, status);

    memset(&ctr, 0, sizeof(ctr));
    memset(&ctr2, 0, sizeof(ctr2));
    memset(&ctr3, 0, sizeof(ctr3));

    *entriesread = 0;
    *bufptr = NULL;

    switch (level) {
    case 2:
        ctr.ctr2 = &ctr2;
        break;
    case 3:
        ctr.ctr3 = &ctr3;
        break;
    }

    DCERPC_CALL(status,
                _NetrFileEnum(pContext->hBinding,
                              (wchar16_t *)servername,
                              (wchar16_t *)basepath,
                              (wchar16_t *)username,
                              &l, &ctr,
                              prefmaxlen, totalentries,
                              resume_handle));

    if (l != level) {
        status = ERROR_BAD_NET_RESP;
        BAIL_ON_WIN_ERROR(status);
    }

    memerr = NfsSvcCopyNetFileCtr(l, &ctr, entriesread, bufptr);
    BAIL_ON_WIN_ERROR(memerr);

cleanup:
    switch (level) {
    case 2:
        if (ctr.ctr2 == &ctr2) {
            ctr.ctr2 = NULL;
        }
        break;
    case 3:
        if (ctr.ctr3 == &ctr3) {
            ctr.ctr3 = NULL;
        }
        break;
    }
    NfsSvcClearNetFileCtr(l, &ctr);

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
