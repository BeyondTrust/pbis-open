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
 *        net_useradd.h
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
NetUserSetInfo(
    PCWSTR  pwszHostname,
    PCWSTR  pwszUsername,
    DWORD   dwLevel,
    PVOID   pBuffer,
    PDWORD  pdwParmErr
    )
{
    /* This is necessary to be able to set account password.
       Otherwise we get access denied. Don't ask... */
    const DWORD dwDomainAccess = DOMAIN_ACCESS_LOOKUP_INFO_1;

    const DWORD dwUserAccess = USER_ACCESS_GET_NAME_ETC |
                               USER_ACCESS_GET_LOCALE |
                               USER_ACCESS_GET_LOGONINFO |
                               USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_GET_GROUPS |
                               USER_ACCESS_GET_GROUP_MEMBERSHIP |
                               USER_ACCESS_SET_LOC_COM |
                               USER_ACCESS_SET_ATTRIBUTES |
                               USER_ACCESS_CHANGE_PASSWORD |
                               USER_ACCESS_SET_PASSWORD;

    NTSTATUS status = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    PNET_CONN pConn = NULL;
    SAMR_BINDING hSamrBinding = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    DWORD dwUserRid = 0;
    DWORD dwSamrInfoLevel = 0;
    DWORD dwSamrPasswordInfoLevel = 0;
    DWORD dwParmErr = 0;
    UserInfo *pSamrUserInfo = NULL;
    UserInfo *pSamrPasswordUserInfo = NULL;
    DWORD dwSize = 0;
    DWORD dwSpaceLeft = 0;
    PIO_CREDS pCreds = NULL;
    NET_VALIDATION_LEVEL eValidation = NET_VALIDATION_USER_SET;

    if (!(dwLevel == 0 ||
          dwLevel == 1 ||
          dwLevel == 2 ||
          dwLevel == 3 ||
          dwLevel == 4 ||
          dwLevel == 1003 ||
          dwLevel == 1007 ||
          dwLevel == 1008 ||
          dwLevel == 1011))
    {
        err = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(err);
    }


    BAIL_ON_INVALID_PTR(pwszUsername, err);
    BAIL_ON_INVALID_PTR(pBuffer, err);

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
        status = NetAllocateMemory(OUT_PPVOID(&pSamrUserInfo),
                                   dwSpaceLeft);
        BAIL_ON_NT_STATUS(status);

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
    }

    status = NetConnectSamr(&pConn,
                            pwszHostname,
                            dwDomainAccess,
                            0,
                            pCreds);
    BAIL_ON_NT_STATUS(status);
    
    hSamrBinding = pConn->Rpc.Samr.hBinding;

    status = NetOpenUser(pConn,
                         pwszUsername,
                         dwUserAccess,
                         &hUser,
                         &dwUserRid);
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
            status = NetAllocateMemory(OUT_PPVOID(&pSamrPasswordUserInfo),
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
    }
    else if (err == ERROR_INVALID_LEVEL ||
             (err == ERROR_INVALID_PASSWORD &&
              dwLevel != 1003))
    {
        /* This error only means we're not going to try
           set the password.
           Either it's set to NULL in infolevel where it's optional
           or called infolevel doesn't support setting password */
        err = ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_WIN_ERROR(err);
    }

    if (dwSamrInfoLevel)
    {
        status = SamrSetUserInfo(hSamrBinding,
                                 hUser,
                                 dwSamrInfoLevel,
                                 pSamrUserInfo);
        BAIL_ON_NT_STATUS(status);
    }
	
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

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(status);
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
