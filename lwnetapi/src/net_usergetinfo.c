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
 *        net_usergetinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetUserGetInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NetUserGetInfo(
    PCWSTR  pwszHostname,
    PCWSTR  pwszUsername,
    DWORD   dwLevel,
    PVOID  *ppBuffer
    )
{
    const DWORD dwUserAccessFlags = USER_ACCESS_GET_NAME_ETC |
                                    USER_ACCESS_GET_LOCALE |
                                    USER_ACCESS_GET_LOGONINFO |
                                    USER_ACCESS_GET_ATTRIBUTES |
                                    USER_ACCESS_GET_GROUPS |
                                    USER_ACCESS_GET_GROUP_MEMBERSHIP;
    const DWORD dwSamrInfoLevel = 21;

    NTSTATUS status = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    PWSTR pwszNameCopy = NULL;
    PNET_CONN pConn = NULL;
    SAMR_BINDING hSamrBinding = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    DWORD dwUserRid = 0;
    UserInfo *pSamrUserInfo = NULL;
    PVOID pSourceBuffer = NULL;
    DWORD dwSize = 0;
    DWORD dwSpaceAvailable = 0;
    PVOID pBuffer = NULL;
    PIO_CREDS pCreds = NULL;
    NET_VALIDATION_LEVEL eValidation = NET_VALIDATION_NONE;

    BAIL_ON_INVALID_PTR(pwszUsername, err);
    BAIL_ON_INVALID_PTR(ppBuffer, err);

    if (!(dwLevel == 0 ||
          dwLevel == 1 ||
          dwLevel == 2 ||
          dwLevel == 3 ||
          dwLevel == 4 ||
          dwLevel == 10 ||
          dwLevel == 11 ||
          dwLevel == 20 ||
          dwLevel == 23))
    {
        err = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(err);
    }
    
    status = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&pConn,
                            pwszHostname,
                            0,
                            0,
                            pCreds);
    BAIL_ON_NT_STATUS(status);

    status = NetOpenUser(pConn,
                         pwszUsername,
                         dwUserAccessFlags,
                         &hUser,
                         &dwUserRid);
    BAIL_ON_NT_STATUS(status);

    hSamrBinding = pConn->Rpc.Samr.hBinding;

    if (dwLevel == 0)
    {
        err = LwAllocateWc16String(&pwszNameCopy,
                                   pwszUsername);
        BAIL_ON_WIN_ERROR(err);

        pSourceBuffer = pwszNameCopy;
    }
    else
    {
        status = SamrQueryUserInfo(hSamrBinding,
                                   hUser,
                                   dwSamrInfoLevel,
                                   &pSamrUserInfo);
        BAIL_ON_NT_STATUS(status);

        pSourceBuffer = &pSamrUserInfo->info21;
    }

    err = NetAllocateUserInfo(NULL,
                              NULL,
                              dwLevel,
                              pSourceBuffer,
                              &dwSize,
                              eValidation);
    BAIL_ON_WIN_ERROR(err);

    dwSpaceAvailable = dwSize;
    dwSize           = 0;

    status = NetAllocateMemory(OUT_PPVOID(&pBuffer),
                               dwSpaceAvailable);
    BAIL_ON_NT_STATUS(status);

    err = NetAllocateUserInfo(pBuffer,
                              &dwSpaceAvailable,
                              dwLevel,
                              pSourceBuffer,
                              &dwSize,
                              eValidation);
    BAIL_ON_WIN_ERROR(err);

    if (dwLevel == 4)
    {
        //
        // USER_INFO_4 requires providing a complete user SID which
        // has to be derived from domain SID and user RID
        //

        PUSER_INFO_4 pInfo4 = pBuffer;
        DWORD sidSize = RtlLengthRequiredSid(SID_MAX_SUB_AUTHORITIES);

        status = RtlCopySid(sidSize,
                            pInfo4->usri4_user_sid,
                            pConn->Rpc.Samr.pDomainSid);
        BAIL_ON_NT_STATUS(status);

        status = RtlAppendRidSid(sidSize,
                                 pInfo4->usri4_user_sid,
                                 dwUserRid);
        BAIL_ON_NT_STATUS(status);
    }

    status = SamrClose(hSamrBinding, hUser);
    BAIL_ON_NT_STATUS(status);

    *ppBuffer = pBuffer;

cleanup:
    NetDisconnectSamr(&pConn);

    if (pSamrUserInfo)
    {
        SamrFreeMemory(pSamrUserInfo);
    }

    LW_SAFE_FREE_MEMORY(pwszNameCopy);

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
    if (pBuffer)
    {
        NetFreeMemory((void*)pBuffer);
    }

    *ppBuffer = NULL;
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
