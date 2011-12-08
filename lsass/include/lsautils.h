/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lsautils.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) system utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __LSA_UTILS_H__
#define __LSA_UTILS_H__

#include "lsaipc.h"
#include "lsalist.h"
#include <lw/ntstatus.h>
#include <lw/winerror.h>
#include <lw/security-types.h>
#include <lwdlinked-list.h>
#include <pthread.h>

#ifndef LW_ENDIAN_SWAP16

#define LW_ENDIAN_SWAP16(wX)                     \
        ((((UINT16)(wX) & 0xFF00) >> 8) |        \
         (((UINT16)(wX) & 0x00FF) << 8))

#endif

#ifndef LW_ENDIAN_SWAP32

#define LW_ENDIAN_SWAP32(dwX)                    \
        ((((UINT32)(dwX) & 0xFF000000L) >> 24) | \
         (((UINT32)(dwX) & 0x00FF0000L) >>  8) | \
         (((UINT32)(dwX) & 0x0000FF00L) <<  8) | \
         (((UINT32)(dwX) & 0x000000FFL) << 24))

#endif

#ifndef LW_ENDIAN_SWAP64

#define LW_ENDIAN_SWAP64(llX)         \
   (((UINT64)(LW_ENDIAN_SWAP32(((UINT64)(llX) & 0xFFFFFFFF00000000LL) >> 32))) | \
   (((UINT64)LW_ENDIAN_SWAP32(((UINT64)(llX) & 0x00000000FFFFFFFFLL))) << 32))

#endif

#ifndef WIN32

#ifndef BAIL_ON_NT_STATUS
#define BAIL_ON_NT_STATUS(err) \
    do { \
        if ((err) != STATUS_SUCCESS) { \
            LSA_LOG_DEBUG("Error at %s:%d [code: %X]", \
                          __FILE__, __LINE__, (err));  \
		    goto error; \
	    } \
    } while (0);
#endif

#define BAIL_ON_LSA_ERROR(dwError) \
    if (dwError) {\
       LSA_LOG_DEBUG("Error code: %u (symbol: %s)", dwError, LSA_SAFE_LOG_STRING(LwWin32ExtErrorToName(dwError))); \
       goto error;                 \
    }

#define BAIL_ON_LWMSG_ERROR(pAssoc, dwError) \
    do { \
        if (dwError) \
        { \
           (dwError) = LsaTranslateLwMsgError(pAssoc, dwError, __FILE__, __LINE__); \
           goto error; \
        } \
    } while (0)

#define BAIL_WITH_LSA_ERROR(_newerror_) \
    do {dwError = (_newerror_); BAIL_ON_LSA_ERROR(dwError);} while (0)

#endif

/*
 * Logging
 */

#define LSA_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define _LSA_LOG_AT(Level, ...) LW_RTL_LOG_AT_LEVEL(Level, "lsass", __VA_ARGS__)
#define LSA_LOG_ALWAYS(...) _LSA_LOG_AT(LW_RTL_LOG_LEVEL_ALWAYS, __VA_ARGS__)
#define LSA_LOG_ERROR(...) _LSA_LOG_AT(LW_RTL_LOG_LEVEL_ERROR, __VA_ARGS__)
#define LSA_LOG_WARNING(...) _LSA_LOG_AT(LW_RTL_LOG_LEVEL_WARNING, __VA_ARGS__)
#define LSA_LOG_INFO(...) _LSA_LOG_AT(LW_RTL_LOG_LEVEL_INFO, __VA_ARGS__)
#define LSA_LOG_VERBOSE(...) _LSA_LOG_AT(LW_RTL_LOG_LEVEL_VERBOSE, __VA_ARGS__)
#define LSA_LOG_DEBUG(...) _LSA_LOG_AT(LW_RTL_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LSA_LOG_TRACE(...) _LSA_LOG_AT(LW_RTL_LOG_LEVEL_TRACE, __VA_ARGS__)

#define LSA_LOG_ERROR_API_FAILED(hServer, dwError, szFmt, ...) \
    LSA_LOG_ERROR("Failed to " szFmt " -> error = %u, symbol = %s, client pid = %ld", \
         ## __VA_ARGS__, \
         dwError, \
         LSA_SAFE_LOG_STRING(LwWin32ExtErrorToName(dwError)), \
         (long)(hServer? ((PLSA_SRV_API_STATE)hServer)->peerPID : getpid()))

#define LSA_LOG_VERBOSE_ENTRY_NOT_FOUND(hServer, dwError, szFmt, ...) \
    LSA_LOG_VERBOSE("Failed to " szFmt " -> error = no such entry, client pid = %ld", \
         ## __VA_ARGS__, \
         (long)(hServer? ((PLSA_SRV_API_STATE)hServer)->peerPID : getpid()))

// Like assert() but also calls LSA_LOG.
#define _LSA_ASSERT(Expression, Action) \
    do { \
        if (!(Expression)) \
        { \
            LSA_LOG_DEBUG("Assertion failed: '" # Expression "'"); \
            Action; \
        } \
    } while (0)
#define _LSA_ASSERT_OR_BAIL(Expression, dwError, Action) \
    _LSA_ASSERT(Expression, \
                (dwError) = LW_ERROR_INTERNAL; \
                Action ; \
                BAIL_ON_LSA_ERROR(dwError))
#ifdef NDEBUG
#define LSA_ASSERT(Expression)
#define LSA_ASSERT_OR_BAIL(Expression, dwError) \
    _LSA_ASSERT_OR_BAIL(Expression, dwError, 0)
#else
#define LSA_ASSERT(Expression) \
    _LSA_ASSERT(Expression, abort())
#define LSA_ASSERT_OR_BAIL(Expression, dwError) \
    _LSA_ASSERT_OR_BAIL(Expression, dwError, abort())
#endif

#define LSA_IS_XOR(Expression1, Expression2) \
    (!!(Expression1) ^ !!(Expression2))

#if defined(LW_ENABLE_THREADS)

extern pthread_mutex_t gTraceLock;

#define LSA_LOCK_TRACER   pthread_mutex_lock(&gTraceLock)
#define LSA_UNLOCK_TRACER pthread_mutex_unlock(&gTraceLock)

#else

#define LSA_LOCK_TRACER
#define LSA_UNLOCK_TRACER

#endif

#define LSA_TRACE_BEGIN_FUNCTION(traceFlagArray, dwNumFlags)  \
    do {                                                      \
        LSA_LOCK_TRACER;                                      \
        if (LsaTraceIsAllowed(traceFlagArray, dwNumFlags)) {  \
            LSA_LOG_ALWAYS("Begin %s() %s:%d",                \
                           __FUNCTION__,                      \
                           __FILE__,                          \
                           __LINE__);                         \
        }                                                     \
        LSA_UNLOCK_TRACER;                                    \
    } while (0)

#define LSA_TRACE_END_FUNCTION(traceFlagArray, dwNumFlags)   \
    do {                                                     \
        LSA_LOCK_TRACER;                                     \
        if (LsaTraceIsAllowed(traceFlagArray, dwNumFlags)) { \
            LSA_LOG_ALWAYS("End %s() %s:%d",                 \
                           __FUNCTION__,                     \
                           __FILE__,                         \
                           __LINE__);                        \
        }                                                    \
        LSA_UNLOCK_TRACER;                                   \
    } while (0)

#define SERVICE_LDAP        1
#define SERVICE_KERBEROS    2

typedef struct _DNS_FQDN
{
    PSTR pszFQDN;
    PSTR pszAddress;
} DNS_FQDN, *PDNS_FQDN;

//defined flags in dwOptions
#define LSA_CFG_OPTION_STRIP_SECTION    0x00000001
#define LSA_CFG_OPTION_STRIP_NAME_VALUE_PAIR  0x00000002
#define LSA_CFG_OPTION_STRIP_ALL              (LSA_CFG_OPTION_STRIP_SECTION | \
                                              LSA_CFG_OPTION_STRIP_NAME_VALUE_PAIR)

#define INIT_SEC_BUFFER_S(_s_, _l_) do{(_s_)->length = (_s_)->maxLength = (_l_); memset((_s_)->buffer, 0, S_BUFLEN);} while (0)
#define INIT_SEC_BUFFER_S_VAL(_s_, _l_, _v_) do{(_s_)->length = (_s_)->maxLength = (_l_); memcpy((_s_)->buffer,(_v_), _l_);} while (0)
#define SEC_BUFFER_COPY(_d_,_s_) memcpy((_d_)->buffer,(_s_)->buffer,(_s_)->maxLength)
#define SEC_BUFFER_S_CONVERT(_sb_,_sbs_) do{(_sb_)->length = (_sbs_)->length;(_sb_)->maxLength=(_sbs_)->maxLength;(_sb_)->buffer = (_sbs_)->buffer;} while (0)

#define BAIL_ON_INVALID_STRING(pszParam)          \
        if (LW_IS_NULL_OR_EMPTY_STR(pszParam)) {      \
           dwError = LW_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LSA_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_HANDLE(hParam)            \
        if (hParam == (HANDLE)NULL) {             \
           dwError = LW_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LSA_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_POINTER(p)                \
        if (NULL == p) {                          \
           dwError = LW_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LSA_ERROR(dwError);            \
        }

//format of string representation of SID in SECURITYIDENTIFIER:
//S-<revision>-<authority>-<domain_computer_id>-<relative ID>
//example: S-1-5-32-546 (Guests)
//See http://support.microsoft.com/kb/243330
/*
#define WELLKNOWN_SID_DOMAIN_USER_GROUP_RID 513

typedef struct __LSA_SECURITY_IDENTIFIER {
    UCHAR* pucSidBytes;  //byte representation of multi-byte Security Identification Descriptor
    DWORD dwByteLength;
} LSA_SECURITY_IDENTIFIER, *PLSA_SECURITY_IDENTIFIER;
*/

typedef struct __LSA_BIT_VECTOR
{
    DWORD  dwNumBits;
    PDWORD pVector;
} LSA_BIT_VECTOR, *PLSA_BIT_VECTOR;

typedef DWORD (*PFN_LSA_FOREACH_STACK_ITEM)(PVOID pItem, PVOID pUserData);

typedef struct __LSA_STACK
{
    PVOID pItem;

    struct __LSA_STACK * pNext;

} LSA_STACK, *PLSA_STACK;

/* wire definition */
typedef struct _SEC_BUFFER_RELATIVE {
    USHORT length;
    USHORT maxLength;
    ULONG  offset;
} SEC_BUFFER_RELATIVE, *PSEC_BUFFER_RELATIVE;

typedef struct _OID {
    DWORD length;
    PVOID *elements;
} OID, *POID;

typedef struct _OID_SET {
    DWORD count;
    POID  elements;
} OID_SET, *POID_SET;

typedef struct _LSA_STRING_BUFFER
{
    PSTR pszBuffer;
    // length of the string excluding terminating null
    size_t sLen;
    // capacity of the buffer excluding terminating null
    size_t sCapacity;
} LSA_STRING_BUFFER;

typedef struct passwd * passwd_ptr_t;
typedef struct group  * group_ptr_t;

typedef enum
{
    LsaMetricSuccessfulAuthentications    =  0,
    LsaMetricFailedAuthentications        =  1,
    LsaMetricRootUserAuthentications      =  2,
    LsaMetricSuccessfulUserLookupsByName  =  3,
    LsaMetricFailedUserLookupsByName      =  4,
    LsaMetricSuccessfulUserLookupsById    =  5,
    LsaMetricFailedUserLookupsById        =  6,
    LsaMetricSuccessfulGroupLookupsByName =  7,
    LsaMetricFailedGroupLookupsByName     =  8,
    LsaMetricSuccessfulGroupLookupsById   =  9,
    LsaMetricFailedGroupLookupsById       = 10,
    LsaMetricSuccessfulOpenSession        = 11,
    LsaMetricFailedOpenSession            = 12,
    LsaMetricSuccessfulCloseSession       = 13,
    LsaMetricFailedCloseSession           = 14,
    LsaMetricSuccessfulChangePassword     = 15,
    LsaMetricFailedChangePassword         = 16,
    LsaMetricUnauthorizedAccesses         = 17,
    LsaMetricSentinel
} LsaMetricType;

typedef struct __LSA_NIS_NICKNAME
{
    PSTR pszMapAlias;
    PSTR pszMapName;
} LSA_NIS_NICKNAME, *PLSA_NIS_NICKNAME;

typedef DWORD (*PFLSA_CACHE_HASH) (PVOID pKey, DWORD dwIndex, PVOID pData);
typedef BOOLEAN (*PFLSA_CACHE_EQUAL) (PVOID pKey1, PVOID pKey2, DWORD dwIndex, PVOID pData);
typedef PVOID (*PFLSA_CACHE_GETKEY) (PVOID pEntry, DWORD dwIndex, PVOID pData);
typedef DWORD (*PFLSA_CACHE_MISS) (PVOID pKey, DWORD dwIndex, PVOID pData, PVOID* ppEntry);
typedef DWORD (*PFLSA_CACHE_KICK) (PVOID pEntry, PVOID pData);

typedef struct __LSA_CACHE_ENTRY
{
    DWORD dwRefCount;
} LSA_CACHE_ENTRY, *PLSA_CACHE_ENTRY;

typedef struct __LSA_CACHE
{
    DWORD dwNumKeys;
    DWORD dwNumBuckets;
    PVOID* ppEntries;
    PFLSA_CACHE_HASH pfHash;
    PFLSA_CACHE_EQUAL pfEqual;
    PFLSA_CACHE_GETKEY pfGetKey;
    PFLSA_CACHE_MISS pfMiss;
    PFLSA_CACHE_KICK pfKick;
    PVOID pData;
    DWORD dwNumHashMisses;
    DWORD dwNumFullMisses;
    DWORD dwNumKicks;
    DWORD dwNumUsedBuckets;
    DWORD dwNumCollisions;
} LSA_CACHE, *PLSA_CACHE;


#if !defined(HAVE_STRTOLL)

long long int
strtoll(
    const char* nptr,
    char**      endptr,
    int         base
    );

#endif /* defined(HAVE_STRTOLL) */

#if !defined(HAVE_STRTOULL)

unsigned long long int
strtoull(
    const char* nptr,
    char**      endptr,
    int         base
    );

#endif /* defined(HAVE_STRTOULL) */

DWORD
LsaInitializeStringBuffer(
        LSA_STRING_BUFFER *pBuffer,
        size_t sCapacity);

DWORD
LsaAppendStringBuffer(
        LSA_STRING_BUFFER *pBuffer,
        PCSTR pszAppend);

void
LsaFreeStringBufferContents(
        LSA_STRING_BUFFER *pBuffer);

DWORD
LsaAppendAndFreePtrs(
    IN OUT PDWORD pdwDestCount,
    IN OUT PVOID** pppDestPtrArray,
    IN OUT PDWORD pdwAppendCount,
    IN OUT PVOID** pppAppendPtrArray
    );

VOID
LsaPrincipalNonRealmToLower(
    IN OUT PSTR pszPrincipal
    );

VOID
LsaPrincipalRealmToUpper(
    IN OUT PSTR pszPrincipal
    );

DWORD
LsaBitVectorCreate(
    DWORD dwNumBits,
    PLSA_BIT_VECTOR* ppBitVector
    );

VOID
LsaBitVectorFree(
    PLSA_BIT_VECTOR pBitVector
    );

BOOLEAN
LsaBitVectorIsSet(
    PLSA_BIT_VECTOR pBitVector,
    DWORD           iBit
    );

DWORD
LsaBitVectorSetBit(
    PLSA_BIT_VECTOR pBitVector,
    DWORD           iBit
    );

DWORD
LsaBitVectorUnsetBit(
    PLSA_BIT_VECTOR pBitVector,
    DWORD           iBit
    );

VOID
LsaBitVectorReset(
    PLSA_BIT_VECTOR pBitVector
    );

DWORD
LsaStackPush(
    PVOID pItem,
    PLSA_STACK* ppStack
    );

PVOID
LsaStackPop(
    PLSA_STACK* ppStack
    );

PVOID
LsaStackPeek(
    PLSA_STACK pStack
    );

DWORD
LsaStackForeach(
    PLSA_STACK pStack,
    PFN_LSA_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    );

PLSA_STACK
LsaStackReverse(
    PLSA_STACK pStack
    );

VOID
LsaStackFree(
    PLSA_STACK pStack
    );

DWORD
LsaRemoveFile(
    PCSTR pszPath
    );

DWORD
LsaCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
LsaCheckSockExists(
    PSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
LsaCheckLinkExists(
    PSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
LsaCheckFileOrLinkExists(
    PSTR pszPath,
    PBOOLEAN pbExists
    );

DWORD
LsaMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
LsaChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
LsaChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
LsaChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
LsaGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
LsaChangeDirectory(
    PSTR pszPath
    );

DWORD
LsaRemoveDirectory(
    PSTR pszPath
    );

DWORD
LsaCopyDirectory(
    PCSTR pszSourceDirPath,
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszDestDirPath
    );

DWORD
LsaCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

DWORD
LsaCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
LsaGetDirectoryFromPath(
    IN PCSTR pszPath,
    OUT PSTR* ppszDir
    );

DWORD
LsaCopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    );

DWORD
LsaGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    );

DWORD
LsaCopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
LsaBackupFile(
    PCSTR pszPath
    );

DWORD
LsaGetSymlinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   );

DWORD
LsaCreateSymlink(
   PCSTR pszOldPath,
   PCSTR pszNewPath
   );

DWORD
LsaGetMatchingFilePathsInFolder(
    PCSTR pszDirPath,
    PCSTR pszFileNameRegExp,
    PSTR** pppszHostFilePaths,
    PDWORD pdwNPaths
    );

DWORD
LsaGetPrefixDirPath(
    PSTR* ppszPath
    );

DWORD
LsaGetLibDirPath(
    PSTR* ppszPath
    );

DWORD
LsaValidateGroupInfoLevel(
    DWORD dwGroupInfoLevel
    );

DWORD
LsaValidateGroupName(
    PCSTR pszGroupName
    );

DWORD
LsaValidateGroupInfo(
    PVOID pGroupInfo,
    DWORD dwGroupInfoLevel
    );

DWORD
LsaValidateUserInfo(
    PVOID pUserInfo,
    DWORD dwUserInfoLevel
    );

DWORD
LsaValidateUserInfoLevel(
    DWORD dwUserInfoLevel
    );

DWORD
LsaValidateUserName(
    PCSTR pszName
    );

DWORD
LsaAllocateGroupInfo(
    PVOID *ppDstGroupInfo,
    DWORD dwLevel,
    PVOID pSrcGroupInfo
    );

DWORD
LsaBuildUserModInfo(
    uid_t uid,
    PLSA_USER_MOD_INFO* ppUserModInfo
    );

DWORD
LsaModifyUser_EnableUser(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LsaModifyUser_DisableUser(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LsaModifyUser_Unlock(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LsaModifyUser_AddToGroups(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGroupList
    );

DWORD
LsaModifyUser_RemoveFromGroups(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGroupList
    );

DWORD
LsaModifyUser_ChangePasswordAtNextLogon(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LsaModifyUser_SetPasswordNeverExpires(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LsaModifyUser_SetPasswordMustExpire(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LsaModifyUser_SetExpiryDate(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszDate
    );

DWORD
LsaModifyUser_SetPrimaryGroup(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGid
    );

DWORD
LsaModifyUser_SetHomedir(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszHomedir
    );

DWORD
LsaModifyUser_SetShell(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszShell
    );

DWORD
LsaModifyUser_SetGecos(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGecos
    );

DWORD
LsaModifyUser_SetPassword(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszPassword
    );

DWORD
LsaModifyUser_SetNtPasswordHash(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszHash
    );

DWORD
LsaModifyUser_SetLmPasswordHash(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszHash
    );

void
LsaFreeUserModInfo(
    PLSA_USER_MOD_INFO pUserModInfo
    );

void
LsaFreeUserModInfo2(
    PLSA_USER_MOD_INFO_2 pUserModInfo
    );

void
LsaFreeUserAddInfo(
    PLSA_USER_ADD_INFO pUserAddInfo
    );

DWORD
LsaBuildGroupModInfo(
    gid_t gid,
    PLSA_GROUP_MOD_INFO *ppGroupModInfo
    );

DWORD
LsaModifyGroup_AddMembers(
    PLSA_GROUP_MOD_INFO pGroupModInfo,
    PVOID pData
    );

DWORD
LsaModifyGroup_RemoveMembers(
    PLSA_GROUP_MOD_INFO pGroupModInfo,
    PVOID pData
    );

void
LsaFreeGroupModInfo(
    PLSA_GROUP_MOD_INFO pGroupModInfo
    );

void
LsaFreeGroupModInfo2(
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo
    );

void
LsaFreeGroupAddInfo(
    PLSA_GROUP_ADD_INFO pGroupAddInfo
    );

void
LsaFreeIpcNameSidsList(
    PLSA_FIND_NAMES_BY_SIDS pNameSidsList
    );

void
LsaFreeIpcUserInfoList(
    PLSA_USER_INFO_LIST pUserIpcInfoList
    );

VOID
LsaSrvFreeIpcMetriPack(
    PLSA_METRIC_PACK pMetricPack
    );

void
LsaFreeIpcGroupInfoList(
    PLSA_GROUP_INFO_LIST pGroupIpcInfoList
    );

void
LsaFreeIpcNssArtefactInfoList(
    PLSA_NSS_ARTEFACT_INFO_LIST pNssArtefactIpcInfoList
    );

DWORD
LsaDnsGetHostInfo(
    PSTR* ppszHostname
    );

DWORD
LsaWc16sHash(
    PCWSTR  pwszStr,
    PDWORD  pdwResult
    );

DWORD
LsaStrHash(
    PCSTR   pszStr,
    PDWORD  pdwResult
    );

DWORD
LsaHashToWc16s(
    DWORD   dwHash,
    PWSTR  *ppwszHashStr
    );

DWORD
LsaHashToStr(
    DWORD   dwHash,
    PSTR   *ppszHashStr
    );

VOID
LsaDnsFreeFQDNList(
    PDNS_FQDN* ppFQDNList,
    DWORD dwNFQDN
    );

VOID
LsaDnsFreeFQDN(
    PDNS_FQDN pFQDN
    );

DWORD
LsaTraceInitialize(
    VOID
    );

BOOLEAN
LsaTraceIsFlagSet(
    DWORD dwTraceFlag
    );

BOOLEAN
LsaTraceIsAllowed(
    DWORD dwTraceFlags[],
    DWORD dwNumFlags
    );

DWORD
LsaTraceSetFlag(
    DWORD dwTraceFlag
    );

DWORD
LsaTraceUnsetFlag(
    DWORD dwTraceFlag
    );

VOID
LsaTraceShutdown(
    VOID
    );

VOID
LsaTraceShutdown(
    VOID
    );

DWORD
LsaByteArrayToHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    );

int
LsaStrError(
    int errnum,
    char *pszBuf,
    size_t buflen
    );

VOID
LsaFreeDCInfo(
    PLSA_DC_INFO pDCInfo
    );

VOID
LsaFreeDomainInfoArray(
    DWORD dwNumDomains,
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfoArray
    );

VOID
LsaFreeDomainInfo(
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfo
    );

VOID
LsaFreeDomainInfoContents(
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfo
    );

DWORD
LsaNISGetNicknames(
    PCSTR         pszNicknameFilePath,
    PLW_DLINKED_LIST* ppNicknameList
    );

PCSTR
LsaNISLookupAlias(
    PLW_DLINKED_LIST pNicknameList,
    PCSTR pszAlias
    );

VOID
LsaNISFreeNicknameList(
    PLW_DLINKED_LIST pNicknameList
    );

DWORD
LsaCacheNew(
    DWORD dwNumKeys,
    DWORD dwNumBuckets,
    PFLSA_CACHE_HASH pfHash,
    PFLSA_CACHE_EQUAL pfEqual,
    PFLSA_CACHE_GETKEY pfGetKey,
    PFLSA_CACHE_MISS pfMiss,
    PFLSA_CACHE_KICK pfKick,
    PVOID pData,
    PLSA_CACHE* ppCache
    );

DWORD
LsaCacheInsert(
    PLSA_CACHE pCache,
    PVOID pEntry
    );

DWORD
LsaCacheRemove(
    PLSA_CACHE pCache,
    PVOID pEntry
    );

DWORD
LsaCacheLookup(
    PLSA_CACHE pCache,
    PVOID pkey,
    DWORD dwIndex,
    PVOID* ppEntry
    );

DWORD
LsaCacheFlush(
    PLSA_CACHE pCache
    );

VOID
LsaCacheDelete(
    PLSA_CACHE pCache
    );

DWORD
LsaTranslateLwMsgError(
        LWMsgAssoc *pAssoc,
        DWORD dwMsgError,
        const char *file,
        int line
        );

DWORD
LsaNtStatusToLsaError(
    IN NTSTATUS Status
    );

NTSTATUS
LsaLsaErrorToNtStatus(
    IN DWORD LsaError
    );

DWORD
LsaAllocateCStringFromSid(
    OUT PSTR* ppszStringSid,
    IN PSID pSid
    );

DWORD
LsaAllocateSidFromCString(
    OUT PSID* ppSid,
    IN PCSTR pszStringSid
    );

DWORD
LsaAllocateSidAppendRid(
    OUT PSID* ppSid,
    IN PSID pDomainSid,
    IN ULONG Rid
    );

VOID
LsaUtilFreeSecurityObject(
    PLSA_SECURITY_OBJECT pObject
    );

VOID
LsaUtilFreeSecurityObjectList(
    DWORD dwCount,
    PLSA_SECURITY_OBJECT* ppObjectList
    );

DWORD
LsaUtilAllocatePamConfig(
    OUT PLSA_PAM_CONFIG *ppConfig
    );

DWORD
LsaUtilInitializePamConfig(
    OUT PLSA_PAM_CONFIG pConfig
    );

VOID
LsaUtilFreePamConfigContents(
    IN PLSA_PAM_CONFIG pConfig
    );

VOID
LsaUtilFreePamConfig(
    IN PLSA_PAM_CONFIG pConfig
    );

#endif /* __LSA_UTILS_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
