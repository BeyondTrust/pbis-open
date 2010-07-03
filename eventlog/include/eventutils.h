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
 *        eventutils.h
 *
 * Abstract:
 *
 *        Likewise Eventlog Service (LWEVT)
 *
 *        Public header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __EVENTUTILS_H__
#define __EVENTUTILS_H__

#ifndef _WIN32

#define PATH_SEPARATOR_STR "/"

#else

#define PATH_SEPARATOR_STR "\\"

#endif

#define BAIL_ON_NON_LWREG_ERROR(dwError) \
        if (!(40700 <= dwError && dwError <= 41200)) {  \
           BAIL_ON_EVT_ERROR(dwError);            \
        }

typedef DWORD (*PFN_EVT_FOREACH_STACK_ITEM)(
                    PVOID pItem,
                    PVOID pUserData
                    );

typedef struct __EVT_STACK
{
    PVOID pItem;

    struct __EVT_STACK * pNext;

} EVT_STACK, *PEVT_STACK;


typedef struct _LOGFILEINFO {
    CHAR szLogPath[PATH_MAX+1];
    FILE* logHandle;
} LOGFILEINFO, *PLOGFILEINFO;

typedef struct _SYSLOGINFO {
    CHAR szIdentifier[PATH_MAX+1];
    DWORD dwOption;
    DWORD dwFacility;
} SYSLOGINFO, *PSYSLOGINFO;

typedef struct _LOGINFO {
    pthread_mutex_t lock;
    DWORD dwLogLevel;
    DWORD logTarget;
    union _logdata {
        LOGFILEINFO logfile;
        SYSLOGINFO syslog;
    } data;
    BOOLEAN  bLoggingInitiated;
} LOGINFO, *PLOGINFO;

typedef struct __EVT_CONFIG_REG EVT_CONFIG_REG, *PEVT_CONFIG_REG;

typedef enum
{
    EVTTypeString,
    EVTTypeDword,
    EVTTypeBoolean,
    EVTTypeChar,
    EVTTypeEnum
} EVT_CONFIG_TYPE;

typedef struct __EVT_CONFIG_TABLE
{
    PCSTR   pszName;
    BOOLEAN bUsePolicy;
    EVT_CONFIG_TYPE Type;
    DWORD dwMin;
    DWORD dwMax;
    const PCSTR *ppszEnumNames;
    PVOID pValue;
} EVT_CONFIG_TABLE, *PEVT_CONFIG_TABLE;

DWORD
EVTAllocateMemory(
    DWORD  dwSize,
    PVOID* ppMemory
    );

DWORD
EVTReallocMemory(
    PVOID  pMemory,
    PVOID* ppNewMemory,
    DWORD  dwSize
    );

VOID
EVTFreeMemory(
    PVOID pMemory
    );

DWORD
RPCAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    );

VOID
RPCFreeMemory(
    PVOID pMemory
    );


DWORD
RPCAllocateString(
    PCSTR pszInputString,
    PSTR* ppszOutputString
    );

VOID
RPCFreeString(
    PSTR pszString
    );

DWORD
EVTAllocateString(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    );


VOID
EVTFreeString(
    PSTR pszString
    );

VOID
EVTFreeStringArray(
    PSTR* ppStringArray,
    DWORD dwCount
    );

DWORD
EVTMbsToWc16s(
    PCSTR pszInputString,
    PWSTR *ppszOutputString
    );

DWORD
EVTStrndup(
    PCSTR  pszInputString,
    size_t size,
    PSTR * ppszOutputString
    );

PCSTR
TableCategoryToStr(
    DWORD tableCategory
    );

BOOLEAN
EVTIsWhiteSpace(
    char c
    );

/*
 * Modify PSTR in-place to convert sequences
 * of whitespace characters into
 * single spaces (0x20)
 */
DWORD
EVTCompressWhitespace(
    PSTR pszString
    );

/*
 * Convert a 16-bit string to an 8-bit string
 * Allocate new memory in the process
 */
DWORD
EVTLpwStrToLpStr(
    PCWSTR pszwString,
    PSTR*  ppszString
    );

VOID
EVTStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    );

VOID
EVTStrToUpper(
    PSTR pszString
    );

VOID
EVTStrToLower(
    PSTR pszString
    );

DWORD
EVTEscapeString(
    PSTR pszOrig,
    PSTR * ppszEscapedString
    );

DWORD
EVTAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    );

DWORD
EVTAllocateStringPrintfV(
    PSTR*   ppszOutputString,
    PCSTR   pszFormat,
    va_list args
    );

DWORD
EVTStackPush(
    PVOID pItem,
    PEVT_STACK* ppStack
    );

PVOID
EVTStackPop(
    PEVT_STACK* ppStack
    );

PVOID
EVTStackPeek(
    PEVT_STACK pStack
    );

DWORD
EVTStackForeach(
    PEVT_STACK pStack,
    PFN_EVT_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    );

PEVT_STACK
EVTStackReverse(
    PEVT_STACK pStack
    );

VOID
EVTStackFree(
    PEVT_STACK pStack
    );

DWORD
EVTProcessConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PEVT_CONFIG_TABLE pConfig,
    DWORD dwConfigEntries
    );

DWORD
EVTOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PEVT_CONFIG_REG *ppReg
    );

VOID
EVTCloseConfig(
    PEVT_CONFIG_REG pReg
    );

DWORD
EVTReadConfigString(
    PEVT_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue
    );

DWORD
EVTReadConfigDword(
    PEVT_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD   dwMin,
    DWORD   dwMax,
    PDWORD pdwValue
    );

DWORD
EVTReadConfigBoolean(
    PEVT_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    );

DWORD
EVTReadConfigEnum(
    PEVT_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD dwMin,
    DWORD dwMax,
    const PCSTR *ppszEnumNames,
    PDWORD pdwValue
    );

DWORD
EVTRemoveFile(
    PCSTR pszPath
    );

DWORD
EVTCheckFileExists(
    PCSTR    pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
EVTMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
EVTChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
EVTChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
EVTChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
EVTGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
EVTChangeDirectory(
    PCSTR pszPath
    );

DWORD
EVTRemoveDirectory(
    PCSTR pszPath
    );

DWORD
EVTGetFileSize(
	PCSTR pszPath,
	PDWORD pdwFileSize
	);

DWORD
EVTCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

DWORD
EVTCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
EVTGetHostname(
    PSTR* ppszHostname
    );

extern FILE*   gBasicLogStreamFD;
extern DWORD   gLogLevel;
extern LOGINFO gEvtLogInfo;

VOID
EVTLogMessage(
    DWORD dwLogLevel,
    PCSTR pszFormat,
    ...
    );

DWORD
EVTInitLoggingToSyslog(
    DWORD dwLogLevel,
    PCSTR pszIdentifier,
    DWORD dwOption,
    DWORD dwFacility
    );

DWORD
EVTSetLogLevel(
    DWORD dwLogLevel
    );

DWORD
EVTInitLoggingToFile(
    DWORD dwLogLevel,
    PCSTR pszLogFilePath
    );

VOID
EVTCloseLog(
    VOID
    );

BOOLEAN
LWIIsLocalHost(
    const char * hostname
    );

DWORD
LWIStr2Inet4Addr(
    IN PCSTR src,
    OUT PSTR *dst
    );

#endif /* __EVENTUTILS_H__ */
