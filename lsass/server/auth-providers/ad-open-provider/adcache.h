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
 *        adcache.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Caching for AD Provider Database Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#ifndef __ADCACHE_H__
#define __ADCACHE_H__

// The bIsInLdap, bIsInPac, and bIsInPacOnly fields indicate where the
// membership came from. Only the following combinations are valid:
//
// bIsInPac                 (state A)
// bIsInPac, bIsInLdap      (state B)
// bIsInPac, bIsInPacOnly   (state C)
// bIsInLdap                (state D)
//
// At any one point a membership may be in one of those four states, or it may
// be non-existent (NX). In states B, C, and D the membership is always listed
// through nsswitch.
//
// State A indicates that a membership was visible through LDAP and the PAC,
// but it is no longer visible through LDAP. This is a strong indication that
// the membership has been removed and this is change is visible only through
// LDAP because the LDAP information is updated more frequently. However, it is
// possible that the membership existed in mutiple forms, and the membership
// more less became invisible through LDAP. Lsass's behavior can be controlled
// for state A through the trim-user-membership configuration option. If it is
// set to yes, memberships in state A are not listed.
//
// Not all state transitions are possible. Below is a list of valid transitions
// along with an explanation of how it occurs:
//
// *                         NX -> B (bIsInPac, bIsInLdap)
// This transition isn't used by the ad provider, but it is possible to ask the
// database to store a new membership that exists in ldap and the PAC.
//
// *                         NX -> C (bIsInPac, bIsInPacOnly)
// This is a new membership that is listed only in the PAC. Typically lsass
// will query LDAP for the user's membership before storing the PAC
// memberships. However, this behavior can be overwritten by setting
// nss-user-membership-query-cache-only to yes. In this case, bIsInPacOnly will
// get set to true because the LDAP information is unknown.
//
// Otherwise this state transition can occur for memberships that are not
// visible through LDAP.
//
// *                         NX -> D (bIsInLdap)
// The user's group membership was queried through LDAP because PAC information
// is not available.
//
// *               (bIsInPac) A -> NX
// An updated PAC is received that no longer contains the membership.
//
// *               (bIsInPac) A -> C (bIsInPac, bIsInPacOnly)
// A new PAC is received that reaffirms that the membership is still valid.
//
// *               (bIsInPac) A -> B (bIsInPac, bIsInLdap)
// Ldap was queried and the membership is listed there too.
//
// *    (bIsInPac, bIsInLdap) B -> A (bIsInPac)
// The membership has been found through LDAP and the PAC, but new LDAP
// information now shows it is not in LDAP.
//
// *    (bIsInPac, bIsInLdap) B -> NX
// An updated PAC is received that no longer contains the membership.
//
// * (bIsInPac, bIsInPacOnly) C -> B (bIsInPac, bIsInLdap)
// Ldap was queried and the membership is listed there too.
//
// * (bIsInPac, bIsInPacOnly) C -> NX
// An updated PAC is received that no longer contains the membership.
//
// *              (bIsInLdap) D -> B (bIsInPac, bIsInLdap)
// A PAC was received and the membership is in there too.
//
// *              (bIsInLdap) D -> NX
// The membership was found through LDAP (pac was never received), but it is no
// longer in LDAP.
typedef struct __LSA_GROUP_MEMBERSHIP
{
    LSA_SECURITY_OBJECT_VERSION_INFO version;
    PSTR    pszParentSid;
    PSTR    pszChildSid;
    BOOLEAN bIsInPac;
    BOOLEAN bIsInPacOnly;
    BOOLEAN bIsInLdap;
    BOOLEAN bIsDomainPrimaryGroup;
} LSA_GROUP_MEMBERSHIP, *PLSA_GROUP_MEMBERSHIP;

typedef struct __LSA_DB_PASSWORD_VERIFIER
{
    LSA_SECURITY_OBJECT_VERSION_INFO version;
    PSTR    pszObjectSid;
    // sequence of hexadecimal digits
    PSTR    pszPasswordVerifier;
} LSA_PASSWORD_VERIFIER, *PLSA_PASSWORD_VERIFIER;

DWORD
ADCacheOpen(
    IN PCSTR pszDbPath,
    IN PLSA_AD_PROVIDER_STATE pState,
    PLSA_DB_HANDLE phDb
    );

// Sets the handle to null after closing it. If a null handle is passed in,
// it is ignored.
void
ADCacheSafeClose(
    PLSA_DB_HANDLE phDb
    );

DWORD
ADCacheFlushToDisk(
    IN LSA_DB_HANDLE hDb
    );

// returns LW_ERROR_NOT_HANDLED if the user is not in the database
DWORD
ADCacheFindUserByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    );

// returns LW_ERROR_NOT_HANDLED if the user is not in the database
DWORD
ADCacheFindUserById(
    LSA_DB_HANDLE hDb,
    uid_t uid,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
ADCacheFindGroupByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
ADCacheFindGroupById(
    LSA_DB_HANDLE hDb,
    uid_t uid,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
ADCacheRemoveUserBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    );

DWORD
ADCacheRemoveGroupBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    );

DWORD
ADCacheEmptyCache(
    IN LSA_DB_HANDLE hDb
    );

DWORD
ADCacheStoreObjectEntry(
    LSA_DB_HANDLE hDb,
    PLSA_SECURITY_OBJECT pObject
    );

DWORD
ADCacheStoreObjectEntries(
    LSA_DB_HANDLE hDb,
    size_t  sObjectCount,
    PLSA_SECURITY_OBJECT* ppObjects
    );

void
ADCacheSafeFreeObject(
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
ADCacheDuplicateObject(
    OUT PLSA_SECURITY_OBJECT* ppDest,
    IN PLSA_SECURITY_OBJECT pSrc
    );

DWORD
ADCacheDuplicateMembership(
    PLSA_GROUP_MEMBERSHIP* ppDest,
    PLSA_GROUP_MEMBERSHIP pSrc
    );

DWORD
ADCacheDuplicateMembershipContents(
    PLSA_GROUP_MEMBERSHIP pDest,
    PLSA_GROUP_MEMBERSHIP pSrc
    );

DWORD
ADCacheDuplicatePasswordVerifier(
    PLSA_PASSWORD_VERIFIER* ppDest,
    PLSA_PASSWORD_VERIFIER pSrc
    );

void
ADCacheSafeFreeObjectList(
        size_t sCount,
        PLSA_SECURITY_OBJECT** pppObjectList);

#define LSA_DB_SAFE_FREE_PASSWORD_VERIFIER(x) \
    if ((x) != NULL) { \
        ADCacheFreePasswordVerifier(x); \
        (x) = NULL; \
    }

void
ADCacheFreePasswordVerifier(
    IN OUT PLSA_PASSWORD_VERIFIER pVerifier
    );

DWORD
ADCacheStoreGroupMembership(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszParentSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers
    );

DWORD
ADCacheStoreGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszChildSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers,
    IN BOOLEAN bIsPacAuthoritative
    );

DWORD
ADCacheGetGroupMembers(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    );

DWORD
ADCacheGetGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    );

void
ADCacheSafeFreeGroupMembership(
        PLSA_GROUP_MEMBERSHIP* ppMembership);

void
ADCacheFreeGroupMembershipContents(
        PLSA_GROUP_MEMBERSHIP pMembership);

void
ADCacheSafeFreeGroupMembershipList(
        size_t sCount,
        PLSA_GROUP_MEMBERSHIP** pppMembershipList);

DWORD
ADCacheFindObjectsByDNList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszDnList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    );

DWORD
ADCacheFindObjectsBySidList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    );

DWORD
ADCacheFindObjectByDN(
    LSA_DB_HANDLE hDb,
    PCSTR pszDN,
    PLSA_SECURITY_OBJECT *ppObject);

DWORD
ADCacheFindObjectBySid(
    LSA_DB_HANDLE hDb,
    PCSTR pszSid,
    PLSA_SECURITY_OBJECT *ppObject);

DWORD
ADCacheEnumUsersCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumUsers,
    IN PCSTR                   pszResume,
    OUT DWORD*                 pdwNumUsersFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
ADCacheEnumGroupsCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumGroups,
    IN PCSTR                   pszResume,
    OUT DWORD *                pdwNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

// returns LW_ERROR_NOT_HANDLED if the user is not in the database
DWORD
ADCacheGetPasswordVerifier(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszUserSid,
    OUT PLSA_PASSWORD_VERIFIER *ppResult
    );

DWORD
ADCacheStorePasswordVerifier(
    IN LSA_DB_HANDLE hDb,
    IN PLSA_PASSWORD_VERIFIER pVerifier
    );

#endif /* __ADCACHE_H__ */
