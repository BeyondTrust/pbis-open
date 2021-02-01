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
