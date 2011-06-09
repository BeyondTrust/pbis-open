/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        queryctxt.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        QueryContextAttributes client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

static
DWORD
NtlmServerQueryCtxtPacLogonInfoAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_PacLogonInfo *ppLogonInfo
    );

static
DWORD
NtlmServerMarshalUserInfoToEncodedPac(
    IN PLSA_AUTH_USER_INFO pUserInfo,
    OUT PDWORD pdwEncodedPacSize,
    OUT PBYTE* ppEncodedPac
    );

static
DWORD
NtlmServerQueryCtxtFlagsAttribute(
    IN  PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_Flags *ppFlagsInfo
    );

static
DWORD
NtlmServerQueryCtxtMappedToGuestAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_MappedToGuest* ppMappedToGuestInfo
    );

DWORD
NtlmServerQueryContextAttributes(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSecPkgContext pContext = (PSecPkgContext)pBuffer;

    switch(ulAttribute)
    {
    case SECPKG_ATTR_NAMES:
        dwError = NtlmServerQueryCtxtNameAttribute(
            phContext,
            &pContext->pNames);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case SECPKG_ATTR_SESSION_KEY:
        dwError = NtlmServerQueryCtxtSessionKeyAttribute(
            phContext,
            &pContext->pSessionKey);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case SECPKG_ATTR_SIZES:
        dwError = NtlmServerQueryCtxtSizeAttribute(
            phContext,
            &pContext->pSizes);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case SECPKG_ATTR_PAC_LOGON_INFO:
        dwError = NtlmServerQueryCtxtPacLogonInfoAttribute(
            phContext,
            &pContext->pLogonInfo);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case SECPKG_ATTR_FLAGS:
        dwError = NtlmServerQueryCtxtFlagsAttribute(
            phContext,
            &pContext->pFlags);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case SECPKG_ATTR_CUSTOM_MAPPED_TO_GUEST:
        dwError = NtlmServerQueryCtxtMappedToGuestAttribute(
            phContext,
            &pContext->pMappedToGuest);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case SECPKG_ATTR_ACCESS_TOKEN:
    case SECPKG_ATTR_AUTHORITY:
    case SECPKG_ATTR_CLIENT_SPECIFIED_TARGET:
    case SECPKG_ATTR_DCE_INFO:
    case SECPKG_ATTR_KEY_INFO:
    case SECPKG_ATTR_LAST_CLIENT_TOKEN_STATUS:
    case SECPKG_ATTR_LIFESPAN:
    case SECPKG_ATTR_LOCAL_CRED:
    case SECPKG_ATTR_NATIVE_NAMES:
    case SECPKG_ATTR_NEGOTIATION_INFO:
    case SECPKG_ATTR_PACKAGE_INFO:
    case SECPKG_ATTR_PASSWORD_EXPIRY:
    case SECPKG_ATTR_ROOT_STORE:
    case SECPKG_ATTR_TARGET_INFORMATION:
        dwError = LW_ERROR_NOT_IMPLEMENTED;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmServerQueryCtxtNameAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_Names *ppNames
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    SEC_CHAR* pFullName = NULL;
    PSecPkgContext_Names pName = NULL;

    *ppNames = NULL;

    if (*phContext == NULL || (*phContext)->NtlmState != NtlmStateResponse)
    {
        dwError = LW_ERROR_INVALID_CONTEXT;
        BAIL_ON_LSA_ERROR(dwError);
    }


    dwError = LwAllocateMemory(sizeof(*pName), OUT_PPVOID(&pName));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
        (*phContext)->pszClientUsername,
        &pFullName);
    BAIL_ON_LSA_ERROR(dwError);

    pName->pUserName = pFullName;

cleanup:
    *ppNames = pName;
    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pName);
    LW_SAFE_FREE_MEMORY(pFullName);
    goto cleanup;
}

DWORD
NtlmServerQueryCtxtSessionKeyAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_SessionKey *ppSessionKey
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_STATE State = NtlmStateBlank;
    PBYTE pKey = NULL;
    PSecPkgContext_SessionKey pSessionKey = NULL;

    *ppSessionKey = NULL;

    dwError = LwAllocateMemory(sizeof(*pSessionKey), OUT_PPVOID(&pSessionKey));
    BAIL_ON_LSA_ERROR(dwError);

    NtlmGetContextInfo(
        *phContext,
        &State,
        NULL,
        &pKey,
        NULL,
        NULL);

    if(State != NtlmStateResponse)
    {
        dwError = LW_ERROR_INVALID_CONTEXT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
        NTLM_SESSION_KEY_SIZE,
        OUT_PPVOID(&pSessionKey->pSessionKey));
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pSessionKey->pSessionKey, pKey, NTLM_SESSION_KEY_SIZE);
    pSessionKey->SessionKeyLength = NTLM_SESSION_KEY_SIZE;

cleanup:
    *ppSessionKey = pSessionKey;
    return dwError;
error:
    if(pSessionKey)
    {
        LW_SAFE_FREE_MEMORY(pSessionKey->pSessionKey);
    }
    LW_SAFE_FREE_MEMORY(pSessionKey);
    goto cleanup;
}

static
DWORD
NtlmServerQueryCtxtPacLogonInfoAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_PacLogonInfo *ppLogonInfo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_STATE State = NtlmStateBlank;
    PSecPkgContext_PacLogonInfo pLogonInfo = NULL;
    PLSA_AUTH_USER_INFO pUserInfo = ((PNTLM_CONTEXT) *phContext)->pUserInfo;

    *ppLogonInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pLogonInfo), OUT_PPVOID(&pLogonInfo));
    BAIL_ON_LSA_ERROR(dwError);

    NtlmGetContextInfo(
        *phContext,
        &State,
        NULL,
        NULL,
        NULL,
        NULL);

    if (State != NtlmStateResponse)
    {
        dwError = LW_ERROR_INVALID_CONTEXT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo == NULL)
    {
        dwError = ERROR_BAD_LOGON_SESSION_STATE;
        BAIL_ON_LSA_ERROR(dwError);
    }


    dwError = NtlmServerMarshalUserInfoToEncodedPac(
        ((PNTLM_CONTEXT) *phContext)->pUserInfo,
        &pLogonInfo->LogonInfoLength,
        &pLogonInfo->pLogonInfo);
    BAIL_ON_LSA_ERROR(dwError);

    *ppLogonInfo = pLogonInfo;

cleanup:

    return dwError;

error:

    if (pLogonInfo)
    {
        LW_SAFE_FREE_MEMORY(pLogonInfo->pLogonInfo);
        LW_SAFE_FREE_MEMORY(pLogonInfo);
    }

    goto cleanup;
}

static
DWORD
NtlmServerQueryCtxtFlagsAttribute(
    IN  PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_Flags *ppFlagsInfo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSecPkgContext_Flags pFlagsInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pFlagsInfo),
                               OUT_PPVOID(&pFlagsInfo));
    BAIL_ON_LSA_ERROR(dwError);

    NtlmGetContextInfo(
        *phContext,
        NULL,
        &(pFlagsInfo->Flags),
        NULL,
        NULL,
        NULL);

    *ppFlagsInfo = pFlagsInfo;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pFlagsInfo);
    *ppFlagsInfo = NULL;

    goto cleanup;
}

DWORD
NtlmServerQueryCtxtSizeAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_Sizes *ppSizes
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSecPkgContext_Sizes pSizes = NULL;

    *ppSizes = NULL;

    dwError = LwAllocateMemory(sizeof(*pSizes), OUT_PPVOID(&pSizes));
    BAIL_ON_LSA_ERROR(dwError);

    // The Challenge message is easily the largest token we send
    pSizes->cbMaxToken =
        sizeof(NTLM_CHALLENGE_MESSAGE) +
        NTLM_LOCAL_CONTEXT_SIZE +
        sizeof(NTLM_SEC_BUFFER) +
        NTLM_WIN_SPOOF_SIZE +
        (HOST_NAME_MAX * 5) +
        (sizeof(NTLM_TARGET_INFO_BLOCK) * 5);

    pSizes->cbMaxSignature = NTLM_SIGNATURE_SIZE;
    pSizes->cbBlockSize = 1;
    pSizes->cbSecurityTrailer = NTLM_PADDING_SIZE;

cleanup:
    *ppSizes = pSizes;
    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pSizes);
    goto cleanup;
}

static
DWORD
NtlmSetUnicodeStringEx(
    PUNICODE_STRING pUnicode,
    PCSTR pszStr
    )
{
    DWORD dwError = 0;
    size_t len = 0;

    if (pszStr)
    {
        dwError = LwMbsToWc16s(pszStr, &pUnicode->Buffer);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LwWc16sLen(pUnicode->Buffer, &len);
        BAIL_ON_LSA_ERROR(dwError);
        
        pUnicode->Length = (UINT16) (len * sizeof(WCHAR));
        pUnicode->MaximumLength = (UINT16) ((len + 1) * sizeof(WCHAR));
    }

error:

    return dwError;
}

static
VOID
NtlmFreeUnicodeStringEx(
    PUNICODE_STRING pUnicode
    )
{
    if (pUnicode->Buffer)
    {
        LwFreeMemory(pUnicode->Buffer);
    }

    memset(pUnicode, 0, sizeof(*pUnicode));
}

static
VOID
NtlmSetWinNtTime(
    WinNtTime* pTime,
    INT64 time
    )
{
    pTime->high = (time >> 32);
    pTime->low = time & 0xFFFFFFFF;
}

static
DWORD
NtlmServerMarshalUserInfoToEncodedPac(
    IN PLSA_AUTH_USER_INFO pUserInfo,
    OUT PDWORD pdwEncodedPacSize,
    OUT PBYTE* ppEncodedPac
    )
{
    DWORD dwError = 0;
    PAC_LOGON_INFO pac;
    NetrSamInfo3* pInfo3 = &pac.info3;
    DWORD i = 0;

    memset(&pac, 0, sizeof(pac));

    NtlmSetWinNtTime(&pInfo3->base.last_logon, pUserInfo->LogonTime);
    NtlmSetWinNtTime(&pInfo3->base.last_logoff, pUserInfo->LogoffTime);
    NtlmSetWinNtTime(&pInfo3->base.acct_expiry, pUserInfo->KickoffTime);
    NtlmSetWinNtTime(&pInfo3->base.last_password_change, pUserInfo->LastPasswordChange);
    NtlmSetWinNtTime(&pInfo3->base.allow_password_change, pUserInfo->CanChangePassword);
    NtlmSetWinNtTime(&pInfo3->base.force_password_change, pUserInfo->MustChangePassword);
    pInfo3->base.logon_count = pUserInfo->LogonCount;
    pInfo3->base.bad_password_count = pUserInfo->BadPasswordCount;
    pInfo3->base.rid = pUserInfo->dwUserRid;
    pInfo3->base.primary_gid = pUserInfo->dwPrimaryGroupRid;
    pInfo3->base.user_flags = pUserInfo->dwUserFlags;
    pInfo3->base.acct_flags = pUserInfo->dwAcctFlags;
    
    pInfo3->base.groups.dwCount = pUserInfo->dwNumRids;
    pInfo3->base.groups.pRids = (PRID_WITH_ATTRIBUTE) pUserInfo->pRidAttribList;

    memcpy(pInfo3->base.key.key, pUserInfo->pSessionKey->pData, sizeof(pInfo3->base.key.key));
    if (pUserInfo->pLmSessionKey)
        memcpy(pInfo3->base.lmkey.key, pUserInfo->pLmSessionKey->pData, sizeof(pInfo3->base.lmkey.key));
    else
        memset(pInfo3->base.lmkey.key, 0, sizeof(pInfo3->base.lmkey.key));

    dwError = NtlmSetUnicodeStringEx(
        &pInfo3->base.account_name,
        pUserInfo->pszAccount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmSetUnicodeStringEx(
        &pInfo3->base.full_name,
        pUserInfo->pszFullName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmSetUnicodeStringEx(
        &pInfo3->base.logon_script,
        pUserInfo->pszLogonScript);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmSetUnicodeStringEx(
        &pInfo3->base.profile_path,
        pUserInfo->pszProfilePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmSetUnicodeStringEx(
        &pInfo3->base.home_directory,
        pUserInfo->pszHomeDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmSetUnicodeStringEx(
        &pInfo3->base.home_drive,
        pUserInfo->pszHomeDrive);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmSetUnicodeStringEx(
        &pInfo3->base.logon_server,
        pUserInfo->pszLogonServer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmSetUnicodeStringEx(
        &pInfo3->base.domain,
        pUserInfo->pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlAllocateSidFromCString(
            &pInfo3->base.domain_sid,
            pUserInfo->pszDomainSid));
    BAIL_ON_LSA_ERROR(dwError);

    pInfo3->sidcount = pUserInfo->dwNumSids;

    if (pInfo3->sidcount)
    {
        dwError = LwAllocateMemory(pInfo3->sidcount * sizeof(NetrSidAttr), OUT_PPVOID(&pInfo3->sids));
        BAIL_ON_LSA_ERROR(dwError);
        
        for (i = 0; i < pInfo3->sidcount; i++)
        {
            pInfo3->sids[i].attribute = pUserInfo->pSidAttribList[i].dwAttrib;
            dwError = LwNtStatusToWin32Error(
                RtlAllocateSidFromCString(
                    &pInfo3->sids[i].sid,
                    pUserInfo->pSidAttribList[i].pszSid));
            BAIL_ON_LSA_ERROR(dwError);
        }

    }
    
    if (EncodePacLogonInfo(
            &pac,
            pdwEncodedPacSize,
            OUT_PPVOID(ppEncodedPac)) != 0)
    {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    NtlmFreeUnicodeStringEx(&pInfo3->base.account_name);
    NtlmFreeUnicodeStringEx(&pInfo3->base.full_name);
    NtlmFreeUnicodeStringEx(&pInfo3->base.logon_script);
    NtlmFreeUnicodeStringEx(&pInfo3->base.profile_path);
    NtlmFreeUnicodeStringEx(&pInfo3->base.home_directory);
    NtlmFreeUnicodeStringEx(&pInfo3->base.home_drive);
    NtlmFreeUnicodeStringEx(&pInfo3->base.logon_server);
    NtlmFreeUnicodeStringEx(&pInfo3->base.domain);

    RTL_FREE(&pInfo3->base.domain_sid);

    for (i = 0; i < pInfo3->sidcount; i++)
    {
        RTL_FREE(&pInfo3->sids[i].sid);
    }

    RTL_FREE(&pInfo3->sids);

    return dwError;

error:

    *pdwEncodedPacSize = 0;
    *ppEncodedPac = NULL;

    goto cleanup;
}

static
DWORD
NtlmServerQueryCtxtMappedToGuestAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_MappedToGuest* ppMappedToGuestInfo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSecPkgContext_MappedToGuest pMappedToGuestInfo = NULL;

    dwError = LwAllocateMemory(sizeof(*pMappedToGuestInfo),
                               OUT_PPVOID(&pMappedToGuestInfo));
    BAIL_ON_LSA_ERROR(dwError);

    NtlmGetContextInfo(
        *phContext,
        NULL,
        NULL,
        NULL,
        NULL,
        &pMappedToGuestInfo->MappedToGuest);

    *ppMappedToGuestInfo = pMappedToGuestInfo;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pMappedToGuestInfo);
    *ppMappedToGuestInfo = NULL;

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
