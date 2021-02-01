/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lpevent.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Event Logging
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

VOID
LocalEventLogServiceStart(
    DWORD dwErrCode
    )
{
    DWORD  dwError = 0;
    HANDLE hDirectory = (HANDLE)NULL;
    DWORD  dwUserCount = 0;
    DWORD  dwGroupCount = 0;
    PSTR   pszData = NULL;
    PSTR   pszDescription = NULL;
    PWSTR  pwszDN = NULL;
    PWSTR  pwszCredentials = NULL;
    ULONG  ulMethod = 0;

    dwError = DirectoryOpen(&hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryBind(
                    hDirectory,
                    pwszDN,
                    pwszCredentials,
                    ulMethod);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetUserCount(
                    hDirectory,
                    &dwUserCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetGroupCount(
                    hDirectory,
                    &dwGroupCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Likewise authentication service provider initialization %s.\r\n\r\n" \
                 "     Authentication provider:        %s\r\n\r\n" \
                 "     Current number of local users:  %u\r\n" \
                 "     Current number of local groups: %u",
                 dwErrCode ? "failed" : "succeeded",
                 LSA_SAFE_LOG_STRING(gpszLocalProviderName),
                 dwUserCount,
                 dwGroupCount);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwErrCode)
    {
        dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);
        BAIL_ON_LSA_ERROR(dwError);

        LsaSrvLogServiceFailureEvent(
                 LSASS_EVENT_FAILED_PROVIDER_INITIALIZATION,
                 SERVICE_EVENT_CATEGORY,
                 pszDescription,
                 pszData);
    }
    else
    {
        LsaSrvLogServiceSuccessEvent(
                 LSASS_EVENT_SUCCESSFUL_PROVIDER_INITIALIZATION,
                 SERVICE_EVENT_CATEGORY,
                 pszDescription,
                 NULL);
    }

cleanup:

    if (hDirectory)
    {
        DirectoryClose(hDirectory);
    }

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}

VOID
LocalEventLogConfigReload(
    VOID
    )
{
    DWORD   dwError = 0;
    PSTR    pszDescription = NULL;
    LONG64  llMaxPwdAge = 0;
    LONG64  llPasswdChangeWarningTime = 0;
    BOOLEAN bEventlogEnabled = FALSE;
    PCSTR   pszFormat =
        "Likewise authentication service provider configuration settings have been reloaded.\r\n\r\n"
        "     Authentication provider:       %s\r\n\r\n"
        "     Current settings are...\r\n"
        "     Enable event log:              %s\r\n"
        "     Password change interval:      %ld\r\n"
        "     Password change warning time : %ld";
    PCSTR   pszEventLogEnabled = NULL;

    dwError = LocalCfgGetMaxPasswordAge(&llMaxPwdAge);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCfgGetPasswordChangeWarningTime(&llPasswdChangeWarningTime);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCfgIsEventlogEnabled(&bEventlogEnabled);
    BAIL_ON_LSA_ERROR(dwError);

    pszEventLogEnabled = bEventlogEnabled ? "true" : "false";

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 pszFormat,
                 gpszLocalProviderName,
                 pszEventLogEnabled,
                 (LONG64)(llMaxPwdAge/10000000LL),
                 (LONG64)(llPasswdChangeWarningTime/10000000LL));
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
             LSASS_EVENT_INFO_SERVICE_CONFIGURATION_CHANGED,
             SERVICE_EVENT_CATEGORY,
             pszDescription,
             NULL);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
LocalEventLogUserAdd(
    PCSTR pszUsername,
    uid_t uid
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "User account created.\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     User name:               %s\r\n" \
                 "     UID:                     %u",
                 LSA_SAFE_LOG_STRING(gpszLocalProviderName),
                 LSA_SAFE_LOG_STRING(pszUsername),
                 uid);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_ADD_USER_ACCOUNT,
            ACCOUNT_MANAGEMENT_EVENT_CATEGORY,
            pszDescription,
            NULL);
cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
LocalEventLogUserDelete(
    uid_t uid
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "User account deleted.\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     UID: %u",
                 LSA_SAFE_LOG_STRING(gpszLocalProviderName),
                 uid);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_DELETE_USER_ACCOUNT,
            ACCOUNT_MANAGEMENT_EVENT_CATEGORY,
            pszDescription,
            NULL);
cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
LocalEventLogGroupAdd(
    PCSTR pszGroupname,
    gid_t gid
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Group created.\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     Group name:  %s\r\n" \
                 "     GID: %u",
                 LSA_SAFE_LOG_STRING(gpszLocalProviderName),
                 LSA_SAFE_LOG_STRING(pszGroupname),
                 gid);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_ADD_GROUP,
            ACCOUNT_MANAGEMENT_EVENT_CATEGORY,
            pszDescription,
            NULL);
cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
LocalEventLogGroupDelete(
    gid_t gid
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Group deleted.\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     GID: %u",
                 LSA_SAFE_LOG_STRING(gpszLocalProviderName),
                 gid);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_DELETE_GROUP,
            ACCOUNT_MANAGEMENT_EVENT_CATEGORY,
            pszDescription,
            NULL);
cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}
