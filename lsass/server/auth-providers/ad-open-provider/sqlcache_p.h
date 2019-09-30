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
 *        sqlcache_p.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 * 
 *        Private functions in sqlite3 Caching backend
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#ifndef __SQLCACHE_P_H__
#define __SQLCACHE_P_H__

#define LSA_DB_FREE_UNUSED_CACHEIDS   \
    "delete from " LSA_DB_TABLE_NAME_CACHE_TAGS " where CacheId NOT IN " \
        "( select CacheId from " LSA_DB_TABLE_NAME_MEMBERSHIP " ) AND " \
        "CacheId NOT IN ( select CacheId from " LSA_DB_TABLE_NAME_OBJECTS " ) AND " \
        "CacheId NOT IN ( select CacheId from " LSA_DB_TABLE_NAME_VERIFIERS " );\n"

typedef struct _LSA_DB_CONNECTION
{
    sqlite3 *pDb;
    pthread_rwlock_t lock;
    PLSA_AD_PROVIDER_STATE pProviderState;

    sqlite3_stmt *pstFindObjectByNT4;
    sqlite3_stmt *pstFindObjectByDN;
    sqlite3_stmt *pstFindObjectBySid;

    sqlite3_stmt *pstFindUserByUPN;
    sqlite3_stmt *pstFindUserByAlias;
    sqlite3_stmt *pstFindUserById;

    sqlite3_stmt *pstFindGroupByAlias;
    sqlite3_stmt *pstFindGroupById;

    sqlite3_stmt *pstRemoveObjectBySid;
    sqlite3_stmt *pstRemoveUserBySid;
    sqlite3_stmt *pstRemoveGroupBySid;

    sqlite3_stmt *pstEnumUsers;
    sqlite3_stmt *pstEnumGroups;

    sqlite3_stmt *pstGetGroupMembers;
    sqlite3_stmt *pstGetGroupsForUser;
 
    sqlite3_stmt *pstGetPasswordVerifier;

    sqlite3_stmt *pstInsertCacheTag;
    sqlite3_stmt *pstGetLastInsertedRow;
    sqlite3_stmt *pstSetLdapMembership;
    sqlite3_stmt *pstSetPrimaryGroupMembership;
    sqlite3_stmt *pstAddMembership;
} LSA_DB_CONNECTION, *PLSA_DB_CONNECTION;

typedef struct _LSA_DB_STORE_GROUP_MEMBERSHIP_CONTEXT
{
    IN PCSTR pszParentSid;
    IN size_t sMemberCount;
    IN PLSA_GROUP_MEMBERSHIP* ppMembers;
    IN PLSA_DB_CONNECTION pConn;
} LSA_DB_STORE_GROUP_MEMBERSHIP_CONTEXT, *PLSA_DB_STORE_GROUP_MEMBERSHIP_CONTEXT;

typedef struct _LSA_DB_STORE_USER_MEMBERSHIP_CONTEXT
{
    IN PCSTR pszChildSid;
    IN size_t sMemberCount;
    IN PLSA_GROUP_MEMBERSHIP* ppMembers;
    IN BOOLEAN bIsPacAuthoritative;
    IN PLSA_DB_CONNECTION pConn;
} LSA_DB_STORE_USER_MEMBERSHIP_CONTEXT, *PLSA_DB_STORE_USER_MEMBERSHIP_CONTEXT;


DWORD
LsaDbUnpackCacheInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PLSA_SECURITY_OBJECT_VERSION_INFO pResult);


DWORD
LsaDbUnpackObjectInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PLSA_SECURITY_OBJECT pResult);


DWORD
LsaDbUnpackUserInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PLSA_SECURITY_OBJECT pResult);


DWORD
LsaDbUnpackGroupInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PLSA_SECURITY_OBJECT pResult);


DWORD
LsaDbQueryObject(
    IN sqlite3_stmt* pstQuery,
    OUT PLSA_SECURITY_OBJECT* ppObject
    );


PCSTR
LsaDbGetObjectFieldList(
    VOID
    );


DWORD
LsaDbFreePreparedStatements(
    IN OUT PLSA_DB_CONNECTION pConn
    );


DWORD
LsaDbCreateCacheTag(
    IN PLSA_DB_CONNECTION pConn,
    IN time_t tLastUpdated,
    OUT int64_t *pqwCacheId
    );


DWORD
LsaDbUpdateMembership(
    IN sqlite3_stmt* pstQuery,
    IN int64_t qwCacheId,
    IN PCSTR pszParentSid,
    IN PCSTR pszChildSid
    );


DWORD
LsaDbAddMembership(
    IN PLSA_DB_CONNECTION pConn,
    IN time_t tLastUpdated,
    IN int64_t qwCacheId,
    IN PCSTR pszParentSid,
    IN PCSTR pszChildSid,
    IN BOOLEAN bIsInPac,
    IN BOOLEAN bIsInPacOnly,
    IN BOOLEAN bIsInLdap,
    IN BOOLEAN bIsDomainPrimaryGroup
    );


DWORD
LsaDbStoreGroupMembershipCallback(
    IN sqlite3 *pDb,
    IN PVOID pContext,
    OUT PSTR* ppszError
    );


DWORD
LsaDbStoreUserMembershipCallback(
    IN sqlite3 *pDb,
    IN PVOID pContext,
    OUT PSTR* ppszError
    );

void
InitializeDbCacheProvider(
    PADCACHE_PROVIDER_FUNCTION_TABLE pCacheTable
    );

#endif /* __SQLCACHE_P_H__ */
