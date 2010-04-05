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


#define LSASS_EVENT_INFO_SERVICE_STARTED                             1000
#define LSASS_EVENT_ERROR_SERVICE_START_FAILURE                      1001
#define LSASS_EVENT_INFO_SERVICE_STOPPED                             1002
#define LSASS_EVENT_ERROR_SERVICE_STOPPED                            1003
#define LSASS_EVENT_INFO_SERVICE_CONFIGURATION_CHANGED               1004

// Logon events
#define LSASS_EVENT_SUCCESSFUL_LOGON_AUTHENTICATE                    1200 // Similar to Window event id 528
#define LSASS_EVENT_SUCCESSFUL_LOGON_CREATE_SESSION                  1201 // Similar to Window event id 528
#define LSASS_EVENT_SUCCESSFUL_LOGON_CHECK_USER                      1203 // Similar to Window event id 528
#define LSASS_EVENT_FAILED_LOGON_UNKNOWN_USERNAME_OR_BAD_PASSWORD    1205 // Similar to Window event id 529
#define LSASS_EVENT_FAILED_LOGON_TIME_RESTRICTION_VIOLATION          1206 // Similar to Window event id 530
#define LSASS_EVENT_FAILED_LOGON_ACCOUNT_DISABLED                    1207 // Similar to Window event id 531
#define LSASS_EVENT_FAILED_LOGON_ACCOUNT_EXPIRED                     1208 // Similar to Window event id 532
#define LSASS_EVENT_FAILED_LOGON_MACHINE_RESTRICTION_VIOLATION       1209 // Similar to Window event id 533
#define LSASS_EVENT_FAILED_LOGON_TYPE_OF_LOGON_NOT_GRANTED           1210 // Similar to Window event id 534
#define LSASS_EVENT_FAILED_LOGON_PASSWORD_EXPIRED                    1211 // Similar to Window event id 535
#define LSASS_EVENT_FAILED_LOGON_NETLOGON_FAILED                     1212 // Similar to Window event id 536
#define LSASS_EVENT_FAILED_LOGON_UNEXPECTED_ERROR                    1213 // Similar to Window event id 537
#define LSASS_EVENT_FAILED_LOGON_ACCOUNT_LOCKED                      1214 // Similar to Window event id 539

#define LSASS_EVENT_LOGON_PHASE_AUTHENTICATE                         1
#define LSASS_EVENT_LOGON_PHASE_CREATE_SESSION                       2
#define LSASS_EVENT_LOGON_PHASE_CHECK_USER                           3

// Logoff events
#define LSASS_EVENT_SUCCESSFUL_LOGOFF                                1220 // Similar to Window event id 538

// User password change events
#define LSASS_EVENT_SUCCESSFUL_PASSWORD_CHANGE                       1300 // Similar to Window event id 627
#define LSASS_EVENT_FAILED_PASSWORD_CHANGE                           1301 // Similar to Window event id 627
#define LSASS_EVENT_SUCCESSFUL_USER_ACCOUNT_KERB_REFRESH             1302
#define LSASS_EVENT_FAILED_USER_ACCOUNT_KERB_REFRESH                 1303

// Machine password change events
#define LSASS_EVENT_SUCCESSFUL_MACHINE_ACCOUNT_PASSWORD_UPDATE       1320
#define LSASS_EVENT_FAILED_MACHINE_ACCOUNT_PASSWORD_UPDATE           1321
#define LSASS_EVENT_SUCCESSFUL_MACHINE_ACCOUNT_TGT_REFRESH           1322
#define LSASS_EVENT_FAILED_MACHINE_ACCOUNT_TGT_REFRESH               1323

// Account management events
#define LSASS_EVENT_ADD_USER_ACCOUNT                                 1400 // Similar to Window event id 624
#define LSASS_EVENT_DELETE_USER_ACCOUNT                              1401 // Similar to Window event id 630
#define LSASS_EVENT_ADD_GROUP                                        1402 // Similar to Window event id 635
#define LSASS_EVENT_DELETE_GROUP                                     1403 // Similar to Window event id 638

// Regss provider events
#define LSASS_EVENT_SUCCESSFUL_PROVIDER_INITIALIZATION               1500
#define LSASS_EVENT_FAILED_PROVIDER_INITIALIZATION                   1501

// Runtime warnings
#define LSASS_EVENT_WARNING_CONFIGURATION_ID_CONFLICT                1601
#define LSASS_EVENT_WARNING_CONFIGURATION_ALIAS_CONFLICT             1602

// Network events
#define LSASS_EVENT_INFO_NETWORK_DOMAIN_ONLINE_TRANSITION            1700
#define LSASS_EVENT_WARNING_NETWORK_DOMAIN_OFFLINE_TRANSITION        1701





//Convert to seconds string of form ##s, ##m, ##h, or ##d
//where s,m,h,d = seconds, minutes, hours, days.
DWORD
RegParseDateString(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    );

DWORD
RegSetSystemTime(
    time_t ttCurTime
    );

DWORD
RegGetCurrentTimeSeconds(
    OUT time_t* pTime
    );

VOID
RegSrvLogUserAliasConflictEvent(
    PCSTR pszAlias,
    PCSTR pszProviderName,
    DWORD dwErrCode
    );

VOID
RegSrvLogDuplicateObjectFoundEvent(
    PCSTR pszName1,
    PCSTR pszName2,
    PCSTR pszProviderName,
    DWORD dwErrCode
    );

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

DWORD
RegSrvFlushSystemCache(
    VOID
    );

#endif /* __REGSRVUTILS_H__ */
