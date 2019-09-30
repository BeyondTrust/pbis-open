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
 *        samr_changepassworduser2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrSrvChangePasswordUser2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvChangePasswordUser2(
    /* [in] */ handle_t         hBinding,
    /* [in] */ UNICODE_STRING  *pDomainName,
    /* [in] */ UNICODE_STRING  *pAccountName,
    /* [in] */ CryptPassword   *pNtPasswordBlob,
    /* [in] */ HashPassword    *pNtVerifier,
    /* [in] */ UINT8            ussLmChange,
    /* [in] */ CryptPassword   *pLmPasswordBlob,
    /* [in] */ HashPassword    *pLmVerifier
    )
{
    PCSTR filterFormat = "%s=%Q";

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    HANDLE hDirectory = NULL;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwNumEntries = 0;
    CHAR szAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrSecurityDesc[] = DS_ATTR_SECURITY_DESCRIPTOR;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrNtHash[] = DS_ATTR_NT_HASH;
    PWSTR pwszUserName = NULL;
    PSTR pszUserName = NULL;
    PWSTR pwszDomainName = NULL;
    WCHAR wszSystemName[] = { '\\', '\\', 0 };
    DWORD dwConnectFlags = SAMR_ACCESS_CONNECT_TO_SERVER;
    PCONNECT_CONTEXT pConnCtx = NULL;
    DWORD dwObjectClass = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    GENERIC_MAPPING GenericMapping = {0};
    DWORD dwAccessGranted = 0;
    POCTET_STRING pOldNtHashBlob = NULL;
    PWSTR pwszNewPassword = NULL;
    size_t sNewPasswordLen = 0;
    OCTET_STRING NewNtHashBlob = {0};
    PWSTR pwszAccountDn = NULL;

    PWSTR wszAttributes[] = {
        wszAttrObjectClass,
        wszAttrDn,
        wszAttrSecurityDesc,
        wszAttrAccountFlags,
        wszAttrNtHash,
        NULL
    };

    BAIL_ON_INVALID_PTR(hBinding);
    BAIL_ON_INVALID_PTR(pDomainName);
    BAIL_ON_INVALID_PTR(pAccountName);
    BAIL_ON_INVALID_PTR(pNtPasswordBlob);
    BAIL_ON_INVALID_PTR(pNtVerifier);

    ntStatus = SamrSrvConnect2(hBinding,
                               (PWSTR)wszSystemName,
                               dwConnectFlags,
                               (CONNECT_HANDLE*)&pConnCtx);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (pConnCtx->pUserToken == NULL)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    hDirectory = pConnCtx->hDirectory;

    dwError = LwAllocateWc16StringFromUnicodeString(
                                   &pwszDomainName,
                                   pDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16StringFromUnicodeString(
                                   &pwszUserName,
                                   pAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszUserName, &pszUserName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryAllocateWC16StringFilterPrintf(
                              &pwszFilter,
                              filterFormat,
                              szAttrSamAccountName, pszUserName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntries,
                              &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        ntStatus = STATUS_NO_SUCH_USER;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = DirectoryGetEntryAttrValueByName(
                                  &pEntries[0],
                                  wszAttrObjectClass,
                                  DIRECTORY_ATTR_TYPE_INTEGER,
                                  &dwObjectClass);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Check if the account really is a user account
     */
    if (dwObjectClass != DS_OBJECT_CLASS_USER)
    {
        ntStatus = STATUS_NO_SUCH_USER;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Access check if requesting user is allowed to change password
     */
    dwError = DirectoryGetEntrySecurityDescriptor(
                                  &(pEntries[0]),
                                  &pSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    if (!RtlAccessCheck(pSecDesc,
                        pConnCtx->pUserToken,
                        USER_ACCESS_CHANGE_PASSWORD,
                        0,
                        &GenericMapping,
                        &dwAccessGranted,
                        &ntStatus))
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Get current NT hash
     */
    dwError = DirectoryGetEntryAttrValueByName(
                                  &pEntries[0],
                                  wszAttrNtHash,
                                  DIRECTORY_ATTR_TYPE_OCTET_STREAM,
                                  &pOldNtHashBlob);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Decrypt the password
     */
    ntStatus = SamrSrvDecryptPasswordBlob(pConnCtx,
                                          pNtPasswordBlob,
                                          pOldNtHashBlob->pBytes,
                                          pOldNtHashBlob->ulNumBytes,
                                          0,
                                          &pwszNewPassword);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwWc16sLen(pwszNewPassword, &sNewPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Calculate new NT hash
     */
    ntStatus = SamrSrvGetNtPasswordHash(pwszNewPassword,
                                        &NewNtHashBlob.pBytes,
                                        &NewNtHashBlob.ulNumBytes);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Verify new NT hash
     */
    ntStatus = SamrSrvVerifyNewNtPasswordHash(
                                   NewNtHashBlob.pBytes,
                                   NewNtHashBlob.ulNumBytes,
                                   pOldNtHashBlob->pBytes,
                                   pOldNtHashBlob->ulNumBytes,
                                   pNtVerifier);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Set the password
     */

    dwError = DirectoryGetEntryAttrValueByName(
                                  &pEntries[0],
                                  wszAttrDn,
                                  DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                  &pwszAccountDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySetPassword(hDirectory,
                                   pwszAccountDn,
                                   pwszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    SamrSrvClose(hBinding,
                 (PVOID*)&pConnCtx);

    LW_SAFE_FREE_MEMORY(pwszDomainName);
    LW_SAFE_FREE_MEMORY(pwszUserName);
    LW_SAFE_FREE_MEMORY(pszUserName);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    DirectoryFreeEntrySecurityDescriptor(&pSecDesc);

    LW_SECURE_FREE_WSTRING(pwszNewPassword);

    if (pOldNtHashBlob)
    {
        memset(pOldNtHashBlob->pBytes, 0, pOldNtHashBlob->ulNumBytes);
    }

    LW_SECURE_FREE_MEMORY(NewNtHashBlob.pBytes, NewNtHashBlob.ulNumBytes);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

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
