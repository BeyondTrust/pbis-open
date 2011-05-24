/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#ifndef __DJ_AUTHINFO_H__
#define __DJ_AUTHINFO_H__

void
DJGetComputerDN(PSTR *dn, LWException **exc);

void DJCreateComputerAccount(
                PSTR *shortDomainName,
                JoinProcessOptions *options,
                LWException **exc);

void DJDisableComputerAccount(PCSTR username,
                PCSTR password,
                JoinProcessOptions *options,
                LWException **exc);

//The answer is non-authoritative
void DJGuessShortDomainName(PCSTR longName,
                PSTR *shortName,
                LWException **exc);

extern const JoinModule DJDoJoinModule;
extern const JoinModule DJDoLeaveModule;
extern const JoinModule DJCacheModule;

void
DJGetConfiguredDnsDomain(
    PSTR* ppszDomain,
    LWException **exc
    );

void
DJGetConfiguredShortDomain(
    PSTR* ppszWorkgroup,
    LWException **exc
    );

DWORD
DJSetMachineSID(
    PSTR pszMachineSID
    );

DWORD
DJGetMachineSID(
    PSTR* ppszMachineSID
    );

DWORD
DJOpenEventLog(
    PHANDLE phEventLog
    );

DWORD
DJCloseEventLog(
    HANDLE hEventLog
    );

DWORD
DJLogInformationEvent(
    HANDLE hEventLog,
    DWORD  dwEventID,
    PCSTR  pszUser, // NULL defaults to SYSTEM
    PCSTR  pszComputer,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

DWORD
DJLogWarningEvent(
    HANDLE hEventLog,
    DWORD  dwEventID,
    PCSTR  pszUser, // NULL defaults to SYSTEM
    PCSTR  pszComputer,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

DWORD
DJLogErrorEvent(
    HANDLE hEventLog,
    DWORD  dwEventID,
    PCSTR  pszUser, // NULL defaults to SYSTEM
    PCSTR  pszComputer,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    );

VOID
DJLogDomainJoinSucceededEvent(
    JoinProcessOptions * JoinOptions,
    PSTR pszOSName,
    PSTR pszDistroVersion,
    PSTR pszLikewiseVersion
    );

VOID
DJLogDomainJoinFailedEvent(
    JoinProcessOptions * JoinOptions,
    PSTR pszOSName,
    PSTR pszDistroVersion,
    PSTR pszLikewiseVersion,
    LWException *exc
    );

VOID
DJLogDomainLeaveSucceededEvent(
    JoinProcessOptions * JoinOptions
    );

VOID
DJLogDomainLeaveFailedEvent(
    JoinProcessOptions * JoinOptions,
    LWException *exc
    );

void SetLsassTimeSync(
    BOOLEAN sync,
    LWException **exc);

#endif /* __DJ_AUTHINFO_H__ */
