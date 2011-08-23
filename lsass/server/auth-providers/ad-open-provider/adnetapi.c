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
 *        adnetapi.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Wrappers for calls to NETAPI
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Gerald Carter <gcarter@likewise.com>
 */

#include "adprovider.h"
#include "adnetapi.h"

typedef struct _LSA_SCHANNEL_STATE {
    NetrCredentials SchannelCreds;
    NetrCredentials *pSchannelCreds;
    NETR_BINDING hSchannelBinding;
    PSTR pszSchannelServer;
    pthread_mutex_t SchannelLock;
    pthread_mutex_t *pSchannelLock;
} LSA_SCHANNEL_STATE;

static
BOOLEAN
AD_NtStatusIsTgtRevokedError(
    NTSTATUS status
    );

static
BOOLEAN
AD_NtStatusIsConnectionError(
    NTSTATUS status
    );

static
BOOLEAN
AD_WinErrorIsTgtRevokedError(
    WINERROR winError
    );

static
BOOLEAN
AD_WinErrorIsConnectionError(
    WINERROR winError
    );

static
VOID
AD_ClearSchannelStateInLock(
    IN PLSA_SCHANNEL_STATE pSchannelState
    );

static
DWORD
AD_GetSystemCreds(
    IN PLSA_AD_PROVIDER_STATE pState,
    OUT LW_PIO_CREDS* ppCreds
    )
{
    LW_PIO_CREDS pCreds = NULL;
    DWORD dwError = 0;
    PSTR pszMachPrincipal = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    dwError = LsaPcacheGetMachineAccountInfoA(
                  pState->pPcache,
                  &pAccountInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszMachPrincipal,
                    "%s@%s",
                    pAccountInfo->SamAccountName,
                    pAccountInfo->DnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwIoCreateKrb5CredsA(
                    pszMachPrincipal,
                    pState->MachineCreds.pszCachePath,
                    &pCreds);
    BAIL_ON_LSA_ERROR(dwError);

    *ppCreds = pCreds;

cleanup:

    LW_SAFE_FREE_STRING(pszMachPrincipal);

    LsaPcacheReleaseMachineAccountInfoA(pAccountInfo);

    return dwError;

error:
    *ppCreds = NULL;
    if (pCreds != NULL)
    {
        LwIoDeleteCreds(pCreds);
    }

    goto cleanup;
}

DWORD
AD_SetSystemAccess(
    IN PLSA_AD_PROVIDER_STATE pState,
    OUT OPTIONAL LW_PIO_CREDS* ppOldToken
    )
{
    LW_PIO_CREDS pOldToken = NULL;
    LW_PIO_CREDS pSystemToken = NULL;
    DWORD dwError = 0;

    dwError = AD_GetSystemCreds(
                  pState,
                  &pSystemToken);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppOldToken)
    {
        dwError = LwIoGetThreadCreds(&pOldToken);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwIoSetThreadCreds(pSystemToken);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pSystemToken != NULL)
    {
        LwIoDeleteCreds(pSystemToken);
    }

    if (ppOldToken)
    {
        *ppOldToken = pOldToken;
    }

    return dwError;

error:
    if (pOldToken != NULL)
    {
        LwIoDeleteCreds(pOldToken);
        pOldToken = NULL;
    }

    goto cleanup;
}

DWORD
AD_NetCreateSchannelState(
    OUT PLSA_SCHANNEL_STATE* ppSchannelState
    )
{
    DWORD dwError = 0;
    PLSA_SCHANNEL_STATE pSchannelState = NULL;

    dwError = LwAllocateMemory(
                  sizeof(*pSchannelState),
                  (PVOID*)&pSchannelState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMapErrnoToLwError(pthread_mutex_init(&pSchannelState->SchannelLock, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    pSchannelState->pSchannelLock = &pSchannelState->SchannelLock;

    *ppSchannelState = pSchannelState;

cleanup:

    return dwError;

error:

    *ppSchannelState = NULL;

    if (pSchannelState)
    {
        AD_NetDestroySchannelState(pSchannelState);
    }

    goto cleanup;
}

VOID
AD_NetDestroySchannelState(
    IN PLSA_SCHANNEL_STATE pSchannelState
    )
{
    AD_ClearSchannelStateInLock(pSchannelState);

    if (pSchannelState->pSchannelLock)
    {
        pthread_mutex_destroy(pSchannelState->pSchannelLock);
    }

    LwFreeMemory(pSchannelState);
}

DWORD
AD_NetUserChangePassword(
    PCSTR pszDomainName,
    PCSTR pszLoginId,
    PCSTR pszUserPrincipalName,
    PCSTR pszOldPassword,
    PCSTR pszNewPassword
    )
{
    DWORD dwError = 0;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszLoginId = NULL;
    PWSTR pwszOldPassword = NULL;
    PWSTR pwszNewPassword = NULL;
    PLSA_CREDS_FREE_INFO pFreeInfo = NULL;

    BAIL_ON_INVALID_STRING(pszDomainName);
    BAIL_ON_INVALID_STRING(pszLoginId);

    dwError = LsaSetSMBCreds(
                    pszUserPrincipalName,
                    pszOldPassword,
                    FALSE,
                    &pFreeInfo);
    if (dwError == LW_ERROR_PASSWORD_EXPIRED)
    {
        dwError = LsaSetSMBAnonymousCreds(
                        &pFreeInfo);
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(
                    pszDomainName,
                    &pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(
                    pszLoginId,
                    &pwszLoginId);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszOldPassword)) {

        dwError = LwMbsToWc16s(
                    pszOldPassword,
                    &pwszOldPassword);
        BAIL_ON_LSA_ERROR(dwError);

    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pszNewPassword)) {

        dwError = LwMbsToWc16s(
                    pszNewPassword,
                    &pwszNewPassword);
        BAIL_ON_LSA_ERROR(dwError);

    }

    dwError = LsaUserChangePassword(
                    pwszDomainName,
                    pwszLoginId,
                    pwszOldPassword,
                    pwszNewPassword);
    if (dwError == ERROR_ACCESS_DENIED)
    {
        // Try again using machine credentials
        LsaFreeSMBCreds(&pFreeInfo);

        dwError = LsaUserChangePassword(
                        pwszDomainName,
                        pwszLoginId,
                        pwszOldPassword,
                        pwszNewPassword);
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pwszDomainName);
    LW_SAFE_FREE_MEMORY(pwszLoginId);
    LW_SECURE_FREE_WSTRING(pwszOldPassword);
    LW_SECURE_FREE_WSTRING(pwszNewPassword);
    LsaFreeSMBCreds(&pFreeInfo);

    return AD_MapNetApiError(dwError);

error:

    goto cleanup;
}

// ISSUE-2008/08/25-dalmeida -- This needs to be pulled in by header
// with LsaOpenPolicy2 and LsaLookupNames2 functions.
#ifndef STATUS_UNHANDLED_EXCEPTION
#define STATUS_UNHANDLED_EXCEPTION 0xc0000144
#endif

static
LSA_OBJECT_TYPE
GetObjectType(
    IN LsaSidType Type
    )
{
    LSA_OBJECT_TYPE ObjectType = LSA_OBJECT_TYPE_UNDEFINED;

    switch(Type)
    {
        case SID_TYPE_USER:
            ObjectType = LSA_OBJECT_TYPE_USER;
            break;

        case SID_TYPE_DOM_GRP:
        case SID_TYPE_ALIAS:
        case SID_TYPE_WKN_GRP:
            ObjectType = LSA_OBJECT_TYPE_GROUP;
            break;
        case SID_TYPE_DOMAIN:
            ObjectType = LSA_OBJECT_TYPE_DOMAIN;
            break;

        default:
            ObjectType = LSA_OBJECT_TYPE_UNDEFINED;
    }

    return ObjectType;
}

DWORD
AD_NetLookupObjectSidByName(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszHostname,
    IN PCSTR pszObjectName,
    OUT PSTR* ppszObjectSid,
    OUT LSA_OBJECT_TYPE* pObjectType,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PLSA_TRANSLATED_NAME_OR_SID* ppTranslatedSids = NULL;
    PSTR pszObjectSid = NULL;
    BOOLEAN bIsNetworkError = FALSE;

    dwError = AD_NetLookupObjectSidsByNames(
                 pState,
                 pszHostname,
                 1,
                 (PSTR*)&pszObjectName,
                 &ppTranslatedSids,
                 NULL,
                 &bIsNetworkError);
    BAIL_ON_LSA_ERROR(dwError);

    // In case of NOT found, the above function bails out with dwError == LW_ERROR_RPC_LSA_LOOKUPNAMES_FAILED
    // Double check here again
    if (!ppTranslatedSids || !ppTranslatedSids[0])
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateString(ppTranslatedSids[0]->pszNT4NameOrSid,
                                &pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszObjectSid = pszObjectSid;
    *pObjectType = ppTranslatedSids[0]->ObjectType;

cleanup:
    *pbIsNetworkError = bIsNetworkError;
    if (ppTranslatedSids)
    {
       LsaFreeTranslatedNameList(ppTranslatedSids, 1);
    }
    return dwError;

error:
    *ppszObjectSid = NULL;
    LW_SAFE_FREE_STRING(pszObjectSid);
    *pObjectType = LSA_OBJECT_TYPE_UNDEFINED;
    LSA_LOG_ERROR("Failed to find user, group, or domain by name (name = '%s', searched host = '%s') -> error = %u, symbol = %s",
            LSA_SAFE_LOG_STRING(pszObjectName),
            LSA_SAFE_LOG_STRING(pszHostname),
            dwError,
            LwWin32ExtErrorToName(dwError));
    dwError = LW_ERROR_NO_SUCH_OBJECT;

    goto cleanup;
}

DWORD
AD_NetLookupObjectSidsByNames(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszHostname,
    IN DWORD dwNamesCount,
    IN PSTR* ppszNames,
    OUT PLSA_TRANSLATED_NAME_OR_SID** pppTranslatedSids,
    OUT OPTIONAL PDWORD pdwFoundSidsCount,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PWSTR pwcHost = NULL;
    NTSTATUS status = 0;
    LSA_BINDING lsa_binding = (HANDLE)NULL;
    DWORD dwAccess_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    POLICY_HANDLE hPolicy = NULL;
    DWORD dwLevel;
    DWORD dwFoundSidsCount = 0;
    PWSTR* ppwcNames = NULL;
    RefDomainList* pDomains = NULL;
    TranslatedSid2* pSids = NULL;
    PLSA_TRANSLATED_NAME_OR_SID* ppTranslatedSids = NULL;
    PSID pObject_sid = NULL;
    BOOLEAN bIsNetworkError = FALSE;
    DWORD i = 0;
    LW_PIO_CREDS pCreds = NULL;
    LW_PIO_CREDS pOldToken = NULL;
    BOOLEAN bChangedToken = FALSE;

    BAIL_ON_INVALID_STRING(pszHostname);
    dwError = LwMbsToWc16s(
                  pszHostname,
                  &pwcHost);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_SetSystemAccess(
                  pState,
                  &pOldToken);
    BAIL_ON_LSA_ERROR(dwError);
    bChangedToken = TRUE;

    status = LwIoGetThreadCreds(&pCreds);
    dwError = LwNtStatusToWin32Error(status);
    BAIL_ON_LSA_ERROR(dwError);

    status = LsaInitBindingDefault(&lsa_binding, pwcHost, pCreds);
    if (status != 0)
    {
        LSA_LOG_DEBUG("LsaInitBindingDefault() failed with %u (0x%08x)", status, status);
        dwError = LW_ERROR_RPC_LSABINDING_FAILED;
        bIsNetworkError = TRUE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (lsa_binding == NULL)
    {
        dwError = LW_ERROR_RPC_LSABINDING_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Convert ppszNames to ppwcNames
    dwError = LwAllocateMemory(
                    sizeof(*ppwcNames)*dwNamesCount,
                    (PVOID*)&ppwcNames);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwNamesCount; i++)
    {
        dwError = LwMbsToWc16s(
                      ppszNames[i],
                      &ppwcNames[i]);
        BAIL_ON_LSA_ERROR(dwError);
    }

    status = LsaOpenPolicy2(lsa_binding,
                            pwcHost,
                            NULL,
                            dwAccess_rights,
                            &hPolicy);
    if (status != 0)
    {
        LSA_LOG_DEBUG("LsaOpenPolicy2() failed with %u (0x%08x)", status, status);

        if (AD_NtStatusIsTgtRevokedError(status))
        {
            bIsNetworkError = TRUE;
            dwError = LW_ERROR_KRB5KDC_ERR_TGT_REVOKED;
        }
        else
        {
            if (AD_NtStatusIsConnectionError(status))
            {
                bIsNetworkError = TRUE;
            }

            dwError = LW_ERROR_RPC_OPENPOLICY_FAILED;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Lookup name to sid */
    dwLevel = 1;
    status = LsaLookupNames2(
                   lsa_binding,
                   hPolicy,
                   dwNamesCount,
                   ppwcNames,
                   &pDomains,
                   &pSids,
                   dwLevel,
                   &dwFoundSidsCount);
    if (status != 0)
    {
        if (LW_STATUS_NONE_MAPPED == status)
        {
            // This is ok.
            LSA_LOG_DEBUG("LsaLookupNames2() empty results (0 out of %u)",
                          dwNamesCount);

            dwFoundSidsCount = 0;

            dwError = LwAllocateMemory(
                            sizeof(*ppTranslatedSids)*dwNamesCount,
                            (PVOID*)&ppTranslatedSids);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = 0;
            goto cleanup;
        }
        else if (LW_STATUS_SOME_NOT_MAPPED == status)
        {
            LSA_LOG_DEBUG("LsaLookupNames2() partial results (%u out of %u)",
                          dwFoundSidsCount, dwNamesCount);
            dwError = 0;
        }
        else
        {
            LSA_LOG_DEBUG("LsaLookupNames2() failed with %u (0x%08x)", status, status);

            if (AD_NtStatusIsTgtRevokedError(status))
            {
                bIsNetworkError = TRUE;
                dwError = LW_ERROR_KRB5KDC_ERR_TGT_REVOKED;
            }
            else
            {
                if (AD_NtStatusIsConnectionError(status))
                {
                    bIsNetworkError = TRUE;
                }

                dwError = LW_ERROR_RPC_LSA_LOOKUPNAME2_FAILED;
            }

            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    if (dwFoundSidsCount == 0)
    {
        dwError = LW_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwFoundSidsCount > dwNamesCount)
    {
        dwError = LW_ERROR_RPC_LSA_LOOKUPNAME2_FOUND_DUPLICATES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!pSids || !pDomains)
    {
        dwError = LW_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // For incomplete results (LW_STATUS_SOME_NOT_MAPPED == status), leave ppTranslatedSids[i] as NULL for those NOT found
    // to maintain ppszNames[i] -> ppTranslatedSids[i]
    dwError = LwAllocateMemory(
                    sizeof(*ppTranslatedSids)*dwNamesCount,
                    (PVOID*)&ppTranslatedSids);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwNamesCount; i++)
    {
        LSA_OBJECT_TYPE ObjectType = LSA_OBJECT_TYPE_UNDEFINED;
        DWORD dwDomainSid_index = 0;

        ObjectType = GetObjectType(pSids[i].type);
        if (ObjectType == LSA_OBJECT_TYPE_UNDEFINED)
        {
            continue;
        }

        dwError = LwAllocateMemory(
                            sizeof(*ppTranslatedSids[i]),
                            (PVOID*)&ppTranslatedSids[i]);
        BAIL_ON_LSA_ERROR(dwError);

        ppTranslatedSids[i]->ObjectType = ObjectType;

        dwDomainSid_index = pSids[i].index;

        if (dwDomainSid_index >= pDomains->count)
        {
            dwError = LW_ERROR_RPC_LSA_LOOKUPNAME2_FAILED;
            BAIL_ON_LSA_ERROR(dwError);
        }

        LW_SAFE_FREE_MEMORY(pObject_sid);

        if (LSA_OBJECT_TYPE_DOMAIN == ObjectType)
        {
            dwError = LsaAllocateCStringFromSid(
                            &ppTranslatedSids[i]->pszNT4NameOrSid,
                            pDomains->domains[dwDomainSid_index].sid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LsaAllocateSidAppendRid(
                            &pObject_sid,
                            pDomains->domains[dwDomainSid_index].sid,
                            pSids[i].rid);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAllocateCStringFromSid(
                            &ppTranslatedSids[i]->pszNT4NameOrSid,
                            pObject_sid);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pwcHost);
    if (ppwcNames)
    {
        for (i = 0; i < dwNamesCount; i++)
        {
            LW_SAFE_FREE_MEMORY(ppwcNames[i]);
        }
        LwFreeMemory(ppwcNames);
    }
    if (pDomains)
    {
        LsaRpcFreeMemory(pDomains);
    }
    if (pSids)
    {
        LsaRpcFreeMemory(pSids);
    }
    LW_SAFE_FREE_MEMORY(pObject_sid);
    status = LsaClose(lsa_binding, hPolicy);
    if (status != 0 && dwError == 0)
    {
        LSA_LOG_DEBUG("LsaClose() failed with %u (0x%08x)", status, status);
        dwError = LW_ERROR_RPC_CLOSEPOLICY_FAILED;
    }
    if (lsa_binding)
    {
        LsaFreeBinding(&lsa_binding);
    }
    if (bChangedToken)
    {
        LwIoSetThreadCreds(pOldToken);
    }
    if (pOldToken != NULL)
    {
        LwIoDeleteCreds(pOldToken);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    *pppTranslatedSids = ppTranslatedSids;

    if (pdwFoundSidsCount)
    {
        *pdwFoundSidsCount = dwFoundSidsCount;
    }

    if (pbIsNetworkError)
    {
        *pbIsNetworkError = bIsNetworkError;
    }

    return dwError;

error:
   if (ppTranslatedSids)
   {
       LsaFreeTranslatedNameList(ppTranslatedSids, dwNamesCount);
       ppTranslatedSids = NULL;
   }
   dwFoundSidsCount = 0;

   goto cleanup;
}

DWORD
AD_NetLookupObjectNameBySid(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszHostname,
    IN PCSTR pszObjectSid,
    OUT PSTR* ppszNT4Name,
    OUT LSA_OBJECT_TYPE* pObjectType,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PLSA_TRANSLATED_NAME_OR_SID* ppTranslatedNames = NULL;
    PSTR pszNT4Name = NULL;
    BOOLEAN bIsNetworkError = FALSE;

    dwError = AD_NetLookupObjectNamesBySids(
                 pState,
                 pszHostname,
                 1,
                 (PSTR*)&pszObjectSid,
                 &ppTranslatedNames,
                 NULL,
                 &bIsNetworkError);
    BAIL_ON_LSA_ERROR(dwError);

    // In case of NOT found, the above function bails out with dwError == LW_ERROR_RPC_LSA_LOOKUPSIDS_FAILED
    // Double check here again
    if (!ppTranslatedNames || !ppTranslatedNames[0])
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateString(ppTranslatedNames[0]->pszNT4NameOrSid,
                                &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;
    *pObjectType = ppTranslatedNames[0]->ObjectType;

cleanup:
    *pbIsNetworkError = bIsNetworkError;

    if (ppTranslatedNames)
    {
       LsaFreeTranslatedNameList(ppTranslatedNames, 1);
    }
    return dwError;

error:
    *ppszNT4Name = NULL;
    LW_SAFE_FREE_STRING(pszNT4Name);
    *pObjectType = LSA_OBJECT_TYPE_UNDEFINED;

    LSA_LOG_INFO("Failed to find user, group, or domain by sid (sid = '%s', searched host = '%s') -> error = %u, symbol = %s",
            LSA_SAFE_LOG_STRING(pszObjectSid),
            LSA_SAFE_LOG_STRING(pszHostname),
            dwError,
            LwWin32ExtErrorToName(dwError));

    dwError = LW_ERROR_NO_SUCH_OBJECT;

    goto cleanup;
}

DWORD
AD_NetLookupObjectNamesBySids(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszHostname,
    IN DWORD dwSidsCount,
    IN PSTR* ppszObjectSids,
    OUT PLSA_TRANSLATED_NAME_OR_SID** pppTranslatedNames,
    OUT OPTIONAL PDWORD pdwFoundNamesCount,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PWSTR pwcHost = NULL;
    NTSTATUS status = 0;
    LSA_BINDING lsa_binding = (HANDLE)NULL;
    DWORD dwAccess_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    POLICY_HANDLE hPolicy = NULL;
    SID_ARRAY sid_array  = {0};
    DWORD dwLevel = 1;
    DWORD dwFoundNamesCount = 0;
    RefDomainList* pDomains = NULL;
    PSTR* ppszDomainNames = NULL;
    PSID pObjectSID = NULL;
    TranslatedName* name_array = NULL;
    PSTR pszUsername = NULL;
    PLSA_TRANSLATED_NAME_OR_SID* ppTranslatedNames = NULL;
    BOOLEAN bIsNetworkError = FALSE;
    DWORD i = 0;
    PIO_CREDS pCreds = NULL;
    LW_PIO_CREDS pOldToken = NULL;
    BOOLEAN bChangedToken = FALSE;

    BAIL_ON_INVALID_STRING(pszHostname);

    dwError = LwMbsToWc16s(
                  pszHostname,
                  &pwcHost);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_SetSystemAccess(
                  pState,
                  &pOldToken);
    BAIL_ON_LSA_ERROR(dwError);
    bChangedToken = TRUE;

    status = LwIoGetThreadCreds(&pCreds);
    dwError = LwNtStatusToWin32Error(status);
    BAIL_ON_LSA_ERROR(dwError);

    status = LsaInitBindingDefault(&lsa_binding, pwcHost, pCreds);
    if (status != 0)
    {
        LSA_LOG_DEBUG("LsaInitBindingDefault() failed with %u (0x%08x)", status, status);
        dwError = LW_ERROR_RPC_LSABINDING_FAILED;
        bIsNetworkError = TRUE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (lsa_binding == NULL)
    {
        dwError = LW_ERROR_RPC_LSABINDING_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Convert ppszObjectSids to sid_array
    sid_array.dwNumSids = dwSidsCount;
    dwError = LwAllocateMemory(
                    sizeof(*sid_array.pSids)*sid_array.dwNumSids,
                    (PVOID*)&sid_array.pSids);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < sid_array.dwNumSids; i++)
    {
        dwError = LsaAllocateSidFromCString(
                        &pObjectSID,
                        ppszObjectSids[i]);
        BAIL_ON_LSA_ERROR(dwError);

        sid_array.pSids[i].pSid = pObjectSID;
        pObjectSID = NULL;
    }

    status = LsaOpenPolicy2(lsa_binding,
                            pwcHost,
                            NULL,
                            dwAccess_rights,
                            &hPolicy);
    if (status != 0)
    {
        LSA_LOG_DEBUG("LsaOpenPolicy2() failed with %u (0x%08x)", status, status);

        if (AD_NtStatusIsTgtRevokedError(status))
        {
            bIsNetworkError = TRUE;
            dwError = LW_ERROR_KRB5KDC_ERR_TGT_REVOKED;
        }
        else
        {
            if (AD_NtStatusIsConnectionError(status))
            {
                bIsNetworkError = TRUE;
            }

            dwError = LW_ERROR_RPC_OPENPOLICY_FAILED;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Lookup sid to name */
    status = LsaLookupSids(
                   lsa_binding,
                   hPolicy,
                   &sid_array,
                   &pDomains,
                   &name_array,
                   dwLevel,
                   &dwFoundNamesCount);
    if (status != 0)
    {
        if (LW_STATUS_NONE_MAPPED == status)
        {
            // This is ok.
            LSA_LOG_DEBUG("LsaLookupNames2() empty results (0 out of %u)",
                          dwSidsCount);

            dwFoundNamesCount = 0;

            dwError = LwAllocateMemory(
                            sizeof(*ppTranslatedNames)*dwSidsCount,
                            (PVOID*)&ppTranslatedNames);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = 0;
            goto cleanup;
        }
        else if (LW_STATUS_SOME_NOT_MAPPED == status)
        {
            LSA_LOG_DEBUG("LsaLookupSids() partial results (%u out of %u)",
                          dwFoundNamesCount, dwSidsCount);
            dwError = 0;
        }
        else
        {
            LSA_LOG_DEBUG("LsaLookupSids() failed with %u (0x%08x)", status, status);

            if (AD_NtStatusIsTgtRevokedError(status))
            {
                bIsNetworkError = TRUE;
                dwError = LW_ERROR_KRB5KDC_ERR_TGT_REVOKED;
            }
            else
            {
                if (AD_NtStatusIsConnectionError(status))
                {
                    bIsNetworkError = TRUE;
                }

                dwError = LW_ERROR_RPC_LSA_LOOKUPSIDS_FAILED;
            }

            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    if (dwFoundNamesCount == 0)
    {
        dwError = LW_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwFoundNamesCount > dwSidsCount)
    {
        dwError = LW_ERROR_RPC_LSA_LOOKUPSIDS_FOUND_DUPLICATES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!name_array || !pDomains)
    {
        dwError = LW_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                     sizeof(*ppszDomainNames)*pDomains->count,
                     (PVOID*)&ppszDomainNames);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < pDomains->count; i++)
    {
        if (pDomains->domains[i].name.Length > 0)
        {
            dwError = LwWc16snToMbs(
                          pDomains->domains[i].name.Buffer,
                          &ppszDomainNames[i],
                          pDomains->domains[i].name.Length / 2);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    // For incomplete results (LW_STATUS_SOME_NOT_MAPPED == status), leave ppTranslatedNames[i] as NULL for those NOT found
    // to maintain ppszObjectSids[i] -> ppTranslatedNames[i]
    dwError = LwAllocateMemory(
                    sizeof(*ppTranslatedNames)*dwSidsCount,
                    (PVOID*)&ppTranslatedNames);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwSidsCount; i++)
    {
        LSA_OBJECT_TYPE ObjectType = LSA_OBJECT_TYPE_UNDEFINED;
        PCSTR pszDomainName = NULL;

        ObjectType = GetObjectType(name_array[i].type);
        if (ObjectType == LSA_OBJECT_TYPE_UNDEFINED)
        {
            continue;
        }

        // Check for invalid domain indexing
        if (name_array[i].sid_index >= pDomains->count)
        {
            dwError = LW_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND;
            BAIL_ON_LSA_ERROR(dwError);
        }

        pszDomainName = ppszDomainNames[name_array[i].sid_index];

        if (name_array[i].name.Length > 0)
        {
            dwError = LwWc16snToMbs(
                           name_array[i].name.Buffer,
                           &pszUsername,
                           name_array[i].name.Length / 2);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (LW_IS_NULL_OR_EMPTY_STR(pszDomainName) ||
            ((ObjectType != LSA_OBJECT_TYPE_DOMAIN) && LW_IS_NULL_OR_EMPTY_STR(pszUsername)) ||
            ((ObjectType == LSA_OBJECT_TYPE_DOMAIN) && !LW_IS_NULL_OR_EMPTY_STR(pszUsername)))
        {
            dwError = LW_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LwAllocateMemory(
                        sizeof(*ppTranslatedNames[i]),
                        (PVOID*)&ppTranslatedNames[i]);
        BAIL_ON_LSA_ERROR(dwError);

        ppTranslatedNames[i]->ObjectType = ObjectType;

        if (ObjectType != LSA_OBJECT_TYPE_DOMAIN)
        {
            dwError = ADGetDomainQualifiedString(
                            pszDomainName,
                            pszUsername,
                            &ppTranslatedNames[i]->pszNT4NameOrSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LwAllocateString(
                            pszDomainName,
                            &ppTranslatedNames[i]->pszNT4NameOrSid);
            BAIL_ON_LSA_ERROR(dwError);

            LwStrToUpper(ppTranslatedNames[i]->pszNT4NameOrSid);
        }

        LW_SAFE_FREE_STRING(pszUsername);
    }

cleanup:
    if (pDomains)
    {
        LwFreeStringArray(ppszDomainNames,pDomains->count);
        LsaRpcFreeMemory(pDomains);
    }

    LW_SAFE_FREE_STRING(pszUsername);
    LW_SAFE_FREE_MEMORY(pwcHost);


    if (name_array)
    {
        LsaRpcFreeMemory(name_array);
    }

    if (sid_array.pSids)
    {
        for (i = 0; i < sid_array.dwNumSids; i++)
        {
            LW_SAFE_FREE_MEMORY(sid_array.pSids[i].pSid);
        }
        LW_SAFE_FREE_MEMORY(sid_array.pSids);
    }

    LW_SAFE_FREE_MEMORY(pObjectSID);

    status = LsaClose(lsa_binding, hPolicy);
    if (status != 0 && dwError == 0){
        LSA_LOG_DEBUG("LsaClose() failed with %u (0x%08x)", status, status);
        dwError = LW_ERROR_RPC_CLOSEPOLICY_FAILED;
    }

    if (lsa_binding)
    {
        LsaFreeBinding(&lsa_binding);
    }
    if (bChangedToken)
    {
        LwIoSetThreadCreds(pOldToken);
    }
    if (pOldToken != NULL)
    {
        LwIoDeleteCreds(pOldToken);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    *pppTranslatedNames = ppTranslatedNames;

    if (pdwFoundNamesCount)
    {
        *pdwFoundNamesCount = dwFoundNamesCount;
    }

    if (pbIsNetworkError)
    {
        *pbIsNetworkError = bIsNetworkError;
    }

    return dwError;

error:
    if (ppTranslatedNames)
    {
        LsaFreeTranslatedNameList(ppTranslatedNames, dwSidsCount);
        ppTranslatedNames = NULL;
    }
    dwFoundNamesCount = 0;

    goto cleanup;
}

DWORD
AD_DsEnumerateDomainTrusts(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszDomainControllerName,
    IN DWORD dwFlags,
    OUT NetrDomainTrust** ppTrusts,
    OUT PDWORD pdwCount,
    OUT OPTIONAL PBOOLEAN pbIsNetworkError
    )
{
    NTSTATUS status = 0;
    DWORD dwError = 0;
    PWSTR pwcDomainControllerName = NULL;
    NETR_BINDING netr_b = NULL;
    WINERROR winError = 0;
    NetrDomainTrust* pTrusts = NULL;
    DWORD dwCount = 0;
    BOOLEAN bIsNetworkError = FALSE;
    LW_PIO_CREDS pCreds = NULL;
    LW_PIO_CREDS pOldToken = NULL;
    BOOLEAN bChangedToken = FALSE;

    dwError = LwMbsToWc16s(pszDomainControllerName, &pwcDomainControllerName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_SetSystemAccess(
                  pState,
                  &pOldToken);
    BAIL_ON_LSA_ERROR(dwError);
    bChangedToken = TRUE;

    status = LwIoGetThreadCreds(&pCreds);
    dwError = LwNtStatusToWin32Error(status);
    BAIL_ON_LSA_ERROR(dwError);

    status = NetrInitBindingDefault(&netr_b,
                                    pwcDomainControllerName,
                                    pCreds);
    if (status != 0)
    {
        LSA_LOG_DEBUG("Failed to bind to %s (error %u)",
                      pszDomainControllerName, status);
        dwError = LW_ERROR_RPC_NETLOGON_FAILED;
        bIsNetworkError = TRUE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    winError = DsrEnumerateDomainTrusts(netr_b,
                                        pwcDomainControllerName,
                                        dwFlags,
                                        &pTrusts,
                                        &dwCount);
    if (winError)
    {
        LSA_LOG_DEBUG("Failed to enumerate trusts at %s (error %u)",
                      pszDomainControllerName, winError);

        if (AD_WinErrorIsTgtRevokedError(winError))
        {
            bIsNetworkError = TRUE;
            dwError = LW_ERROR_KRB5KDC_ERR_TGT_REVOKED;
        }
        else
        {
            dwError = winError;
     
            if (AD_WinErrorIsConnectionError(winError))
            {
                bIsNetworkError = TRUE;
            }
        }

        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (netr_b)
    {
        NetrFreeBinding(&netr_b);
        netr_b = NULL;
    }
    LW_SAFE_FREE_MEMORY(pwcDomainControllerName);
    if (bChangedToken)
    {
        LwIoSetThreadCreds(pOldToken);
    }
    if (pOldToken != NULL)
    {
        LwIoDeleteCreds(pOldToken);
    }
    if (pCreds != NULL)
    {
        LwIoDeleteCreds(pCreds);
    }
    *ppTrusts = pTrusts;
    *pdwCount = dwCount;
    if (pbIsNetworkError)
    {
        *pbIsNetworkError = bIsNetworkError;
    }
    return dwError;

error:
    dwCount = 0;
    if (pTrusts)
    {
        NetrFreeMemory(pTrusts);
        pTrusts = NULL;
    }
    goto cleanup;
}

VOID
AD_FreeDomainTrusts(
    IN OUT NetrDomainTrust** ppTrusts
    )
{
    if (ppTrusts && *ppTrusts)
    {
        NetrFreeMemory(*ppTrusts);
        *ppTrusts = NULL;
    }
}

DWORD
AD_DsGetDcName(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszServerName,
    IN PCSTR pszDomainName,
    IN BOOLEAN bReturnDnsName,
    OUT PSTR* ppszDomainDnsOrFlatName,
    OUT PSTR* ppszDomainForestDnsName,
    OUT OPTIONAL PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PWSTR pwcServerName = NULL;
    PWSTR pwcDomainName = NULL;
    NETR_BINDING netr_b = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    WINERROR winError = 0;
    BOOLEAN bIsNetworkError = FALSE;
    LW_PIO_CREDS pCreds = NULL;
    LW_PIO_CREDS pOldToken = NULL;
    BOOLEAN bChangedToken = FALSE;
    const UINT32 dwGetDcNameflags = bReturnDnsName ? DS_RETURN_DNS_NAME : DS_RETURN_FLAT_NAME;
    DsrDcNameInfo* pDcNameInfo = NULL;
    PSTR pszDomainDnsOrFlatName = NULL;
    PSTR pszDomainForestDnsName = NULL;

    dwError = LwMbsToWc16s(pszServerName, &pwcServerName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_SetSystemAccess(
                  pState,
                  &pOldToken);
    BAIL_ON_LSA_ERROR(dwError);
    bChangedToken = TRUE;

    status = LwIoGetThreadCreds(&pCreds);
    dwError = LwNtStatusToWin32Error(status);
    BAIL_ON_LSA_ERROR(dwError);

    status = NetrInitBindingDefault(&netr_b,
                                    pwcServerName,
                                    pCreds);
    if (status != 0)
    {
        LSA_LOG_DEBUG("Failed to bind to %s (error %u)",
                       pszServerName, status);
        dwError = LW_ERROR_RPC_NETLOGON_FAILED;
        bIsNetworkError = TRUE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwMbsToWc16s(pszDomainName, &pwcDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    winError = DsrGetDcName(netr_b,
                            pwcServerName,
                            pwcDomainName,
                            NULL,
                            NULL,
                            dwGetDcNameflags,
                            &pDcNameInfo);
    if (winError)
    {
        LSA_LOG_DEBUG("Failed to get dc name information for %s at %s (error %u)",
                      pszDomainName,
                      pszServerName,
                      winError);

        if (AD_WinErrorIsTgtRevokedError(winError))
        {
            bIsNetworkError = TRUE;
            dwError = LW_ERROR_KRB5KDC_ERR_TGT_REVOKED;
        }
        else
        {
            if (ERROR_NO_SUCH_DOMAIN == winError)
            {
                dwError = LW_ERROR_NO_SUCH_DOMAIN;
            }
            else
            {
                dwError = LW_ERROR_GET_DC_NAME_FAILED;
            }
            if (AD_WinErrorIsConnectionError(winError))
            {
                bIsNetworkError = TRUE;
            }
        }

        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwWc16sToMbs(pDcNameInfo->domain_name, &pszDomainDnsOrFlatName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pDcNameInfo->forest_name, &pszDomainForestDnsName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (netr_b)
    {
        NetrFreeBinding(&netr_b);
        netr_b = NULL;
    }
    LW_SAFE_FREE_MEMORY(pwcServerName);
    LW_SAFE_FREE_MEMORY(pwcDomainName);
    if (bChangedToken)
    {
        LwIoSetThreadCreds(pOldToken);
    }
    if (pOldToken != NULL)
    {
        LwIoDeleteCreds(pOldToken);
    }
    if (pCreds != NULL)
    {
        LwIoDeleteCreds(pCreds);
    }
    NetrFreeMemory((void*)pDcNameInfo);

    *ppszDomainDnsOrFlatName = pszDomainDnsOrFlatName;
    *ppszDomainForestDnsName = pszDomainForestDnsName;

    if (pbIsNetworkError)
    {
        *pbIsNetworkError = bIsNetworkError;
    }

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszDomainDnsOrFlatName);
    LW_SAFE_FREE_STRING(pszDomainForestDnsName);

    goto cleanup;
}

DWORD
AD_MapNetApiError(
    DWORD dwADError
    )
{
    DWORD dwError = 0;

    switch(dwADError)
    {
        case 1325:
            // There is no RPC error code for this at present.

            dwError = LW_ERROR_PASSWORD_RESTRICTION;
            break;
        default:
            dwError = dwADError;
            break;
    }

    return dwError;
}

void
LsaFreeTranslatedNameInfo(
    IN OUT PLSA_TRANSLATED_NAME_OR_SID pNameInfo
    )
{
    LW_SAFE_FREE_STRING(pNameInfo->pszNT4NameOrSid);
    LwFreeMemory(pNameInfo);
}

void
LsaFreeTranslatedNameList(
    IN OUT PLSA_TRANSLATED_NAME_OR_SID* pNameList,
    IN DWORD dwNumNames
    )
{
    DWORD iName = 0;

    for (iName = 0; iName < dwNumNames; iName++)
    {
        PLSA_TRANSLATED_NAME_OR_SID pNameInfo = pNameList[iName];
        if (pNameInfo)
        {
            LsaFreeTranslatedNameInfo(pNameInfo);
        }
    }
    LwFreeMemory(pNameList);
}

INT64
WinTimeToInt64(
    WinNtTime WinTime
    )
{
    INT64 Int64Time = 0;

    Int64Time = WinTime.high;
    Int64Time = Int64Time << 32;
    Int64Time |= WinTime.low;

    return Int64Time;
}

static DWORD
LsaCopyNetrUserInfo3(
    IN PLSA_SCHANNEL_STATE pSchannelState,
    OUT PLSA_AUTH_USER_INFO pUserInfo,
    IN NetrValidationInfo *pNetrUserInfo3
    )
{
    DWORD dwError = LW_ERROR_INTERNAL;
    NetrSamBaseInfo *pBase = NULL;
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;    
    RC4_KEY RC4Key;

    BAIL_ON_INVALID_POINTER(pUserInfo);
    BAIL_ON_INVALID_POINTER(pNetrUserInfo3);

    pBase = &pNetrUserInfo3->sam3->base;

    pUserInfo->dwUserFlags = pBase->user_flags;

    pUserInfo->LogonTime          = WinTimeToInt64(pBase->last_logon);
    pUserInfo->LogoffTime         = WinTimeToInt64(pBase->last_logoff);
    pUserInfo->KickoffTime        = WinTimeToInt64(pBase->acct_expiry);
    pUserInfo->LastPasswordChange = WinTimeToInt64(pBase->last_password_change);
    pUserInfo->CanChangePassword  = WinTimeToInt64(pBase->allow_password_change);
    pUserInfo->MustChangePassword = WinTimeToInt64(pBase->force_password_change);

    pUserInfo->LogonCount       = pBase->logon_count;
    pUserInfo->BadPasswordCount = pBase->bad_password_count;

    pUserInfo->dwAcctFlags = pBase->acct_flags;

    dwError = LsaDataBlobStore(&pUserInfo->pSessionKey,
                               sizeof(pBase->key.key),
                               pBase->key.key);
    BAIL_ON_LSA_ERROR(dwError);

    /* We have to decrypt the user session key before we can use it */

    RC4_set_key(&RC4Key, 16, pSchannelState->pSchannelCreds->session_key);
    RC4(&RC4Key, 
        pUserInfo->pSessionKey->dwLen,
        pUserInfo->pSessionKey->pData, 
        pUserInfo->pSessionKey->pData);

    dwError = LsaDataBlobStore(&pUserInfo->pLmSessionKey,
                               sizeof(pBase->lmkey.key),
                               pBase->lmkey.key);
    BAIL_ON_LSA_ERROR(dwError);

    RC4_set_key(&RC4Key, 16, pSchannelState->pSchannelCreds->session_key);
    RC4(&RC4Key, 
        pUserInfo->pLmSessionKey->dwLen,
        pUserInfo->pLmSessionKey->pData, 
        pUserInfo->pLmSessionKey->pData);

    pUserInfo->pszUserPrincipalName = NULL;
    pUserInfo->pszDnsDomain = NULL;

    if (pBase->account_name.Length)
    {
        dwError = LwWc16sToMbs(pBase->account_name.Buffer, &pUserInfo->pszAccount);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->full_name.Length)
    {
        dwError = LwWc16sToMbs(pBase->full_name.Buffer, &pUserInfo->pszFullName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->domain.Length)
    {
        dwError = LwWc16sToMbs(pBase->domain.Buffer, &pUserInfo->pszDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->logon_server.Length)
    {
        dwError = LwWc16sToMbs(pBase->logon_server.Buffer, &pUserInfo->pszLogonServer);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->logon_script.Length)
    {
        dwError = LwWc16sToMbs(pBase->logon_script.Buffer, &pUserInfo->pszLogonScript);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->home_directory.Length)
    {
        dwError = LwWc16sToMbs(pBase->home_directory.Buffer, &pUserInfo->pszHomeDirectory);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->home_drive.Length)
    {
        dwError = LwWc16sToMbs(pBase->home_drive.Buffer, &pUserInfo->pszHomeDrive);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->profile_path.Length)
    {
        dwError = LwWc16sToMbs(pBase->profile_path.Buffer, &pUserInfo->pszProfilePath);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ntError = RtlAllocateCStringFromSid(&pUserInfo->pszDomainSid, pBase->domain_sid);
    dwError = LwNtStatusToWin32Error(ntError);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->dwUserRid         = pBase->rid;
    pUserInfo->dwPrimaryGroupRid = pBase->primary_gid;

    pUserInfo->dwNumRids = pBase->groups.dwCount;
    if (pUserInfo->dwNumRids != 0)
    {
        int i = 0;

        dwError = LwAllocateMemory(sizeof(LSA_RID_ATTRIB)*(pUserInfo->dwNumRids),
                                    (PVOID*)&pUserInfo->pRidAttribList);
        BAIL_ON_LSA_ERROR(dwError);

        for (i=0; i<pUserInfo->dwNumRids; i++)
        {
            pUserInfo->pRidAttribList[i].Rid      = pBase->groups.pRids[i].dwRid;
            pUserInfo->pRidAttribList[i].dwAttrib = pBase->groups.pRids[i].dwAttributes;
        }

    }

    if (pNetrUserInfo3->sam3->sidcount != 0)
    {
        int i = 0;

        pUserInfo->dwNumSids = 0;        

        dwError = LwAllocateMemory(
                      sizeof(LSA_SID_ATTRIB)*(pNetrUserInfo3->sam3->sidcount),
                      (PVOID*)&pUserInfo->pSidAttribList);
        BAIL_ON_LSA_ERROR(dwError);

        for (i=0; i<pNetrUserInfo3->sam3->sidcount; i++)
        {
            PLSA_SID_ATTRIB pSidAttrib = &(pUserInfo->pSidAttribList[pUserInfo->dwNumSids]);

            dwError = ERROR_SUCCESS;

            pSidAttrib->dwAttrib = pNetrUserInfo3->sam3->sids[i].attribute;

            ntError = RtlAllocateCStringFromSid(
                          &pSidAttrib->pszSid,
                          pNetrUserInfo3->sam3->sids[i].sid);
            dwError = LwNtStatusToWin32Error(ntError);
            if (dwError != ERROR_SUCCESS)
            {
                if (pNetrUserInfo3->sam3->sids[i].sid)
                {
                    LSA_LOG_DEBUG(
                        "Ignoring invalid SID (User = %s\\%s, Attribute = 0x%x, Revision = %d, SubAuthorityCount = %d).\n",
                        pUserInfo->pszDomain ? pUserInfo->pszDomain : "NULL",
                        pUserInfo->pszAccount ? pUserInfo->pszAccount : "NULL",
                        pNetrUserInfo3->sam3->sids[i].attribute,
                        pNetrUserInfo3->sam3->sids[i].sid->Revision,
                        pNetrUserInfo3->sam3->sids[i].sid->SubAuthorityCount);
                }
                else
                {
                    LSA_LOG_DEBUG(
                        "Ignoring NULL SID (User = %s\\%s, Attribute = 0x%x).\n",
                        pUserInfo->pszDomain ? pUserInfo->pszDomain : "NULL",
                        pUserInfo->pszAccount ? pUserInfo->pszAccount : "NULL",
                        pNetrUserInfo3->sam3->sids[i].attribute);
                }
                continue;
            }

            // Successfully added another SID
            pUserInfo->dwNumSids++;
        }
    }

    /* All done */

    dwError = LW_ERROR_SUCCESS;

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
AD_NetlogonAuthenticationUserEx(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PSTR pszDomainController,
    IN PLSA_AUTH_USER_PARAMS pUserParams,
    OUT PLSA_AUTH_USER_INFO *ppUserInfo,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = LW_ERROR_INTERNAL;
    PLSA_SCHANNEL_STATE pSchannelState = pState->hSchannelState;
    PWSTR pwszDomainController = NULL;
    PWSTR pwszServerName = NULL;
    PWSTR pwszShortDomain = NULL;
    PWSTR pwszPrimaryShortDomain = NULL;
    PWSTR pwszPrimaryFqdn = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszComputer = NULL;
    NTSTATUS status = 0;
    NETR_BINDING netr_b = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W pMachinePasswordInfo = NULL;
    BOOLEAN bIsNetworkError = FALSE;
    NTSTATUS nt_status = STATUS_UNHANDLED_EXCEPTION;
    NetrValidationInfo  *pValidationInfo = NULL;
    UINT8 dwAuthoritative = 0;
    PSTR pszServerName = NULL;
    DWORD dwDCNameLen = 0;
    PBYTE pChal = NULL;
    PBYTE pLMResp = NULL;
    DWORD LMRespLen = 0;
    PBYTE pNTResp = NULL;
    DWORD NTRespLen = 0;
    LW_PIO_CREDS pCreds = NULL;
    LW_PIO_CREDS pOldToken = NULL;
    BOOLEAN bChangedToken = FALSE;
    BOOLEAN bResetSchannel = FALSE;
    PLSA_AUTH_USER_INFO pUserInfo = NULL;
    BOOLEAN passwordLocked = FALSE;

    pthread_mutex_lock(pSchannelState->pSchannelLock);

    /* Grab the machine password and account info */

    AD_LOCK_MACHINE_PASSWORD(
                    pState->hMachinePwdState,
                    passwordLocked);

    dwError = LsaPcacheGetMachinePasswordInfoW(
                  pState->pPcache,
                  &pMachinePasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    /* Gather other Schannel params */

    /* Allocate space for the servername.  Include room for the terminating
       NULL and \\ */

    dwDCNameLen = strlen(pszDomainController) + 3;
    dwError = LwAllocateMemory(dwDCNameLen, (PVOID*)&pszServerName);
    BAIL_ON_LSA_ERROR(dwError);

    snprintf(pszServerName, dwDCNameLen, "\\\\%s", pszDomainController);
    dwError = LwMbsToWc16s(pszServerName, &pwszServerName);
    BAIL_ON_LSA_ERROR(dwError);

    if (pUserParams->pszDomain)
    {
        dwError = LwMbsToWc16s(pUserParams->pszDomain, &pwszShortDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pwszComputer = wc16sdup(pMachinePasswordInfo->Account.SamAccountName);
    if (!pwszComputer)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
    }
    BAIL_ON_LSA_ERROR(dwError);

    // Remove $ from account name
    pwszComputer[wc16slen(pwszComputer) - 1] = 0;

    if (pSchannelState->pszSchannelServer && strcasecmp(pSchannelState->pszSchannelServer, pszDomainController))
    {
        LSA_LOG_VERBOSE("Resetting schannel due to switching DC from '%s' to '%s'",
                        pSchannelState->pszSchannelServer, pszDomainController);
        AD_ClearSchannelStateInLock(pSchannelState);
    }

    if (!pSchannelState->hSchannelBinding)
    {
        dwError = LwMbsToWc16s(pszDomainController, &pwszDomainController);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwMbsToWc16s(pState->pProviderData->szShortDomain,
                                &pwszPrimaryShortDomain);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwMbsToWc16s(pState->pProviderData->szDomain,
                               &pwszPrimaryFqdn);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwWc16sToLower(pwszPrimaryFqdn);
        BAIL_ON_LSA_ERROR(dwError);

        /* Establish the initial bind to \NETLOGON */

        dwError = AD_SetSystemAccess(
                      pState,
                      &pOldToken);
        BAIL_ON_LSA_ERROR(dwError);
        bChangedToken = TRUE;

        status = LwIoGetThreadCreds(&pCreds);
        dwError = LwNtStatusToWin32Error(status);
        BAIL_ON_LSA_ERROR(dwError);

        status = NetrInitBindingDefault(&netr_b, pwszDomainController, pCreds);
        if (status != 0)
        {
            LSA_LOG_DEBUG("Failed to bind to %s (error %u)",
                          pszDomainController, status);
            dwError = LW_ERROR_RPC_NETLOGON_FAILED;
            bIsNetworkError = TRUE;
            BAIL_ON_LSA_ERROR(dwError);
        }

        /* Now setup the Schannel session */

        nt_status = NetrOpenSchannel(netr_b,
                                     pMachinePasswordInfo->Account.SamAccountName,
                                     pwszDomainController,
                                     pwszServerName,
                                     pwszPrimaryShortDomain,
                                     pwszPrimaryFqdn,
                                     pwszComputer,
                                     pMachinePasswordInfo->Password,
                                     &pSchannelState->SchannelCreds,
                                     &pSchannelState->hSchannelBinding);

        if (nt_status != STATUS_SUCCESS)
        {
            LSA_LOG_DEBUG("NetrOpenSchannel() failed with %u (0x%08x)", nt_status, nt_status);

            if (AD_NtStatusIsTgtRevokedError(nt_status))
            {
                bIsNetworkError = TRUE;
                dwError = LW_ERROR_KRB5KDC_ERR_TGT_REVOKED;
            }
            else if (nt_status == STATUS_NO_TRUST_SAM_ACCOUNT)
            {
                bIsNetworkError = TRUE;
                dwError = ERROR_NO_TRUST_SAM_ACCOUNT;
            }
            else
            {
                bResetSchannel = TRUE;
                dwError = LW_ERROR_RPC_ERROR;
                if (AD_NtStatusIsConnectionError(nt_status))
                {
                    bIsNetworkError = TRUE;
                }
            }
        }
        BAIL_ON_LSA_ERROR(dwError);

        if (!pSchannelState->pszSchannelServer)
        {
            dwError = LwAllocateString(
                          pszDomainController,
                          &pSchannelState->pszSchannelServer);
            BAIL_ON_LSA_ERROR(dwError);
        }

        pSchannelState->pSchannelCreds = &pSchannelState->SchannelCreds;
    }
    AD_UNLOCK_MACHINE_PASSWORD(
                    pState->hMachinePwdState,
                    passwordLocked);


    /* Time to do the authentication */

    dwError = LwMbsToWc16s(pUserParams->pszAccountName, &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    /* Get the data blob buffers */

    if (pUserParams->pass.chap.pChallenge)
        pChal = LsaDataBlobBuffer(pUserParams->pass.chap.pChallenge);

    if (pUserParams->pass.chap.pLM_resp) {
        pLMResp = LsaDataBlobBuffer(pUserParams->pass.chap.pLM_resp);
        LMRespLen = LsaDataBlobLength(pUserParams->pass.chap.pLM_resp);
    }

    if (pUserParams->pass.chap.pNT_resp) {
        pNTResp = LsaDataBlobBuffer(pUserParams->pass.chap.pNT_resp);
        NTRespLen = LsaDataBlobLength(pUserParams->pass.chap.pNT_resp);
    }

    nt_status = NetrSamLogonNetworkEx(pSchannelState->hSchannelBinding,
                                      pwszServerName,
                                      pwszShortDomain,
                                      pwszComputer,
                                      pwszUsername,
                                      pChal,
                                      pLMResp,
                                      LMRespLen,
                                      pNTResp,
                                      NTRespLen,
                                      2,  /* Network login */
                                      3,  /* Return NetSamInfo3 */
                                      &pValidationInfo,
                                      &dwAuthoritative);

    if (nt_status)
    {
        LSA_LOG_DEBUG("NetrSamLogonNetworkEx() failed with %u (0x%08x) (symbol: '%s')"
                      " while authenticating user '%s\\%s'",
                      nt_status, nt_status, LSA_SAFE_LOG_STRING(LwNtStatusToName(nt_status)),
                      pUserParams->pszDomain, pUserParams->pszAccountName);

        if (AD_NtStatusIsTgtRevokedError(nt_status))
        {
            bIsNetworkError = TRUE;
            dwError = LW_ERROR_KRB5KDC_ERR_TGT_REVOKED;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            if (AD_NtStatusIsConnectionError(nt_status))
            {
                bIsNetworkError = TRUE;
            }
        }
    }

    switch (nt_status)
    {
    case STATUS_SUCCESS:
        dwError = 0;
        break;
    case STATUS_NO_SUCH_USER:
        dwError = LW_ERROR_NO_SUCH_USER;
        break;
    case STATUS_ACCOUNT_LOCKED_OUT:
        dwError = LW_ERROR_ACCOUNT_LOCKED;
        break;
    case STATUS_ACCOUNT_DISABLED:
        dwError = LW_ERROR_ACCOUNT_DISABLED;
        break;
    case STATUS_ACCOUNT_EXPIRED:
        dwError = LW_ERROR_ACCOUNT_EXPIRED;
        break;
    case STATUS_PASSWORD_EXPIRED:
        dwError = LW_ERROR_PASSWORD_EXPIRED;
        break;
    case STATUS_PASSWORD_MUST_CHANGE:
        dwError = ERROR_PASSWORD_MUST_CHANGE;
        break;
    case STATUS_WRONG_PASSWORD:
        dwError = LW_ERROR_INVALID_PASSWORD;
        break;
    case STATUS_INVALID_ACCOUNT_NAME:
        dwError = ERROR_INVALID_ACCOUNT_NAME;
        break;
    case STATUS_ACCOUNT_RESTRICTION:
    case STATUS_LOGON_FAILURE:
    case STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT:
    case STATUS_NOLOGON_SERVER_TRUST_ACCOUNT:
        dwError = LW_ERROR_LOGON_FAILURE;
        break;
    case STATUS_UNHANDLED_EXCEPTION:
    default:
        bResetSchannel = TRUE;
        dwError = LW_ERROR_RPC_NETLOGON_FAILED;
        LSA_LOG_ERROR("Resetting schannel due to status 0x%08x while "
                      "authenticating user '%s\\%s'",
                      nt_status,
                      pUserParams->pszDomain, pUserParams->pszAccountName);
        break;
    }
  
    BAIL_ON_LSA_ERROR(dwError);

    /* Translate the returned NetrValidationInfo to the
       LSA_AUTH_USER_INFO out param */

    dwError = LwAllocateMemory(sizeof(LSA_AUTH_USER_INFO), (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCopyNetrUserInfo3(pSchannelState, pUserInfo, pValidationInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    AD_UNLOCK_MACHINE_PASSWORD(
                    pState->hMachinePwdState,
                    passwordLocked);
    LsaPcacheReleaseMachinePasswordInfoW(pMachinePasswordInfo);

    if (netr_b)
    {
        NetrFreeBinding(&netr_b);
        netr_b = NULL;
    }

    if (bChangedToken)
    {
        LwIoSetThreadCreds(pOldToken);
    }
    if (pOldToken != NULL)
    {
        LwIoDeleteCreds(pOldToken);
    }
    if (pCreds != NULL)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (pValidationInfo) {
        NetrFreeMemory((void*)pValidationInfo);
    }    

    LW_SAFE_FREE_MEMORY(pszServerName);

    LW_SAFE_FREE_MEMORY(pwszUsername);
    LW_SAFE_FREE_MEMORY(pwszDomainController);
    LW_SAFE_FREE_MEMORY(pwszServerName);
    LW_SAFE_FREE_MEMORY(pwszShortDomain);
    LW_SAFE_FREE_MEMORY(pwszPrimaryShortDomain);
    LW_SAFE_FREE_MEMORY(pwszPrimaryFqdn);
    LW_SAFE_FREE_MEMORY(pwszComputer);

    pthread_mutex_unlock(pSchannelState->pSchannelLock);

    *ppUserInfo = pUserInfo;

    if (pbIsNetworkError)
    {
        *pbIsNetworkError = bIsNetworkError;
    }

    return dwError;

error:

    LsaFreeAuthUserInfo(&pUserInfo);

    if (bResetSchannel)
    {
        AD_ClearSchannelStateInLock(pSchannelState);
    }

    goto cleanup;
}

static
VOID
AD_ClearSchannelStateInLock(
    IN PLSA_SCHANNEL_STATE pSchannelState
    )
{
    if (pSchannelState->hSchannelBinding)
    {
        NetrCloseSchannel(pSchannelState->hSchannelBinding);

        pSchannelState->hSchannelBinding = NULL;

        memset(&pSchannelState->SchannelCreds,
               0,
               sizeof(pSchannelState->SchannelCreds));
        pSchannelState->pSchannelCreds = NULL;

        LW_SAFE_FREE_MEMORY(pSchannelState->pszSchannelServer);
    }
}

static
BOOLEAN
AD_NtStatusIsTgtRevokedError(
    NTSTATUS status
    )
{
    switch (status)
    {
    case STATUS_KDC_CERT_REVOKED:
        return TRUE;
    default:
        return FALSE;
    }
}

static
BOOLEAN
AD_NtStatusIsConnectionError(
    NTSTATUS status
    )
{
    /* ACCESS_DENIED is listed as a connection error
     * because it may be specific to a given domain controller.
     * That is, reconnecting to a different DC with the same
     * credentials may work. */

    switch (status)
    {
    case STATUS_CONNECTION_ABORTED:
    case STATUS_CONNECTION_DISCONNECTED:
    case STATUS_CONNECTION_REFUSED:
    case STATUS_CONNECTION_RESET:
    case STATUS_IO_TIMEOUT:
    case STATUS_ACCESS_DENIED:
    case STATUS_TIME_DIFFERENCE_AT_DC:
    case STATUS_INVALID_CONNECTION:
    case STATUS_PIPE_DISCONNECTED:
    case STATUS_BAD_NETWORK_NAME:
        return TRUE;
    default:
        return FALSE;
    }
}

static
BOOLEAN
AD_WinErrorIsTgtRevokedError(
    WINERROR winError
    )
{
    switch (winError)
    {
        case SEC_E_KDC_CERT_REVOKED:
            return TRUE;

        default:
            return FALSE;
    }
}

static
BOOLEAN
AD_WinErrorIsConnectionError(
    WINERROR winError
    )
{
    switch (winError)
    {
        case ERROR_CONNECTION_ABORTED:
        case ERROR_CONNECTION_REFUSED:
        case ERROR_NETNAME_DELETED:
        case ERROR_SEM_TIMEOUT:
        case ERROR_ACCESS_DENIED:
        case ERROR_TIME_SKEW:
        case ERROR_UNEXP_NET_ERR:
        case ERROR_PIPE_NOT_CONNECTED:
        case ERROR_BAD_NET_NAME:
        case WSAECONNRESET:
            return TRUE;

        default:
            return FALSE;
    }
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
