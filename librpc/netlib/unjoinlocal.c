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


NTSTATUS
DisableWksAccount(
    PNET_CONN       pConn,
    wchar16_t      *account_name,
    ACCOUNT_HANDLE *phAccount
    )
{
	const UINT32 user_access = USER_ACCESS_GET_ATTRIBUTES | 
                                   USER_ACCESS_SET_ATTRIBUTES;
	const UINT32 acct_flags_level = 16;

	NTSTATUS status;
	handle_t samr_b;
	DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hAccount = NULL;
	wchar16_t *names[1];
	UserInfo16 *info16;
	UINT32 *rids, *types;
	UserInfo sinfo;
    UserInfo *qinfo = NULL;

    memset((void*)&sinfo, 0, sizeof(sinfo));

	samr_b  = pConn->Rpc.Samr.hBinding;
	hDomain = pConn->Rpc.Samr.hDomain;
	info16  = &sinfo.info16;

	names[0] = account_name;
	status = SamrLookupNames(samr_b, hDomain, 1, names, &rids, &types, NULL);
	if (status != STATUS_SUCCESS) goto error;

	/* TODO: what should we actually do if the number of rids found
	   is greater than 1 ? */

	status = SamrOpenUser(samr_b, hDomain, user_access, rids[0], &hAccount);
	if (status != STATUS_SUCCESS) goto error;

	status = SamrQueryUserInfo(samr_b, hAccount, acct_flags_level, &qinfo);
	if (status != STATUS_SUCCESS) goto error;

	/* set "account disabled" flag */
	info16->account_flags = qinfo->info16.account_flags;
	info16->account_flags |= ACB_DISABLED;

	status = SamrSetUserInfo(samr_b, hAccount, acct_flags_level, &sinfo);

    *phAccount = hAccount;

cleanup:
    if (rids)
    {
        SamrFreeMemory(rids);
    }

    if (types)
    {
        SamrFreeMemory(types);
    }

    if (qinfo)
    {
        SamrFreeMemory(qinfo);
    }

	return status;

error:
    *phAccount = NULL;

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
