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
 * lwt_lsa_add_user 
 *
 * Verify that information returned by AD matches the CSV file.
 *
 *
 */


#include "includes.h"

static
DWORD
TestAddGroup(
    HANDLE hLsaConnection,
    PSTR pszGroup
    );

static
DWORD
TestAddUser(
    HANDLE hLsaConnection,
    PSTR pszUser
    );
#if 0
DWORD
TestEnumGroups(
    HANDLE hLsaConnection,
    PSTR pszGroup
    );

DWORD
TestEnumUsers(
    HANDLE hLsaConnection,
    PSTR pszUser
    );
#endif

static
DWORD
BuildUserInfo(
    uid_t uid,
    gid_t gid,
    PCSTR pszLoginId,
    PCSTR pszShell,
    PCSTR pszHomedir,
    PLSA_USER_INFO_0* ppUserInfo
    );

static
DWORD
BuildGroupInfo(
    gid_t gid,
    PCSTR pszGroupName,
    PLSA_GROUP_INFO_1* ppGroupInfo
    );

static
DWORD
GetGroupId(
    HANDLE hLsaConnection,
    PCSTR  pszGroupName,
    gid_t* pGid
    );

void
GetRandomName(
    PSTR pszGroup, 
    PSTR pszUser, 
    int len
    );

static
DWORD
TestDelGroup(
    HANDLE hLsaConnection,
    PSTR pszGroup
    );

static
DWORD
TestDelUser(
    HANDLE hLsaConnection,
    PSTR pszUser
    );

DWORD
Lwt_LsaTestLocalProvider(
    HANDLE hLsaConnection
    );

int 
test_local_provider_main(
    int argc, 
    char *argv[]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    HANDLE hLsaConnection = NULL;
    PTESTDATA pTestData = NULL;

    dwError = Lwt_LsaTestSetup(argc, argv, &hLsaConnection, &pTestData);
    if ( dwError )
        goto error;

    dwError = Lwt_LsaTestLocalProvider(hLsaConnection);
    if ( dwError )
        goto error;

cleanup:

    Lwt_LsaTestTeardown(&hLsaConnection, &pTestData);

    return LwtMapErrorToProgramStatus(dwError);

error:
    goto cleanup;

}

DWORD
Lwt_LsaTestLocalProvider(
    HANDLE hLsaConnection
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    char szGroup[10]="";
    char szUser[10] = "";
    
    GetRandomName(szGroup, szUser, 8);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_TEST_BROKE(dwError);
    
    dwError = TestAddGroup( hLsaConnection, szGroup);
    BAIL_ON_TEST_BROKE(dwError);
    
//    dwError = TestEnumGroups( hLsaConnection, szGroup);
//    BAIL_ON_TEST_BROKE(dwError);

    dwError = TestDelGroup( hLsaConnection, szGroup);
    BAIL_ON_TEST_BROKE(dwError);

    dwError = TestAddUser( hLsaConnection, szUser);
    BAIL_ON_TEST_BROKE(dwError);

//    dwError = TestEnumUsers( hLsaConnection, szUser);
//    BAIL_ON_TEST_BROKE(dwError);

    dwError = TestDelUser( hLsaConnection, szUser);
    BAIL_ON_TEST_BROKE(dwError);
    
cleanup:

    if (hLsaConnection != (HANDLE)NULL) 
    {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:
    goto cleanup;
}

static
DWORD
BuildUserInfo(
    uid_t uid,
    gid_t gid,
    PCSTR pszLoginId,
    PCSTR pszShell,
    PCSTR pszHomedir,
    PLSA_USER_INFO_0* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;

    dwError = LwAllocateMemory(
                   sizeof(LSA_USER_INFO_0),
                   (PVOID*)&pUserInfo
                   );
    BAIL_ON_TEST_BROKE(dwError);

    pUserInfo->uid = uid;
    pUserInfo->gid = gid;

    dwError = LwAllocateString(pszLoginId, &pUserInfo->pszName);
    BAIL_ON_TEST_BROKE(dwError);

    dwError = LwAllocateString(pszShell, &pUserInfo->pszShell);
    BAIL_ON_TEST_BROKE(dwError);

    dwError = LwAllocateString(pszHomedir, &pUserInfo->pszHomedir);
    BAIL_ON_TEST_BROKE(dwError);

    // TODO: Gecos

    *ppUserInfo = pUserInfo;

cleanup:

    return dwError;

error:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    goto cleanup;
}

static
DWORD
BuildGroupInfo(
    gid_t gid,
    PCSTR pszGroupName,
    PLSA_GROUP_INFO_1* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 1;

    dwError = LwAllocateMemory(
                    sizeof(LSA_GROUP_INFO_1),
                    (PVOID*)&pGroupInfo);
    BAIL_ON_TEST_BROKE(dwError);

    pGroupInfo->gid = gid;

    dwError = LwAllocateString(pszGroupName, &pGroupInfo->pszName);
    BAIL_ON_TEST_BROKE(dwError);

    *ppGroupInfo = pGroupInfo;

cleanup:

    return dwError;

error:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    *ppGroupInfo = NULL;

    goto cleanup;
}

static
DWORD
GetGroupId(
    HANDLE hLsaConnection,
    PCSTR  pszGroupName,
    gid_t* pGid
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 0;
    gid_t gid = 0;

    dwError = LsaFindGroupByName(
                    hLsaConnection,
                    pszGroupName,
                    0,
                    dwGroupInfoLevel,
                    &pGroupInfo);
    BAIL_ON_TEST_BROKE(dwError);

    switch(dwGroupInfoLevel)
    {
        case 0:
        {
            gid = ((PLSA_GROUP_INFO_0)pGroupInfo)->gid;
            break;
        }
        default:
        {
            dwError = LW_ERROR_INVALID_GROUP_INFO_LEVEL;
            BAIL_ON_TEST_BROKE(dwError);
            break;
        }
    }

    *pGid = gid;

cleanup:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    return dwError;

error:

    *pGid = 0;

    goto cleanup;
}

void
GetRandomName(
    PSTR pszGroup, 
    PSTR pszUser, 
    int len
    )
{
    int i = 0;
    int j = 0;

    srand(time(NULL));
    
    j = 7;
    for(i = 0; i < 8; i++)
    {
        int num = (rand() % 91);
        num = 65 + (num % 25);
        pszGroup[i] = num;
        pszUser[j-i] = num;
    }
}

static
DWORD
TestAddGroup(
    HANDLE hLsaConnection,
    PSTR pszGroup
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwGroupInfoLevel = 1;
    PVOID pGroupInfo = NULL;
    LSA_FIND_FLAGS FindFlags = 0;

    PCSTR pszTestDescription = 
        "Verify LsaAddGroup adds a group";
    PCSTR pszTestAPIs = 
        "LsaAddGroup";
    
    char szTestMsg[512] = { 0 };

    dwError = BuildGroupInfo(0, pszGroup, (PLSA_GROUP_INFO_1*)&pGroupInfo);
    BAIL_ON_TEST_BROKE(dwError);

    dwError = LsaAddGroup(hLsaConnection, pGroupInfo, dwGroupInfoLevel);
    BAIL_ON_TEST_BROKE(dwError);

    dwError = LsaFindGroupByName(
                    hLsaConnection,
                    pszGroup,
                    FindFlags,
                    dwGroupInfoLevel,
                    &pGroupInfo);
    BAIL_ON_TEST_BROKE(dwError);
    
    if( !pGroupInfo )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "unexpected result while adding the group %s",
            pszGroup);
        LWT_LOG_TEST(szTestMsg);
    }
cleanup:
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    return dwError;
error:
    goto cleanup;
}

static
DWORD
TestAddUser(
    HANDLE hLsaConnection,
    PSTR pszUser
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwUserInfoLevel = 0;
    PVOID pUserInfo = NULL;
    PSTR pszShell = "/bin/sh";
    PSTR pszHomedir = "/home";
    gid_t gid = 0;
    LSA_FIND_FLAGS FindFlags = 0;

    PCSTR pszTestDescription = 
        "Verify LsaAddUser adds a user";
    PCSTR pszTestAPIs = 
        "LsaAddUser";
    
    char szTestMsg[512] = { 0 };
    
    dwError = GetGroupId( hLsaConnection, pszUser, &gid);
    BAIL_ON_TEST_BROKE(dwError);

    dwError = BuildUserInfo(0, gid, pszUser, pszShell, pszHomedir, (PLSA_USER_INFO_0*)&pUserInfo);
    BAIL_ON_TEST_BROKE(dwError);

    dwError = LsaAddUser(
                 hLsaConnection,
                 pUserInfo,
                 dwUserInfoLevel);
    BAIL_ON_TEST_BROKE(dwError);

    dwError = LsaFindUserByName(
                    hLsaConnection,
                    pszUser,
                    FindFlags,
                    &pUserInfo);
    BAIL_ON_TEST_BROKE(dwError);
    
    if( !pUserInfo )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "unexpected result while adding the user %s",
            pszUser);
        LWT_LOG_TEST(szTestMsg);
    }

cleanup:
    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }
    return dwError;
error:
    goto cleanup;
}

#if 0
DWORD
TestEnumGroups(
    HANDLE hLsaConnection,
    PSTR pszGroup
    )
{
    DWORD dwError = 0;
    DWORD dwGroupInfoLevel = 0;
    DWORD dwBatchSize = 10;
    HANDLE hLsaConnection = (HANDLE)NULL;
    HANDLE hResume = (HANDLE)NULL;
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwNumGroupsFound = 0;
    DWORD  dwTotalGroupsFound = 0;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = FALSE;
    BOOLEAN bCheckGroupMembersOnline = FALSE;

dwError = LsaBeginEnumGroupsWithCheckOnlineOption(
                    hLsaConnection,
                    dwGroupInfoLevel,
                    dwBatchSize,
                    bCheckGroupMembersOnline,
                    0,
                    &hResume);
    BAIL_ON_LSA_ERROR(dwError);

    do
    {
        DWORD iGroup = 0;

        if (ppGroupInfoList) {
            LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
            ppGroupInfoList = NULL;
        }

        dwError = LsaEnumGroups(
                    hLsaConnection,
                    hResume,
                    &dwNumGroupsFound,
                    &ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        if (!dwNumGroupsFound) {
            break;
        }

        dwTotalGroupsFound+=dwNumGroupsFound;
        
        for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
        {
            PVOID pGroupInfo = *(ppGroupInfoList + iGroup);
       
            switch(dwGroupInfoLevel)
            {
                case 0:
                    PrintGroupInfo_0((PLSA_GROUP_INFO_0)pGroupInfo);
                    break;
                case 1:
                    PrintGroupInfo_1((PLSA_GROUP_INFO_1)pGroupInfo);
                    break;
                default:
                    break;
            }
        }
    } while (dwNumGroupsFound);
}

DWORD
TestEnumUsers(
    HANDLE hLsaConnection,
    PSTR pszUser
    )
{
    
}

#endif

static
DWORD
TestDelGroup(
    HANDLE hLsaConnection,
    PSTR pszGroup
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwGroupInfoLevel = 1;
    PVOID pGroupInfo = NULL;
    LSA_FIND_FLAGS FindFlags = 0;

    PCSTR pszTestDescription = 
        "Verify LsaDeleteGroupByName deletes a group";
    PCSTR pszTestAPIs = 
        "LsaDeleteGroupByName";
    
    char szTestMsg[512] = { 0 };

    dwError = LsaDeleteGroupByName(
                    hLsaConnection,
                    pszGroup);
    BAIL_ON_TEST_BROKE(dwError);
    
    dwError = LsaFindGroupByName(
                    hLsaConnection,
                    pszGroup,
                    FindFlags,
                    dwGroupInfoLevel,
                    &pGroupInfo);
    BAIL_ON_TEST_BROKE(dwError);
    
    if( pGroupInfo )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "unexpected result while deleting the group %s",
            pszGroup);
        LWT_LOG_TEST(szTestMsg);
    }

cleanup:

    return dwError;

error:
    goto cleanup;
}

static
DWORD
TestDelUser(
    HANDLE hLsaConnection,
    PSTR pszUser
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LSA_FIND_FLAGS FindFlags = 0;
    PVOID pUserInfo = NULL;

    PCSTR pszTestDescription = 
        "Verify LsaDeleteUserByName adds a user";
    PCSTR pszTestAPIs = 
        "LsaDeleteUserByName";
    
    char szTestMsg[512] = { 0 };

    dwError = LsaDeleteUserByName(
                    hLsaConnection,
                    pszUser);
    BAIL_ON_TEST_BROKE(dwError);
    
    dwError = LsaFindUserByName(
                    hLsaConnection,
                    pszUser,
                    FindFlags,
                    &pUserInfo);
    BAIL_ON_TEST_BROKE(dwError);
    
    if( pUserInfo )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "unexpected result while deleting the user %s",
            pszUser);
        LWT_LOG_TEST(szTestMsg);
    }

cleanup:

    return dwError;

error:
    goto cleanup;
}

