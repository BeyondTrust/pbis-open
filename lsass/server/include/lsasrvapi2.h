/*
 * Copyright Likewise Software
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
 *        lsasrvapi2.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Server API (version 2)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 */

#ifndef __LSASRVAPI2_H__
#define __LSASRVAPI2_H__

#include "lsasrvapi.h"

DWORD
LsaSrvFindObjects(
    IN HANDLE hServer,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
LsaSrvOpenEnumObjects(
    IN HANDLE hServer,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    );

DWORD
LsaSrvEnumObjects(
    IN HANDLE hServer,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
LsaSrvOpenEnumMembers(
    IN HANDLE hServer,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    );

DWORD
LsaSrvEnumMembers(
    IN HANDLE hServer,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PSTR** pppszMember
    );

DWORD
LsaSrvQueryMemberOf(
    IN HANDLE hServer,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    );

LW_DWORD
LsaSrvFindGroupAndExpandedMembers(
    LW_IN LW_HANDLE hServer,
    LW_PCSTR pszTargetProvider,
    LW_IN LSA_FIND_FLAGS FindFlags,
    LW_IN LSA_QUERY_TYPE QueryType,
    LW_IN LSA_QUERY_ITEM QueryItem,
    LW_OUT PLSA_SECURITY_OBJECT* ppGroupObject,
    LW_OUT LW_PDWORD pdwMemberObjectCount,
    LW_OUT PLSA_SECURITY_OBJECT** pppMemberObjects
    );

VOID
LsaSrvCloseEnum(
    IN HANDLE hServer,
    IN OUT HANDLE hEnum
    );

DWORD
LsaSrvGetSmartCardUserObject(
    IN HANDLE hServer,
    OUT PLSA_SECURITY_OBJECT* ppObject,
    OUT PSTR* ppszSmartCardReader
    );


#endif
