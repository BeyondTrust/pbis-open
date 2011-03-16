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
 *        rsys-utils.h
 *
 * Abstract:
 *
 *        Reaper for syslog
 *
 *        System utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __RSYS_UTILS_H__
#define __RSYS_UTILS_H__

typedef struct __DLINKEDLIST
{
    PVOID pItem;
    
    struct __DLINKEDLIST * pNext;
    
    struct __DLINKEDLIST * pPrev;
    
} DLINKEDLIST, *PDLINKEDLIST;

typedef VOID (*PFN_DLINKEDLIST_FUNC)(PVOID pData, PVOID pUserData);

typedef DWORD (*PFN_RSYS_FOREACH_STACK_ITEM)(PVOID pItem, PVOID pUserData);

typedef struct __RSYS_STACK
{
    PVOID pItem;
    
    struct __RSYS_STACK * pNext;
    
} RSYS_STACK, *PRSYS_STACK;

//defined flags in dwOptions
#define RSYS_CFG_OPTION_STRIP_SECTION          0x00000001
#define RSYS_CFG_OPTION_STRIP_NAME_VALUE_PAIR  0x00000002
#define RSYS_CFG_OPTION_STRIP_ALL (RSYS_CFG_OPTION_STRIP_SECTION |      \
                                     RSYS_CFG_OPTION_STRIP_NAME_VALUE_PAIR)

// This standardizes the width to 64 bits.  This is useful for
/// writing to files and such.

// This is in seconds (or milliseconds, microseconds, nanoseconds) since
// Jan 1, 1970.
typedef int64_t RSYS_UNIX_TIME_T, *PRSYS_UNIX_TIME_T;
typedef int64_t RSYS_UNIX_MS_TIME_T, *PRSYS_UNIX_MS_TIME_T;
typedef int64_t RSYS_UNIX_US_TIME_T, *PRSYS_UNIX_US_TIME_T;
typedef int64_t RSYS_UNIX_NS_TIME_T, *PRSYS_UNIX_NS_TIME_T;

// This is in 100ns units from Jan 1, 1601:
typedef int64_t RSYS_WINDOWS_TIME_T, *PRSYS_WINDOWS_TIME_T;

/*
 * Config parsing callbacks
 */
typedef DWORD (*PFNRSYS_CONFIG_START_SECTION)(
                        PCSTR    pszSectionName,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNRSYS_CONFIG_COMMENT)(
                        PCSTR    pszComment,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNRSYS_CONFIG_NAME_VALUE_PAIR)(
                        PCSTR    pszName,
                        PCSTR    pszValue,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNRSYS_CONFIG_END_SECTION)(
                        PCSTR pszSectionName,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef struct _RSYS_CONFIG_SETTINGS RSYS_CONFIG_SETTINGS, *PRSYS_CONFIG_SETTINGS;

typedef DWORD (*PFNRSYS_CFG_START_SECTION)(
                        PRSYS_CONFIG_SETTINGS pSettings,
                        PCSTR pszSectionName
                        );

typedef DWORD (*PFNRSYS_CFG_UNKNOWN_VALUE)(
                        PRSYS_CONFIG_SETTINGS pSettings,
                        PCSTR pszName,
                        PCSTR pszValue
                        );

typedef enum
{
    Dword,
    String,
    Enum,
    Boolean
} RSysCfgType;

typedef struct _RSYS_CONFIG_SETTING
{
    PCSTR pszSectionName;
    PCSTR pszName;
    BOOLEAN bUsePolicy;
    RSysCfgType type;
    DWORD dwMin;
    DWORD dwMax;
    const PCSTR* ppszEnumNames;
    PVOID pvDest;
    BOOLEAN bParsed;
} RSYS_CONFIG_SETTING, *PRSYS_CONFIG_SETTING;

struct _RSYS_CONFIG_SETTINGS
{
    DWORD dwCount;
    PRSYS_CONFIG_SETTING pSettings;
    PVOID pvUserData;
};

void
RSysFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    );

void
RSysFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    );

DWORD
RSysParseConfigFileEx(
    PVOID                            pData
    );

DWORD
RSysParseConfigFile(
    PRSYS_CONFIG_SETTINGS pSettings
    );

DWORD
RSysDLinkedList(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

DWORD
RSysDLinkedListAppend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

BOOLEAN
RSysDLinkedListDelete(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

VOID
RSysDLinkedListForEach(
    PDLINKEDLIST          pList,
    PFN_DLINKEDLIST_FUNC pFunc,
    PVOID                pUserData
    );

VOID
RSysDLinkedListFree(
    PDLINKEDLIST pList
    );

DWORD
RSysStackPush(
    PVOID pItem,
    PRSYS_STACK* ppStack
    );

PVOID
RSysStackPop(
    PRSYS_STACK* ppStack
    );

PVOID
RSysStackPeek(
    PRSYS_STACK pStack
    );

DWORD
RSysStackForeach(
    PRSYS_STACK pStack,
    PFN_RSYS_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    );

PRSYS_STACK
RSysStackReverse(
    PRSYS_STACK pStack
    );

VOID
RSysStackFree(
    PRSYS_STACK pStack
    );

DWORD
RSysRemoveFile(
    PCSTR pszPath
    );

DWORD
RSysCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
RSysCheckSockExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
RSysMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
RSysChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
RSysChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
RSysChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
RSysGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
RSysChangeDirectory(
    PSTR pszPath
    );

DWORD
RSysRemoveDirectory(
    PCSTR pszPath
    );

DWORD
RSysCopyDirectory(
    PCSTR pszSourceDirPath,
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszDestDirPath
    );

DWORD
RSysCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

DWORD
RSysCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
RSysCopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    );

DWORD
RSysGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    );

DWORD
RSysCopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
RSysBackupFile(
    PCSTR pszPath
    );

DWORD
RSysGetSymlinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   );

DWORD
RSysCreateSymlink(
   PCSTR pszOldPath,
   PCSTR pszNewPath
   );

DWORD
RSysGetMatchingFilePathsInFolder(
    PCSTR pszDirPath,
    PCSTR pszFileNameRegExp,
    PSTR** pppszHostFilePaths,
    PDWORD pdwNPaths
    );

DWORD
RSysProcessConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PRSYS_CONFIG_SETTING pConfig,
    DWORD dwConfigEntries
    );

#endif /* __RSYS_UTILS_H__ */
