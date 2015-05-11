/*
 *   Copyright Likewise Software    2009
 *   All rights reserved.
 *  
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *   
 *    This program is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *    for more details.  You should have received a copy of the GNU General
 *    Public License along with this program.  If not, see
 *    <http://www.gnu.org/licenses/>.
 *    
 *    LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 *    TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 *    WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 *    TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 *    GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 *    HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 *    TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 *    license@likewisesoftware.com
 */
 

 /*                                                              *
 *   Verify that information returned in two group level api     *
 *   calls are coorect                                                            
 *                                                               *  
 */
#include "includes.h"


/*
 * Frees Group Info structure
 */
static 
VOID
FreeGroups(
    PVOID *ppGroupInfo, 
    DWORD dwMaxGroup, 
    int Level
    );

/*
 *
 * Compares the common properties of a group with Level 0
 * This is only done if the same group exists in Level 1 also
 *
 */
static 
DWORD 
Lwt_LsaCompareGroupsInfo(
    PLSA_GROUP_INFO_0 *ppGrpInfoList_0,
    PLSA_GROUP_INFO_1 *ppGrpInfoList_1,
    DWORD dwMaxGrp_0,
    DWORD dwMaxGrp_1
    );


/*
 *
 * Validates Csv file information with the group info list
 */
static
DWORD 
Lwt_LsaValidateGroupInfo(
    PTESTDATA pTestData, 
    PLSA_GROUP_INFO_0 *ppGrpInfo, 
    DWORD dwMaxGroup
    );

/*
* Handle invalid parameter for APIS Enumeration
*
*/
static
DWORD
ValidateEnumGroupForInvalidData(
    HANDLE hLsaConnecton,
    PTESTDATA pTestData
    );


/*
* API for checking invalid error
*
*/
static
DWORD
CheckAPIForInvalidData(
    HANDLE hLsaConnection,
    PLWTFAILDATA pLwtFailData
    );


/*
 *
 *
 *
 */
static
DWORD
Lwt_LsaEnumerateGroups(
    HANDLE hLSAConnection,
    PTESTDATA pTestData,
    BOOL bOnline
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwMaxGrp_1 = 0;
    DWORD dwMaxGrp_0 = 0;
    PLSA_GROUP_INFO_0 ppGroupInfoList_0[1000];
    PLSA_GROUP_INFO_1 ppGroupInfoList_1[1000];
    BOOL bCheckLevelConsistency = TRUE;
    

    /*Start collect the groups information*/
    dwLocalError = Lwt_LsaBeginEnumerationGroups(
        hLSAConnection, 
        0, 
        (PVOID *)ppGroupInfoList_0, 
        &dwMaxGrp_0,
        bOnline);

    if (pTestData->dwNumGroups && dwMaxGrp_0)
    {
        /* Check informations returned are consistent*/
        dwLocalError = Lwt_LsaValidateGroupInfo(
            pTestData, 
            (PLSA_GROUP_INFO_0 *)ppGroupInfoList_0, 
            dwMaxGrp_0);

        BAIL_ON_TEST_BROKE(dwLocalError);
    }
    
    /*Start collect the grups for the level 0*/
    dwLocalError = Lwt_LsaBeginEnumerationGroups(
        hLSAConnection, 
        1,
        (PVOID *)ppGroupInfoList_1, 
        &dwMaxGrp_1,
        bOnline);
    BAIL_ON_TEST_BROKE(dwLocalError);

    if (pTestData->dwNumGroups && dwMaxGrp_1)
    {
            dwLocalError = Lwt_LsaValidateGroupInfo(
                pTestData, 
                (PLSA_GROUP_INFO_0 *)ppGroupInfoList_1, 
                dwMaxGrp_1);

            BAIL_ON_TEST_BROKE(dwLocalError);
    }
   
    if (bCheckLevelConsistency)
    { 
         if (dwMaxGrp_1 && dwMaxGrp_0)
         {
            dwLocalError = Lwt_LsaCompareGroupsInfo(
                ppGroupInfoList_0,
                ppGroupInfoList_1, 
                dwMaxGrp_0, 
                dwMaxGrp_1);
            BAIL_ON_TEST_BROKE(dwLocalError);
         }
    }    

cleanup:
    if (dwMaxGrp_0)
    {
        FreeGroups((PVOID*)ppGroupInfoList_0, dwMaxGrp_0, 0);
    }
    
    if (dwMaxGrp_1)
    {
        FreeGroups((PVOID*)ppGroupInfoList_1, dwMaxGrp_1, 1);
    }
    return dwError;
    
error:
    goto cleanup;
}


/*
 *
 * This function retrieves all the Groups for different levels
 * Stores it ppGroupInfo structure for validating and other purpose
 */
int 
Lwt_LsaBeginEnumerationGroups( 
    HANDLE hLSAServer,
    DWORD dwLevel,
    PVOID* ppGroupInfo,
    DWORD* pdwMaxGroups,
    BOOL bOnline
    )   
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hResume = (HANDLE)NULL;
    PVOID *ppTempInfoList=NULL;
    DWORD dwMaxGrpCount=0;
    DWORD dwBatchLimit = 10;
    DWORD dwIndex = 0;
    char pszTestMsg[256];
    PCSTR pszTestDescription = "API test for LSABeginEnumGroups with Online and offline options";
    PCSTR pszTestAPIs = "LsaBeginEnumGroups,"
                        "LsaEnumGroups,"
                        "LsaEndEnumGroups,";
    snprintf(
        pszTestMsg, 
        sizeof(pszTestMsg), 
        "Enumerating Groups for Level:%lu and ", 
        (unsigned long)dwLevel);
    strcat (pszTestMsg, (bOnline?"online":"offline"));

    dwError = LsaBeginEnumGroupsWithCheckOnlineOption(
        hLSAServer, 
        dwLevel, 
        dwBatchLimit,
        bOnline,
        0, 
        &hResume);
    BAIL_ON_TEST_BROKE(dwError);

    do
    {
        dwError = LsaEnumGroups(
            hLSAServer,
            hResume,
            &dwMaxGrpCount,
            &ppTempInfoList);
        BAIL_ON_TEST_BROKE(dwError);
        
        for (dwIndex=0; dwIndex < dwMaxGrpCount; dwIndex++)
        {
            ppGroupInfo[*pdwMaxGroups + dwIndex]= *(ppTempInfoList + dwIndex);
        }
        *pdwMaxGroups +=dwMaxGrpCount;
        LwFreeMemory(ppTempInfoList);
    }while (dwMaxGrpCount);
    

cleanup:
    if (hLSAServer&&hResume)
    {
        LsaEndEnumGroups(hLSAServer, hResume);
        hResume = (HANDLE)NULL;
    }
    LWT_LOG_TEST(pszTestMsg);
    return (dwError);

error:
    goto cleanup;
}


static 
DWORD 
Lwt_LsaCompareGroupsInfo(
    PLSA_GROUP_INFO_0 *ppGrpInfoList_0,
    PLSA_GROUP_INFO_1 *ppGrpInfoList_1,
    DWORD dwMaxGrp_0,
    DWORD dwMaxGrp_1
    )
{
    int i=0, j =0;
    DWORD dwError = LW_ERROR_SUCCESS;
    char pszTestMsg[256];
    PCSTR pszTestDescription = "Level Consistency Check for Group members";
    PCSTR pszTestAPIs = "LsaBeginEnumGroups,"
                        "LsaEnumGroups,"
                        "LsaEndEnumGroups,"
                        "LsaFreeGroupInfo";

    for (i = 0; i < dwMaxGrp_0; i++)
    {
        for (j = 0; j < dwMaxGrp_1; j++)
        {
            if (ppGrpInfoList_0[i]->gid == ppGrpInfoList_1[j]->gid)
            {
                if (!StringsAreEqual(ppGrpInfoList_0[i]->pszName, ppGrpInfoList_1[j]->pszName))
                {
                    snprintf(
                        pszTestMsg, 
                        sizeof(pszTestMsg),
                        "Group name inconsistent for level 1 and 0 with gid:%lu %s %lu",
                        (unsigned long) ppGrpInfoList_0[i]->gid,
                        ppGrpInfoList_1[j]->pszName, 
                        (unsigned long)ppGrpInfoList_0[i]->pszName);

                    dwError = LW_ERROR_TEST_FAILED;
                    LWT_LOG_TEST(pszTestMsg);
                }
                if (!StringsAreEqual(ppGrpInfoList_0[i]->pszSid, ppGrpInfoList_1[j]->pszSid))
                {
                    snprintf(
                        pszTestMsg, 
                        sizeof(pszTestMsg), 
                        "The SID inconsitent for Leve 1 and 2 with gid: %lu %s %s", 
                        (unsigned long)ppGrpInfoList_0[i]->gid, 
                        ppGrpInfoList_1[i]->pszSid, 
                        ppGrpInfoList_0[i]->pszSid);

                    dwError = LW_ERROR_TEST_FAILED;
                    LWT_LOG_TEST(pszTestMsg);
                }
            }
        }
    }
    return dwError;
}


int 
enum_groups_main(
    int argc, 
    char *argv[]
    )
{
    HANDLE hLSAConnection = NULL;
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PTESTDATA pTestData = NULL;

    dwError = Lwt_LsaTestSetup(
        argc,
        argv,
        &hLSAConnection,
        &pTestData
        );
    BAIL_ON_LWT_ERROR(dwError);
    
    /*Check For online Groups*/
    dwLocalError = Lwt_LsaEnumerateGroups(
        hLSAConnection,
        pTestData,
        0);

    
    /*Check for offline Groups*/
    dwLocalError = Lwt_LsaEnumerateGroups(
        hLSAConnection,
        pTestData,
        1);

    dwLocalError = ValidateEnumGroupForInvalidData(
        hLSAConnection,
        pTestData);
    
    
cleanup:
   Lwt_LsaTestTeardown( 
        &hLSAConnection,
        &pTestData);
    return dwError;

error:
    goto cleanup;

}


static 
VOID 
FreeGroups(
    PVOID *ppGroupInfo, 
    DWORD dwMaxGroup, 
    int Level
    )
{
    int i = 0;
    for (i = 0;i < dwMaxGroup;i++) 
    {
        LsaFreeGroupInfo(Level, (PVOID )ppGroupInfo[i]);
    }
}



static
DWORD 
Lwt_LsaValidateGroupInfo(
    PTESTDATA pTestData, 
    PLSA_GROUP_INFO_0 *ppGrpInfo, 
    DWORD dwMaxGroup
    )
{
    size_t i= 0;
    size_t j = 0;
    DWORD dwMaxCount = 0;
    DWORD dwError = LW_ERROR_SUCCESS;
    PLWTGROUP pGroup = NULL;
    char pszTestMsg[256];
    PCSTR pszTestDescription = "Validate Groups";
    PCSTR pszTestAPIs = "LsaEnumGroups,";

     /* For each user (line), verify the information is correct. */
    dwMaxCount = pTestData->dwNumGroups;
    for (i = 0;i < dwMaxGroup; i++)
    {
        for (j=0; j < dwMaxCount; j++)
        {
            dwError = GetGroup(pTestData, j, &pGroup); 
            BAIL_ON_LWT_ERROR(dwError);

            if (pGroup)
            {
                if (ppGrpInfo[i]->gid ==  pGroup->nGid)
                {
                    /*Compare Sid information*/
                    if (!StringsAreEqual(ppGrpInfo[i]->pszSid, pGroup->pszSid))
                    {
                        dwError = LW_ERROR_TEST_FAILED;
                        snprintf(
                            pszTestMsg, 
                            sizeof(pszTestMsg), 
                            "SID for  Group:%s are inconsistent, SID from AD: %s and SID for CSV: %s",
                            ppGrpInfo[i]->pszName,  
                            ppGrpInfo[i]->pszSid, 
                            pGroup->pszSid);

                        LWT_LOG_TEST(pszTestMsg);        
                    }
                }
                FreeGroup(&pGroup);
                pGroup = NULL;
            }
        }
    }

cleanup:
    return dwError;

error:
    if (pGroup)
    {
        FreeGroup(&pGroup);
        pGroup = NULL;
    }
    goto cleanup;
}


static
DWORD
ValidateEnumGroupForInvalidData(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwFailDataIndex = 0;
    PLWTFAILDATA pLwtFailData = NULL;

    if (!pTestData || !pTestData->pInvalidDataIface)
    {
        dwError = LW_ERROR_TEST_SKIPPED;
        BAIL_ON_TEST_BROKE(dwError);
    }

    for (dwFailDataIndex = 0; dwFailDataIndex < pTestData->dwNumInvalidDataSet;
         dwFailDataIndex++)
    {
        dwError = GetInvalidDataRecord(
            pTestData,
            dwFailDataIndex,
            &pLwtFailData);
        BAIL_ON_LWT_ERROR(dwError);

        dwError = CheckAPIForInvalidData(hLsaConnection, pLwtFailData);
    }

cleanup:
    return dwError;

error:
    if (pLwtFailData)
    {
        FreeInvalidDataRecord(pLwtFailData);
    }
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
    PLWTFAILDATA pLwtFailData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hResume = (HANDLE)NULL;
    DWORD dwBatchLimit = 10;
    
    PCSTR pszTestDescription = 
        "Verify LsaEnumGroups parametre for invalid APIs";
    PCSTR pszTestAPIs = 
        "LsaBeginEnumGroups";
    
    char szTestMsg[256] = { 0 };

    if (pLwtFailData->Field == LWTMAXGROUPS_INVALID)
    {
        dwError = LsaBeginEnumGroups(
            hLsaConnection,
            pLwtFailData->dwLevel,
            pLwtFailData->dwMaxEntries,     
            0,
            &hResume);

        if (dwError != pLwtFailData->dwErrorCode)
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(
                szTestMsg,
                sizeof(szTestMsg),
                "Invalid Error Code %lu returned for invalid max group %lu to group name %s",
                (unsigned long)dwError,
                (unsigned long)pLwtFailData->dwMaxEntries,
                pLwtFailData->pszGroupName);

            LWT_LOG_TEST(szTestMsg);

        }
    }
    
    if (pLwtFailData->Field == LWTGROUPINFOLEVEL_INVALID)
    {
        dwError = LsaBeginEnumGroups(
            hLsaConnection,
            pLwtFailData->dwLevel,
            dwBatchLimit,
            0,
            &hResume);

        if (dwError != pLwtFailData->dwErrorCode)
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(
                szTestMsg,
                sizeof(szTestMsg),
                "Invalid Error Code %lu returned for invalid group info level %lu to group name %s",
                (unsigned long)dwError,
                (unsigned long)pLwtFailData->dwLevel,
                pLwtFailData->pszGroupName);
            
            LWT_LOG_TEST(szTestMsg);
        }

    }
    if (hResume)
    {
        LsaEndEnumGroups(hLsaConnection, hResume);
        hResume = NULL;
    }
    return dwError;
}


