/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        samdb.h
 *
 * Abstract:
 *
 *
 *      BeyondTrust SAM Database Provider
 *
 *      SAM Provider API
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#ifndef __SAMDB_H__
#define __SAMDB_H__

DWORD
SamDbInit(
    VOID
    );

DWORD
SamDbOpen(
    PHANDLE phDirectory
    );

DWORD
SamDbBind(
    HANDLE hDirectory,
    PWSTR  pwszDistinguishedName,
    PWSTR  pwszCredential,
    ULONG  ulMethod
    );

DWORD
SamDbAddObject(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbModifyObject(
    HANDLE        hBindHandle,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD Modifications[]
    );

DWORD
SamDbSearchObject(
    HANDLE            hDirectory,
    PWSTR             pwszBaseDN,
    ULONG             ulScope,
    PWSTR             pwszFilter,
    PWSTR             pwszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
SamDbSearchObject_inlock(
    HANDLE            hDirectory,
    PWSTR             pwszBase,
    ULONG             ulScope,
    PWSTR             pwszFilter,
    PWSTR             wszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
SamDbDeleteObject(
    HANDLE hBindHandle,
    PWSTR  pwszObjectDN
    );

DWORD
SamDbAddUser(
    HANDLE        hDirectory,
    PWSTR         pwszUserDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbModifyUser(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbSearchUsers(
    HANDLE            hDirectory,
    PWSTR             pwszBaseDN,
    ULONG             ulScope,
    PWSTR             pwszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
SamDbDeleteUser(
    HANDLE hDirectory,
    PWSTR  pwszUserDN
    );

DWORD
SamDbAddGroup(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbNumMembersInGroup_inlock(
    HANDLE hDirectory,
    PCSTR  pszGroupName,
    PCSTR  pszDomain,
    PDWORD pdwNumGroupMembers
    );

DWORD
SamDbModifyGroup(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbSearchGroups(
    HANDLE            hDirectory,
    PWSTR             pwszBaseDN,
    ULONG             ulScope,
    PWSTR             pwszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
SamDbFindGID_inlock(
    HANDLE hDirectory,
    PWSTR  pwszGroupDN,
    PDWORD pdwGID
    );

DWORD
SamDbDeleteGroup(
    HANDLE hDirectory,
    PWSTR  pwszGroupDN
    );

DWORD
SamDbAddDomain(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbFindDomains(
    HANDLE                hDirectory,
    PWSTR                 pwszDomainName,
    PSAM_DB_DOMAIN_INFO** pppDomainInfoList,
    PDWORD                pdwNumDomains
    );

DWORD
SamDbBuildDomainInfo(
    PSTR*                 ppszResult,
    int                   nRows,
    int                   nCols,
    int                   nHeaderColsToSkip,
    PSAM_DB_DOMAIN_INFO** pppDomainInfo,
    PDWORD                pdwNumDomainsFound
    );

DWORD
SamDbModifyDomain(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbSearchDomains(
    HANDLE            hDirectory,
    PWSTR             pwszBaseDN,
    ULONG             ulScope,
    PWSTR             pwszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
SamDbDeleteDomain(
    HANDLE hDirectory,
    PWSTR  pswzObjectDN
    );

DWORD
SamDbNumObjectsInDomain_inlock(
    HANDLE hDirectory,
    PSTR   pszDomainName,
    PDWORD pdwNumObjects
    );

VOID
SamDbFreeDomainInfoList(
    PSAM_DB_DOMAIN_INFO* ppDomainInfoList,
    DWORD dwNumDomains
    );

VOID
SamDbFreeDomainInfo(
    PSAM_DB_DOMAIN_INFO pDomainInfo
    );

DWORD
SamDbBuildDomainDirectoryEntries(
    PSAM_DIRECTORY_CONTEXT pDirContext,
    PWSTR                  pwszAttributes[],
    ULONG                  ulAttributesOnly,
    PSAM_DB_DOMAIN_INFO*   ppDomainInfoList,
    DWORD                  dwNumDomains,
    PDIRECTORY_ENTRY*      ppDirectoryEntries
    );

DWORD
SamDbConvertFiltertoTable(
    PWSTR             pwszFilter,
    SAMDB_ENTRY_TYPE* pdwTable
    );

VOID
SamDbClose(
    HANDLE hDirectory
    );

#endif /* __SAMDB_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
