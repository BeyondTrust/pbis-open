/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lpevent.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
