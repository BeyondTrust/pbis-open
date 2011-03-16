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


typedef struct _RSYS_CONFIG_SETTINGS RSYS_CONFIG_SETTINGS, *PRSYS_CONFIG_SETTINGS;

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
RSysProcessConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PRSYS_CONFIG_SETTING pConfig,
    DWORD dwConfigEntries
    );

#endif /* __RSYS_UTILS_H__ */
