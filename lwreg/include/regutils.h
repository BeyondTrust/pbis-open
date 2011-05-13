/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        regutils.h
 *
 * Abstract:
 *
 *        Registry system utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#ifndef __REG_UTILS_H__
#define __REG_UTILS_H__

#ifndef WIN32

/* Special check for parsing error codes */

#define BAIL_ON_REG_PARSE_ERROR(dwError)              \
	if ((dwError != LW_ERROR_SUCCESS) &&             \
	    (dwError != REG_ERROR_INSUFFICIENT_BUFFER)) { \
		REG_LOG_DEBUG("Error: %d", dwError);       \
		goto error;                                   \
	}


#ifndef BAIL_ON_NT_STATUS
#define BAIL_ON_NT_STATUS(status)                \
    if ((status) != STATUS_SUCCESS) {                              \
        REG_LOG_DEBUG("Status: %s = 0x%08X (%d)]", \
        RegNtStatusToName(status), \
        status, status);          \
        goto error;                                 \
    }
#endif

#define BAIL_ON_REG_ERROR(dwError)                                              \
    if (dwError) {                                                              \
       REG_LOG_DEBUG("Error: %d", dwError); \
       goto error;                                                              \
    }

#define BAIL_ON_LWMSG_ERROR(pAssoc, dwError)                                         \
    do {                                                                             \
        if (dwError)                                                                 \
        {                                                                            \
           (dwError) = RegTranslateLwMsgError(pAssoc, dwError, __FILE__, __LINE__); \
           goto error;                                                               \
        }                                                                            \
    } while (0)

#define BAIL_ON_INVALID_RESERVED_VALUE(dwReserved)      \
        if (0 != dwReserved) {                          \
           status = STATUS_INVALID_PARAMETER; \
           BAIL_ON_NT_STATUS(status);            \
        }

#define BAIL_ON_INVALID_RESERVED_POINTER(pdwReserved)      \
        if (NULL != pdwReserved) {                          \
           status = STATUS_INVALID_PARAMETER; \
           BAIL_ON_NT_STATUS(status);            \
        }

#define BAIL_ON_INVALID_STRING(pszParam)          \
        if (IsNullOrEmptyString(pszParam)) {      \
           dwError = ERROR_INVALID_PARAMETER; \
           BAIL_ON_REG_ERROR(dwError);            \
        }

#define BAIL_ON_NT_INVALID_STRING(pszParam)          \
        if (IsNullOrEmptyString(pszParam)) {      \
           status = STATUS_INVALID_PARAMETER; \
           BAIL_ON_NT_STATUS(status);            \
        }

#define BAIL_ON_INVALID_HANDLE(hParam)            \
        if (hParam == (HANDLE)NULL) {             \
           dwError = ERROR_INVALID_PARAMETER; \
           BAIL_ON_REG_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_POINTER(p)                \
        if (NULL == p) {                          \
           dwError = ERROR_INVALID_PARAMETER; \
           BAIL_ON_REG_ERROR(dwError);            \
        }

#define BAIL_ON_NT_INVALID_POINTER(p)                \
        if (NULL == p) {                          \
           status = STATUS_INVALID_PARAMETER; \
           BAIL_ON_NT_STATUS(status);            \
        }

#define BAIL_ON_INVALID_KEY_CONTEXT(pKey)                 \
        BAIL_ON_NT_INVALID_POINTER(pKey);            \
        if (LW_IS_NULL_OR_EMPTY_STR(pKey->pwszKeyName)) \
        { \
            status = STATUS_INVALID_PARAMETER; \
            BAIL_ON_NT_STATUS(status); \
        }

#define BAIL_ON_INVALID_REG_ENTRY(pRegEntry)                 \
        BAIL_ON_NT_INVALID_POINTER(pRegEntry);            \
        if (LW_IS_NULL_OR_EMPTY_STR(pRegEntry->pwszKeyName)) \
        { \
            status = STATUS_INVALID_PARAMETER; \
            BAIL_ON_NT_STATUS(status); \
        }

#define LWREG_SAFE_FREE_MEMORY(mem) \
    do { \
        if (mem) \
        { \
            RegMemoryFree(mem); \
            (mem) = NULL; \
        } \
    } while (0)

#define LWREG_SAFE_FREE_STRING(str) \
    do { \
        if (str) \
        { \
            RegFreeString(str); \
            (str) = NULL; \
        } \
    } while (0)

#define LW_IS_NULL_OR_EMPTY_STR(str) (!(str) || !(*(str)))

typedef struct __REG_BIT_VECTOR
{
    DWORD  dwNumBits;
    PDWORD pVector;
} REG_BIT_VECTOR, *PREG_BIT_VECTOR;

typedef struct __REG_HASH_ENTRY REG_HASH_ENTRY;

typedef int (*REG_HASH_KEY_COMPARE)(PCVOID, PCVOID);
typedef size_t (*REG_HASH_KEY)(PCVOID);
typedef void (*REG_HASH_FREE_ENTRY)(const REG_HASH_ENTRY *);
typedef DWORD (*REG_HASH_COPY_ENTRY)(const REG_HASH_ENTRY *, REG_HASH_ENTRY *);

struct __REG_HASH_ENTRY
{
    PVOID pKey;
    PVOID pValue;
    //Next value in chain
    struct __REG_HASH_ENTRY *pNext;
};

typedef struct __REG_HASH_TABLE
{
    size_t sTableSize;
    size_t sCount;
    REG_HASH_ENTRY **ppEntries;
    REG_HASH_KEY_COMPARE fnComparator;
    REG_HASH_KEY fnHash;
    REG_HASH_FREE_ENTRY fnFree;
    REG_HASH_COPY_ENTRY fnCopy;
} REG_HASH_TABLE, *PREG_HASH_TABLE;

typedef struct __REG_HASH_ITERATOR
{
    REG_HASH_TABLE *pTable;
    size_t sEntryIndex;
    REG_HASH_ENTRY *pEntryPos;
} REG_HASH_ITERATOR;

#define _LW_REG_ASSERT(Expression, Action)                            \
    do {                                                           \
        if (!(Expression))                                         \
        {                                                          \
            REG_LOG_DEBUG("Assertion failed: '" # Expression "'"); \
            Action;                                                \
        }                                                          \
    } while (0)

#define _LW_REG_ASSERT_OR_BAIL(Expression, dwError, Action) \
    _LW_REG_ASSERT(Expression,                              \
                (dwError) = ERROR_INTERNAL_ERROR;          \
                Action ;                                 \
                BAIL_ON_REG_ERROR(dwError))

/*
 * Logging
 */

typedef enum
{
    REG_LOG_LEVEL_ALWAYS = LW_RTL_LOG_LEVEL_ALWAYS,
    REG_LOG_LEVEL_ERROR = LW_RTL_LOG_LEVEL_ERROR,
    REG_LOG_LEVEL_WARNING = LW_RTL_LOG_LEVEL_WARNING,
    REG_LOG_LEVEL_INFO = LW_RTL_LOG_LEVEL_INFO,
    REG_LOG_LEVEL_VERBOSE = LW_RTL_LOG_LEVEL_VERBOSE,
    REG_LOG_LEVEL_DEBUG = LW_RTL_LOG_LEVEL_DEBUG,
    REG_LOG_LEVEL_TRACE = LW_RTL_LOG_LEVEL_TRACE
} RegLogLevel;

#define REG_SAFE_LOG_STRING(x) ( (x) ? (x) : "<null>" )

#define _REG_LOG_AT(Level, ...) LW_RTL_LOG_AT_LEVEL(Level, "lwreg", __VA_ARGS__)
#define REG_LOG_ALWAYS(...) _REG_LOG_AT(LW_RTL_LOG_LEVEL_ALWAYS, __VA_ARGS__)
#define REG_LOG_ERROR(...) _REG_LOG_AT(LW_RTL_LOG_LEVEL_ERROR, __VA_ARGS__)
#define REG_LOG_WARNING(...) _REG_LOG_AT(LW_RTL_LOG_LEVEL_WARNING, __VA_ARGS__)
#define REG_LOG_INFO(...) _REG_LOG_AT(LW_RTL_LOG_LEVEL_INFO, __VA_ARGS__)
#define REG_LOG_VERBOSE(...) _REG_LOG_AT(LW_RTL_LOG_LEVEL_VERBOSE, __VA_ARGS__)
#define REG_LOG_DEBUG(...) _REG_LOG_AT(LW_RTL_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define REG_LOG_TRACE(...) _REG_LOG_AT(LW_RTL_LOG_LEVEL_TRACE, __VA_ARGS__)

extern pthread_mutex_t gTraceLock;

#define REG_LOCK_TRACER   pthread_mutex_lock(&gTraceLock)
#define REG_UNLOCK_TRACER pthread_mutex_unlock(&gTraceLock)

#define IsNullOrEmptyString(str) (!(str) || !(*(str)))

#ifdef NDEBUG
#define LW_REG_ASSERT(Expression)
#define LW_REG_ASSERT_OR_BAIL(Expression, dwError) \
    _LW_REG_ASSERT_OR_BAIL(Expression, dwError, 0)
#else
#define LW_REG_ASSERT(Expression) \
    _LW_REG_ASSERT(Expression, abort())
#define LW_REG_ASSERT_OR_BAIL(Expression, dwError)       \
    _LW_REG_ASSERT_OR_BAIL(Expression, dwError, abort())
#endif

#endif //WIN32

typedef struct __DLINKEDLIST
{
    PVOID pItem;

    struct __DLINKEDLIST * pNext;

    struct __DLINKEDLIST * pPrev;

} DLINKEDLIST, *PDLINKEDLIST;

typedef VOID (*PFN_DLINKEDLIST_FUNC)(PVOID pData, PVOID pUserData);

typedef DWORD (*PFN_REG_FOREACH_STACK_ITEM)(PVOID pItem, PVOID pUserData);

typedef struct __LWREG_STACK
{
    PVOID pItem;

    struct __LWREG_STACK * pNext;

} LWREG_STACK, *PLWREG_STACK;

//defined flags in dwOptions
#define REG_CFG_OPTION_STRIP_SECTION          0x00000001
#define REG_CFG_OPTION_STRIP_NAME_VALUE_PAIR  0x00000002
#define REG_CFG_OPTION_STRIP_ALL (REG_CFG_OPTION_STRIP_SECTION |      \
                                     REG_CFG_OPTION_STRIP_NAME_VALUE_PAIR)

typedef struct _REG_STRING_BUFFER
{
    PSTR pszBuffer;
    // length of the string excluding terminating null
    size_t sLen;
    // capacity of the buffer excluding terminating null
    size_t sCapacity;
} REG_STRING_BUFFER;

/*
 * Config parsing callbacks
 */
typedef DWORD (*PFNREG_CONFIG_START_SECTION)(
                        PCSTR    pszSectionName,
                        PVOID    pData,
                        PBOOLEAN pbSkipSection,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNREG_CONFIG_COMMENT)(
                        PCSTR    pszComment,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNREG_CONFIG_NAME_VALUE_PAIR)(
                        PCSTR    pszName,
                        PCSTR    pszValue,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNREG_CONFIG_END_SECTION)(
                        PCSTR pszSectionName,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

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
RegGetCurrentTimeSeconds(
    OUT time_t* pTime
    );

DWORD
RegRemoveFile(
    PCSTR pszPath
    );

DWORD
RegCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
RegCheckSockExists(
    PSTR pszPath,
    PBOOLEAN pbSockExists
    );

DWORD
RegCheckLinkExists(
    PSTR pszPath,
    PBOOLEAN pbLinkExists
    );

DWORD
RegCheckFileOrLinkExists(
    PSTR pszPath,
    PBOOLEAN pbExists
    );

DWORD
RegMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
RegChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
RegChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
RegChangeDirectory(
    PSTR pszPath
    );

DWORD
RegRemoveDirectory(
    PSTR pszPath
    );

DWORD
RegCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

DWORD
RegGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
RegCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
RegGetDirectoryFromPath(
    IN PCSTR pszPath,
    OUT PSTR* ppszDir
    );

DWORD
RegGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    );

DWORD
RegCopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    );

DWORD
RegCopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
RegChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
RegBackupFile(
    PCSTR pszPath
    );

DWORD
RegGetSymlinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   );

DWORD
RegCreateSymlink(
   PCSTR pszOldPath,
   PCSTR pszNewPath
   );

DWORD
RegCopyDirectory(
    PCSTR pszSourceDirPath,
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszDestDirPath
    );

DWORD
RegGetMatchingFilePathsInFolder(
    PCSTR pszDirPath,
    PCSTR pszFileNameRegExp,
    PSTR** pppszHostFilePaths,
    PDWORD pdwNPaths
    );

DWORD
RegShutdownLogging(
    VOID
    );

DWORD
RegDLinkedList(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

DWORD
RegDLinkedListAppend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

DWORD
RegDLinkedListPrepend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

BOOLEAN
RegDLinkedListDelete(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

VOID
RegDLinkedListForEach(
    PDLINKEDLIST          pList,
    PFN_DLINKEDLIST_FUNC pFunc,
    PVOID                pUserData
    );

VOID
RegDLinkedListFree(
    PDLINKEDLIST pList
    );

NTSTATUS
RegHashCreate(
    size_t sTableSize,
    REG_HASH_KEY_COMPARE fnComparator,
    REG_HASH_KEY fnHash,
    REG_HASH_FREE_ENTRY fnFree, //optional
    REG_HASH_COPY_ENTRY fnCopy, //optional
    REG_HASH_TABLE** ppResult
    );

void
RegHashRemoveAll(
    REG_HASH_TABLE* pResult
    );

void
RegHashSafeFree(
    REG_HASH_TABLE** ppResult
    );

NTSTATUS
RegHashSetValue(
    REG_HASH_TABLE *pTable,
    PVOID  pKey,
    PVOID  pValue
    );

//Returns ENOENT if pKey is not in the table
NTSTATUS
RegHashGetValue(
    REG_HASH_TABLE *pTable,
    PCVOID  pKey,
    PVOID* ppValue
    );

BOOLEAN
RegHashExists(
    IN PREG_HASH_TABLE pTable,
    IN PCVOID pKey
    );

NTSTATUS
RegHashCopy(
    IN  REG_HASH_TABLE *pTable,
    OUT REG_HASH_TABLE **ppResult
    );

//Invalidates all iterators
NTSTATUS
RegHashResize(
    REG_HASH_TABLE *pTable,
    size_t sTableSize
    );

VOID
RegHashGetIterator(
    REG_HASH_TABLE *pTable,
    REG_HASH_ITERATOR *pIterator
    );

// returns NULL after passing the last entry
REG_HASH_ENTRY *
RegHashNext(
    REG_HASH_ITERATOR *pIterator
    );

NTSTATUS
RegHashRemoveKey(
    REG_HASH_TABLE *pTable,
    PVOID  pKey
    );

int
RegHashCaselessWC16StringCompare(
    PCVOID str1,
    PCVOID str2
    );

size_t
RegHashCaselessStringHash(
    PCVOID str
    );

size_t
RegHashCaselessWc16String(
    PCVOID str
    );

int
RegHashPVoidCompare(
    IN PCVOID pvData1,
    IN PCVOID pvData2
    );

size_t
RegHashPVoidHash(
    IN PCVOID pvData
    );

VOID
RegHashFreeWc16StringKey(
    IN OUT const REG_HASH_ENTRY *pEntry
    );

NTSTATUS
RegInitializeStringBuffer(
    REG_STRING_BUFFER *pBuffer,
    size_t sCapacity
    );

NTSTATUS
RegAppendStringBuffer(
    REG_STRING_BUFFER *pBuffer,
    PCSTR pszAppend
    );

void
RegFreeStringBufferContents(
    REG_STRING_BUFFER *pBuffer
    );

VOID
RegPrintError(
    IN OPTIONAL PCSTR pszErrorPrefix,
    IN DWORD dwError
    );

DWORD
RegMapErrnoToLwRegError(
    DWORD dwErrno
    );

DWORD
RegNtStatusToWin32Error(
    NTSTATUS ntStatus
    );

PCSTR
RegNtStatusToName(
    IN NTSTATUS status
    );

DWORD
RegReallocMemory(
    IN PVOID pMemory,
    OUT PVOID* ppNewMemory,
    IN DWORD dwSize
    );

NTSTATUS
NtRegReallocMemory(
    IN PVOID pMemory,
    OUT PVOID* ppNewMemory,
    IN DWORD dwSize
    );

void
RegSafeFreeValueAttributes(
    PLWREG_VALUE_ATTRIBUTES* ppValueAttrs
    );

void
RegSafeFreeCurrentValueInfo(
    PLWREG_CURRENT_VALUEINFO* ppValueInfo
    );

void
RegFreeString(
    PSTR pszString
    );

void
RegFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    );

void
RegFreeWC16StringArray(
    PWSTR * ppwStringArray,
    DWORD dwCount
    );

void
RegFreeWC16StringArrayWithNullTerminator(
    PWSTR * ppwStringArray
    );

void
RegFreeValueByteArray(
    PBYTE* ppValues,
    DWORD dwCount
    );

DWORD
RegStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
    );

NTSTATUS
RegHexStrToByteArray(
    IN PCSTR pszHexString,
    IN OPTIONAL DWORD* pdwHexStringLength,
    OUT UCHAR** ppucByteArray,
    OUT DWORD*  pdwByteArrayLength
    );

NTSTATUS
RegByteArrayToHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    );

NTSTATUS
RegStrDupOrNull(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    );

NTSTATUS
RegWcStrDupOrNull(
    PCWSTR pwszInputString,
    PWSTR *ppwszOutputString
    );

void
RegStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    );

DWORD
RegAllocateMemory(
    size_t Size,
    LW_PVOID * ppMemory
    );

DWORD
RegCStringDuplicate(
    OUT PSTR* ppszNewString,
    IN PCSTR pszOriginalString
    );

void
RegMemoryFree(
	IN OUT LW_PVOID pMemory
	);

DWORD
RegWC16StringAllocateFromCString(
    OUT PWSTR* ppszNewString,
    IN PCSTR pszOriginalString
    );

DWORD
RegCStringAllocateFromWC16String(
    OUT PSTR* ppszNewString,
    IN PCWSTR pszOriginalString
    );

DWORD
RegWC16StringAllocatePrintfW(
    LW_OUT LW_PWSTR* ppszString,
    LW_IN const wchar_t* pszFormat,
    LW_IN ...
    );

DWORD
RegWC16StringDuplicate(
    PWSTR* ppwszNewString,
    PCWSTR pwszOriginalString
    );

DWORD
RegCStringAllocatePrintf(
    OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    IN ...
    );

PCWSTR
RegStrrchr(
    PCWSTR pwszStr,
    wchar16_t wch
    );

PCWSTR
RegStrchr(
    PCWSTR pwszStr,
    wchar16_t wch
    );

LW_DWORD
RegGetErrorMessageForLoggingEvent(
    LW_DWORD dwError,
    LW_PSTR* ppszErrorMsg
    );

DWORD
RegWC16StringArraysAllocateFromCStringArraysWithNullTerminator(
    IN PSTR* ppszStrings,
    OUT PWSTR** pppwszStrings
    );

DWORD
RegCopyValueAToW(
    IN REG_DATA_TYPE dwType,
    IN PVOID pData,
    IN DWORD cbData,
    OUT PVOID* ppOutData,
    OUT PDWORD pcbOutDataLen
    );

DWORD
RegConvertValueAttributesAToW(
    LWREG_VALUE_ATTRIBUTES_A attrA,
    PLWREG_VALUE_ATTRIBUTES* ppAttrW
    );


#endif /* __REG_UTILS_H__ */
