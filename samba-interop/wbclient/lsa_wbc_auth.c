/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software
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
 *        lsa_wbc_auth.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *
 */

#include "wbclient.h"
#include "lsawbclient_p.h"
#include <lsa/lsa.h>
#include <lwstr.h>
#include <lwmem.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>

wbcErr wbcAuthenticateUser(
    const char *username,
    const char *password
    )
{
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

    BAIL_ON_NULL_PTR_PARAM(username, dwErr);
    BAIL_ON_NULL_PTR_PARAM(password, dwErr);

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaAuthenticateUser(hLsa, username, password, NULL);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

cleanup:
    if (hLsa) {
        LsaCloseServer(hLsa);
        hLsa = (HANDLE)NULL;
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

/******************************************************************
 */

static DWORD
CopyPlaintextParams(
    PLSA_AUTH_USER_PARAMS pLsaParams,
    const struct wbcAuthUserParams *params
    )
{
    DWORD dwErr = LW_ERROR_INTERNAL;
    PSTR pszPass = NULL;

    dwErr = LwAllocateString(params->password.plaintext, &pszPass);
    BAIL_ON_LSA_ERR(dwErr);

    pLsaParams->pass.clear.pszPassword = pszPass;

cleanup:
    return dwErr;
}

/******************************************************************
 */

static DWORD
CopyChapParams(
    PLSA_AUTH_USER_PARAMS pLsaParams,
    const struct wbcAuthUserParams *params
    )
{
    DWORD dwErr = LW_ERROR_INTERNAL;

    /* make sure we have at least one response */

    if ((params->password.response.nt_length == 0) &&
        (params->password.response.lm_length == 0))
    {
        dwErr = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Challenge */

    dwErr = LsaDataBlobStore(&pLsaParams->pass.chap.pChallenge,
                 8,
                 (const PBYTE)params->password.response.challenge);
    BAIL_ON_LSA_ERR(dwErr);

    /* NT Response */

    dwErr = LsaDataBlobStore(&pLsaParams->pass.chap.pNT_resp,
                 params->password.response.nt_length,
                 (const PBYTE)params->password.response.nt_data );
    BAIL_ON_LSA_ERR(dwErr);

    /* LM Response */

    dwErr = LsaDataBlobStore(&pLsaParams->pass.chap.pLM_resp,
                 params->password.response.lm_length,
                 (const PBYTE)params->password.response.lm_data );
    BAIL_ON_LSA_ERR(dwErr);

cleanup:
    if (!LW_ERROR_IS_OK(dwErr)) {
        LsaDataBlobFree(&pLsaParams->pass.chap.pChallenge);
        LsaDataBlobFree(&pLsaParams->pass.chap.pNT_resp);
        LsaDataBlobFree(&pLsaParams->pass.chap.pLM_resp);
    }

    return dwErr;
}

/******************************************************************
 */

static DWORD InitLsaAuthParams(
    PLSA_AUTH_USER_PARAMS pLsaParams,
    const struct wbcAuthUserParams *params
    )
{
    DWORD dwErr = LW_ERROR_INTERNAL;

    /* Check the auth level requested to validate input parms */

    switch (params->level)
    {
    case WBC_AUTH_USER_LEVEL_PLAIN:
        pLsaParams->AuthType = LSA_AUTH_PLAINTEXT;
        break;

    case WBC_AUTH_USER_LEVEL_RESPONSE:
        pLsaParams->AuthType = LSA_AUTH_CHAP;
        break;

    case WBC_AUTH_USER_LEVEL_HASH:
        dwErr = LW_ERROR_NOT_IMPLEMENTED;
        BAIL_ON_LSA_ERR(dwErr);
        break;

    default:
        dwErr = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Get the string data first */

    dwErr = LwAllocateString(params->account_name, &pLsaParams->pszAccountName);
    BAIL_ON_LSA_ERR(dwErr);

    if (params->domain_name) {
        dwErr = LwAllocateString(params->domain_name, &pLsaParams->pszDomain);
        BAIL_ON_LSA_ERR(dwErr);
    }

    if (params->workstation_name) {
        dwErr = LwAllocateString(params->workstation_name, 
                                  &pLsaParams->pszWorkstation);
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Now deal with the level specific parms */

    switch (pLsaParams->AuthType)
    {
    case LSA_AUTH_PLAINTEXT:
        dwErr = CopyPlaintextParams(pLsaParams, params);
        BAIL_ON_LSA_ERR(dwErr);
        break;

    case LSA_AUTH_CHAP:
        dwErr = CopyChapParams(pLsaParams, params);
        BAIL_ON_LSA_ERR(dwErr);
        break;
    }

cleanup:
    return dwErr;
}

/******************************************************************
 *****************************************************************/

static DWORD
CopyLsaUserInfoToWbcInfo(
    struct wbcAuthUserInfo *pWbcUserInfo,
    PLSA_AUTH_USER_INFO pLsaUserInfo
    )
{
    DWORD dwErr = LW_ERROR_INTERNAL;
    int i = 0;
    DWORD dwSidCount = 0;
    struct wbcDomainSid DomainSid = {0};

    BAIL_ON_NULL_PTR_PARAM(pWbcUserInfo, dwErr);
    BAIL_ON_NULL_PTR_PARAM(pLsaUserInfo, dwErr);

    memset(pWbcUserInfo, 0, sizeof(struct wbcAuthUserInfo));    

    pWbcUserInfo->user_flags = pLsaUserInfo->dwUserFlags;

    if (pLsaUserInfo->pszAccount) {
        pWbcUserInfo->account_name = _wbc_strdup(pLsaUserInfo->pszAccount);
        BAIL_ON_NULL_PTR(pWbcUserInfo->account_name, dwErr);
    }

    if (pLsaUserInfo->pszUserPrincipalName) {
        pWbcUserInfo->user_principal = _wbc_strdup(pLsaUserInfo->pszUserPrincipalName);
        BAIL_ON_NULL_PTR(pWbcUserInfo->user_principal, dwErr);
    }

    if (pLsaUserInfo->pszFullName) {
        pWbcUserInfo->full_name = _wbc_strdup(pLsaUserInfo->pszFullName);
        BAIL_ON_NULL_PTR(pWbcUserInfo->full_name, dwErr);
    }

    if (pLsaUserInfo->pszDomain) {
        pWbcUserInfo->domain_name = _wbc_strdup(pLsaUserInfo->pszDomain);
        BAIL_ON_NULL_PTR(pWbcUserInfo->domain_name, dwErr);
    }

    if (pLsaUserInfo->pszDnsDomain) {
        pWbcUserInfo->dns_domain_name = _wbc_strdup(pLsaUserInfo->pszDnsDomain);
        BAIL_ON_NULL_PTR(pWbcUserInfo->dns_domain_name, dwErr);
    }

    pWbcUserInfo->acct_flags = pLsaUserInfo->dwAcctFlags;

    if (pLsaUserInfo->pSessionKey) {        
        memcpy(pWbcUserInfo->user_session_key,
               LsaDataBlobBuffer(pLsaUserInfo->pSessionKey),
               sizeof(pWbcUserInfo->user_session_key));
    }

    if (pLsaUserInfo->pLmSessionKey) {        
        memcpy(pWbcUserInfo->lm_session_key,
               LsaDataBlobBuffer(pLsaUserInfo->pLmSessionKey),
               sizeof(pWbcUserInfo->lm_session_key));
    }    

    pWbcUserInfo->logon_count        = pLsaUserInfo->LogonCount;
    pWbcUserInfo->bad_password_count = pLsaUserInfo->BadPasswordCount;

    pWbcUserInfo->logon_time            = pLsaUserInfo->LogonTime;
    pWbcUserInfo->logoff_time           = pLsaUserInfo->LogoffTime;
    pWbcUserInfo->kickoff_time          = pLsaUserInfo->KickoffTime;
    pWbcUserInfo->pass_last_set_time    = pLsaUserInfo->LastPasswordChange;
    pWbcUserInfo->pass_can_change_time  = pLsaUserInfo->CanChangePassword;
    pWbcUserInfo->pass_must_change_time = pLsaUserInfo->MustChangePassword;

    if (pLsaUserInfo->pszLogonServer) {
        pWbcUserInfo->logon_server = _wbc_strdup(pLsaUserInfo->pszLogonServer);
        BAIL_ON_NULL_PTR(pWbcUserInfo->logon_server, dwErr);
    }

    if (pLsaUserInfo->pszLogonScript) {
        pWbcUserInfo->logon_script = _wbc_strdup(pLsaUserInfo->pszLogonScript);
        BAIL_ON_NULL_PTR(pWbcUserInfo->logon_script, dwErr);
    }

    if (pLsaUserInfo->pszProfilePath) {
        pWbcUserInfo->profile_path = _wbc_strdup(pLsaUserInfo->pszProfilePath);
        BAIL_ON_NULL_PTR(pWbcUserInfo->profile_path, dwErr);
    }

    if (pLsaUserInfo->pszHomeDirectory) {
        pWbcUserInfo->home_directory = _wbc_strdup(pLsaUserInfo->pszHomeDirectory);
        BAIL_ON_NULL_PTR(pWbcUserInfo->home_directory, dwErr);
    }

    if (pLsaUserInfo->pszHomeDrive) {
        pWbcUserInfo->home_drive = _wbc_strdup(pLsaUserInfo->pszHomeDrive);
        BAIL_ON_NULL_PTR(pWbcUserInfo->home_drive, dwErr);
    }

    /* Copy the SIDs (include the user and primary group sids here) */

    pWbcUserInfo->num_sids = pLsaUserInfo->dwNumRids  + pLsaUserInfo->dwNumSids + 2;

    pWbcUserInfo->sids = _wbc_malloc_zero(sizeof(struct wbcSidWithAttr) *
                          pWbcUserInfo->num_sids,
                          NULL);
    BAIL_ON_NULL_PTR(pWbcUserInfo->sids, dwErr);


    dwErr = wbcStringToSid(pLsaUserInfo->pszDomainSid, &DomainSid);
    BAIL_ON_LSA_ERR(dwErr);

    /* User SID must be first */

    dwErr = wbcSidCopy(&(pWbcUserInfo->sids[dwSidCount].sid),
                       &DomainSid);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = wbcSidAppendRid(&(pWbcUserInfo->sids[dwSidCount].sid),
                            pLsaUserInfo->dwUserRid);
    BAIL_ON_LSA_ERR(dwErr);
    dwSidCount++;

    /* Primary group SID is second */

    dwErr = wbcSidCopy(&(pWbcUserInfo->sids[dwSidCount].sid),
                       &DomainSid);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = wbcSidAppendRid(&(pWbcUserInfo->sids[dwSidCount].sid),
                            pLsaUserInfo->dwPrimaryGroupRid);
    BAIL_ON_LSA_ERR(dwErr);
    dwSidCount++;

    /* Copy RIDs */

    for (i=0;
         (i<pLsaUserInfo->dwNumRids) && (dwSidCount<pWbcUserInfo->num_sids);
         i++, dwSidCount++)
    {
        dwErr = wbcSidCopy(&(pWbcUserInfo->sids[dwSidCount].sid),
                           &DomainSid);
        BAIL_ON_LSA_ERR(dwErr);

        dwErr = wbcSidAppendRid(&(pWbcUserInfo->sids[dwSidCount].sid),
                                pLsaUserInfo->pRidAttribList[i].Rid);
        BAIL_ON_LSA_ERR(dwErr);

        pWbcUserInfo->sids[dwSidCount].attributes = pLsaUserInfo->pRidAttribList[i].dwAttrib;
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Copy Other SIDs */

    for (i=0;
         (i<pLsaUserInfo->dwNumSids) && (dwSidCount<pWbcUserInfo->num_sids);
         i++, dwSidCount++)
    {
        dwErr = wbcStringToSid(pLsaUserInfo->pSidAttribList[i].pszSid,
                               &(pWbcUserInfo->sids[dwSidCount].sid));
        BAIL_ON_LSA_ERR(dwErr);

        pWbcUserInfo->sids[dwSidCount].attributes = pLsaUserInfo->pSidAttribList[i].dwAttrib;
        BAIL_ON_LSA_ERR(dwErr);
    }

    dwErr = LW_ERROR_SUCCESS;

cleanup:
    return dwErr;
}

/******************************************************************
 */

static PCSTR
BuildDomainAccountName(
    PCSTR pszDomain,
    PCSTR pszAccountName
    )
{
    PSTR pszFullname = NULL;
    DWORD dwLen = 0;

    if (!pszDomain || !pszAccountName) {
        return NULL;
    }

    /* Include space for '\' and terminating NULL */

    dwLen = strlen(pszDomain) + strlen(pszAccountName) + 2;
    pszFullname = _wbc_malloc(dwLen, NULL);

    snprintf(pszFullname, dwLen, "%s\\%s", pszDomain, pszAccountName);

    return pszFullname;
}

/******************************************************************
 */

static int
FreeWbcUserInfo(
    void *p
    )
{
    struct wbcAuthUserInfo *info = (struct wbcAuthUserInfo*)p;

    _WBC_FREE(info->account_name);
    _WBC_FREE(info->user_principal);
    _WBC_FREE(info->full_name);
    _WBC_FREE(info->domain_name);
    _WBC_FREE(info->dns_domain_name);

    _WBC_FREE(info->logon_server);
    _WBC_FREE(info->logon_script);
    _WBC_FREE(info->profile_path);
    _WBC_FREE(info->home_directory);
    _WBC_FREE(info->home_drive);

    _WBC_FREE(info->sids);

    return 0;
}

static int
FreeWbcErrorInfo(
    void *p
    )
{
    struct wbcAuthErrorInfo *e = (struct wbcAuthErrorInfo*)p;

    if (e == NULL)
        return 0;

    _WBC_FREE(e->nt_string);
    _WBC_FREE(e->display_string);

    return 0;
}


static NTSTATUS
MapLsaErrorToNtStatus(
    DWORD LsaError
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    switch(LsaError)
    {
    case LW_ERROR_SUCCESS:
        ntStatus = STATUS_SUCCESS;
        break;
    case LW_ERROR_LOGON_FAILURE:
        ntStatus = STATUS_LOGON_FAILURE;
        break;
    case LW_ERROR_PASSWORD_EXPIRED:
        ntStatus = STATUS_PASSWORD_EXPIRED;
        break;
    case LW_ERROR_ACCOUNT_EXPIRED:
        ntStatus = STATUS_ACCOUNT_EXPIRED;        
        break;
    case LW_ERROR_ACCOUNT_LOCKED:
        ntStatus = STATUS_ACCOUNT_LOCKED_OUT;
        break;
    case LW_ERROR_ACCOUNT_DISABLED:
        ntStatus = STATUS_ACCOUNT_DISABLED;
        break;
    default:
        break;        
    }

    return ntStatus;    
}

DWORD
wbcFillErrorInfo(
    DWORD dwError,
    struct wbcAuthErrorInfo **ppWbcError
    )
{
    DWORD dwErr = LW_ERROR_INTERNAL;
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    struct wbcAuthErrorInfo *pError = NULL;

    pError = _wbc_malloc_zero(sizeof(struct wbcAuthErrorInfo),
                  FreeWbcErrorInfo);
    BAIL_ON_NULL_PTR(pError, dwErr);
    
    /* Fill in errors here */

    ntStatus = MapLsaErrorToNtStatus(dwError);    

    pError->nt_status = ntStatus;    

    *ppWbcError = pError;

cleanup:
    return dwErr;
}


wbcErr
wbcAuthenticateUserEx(
    const struct wbcAuthUserParams *params,
    struct wbcAuthUserInfo **info,
    struct wbcAuthErrorInfo **error
    )
{
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    LSA_AUTH_USER_PARAMS *pLsaParams = NULL;
    LSA_AUTH_USER_INFO *pLsaUserInfo = NULL;
    struct wbcAuthUserInfo *pWbcUserInfo = NULL;

    /* Sanity and setup */

    BAIL_ON_NULL_PTR_PARAM(params, dwErr);
    BAIL_ON_NULL_PTR_PARAM(params->account_name, dwErr);

    dwErr = LwAllocateMemory(sizeof(LSA_AUTH_USER_PARAMS), (PVOID*)&pLsaParams);
    BAIL_ON_LSA_ERR(dwErr);

    /* Open connection to the server and get moving */

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = InitLsaAuthParams(pLsaParams, params);
    BAIL_ON_LSA_ERR(dwErr);

    switch (pLsaParams->AuthType)
    {
    case LSA_AUTH_PLAINTEXT:
    {
        PCSTR pszFullUsername = NULL;

        /* We need the fully qualified name here */

        pszFullUsername = BuildDomainAccountName(pLsaParams->pszDomain,
                             pLsaParams->pszAccountName);
        BAIL_ON_NULL_PTR(pszFullUsername, dwErr);

        dwErr = LsaAuthenticateUser(hLsa,
                        pszFullUsername,
                        pLsaParams->pass.clear.pszPassword,
                        NULL);
        _WBC_FREE(pszFullUsername);
        BAIL_ON_LSA_ERR(dwErr);
        break;
    }

    case LSA_AUTH_CHAP:
    {
        dwErr = LsaAuthenticateUserEx(hLsa,
                          NULL,
                          pLsaParams,
                          &pLsaUserInfo);
        BAIL_ON_LSA_ERR(dwErr);
        break;
    }
    }

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    /* Copy the out parms now if we have an out pointer */

    if (!info || !pLsaUserInfo->pszAccount) {
        goto cleanup;
    }

    pWbcUserInfo = _wbc_malloc_zero(sizeof(struct wbcAuthUserInfo),
                    FreeWbcUserInfo);

    dwErr = CopyLsaUserInfoToWbcInfo(pWbcUserInfo, pLsaUserInfo);
    BAIL_ON_LSA_ERR(dwErr);

    /* Copy OUT params */
    *info = pWbcUserInfo;

    wbcFillErrorInfo(dwErr, error);


cleanup:
    if (hLsa) {
        LsaCloseServer(hLsa);
        hLsa = (HANDLE)NULL;
    }

    LsaFreeAuthUserInfo(&pLsaUserInfo);
    LsaFreeAuthUserParams(&pLsaParams);

    if (!LW_ERROR_IS_OK(dwErr)) {
        _WBC_FREE(pWbcUserInfo);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

wbcErr
wbcLogonUser(
    const struct wbcLogonUserParams *params,
    struct wbcLogonUserInfo **info,
    struct wbcAuthErrorInfo **error,
    struct wbcUserPasswordPolicyInfo **policy
)
{
    // This function is only used by pam_winbind. The user should use pam_lsass instead

    if (info != NULL)
    {
        *info = NULL;
    }
    if (error != NULL)
    {
        *error = NULL;
    }
    if (policy != NULL)
    {
        *policy = NULL;
    }
    return LW_ERROR_NOT_IMPLEMENTED;
}

wbcErr
wbcLogoffUser(
    const char *username,
    uid_t uid,
    const char *ccfilename
    )
{
    // This function is only used by pam_winbind. The user should use pam_lsass instead

    return LW_ERROR_NOT_IMPLEMENTED;
}

wbcErr
wbcLogoffUserEx(
    const struct wbcLogoffUserParams *params,
    struct wbcAuthErrorInfo **error
    )
{
    // This function is only used by pam_winbind. The user should use pam_lsass instead

    if (error != NULL)
    {
        *error = NULL;
    }
    return LW_ERROR_NOT_IMPLEMENTED;
}

wbcErr
wbcChangeUserPassword(
    const char *username,
    const char *old_password,
    const char *new_password
    )
{
    // Nothing calls this function

    return LW_ERROR_NOT_IMPLEMENTED;
}

wbcErr
wbcChangeUserPasswordEx(
    const struct wbcChangePasswordParams *params,
    struct wbcAuthErrorInfo **error,
    enum wbcPasswordChangeRejectReason *reject_reason,
    struct wbcUserPasswordPolicyInfo **policy
    )
{
    // This function is only used by pam_winbind. The user should use pam_lsass instead

    if (error != NULL)
    {
        *error = NULL;
    }
    if (reject_reason != NULL)
    {
        *reject_reason = 0;
    }
    if (policy != NULL)
    {
        *policy = NULL;
    }
    return LW_ERROR_NOT_IMPLEMENTED;
}

wbcErr
wbcCredentialCache(
    struct wbcCredentialCacheParams *params,
    struct wbcCredentialCacheInfo **info,
    struct wbcAuthErrorInfo **error
    )
{
    // Nothing calls this function

    if (info != NULL)
    {
        *info = NULL;
    }
    if (error != NULL)
    {
        *error = NULL;
    }
    return LW_ERROR_NOT_IMPLEMENTED;
}

wbcErr
wbcCredentialSave(
    const char *user,
    const char *password
    )
{
    return LW_ERROR_NOT_IMPLEMENTED;
}

wbcErr
wbcAddNamedBlob(
    size_t *num_blobs,
    struct wbcNamedBlob **blobs,
    const char *name,
    uint32_t flags,
    uint8_t *data,
    size_t length
    )
{
    // This function is only used by pam_winbind. The user should use pam_lsass instead

    return LW_ERROR_NOT_IMPLEMENTED;
}

