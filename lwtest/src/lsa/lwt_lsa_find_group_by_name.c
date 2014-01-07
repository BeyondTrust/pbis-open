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
 * lwt_lsa_find_group_by_name <Config File>
 *
 * Verify that information returned by AD matches the CSV file.
 *
 *
 */


#include "includes.h"

void PrepareString(PSTR);
/*
 *
 * Verify group info is same  
 * in levels 0 & 1
 *
 */

DWORD
Lwt_LsaVerifyGroupInfoByName(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );
/*
 *
 * Verify group info 
 *
 *
 */
static
DWORD 
VerifyGroupInfo(
    HANDLE hLsaConnection, 
    PLWTGROUP pGroup 
    );

/*
 *
 * Verify group info
 * against csv in level 0
 *
 */
static
DWORD
VerifyGroupInfo0(
    PLWTGROUP pGroup, 
    PLSA_GROUP_INFO_0 pGroupInfo0
    );
/*
 *
 * Verify group info
 * against csv in level 1
 *
 */
static
DWORD
VerifyGroupInfo1(
    PLWTGROUP pGroup, 
    PLSA_GROUP_INFO_1 pGroupInfo1
    );

/*
 *
 * Comapare Group info in 
 * different levels
 *
 */
static
DWORD
CompareInfo(
    PLSA_GROUP_INFO_0 pGroupInfo0,
    PLSA_GROUP_INFO_1 pGroupInfo1, 
    PSTR pszGroupName
    );


/*
* Verify Group Info API for invalid parameters
*/
static
DWORD
Lwt_LsaVerifyGroupInfoByNameForInvalidParams(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );



static
DWORD
CheckAPIForInvalidData(
    HANDLE hLsaConnection,
    PLWTFAILDATA pLwtFailData
    );



/*
 *
 * find_group_by_name_main
 *
 */

int 
find_group_by_name_main(
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

    dwError = Lwt_LsaVerifyGroupInfoByName(hLsaConnection, pTestData);
    if(dwError)
        goto error;
    
    dwError = Lwt_LsaVerifyGroupInfoByNameForInvalidParams(
        hLsaConnection,
        pTestData);

cleanup:
    Lwt_LsaTestTeardown(&hLsaConnection, &pTestData);
    return LwtMapErrorToProgramStatus(dwError);

error:
    goto cleanup;

}

/*
 *
 * Verify group info is same  
 * in levels 0 & 1
 *
 */

DWORD
Lwt_LsaVerifyGroupInfoByName(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    size_t nCurrentGroup;
    PLWTGROUP pGroup = NULL;

    if ( !pTestData )
    {
        dwError = LW_ERROR_TEST_SKIPPED;
        goto error;
    }

    /* For each user (line), verify the information is correct. */
    for ( nCurrentGroup = 0; 
          nCurrentGroup < pTestData->dwNumGroups; 
          nCurrentGroup++)
    {
        dwLocalError = GetGroup(pTestData, nCurrentGroup, &pGroup);
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwLocalError = VerifyGroupInfo(hLsaConnection, pGroup);
        BAIL_ON_TEST_BROKE(dwLocalError);

        FreeGroup(&pGroup);
    }

cleanup:
    return dwError;

error:
    FreeGroup(&pGroup);
    goto cleanup;
}


/*
 *
 * Verify group info 
 * in different levels
 *
 */
static
DWORD 
VerifyGroupInfo(
    HANDLE hLsaConnection, 
    PLWTGROUP pGroup
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PVOID pGroupInfo[2] = {NULL, NULL};
    size_t nLevel = 0;

    char szTestMsg[512] = { 0 };
    PCSTR pszTestDescription = 
        "Verify LsaFindGroupByName gives out the expected information";
    PCSTR pszTestAPIs = 
        "LsaFindGroupByName,"
        "LsaFreeGroupInfo";
    
    for ( nLevel = 0; nLevel < 2; nLevel++ ) 
    {
        dwLocalError = LsaFindGroupByName(
            hLsaConnection,
            pGroup->pszName,
            0,
            nLevel,
            &pGroupInfo[nLevel]);

        if ( dwLocalError != LW_ERROR_SUCCESS )
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(szTestMsg, sizeof(szTestMsg), 
                "unexpected result for the user %s in GroupInfo level %lu",
                pGroup->pszName,
                (unsigned long)nLevel);
            LWT_LOG_TEST(szTestMsg);
            goto error;
        }

        switch(nLevel)
        {
            case 0:
                dwLocalError = VerifyGroupInfo0(pGroup, pGroupInfo[0]);
                break;
            case 1:
             dwLocalError = VerifyGroupInfo1(pGroup, pGroupInfo[1]);
                break;
        }

        if ( dwLocalError != LW_ERROR_SUCCESS )
        {
            dwLocalError = LW_ERROR_TEST_FAILED;
            BAIL_ON_TEST_BROKE(dwLocalError);
        }
    }
    dwLocalError = CompareInfo(pGroupInfo[0], pGroupInfo[1], pGroup->pszName);
    BAIL_ON_TEST_BROKE(dwLocalError);

cleanup:
    for ( nLevel = 0; nLevel < 2; nLevel++ )
    {
        if ( pGroupInfo[nLevel] )
        {
            LsaFreeGroupInfo(nLevel, pGroupInfo[nLevel]);
        }
    }
    return dwError;

error:
    goto cleanup;
}


/*
 *
 * Verify group info
 * against csv in level 0
 *
 */

static
DWORD
VerifyGroupInfo0(
    PLWTGROUP pGroup,
    PLSA_GROUP_INFO_0 pGroupInfo0)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    char szTestMsg[512] = { 0 };
    PCSTR pszTestDescription = 
        "Verify LSA_GROUP_INFO_0 retrieved using LsaFindGroupByName has the same value as in CSV file";
    PCSTR pszTestAPIs = 
        "LsaFindGroupByName,"
        "LsaFreeGroupInfo";
    

    PrepareString(pGroup->pszName);
    snprintf(
        szTestMsg, 
        sizeof(szTestMsg), 
        "Got inconsistant information in the group %s for the fields: ", 
        pGroup->pszName);
    
    if ( !IsNullOrEmpty(pGroupInfo0->pszName) && 
        strcasecmp(pGroupInfo0->pszName, pGroup->pszName) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        Lwt_strcat(szTestMsg, sizeof(szTestMsg), "pszName, ");
    }
 
    if ( !IsNullOrEmpty(pGroupInfo0->pszSid) && 
        strcasecmp(pGroupInfo0->pszSid, pGroup->pszSid) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        Lwt_strcat(szTestMsg, sizeof(szTestMsg), "pszSid, ");
    }

    LWT_LOG_TEST(szTestMsg);
    return dwError;
}

/*
 *
 * Verify group info
 * against csv in level 1
 *
 */
static
DWORD
VerifyGroupInfo1(
    PLWTGROUP pGroup,
    PLSA_GROUP_INFO_1 pGroupInfo1)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;

    char szTestMsg[128] = { 0 };
    PCSTR pszTestDescription = 
        "Verify LSA_GROUP_INFO_1 retrieved using LsaFindGroupByName \
        has the same value as in CSV file";
    PCSTR pszTestAPIs = 
        "LsaFindGroupByName,"
        "LsaFreeGroupInfo";
    
    snprintf(
        szTestMsg, 
        sizeof(szTestMsg), 
        "Got inconsistant information in the group %s for the fields: ", 
        pGroup->pszName);

    dwLocalError = VerifyGroupInfo0(
        pGroup,
        (PLSA_GROUP_INFO_0)pGroupInfo1);
    BAIL_ON_TEST_BROKE(dwLocalError);

    if ( !IsNullOrEmpty(pGroupInfo1->pszDN) && 
        strcasecmp(pGroupInfo1->pszDN, pGroup->pszDN) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        Lwt_strcat(szTestMsg, sizeof(szTestMsg), "pszDN, ");
    }
 
    if ( !IsNullOrEmpty(pGroupInfo1->pszPasswd) && 
        strcasecmp(pGroupInfo1->pszPasswd, pGroup->pszPassword) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        Lwt_strcat(szTestMsg, sizeof(szTestMsg), "pszPasswd, ");
    }

cleanup:
    LWT_LOG_TEST(szTestMsg);
    return dwError;
error:
    goto cleanup;
}


/*
 *
 * Comapare Group info in 
 * different levels
 *
 */
static
DWORD
CompareInfo(
    PLSA_GROUP_INFO_0 pGroupInfo0,
    PLSA_GROUP_INFO_1 pGroupInfo1,
    PSTR pszGroupName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PCSTR pszTestDescription = 
        "Verify LSA_GROUPINFO_0 and LSA_GROUPINFO_1 retrieved using LsaFindGroupByName have same values";
    PCSTR pszTestAPIs = "LsaFindGroupByName";
 
    char szTestMsg[128] = { 0 };

    snprintf(
        szTestMsg, 
        sizeof(szTestMsg), 
        "Got inconsistant information in the group %s for the fields:",
        pszGroupName);

    if ( (pGroupInfo0 && !pGroupInfo1) || (!pGroupInfo0 && pGroupInfo1) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(
            szTestMsg, 
            sizeof(szTestMsg), 
            "could'nt get groupinfo for level %s\n", 
            (!pGroupInfo0 ? "0" : (!pGroupInfo1 ? "1" : "0 & 1")));
        goto error;
    }

    if ( !IsNullOrEmpty(pGroupInfo0->pszName) && 
        !IsNullOrEmpty(pGroupInfo1->pszName ) ) /* both are non empty */
    {
        if (strcasecmp(pGroupInfo0->pszName, pGroupInfo1->pszName) )
        {
            dwError = LW_ERROR_TEST_FAILED;
            Lwt_strcat(szTestMsg, sizeof(szTestMsg), "pszName, ");
        }
    }
    else if ( !(IsNullOrEmpty(pGroupInfo0->pszName) && 
        IsNullOrEmpty(pGroupInfo1->pszName)) ) /* if one of them is empty */
    {
        dwError = LW_ERROR_TEST_FAILED;
        Lwt_strcat(szTestMsg, sizeof(szTestMsg), "pszName, ");
    }

    if(pGroupInfo0->gid != pGroupInfo1->gid)
    {
        dwError = LW_ERROR_TEST_FAILED;
        Lwt_strcat(szTestMsg, sizeof(szTestMsg), "gid, ");
    }

    if ( !IsNullOrEmpty(pGroupInfo0->pszSid) && 
        !IsNullOrEmpty(pGroupInfo1->pszSid )) /* both are non empty */
    {
        if ( strcasecmp(pGroupInfo0->pszSid, pGroupInfo1->pszSid) )
        {
            dwError = LW_ERROR_TEST_FAILED;
            Lwt_strcat(szTestMsg, sizeof(szTestMsg), "pszSid, ");
        }
    }
    else if ( !(IsNullOrEmpty(pGroupInfo0->pszSid) && 
        IsNullOrEmpty(pGroupInfo1->pszSid)) ) /* if one of them is empty*/
    {
        dwError = LW_ERROR_TEST_FAILED;
        Lwt_strcat(szTestMsg, sizeof(szTestMsg), "pszSid, ");
    }
cleanup:
    
    LWT_LOG_TEST(szTestMsg);
    return dwError;

error:
    goto cleanup;
}


static
DWORD
Lwt_LsaVerifyGroupInfoByNameForInvalidParams(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLWTFAILDATA pLwtFailData = NULL;
    DWORD dwFailRecordCount = 0;

    if (!pTestData || !pTestData->pInvalidDataIface)
    {
        dwError = LW_ERROR_TEST_SKIPPED;
        BAIL_ON_TEST_BROKE(dwError);
    }

    for ( dwFailRecordCount = 0; dwFailRecordCount < pTestData->dwNumInvalidDataSet;
          dwFailRecordCount++)
    {
        dwError = GetInvalidDataRecord(
            pTestData,
            dwFailRecordCount,
            &pLwtFailData);
        BAIL_ON_LWT_ERROR(dwError);

        dwError = CheckAPIForInvalidData(
            hLsaConnection,
            pLwtFailData);

        FreeInvalidDataRecord(pLwtFailData);
    }

cleanup:
    return dwError;

error :
    FreeInvalidDataRecord(pLwtFailData);
    goto cleanup;
}


/*
* API for checking invalid error
*
*/
static
DWORD
CheckAPIForInvalidData(
    HANDLE hLsaConnection,
    PLWTFAILDATA pLwtFailData)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVOID pGroupInfo = NULL;
    
    char szTestMsg[256] = { 0 };
    PCSTR pszTestDescription = 
        "Verify LsaFindGroupInfo parametre for invalid APIs";
    PCSTR pszTestAPIs = 
        "LsaFindGroupByName,"
        "LsaFreeGroupInfo";
    
    
    if (pLwtFailData->Field == LWTGROUP_INVALID)
    {
        dwError = LsaFindGroupByName(
            hLsaConnection,
            pLwtFailData->pszGroupName,
            0,
            pLwtFailData->dwLevel,
            &pGroupInfo);

        /* Check whether it returned invalid error code*/
        if (dwError != pLwtFailData->dwErrorCode)
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(
                szTestMsg,
                sizeof(szTestMsg),
                "Returned error code %lu for invalid group name: %s",
                (unsigned long)dwError,
                pLwtFailData->pszGroupName);
            /*Log Test Failure*/
            LWT_LOG_TEST(szTestMsg);
        }
    }
    
    if (pLwtFailData->Field == LWTGROUPINFOLEVEL_INVALID)
    {
        dwError = LsaFindGroupByName(
            hLsaConnection,
            pLwtFailData->pszGroupName,
            0,
            pLwtFailData->dwLevel,
            &pGroupInfo);
        
        if (dwError != pLwtFailData->dwErrorCode)
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(
                szTestMsg,
                sizeof(szTestMsg),
                "Returned error code %lu for invalid group info level %lu for group: %s",
                (unsigned long)dwError,
                (unsigned long)pLwtFailData->dwLevel,
                pLwtFailData->pszGroupName);

            LWT_LOG_TEST(szTestMsg);
        }
    }
    if (pGroupInfo)
    {
        LsaFreeGroupInfo(pLwtFailData->dwLevel, pGroupInfo);
        pGroupInfo = NULL;
    }
    return dwError;
}




void PrepareString(PSTR pszStr)
{
    int nLen = 0;
    int nCount = 0;
    
    nLen = strlen(pszStr);

    while(nCount < nLen)
    {
        if(pszStr[nCount] == ' ')
            pszStr[nCount] = '^'; 

        nCount++;
    }
}
