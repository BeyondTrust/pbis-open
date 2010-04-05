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
 *        evtfwd-utils.h
 *
 * Abstract:
 *
 *        Event forwarder from eventlogd to collector service
 *
 *        System utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __EFD_UTILS_H__
#define __EFD_UTILS_H__

typedef struct __DLINKEDLIST
{
    PVOID pItem;
    
    struct __DLINKEDLIST * pNext;
    
    struct __DLINKEDLIST * pPrev;
    
} DLINKEDLIST, *PDLINKEDLIST;

typedef VOID (*PFN_DLINKEDLIST_FUNC)(PVOID pData, PVOID pUserData);

typedef DWORD (*PFN_EFD_FOREACH_STACK_ITEM)(PVOID pItem, PVOID pUserData);

typedef struct __EFD_STACK
{
    PVOID pItem;
    
    struct __EFD_STACK * pNext;
    
} EFD_STACK, *PEFD_STACK;

//defined flags in dwOptions
#define EFD_CFG_OPTION_STRIP_SECTION          0x00000001
#define EFD_CFG_OPTION_STRIP_NAME_VALUE_PAIR  0x00000002
#define EFD_CFG_OPTION_STRIP_ALL (EFD_CFG_OPTION_STRIP_SECTION |      \
                                     EFD_CFG_OPTION_STRIP_NAME_VALUE_PAIR)

// This standardizes the width to 64 bits.  This is useful for
/// writing to files and such.

// This is in seconds (or milliseconds, microseconds, nanoseconds) since
// Jan 1, 1970.
typedef int64_t EFD_UNIX_TIME_T, *PEFD_UNIX_TIME_T;
typedef int64_t EFD_UNIX_MS_TIME_T, *PEFD_UNIX_MS_TIME_T;
typedef int64_t EFD_UNIX_US_TIME_T, *PEFD_UNIX_US_TIME_T;
typedef int64_t EFD_UNIX_NS_TIME_T, *PEFD_UNIX_NS_TIME_T;

// This is in 100ns units from Jan 1, 1601:
typedef int64_t EFD_WINDOWS_TIME_T, *PEFD_WINDOWS_TIME_T;

/*
 * Config parsing callbacks
 */
typedef DWORD (*PFNEFD_CONFIG_START_SECTION)(
                        PCSTR    pszSectionName,
                        PVOID    pData,
                        PBOOLEAN pbSkipSection,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNEFD_CONFIG_COMMENT)(
                        PCSTR    pszComment,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNEFD_CONFIG_NAME_VALUE_PAIR)(
                        PCSTR    pszName,
                        PCSTR    pszValue,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNEFD_CONFIG_END_SECTION)(
                        PCSTR pszSectionName,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef struct __EFD_CONFIG_REG EFD_CONFIG_REG, *PEFD_CONFIG_REG;

typedef enum
{
    Dword,
    String,
    Enum,
    Boolean
} EfdCfgType;

typedef struct _EFD_CONFIG_SETTING
{
    PCSTR pszSectionName;
    PCSTR pszName;
    EfdCfgType type;
    DWORD dwMin;
    DWORD dwMax;
    const PCSTR* ppszEnumNames;
    PVOID pvDest;
    BOOLEAN bParsed;
} EFD_CONFIG_SETTING, *PEFD_CONFIG_SETTING;

typedef struct _EFD_CONFIG_SETTINGS
{
    DWORD dwCount;
    PEFD_CONFIG_SETTING pSettings;
} EFD_CONFIG_SETTINGS, *PEFD_CONFIG_SETTINGS;

DWORD
EfdProcessConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PEFD_CONFIG_SETTING pConfig,
    DWORD dwConfigEntries
    );

DWORD
EfdOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PEFD_CONFIG_REG *ppReg
    );

VOID
EfdCloseConfig(
    PEFD_CONFIG_REG pReg
    );

DWORD
EfdReadConfigString(
    PEFD_CONFIG_REG pReg,
    PCSTR   pszName,
    PSTR    *ppszValue
    );

DWORD
EfdReadConfigDword(
    PEFD_CONFIG_REG pReg,
    PCSTR pszName,
    DWORD   dwMin,
    DWORD   dwMax,
    PDWORD pdwValue
    );

DWORD
EfdReadConfigBoolean(
    PEFD_CONFIG_REG pReg,
    PCSTR pszName,
    PBOOLEAN pbValue
    );

DWORD
EfdReadConfigEnum(
    PEFD_CONFIG_REG pReg,
    PCSTR pszName,
    DWORD dwMin,
    DWORD dwMax,
    const PCSTR *ppszEnumNames,
    PDWORD pdwValue
    );

DWORD
EfdAllocateMemory(
    DWORD  dwSize,
    PVOID* ppMemory
    );

VOID
EfdFreeMemory(
    PVOID pMemory
    );

DWORD
EfdAllocateString(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    );

void
EfdFreeString(
    PSTR pszString
    );

void
EfdFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    );

void
EfdFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    );

DWORD
EfdParseConfigFileEx(
    PCSTR                            pszFilePath,
    DWORD                            dwOptions,
    PFNEFD_CONFIG_START_SECTION   pfnStartSectionHandler,
    PFNEFD_CONFIG_COMMENT         pfnCommentHandler,
    PFNEFD_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNEFD_CONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                            pData
    );

DWORD
EfdParseConfigFile(
    PCSTR pszFilePath,
    DWORD dwOptions,
    PEFD_CONFIG_SETTINGS pSettings
    );

DWORD
EfdDLinkedList(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

DWORD
EfdDLinkedListAppend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

BOOLEAN
EfdDLinkedListDelete(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

VOID
EfdDLinkedListForEach(
    PDLINKEDLIST          pList,
    PFN_DLINKEDLIST_FUNC pFunc,
    PVOID                pUserData
    );

VOID
EfdDLinkedListFree(
    PDLINKEDLIST pList
    );

DWORD
EfdStackPush(
    PVOID pItem,
    PEFD_STACK* ppStack
    );

PVOID
EfdStackPop(
    PEFD_STACK* ppStack
    );

PVOID
EfdStackPeek(
    PEFD_STACK pStack
    );

DWORD
EfdStackForeach(
    PEFD_STACK pStack,
    PFN_EFD_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    );

PEFD_STACK
EfdStackReverse(
    PEFD_STACK pStack
    );

VOID
EfdStackFree(
    PEFD_STACK pStack
    );

DWORD
EfdRemoveFile(
    PCSTR pszPath
    );

DWORD
EfdCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
EfdCheckSockExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
EfdMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
EfdChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
EfdChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
EfdChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
EfdGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
EfdChangeDirectory(
    PSTR pszPath
    );

DWORD
EfdRemoveDirectory(
    PCSTR pszPath
    );

DWORD
EfdCopyDirectory(
    PCSTR pszSourceDirPath,
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszDestDirPath
    );

DWORD
EfdCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

DWORD
EfdCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
EfdCopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    );

DWORD
EfdGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    );

DWORD
EfdCopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
EfdBackupFile(
    PCSTR pszPath
    );

DWORD
EfdGetSymlinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   );

DWORD
EfdCreateSymlink(
   PCSTR pszOldPath,
   PCSTR pszNewPath
   );

DWORD
EfdGetMatchingFilePathsInFolder(
    PCSTR pszDirPath,
    PCSTR pszFileNameRegExp,
    PSTR** pppszHostFilePaths,
    PDWORD pdwNPaths
    );

DWORD
EfdGetPrefixDirPath(
    PSTR* ppszPath
    );

DWORD
EfdGetLibDirPath(
    PSTR* ppszPath
    );

#if !HAVE_DECL_ISBLANK && !defined(isblank)
int isblank(int c);
#endif

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


void
EfdStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    );

DWORD
EfdStrIsAllSpace(
    PCSTR pszString,
    PBOOLEAN pbIsAllSpace
    );

void
EfdStrToUpper(
    PSTR pszString
    );

void
EfdStrToLower(
    PSTR pszString
    );

DWORD
EfdStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
    );

DWORD
EfdStrDupOrNull(
    PCSTR pszInputString, 
    PSTR *ppszOutputString
    );

DWORD
EfdHexStrToByteArray(
    PCSTR   pszHexString,
    UCHAR** ppucByteArray,
    DWORD*  pdwByteArrayLength
    );


DWORD
EfdByteArrayToHexStr(
    UCHAR* pucByteArray,
    DWORD dwByteArrayLength,
    PSTR* ppszHexString
    );


DWORD
EfdHexCharToByte(
    CHAR cHexChar,
    UCHAR* pucByte
    );

int
EfdStrError(
    int errnum,
    char *pszBuf,
    size_t buflen
    );

#endif /* __EFD_UTILS_H__ */
