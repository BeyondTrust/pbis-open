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
 *        net_localgroupsetinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetLocalGroupSetInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NetLocalGroupSetInfo(
    PCWSTR  pwszHostname,
    PCWSTR  pwszAliasname,
    DWORD   dwLevel,
    PVOID   pBuffer,
    PDWORD  pdwParmErr
    )
{
    const DWORD dwAliasAccessRights = ALIAS_ACCESS_SET_INFO;

    NTSTATUS status = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    PNET_CONN pConn = NULL;
    SAMR_BINDING hSamrBinding = NULL;
    ACCOUNT_HANDLE hAlias = NULL;
    PLOCALGROUP_INFO_0 pInfo0 = NULL;
    PLOCALGROUP_INFO_1 pInfo1 = NULL;
    PLOCALGROUP_INFO_1002 pInfo1002 = NULL;
    PWSTR pwszComment = NULL;
    PWSTR pwszNewAliasname = NULL;
    DWORD dwAliasRid = 0;
    DWORD dwSamrInfoLevel = 0;
    AliasInfo InfoName;
    AliasInfo InfoDescription;
    PIO_CREDS pCreds = NULL;

    memset(&InfoName, 0, sizeof(InfoName));
    memset(&InfoDescription, 0, sizeof(InfoDescription));

    BAIL_ON_INVALID_PTR(pwszAliasname, err);
    BAIL_ON_INVALID_PTR(pBuffer, err);

    status = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&pConn,
                            pwszHostname,
                            0,
                            0,
                            pCreds);
    BAIL_ON_NT_STATUS(status);

    hSamrBinding = pConn->Rpc.Samr.hBinding;

    status = NetOpenAlias(pConn,
                          pwszAliasname,
                          dwAliasAccessRights,
                          &hAlias,
                          &dwAliasRid);
    BAIL_ON_NT_STATUS(status);

    switch (dwLevel)
    {
    case 0:
        pInfo0 = (PLOCALGROUP_INFO_0)pBuffer;

        err = LwAllocateWc16String(&pwszNewAliasname,
                                   pInfo0->lgrpi0_name);
        BAIL_ON_WIN_ERROR(err);
        break;

    case 1:
        pInfo1 = (PLOCALGROUP_INFO_1)pBuffer;

        /* lgrpi1_name is ignored in NetLocalGroupSetInfo */

        err = LwAllocateWc16String(&pwszComment,
                                   pInfo1->lgrpi1_comment);
        BAIL_ON_WIN_ERROR(err);
        break;

    case 1002:
        pInfo1002 = (PLOCALGROUP_INFO_1002)pBuffer;

        err = LwAllocateWc16String(&pwszComment,
                                   pInfo1002->lgrpi1002_comment);
        BAIL_ON_WIN_ERROR(err);
        break;

    default:
        err = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN_ERROR(err);
    }

    if (pwszNewAliasname)
    {
        dwSamrInfoLevel = ALIAS_INFO_NAME;

        err = LwAllocateUnicodeStringFromWc16String(
                                  (PUNICODE_STRING)&InfoName.name,
                                  pwszNewAliasname);

        status = SamrSetAliasInfo(hSamrBinding,
                                  hAlias,
                                  dwSamrInfoLevel,
                                  &InfoName);
        BAIL_ON_NT_STATUS(status);
    }

    if (pwszComment)
    {
        dwSamrInfoLevel = ALIAS_INFO_DESCRIPTION;

        err = LwAllocateUnicodeStringFromWc16String(
                                  (PUNICODE_STRING)&InfoDescription.description,
                                  pwszComment);

        status = SamrSetAliasInfo(hSamrBinding,
                                  hAlias,
                                  dwSamrInfoLevel,
                                  &InfoDescription);
        BAIL_ON_NT_STATUS(status);
    }

    status = SamrClose(hSamrBinding, hAlias);
    BAIL_ON_NT_STATUS(status);

cleanup:
    NetDisconnectSamr(&pConn);

    LwFreeUnicodeString((PUNICODE_STRING)&InfoName.name);
    LwFreeUnicodeString((PUNICODE_STRING)&InfoDescription.description);

    LW_SAFE_FREE_MEMORY(pwszNewAliasname);
    LW_SAFE_FREE_MEMORY(pwszComment);

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS) {
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
