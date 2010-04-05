/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwmgmtcommon.h
 *
 * Abstract:
 *
 *        Likewise Management Service (LWMGMT)
 *
 *        Project Private Header for lwmgmt/common
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LWMGMTCOMMON_H__
#define __LWMGMTCOMMON_H__

#include <lw/types.h>

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

/*
 * Stack
 */
typedef DWORD (*PFN_FOREACH_STACK_ITEM)(
                PVOID pItem,
                PVOID pUserData
                );

typedef struct __LWMGMTSTACK
{
    PVOID pItem;
    
    struct __LWMGMTSTACK * pNext;
    
} LWMGMTSTACK, *PLWMGMTSTACK;

/*
 * Config parsing
 */
typedef DWORD (*PFNCONFIG_START_SECTION)(
                        PCSTR    pszSectionName,
                        PBOOLEAN pbSkipSection,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNCONFIG_COMMENT)(
                        PCSTR    pszComment,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNCONFIG_NAME_VALUE_PAIR)(
                        PCSTR    pszName,
                        PCSTR    pszValue,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNCONFIG_END_SECTION)(
                        PCSTR pszSectionName,
                        PBOOLEAN pbContinue
                        );

#if !HAVE_DECL_ISBLANK
int isblank(int c);
#endif

DWORD
LWMGMTStackPush(
    PVOID pItem,
    PLWMGMTSTACK* ppStack
    );

PVOID
LWMGMTStackPop(
    PLWMGMTSTACK* ppStack
    );

PVOID
LWMGMTStackPeek(
    PLWMGMTSTACK pStack
    );

DWORD
LWMGMTStackForeach(
    PLWMGMTSTACK pStack,
    PFN_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    );

PLWMGMTSTACK
LWMGMTStackReverse(
    PLWMGMTSTACK pStack
    );

VOID
LWMGMTStackFree(
    PLWMGMTSTACK pStack
    );

DWORD
LWMGMTParseConfigFile(
    PCSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler
    );

DWORD
LWMGMTRemoveFile(
    PSTR pszPath
    );

DWORD
LWMGMTCheckFileExists(
    PSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
LWMGMTMoveFile(
    PSTR pszSrcPath,
    PSTR pszDstPath
    );

DWORD
LWMGMTChangePermissions(
    PSTR pszPath,
    mode_t dwFileMode
    );

DWORD
LWMGMTChangeOwner(
    PSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
LWMGMTChangeOwnerAndPermissions(
    PSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
LWMGMTGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
LWMGMTChangeDirectory(
    PSTR pszPath
    );

DWORD
LWMGMTRemoveDirectory(
    PSTR pszPath
    );

DWORD
LWMGMTCheckDirectoryExists(
    PSTR pszPath,
    PBOOLEAN pbDirExists
    );

DWORD
LWMGMTCreateDirectory(
    PSTR pszPath,
    mode_t dwFileMode
    );

DWORD
LWMGMTAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    );

DWORD
LWMGMTReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    );


void
LWMGMTFreeMemory(
    PVOID pMemory
    );


DWORD
LWMGMTAllocateString(
    PCSTR pszInputString, 
    PSTR *ppszOutputString
    );


void
LWMGMTFreeString(
    PSTR pszString
    );

void
LWMGMTFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    );

#ifdef _WIN32
 void __RPC_FAR * __RPC_USER midl_user_allocate(size_t cBytes);

 void __RPC_USER midl_user_free(void __RPC_FAR * p);
#endif
 
 DWORD
 LWMGMTStrndup(
     PCSTR pszInputString,
     size_t size,
     PSTR * ppszOutputString
     );

 PCSTR 
 TableCategoryToStr(
     DWORD tableCategory
     );

 BOOLEAN
 LWMGMTIsWhiteSpace(
     char c
     );

 /* 
  * modify PSTR in-place to conver sequences of
  * whitespace characters into single spaces (0x20)
  */
 DWORD
 LWMGMTCompressWhitespace(
     PSTR pszString
     );


 /* convert a 16-bit string to an 8-bit string
  * Allocate new memory in the process
  */
 DWORD
 LWMGMTLpwStrToLpStr(
     PCWSTR pszwString,
     PSTR* ppszString
     );

 void
 LWMGMTStripWhitespace(
     PSTR pszString,
     BOOLEAN bLeading,
     BOOLEAN bTrailing
     );

 void
 LWMGMTStrToUpper(
     PSTR pszString
     );

 void
 LWMGMTStrToLower(
     PSTR pszString
     );

 DWORD
 LWMGMTEscapeString(
     PSTR pszOrig,
     PSTR * ppszEscapedString
     );
 
 DWORD
 LWMGMTAllocateStringPrintf(
     PSTR* ppszOutputString,
     PCSTR pszFormat,
     ...
     );
 
 DWORD
 LWMGMTAllocateStringPrintfV(
     PSTR*   ppszOutputString,
     PCSTR   pszFormat,
     va_list args
     );
 
 DWORD
 RPCAllocateMemory(
     DWORD dwSize,
     PVOID * ppMemory
     );

 void
 RPCFreeMemory(
     PVOID pMemory
     );


 DWORD
 RPCAllocateString(
     PSTR pszInputString, 
     PSTR *ppszOutputString
     );


 void
 RPCFreeString(
     PSTR pszString
     );

DWORD
LWMGMTGetRpcError(
    dcethread_exc* exCatch,
    DWORD dwLWMGMTError
    );
 
extern FILE* gBasicLogStreamFD;
extern DWORD gLogLevel;
extern LOGINFO gLWMGMTLogInfo;

void
LWMGMTLogMessage(
     DWORD dwLogLevel,
     PSTR pszFormat, ...
     );


DWORD
LWMGMTInitLoggingToSyslog(
     DWORD dwLogLevel,
     PSTR  pszIdentifier,
     DWORD dwOption,
     DWORD dwFacility
     );

DWORD
LWMGMTSetLogLevel(
     DWORD dwLogLevel
     );

DWORD
LWMGMTInitLoggingToFile(
     DWORD dwLogLevel,
     PSTR  pszLogFilePath
     );

void
LWMGMTCloseLog();

DWORD
LWMGMTInitBasicLogStream(
     PSTR  pszLogFilePath
     );

void
LWMGMTCloseBasicLogStream();


#endif /* __LWMGMTCOMMON_H__ */

