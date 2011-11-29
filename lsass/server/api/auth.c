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
 *        auth.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Authentication API (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

DWORD
LsaSrvAuthenticateUserPam(
    HANDLE hServer,
    PLSA_AUTH_USER_PAM_PARAMS pParams,
    PLSA_AUTH_USER_PAM_INFO* ppPamAuthInfo
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_AUTHENTICATION};
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    PLSA_AUTH_USER_PAM_INFO pPamAuthInfo = NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    BAIL_ON_INVALID_POINTER(pParams);
    BAIL_ON_INVALID_STRING(pParams->pszLoginName);

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      NULL,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        if (pPamAuthInfo)
        {
            LsaFreeAuthUserPamInfo(pPamAuthInfo);
            pPamAuthInfo = NULL;
        }
        dwError = pProvider->pFnTable->pfnAuthenticateUserPam(
                                            hProvider,
                                            pParams,
                                            &pPamAuthInfo);
        if (!dwError)
        {
            if (LsaSrvEventlogEnabled())
            {
                LsaSrvWriteLoginSuccessEvent(hServer,
                                             pProvider->pszName,
                                             pParams->pszLoginName,
                                             pParams->pszPamSource,
                                             pParams->dwFlags,
                                             LSASS_EVENT_LOGON_PHASE_AUTHENTICATE,
                                             dwError);
            }
            break;
        }
        else if ((dwError == LW_ERROR_NOT_HANDLED) ||
                 (dwError == LW_ERROR_NO_SUCH_USER))
        {
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;
            continue;
        }
        else
        {
            if (LsaSrvEventlogEnabled())
            {
                LsaSrvWriteLoginFailedEvent(hServer,
                                            pProvider->pszName,
                                            pParams->pszLoginName,
                                            pParams->pszPamSource,
                                            pParams->dwFlags,
                                            LSASS_EVENT_LOGON_PHASE_AUTHENTICATE,
                                            dwError);
            }
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pProvider == NULL)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (ppPamAuthInfo)
    {
        *ppPamAuthInfo = pPamAuthInfo;
    }
    else if (pPamAuthInfo)
    {
        LsaFreeAuthUserPamInfo(pPamAuthInfo);
    }
    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    if (!dwError)
    {
        LsaSrvIncrementMetricValue(LsaMetricSuccessfulAuthentications);
    }
    else
    {
        LsaSrvIncrementMetricValue(LsaMetricFailedAuthentications);
    }

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

error:

    if (dwError == LW_ERROR_NOT_HANDLED ||
        dwError == LW_ERROR_NO_SUCH_USER)
    {
        LSA_LOG_VERBOSE_ENTRY_NOT_FOUND(hServer, dwError, "authenticate user (name = '%s')", LSA_SAFE_LOG_STRING(pParams->pszLoginName));
    }
    else
    {
        /* if (dwError == LW_ERROR_PASSWORD_MISMATCH && LsaSrvLogPasswords())
        {
            LSA_LOG_ERROR_API_FAILED(hServer, dwError, "authenticate user (name = '%s', password = '%s')", LSA_SAFE_LOG_STRING(pParams->pszLoginName), LSA_SAFE_LOG_STRING(pParams->pszPassword));
        }
        else */
        {
            LSA_LOG_ERROR_API_FAILED(hServer, dwError, "authenticate user (name = '%s')", LSA_SAFE_LOG_STRING(pParams->pszLoginName));
        }
    }

    goto cleanup;
}

DWORD
LsaSrvAuthenticateUserEx(
    HANDLE hServer,
    PCSTR pszTargetProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUserInfo
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_AUTHENTICATION};
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    LSA_AUTH_USER_PARAMS localUserParams;
    BOOLEAN bFoundProvider = FALSE;
    PSTR pszTargetProviderName = NULL;
    PSTR pszTargetInstance = NULL;
    PSTR pCombinedAccountName = NULL;
    LSA_AUTH_USER_PAM_PARAMS pamParams = { 0 };
    PLSA_AUTH_USER_PAM_INFO pPamAuthInfo = NULL;

    BAIL_ON_INVALID_POINTER(pUserParams);
    BAIL_ON_INVALID_POINTER(ppUserInfo);

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    /* Make copy of parameters structure so we can modify it */
    localUserParams = *pUserParams;

    /* Validate the AuthType */

    switch (pUserParams->AuthType)
    {
    case LSA_AUTH_PLAINTEXT:
        if (pUserParams->pszDomain)
        {
            dwError = LwAllocateStringPrintf(
                            &pCombinedAccountName,
                            "%s\\%s", 
                            pUserParams->pszDomain,
                            pUserParams->pszAccountName);
            pamParams.pszLoginName = pCombinedAccountName;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            pamParams.pszLoginName = pUserParams->pszAccountName;
        }
        pamParams.pszPassword = pUserParams->pass.clear.pszPassword;
        break;

    case LSA_AUTH_CHAP:
        /* NTLM is what we'll do for the rest of the routine */
        break;

    default:
        /* Bad AuthType */
        dwError = LW_ERROR_INVALID_PARAMETER;
        goto cleanup;
        break;
    }

    /* Fix up the name.  This allows us to handle a UPN */
    dwError = LsaSrvCrackDomainQualifiedName(pUserParams->pszAccountName,
                                          &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pLoginInfo->nameType == NameType_UPN)
    {
        /* If they gave us a UPN, our domain MUST be NULL */
        localUserParams.pszDomain = NULL;
    }
    
    /* Do the NTLM authentication */

    if (pszTargetProvider)
    {
        dwError = LsaSrvGetTargetElements(
                      pszTargetProvider,
                      &pszTargetProviderName,
                      &pszTargetInstance);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        if (pszTargetProviderName)
        {
            if (!strcmp(pszTargetProviderName, pProvider->pszName))
            {
                bFoundProvider = TRUE;
            }
            else
            {
                continue;
            }
        }

        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      pszTargetInstance,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        switch (pUserParams->AuthType)
        {
        case LSA_AUTH_PLAINTEXT:
            dwError = pProvider->pFnTable->pfnAuthenticateUserPam(
                                                hProvider,
                                                &pamParams,
                                                &pPamAuthInfo);
            if (pPamAuthInfo)
            {
                LsaFreeAuthUserPamInfo(pPamAuthInfo);
            }
            break;

        case LSA_AUTH_CHAP:
            dwError = pProvider->pFnTable->pfnAuthenticateUserEx(
                                                hProvider,
                                                &localUserParams,
                                                ppUserInfo);
            break;
        }
        if (!dwError) 
        {
            if (LsaSrvEventlogEnabled())
            {
                LsaSrvWriteLoginSuccessEvent(hServer,
                                             pProvider->pszName,
                                             pUserParams && pUserParams->pszAccountName ?
                                             pUserParams->pszAccountName : "",
                                             NULL,
                                             0,
                                             LSASS_EVENT_LOGON_PHASE_AUTHENTICATE,
                                             dwError);
            }
            break;
        }
        else if ((dwError == LW_ERROR_NOT_HANDLED) ||
                 (dwError == LW_ERROR_NO_SUCH_USER)) 
        {
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;
            continue;
        }
        else
        {
            if (LsaSrvEventlogEnabled())
            {
                LsaSrvWriteLoginFailedEvent(hServer,
                                            pProvider->pszName,
                                            pUserParams && pUserParams->pszAccountName ?
                                            pUserParams->pszAccountName : "",
                                            NULL,
                                            0,
                                            LSASS_EVENT_LOGON_PHASE_AUTHENTICATE,
                                            dwError);
            }
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pszTargetProviderName && !bFoundProvider)
    {
        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pProvider == NULL) {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pCombinedAccountName);
    LW_SAFE_FREE_STRING(pszTargetProviderName);
    LW_SAFE_FREE_STRING(pszTargetInstance);

    if (hProvider != (HANDLE)NULL) 
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    if (pLoginInfo)
    {
        LsaSrvFreeNameInfo(pLoginInfo);
    }

    if (!dwError) 
    {
        LsaSrvIncrementMetricValue(LsaMetricSuccessfulAuthentications);
    }
    else
    {
        LsaSrvIncrementMetricValue(LsaMetricFailedAuthentications);
    }

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

error:

    if (dwError == LW_ERROR_NOT_HANDLED ||
        dwError == LW_ERROR_NO_SUCH_USER)
    {
        LSA_LOG_VERBOSE_ENTRY_NOT_FOUND(hServer, dwError, "authenticate user (name = '%s')",
                  pUserParams && pUserParams->pszAccountName ? pUserParams->pszAccountName : "");
    }
    else
    {
        LSA_LOG_ERROR_API_FAILED(hServer, dwError, "authenticate user (name = '%s')",
                  pUserParams && pUserParams->pszAccountName ? pUserParams->pszAccountName : "");
    }

    goto cleanup;	
}

DWORD
LsaSrvValidateUser(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_AUTHENTICATION};
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    BAIL_ON_INVALID_STRING(pszLoginId);

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      NULL,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnValidateUser(
                                            hProvider,
                                            pszLoginId,
                                            pszPassword);
        if (!dwError) {
           break;
        }
        else if ((dwError == LW_ERROR_NOT_HANDLED) ||
                 (dwError == LW_ERROR_NO_SUCH_USER)) {
           LsaSrvCloseProvider(pProvider, hProvider);
           hProvider = (HANDLE)NULL;
           continue;
        } else {
           BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

error:
    if (dwError == LW_ERROR_NOT_HANDLED ||
        dwError == LW_ERROR_NO_SUCH_USER)
    {
        LSA_LOG_VERBOSE_ENTRY_NOT_FOUND(hServer, dwError, "validate user for login (name = '%s')", LSA_SAFE_LOG_STRING(pszLoginId));
    }
    else
    {
        LSA_LOG_ERROR_API_FAILED(hServer, dwError, "validate user for login (name = '%s')", LSA_SAFE_LOG_STRING(pszLoginId));
    }

    goto cleanup;
}

DWORD
LsaSrvCheckUserInList(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszListName
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_AUTHENTICATION};
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      NULL,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnCheckUserInList(
                                            hProvider,
                                            pszLoginId,
                                            pszListName);
        if (!dwError)
        {
            if (LsaSrvEventlogEnabled())
            {
                LsaSrvWriteLoginSuccessEvent(
                    hServer,
                    pProvider->pszName,
                    pszLoginId,
                    NULL,
                    0,
                    LSASS_EVENT_LOGON_PHASE_CHECK_USER,
                    dwError);
            }
            break;
        }
        else if ((dwError == LW_ERROR_NOT_HANDLED) ||
                 (dwError == LW_ERROR_NO_SUCH_USER))
        {
           LsaSrvCloseProvider(pProvider, hProvider);
           hProvider = (HANDLE)NULL;
           continue;
        }
        else
        {
            if (LsaSrvEventlogEnabled())
            {
                LsaSrvWriteLoginFailedEvent(
                    hServer,
                    pProvider->pszName,
                    pszLoginId,
                    NULL,
                    0,
                    LSASS_EVENT_LOGON_PHASE_CHECK_USER,
                    dwError);
            }
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:
    if (dwError == LW_ERROR_ACCESS_DENIED)
    {
        LSA_LOG_VERBOSE_ENTRY_NOT_FOUND(hServer, dwError, "find user in list (user = '%s', list = '%s')", LSA_SAFE_LOG_STRING(pszLoginId), LSA_SAFE_LOG_STRING(pszListName));
    }
    else
    {
        LSA_LOG_ERROR_API_FAILED(hServer, dwError, "find user in list (user = '%s', list = '%s')", LSA_SAFE_LOG_STRING(pszLoginId), LSA_SAFE_LOG_STRING(pszListName));
    }

    goto cleanup;
}

DWORD
LsaSrvChangePassword(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword
    )
{
    DWORD  dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_AUTHENTICATION};
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    BOOLEAN bInLock = FALSE;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      NULL,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnChangePassword(
                                        hProvider,
                                        pszLoginId,
                                        pszPassword,
                                        pszOldPassword);
        if (!dwError)
        {
            if (LsaSrvEventlogEnabled())
            {
                LsaSrvWriteUserPWChangeSuccessEvent(
                    hServer,
                    pProvider->pszName,
                    pszLoginId);
            }
            break;
        }
        else if ((dwError == LW_ERROR_NOT_HANDLED) ||
                 (dwError == LW_ERROR_NO_SUCH_USER))
        {
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;
            continue;
        }
        else
        {
            if (LsaSrvEventlogEnabled())
            {
                LsaSrvWriteUserPWChangeFailureEvent(
                    hServer,
                    pProvider->pszName,
                    pszLoginId,
                    dwError);
            }
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    if (hProvider != (HANDLE)NULL) {
       LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    if (!dwError)
    {
        LsaSrvIncrementMetricValue(LsaMetricSuccessfulChangePassword);
    }
    else
    {
        LsaSrvIncrementMetricValue(LsaMetricFailedChangePassword);
    }

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "change password of user (name = '%s')", LSA_SAFE_LOG_STRING(pszLoginId));

    goto cleanup;
}

DWORD
LsaSrvSetPassword(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_AUTHENTICATION};
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    BOOLEAN bInLock = FALSE;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      NULL,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnSetPassword(
                                        hProvider,
                                        pszLoginId,
                                        pszPassword);
        if (!dwError)
        {
            break;
        }
        else if ((dwError == LW_ERROR_NOT_HANDLED) ||
                 (dwError == LW_ERROR_NO_SUCH_USER))
        {
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;
            continue;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    if (hProvider != (HANDLE)NULL)
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "set password of user (name = '%s')", LSA_SAFE_LOG_STRING(pszLoginId));

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
