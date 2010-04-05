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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        db_p.h
 *
 * Abstract:
 *
 *        Machine Password Database API
 * 
 *        Private Header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#ifndef __DB_P_H__
#define __DB_P_H__

typedef struct _MACHINE_ACCT_INFO {
    PSTR   pszDomainSID;
    PSTR   pszDomainName;
    PSTR   pszDomainDnsName;
    PSTR   pszHostName;
    PSTR   pszHostDnsDomain;
    PSTR   pszMachineAccountName;
    PSTR   pszMachineAccountPassword;
    time_t tPwdCreationTimestamp;
    time_t tPwdClientModifyTimestamp;
    UINT32 dwSchannelType;
} MACHINE_ACCT_INFO, *PMACHINE_ACCT_INFO;

DWORD
SqlDBDbInitGlobals();

DWORD
SqlDBCreateDb();

DWORD
SqlDBOpen(
    PHANDLE phDb
    );

DWORD
SqlDBSetPwdEntry(
    HANDLE             hDb,
    PMACHINE_ACCT_INFO pAcct
    );

DWORD
SqlDBGetPwdEntryByDomainDnsName(
    HANDLE              hDb,
    PCSTR               pszDomainDnsName,
    PMACHINE_ACCT_INFO* ppAcct
    );

DWORD
SqlDBGetPwdEntryByHostName(
    HANDLE              hDb,
    PCSTR               pszHostName,
    PMACHINE_ACCT_INFO* ppAcct
    );

DWORD
SqlDBDeletePwdEntryByHostName(
    HANDLE hDb,
    PCSTR  pszHostName
    );

DWORD
SqlDBDeleteAllEntries(
    HANDLE hDb
    );

DWORD
SqlDBGetPwdEntry(
    HANDLE              hDb,
    PCSTR               pszQuery,
    PMACHINE_ACCT_INFO* pAcct
    );

DWORD
SqlDBGetPwdEntries(
    HANDLE               hDb,
    PMACHINE_ACCT_INFO** pppAcct,
    PDWORD               pdwNumEntries
    );

VOID
SqlDBFreeEntryList(
    PMACHINE_ACCT_INFO* ppAcct,
    DWORD               dwNumEntries
    );

VOID
SqlDBFreeMachineAcctInfo(
    PMACHINE_ACCT_INFO pAcct
    );

VOID
SqlDBClose(
    HANDLE hDb
    );

#endif /* __DB_P_H__ */
