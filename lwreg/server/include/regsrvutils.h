/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        regsrvutils.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        A utility library that is only used by lsassd, not the nsswitch,
 *        lsaclient, or pam libraries. This utility library can safely link
 *        to libpthread, while client libraries cannot.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */
#ifndef __REGSRVUTILS_H__
#define __REGSRVUTILS_H__

#define LOGIN_LOGOFF_EVENT_CATEGORY "Login/Logoff"
#define PASSWORD_EVENT_CATEGORY     "Password"
#define KERBEROS_EVENT_CATEGORY     "Kerberos"
#define SERVICE_EVENT_CATEGORY      "Service"
#define ACCOUNT_MANAGEMENT_EVENT_CATEGORY "Account Management"
#define NETWORK_EVENT_CATEGORY      "Network"

#define SUCCESS_AUDIT_EVENT_TYPE    "Success Audit"
#define FAILURE_AUDIT_EVENT_TYPE    "Failure Audit"
#define INFORMATION_EVENT_TYPE      "Information"
#define WARNING_EVENT_TYPE          "Warning"
#define ERROR_EVENT_TYPE            "Error"

VOID
RegSrvLogServiceSuccessEvent(
    DWORD dwEventID,
    PCSTR pszEventCategory,
    PCSTR pszDescription,
    PCSTR pszData
    );

VOID
RegSrvLogServiceWarningEvent(
    DWORD dwEventID,
    PCSTR pszEventCategory,
    PCSTR pszDescription,
    PCSTR pszData
    );

VOID
RegSrvLogServiceFailureEvent(
    DWORD dwEventID,
    PCSTR pszEventCategory,
    PCSTR pszDescription,
    PCSTR pszData
    );

DWORD
RegSrvOpenEventLog(
    PSTR pszCategoryType,
    PHANDLE phEventLog
    );

DWORD
RegSrvLogInformationEvent(
    HANDLE hEventLog,
    DWORD  dwEventID,
    PCSTR  pszUser,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

DWORD
RegSrvLogWarningEvent(
    HANDLE hEventLog,
    DWORD  dwEventID,
    PCSTR  pszUser,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

DWORD
RegSrvLogErrorEvent(
    HANDLE hEventLog,
    DWORD  dwEventID,
    PCSTR  pszUser,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

DWORD
RegSrvLogSuccessAuditEvent(
    HANDLE hEventLog,
    DWORD  dwEventID,
    PCSTR  pszUser,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

DWORD
RegSrvLogFailureAuditEvent(
    HANDLE hEventLog,
    DWORD  dwEventID,
    PCSTR  pszUser,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

DWORD
RegSrvCloseEventLog(
    HANDLE hEventLog
    );

#endif /* __REGSRVUTILS_H__ */
