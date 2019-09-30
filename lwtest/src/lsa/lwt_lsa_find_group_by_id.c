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
 

#include "includes.h"

/*
 *
 * Finds the group information based on the group id.
 *
 */
static
DWORD 
Lwt_LsaFindGroupById(
    HANDLE hLSAConnection,
    PTESTDATA pTestData,
    DWORD dwLevel
    );


/*
 * ValidateForInvalidParams
 * 
 * Function validates the API for Invalid function parameters.
 * 
 */
static
DWORD 
ValidateForInvalidParams(
    HANDLE hLSAConnection,
    PTESTDATA pTestData
    );

/*
 * Validates Csv file information with the group info list
 */
static
DWORD 
Lwt_LsaValidateGroupInfo(
    PLSA_GROUP_INFO_0 ppGrpInfo,
    PLWTGROUP pGroup 
    );



static
DWORD
CheckAPIForInvalidData(
    HANDLE hLsaConnection,
    PLWTFAILDATA pInvalidData
    );



int 
find_group_by_id_main(
    int argc, 
    char *argv[]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwIndex = 0;
    HANDLE hLSAConnection = NULL;
    PTESTDATA pTestData = NULL;

    dwError = Lwt_LsaTestSetup(
        argc,
        argv,
        &hLSAConnection,
        &pTestData);

    BAIL_ON_LWT_ERROR(dwError);

    for (dwIndex = 0; dwIndex < 2; dwIndex++)
    {
        dwLocalError = Lwt_LsaFindGroupById(
            hLSAConnection,
            pTestData,
            dwIndex);

        BAIL_ON_TEST_BROKE(dwLocalError);    
    }

    dwError = ValidateForInvalidParams(hLSAConnection, pTestData);

cleanup:
    Lwt_LsaTestTeardown(&hLSAConnection, &pTestData);
    return dwError;

error:
    goto cleanup;

}



/*
 * Higher level api to query the user info by ID
 *
 */
static
DWORD 
Lwt_LsaFindGroupById(
    HANDLE hLSAConnection,
    PTESTDATA pTestData,
    DWORD dwLevel
    )
{
    
    size_t nGroups = 0;
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    PVOID pGroupInfo = NULL;
    PCSTR pszTestDescription = "Finding Groups by Id";
    PCSTR pszTestAPIs = "LsaFindGroupById";
    char pszTestMsg[256] = {0};
    
    for (nGroups = 0; nGroups != pTestData->dwNumGroups; nGroups++)
    {
        PLWTGROUP pGroup = NULL;
       
        dwError = GetGroup(pTestData, nGroups, &pGroup);
        BAIL_ON_LWT_ERROR(dwError);

        if (pGroup && pGroup->nGid > 0)
        {
            snprintf(
                pszTestMsg,
                sizeof(pszTestMsg),
                "Looking for Group Gid: %lu",
                (unsigned long)pGroup->nGid);

            dwError = LsaFindGroupById(
                hLSAConnection,
                pGroup->nGid,
                0,
                dwLevel,
                &pGroupInfo);
            if (dwError != LW_ERROR_SUCCESS)
            {
                LWT_LOG_TEST(pszTestMsg);
            }
            else
            {
                if (pGroupInfo)
                {
                    dwLocalError = Lwt_LsaValidateGroupInfo(
                        pGroupInfo, 
                        pGroup);
                    BAIL_ON_TEST_BROKE(dwLocalError);    

                    LsaFreeGroupInfo(dwLevel, pGroupInfo);
                    pGroupInfo = NULL;
                }
            }
        }
        FreeGroup(&pGroup);
        pGroup = NULL;
    }
cleanup:
    return dwError;

error:
    if (pGroupInfo)
    {
        LsaFreeGroupInfo(
            dwLevel, 
            pGroupInfo);
        pGroupInfo = NULL;
    }
    goto cleanup;
}




/*
 * validates the information of the Group info returned wiht the user info
 * retrned from the csv file
 *
 */
static 
DWORD  
Lwt_LsaValidateGroupInfo(
    PLSA_GROUP_INFO_0 pGroupInfo, 
    PLWTGROUP pGroup
    )
{

    DWORD dwError = LW_ERROR_SUCCESS;
    PCSTR pszTestDescription = "Validating the Group information with the CSV file information";
    PCSTR pszTestAPIs = "LsaFindGroupById";
    char pszTestMsg[256] = {0};

    if (!StringsAreEqual(pGroup->pszSid, pGroupInfo->pszSid))
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(
            pszTestMsg, 
            sizeof(pszTestMsg), 
            "SID not matching , \nCSV sid:%sm AD sid:%s",
            pGroup->pszSid, 
            pGroupInfo->pszSid);

        BAIL_ON_TEST_BROKE(dwError);
    }
cleanup:
    LWT_LOG_TEST(pszTestMsg);
    return dwError;

error:
    goto cleanup;
}


/*
 * ValidateForInvalidParams
 * 
 * Function validates the API for Invalid function parameters.
 * 
 */
static
DWORD 
ValidateForInvalidParams(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_TEST_SKIPPED;
    DWORD dwTest = 0;
    PLWTFAILDATA pInvalidData = NULL;
    
    if ( !pTestData || !pTestData->pInvalidDataIface )
    {
        dwLocalError = LW_ERROR_TEST_SKIPPED;
        BAIL_ON_TEST_BROKE(dwLocalError);
    }
    for ( dwTest = 0; dwTest < pTestData->dwNumInvalidDataSet; dwTest++ )
    {
        dwLocalError = GetInvalidDataRecord(
            pTestData,
            dwTest, 
            &pInvalidData);
        BAIL_ON_TEST_BROKE(dwLocalError);

        dwError = CheckAPIForInvalidData(
            hLsaConnection, 
            pInvalidData);

        /* Frees the invalid data record*/
        FreeInvalidDataRecord(pInvalidData);
    }

error:
    return dwError;
}



static
DWORD
CheckAPIForInvalidData(
    HANDLE hLsaConnection,
    PLWTFAILDATA pInvalidData)
{

    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    CHAR  szTestMsg[256] = { 0 };
    PVOID pGroupInfo = NULL;
    PCSTR pszTestAPIs = "LsaFindGroupById";
    PCSTR pszTestDescription = 
            "API returns correct error codes for invalid data";

    if (pInvalidData->Field == LWTGID_INVALID)
    {
        dwLocalError = LsaFindGroupById(
            hLsaConnection,
            pInvalidData->nGid,
            0,
            pInvalidData->dwLevel,
            &pGroupInfo);

        if (dwLocalError != pInvalidData->dwErrorCode)
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(
                szTestMsg, 
                sizeof(szTestMsg), 
                "API returned with error code (%lu) for invalid group id (%lu) parameter",
                (unsigned long)dwLocalError,
                (unsigned long)pInvalidData->nGid);
                LWT_LOG_TEST(szTestMsg);
        }
    }
    if (pInvalidData->Field == LWTGROUPINFOLEVEL_INVALID)
    {
        dwLocalError = LsaFindGroupById(
            hLsaConnection,
            pInvalidData->nGid,
            0,
            pInvalidData->dwLevel,
            &pGroupInfo);
        if (dwLocalError != pInvalidData->dwErrorCode)
        {
            dwError = LW_ERROR_TEST_FAILED;
            snprintf(
                szTestMsg, 
                sizeof(szTestMsg), 
                "API returned with error code (%lu) for invalid group level(%lu) parameter",
                (unsigned long)dwLocalError,
                (unsigned long)pInvalidData->dwLevel);
                LWT_LOG_TEST(szTestMsg);
        }
    }
    if (pGroupInfo)
    {
        LsaFreeGroupInfo(pInvalidData->dwLevel, pGroupInfo);
        pGroupInfo = NULL;
    }
    return dwError;
}
