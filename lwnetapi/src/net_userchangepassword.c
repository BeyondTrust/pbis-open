/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "includes.h"


NET_API_STATUS
NetUserChangePassword(
    IN  PCWSTR  pwszDomainName,
    IN  PCWSTR  pwszUserName,
    IN  PCWSTR  pwszOldPassword,
    IN  PCWSTR  pwszNewPassword
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    SAMR_BINDING hSamrBinding = NULL;
    PSTR pszHostname = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszUser = NULL;
    size_t sOldPasswordLen = 0;
    size_t sNewPasswordLen = 0;
    BYTE OldNtHash[16] = {0};
    BYTE NewNtHash[16] = {0};
    BYTE NtPasswordBuffer[516] = {0};
    BYTE NtVerHash[16] = {0};
    RC4_KEY RC4Key;
    PIO_CREDS pCreds = NULL;

    memset(&RC4Key, 0, sizeof(RC4Key));

    BAIL_ON_INVALID_PTR(pwszDomainName, err);
    BAIL_ON_INVALID_PTR(pwszUserName, err);
    BAIL_ON_INVALID_PTR(pwszOldPassword, err);
    BAIL_ON_INVALID_PTR(pwszNewPassword, err);

    ntStatus = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LwWc16sToMbs(pwszDomainName, &pszHostname);
    BAIL_ON_WIN_ERROR(err);

    err = LwAllocateWc16String(&pwszDomain, pwszDomainName);
    BAIL_ON_WIN_ERROR(err);

    err = LwAllocateWc16String(&pwszUser, pwszUserName);
    BAIL_ON_WIN_ERROR(err);

    ntStatus = SamrInitBindingDefault(&hSamrBinding,
                                      pwszDomainName,
                                      pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LwWc16sLen(pwszOldPassword, &sOldPasswordLen);
    BAIL_ON_WIN_ERROR(err);

    err = LwWc16sLen(pwszNewPassword, &sNewPasswordLen);
    BAIL_ON_WIN_ERROR(err);

    /* prepare NT password hashes */
    err = NetGetNtPasswordHash(pwszOldPassword,
                               OldNtHash,
                               sizeof(OldNtHash));
    BAIL_ON_WIN_ERROR(err);

    err = NetGetNtPasswordHash(pwszNewPassword,
                               NewNtHash,
                               sizeof(NewNtHash));
    BAIL_ON_WIN_ERROR(err);

    /* encode password buffer */
    err = NetEncodePasswordBuffer(pwszNewPassword,
                                  NtPasswordBuffer,
                                  sizeof(NtPasswordBuffer));
    BAIL_ON_WIN_ERROR(err);

    RC4_set_key(&RC4Key, 16, (unsigned char*)OldNtHash);
    RC4(&RC4Key, sizeof(NtPasswordBuffer), NtPasswordBuffer, NtPasswordBuffer);

    /* encode NT verifier */
    err = NetEncryptNtHashVerifier(NewNtHash, sizeof(NewNtHash),
                                   OldNtHash, sizeof(OldNtHash),
                                   NtVerHash, sizeof(NtVerHash));
    BAIL_ON_WIN_ERROR(err);

    ntStatus = SamrChangePasswordUser2(hSamrBinding,
                                       pwszDomain,
                                       pwszUser,
                                       NtPasswordBuffer,
                                       NtVerHash,
                                       0,
                                       NULL,
                                       NULL);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    if (hSamrBinding)
    {
        SamrFreeBinding(&hSamrBinding);
    }

    LW_SAFE_FREE_MEMORY(pszHostname);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszUser);

    memset(OldNtHash, 0, sizeof(OldNtHash));
    memset(NewNtHash, 0, sizeof(NewNtHash));
    memset(NtPasswordBuffer, 0, sizeof(NtPasswordBuffer));

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = NtStatusToWin32Error(ntStatus);
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
