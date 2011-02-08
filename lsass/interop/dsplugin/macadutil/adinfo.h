/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#ifndef __ADINFO_H__
#define __ADINFO_H__


DWORD
ADUParseAndGetGPTVersion(
    PSTR   pszFilePath,
    PDWORD pdwGPTVersion
    );

DWORD
ADUParseAndSetGPTVersion(
    PSTR  pszFilePath,
    DWORD dwGPTVersion
    );

DWORD
ADUGetGPTFileAndVersionNumber(
    PSTR   pszgPCFileSysPath,
    PSTR * ppszGPTFilePath,
    PDWORD pdwGPTVersion
    );

DWORD
ADUSetGPTVersionNumber(
    PSTR  pszgPCFileSysPath,
    DWORD dwFileVersion
    );

VOID
ADUGetComputerAndUserVersionNumbers(
    DWORD dwVersion,
    PWORD pwUserVersion,
    PWORD pwComputerVersion
    );

DWORD
ADUGetVersionFromUserAndComputer(
    WORD wUser,
    WORD wComputer
    );

DWORD
ADUGetAllMCXPolicies(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PGROUP_POLICY_OBJECT * ppGroupPolicyObjects
    );

DWORD
ADUGetMCXPolicy(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PCSTR  pszGPOName,
    PGROUP_POLICY_OBJECT * ppGroupPolicyObjects
    );

DWORD
ADUGetPolicyInformation(
    HANDLE hDirectory,
    PCSTR  pszPolicyDN,
    PGROUP_POLICY_OBJECT pGroupPolicyObject
    );

DWORD
ADUSetPolicyVersionInAD(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD dwVersion);

#endif /* __ADINFO_H__ */
