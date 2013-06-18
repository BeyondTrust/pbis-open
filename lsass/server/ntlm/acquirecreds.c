/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        acquirecreds.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        AcquireCredentialsHandle client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerAcquireCredentialsHandle(
    IN LWMsgCall* pCall,
    IN const SEC_CHAR* pszPrincipal,
    IN const SEC_CHAR* pszPackage,
    IN DWORD fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    OUT PNTLM_CRED_HANDLE phCredential,
    OUT PTimeStamp ptsExpiry
    )
{
    DWORD dwError = 0;
    PSEC_WINNT_AUTH_IDENTITY pSecWinAuthData = pAuthData;
    LSA_CRED_HANDLE LsaCredHandle = NULL;
    PNTLM_CREDENTIALS pNtlmCreds = NULL;
    PSTR pUserName = NULL;
    PSTR pNT4UserName = NULL;
    PSTR pPassword = NULL;
    uid_t InvalidUid = -1;

    // Per Windows behavior, ignore pszPrincipal

    // While it is true that 0 is the id for root, for now we don't store root
    // credentials in our list so we can use it as an invalid value
    uid_t Uid = (uid_t)0;
    gid_t Gid = (gid_t)0;

    *phCredential = NULL;
    *ptsExpiry = 0;

    // For the moment, we're not going to worry about fCredentialUse... it
    // will not effect anything at this point (though we do want to track the
    // information it provides).

    if (strcmp("NTLM", pszPackage))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (fCredentialUse == NTLM_CRED_OUTBOUND)
    {
        dwError = NtlmGetProcessSecurity(pCall, &Uid, &Gid);
        BAIL_ON_LSA_ERROR(dwError);

        if (!pSecWinAuthData)
        {
            LsaCredHandle = LsaGetCredential(Uid);

            if (!LsaCredHandle)
            {
                dwError = LW_ERROR_NO_CRED;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else
        {
            dwError = LwStrndup(
                pSecWinAuthData->Password,
                pSecWinAuthData->PasswordLength,
                &pPassword);
            BAIL_ON_LSA_ERROR(dwError);

            if (pSecWinAuthData->UserLength)
            {
                if (pSecWinAuthData->DomainLength)
                {
                    dwError = LwAllocateStringPrintf(
                        &pUserName,
                        "%.*s\\%.*s",
                        (int)pSecWinAuthData->DomainLength,
                        pSecWinAuthData->Domain,
                        (int)pSecWinAuthData->UserLength,
                        pSecWinAuthData->User);
                    BAIL_ON_LSA_ERROR(dwError);
                }
                else
                {
                    dwError = LwStrndup(
                        pSecWinAuthData->User,
                        pSecWinAuthData->UserLength,
                        &pUserName);
                    BAIL_ON_LSA_ERROR(dwError);
                }
            }
            else
            {
                // This is an anonymous cred.
                dwError = LwAllocateString("", &pUserName);
                BAIL_ON_LSA_ERROR(dwError);
            }

            // In theory, we probably *shouldn't* add this to the list...
            // but noone should ever be searching the list for -1...
            // so we should be ok.
            dwError = LsaAddCredential(
                pUserName,
                pPassword,
                &InvalidUid,
                &LsaCredHandle);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = NtlmCreateCredential(
        &LsaCredHandle,
        fCredentialUse,
        &pNtlmCreds);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SECURE_FREE_STRING(pPassword);
    LW_SAFE_FREE_STRING(pUserName);
    LW_SAFE_FREE_STRING(pNT4UserName);

    *phCredential = pNtlmCreds;

    return(dwError);
error:

    // If there's an Ntlm cred, the Lsa cred is a part of it, otherwise, just
    // try to free the Lsa cred.
    if (pNtlmCreds)
    {
        NtlmReleaseCredential(&pNtlmCreds);
    }
    else
    {
        LsaReleaseCredential(&LsaCredHandle);
    }

    *ptsExpiry = 0;
    goto cleanup;
}

DWORD
NtlmGetNameInformation(
    IN OPTIONAL PCSTR pszJoinedDomain,
    OUT PSTR* ppszServerName,
    OUT PSTR* ppszDomainName,
    OUT PSTR* ppszDnsServerName,
    OUT PSTR* ppszDnsDomainName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    CHAR FullDomainName[HOST_NAME_MAX + 1];
    PCHAR pSymbol = NULL;
    PSTR pServerName = NULL;
    PSTR pDomainName = NULL;
    PSTR pDnsServerName = NULL;
    PSTR pDnsDomainName = NULL;
    PSTR pName = NULL;
    struct hostent* pHost = NULL;
    DWORD dwHostSize = 0;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    dwError = LsaSrvProviderGetMachineAccountInfoA(
                  LSA_PROVIDER_TAG_AD,
                  pszJoinedDomain,
                  &pAccountInfo);
    if (dwError == LW_ERROR_SUCCESS)
    {
        dwError = LwAllocateString(
                      pAccountInfo->SamAccountName,
                      &pServerName);
        BAIL_ON_LSA_ERROR(dwError);

        // Remove trailing $
        pServerName[strlen(pServerName) - 1] = 0;

        dwError = LwAllocateString(
                      pAccountInfo->NetbiosDomainName,
                      &pDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateString(
                      pAccountInfo->Fqdn,
                      &pDnsServerName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateString(
                      pAccountInfo->DnsDomainName,
                      &pDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        // Fall back to DNS
        dwError = gethostname(FullDomainName, HOST_NAME_MAX);
        if (dwError)
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }

        // There must be a better way to get this information.
        pHost = gethostbyname(FullDomainName);
        if (pHost)
        {
            dwHostSize = strlen((PSTR)pHost->h_name);
            if (dwHostSize > HOST_NAME_MAX)
            {
                dwHostSize = HOST_NAME_MAX;
            }

            memcpy(
                FullDomainName,
                (PSTR)pHost->h_name,
                dwHostSize);
            FullDomainName[dwHostSize] = 0;
        }

        // This is a horrible fall back, but it's all we've got
        pName = (PSTR)FullDomainName;

        dwError = LwAllocateString(pName, &pDnsServerName);
        BAIL_ON_LSA_ERROR(dwError);

        pSymbol = strchr(pName, '.');

        if (pSymbol)
        {
            *pSymbol = '\0';
        }

        dwError = LwAllocateString(pName, &pServerName);
        BAIL_ON_LSA_ERROR(dwError);

        LwStrToUpper(pServerName);

        if (pSymbol)
        {
            pSymbol++;
            pName = (PSTR)pSymbol;
        }

        dwError = LwAllocateString(pName, &pDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        pSymbol = strchr(pName, '.');

        if (pSymbol)
        {
            *pSymbol = '\0';
        }

        dwError = LwAllocateString(pName, &pDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        LwStrToUpper(pDomainName);
    }

cleanup:
    if (ppszServerName)
    {
        *ppszServerName = pServerName;
    }
    else
    {
        LW_SAFE_FREE_STRING(pServerName);
    }
    if (ppszDomainName)
    {
        *ppszDomainName = pDomainName;
    }
    else
    {
        LW_SAFE_FREE_STRING(pDomainName);
    }
    if (ppszDnsServerName)
    {
        *ppszDnsServerName = pDnsServerName;
    }
    else
    {
        LW_SAFE_FREE_STRING(pDnsServerName);
    }
    if (ppszDnsDomainName)
    {
        *ppszDnsDomainName = pDnsDomainName;
    }
    else
    {
        LW_SAFE_FREE_STRING(pDnsDomainName);
    }

    LsaSrvFreeMachineAccountInfoA(pAccountInfo);

    return dwError;
error:
    LW_SAFE_FREE_STRING(pServerName);
    LW_SAFE_FREE_STRING(pDomainName);
    LW_SAFE_FREE_STRING(pDnsServerName);
    LW_SAFE_FREE_STRING(pDnsDomainName);
    goto cleanup;
}

DWORD
NtlmGetProcessSecurity(
    IN LWMsgCall* pCall,
    OUT uid_t* pUid,
    OUT gid_t* pGid
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    uid_t uid = (uid_t) -1;
    gid_t gid = (gid_t) -1;

    LWMsgSession* pSession = lwmsg_call_get_session(pCall);
    LWMsgSecurityToken* token = lwmsg_session_get_peer_security_token(pSession);

    if (token == NULL || strcmp(lwmsg_security_token_get_type(token), "local"))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_local_token_get_eid(token, &uid, &gid));
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *pUid = uid;
    *pGid = gid;

    return dwError;
error:
    uid = (uid_t) -1;
    gid = (gid_t) -1;
    goto cleanup;

}
