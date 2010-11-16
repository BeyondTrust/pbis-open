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
 *        event.h
 *
 * Abstract:
 *
 *        Likewise Security And Authentication Subsystem (LSASS)
 *
 *        Eventlog API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

DWORD
LsaSrvGetPamSourceOffset(
    PCSTR pszPamSource
    )
{
    struct
    {
        DWORD dwOffset;
        const char *pszPamSource;
    } services[] = {
        { 0, "ssh" },
        { 0, "sshd" },
        { 0, "sshd-kbdint" }, // Used by Solaris 10
        { 1, "gdm" },
        { 1, "kdm" },
        { 1, "kde" }, // Used by Ubuntu 10.04
        { 1, "dtlogin" }, // Used by Solaris 10
        { 2, "login" },
        { 3, "ftp" }, // Used by Solaris 10
        { 4, "telnet" }, // Used by Solaris 10
        { 4, "rsh" },
        { 4, "krsh" },
        { 4, "ktelnet" },
        { 4, "rlogin" },
        { 5, "gnome-screensaver" },
        { 5, "dtsession" }, // Used by Solaris 10
        { 6, "sudo" },
        { 7, "passwd" },
        { 8, "su" },
    };
    DWORD i = 0;

    if (pszPamSource != NULL)
    {
        for (i = 0; i < sizeof(services)/sizeof(services[0]); i++)
        {
            if (!strcmp(pszPamSource, services[i].pszPamSource))
            {
                return services[i].dwOffset;
            }
        }
    }

    return 19;
}

VOID
LsaSrvWriteLoginSuccessEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPamSource,
    DWORD  dwFlags,
    DWORD  dwLoginPhase,
    DWORD  dwErrCode
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PSTR pszData = NULL;
    PSTR pszDescription = NULL;
    PCSTR pszLoginPhase = NULL;
    DWORD dwEventID = 0;

    switch(dwLoginPhase)
    {
        case LSASS_EVENT_LOGON_PHASE_AUTHENTICATE:
            pszLoginPhase = "User authenticate";
            if (dwFlags & LSA_AUTH_USER_PAM_FLAG_SMART_CARD)
            {
                dwEventID = LSASS_EVENT_SUCCESSFUL_AUTHENTICATE_SMARTCARD;
            }
            else
            {
                dwEventID = LSASS_EVENT_SUCCESSFUL_AUTHENTICATE_SSH +
                    LsaSrvGetPamSourceOffset(pszPamSource);
            }
            break;

        case LSASS_EVENT_LOGON_PHASE_CREATE_SESSION:
            pszLoginPhase = "User PAM session create";
            dwEventID = LSASS_EVENT_SUCCESSFUL_LOGON_CREATE_SESSION;
            break;

        case LSASS_EVENT_LOGON_PHASE_CHECK_USER:
            pszLoginPhase = "User membership check of the restricted logon list";
            dwEventID = LSASS_EVENT_SUCCESSFUL_LOGON_CHECK_USER;
            break;

        default:
            pszLoginPhase = "Unknown login phase";
    }

    if (pszPamSource != NULL)
    {
        dwError = LwAllocateStringPrintf(
                     &pszDescription,
                     "Successful Logon:\r\n\r\n" \
                     "     Authentication provider: %s\r\n" \
                     "     Caller euid:             %u\r\n" \
                     "\r\n" \
                     "     User Name:               %s\r\n" \
                     "     Login phase:             %s\r\n" \
                     "     Pam source:              %s\r\n",
                     pszProvider,
                     pServerState->peerUID,
                     pszLoginId,
                     pszLoginPhase,
                     pszPamSource);
    }
    else
    {
        dwError = LwAllocateStringPrintf(
                     &pszDescription,
                     "Successful Logon:\r\n\r\n" \
                     "     Authentication provider: %s\r\n" \
                     "     Caller euid:             %u\r\n" \
                     "\r\n" \
                     "     User Name:               %s\r\n" \
                     "     Login phase:             %s\r\n",
                     pszProvider,
                     pServerState->peerUID,
                     pszLoginId,
                     pszLoginPhase);
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                     dwErrCode,
                     &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvLogSuccessAuditEvent(
                     dwEventID,
                     pszLoginId,
                     LOGIN_LOGOFF_EVENT_CATEGORY,
                     pszDescription,
                     pszData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszData);
    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    LSA_LOG_ERROR("Failed to post login success event for [%s]", LSA_SAFE_LOG_STRING(pszLoginId));
    LSA_LOG_ERROR("Error code: [%u]", dwError);

    goto cleanup;
}

VOID
LsaSrvWriteLoginFailedEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPamSource,
    DWORD  dwFlags,
    DWORD  dwLoginPhase,
    DWORD  dwErrCode
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    char  szReason[256] = {0};
    char  szLoginPhase[256] = {0};
    PSTR  pszData = NULL;
    DWORD dwEventID = 0;
    PSTR  pszDescription = NULL;

    switch(dwLoginPhase)
    {
        case LSASS_EVENT_LOGON_PHASE_AUTHENTICATE:
            sprintf(szLoginPhase, "User authenticate");
            break;

        case LSASS_EVENT_LOGON_PHASE_CREATE_SESSION:
            sprintf(szLoginPhase, "User PAM session create");
            break;

        case LSASS_EVENT_LOGON_PHASE_CHECK_USER:
            sprintf(szLoginPhase, "User membership check of the restricted logon list");
            break;

        default:
            sprintf(szLoginPhase, "Unknown login phase");
    }

    switch(dwErrCode)
    {
        case LW_ERROR_NO_SUCH_USER:
        case LW_ERROR_INVALID_PASSWORD:
        case LW_ERROR_PASSWORD_MISMATCH:
            if (dwLoginPhase == LSASS_EVENT_LOGON_PHASE_AUTHENTICATE)
            {
                if (dwFlags & LSA_AUTH_USER_PAM_FLAG_SMART_CARD)
                {
                    dwEventID = LSASS_EVENT_FAILED_AUTHENTICATE_SMARTCARD;
                }
                else
                {
                    dwEventID = LSASS_EVENT_FAILED_AUTHENTICATE_SSH +
                        LsaSrvGetPamSourceOffset(pszPamSource);
                }
            }
            else
            {
                dwEventID = LSASS_EVENT_FAILED_LOGON_UNKNOWN_USERNAME_OR_BAD_PASSWORD;
            }
            strcpy(szReason, "Unknown username or bad password");
            break;

        case SCARD_E_NO_SMARTCARD:
            dwEventID = LSASS_EVENT_FAILED_AUTHENTICATE_SMARTCARD;
            strcpy(szReason, "No smart card was found");
            break;

        case SCARD_E_NO_SUCH_CERTIFICATE:
            dwEventID = LSASS_EVENT_FAILED_AUTHENTICATE_SMARTCARD;
            strcpy(szReason, "No certificate/private key combination found");
            break;

        case ERROR_DECRYPTION_FAILED:
            dwEventID = LSASS_EVENT_FAILED_AUTHENTICATE_SMARTCARD;
            strcpy(szReason, "Encrypt/decrypt validation failed (certificate/private key mismatch)");
            break;

/* Not yet supported, MIT Kerberos needs to return a specific error for these conditions ...
        case LW_ERROR_XYZ:
            dwEventID = LSASS_EVENT_FAILED_LOGON_TIME_RESTRICTION_VIOLATION;
            strcpy(szReason, "Account logon time restriction violation");
            break;
*/

        case LW_ERROR_ACCOUNT_DISABLED:
            dwEventID = LSASS_EVENT_FAILED_LOGON_ACCOUNT_DISABLED;
            strcpy(szReason, "Account currently disabled");
            break;

        case LW_ERROR_ACCOUNT_EXPIRED:
            dwEventID = LSASS_EVENT_FAILED_LOGON_ACCOUNT_EXPIRED;
            strcpy(szReason, "The specified user account has expired");
            break;

/* Not yet supported, MIT Kerberos needs to return a specific error for these conditions ...
        case LW_ERROR_XYZ:
            dwEventID = LSASS_EVENT_FAILED_LOGON_MACHINE_RESTRICTION_VIOLATION;
            strcpy(szReason, "User not allowed to logon at this computer");
            break;

        case LW_ERROR_XYZ:
            dwEventID = LSASS_EVENT_FAILED_LOGON_TYPE_OF_LOGON_NOT_GRANTED;
            strcpy(szReason, "The user has not been granted the requested logon type at this machine");
            break;
*/

        case LW_ERROR_PASSWORD_EXPIRED:
            dwEventID = LSASS_EVENT_FAILED_LOGON_PASSWORD_EXPIRED;
            strcpy(szReason, "The specified account's password has expired");
            break;

        case LW_ERROR_INVALID_NETLOGON_RESPONSE:
        case LW_ERROR_RPC_NETLOGON_FAILED:
            dwEventID = LSASS_EVENT_FAILED_LOGON_NETLOGON_FAILED;
            strcpy(szReason, "The NetLogon component is not active");
            break;

        case LW_ERROR_ACCOUNT_LOCKED:
            dwEventID = LSASS_EVENT_FAILED_LOGON_ACCOUNT_LOCKED;
            strcpy(szReason, "Account locked out");
            break;

        default:
        {
            char szLwError[256];

            dwEventID = LSASS_EVENT_FAILED_LOGON_UNEXPECTED_ERROR;
            LwGetErrorString(dwErrCode, szLwError, sizeof(szLwError));
            snprintf(
                szReason,
                sizeof(szReason),
                "An unexpected error occurred: Error %d (%s)",
                dwErrCode,
                szLwError);
        }
    }

    if (dwErrCode == LSASS_EVENT_LOGON_PHASE_CHECK_USER)
    {
        dwEventID = LSASS_EVENT_FAILED_LOGON_CHECK_USER;
    }

    if (pszPamSource != NULL)
    {
        dwError = LwAllocateStringPrintf(
                     &pszDescription,
                     "Logon Failure:\r\n\r\n" \
                     "     Authentication provider: %s\r\n" \
                     "     Caller euid:             %u\r\n" \
                     "\r\n" \
                     "     Reason:                  %s\r\n" \
                     "     User Name:               %s\r\n" \
                     "     Login phase:             %s\r\n" \
                     "     Pam source:              %s\r\n",
                     pszProvider,
                     pServerState->peerUID,
                     szReason,
                     pszLoginId,
                     szLoginPhase,
                     pszPamSource);
    }
    else
    {
        dwError = LwAllocateStringPrintf(
                     &pszDescription,
                     "Logon Failure:\r\n\r\n" \
                     "     Authentication provider: %s\r\n" \
                     "     Caller euid:             %u\r\n" \
                     "\r\n" \
                     "     Reason:                  %s\r\n" \
                     "     User Name:               %s\r\n" \
                     "     Login phase:             %s\r\n",
                     pszProvider,
                     pServerState->peerUID,
                     szReason,
                     pszLoginId,
                     szLoginPhase);
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                     dwErrCode,
                     &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvLogFailureAuditEvent(
                     dwEventID,
                     pszLoginId,
                     LOGIN_LOGOFF_EVENT_CATEGORY,
                     pszDescription,
                     pszData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszData);
    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    LSA_LOG_ERROR("Failed to post login failure event for [%s]", LSA_SAFE_LOG_STRING(pszLoginId));
    LSA_LOG_ERROR("Error code: [%u]", dwError);

    goto cleanup;
}

VOID
LsaSrvWriteLogoutSuccessEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    DWORD  dwLoginPhase,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    char  szLoginPhase[256] = {0};

    switch(dwLoginPhase)
    {
        /* Not applicable - we cannot detect these phases
        case LSASS_EVENT_LOGON_PHASE_AUTHENTICATE:
            sprintf(szLoginPhase, "User authenticate");
            break;

        case LSASS_EVENT_LOGON_PHASE_CHECK_USER:
            sprintf(szLoginPhase, "User membership check of the restricted logon list");
            break;
        */

        case LSASS_EVENT_LOGON_PHASE_CREATE_SESSION:
            sprintf(szLoginPhase, "User PAM session closed");
            break;

        default:
            sprintf(szLoginPhase, "Unknown login phase");
    }

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "User Logoff:\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     User Name:               %s\r\n" \
                 "     Login phase:             %s",
                 pszProvider,
                 pszLoginId,
                 szLoginPhase);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvLogSuccessAuditEvent(
                     LSASS_EVENT_SUCCESSFUL_LOGOFF,
                     pszLoginId,
                     LOGIN_LOGOFF_EVENT_CATEGORY,
                     pszDescription,
                     NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    LSA_LOG_ERROR("Failed to post logout success event for [%s]", LSA_SAFE_LOG_STRING(pszLoginId));
    LSA_LOG_ERROR("Error code: [%u]", dwError);

    goto cleanup;
}

VOID
LsaSrvWriteUserPWChangeSuccessEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId
    )
{   
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Change Password Attempt:\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     Target Account Name:     %s",
                 pszProvider,
                 LSA_SAFE_LOG_STRING(pszLoginId));
    BAIL_ON_LSA_ERROR(dwError);
   
    dwError = LsaSrvLogSuccessAuditEvent(
                  LSASS_EVENT_SUCCESSFUL_PASSWORD_CHANGE,
                  pszLoginId,
                  PASSWORD_EVENT_CATEGORY,
                  pszDescription,
                  NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    LSA_LOG_ERROR("Failed to post user password change success event.");
    LSA_LOG_ERROR("Error code: [%u]", dwError);

    goto cleanup;
}

VOID
LsaSrvWriteUserPWChangeFailureEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId,
    DWORD  dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Change Password Attempt:\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     Target Account Name:     %s",
                 pszProvider,
                 LSA_SAFE_LOG_STRING(pszLoginId));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                     dwErrCode,
                     &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvLogFailureAuditEvent(
                  LSASS_EVENT_FAILED_PASSWORD_CHANGE,
                  pszLoginId,
                  PASSWORD_EVENT_CATEGORY,
                  pszDescription,
                  pszData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszData);

    return;

error:

    LSA_LOG_ERROR("Failed to post user password change failed event.");
    LSA_LOG_ERROR("Error code: [%u]", dwError);

    goto cleanup;
}


