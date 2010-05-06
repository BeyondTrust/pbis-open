/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsaprovider2.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Authentication Provider Interface Version 2
 *
 * Authors: Krishna Ganugapati (krishnag@likewisee.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Danilo Almeida (dalmeida@likewise.com)
 */
#ifndef __LSAPROVIDER_2_H__
#define __LSAPROVIDER_2_H__

#include <lsa/lsa2.h>

#include "lsaprovider.h"

//
// New Interfaces
//

//
// Lookup objects.
//
// Objects returned in same order as query with NULL
// entries for objects that are not found.
//
typedef DWORD (*PFN_LSA_PROVIDER_FIND_OBJECTS)(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

//
// Enumerate Users/Groups
//
typedef DWORD (*PFN_LSA_PROVIDER_OPEN_ENUM_OBJECTS)(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    );

typedef DWORD (*PFN_LSA_PROVIDER_ENUM_OBJECTS)(
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

//
// Enumerate members of a group.
//
//
typedef DWORD (*PFN_LSA_PROVIDER_OPEN_ENUM_MEMBERS)(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    );

typedef DWORD (*PFN_LSA_PROVIDER_ENUM_MEMBERS)(
    IN HANDLE hEnum,
    IN DWORD dwMaxMemberSidCount,
    OUT PDWORD pdwMemberSidCount,
    OUT PSTR** pppszMemberSids
    );

typedef DWORD (*PFN_LSA_PROVIDER_QUERY_MEMBER_OF)(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    );

//
// Close any enumeration handle.
//
typedef VOID (*PFN_LSA_PROVIDER_CLOSE_ENUM)(
    IN OUT HANDLE hEnum
    );

typedef DWORD (*PFNMODIFYUSER_2)(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO_2 pUserModInfo
    );

typedef DWORD (*PFNDELETEOBJECT) (
    HANDLE hProvider,
    PCSTR pszSid
    );

typedef DWORD (*PFNMODIFYGROUP_2) (
    HANDLE hProvider,
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo
    );

typedef DWORD (*PFNADDUSER_2) (
    HANDLE hProvider,
    PLSA_USER_ADD_INFO pUserInfo
    );

typedef DWORD (*PFNADDGROUP_2) (
    HANDLE hProvider,
    PLSA_GROUP_ADD_INFO pGroupInfo
    );

typedef DWORD (*PFNOPENHANDLE_2)(
    HANDLE hServer,
    PHANDLE phProvider
    );

typedef struct _LSA_PROVIDER_FUNCTION_TABLE_2 {

    PFN_LSA_PROVIDER_FIND_OBJECTS pfnFindObjects;
    //
    // Deprecates:
    //
    // PFNLOOKUPUSERBYNAME            pfnLookupUserByName;
    // PFNLOOKUPUSERBYID              pfnLookupUserById;
    // PFNLOOKUPGROUPBYNAME           pfnLookupGroupByName;
    // PFNLOOKUPGROUPBYID             pfnLookupGroupById;
    // PFNGETNAMESBYSIDLIST           pfnGetNamesBySidList;
    //
    // Adds:
    //
    // FindUserBySid
    // FindGroupBySid
    // FindUserByBn
    // FindGroupByDn
    //

    PFN_LSA_PROVIDER_OPEN_ENUM_OBJECTS pfnOpenEnumObjects;
    //
    // Deprecates:
    //
    // PFNBEGIN_ENUM_USERS            pfnBeginEnumUsers;
    // PFNBEGIN_ENUM_GROUPS           pfnBeginEnumGroups;
    //

    PFN_LSA_PROVIDER_OPEN_ENUM_MEMBERS pfnOpenEnumGroupMembers;
    //
    // Adds:
    //
    // BeginEnumUserGroups (paging functionality)
    // BeginEnumGroupMembers (paging functionality)
    //

    PFN_LSA_PROVIDER_CLOSE_ENUM pfnCloseEnum;
    //
    // Deprecates:
    //
    // PFNEND_ENUM_USERS              pfnEndEnumUsers;
    // PFNEND_ENUM_GROUPS             pfnEndEnumGroups;
    //
    // Adds:
    //
    // EndEnumUserGroups (paging functionality)
    // EndEnumGroupMembers (paging functionality)
    //

    PFN_LSA_PROVIDER_ENUM_OBJECTS pfnEnumObjects;
    PFN_LSA_PROVIDER_ENUM_MEMBERS pfnEnumGroupMembers;
    PFN_LSA_PROVIDER_QUERY_MEMBER_OF pfnQueryMemberOf;
    //
    // Deprecates:
    //
    // PFNENUMUSERS                   pfnEnumUsers;
    // PFNGETGROUPSFORUSER            pfnGetGroupsForUser;
    // PFNENUMGROUPS                  pfnEnumGroups;
    // PFNGETGROUPMEMBERSHIPBYPROV    pfnGetGroupMembershipByProvider;
    // group nfo level 1
    //
    // Adds:
    //
    // ability to get members of groups as something other than aliases.
    //

#if 1
    //
    // Untouched for now -- will at least change type names for readability/consistency.
    //
    PFNSHUTDOWNPROVIDER            pfnShutdownProvider; // ok
    PFNOPENHANDLE_2                pfnOpenHandle; // we should be able to get rid of this and just pass in a LSA_PROVIDER_HANDLE that is created by SRV/API but that provider can attach context.
    PFNCLOSEHANDLE                 pfnCloseHandle; // "
    PFNSERVICESDOMAIN              pfnServicesDomain; // is it necessary?  if we can lookup domains, it is not.
    PFNAUTHENTICATEUSERPAM         pfnAuthenticateUserPam; // ok
    PFNAUTHENTICATEUSEREX          pfnAuthenticateUserEx; // ok
    PFNVALIDATEUSER                pfnValidateUser; // This can be combined with the below (removing password)
    PFNCHECKUSERINLIST             pfnCheckUserInList; // see above
    PFNCHANGEPASSWORD              pfnChangePassword; // ok
    PFNSETPASSWORD                 pfnSetPassword; // ok -- local only unless we support set password protocol to set password as domain admin

    // local only?:
    PFNADDUSER_2                   pfnAddUser;
    PFNMODIFYUSER_2                pfnModifyUser;
    PFNDELETEOBJECT                pfnDeleteObject;
    PFNADDGROUP_2                  pfnAddGroup;
    PFNMODIFYGROUP_2               pfnModifyGroup;

    // PAM
    PFNOPENSESSION                 pfnOpenSession;
    PFNCLOSESESSION                pfnCloseSession;

    // NSS -- pretty good, perhaps module info level/flags?
    PFNLOOKUP_NSS_ARTEFACT_BY_KEY  pfnLookupNSSArtefactByKey;
    PFNBEGIN_ENUM_NSS_ARTEFACTS    pfnBeginEnumNSSArtefacts;
    PFNENUMNSS_ARTEFACTS           pfnEnumNSSArtefacts;
    PFNEND_ENUM_NSS_ARTEFACTS      pfnEndEnumNSSArtefacts; // can use new close enum function

    PFNGET_STATUS                  pfnGetStatus;
    PFNFREE_STATUS                 pfnFreeStatus;

    PFNREFRESH_CONFIGURATION       pfnRefreshConfiguration;
    PFNPROVIDER_IO_CONTROL         pfnProviderIoControl; // fix interface wrt uid/gid stuff
#endif

} LSA_PROVIDER_FUNCTION_TABLE_2, *PLSA_PROVIDER_FUNCTION_TABLE_2;

typedef DWORD (*PFNINITIALIZEPROVIDER_2)(
    OUT PCSTR* ppszProviderName,
    OUT PLSA_PROVIDER_FUNCTION_TABLE_2* ppFnTable
    );

typedef struct _LSA_STATIC_PROVIDER {
    PCSTR pszId;
    PFNINITIALIZEPROVIDER_2 pInitialize;
} LSA_STATIC_PROVIDER, *PLSA_STATIC_PROVIDER;

#endif /* __LSAPROVIDER_2_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
