/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
*/

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        net_useradd.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetUserAdd function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


NET_API_STATUS
NetUserAdd(
    PCWSTR  pwszHostname,
    DWORD   dwLevel,
    PVOID   pBuffer,
    PDWORD  pdwParmErr
    )
{
    const DWORD dwUserAccess = USER_ACCESS_GET_NAME_ETC |
                               USER_ACCESS_SET_LOC_COM |
                               USER_ACCESS_GET_LOCALE |
                               USER_ACCESS_GET_LOGONINFO |
                               USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_GET_GROUPS |
                               USER_ACCESS_GET_GROUP_MEMBERSHIP |
                               USER_ACCESS_CHANGE_GROUP_MEMBERSHIP |
                               USER_ACCESS_SET_ATTRIBUTES |
                               USER_ACCESS_SET_PASSWORD;

    const DWORD dwDomainAccess = DOMAIN_ACCESS_CREATE_USER |
                                 DOMAIN_ACCESS_LOOKUP_INFO_1;

    NTSTATUS status = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    PNET_CONN pConn = NULL;
    SAMR_BINDING hSamrBinding = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    DWORD dwSamrInfoLevel = 0;
    DWORD dwSamrPasswordInfoLevel = 0;
    DWORD dwParmErr = 0;
    UserInfo *pSamrUserInfo = NULL;
    UserInfo *pSamrPasswordUserInfo = NULL;
    DWORD dwSize = 0;
    DWORD dwSpaceLeft = 0;
    PIO_CREDS pCreds = NULL;
    PWSTR pwszUsername = NULL;
    DWORD dwRid = 0;
    BOOL bPasswordSet = FALSE;
    NET_VALIDATION_LEVEL eValidation = NET_VALIDATION_USER_ADD;

    BAIL_ON_INVALID_PTR(pBuffer, err);

    if (!(dwLevel == 1 ||
          dwLevel == 2 ||
          dwLevel == 3 ||
          dwLevel == 4))
    {
        err = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(err);
    }

    status = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(status);

    err = NetAllocateSamrUserInfo(NULL,
                                  &dwSamrInfoLevel,
                                  NULL,
                                  dwLevel,
                                  pBuffer,
                                  pConn,
                                  &dwSize,
                                  eValidation,
                                  &dwParmErr);
    BAIL_ON_WIN_ERROR(err);

    dwSpaceLeft = dwSize;
    dwSize      = 0;

    if (dwSpaceLeft)
    {
        status = NetAllocateMemory((void**)&pSamrUserInfo,
                                   dwSpaceLeft);
        BAIL_ON_NT_STATUS(status);
    }

    err = NetAllocateSamrUserInfo(&pSamrUserInfo->info21,
                                  &dwSamrInfoLevel,
                                  &dwSpaceLeft,
                                  dwLevel,
                                  pBuffer,
                                  pConn,
                                  &dwSize,
                                  eValidation,
                                  &dwParmErr);
    BAIL_ON_WIN_ERROR(err);

    status = NetConnectSamr(&pConn,
                            pwszHostname,
                            dwDomainAccess,
                            0,
                            pCreds);
    BAIL_ON_NT_STATUS(status);

    hSamrBinding = pConn->Rpc.Samr.hBinding;
    hDomain      = pConn->Rpc.Samr.hDomain;

    err = LwAllocateWc16StringFromUnicodeString(
                         &pwszUsername,
                         (PUNICODE_STRING)&pSamrUserInfo->info21.account_name);
    BAIL_ON_WIN_ERROR(err);

    status = SamrCreateUser(hSamrBinding,
                            hDomain,
                            pwszUsername,
                            dwUserAccess,
                            &hUser,
                            &dwRid);
    if (status == STATUS_USER_EXISTS)
    {
        err = NERR_UserExists;
    }
    else if (status == STATUS_ALIAS_EXISTS)
    {
        err = NERR_GroupExists;
    }
    BAIL_ON_NT_STATUS(status);

    /*
     * Check if there's password to be set (if it's NULL
     * the function returns ERROR_INVALID_PASSWORD)
     */

    dwSamrPasswordInfoLevel = 26;
    dwSize                  = 0;

    err = NetAllocateSamrUserInfo(NULL,
                                  &dwSamrPasswordInfoLevel,
                                  NULL,
                                  dwLevel,
                                  pBuffer,
                                  pConn,
                                  &dwSize,
                                  eValidation,
                                  &dwParmErr);
    if (err == ERROR_SUCCESS)
    {
        dwSpaceLeft = dwSize;
        dwSize      = 0;

        if (dwSpaceLeft)
        {
            status = NetAllocateMemory((void**)&pSamrPasswordUserInfo,
                                       dwSpaceLeft);
            BAIL_ON_NT_STATUS(status);
        }

        err = NetAllocateSamrUserInfo(&pSamrPasswordUserInfo->info26,
                                      &dwSamrPasswordInfoLevel,
                                      &dwSpaceLeft,
                                      dwLevel,
                                      pBuffer,
                                      pConn,
                                      &dwSize,
                                      eValidation,
                                      &dwParmErr);
        BAIL_ON_WIN_ERROR(err);

        status = SamrSetUserInfo(hSamrBinding,
                                 hUser,
                                 dwSamrPasswordInfoLevel,
                                 pSamrPasswordUserInfo);
        BAIL_ON_NT_STATUS(status);

        bPasswordSet = TRUE;

    }
    else if (err == ERROR_INVALID_PASSWORD)
    {
        /* This error only means we're not going to try
           set the password */
        err = ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_WIN_ERROR(err);
    }

    /*
     * Prevent from trying to rename (to the same name) the account
     * that has just been created. Created samr user info buffer
     * contains whatever is passed from net user info buffer.
     */
    if (dwSamrInfoLevel == 21 &&
        (pSamrUserInfo->info21.fields_present & SAMR_FIELD_ACCOUNT_NAME))
    {
        pSamrUserInfo->info21.fields_present ^= SAMR_FIELD_ACCOUNT_NAME;
    }

    /*
     * Disable the account only if there was no password
     */
    if (!bPasswordSet &&
        dwSamrInfoLevel == 21)
    {
        pSamrUserInfo->info21.account_flags |= ACB_DISABLED;
    }

    status = SamrSetUserInfo(hSamrBinding,
                             hUser,
                             dwSamrInfoLevel,
                             pSamrUserInfo);
    BAIL_ON_NT_STATUS(status);

    status = SamrClose(hSamrBinding, hUser);
    BAIL_ON_NT_STATUS(status);

cleanup:
    NetDisconnectSamr(&pConn);

    if (pdwParmErr)
    {
        *pdwParmErr = dwParmErr;
    }

    if (pSamrUserInfo)
    {
        NetFreeMemory(pSamrUserInfo);
    }

    if (pSamrPasswordUserInfo)
    {
        NetFreeMemory(pSamrPasswordUserInfo);
    }

    LW_SAFE_FREE_MEMORY(pwszUsername);

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = NtStatusToWin32Error(status);
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
