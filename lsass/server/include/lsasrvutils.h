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
 *        lsasrvutils.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *
 *        A utility library that is only used by lsassd, not the nsswitch,
 *        lsaclient, or pam libraries. This utility library can safely link
 *        to libpthread, while client libraries cannot.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#ifndef __LSASRVUTILS_H__
#define __LSASRVUTILS_H__

#include <reg/lwreg.h>

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
#define LSASS_EVENT_SUCCESSFUL_LOGON_AUTHENTICATE                    1200 // Similar to Window event id 528 - deprecated
#define LSASS_EVENT_SUCCESSFUL_LOGON_CREATE_SESSION                  1201
#define LSASS_EVENT_SUCCESSFUL_LOGON_CHECK_USER                      1203
// Only fired for the non-authenticate phases
#define LSASS_EVENT_FAILED_LOGON_UNKNOWN_USERNAME_OR_BAD_PASSWORD    1205
#define LSASS_EVENT_FAILED_LOGON_TIME_RESTRICTION_VIOLATION          1206
#define LSASS_EVENT_FAILED_LOGON_ACCOUNT_DISABLED                    1207
#define LSASS_EVENT_FAILED_LOGON_ACCOUNT_EXPIRED                     1208
#define LSASS_EVENT_FAILED_LOGON_MACHINE_RESTRICTION_VIOLATION       1209
#define LSASS_EVENT_FAILED_LOGON_TYPE_OF_LOGON_NOT_GRANTED           1210
#define LSASS_EVENT_FAILED_LOGON_PASSWORD_EXPIRED                    1211
#define LSASS_EVENT_FAILED_LOGON_NETLOGON_FAILED                     1212
#define LSASS_EVENT_FAILED_LOGON_UNEXPECTED_ERROR                    1213
#define LSASS_EVENT_FAILED_LOGON_ACCOUNT_LOCKED                      1214
#define LSASS_EVENT_FAILED_LOGON_CHECK_USER                          1215

#define LSASS_EVENT_LOGON_PHASE_AUTHENTICATE                         1
#define LSASS_EVENT_LOGON_PHASE_CREATE_SESSION                       2
#define LSASS_EVENT_LOGON_PHASE_CHECK_USER                           3

// Logoff events
#define LSASS_EVENT_SUCCESSFUL_LOGOFF                                1220

// Logon success events
#define LSASS_EVENT_SUCCESSFUL_AUTHENTICATE_SSH                      1230
#define LSASS_EVENT_SUCCESSFUL_AUTHENTICATE_GRAPHICAL                1231
#define LSASS_EVENT_SUCCESSFUL_AUTHENTICATE_CONSOLE                  1232
#define LSASS_EVENT_SUCCESSFUL_AUTHENTICATE_FTP                      1233
#define LSASS_EVENT_SUCCESSFUL_AUTHENTICATE_TELNET                   1234
#define LSASS_EVENT_SUCCESSFUL_AUTHENTICATE_SCREEENSAVER             1235
#define LSASS_EVENT_SUCCESSFUL_AUTHENTICATE_SUDO                     1236
#define LSASS_EVENT_SUCCESSFUL_AUTHENTICATE_PASSWD                   1237
#define LSASS_EVENT_SUCCESSFUL_AUTHENTICATE_SU                       1238
// 1239-1248 reserved
#define LSASS_EVENT_SUCCESSFUL_AUTHENTICATE_OTHER                    1249

// Logon failed events
// These are fired during the authenticate phase if the username is unknown or
// the password is incorrect. Disabled accounts, etc.. create different events.
#define LSASS_EVENT_FAILED_AUTHENTICATE_SSH                          1250
#define LSASS_EVENT_FAILED_AUTHENTICATE_GRAPHICAL                    1251
#define LSASS_EVENT_FAILED_AUTHENTICATE_CONSOLE                      1252
#define LSASS_EVENT_FAILED_AUTHENTICATE_FTP                          1253
#define LSASS_EVENT_FAILED_AUTHENTICATE_TELNET                       1254
#define LSASS_EVENT_FAILED_AUTHENTICATE_SCREENSAVER                  1255
#define LSASS_EVENT_FAILED_AUTHENTICATE_SUDO                         1256
#define LSASS_EVENT_FAILED_AUTHENTICATE_PASSWD                       1257
#define LSASS_EVENT_FAILED_AUTHENTICATE_SU                           1258
// 1259-1268 reserved
#define LSASS_EVENT_FAILED_AUTHENTICATE_OTHER                        1269

//
// These are outside the range of other AUTHENTICATE events
// because those events are indexed by the PAM source, and
// smartcard authentication is separate from that (although
// it currently only occurs on graphical logins).
//
#define LSASS_EVENT_SUCCESSFUL_AUTHENTICATE_SMARTCARD                1270
#define LSASS_EVENT_FAILED_AUTHENTICATE_SMARTCARD                    1271

// User password change events
#define LSASS_EVENT_SUCCESSFUL_PASSWORD_CHANGE                       1300
#define LSASS_EVENT_FAILED_PASSWORD_CHANGE                           1301
#define LSASS_EVENT_SUCCESSFUL_USER_ACCOUNT_KERB_REFRESH             1302
#define LSASS_EVENT_FAILED_USER_ACCOUNT_KERB_REFRESH                 1303

// Machine password change events
#define LSASS_EVENT_SUCCESSFUL_MACHINE_ACCOUNT_PASSWORD_UPDATE       1320
#define LSASS_EVENT_FAILED_MACHINE_ACCOUNT_PASSWORD_UPDATE           1321
#define LSASS_EVENT_SUCCESSFUL_MACHINE_ACCOUNT_TGT_REFRESH           1322
#define LSASS_EVENT_FAILED_MACHINE_ACCOUNT_TGT_REFRESH               1323

// Account management events
#define LSASS_EVENT_ADD_USER_ACCOUNT                                 1400
#define LSASS_EVENT_DELETE_USER_ACCOUNT                              1401
#define LSASS_EVENT_ADD_GROUP                                        1402
#define LSASS_EVENT_DELETE_GROUP                                     1403

// Lsass provider events
#define LSASS_EVENT_SUCCESSFUL_PROVIDER_INITIALIZATION               1500
#define LSASS_EVENT_FAILED_PROVIDER_INITIALIZATION                   1501
#define LSASS_EVENT_INFO_REQUIRE_MEMBERSHIP_OF_UPDATED               1502
#define LSASS_EVENT_INFO_AUDITING_CONFIGURATION_ENABLED              1503
#define LSASS_EVENT_INFO_AUDITING_CONFIGURATION_DISABLED             1504

// Runtime warnings
#define LSASS_EVENT_WARNING_CONFIGURATION_ID_CONFLICT                1601
#define LSASS_EVENT_WARNING_CONFIGURATION_ALIAS_CONFLICT             1602

// Network events
#define LSASS_EVENT_INFO_NETWORK_DOMAIN_ONLINE_TRANSITION            1700
#define LSASS_EVENT_WARNING_NETWORK_DOMAIN_OFFLINE_TRANSITION        1701

typedef enum
{
    NameType_NT4 = 0,
    NameType_UPN = 1,
    NameType_Alias
} ADLogInNameType;

typedef struct __LSA_LOGIN_NAME_INFO
{
    ADLogInNameType nameType;
    PSTR  pszDomain;
    PSTR  pszName;
} LSA_LOGIN_NAME_INFO, *PLSA_LOGIN_NAME_INFO;

#define LSA_SAFE_FREE_LOGIN_NAME_INFO(pLoginNameInfo) \
    do { \
        if (pLoginNameInfo) \
        { \
            LsaSrvFreeNameInfo(pLoginNameInfo); \
            (pLoginNameInfo) = NULL; \
        } \
    } while(0);


#define PTHREAD_CALL_MUST_SUCCEED(Call) \
    do { \
        int localError = Call; \
        LSA_ASSERT(localError == 0); \
    } while (0)
	
//Convert to seconds string of form ##s, ##m, ##h, or ##d
//where s,m,h,d = seconds, minutes, hours, days.
DWORD
LsaParseDateString(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    );

DWORD
LsaSetSystemTime(
    time_t ttCurTime
    );

DWORD
LsaGetCurrentTimeSeconds(
    OUT time_t* pTime
    );

VOID
LsaSrvLogUserIDConflictEvent(
    uid_t uid,
    PCSTR pszProviderName,
    DWORD dwErrCode
    );

VOID
LsaSrvLogUserAliasConflictEvent(
    PCSTR pszAlias,
    PCSTR pszProviderName,
    DWORD dwErrCode
    );

VOID
LsaSrvLogDuplicateObjectFoundEvent(
    PCSTR pszName1,
    PCSTR pszName2,
    PCSTR pszProviderName,
    DWORD dwErrCode
    );

#ifdef ENABLE_EVENTLOG
DWORD
LsaSrvStartEventLoggingThread(
    VOID
    );

DWORD
LsaSrvStopEventLoggingThread(
    VOID
    );

VOID
LsaSrvLogServiceSuccessEvent(
    DWORD dwEventID,
    PCSTR pszEventCategory,
    PCSTR pszDescription,
    PCSTR pszData
    );

VOID
LsaSrvLogServiceWarningEvent(
    DWORD dwEventID,
    PCSTR pszEventCategory,
    PCSTR pszDescription,
    PCSTR pszData
    );

VOID
LsaSrvLogServiceFailureEvent(
    DWORD dwEventID,
    PCSTR pszEventCategory,
    PCSTR pszDescription,
    PCSTR pszData
    );

DWORD
LsaSrvLogInformationEvent(
    DWORD  dwEventID,
    PCSTR  pszUser,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

DWORD
LsaSrvLogWarningEvent(
    DWORD  dwEventID,
    PCSTR  pszUser,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

DWORD
LsaSrvLogErrorEvent(
    DWORD  dwEventID,
    PCSTR  pszUser,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

DWORD
LsaSrvLogSuccessAuditEvent(
    DWORD  dwEventID,
    PCSTR  pszUser,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

DWORD
LsaSrvLogFailureAuditEvent(
    DWORD  dwEventID,
    PCSTR  pszUser,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );
#else

static inline
VOID
LsaSrvLogServiceSuccessEvent(
    DWORD dwEventID,
    PCSTR pszEventCategory,
    PCSTR pszDescription,
    PCSTR pszData
    )
{
}

static inline
VOID
LsaSrvLogServiceFailureEvent(
    DWORD dwEventID,
    PCSTR pszEventCategory,
    PCSTR pszDescription,
    PCSTR pszData
    )
{
}

static inline
DWORD
LsaSrvLogWarningEvent(
    DWORD  dwEventID,
    PCSTR  pszUser,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
    return 0;
}

static inline
VOID
LsaSrvLogServiceWarningEvent(
    DWORD dwEventID,
    PCSTR pszEventCategory,
    PCSTR pszDescription,
    PCSTR pszData
    )
{
}


#endif

DWORD
LsaSrvFlushSystemCache(
    VOID
    );

char
LsaSrvSpaceReplacement(
    VOID
    );

char
LsaSrvDomainSeparator(
    VOID
    );

DWORD
LsaSrvCrackDomainQualifiedName(
    PCSTR pszName,
    PLSA_LOGIN_NAME_INFO* ppNameInfo
    );

void
LsaSrvFreeNameInfo(
    PLSA_LOGIN_NAME_INFO pNameInfo
    );

DWORD
LsaInitTracing_r(
    VOID
    );

DWORD
LsaTraceSetFlag_r(
    DWORD   dwTraceFlag,
    BOOLEAN bStatus
    );

DWORD
LsaTraceGetInfo_r(
    DWORD    dwTraceFlag,
    PBOOLEAN pbStatus
    );

VOID
LsaShutdownTracing_r(
    VOID
    );

#endif /* __LSASRVUTILS_H__ */
