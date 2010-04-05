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
 * Module Name: lwt_lsa_validate_groupinfo_common.h
 *
 * Verifies the group information returned from AD with the CSV file.
 *
 */

#include "includes.h"

#define MAX_GROUP_INFOLEVEL 1 /* 0 and 1*/


/*
 * ValidateGroupInfoLevel0
 * 
 * Function validates group level 0 information from AD with group \
 * information in csv file
 * 
 */
DWORD 
ValidateGroupInfoLevel0(
    PLSA_GROUP_INFO_0 pADGroupInfoList,
    PLWTGROUP pGroupList,
    PSTR pszLogInfo,
    PCSTR pszTestAPI
    );

/*
 * ValidateGroupInfoLevel1
 * 
 * Function validates group level 1 information from AD with group \
 * information in csv file
 * 
 */
DWORD
ValidateGroupInfoLevel1(
    PLSA_GROUP_INFO_1 pADGroupInfoList,
    PLWTGROUP pGroupList,
    PTESTDATA pTestData,
    PSTR  pszLogInfo,
    PCSTR pszTestAPI
    );

/*
 * CompareGroupInfoLevels
 * 
 * Function compares group information in level 0 and level1 
 * 
 */
DWORD
CompareGroupInfoLevels(
    PLSA_GROUP_INFO_0 *ppListGrpInfoLevel0,
    DWORD dwGrpCountInLevel0,
    PLSA_GROUP_INFO_1 *ppListGrpInfoLevel1,
    DWORD dwGrpCountInLevel1,
    PSTR  pszLogInfo,
    PCSTR pszTestApi
    );

/*
 * ResolveGroupListByName
 * 
 * Function gets the csv grouplist for a group
 * 
 */
DWORD
ResolveGroupListByName(
    PSTR       pszGroupName,
    PTESTDATA  pTestData,
    PLWTGROUP* ppGroupList
    );

/*
 * GetGroupsForUserByName
 *   
 * Function retrives the group information for user from AD
 * 
 */
DWORD
GetGroupsForUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszUserName,
    DWORD  dwGroupInfoLevel,
    PVOID  **pppGroupInfoList,
    PDWORD pdwGroups
    );

/*
 * GetGroupsForUserById
 *
 * Function retrives the group information for user from AD
 *
 */
DWORD 
GetGroupsForUserById(
    HANDLE hLsaConnection,
    uid_t  nUnixUid,
    DWORD  dwGroupInfoLevel,
    PVOID  **pppGroupInfoList,
    PDWORD pdwNumGroups
    );

/*
 * CompareMemberList 
 * 
 * Function checks for existance of members in one list with the other list  
 * 
 */
DWORD
CompareMemberList(
    PSTR* ppszGrpMemberList_1,
    PSTR* ppszGrpMemberList_2,
    PBOOLEAN pbEqual 
    );

