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

#ifndef __LSANSS_H__
#define __LSANSS_H__

#include "config.h"

#ifdef HAVE_NSS_COMMON_H

#include <nss_common.h>
#include <nss_dbdefs.h>

typedef nss_status_t NSS_STATUS;
#define NSS_STATUS_SUCCESS NSS_SUCCESS
#define NSS_STATUS_NOTFOUND NSS_NOTFOUND
#define NSS_STATUS_UNAVAIL NSS_UNAVAIL
#define NSS_STATUS_TRYAGAIN NSS_TRYAGAIN
#define NSS_STATUS_RETURN NSS_RETURN

#elif HAVE_NSS_H

#include <nss.h>

typedef enum nss_status NSS_STATUS;

#elif defined(__LWI_HP_UX__)

#include <nsswitch.h>
#include <nss_dbdefs_hpux.h>

typedef nss_status_t NSS_STATUS;
#define NSS_STATUS_SUCCESS NSS_SUCCESS
#define NSS_STATUS_NOTFOUND NSS_NOTFOUND
#define NSS_STATUS_UNAVAIL NSS_UNAVAIL
#define NSS_STATUS_TRYAGAIN NSS_TRYAGAIN
#define NSS_STATUS_RETURN NSS_RETURN

#else

enum nss_status
{
  NSS_STATUS_TRYAGAIN = -2,
  NSS_STATUS_UNAVAIL,
  NSS_STATUS_NOTFOUND,
  NSS_STATUS_SUCCESS,
  NSS_STATUS_RETURN
};

typedef enum nss_status NSS_STATUS;

#endif

#define DISABLE_NSS_ENUMERATION_ENV "_DISABLE_LSASS_NSS_ENUMERATION"

#define BAIL_ON_NSS_ERROR(errCode)                      \
    do {                                                \
        if (NSS_STATUS_SUCCESS != (errCode))            \
            goto error;                                 \
    } while(0);

typedef enum
{
    LSA_NSS_NETGROUP_ENTRY_TRIPLE,
    LSA_NSS_NETGROUP_ENTRY_GROUP,
    LSA_NSS_NETGROUP_ENTRY_END
} LSA_NSS_NETGROUP_ENTRY_TYPE;

#include <pwd.h>
#include <grp.h>

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

#include "lsasystem.h"
#include "lsa/lsa.h"
#include "lsadef.h"
#include "lsaclient.h"
#include "nss-error.h"
#include "lsaauth.h"

typedef struct __LSA_ENUMGROUPS_STATE
{
    HANDLE  hResume;
    PVOID*  ppGroupInfoList;
    DWORD   dwNumGroups;
    DWORD   dwGroupInfoLevel;
    DWORD   idxGroup;
    BOOLEAN bTryAgain;
} LSA_ENUMGROUPS_STATE, *PLSA_ENUMGROUPS_STATE;

typedef struct __LSA_ENUMUSERS_STATE
{
    HANDLE  hResume;
    PVOID*  ppUserInfoList;
    DWORD   dwNumUsers;
    DWORD   dwUserInfoLevel;
    DWORD   idxUser;
    BOOLEAN bTryAgain;
} LSA_ENUMUSERS_STATE, *PLSA_ENUMUSERS_STATE;

typedef struct __LSA_ENUMARTEFACTS_STATE
{
    HANDLE  hResume;
    PVOID*  ppArtefactInfoList;
    DWORD   dwNumArtefacts;
    DWORD   dwArtefactInfoLevel;
    DWORD   idxArtefact;
    BOOLEAN bTryAgain;
} LSA_ENUMARTEFACTS_STATE, *PLSA_ENUMARTEFACTS_STATE;

typedef struct __LSA_NSS_CACHED_HANDLE
{
    HANDLE hLsaConnection;
    pid_t owner;
} LSA_NSS_CACHED_HANDLE, *PLSA_NSS_CACHED_HANDLE;

DWORD
LsaNssCommonEnsureConnected(
    PLSA_NSS_CACHED_HANDLE pConnection
    );

DWORD
LsaNssCommonCloseConnection(
    PLSA_NSS_CACHED_HANDLE pConnection
    );

VOID
LsaNssClearEnumUsersState(
    HANDLE hLsaConnection,
    PLSA_ENUMUSERS_STATE pState
    );

DWORD
LsaNssComputeUserStringLength(
    PLSA_USER_INFO_0 pUserInfo
    );

DWORD
LsaNssWriteUserInfo(
    DWORD        dwUserInfoLevel,
    PVOID        pUserInfo,
    passwd_ptr_t pResultUser,
    char**       ppszBuf,
    int          bufLen
    );

#ifdef HAVE_SHADOW_H
DWORD
LsaNssWriteShadowInfo(
    DWORD        dwUserInfoLevel,
    PVOID        pUserInfo,
    spwd_ptr_t   pResultUser,
    char**       ppszBuf,
    int          bufLen
    );
#endif

VOID
LsaNssClearEnumGroupsState(
    HANDLE hLsaConnection,
    PLSA_ENUMGROUPS_STATE pState
    );


DWORD
LsaNssGetNumberGroupMembers(
    PSTR* ppszMembers
    );

DWORD
LsaNssComputeGroupStringLength(
    DWORD dwAlignBytes,
    PLSA_GROUP_INFO_1 pGroupInfo
    );

DWORD
LsaNssWriteGroupInfo(
    DWORD       dwGroupInfoLevel,
    PVOID       pGroupInfo,
    group_ptr_t pResultGroup,
    char**      ppszBuf,
    int         bufLen
    );

VOID
LsaNssClearEnumArtefactsState(
    HANDLE hLsaConnection,
    PLSA_ENUMARTEFACTS_STATE pState
    );

NSS_STATUS
LsaNssCommonPasswdSetpwent(
    PLSA_NSS_CACHED_HANDLE pConnection,
    PLSA_ENUMUSERS_STATE pEnumUsersState
    );

NSS_STATUS
LsaNssCommonPasswdGetpwent(
    PLSA_NSS_CACHED_HANDLE  pConnection,
    PLSA_ENUMUSERS_STATE    pEnumUsersState,
    struct passwd *         pResultUser,
    char*                   pszBuf,
    size_t                  bufLen,
    int*                    pErrorNumber
    );

NSS_STATUS
LsaNssCommonPasswdEndpwent(
    PLSA_NSS_CACHED_HANDLE  pConnection,
    PLSA_ENUMUSERS_STATE    pEnumUsersState
    );

NSS_STATUS
LsaNssCommonPasswdGetpwnam(
    PLSA_NSS_CACHED_HANDLE pConnection,
    const char * pszLoginId,
    struct passwd * pResultUser,
    char * pszBuf,
    size_t bufLen,
    int * pErrorNumber
    );

NSS_STATUS
LsaNssCommonPasswdGetpwuid(
    PLSA_NSS_CACHED_HANDLE pConnection,
    uid_t uid,
    struct passwd * pResultUser,
    char * pszBuf,
    size_t bufLen,
    int * pErrorNumber
    );

#ifdef HAVE_SHADOW_H
NSS_STATUS
LsaNssCommonShadowGetspent(
    PLSA_NSS_CACHED_HANDLE  pConnection,
    PLSA_ENUMUSERS_STATE    pEnumUsersState,
    struct spwd *           pResultUser,
    char*                   pszBuf,
    size_t                  bufLen,
    int*                    pErrorNumber
    );

NSS_STATUS
LsaNssCommonShadowGetspnam(
    PLSA_NSS_CACHED_HANDLE pConnection,
    const char * pszLoginId,
    struct spwd * pResultUser,
    char * pszBuf,
    size_t bufLen,
    int * pErrorNumber
    );
#endif


NSS_STATUS
LsaNssCommonGroupSetgrent(
    PLSA_NSS_CACHED_HANDLE pConnection,
    PLSA_ENUMGROUPS_STATE pEnumGroupsState
    );

NSS_STATUS
LsaNssCommonGroupGetgrent(
    PLSA_NSS_CACHED_HANDLE    pConnection,
    PLSA_ENUMGROUPS_STATE     pEnumGroupsState,
    struct group*             pResultGroup,
    char *                    pszBuf,
    size_t                    bufLen,
    int*                      pErrorNumber
    );

NSS_STATUS
LsaNssCommonGroupEndgrent(
    PLSA_NSS_CACHED_HANDLE    pConnection,
    PLSA_ENUMGROUPS_STATE     pEnumGroupsState
    );

NSS_STATUS
LsaNssCommonGroupGetgrgid(
    PLSA_NSS_CACHED_HANDLE pConnection,
    gid_t gid,
    struct group* pResultGroup,
    char* pszBuf,
    size_t bufLen,
    int* pErrorNumber
    );

NSS_STATUS
LsaNssCommonGroupGetgrnam(
    PLSA_NSS_CACHED_HANDLE pConnection,
    const char * pszGroupName,
    struct group * pResultGroup,
    char * pszBuf,
    size_t bufLen,
    int* pErrorNumber
    );

NSS_STATUS
LsaNssCommonGroupGetGroupsByUserName(
    PLSA_NSS_CACHED_HANDLE pConnection,
    PCSTR pszUserName,
    size_t resultsExistingSize,
    size_t resultsCapacity,
    size_t* pResultSize,
    gid_t* pGidResults,
    int* pErrorNumber
    );

NSS_STATUS
LsaNssCommonNetgroupFindByName(
    PLSA_NSS_CACHED_HANDLE pConnection,
    PCSTR pszName,
    PSTR* ppszValue
    );

NSS_STATUS
LsaNssCommonNetgroupParse(
    PSTR* ppszCursor,
    LSA_NSS_NETGROUP_ENTRY_TYPE* pType,
    PSTR* ppszHost,
    PSTR* ppszUser,
    PSTR* ppszDomain,
    PSTR* ppszGroup
    );

#endif
