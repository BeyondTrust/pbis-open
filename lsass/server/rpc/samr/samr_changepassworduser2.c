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
    const wchar_t wszFilterFmt[] = L"%ws='%ws'";

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    HANDLE hDirectory = NULL;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    DWORD dwFilterLen = 0;
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwNumEntries = 0;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrSecurityDesc[] = DS_ATTR_SECURITY_DESCRIPTOR;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrNtHash[] = DS_ATTR_NT_HASH;
    size_t sUserNameLen = 0;
    PWSTR pwszUserName = NULL;
    size_t sDomainNameLen = 0;
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

    sDomainNameLen = pDomainName->Length / 2;
    sUserNameLen   = pAccountName->Length / 2;

    dwFilterLen = (((sizeof(wszAttrSamAccountName)
                     /sizeof(wszAttrSamAccountName[0])) - 1) +
                   sUserNameLen +
                   (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0])));

    dwError = LwAllocateMemory(dwFilterLen * sizeof(WCHAR),
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                    wszAttrSamAccountName, pwszUserName) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

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
