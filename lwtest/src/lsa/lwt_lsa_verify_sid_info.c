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
 * lwt_lsa_verify_sid_info <Config File>
 *
 * Verify that information returned by AD matches the CSV file.
 *
 *
 */


#include "includes.h"

/*
 *
 * Get user/group info using SID 
 * and verify for correctness
 *
 */
DWORD
Lwt_LsaVerifySidInfo(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/*
 *
 * check for NULL arguements 
 *
 */

static
DWORD
CheckNULLArgs(
    HANDLE hLsaConnection,
    PSTR pszSid
    );

/*
 *
 * Verify LsaGetNamesbysidList
 * by passing a list of SIDs
 *
 */
static
DWORD
VerifySidList(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/*
 *
 * Add Sids to the list 
 *
 */
static
DWORD
AddSidToList(
    PSTR pszSrc, 
    int nPos,
    PSTR *ppszSidList
    );

/*
 *
 *  free SIDs from the list
 *
 */

static
DWORD
FreeSid(
    PSTR *ppszSidList,
    int nCount
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
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    );

/*
 *
 * verify_sid_info_main
 *
 */

int 
verify_sid_info_main(
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

    dwError = Lwt_LsaVerifySidInfo(hLsaConnection, pTestData);
    if(dwError)
        goto error;

cleanup:

    Lwt_LsaTestTeardown(&hLsaConnection, &pTestData);
    
    return LwtMapErrorToProgramStatus(dwError);

error:
    goto cleanup;

}

/*
 *
 * Verify Information got using SID 
 * is correct against the csv file
 *
 */

DWORD
Lwt_LsaVerifySidInfo(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;

    size_t nCount;
    PLWTUSER pUser = NULL;
    PLWTGROUP pGroup = NULL;
    PLSA_SID_INFO pSIDInfoList = NULL;

    CHAR chDomainSep;
    PCSTR pszTestDescription = 
        "verify LsaGetNamesBySidList works as expected";
    PCSTR pszTestAPIs = 
        "LsaGetNamesBySidList,"
        "LsaFreeSIDInfoList";
    
    char szTestMsg[128] = { 0 };
    if ( !pTestData )
    {
        dwError = LW_ERROR_TEST_SKIPPED;
        goto error;
    }

    /* For each user (line), verify the information is correct. */
   
    for ( nCount = 0; 
          nCount < pTestData->dwNumUsers; 
          nCount++)
    {
        dwLocalError = GetUser(pTestData, nCount, &pUser);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( !IsNullOrEmpty(pUser->pszSid) )
        {
            dwLocalError = LsaGetNamesBySidList(
                            hLsaConnection,
                            1,
                            &pUser->pszSid,
                            &pSIDInfoList, 
                            &chDomainSep);
            
            if ( dwLocalError != LW_ERROR_SUCCESS )
            {
                dwError = LW_ERROR_TEST_FAILED;

                snprintf(szTestMsg, sizeof(szTestMsg), 
                    "LsaGetNamesBySidList failed for valid SID");
                LWT_LOG_TEST(szTestMsg);
                goto error;
            }
            
            dwError = VerifySidInformation(
                        pSIDInfoList,
                        pUser->pszSamAccountName,
                        pUser->pszNetBiosName,
                        AccountType_User);

            BAIL_ON_TEST_BROKE(dwError);
        }
        
        FreeUser(&pUser);
        if ( pSIDInfoList )
        {
            LsaFreeSIDInfoList(pSIDInfoList, 1);
            pSIDInfoList = NULL;
        }
        chDomainSep = '\0';
    }

    /* For each group (line), verify the information is correct. */
   
    for ( nCount = 0; 
          nCount < pTestData->dwNumGroups; 
          nCount++)
    {
        dwLocalError = GetGroup(pTestData, nCount, &pGroup);
        BAIL_ON_TEST_BROKE(dwLocalError);
        
        if ( !IsNullOrEmpty(pGroup->pszSid) )
        {
            dwLocalError = LsaGetNamesBySidList(
                            hLsaConnection,
                            1,
                            &pGroup->pszSid,
                            &pSIDInfoList, 
                            &chDomainSep);

            if ( dwLocalError != LW_ERROR_SUCCESS )
            {
                dwError = LW_ERROR_TEST_FAILED;

                snprintf(szTestMsg, sizeof(szTestMsg), 
                    "LsaGetNamesBySidList failed for valid SID");
                LWT_LOG_TEST(szTestMsg);
                goto error;
            }
            
            dwError = VerifySidInformation(
                        pSIDInfoList,
                        pGroup->pszSamAccountName,
                        pGroup->pszNetBiosName,
                        AccountType_Group
                        );
            BAIL_ON_TEST_BROKE(dwError);
        }

        FreeGroup(&pGroup);
   
        if ( pSIDInfoList )
        {
            LsaFreeSIDInfoList(pSIDInfoList, 1);
            pSIDInfoList = NULL;
        }
        chDomainSep = '\0';
    }

    /* Verify using a list of SIDs */
    dwError = VerifySidList(hLsaConnection, pTestData);
    BAIL_ON_TEST_BROKE(dwError);

    dwError = ValidateForInvalidParams(hLsaConnection, pTestData);
    BAIL_ON_TEST_BROKE(dwError);

    if( pTestData->dwNumUsers )
    {
        dwLocalError = GetUser(pTestData, 0, &pUser);
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

    dwLocalError = CheckNULLArgs(hLsaConnection, pUser->pszSid); 
    FreeUser(&pUser);

cleanup:
    if( pSIDInfoList )
    {
        LsaFreeSIDInfoList(pSIDInfoList, 1);
        pSIDInfoList = NULL;
    }
            
    FreeUser(&pUser);
    FreeGroup(&pGroup);
    return dwError;
error:
    goto cleanup;
}

/*
 *
 * Verify LSA_SID_INFO  
 * against info in csv file
 * 
 */

DWORD
VerifySidInformation(
    PLSA_SID_INFO pSIDInfoList,
    PCSTR pszSamAccountName,
    PCSTR pszDomainName,
    DWORD dwAccountType
    )
{

    DWORD dwError = LW_ERROR_SUCCESS;
 
    PCSTR pszTestDescription = 
        "verify information in LSA_SID_INFO list retrieved from LsaGetNamesBySidList has the expected information";
    PCSTR pszTestAPIs = 
        "LsaGetNamesBySidList,"
        "LsaFreeSIDInfoList";
    
    char szTestMsg[128] = { 0 };
   
    if ( IsNullOrEmpty(pSIDInfoList->pszSamAccountName) ||
         strcmp(pSIDInfoList->pszSamAccountName, pszSamAccountName) )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "found inconsistant info for the field pszSamAccountName:%s,info in CSV :%s",
            pSIDInfoList->pszSamAccountName, pszSamAccountName);
        LWT_LOG_TEST(szTestMsg);
    }

    if ( IsNullOrEmpty(pSIDInfoList->pszDomainName) || 
        strcmp(pSIDInfoList->pszDomainName, pszDomainName) ) 
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg),
            "found inconsistant info for the field pszDomainName:%s,info in CSV:%s, SAMAccountName: %s",
            pSIDInfoList->pszDomainName, pszDomainName, pszSamAccountName);
        LWT_LOG_TEST(szTestMsg);
    }

    if ( pSIDInfoList->accountType != dwAccountType )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "AccountType Doesn't Match. Actual account type:%s, SAMAccountName: %s",
            dwAccountType == AccountType_User? 
            "AccountType_User": "AccountType_Group", pszSamAccountName);
        LWT_LOG_TEST(szTestMsg);
    }

    return dwError;
}

/*
 *
 * check for NULL arguements 
 *
 */

static
DWORD
CheckNULLArgs(
    HANDLE hLsaConnection,
    PSTR pszSid
    )
{
    
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;

    PLSA_SID_INFO pSIDInfoList = NULL;

    CHAR chDomainSep;

    PCSTR pszTestDescription = 
        "verify LsaGetNamesBySidList with NULL arguements";
    PCSTR pszTestAPIs = 
        "LsaGetNamesBySidList,"
        "LsaFreeSIDInfoList";
    
    char szTestMsg[128] = { 0 };
/*   
    dwLocalError = LsaGetNamesBySidList(             // crashes
                    NULL,
                    1,
                    &pszSid,
                    &pSIDInfoList, 
                    &chDomainSep);

    if( dwLocalError != LW_ERROR_INVALID_PARAMETER )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "LsaGetNamesBySidList failed for NULL connection handle");
        LWT_LOG_TEST(szTestMsg);
    }
*/
    dwLocalError = LsaGetNamesBySidList(
                    hLsaConnection,
                    0,
                    &pszSid,
                    &pSIDInfoList, 
                    &chDomainSep);

    if( dwLocalError != LW_ERROR_SUCCESS )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "LsaGetNamesBySidList failed for sCount=0");
        LWT_LOG_TEST(szTestMsg);
    }
/* 
   if ( pSIDInfoList )
       LsaFreeSIDInfoList(pSIDInfoList, 1);                    //crashes
   pSIDInfoList = NULL; 
*/

/*
    dwLocalError = LsaGetNamesBySidList(                       //crashes
                    hLsaConnection,
                    1,
                    NULL,
                    &pSIDInfoList, 
                    &chDomainSep);

    if( dwLocalError != LW_ERROR_INVALID_PARAMETER )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "LsaGetNamesBySidList failed for NULL SID, 3rd param");
        LWT_LOG_TEST(szTestMsg);
    }
*/
/*    dwLocalError = LsaGetNamesBySidList(                      //crashes
                    hLsaConnection,
                    1,
                    &pszSid,
                    NULL, 
                    &chDomainSep);

    if( dwLocalError != LW_ERROR_INVALID_PARAMETER )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "LsaGetNamesBySidList failed for NULL pSIDInfoList, 4th param");
        LWT_LOG_TEST(szTestMsg);
    }
*/
    dwLocalError = LsaGetNamesBySidList(
                    hLsaConnection,
                    1,
                    &pszSid,
                    &pSIDInfoList, 
                    NULL);

    if( dwLocalError != LW_ERROR_SUCCESS )
    {
        dwError = LW_ERROR_TEST_FAILED;
        snprintf(szTestMsg, sizeof(szTestMsg), 
            "LsaGetNamesBySidList failed for NULL chDomainSep, 5th param");
        LWT_LOG_TEST(szTestMsg);
    }

   if ( pSIDInfoList )
       LsaFreeSIDInfoList(pSIDInfoList, 1);
   pSIDInfoList = NULL;

   return dwError;
}

/*
 *
 * Verify LsaGetNamesbysidList
 * by passing a list of SIDs
 *
 */
static
DWORD
VerifySidList(
    HANDLE hLsaConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;

    PLSA_SID_INFO pSIDInfoList = NULL;
    PLWTUSER pUser = NULL;
    PLWTGROUP pGroup = NULL;
    PSTR *ppszUserSidList = NULL;  /* Sid List */
    PSTR *ppszGroupSidList = NULL;  /* Sid List */

    int nCount = 0;
    int nPos = 0;

    PCSTR pszTestDescription = 
        "verify LsaGetNamesBySidList with a list of SIDs";
    PCSTR pszTestAPIs = 
        "LsaGetNamesBySidList,"
        "LsaFreeSIDInfoList";
    
    char szTestMsg[128] = { 0 };

    dwError = LwAllocateMemory(
                pTestData->dwNumUsers * sizeof(PSTR),
                (PVOID)&ppszUserSidList);
    BAIL_ON_LWT_ERROR(dwError);

    for ( nCount = 0; 
          nCount < pTestData->dwNumUsers; 
          nCount++)
    {
        dwLocalError = GetUser(pTestData, nCount, &pUser);
        BAIL_ON_TEST_BROKE(dwLocalError);
     
        if ( !IsNullOrEmpty(pUser->pszSid) )
        {
            dwError = AddSidToList(pUser->pszSid, nPos, ppszUserSidList);
            BAIL_ON_TEST_BROKE(dwError);
            nPos++;
        }
        FreeUser(&pUser);
    }
            
    dwLocalError = LsaGetNamesBySidList(
                    hLsaConnection,
                    nPos,
                    ppszUserSidList,
                    &pSIDInfoList, 
                    NULL);
        
    if ( pSIDInfoList )
    {
        LsaFreeSIDInfoList(pSIDInfoList, nPos);
        pSIDInfoList = NULL;
    }
    
    if ( ppszUserSidList )
    {
        FreeSid(ppszUserSidList, nPos);
        LW_SAFE_FREE_MEMORY(ppszUserSidList);
        ppszUserSidList = NULL;
    }

    if ( dwLocalError != LW_ERROR_SUCCESS )
    {
        dwError = LW_ERROR_TEST_FAILED;

        snprintf(szTestMsg, sizeof(szTestMsg), 
            "LsaGetNamesBySidList failed for valid SID List");
        LWT_LOG_TEST(szTestMsg);
        goto error;
    }

    dwError = LwAllocateMemory(
                pTestData->dwNumGroups * sizeof(PSTR),
                (PVOID)&ppszGroupSidList);
    BAIL_ON_LWT_ERROR(dwError);

    for ( nCount = 0, nPos = 0; 
          nCount < pTestData->dwNumGroups; 
          nCount++)
    {
        dwLocalError = GetGroup(pTestData, nCount, &pGroup);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( !IsNullOrEmpty(pGroup->pszSid) )
        {
            dwError = AddSidToList(pGroup->pszSid, nPos, ppszGroupSidList);
            BAIL_ON_TEST_BROKE(dwError);
            nPos++;
        }
    }

    dwLocalError = LsaGetNamesBySidList(
                    hLsaConnection,
                    nPos,
                    ppszGroupSidList,
                    &pSIDInfoList, 
                    NULL);

    if ( pSIDInfoList )
    {
        LsaFreeSIDInfoList(pSIDInfoList, nPos);
        pSIDInfoList = NULL;
    }

    if (ppszGroupSidList)
    {
        FreeSid(ppszGroupSidList, nPos);
        LW_SAFE_FREE_MEMORY(ppszGroupSidList);
        ppszGroupSidList = NULL;
    }

    if ( dwLocalError != LW_ERROR_SUCCESS )
    {
        dwError = LW_ERROR_TEST_FAILED;

        snprintf(szTestMsg, sizeof(szTestMsg), 
            "LsaGetNamesBySidList failed for valid SID List");
        LWT_LOG_TEST(szTestMsg);
        goto error;
    }

cleanup:
    if ( pSIDInfoList )
    {
        LsaFreeSIDInfoList(pSIDInfoList, nPos);
        pSIDInfoList = NULL;
    }

    if (ppszGroupSidList)
    {
        FreeSid(ppszGroupSidList, nPos);
        LW_SAFE_FREE_MEMORY(ppszGroupSidList);
        ppszGroupSidList = NULL;
    }
    return dwError;
error:
   goto cleanup; 
}

/*
 *
 * Add Sids to the list 
 *
 */
static
DWORD
AddSidToList(
    PSTR pszSrc, 
    int nPos,
    PSTR *ppszSidList
    )
{
    DWORD dwError = 0;
    PSTR pszStr = NULL;

    if ( pszSrc == NULL )
        return LW_ERROR_INVALID_PARAMETER;

    dwError = LwAllocateString(pszSrc, &pszStr);
    BAIL_ON_LWT_ERROR(dwError);

    ppszSidList[nPos] = pszStr;

error:
    return dwError;
}

/*
 *
 *  free SIDs from the list
 *
 */
static
DWORD
FreeSid(
    PSTR *ppszSidList,
    int nCount
    )
{
    DWORD i = 0;

    if(!ppszSidList)
    {
        return LW_ERROR_INVALID_PARAMETER;
    }

    for (i = 0; i < nCount; i++)
    {
        LW_SAFE_FREE_STRING(ppszSidList[i]);
    }

    return LW_ERROR_SUCCESS;
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
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwTest = 0;
    PLSA_SID_INFO pSIDInfoList = NULL;
    CHAR chDomainSep = '\0';
    CHAR  szTestMsg[256] = { 0 };
    PLWTFAILDATA pInvalidData = NULL;
    PCSTR pszTestAPIs = "LsaGetNamesBySidList";
    PCSTR pszTestDescription = 
            "API returns invalid parameter error for invalid function parameters";

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

        if ( LWTSID_INVALID == pInvalidData->Field )
        {
            /* Test with invalid SID */
            dwLocalError = LsaGetNamesBySidList(
                            hLsaConnection,
                            1,
                            &(pInvalidData->pszSid),
                            &pSIDInfoList, 
                            &chDomainSep);

            if ( dwLocalError != pInvalidData->dwErrorCode)
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf( szTestMsg, 
                          sizeof(szTestMsg), 
                          "API returned with error code (%lu) for invalid sid parameter",
                          (unsigned long)dwLocalError);
                LWT_LOG_TEST(szTestMsg);
            }

            if ( pSIDInfoList )
            {
                LsaFreeSIDInfoList(pSIDInfoList, 1);
                pSIDInfoList = NULL;
            }
        }
    }
cleanup:
    return dwError;
error:
    goto cleanup;
}

