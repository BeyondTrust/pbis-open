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
 *        lsasrvapi2.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
    IN OPTIONAL PCSTR pszSmartcardUser,
    OUT PLSA_SECURITY_OBJECT* ppObject,
    OUT PSTR* ppszSmartCardReader
    );


#endif
