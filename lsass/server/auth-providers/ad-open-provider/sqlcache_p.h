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
 *        sqlcache_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
