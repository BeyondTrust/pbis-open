/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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

