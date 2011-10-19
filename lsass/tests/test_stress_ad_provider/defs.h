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
 *        defs.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Test Program for stress testing AD Provider
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __DEFS_H__
#define __DEFS_H__

#define LADS_SECTION_NAME_AUTH_PROVIDER      "auth-provider"
#define LADS_SECTION_NAME_FIND_USER_BY_NAME  "find-user-by-name"
#define LADS_SECTION_NAME_FIND_USER_BY_ID    "find-user-by-id"
#define LADS_SECTION_NAME_ENUM_USERS         "enum-users"
#define LADS_SECTION_NAME_FIND_GROUP_BY_NAME "find-group-by-name"    
#define LADS_SECTION_NAME_FIND_GROUP_BY_ID   "find-group-by-id"
#define LADS_SECTION_NAME_ENUM_GROUPS        "enum-groups"

#define LADS_ATTR_NAME_CONFIG_FILE           "config-file"
#define LADS_ATTR_NAME_LIBPATH               "lib-path"
#define LADS_ATTR_NAME_NAME                  "name"
#define LADS_ATTR_NAME_ID                    "id"
#define LADS_ATTR_NAME_THREADS               "threads"
#define LADS_ATTR_NAME_SLEEP                 "sleep-msecs"
#define LADS_ATTR_NAME_INFO_LEVEL            "info-level"

typedef struct _TEST_AUTH_PROVIDER {
    PSTR pszId;
    PSTR pszProviderLibpath;
    PVOID pLibHandle;
    PCSTR pszName;
    PLSA_PROVIDER_FUNCTION_TABLE pFnTable;
    struct _TEST_AUTH_PROVIDER *pNext;
} TEST_AUTH_PROVIDER, *PTEST_AUTH_PROVIDER;

typedef enum
{
    LADS_AUTH_PROVIDER = 0,
    LADS_FIND_USER_BY_NAME,
    LADS_FIND_USER_BY_ID,
    LADS_ENUM_USERS,
    LADS_FIND_GROUP_BY_NAME,
    LADS_FIND_GROUP_BY_ID,
    LADS_ENUM_GROUPS,
    LADS_SENTINEL
} LADSItemType;

typedef struct __LADS_STRESS_DATA
{
    LADSItemType type;
    DWORD        dwNumThreads;
    pthread_t*   pThreadArray;
    DWORD        dwSleepMSecs;
    DWORD        dwInfoLevel;
    DWORD        dwNumItems;
    union
    {
        PSTR*  ppszNames;
        uid_t* pUidArray;
        gid_t* pGidArray;
    } data;
} LADS_STRESS_DATA, *PLADS_STRESS_DATA;

typedef struct __LADS_CONFIG_DATA
{
    LADSItemType itemType;
    DWORD        dwNumThreads;
    DWORD        dwSleepMSecs;
    DWORD        dwInfoLevel;
    DWORD        dwNumItems;
    PDLINKEDLIST pItemList;
    
} LADS_CONFIG_DATA, *PLADS_CONFIG_DATA;

#endif /* __DEFS_H__ */
