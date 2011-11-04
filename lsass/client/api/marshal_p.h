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
 * Module Name:
 *
 *        marshal_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Client API Info Level Marshalling
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *          Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#ifndef __LSA_CLIENT_MARSHAL_P_H__
#define __LSA_CLIENT_MARSHAL_P_H__

DWORD
LsaMarshalUserInfo(
    PLSA_SECURITY_OBJECT pUser,
    DWORD       dwUserInfoLevel,
    PVOID*      ppUserInfo
    );

DWORD
LsaMarshalUserInfoList(
    DWORD dwObjectCount,
    PLSA_SECURITY_OBJECT* ppObjects,
    DWORD dwUserInfoLevel,
    DWORD dwUserInfoLength,
    PVOID* ppUserInfo,
    PDWORD pdwObjectUsedCount,
    PDWORD pdwUserInfoCount
    );

DWORD
LsaMarshalGroupInfo(
    HANDLE hLsa,
    LSA_FIND_FLAGS FindFlags,
    PLSA_SECURITY_OBJECT     pGroup,
    DWORD                   dwGroupInfoLevel,
    PVOID*                  ppGroupInfo
    );

DWORD
LsaMarshalGroupInfo1(
    HANDLE hLsa,
    LSA_FIND_FLAGS FindFlags,
    PLSA_SECURITY_OBJECT     pGroup,
    DWORD dwMemberCount,
    PLSA_SECURITY_OBJECT* ppMembers,
    DWORD                   dwGroupInfoLevel,
    PVOID*                  ppGroupInfo
    );

DWORD
LsaMarshalGroupInfoList(
    HANDLE hLsa,
    LSA_FIND_FLAGS FindFlags,
    DWORD dwObjectCount,
    PLSA_SECURITY_OBJECT* ppObjects,
    DWORD dwGroupInfoLevel,
    DWORD dwGroupInfoLength,
    PVOID* ppGroupInfo,
    PDWORD pdwObjectUsedCount,
    PDWORD pdwGroupInfoCount
    );

DWORD
LsaMarshalUserModInfoToUserModInfo2(
    HANDLE hLsa,
    PLSA_USER_MOD_INFO pModInfo1,
    PLSA_USER_MOD_INFO_2* ppModInfo2
    );

DWORD
LsaMarshalGroupModInfoToGroupModInfo2(
    HANDLE hLsa,
    PLSA_GROUP_MOD_INFO pModInfo1,
    PLSA_GROUP_MOD_INFO_2* ppModInfo2
    );

DWORD
LsaMarshalGroupInfo0ToGroupAddInfo(
    HANDLE hLsa,
    PLSA_GROUP_INFO_0 pGroupInfo,
    PLSA_GROUP_ADD_INFO* ppAddInfo
    );

DWORD
LsaMarshalGroupInfo1ToGroupAddInfo(
    HANDLE hLsa,
    PLSA_GROUP_INFO_1 pGroupInfo,
    PLSA_GROUP_ADD_INFO* ppAddInfo
    );

DWORD
LsaMarshalUserInfo0ToUserAddInfo(
    HANDLE hLsa,
    PLSA_USER_INFO_0 pUserInfo,
    PLSA_USER_ADD_INFO* ppAddInfo
    );

#endif
