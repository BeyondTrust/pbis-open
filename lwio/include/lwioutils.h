/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        lwioutils.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __LWIO_UTILS_H__
#define __LWIO_UTILS_H__

#include <lwio/lwio.h>


#define LWIO_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define _LWIO_LOG_AT(Level, ...) LW_RTL_LOG_AT_LEVEL(Level, "lwio", __VA_ARGS__)
#define LWIO_LOG_ALWAYS(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_ALWAYS, __VA_ARGS__)
#define LWIO_LOG_ERROR(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_ERROR, __VA_ARGS__)
#define LWIO_LOG_WARNING(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_WARNING, __VA_ARGS__)
#define LWIO_LOG_INFO(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_INFO, __VA_ARGS__)
#define LWIO_LOG_VERBOSE(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_VERBOSE, __VA_ARGS__)
#define LWIO_LOG_DEBUG(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LWIO_LOG_TRACE(...) _LWIO_LOG_AT(LW_RTL_LOG_LEVEL_TRACE, __VA_ARGS__)

/*
 * Registry
 */

typedef struct __LWIO_CONFIG_REG LWIO_CONFIG_REG, *PLWIO_CONFIG_REG;

typedef enum
{
    LwIoTypeString,
    LwIoTypeMultiString,
    LwIoTypeDword,
    LwIoTypeBoolean,
    LwIoTypeChar,
    LwIoTypeEnum
} LWIO_CONFIG_TYPE;

typedef struct __LWIO_CONFIG_TABLE
{
    PCSTR   pszName;
    BOOLEAN bUsePolicy;
    LWIO_CONFIG_TYPE Type;
    DWORD dwMin;
    DWORD dwMax;
    const PCSTR *ppszEnumNames;
    PVOID pValue;
} LWIO_CONFIG_TABLE, *PLWIO_CONFIG_TABLE;

NTSTATUS
LwIoProcessConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLWIO_CONFIG_TABLE pConfig,
    DWORD dwConfigEntries,
    BOOLEAN bIgnoreNotFound
    );

NTSTATUS
LwIoOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLWIO_CONFIG_REG *ppReg
    );

VOID
LwIoCloseConfig(
    PLWIO_CONFIG_REG pReg
    );

NTSTATUS
LwIoReadConfigString(
    PLWIO_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue
    );

NTSTATUS
LwIoReadConfigMultiString(
    PLWIO_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    **pppszValue
    );

NTSTATUS
LwIoMultiStringCopy(
    PSTR **pppszNewStrings,
    PCSTR *ppszOriginalStrings
    );

VOID
LwIoMultiStringFree(
    PSTR **pppszValue
    );

NTSTATUS
LwIoReadConfigDword(
    PLWIO_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD   dwMin,
    DWORD   dwMax,
    PDWORD pdwValue
    );

NTSTATUS
LwIoReadConfigBoolean(
    PLWIO_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    );

NTSTATUS
LwIoReadConfigEnum(
    PLWIO_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD dwMin,
    DWORD dwMax,
    const PCSTR *ppszEnumNames,
    PDWORD pdwValue
    );


#ifndef _SMB_ENDIAN_SWAP16

#define _SMB_ENDIAN_SWAP16(wX)                     \
        ((((uint16_t)(wX) & 0xFF00) >> 8) |        \
         (((uint16_t)(wX) & 0x00FF) << 8))

#endif

#ifndef _SMB_ENDIAN_SWAP32

#define _SMB_ENDIAN_SWAP32(dwX)                    \
        ((((uint32_t)(dwX) & 0xFF000000L) >> 24) | \
         (((uint32_t)(dwX) & 0x00FF0000L) >>  8) | \
         (((uint32_t)(dwX) & 0x0000FF00L) <<  8) | \
         (((uint32_t)(dwX) & 0x000000FFL) << 24))

#endif

#ifndef _SMB_ENDIAN_SWAP64

#define _SMB_ENDIAN_SWAP64(llX)         \
   (((uint64_t)(_SMB_ENDIAN_SWAP32(((uint64_t)(llX) & 0xFFFFFFFF00000000LL) >> 32))) | \
   (((uint64_t)_SMB_ENDIAN_SWAP32(((uint64_t)(llX) & 0x00000000FFFFFFFFLL))) << 32))

#endif

#if defined(WORDS_BIGENDIAN)
#define SMB_HTOL16(x) _SMB_ENDIAN_SWAP16(x)
#define SMB_HTOL32(x) _SMB_ENDIAN_SWAP32(x)
#define SMB_HTOL64(x) _SMB_ENDIAN_SWAP64(x)
#define SMB_HTOLWSTR(dst, src, len)                                 \
    do                                                              \
    {                                                               \
        PCWSTR __src = (PCWSTR) (src);                              \
        PWSTR __dst = (PWSTR) (dst);                                \
        ssize_t __len = (ssize_t) (len);                            \
        swab(__src, __dst, __len * sizeof(WCHAR));                  \
        __dst[__len] = '\0';                                        \
    } while (0)

#define SMB_LTOH16(x) _SMB_ENDIAN_SWAP16(x)
#define SMB_LTOH32(x) _SMB_ENDIAN_SWAP32(x)
#define SMB_LTOH64(x) _SMB_ENDIAN_SWAP64(x)
#define SMB_LTOHWSTR(dst, src, len) SMB_HTOLWSTR((dst), (src), (len))
#else
#define SMB_HTOL16(x) (x)
#define SMB_HTOL32(x) (x)
#define SMB_HTOL64(x) (x)
#define SMB_HTOLWSTR(dst, src, len)                                 \
    do                                                              \
    {                                                               \
        PCWSTR __src = (PCWSTR) (src);                              \
        PWSTR __dst = (PWSTR) (dst);                                \
        ssize_t __len = (ssize_t) (len);                            \
        memcpy(__dst, __src, __len * sizeof(WCHAR));                \
        __dst[__len] = '\0';                                        \
    } while (0)

#define SMB_LTOH16(x) (x)
#define SMB_LTOH32(x) (x)
#define SMB_LTOH64(x) (x)
#define SMB_LTOHWSTR(dst, src, len) SMB_HTOLWSTR((dst), (src), (len))
#endif

// No-ops, but for readability.
#define SMB_HTOL8(x) (x)
#define SMB_LTOH8(x) (x)

#define SMB_HTOL8_INPLACE(x) ((x) = SMB_HTOL8(x))
#define SMB_HTOL16_INPLACE(x) ((x) = SMB_HTOL16(x))
#define SMB_HTOL32_INPLACE(x) ((x) = SMB_HTOL32(x))
#define SMB_HTOL64_INPLACE(x) ((x) = SMB_HTOL64(x))

#define SMB_LTOH8_INPLACE(x) ((x) = SMB_LTOH8(x))
#define SMB_LTOH16_INPLACE(x) ((x) = SMB_LTOH16(x))
#define SMB_LTOH32_INPLACE(x) ((x) = SMB_LTOH32(x))
#define SMB_LTOH64_INPLACE(x) ((x) = SMB_LTOH64(x))

#define BAIL_ON_LWIO_ERROR(dwError) BAIL_ON_NT_STATUS(dwError)

#define BAIL_ON_NT_STATUS(ntStatus)                \
    if ((ntStatus)) {                              \
       LWIO_LOG_DEBUG("Status: %s = 0x%08X (%d)",  \
           LwNtStatusToName(ntStatus),             \
           ntStatus, ntStatus);                    \
       goto error;                                 \
    }

#define BAIL_ON_NULL_PTR(x, err)                    \
    do {                                            \
        if ((x) == NULL) {                          \
            err = STATUS_INSUFFICIENT_RESOURCES;	\
            goto error;                             \
        }                                           \
    } while(0)                                     \

#define BAIL_ON_INVALID_PTR(ptr, err)                  \
    do {                                               \
        if ((ptr) == NULL) {                           \
            err = STATUS_INVALID_PARAMETER;            \
            goto error;                                \
        }                                              \
    } while (0)

#define BAIL_ON_ZERO_LENGTH(len, err)                  \
    do {                                               \
        if ((len) == 0) {                              \
            err = STATUS_INVALID_PARAMETER;            \
            goto error;                                \
        }                                              \
    } while (0)

#define GOTO_CLEANUP_ON_SMB_ERROR(error) \
    _GOTO_CLEANUP_ON_NONZERO(error)

#define GOTO_CLEANUP_ON_LWIO_ERROR_EE(error, EE) \
    _GOTO_CLEANUP_ON_NONZERO_EE(error, EE)

#define LWIO_SAFE_FREE_STRING(str) \
        do {                      \
           if (str) {             \
              SMBFreeString(str); \
              (str) = NULL;       \
           }                      \
        } while(0);

#define LWIO_SAFE_CLEAR_FREE_STRING(str)       \
        do {                                  \
           if (str) {                         \
              if (*str) {                     \
                 memset(str, 0, strlen(str)); \
              }                               \
              SMBFreeString(str);             \
              (str) = NULL;                   \
           }                                  \
        } while(0);

#define LWIO_SAFE_FREE_MEMORY(mem) \
        do {                      \
           if (mem) {             \
              LwIoFreeMemory(mem); \
              (mem) = NULL;       \
           }                      \
        } while(0);

#define LWIO_SAFE_FREE_STRING_ARRAY(ppszArray)               \
        do {                                                \
           if (ppszArray) {                                 \
               SMBFreeNullTerminatedStringArray(ppszArray); \
               (ppszArray) = NULL;                          \
           }                                                \
        } while (0);

#define IsNullOrEmptyString(str) (!(str) || !(*(str)))

#define LWIO_LOCK_MUTEX(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_mutex_lock(mutex); \
       if (thr_err) { \
           LWIO_LOG_ERROR("Failed to lock mutex: %d. Aborting program", \
               thr_err); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LWIO_UNLOCK_MUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_mutex_unlock(mutex); \
       if (thr_err) { \
           LWIO_LOG_ERROR("Failed to unlock mutex: %d. Aborting program", \
               thr_err); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

#define LWIO_LOCK_RWMUTEX_SHARED(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_rwlock_rdlock(mutex); \
       if (thr_err) { \
           LWIO_LOG_ERROR("Failed to acquire shared lock on rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_rwlock_wrlock(mutex); \
       if (thr_err) { \
           LWIO_LOG_ERROR("Failed to acquire exclusive lock on rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LWIO_UNLOCK_RWMUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_rwlock_unlock(mutex); \
       if (thr_err) { \
           LWIO_LOG_ERROR("Failed to unlock rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

VOID
LwIoAssertionFailed(
    IN PCSTR Expression,
    IN OPTIONAL PCSTR Message,
    IN PCSTR Function,
    IN PCSTR File,
    IN ULONG Line
    );

VOID
LwIoAssertionFailedFormat(
    IN PCSTR Expression,
    IN PCSTR Format,
    IN PCSTR Function,
    IN PCSTR File,
    IN ULONG Line,
    ...
    );

#define LWIO_ASSERT_MSG(Expression, Message) \
    do { \
        if (!(Expression)) \
        { \
            LwIoAssertionFailed(#Expression, Message, __FUNCTION__, __FILE__, __LINE__); \
        } \
    } while(0)

#define LWIO_ASSERT_FORMAT(Expression, Format, ...) \
    do { \
        if (!(Expression)) \
        { \
            LwIoAssertionFailedFormat(#Expression, Format, __FUNCTION__, __FILE__, __LINE__, ## __VA_ARGS__); \
        } \
    } while(0)


#define LWIO_ASSERT(Expression) \
    LWIO_ASSERT_MSG(Expression, NULL)

#define LWIO_ASSERT_VALUE_MSG(Expression, Message) \
    ((Expression) ? TRUE : \
        (LwIoAssertionFailed(#Expression, Message, __FUNCTION__, __FILE__, __LINE__), FALSE))

//defined flags in dwOptions
#define SMB_CFG_OPTION_STRIP_SECTION          0x00000001
#define SMB_CFG_OPTION_STRIP_NAME_VALUE_PAIR  0x00000002
#define SMB_CFG_OPTION_STRIP_ALL              \
        (SMB_CFG_OPTION_STRIP_SECTION | SMB_CFG_OPTION_STRIP_NAME_VALUE_PAIR)

#define BAIL_ON_INVALID_STRING(pszParam)          \
        if (IsNullOrEmptyString(pszParam)) {      \
           dwError = LWIO_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LWIO_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_HANDLE(hParam)            \
        if (hParam == (HANDLE)NULL) {             \
           dwError = LWIO_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LWIO_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_SMBHANDLE(hParam)         \
        if (hParam == (SMBHANDLE) 0) {            \
           dwError = LWIO_ERROR_INVALID_HANDLE;    \
           BAIL_ON_LWIO_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_POINTER(p)                \
        if (NULL == p) {                          \
           dwError = LWIO_ERROR_INVALID_PARAMETER; \
           BAIL_ON_LWIO_ERROR(dwError);            \
        }

typedef struct __LWIO_BIT_VECTOR
{
    ULONG  ulNumBits;
    PULONG pVector;
} LWIO_BIT_VECTOR, *PLWIO_BIT_VECTOR;

typedef struct __SMB_HASH_ENTRY SMB_HASH_ENTRY, *PSMB_HASH_ENTRY;

typedef int (*SMB_HASH_KEY_COMPARE)(PCVOID, PCVOID);
typedef size_t (*SMB_HASH_KEY)(PCVOID);
typedef void (*SMB_HASH_FREE_ENTRY)(const SMB_HASH_ENTRY *);

struct __SMB_HASH_ENTRY
{
    PVOID pKey;
    PVOID pValue;
    //Next value in chain
    struct __SMB_HASH_ENTRY *pNext;
};

typedef struct __SMB_HASH_TABLE
{
    size_t sTableSize;
    size_t sCount;
    SMB_HASH_ENTRY **ppEntries;
    SMB_HASH_KEY_COMPARE fnComparator;
    SMB_HASH_KEY fnHash;
    SMB_HASH_FREE_ENTRY fnFree;
} SMB_HASH_TABLE, *PSMB_HASH_TABLE;

typedef struct __SMB_HASH_ITERATOR
{
    SMB_HASH_TABLE *pTable;
    size_t sEntryIndex;
    SMB_HASH_ENTRY *pEntryPos;
} SMB_HASH_ITERATOR;

typedef struct __SMB_DLINKEDLIST
{
    PVOID pItem;

    struct __SMB_DLINKEDLIST * pNext;

    struct __SMB_DLINKEDLIST * pPrev;

} SMBDLINKEDLIST, *PSMBDLINKEDLIST;

typedef VOID (*PFNSMB_DLINKEDLIST_FUNC)(PVOID pData, PVOID pUserData);

typedef DWORD (*PFNSMB_FOREACH_STACK_ITEM)(PVOID pItem, PVOID pUserData);

typedef struct __SMB_STACK
{
    PVOID pItem;

    struct __SMB_STACK * pNext;

} SMB_STACK, *PSMB_STACK;

typedef VOID (*PFNLWIO_QUEUE_FUNC)(PVOID pData, PVOID pUserData);

typedef DWORD (*PFNLWIO_FOREACH_QUEUE_ITEM)(PVOID pItem, PVOID pUserData);

typedef struct __LWIO_QUEUE_ITEM
{
    PVOID pItem;

    struct __LWIO_QUEUE_ITEM * pNext;
} LWIO_QUEUE_ITEM, *PLWIO_QUEUE_ITEM;

typedef struct __LWIO_QUEUE
{

    PLWIO_QUEUE_ITEM pHead;
    PLWIO_QUEUE_ITEM pTail;

} LWIO_QUEUE, *PLWIO_QUEUE;

typedef DWORD SMBHANDLE, *PSMBHANDLE;

typedef enum
{
    SMB_HANDLE_TYPE_UNKNOWN = 0,
    SMB_HANDLE_TYPE_FILE,
    SMB_HANDLE_TYPE_NAMED_PIPE,
    SMB_HANDLE_TYPE_SOCKET
} SMBHandleType;

typedef struct __SMB_HANDLE_MANAGER
{
    SMBHANDLE       dwHandleMax;
    PSMB_HASH_TABLE pHandleTable;
    PLWIO_BIT_VECTOR pFreeHandleIndex;
} SMB_HANDLE_MANAGER, *PSMB_HANDLE_MANAGER;

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

char*
lsmb_stpncpy(
    char *dest,
    const char* src,
    size_t n
    );

#if !defined(HAVE_STRNLEN)

size_t
strnlen(
    const char *s,
    size_t maxlen
    );

#endif /* !defined(HAVE_STRNLEN) */

LW_NTSTATUS
LwIoAllocateMemory(
    size_t Size,
    LW_PVOID * ppMemory
    );

LW_NTSTATUS
LwIoAllocateBuffer(
    size_t     Size,
    LW_PVOID * ppMemory
    );

VOID
LwIoFreeBuffer(
    LW_PVOID pMemory
    );

LW_NTSTATUS
LwIoReallocMemory(
    LW_PVOID pMemory,
    size_t Size,
    LW_PVOID * ppNewMemory
    );

VOID
LwIoFreeMemory(
    PVOID pMemory
    );

#define IO_SAFE_FREE_MEMORY(pMem)               \
    do                                          \
    {                                           \
        if ((pMem))                             \
        {                                       \
            LwIoFreeMemory((pMem));             \
            (pMem) = NULL;                      \
        }                                       \
    } while (0)

DWORD
SMBAllocateString(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    );

void
SMBFreeString(
    PSTR pszString
    );

void
SMBStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    );

DWORD
SMBStrIsAllSpace(
    PCSTR pszString,
    PBOOLEAN pbIsAllSpace
    );

void
SMBStrToUpper(
    PSTR pszString
    );

void
SMBStrnToUpper(
    PSTR  pszString,
    DWORD dwLen
    );

void
SMBStrToLower(
    PSTR pszString
    );

void
SMBStrnToLower(
    PSTR  pszString,
    DWORD dwLen
    );

DWORD
SMBEscapeString(
    PSTR pszOrig,
    PSTR * ppszEscapedString
    );

DWORD
SMBStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
    );

DWORD
SMBAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    );

DWORD
SMBAllocateStringPrintfV(
    PSTR*   ppszOutputString,
    PCSTR   pszFormat,
    va_list args
    );

VOID
SMBStrCharReplace(
    PSTR pszStr,
    CHAR oldCh,
    CHAR newCh);

// If pszInputString == NULL, then *ppszOutputString = NULL
DWORD
SMBStrDupOrNull(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    );

// If pszInputString == NULL, return "", else return pszInputString
// The return value does not need to be freed.
PCSTR
SMBEmptyStrForNull(
    PCSTR pszInputString
    );

void
SMBStrChr(
    PCSTR pszInputString,
    CHAR  c,
    PSTR *pszOutputString
    );

void
SMBFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    );

void
SMBFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    );

DWORD
SMBAllocateStringW(
    PWSTR  pwszInputString,
    PWSTR* ppwszOutputString
    );

DWORD
SMBMbsToWc16s(
    PCSTR     pszInput,
    PWSTR* ppwszOutput
    );

DWORD
SMBWc16snToMbs(
    PCWSTR pwszInput,
    size_t    sMaxChars,
    PSTR*     ppszOutput
    );

DWORD
SMBWc16sToMbs(
    PCWSTR pwszInput,
    PSTR*     ppszOutput
    );

DWORD
SMBWc16sLen(
    PCWSTR  pwszInput,
    size_t*    psLen
    );

int
SMBWc16sCmp(
    PCWSTR  pwszFirst,
    PCWSTR  pwszSecond
    );

int
SMBWc16snCmp(
    PCWSTR pwszFirst,
    PCWSTR pwszSecond,
    size_t sLen
    );

int
SMBWc16sCaseCmp(
    PCWSTR  pwszFirst,
    PCWSTR  pwszSecond
    );

DWORD
SMBWc16sDup(
    PCWSTR pwszInput,
    PWSTR* pwszOutput
    );

DWORD
SMBDLinkedListPrepend(
    PSMBDLINKEDLIST* ppList,
    PVOID        pItem
    );

DWORD
SMBDLinkedListAppend(
    PSMBDLINKEDLIST* ppList,
    PVOID        pItem
    );

BOOLEAN
SMBDLinkedListDelete(
    PSMBDLINKEDLIST* ppList,
    PVOID        pItem
    );

VOID
SMBDLinkedListForEach(
    PSMBDLINKEDLIST          pList,
    PFNSMB_DLINKEDLIST_FUNC pFunc,
    PVOID                pUserData
    );

VOID
SMBDLinkedListFree(
    PSMBDLINKEDLIST pList
    );

DWORD
SMBStackPush(
    PVOID pItem,
    PSMB_STACK* ppStack
    );

VOID
SMBStackPushNoAlloc(
    PSMB_STACK* ppStack,
    PSMB_STACK  pStack
    );

PVOID
SMBStackPop(
    PSMB_STACK* ppStack
    );

VOID
SMBStackPopNoFree(
    PSMB_STACK* ppStack
    );

PVOID
SMBStackPeek(
    PSMB_STACK pStack
    );

DWORD
SMBStackForeach(
    PSMB_STACK pStack,
    PFNSMB_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    );

PSMB_STACK
SMBStackReverse(
    PSMB_STACK pStack
    );

VOID
SMBStackFree(
    PSMB_STACK pStack
    );

DWORD
LWIOQueueCreate(
    PLWIO_QUEUE* ppQueue
    );

DWORD
SMBEnqueue(
    PLWIO_QUEUE pQueue,
    PVOID      pItem
    );

DWORD
SMBEnqueueFront(
    PLWIO_QUEUE pQueue,
    PVOID      pItem
    );

PVOID
SMBDequeue(
    PLWIO_QUEUE pQueue
    );

BOOLEAN
LWIOQueueIsEmpty(
    PLWIO_QUEUE pQueue
    );

DWORD
LWIOQueueForeach(
    PLWIO_QUEUE pQueue,
    PFNLWIO_FOREACH_QUEUE_ITEM pfnAction,
    PVOID pUserData
    );

VOID
LWIOQueueFree(
    PLWIO_QUEUE pQueue
    );

NTSTATUS
LwioBitVectorCreate(
    ULONG             ulNumBits,
    PLWIO_BIT_VECTOR* ppBitVector
    );

BOOLEAN
LwioBitVectorIsSet(
    PLWIO_BIT_VECTOR pBitVector,
    ULONG            iBit
    );

NTSTATUS
LwioBitVectorSetBit(
    PLWIO_BIT_VECTOR pBitVector,
    ULONG            iBit
    );

NTSTATUS
LwioBitVectorUnsetBit(
    PLWIO_BIT_VECTOR pBitVector,
    ULONG            iBit
    );

NTSTATUS
LwioBitVectorFirstUnsetBit(
    PLWIO_BIT_VECTOR pBitVector,
    PULONG           pulUnsetBit
    );

VOID
LwioBitVectorReset(
    PLWIO_BIT_VECTOR pBitVector
    );

VOID
LwioBitVectorFree(
    PLWIO_BIT_VECTOR pBitVector
    );

NTSTATUS
SMBHashCreate(
        size_t sTableSize,
        SMB_HASH_KEY_COMPARE fnComparator,
        SMB_HASH_KEY fnHash,
        SMB_HASH_FREE_ENTRY fnFree, //optional
        SMB_HASH_TABLE** ppResult);

VOID
SMBHashSafeFree(
        SMB_HASH_TABLE** ppResult);

NTSTATUS
SMBHashSetValue(
        SMB_HASH_TABLE *pTable,
        PVOID  pKey,
        PVOID  pValue);

//Returns STATUS_NOT_FOUND if pKey is not in the table
NTSTATUS
SMBHashGetValue(
        SMB_HASH_TABLE *pTable,
        PCVOID  pKey,
        PVOID* ppValue);

BOOLEAN
SMBHashExists(
    PSMB_HASH_TABLE pTable,
    PCVOID pKey
    );

//Invalidates all iterators
NTSTATUS
SMBHashResize(
        SMB_HASH_TABLE *pTable,
        size_t sTableSize);

NTSTATUS
SMBHashGetIterator(
        SMB_HASH_TABLE *pTable,
        SMB_HASH_ITERATOR *pIterator);

// returns NULL after passing the last entry
SMB_HASH_ENTRY *
SMBHashNext(
        SMB_HASH_ITERATOR *pIterator
        );

NTSTATUS
SMBHashRemoveKey(
        SMB_HASH_TABLE *pTable,
        PCVOID  pKey);

int
SMBHashCaselessStringCompare(
        PCVOID str1,
        PCVOID str2);

int
SMBHashCaselessWc16StringCompare(
        PCVOID str1,
        PCVOID str2);

int
SMBHashCompareUINT32(
        PCVOID key1,
        PCVOID key2
        );

size_t
SMBHashCaselessString(
    PCVOID str);

size_t
SMBHashCaselessWc16String(
    PCVOID str);

DWORD
SMBRemoveFile(
    PCSTR pszPath
    );

DWORD
SMBCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
SMBCheckSockExists(
    PSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
SMBMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
SMBChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
SMBChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
SMBChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
SMBGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
SMBChangeDirectory(
    PSTR pszPath
    );

DWORD
SMBRemoveDirectory(
    PSTR pszPath
    );

DWORD
SMBCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

DWORD
SMBCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
SMBCopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    );

DWORD
SMBGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    );

DWORD
SMBCopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
SMBCreateSymlink(
   PCSTR pszOldPath,
   PCSTR pszNewPath
   );

DWORD
SMBGetPrefixDirPath(
    PSTR* ppszPath
    );

DWORD
SMBGetLibDirPath(
    PSTR* ppszPath
    );

DWORD
LwioGetHostInfo(
    PSTR* ppszHostname
    );

#endif /* __LWIO_UTILS_H__ */





/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
