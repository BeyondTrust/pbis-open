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


#include "includes.h"


/*
 *
 * Validates the Users retrieved from AD with the information of
 * TEST DATA
 */
static
DWORD 
Lwt_LsaValidateUserById(
    HANDLE hLSAConnection,
    DWORD dwLevel,
    PTESTDATA pTestData
    );

/*
 * MatchUserInfo0
 *
 * Check LSA_USER_INFO_0 matches information in CSV.
 */
static
DWORD
MatchUserInfo0(
    PLWTUSER pUser,
    uid_t nLookedUpBy,
    PLSA_USER_INFO_0 pUserInfo
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
 *
 *
 * This programs verifies the user information across different level are consistent
 * It validates the information from the AD with a CSV fie for different user properties
 */

int 
find_user_by_id_main(
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
                            &pTestData
                            );
    BAIL_ON_LWT_ERROR(dwError);
    
    /*Start collect the user infos for the level 0*/
    for (dwIndex = 0; dwIndex < 3; dwIndex++)
    {
        dwLocalError = Lwt_LsaValidateUserById(
                            hLSAConnection, 
                            dwIndex,
                            pTestData
                            );
        BAIL_ON_TEST_BROKE(dwLocalError);
    }    


    /* Compare the group information based on different levels*/
    /*if (pUserInfo0 && pUserInfo1)
    {
        Lwt_Lsa_ValidateInfosBasedOnLevel(
                                        pUserInfo0,
                                        pUserInfo1,
                                        0,
                                        1);
    }
    
    
    if (pUserInfo0 && pUserInfo1)
    {
        Lwt_Lsa_ValidateInfosBasedOnLevel(
                                        pUserInfo0,
                                        pUserInfo1,
                                        0,
                                        2);
    }
    
    
    if(pUserInfo1 && pUserInfo2)
    {
        Lwt_Lsa_ValidateInfosBasedOnLevel(
                                        pUserInfo0,
                                        pUserInfo1,
                                        1,
                                        2);
    }*/
    
cleanup:
    Lwt_LsaTestTeardown(&hLSAConnection, &pTestData);

    return LwtMapErrorToProgramStatus(dwError);
                        
error:
    goto cleanup;
}

/*
 * Higher level api to query the user info by ID
 *
 */
static
DWORD 
Lwt_LsaValidateUserById(
    HANDLE hLSAConnection,
    DWORD dwLevel,
    PTESTDATA pTestData
    )
{
    size_t nUser=0;
    DWORD dwError = LW_ERROR_SUCCESS;
    PVOID pUserInfo = NULL;
    PLWTUSER pUser = NULL;
    BOOL  bError = FALSE;
    PCSTR pszTestDescription = "Find User By Id";
    PCSTR pszTestAPIs = "LsaFindUserById";
    char pszTestMsg[256] = {0};
    
    for (nUser = 0; nUser != pTestData->dwNumUsers; nUser++)
    {
        dwError = GetUser(
                        pTestData,
                        nUser,
                        &pUser
                        );
        BAIL_ON_LWT_ERROR(dwError);

        if (pUser && pUser->pszUnixUid)
        {
            if (pUser->nUnixUid > 0)
            {
                snprintf(pszTestMsg, sizeof(pszTestMsg), "Looking for UID:%lu",
                        (unsigned long)pUser->nUnixUid);
                dwError = LsaFindUserById(
                                    hLSAConnection,
                                    pUser->nUnixUid,
                                    dwLevel,
                                    &pUserInfo
                                    );
                if (dwError != LW_ERROR_SUCCESS)
                {
                    LWT_LOG_TEST(pszTestMsg);
                }
                else
                {
                    if (pUserInfo)
                    {
                        dwError = MatchUserInfo0(
                                    pUser,
                                    pUser->nUnixUid,
                                    pUserInfo);
                        if (dwError)
                        {
                            bError = TRUE;
                        }

                        LsaFreeUserInfo(
                                    dwLevel, 
                                    pUserInfo
                                    );
                        pUserInfo = NULL;
                    }
                }
            }
        }
        FreeUser(&pUser);
        pUser = NULL;     
    }
    
    if (!bError)
    {
        LWT_LOG_TEST(pszTestMsg);
    }

    dwError = ValidateForInvalidParams(hLSAConnection, pTestData);
    BAIL_ON_TEST_BROKE(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

/*
 * MatchUserInfo0
 *
 * Check LSA_USER_INFO_0 matches information in CSV.
 */
static
DWORD
MatchUserInfo0(
    PLWTUSER pUser,
    uid_t nLookedUpBy,
    PLSA_USER_INFO_0 pUserInfo
    )
{
    PCSTR pszTestDescription =
        "LsaFindUserById retrieved LSA_USER_INFO_0 that matches expected values.";
    PCSTR pszTestAPIs = 
        "LsaFindUserByName";
    char szTestMsg[128] = { 0 };

    DWORD dwError = LW_ERROR_SUCCESS;


    snprintf(
        szTestMsg,
        sizeof(szTestMsg),
        "\n\tAccount uid %lu.\n",
        (unsigned long) nLookedUpBy);

    if ( pUser->pszAlias )
    {
        if ( !pUserInfo->pszName || 
             strcasecmp(pUser->pszAlias, pUserInfo->pszName) )
        {
            char buf[128];

            snprintf(
                buf,
                sizeof(buf),
                "\tAlias: test[%s] != lsassd[%s]\n",
                pUser->pszAlias,
                pUserInfo->pszName);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }
    else if ( pUser->pszNTName )
    {
        if ( !pUserInfo->pszName || 
             strcasecmp(pUser->pszNTName, pUserInfo->pszName) )
        {
            char buf[128];

            snprintf(
                buf,
                sizeof(buf),
                "\tNT Name: test[%s] != lsassd[%s]\n",
                pUser->pszNTName,
                pUserInfo->pszName);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }

    if ( !IsNullOrEmpty(pUser->pszSid) )
    {
        if ( ! pUserInfo->pszSid || 
             strcmp(pUser->pszSid, pUserInfo->pszSid) )
        {
            char buf[128];

            snprintf(
                buf,
                sizeof(buf),
                "\tsid: test[%s] != lsassd[%s]\n",
                pUser->pszSid,
                pUserInfo->pszSid);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }


    if ( pUser->pszUnixUid )
    {
        if ( pUser->nUnixUid != pUserInfo->uid )
        {
            char buf[128];

            snprintf(
                buf,
                sizeof(buf),
                "\tuid: test[%lu (%s)] != lsassd[%lu]\n",
                (unsigned long) pUser->nUnixUid,
                pUser->pszUnixUid,
                (unsigned long) pUserInfo->uid);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }

    if ( pUser->pszUnixGid )
    {
        if ( pUser->nUnixGid != pUserInfo->gid )
        {
            char buf[128];

            snprintf(
                buf,
                sizeof(buf),
                "\tgid: test[%lu (%s)] != lsassd[%lu]\n",
                (unsigned long) pUser->nUnixGid,
                pUser->pszUnixGid,
                (unsigned long) pUserInfo->gid);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }

    if ( !IsNullOrEmpty(pUser->pszUnixGecos) )
    {
        if ( ! pUserInfo->pszGecos ||
             strcmp(pUser->pszUnixGecos, pUserInfo->pszGecos) )
        {
            char buf[128];

            snprintf(
                buf,
                sizeof(buf),
                "\tgecos: test[%s] != lsassd[%s]\n",
                pUser->pszUnixGecos,
                pUserInfo->pszGecos);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }

    if ( !IsNullOrEmpty(pUser->pszUnixLoginShell) )
    {
        if ( ! pUserInfo->pszShell || 
             strcmp(pUser->pszUnixLoginShell, pUserInfo->pszShell) )
        {
            char buf[128];

            snprintf(
                buf,
                sizeof(buf),
                "\tshell: test[%s] != lsassd[%s]\n",
                pUser->pszUnixLoginShell,
                pUserInfo->pszShell);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }

    if ( !IsNullOrEmpty(pUser->pszUnixHomeDirectory) )
    {
        if ( ! pUserInfo->pszHomedir || 
             strcmp(pUser->pszUnixHomeDirectory, pUserInfo->pszHomedir) )
        {
            char buf[128];

            snprintf(
                buf,
                sizeof(buf),
                "\thome directory: test[%s] != lsassd[%s]\n",
                pUser->pszUnixHomeDirectory,
                pUserInfo->pszHomedir);

            Lwt_strcat(szTestMsg, sizeof(szTestMsg), buf);
            dwError = LW_ERROR_TEST_FAILED;
        }
    }

    LWT_LOG_TEST(szTestMsg);
    return dwError;
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
    HANDLE hLSAConnection,
    PTESTDATA pTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLocalError = LW_ERROR_SUCCESS;
    DWORD dwTest = 0;
    DWORD dwLevel = 1;
    CHAR  szTestMsg[256] = { 0 };
    PLWTFAILDATA pInvalidData = NULL;
    PVOID pUserInfo = NULL;
    PCSTR pszTestAPIs = "LsaFindUserById";
    PCSTR pszTestDescription = 
            "API returns invalid parameter error for invalid function parameters";

    if ( !pTestData || !pTestData->pInvalidDataIface )
    {
        dwLocalError = LW_ERROR_TEST_SKIPPED;
        BAIL_ON_TEST_BROKE(dwLocalError);
    }

    for ( dwTest = 0; dwTest < pTestData->dwNumInvalidDataSet; dwTest++ )
    {
        dwLocalError = GetInvalidDataRecord( pTestData, 
                                             dwTest, 
                                             &pInvalidData);
        BAIL_ON_TEST_BROKE(dwLocalError);

        if ( LWTUID_INVALID == pInvalidData->Field )
        {
            dwLocalError = LsaFindUserById(
                                hLSAConnection,
                                pInvalidData->nUid,
                                dwLevel,
                                &pUserInfo
                                );

            if ( pInvalidData->dwErrorCode != dwLocalError )
            {
                dwError = LW_ERROR_TEST_FAILED;
                snprintf( szTestMsg, 
                          sizeof(szTestMsg), 
                          "API returned with error code (%lu) for invalid user id parameter",
                          (unsigned long)dwLocalError);
                LWT_LOG_TEST(szTestMsg);
            }
        
            if ( pUserInfo )
            {
                LsaFreeUserInfo(
                                dwLevel, 
                                pUserInfo
                                );
                pUserInfo = NULL;
            }
        }
    }

error:
    return dwError;
}


/*static 
DWORD 
Lwt_Lsa_CompareUserInfo(
          PVOID  pUserInfo_1,
          PVOID  pUserInfo2,
          DWORD dwLevel1,
          DWORD dwLevel2
          )
{
    PCSTR pszTestMsg[]="Comparing the user Information between the different \
                        levels"
    PSTR pszTestApi = "LsaFindUserById";
    PSTR pszTestMsg[128];

 
    Compares the user information between the levels
    if(dwLevel1 >=1)
    {
        Todo:Add the code to compare level 1 and 2 users 
        PLSA_USER_INFO_1 pUserInfo1 = (PLSA_USER_INFO_1) pUserInfo_1;
        PLSA_USER_INFO_2 pUserInfo2 = (PLSA_USER_INFO_2) pUserInfo_2;

        
        if (strcmp(pUserInfo1->pszDN pUserInfo2->pszDN)
        {
            snprintf(pszTestMsg, sizeof(pszTestMsg), 
                   "The domain name  information for the uid:%s is not consitent \ 
                    The domain name info for level %d is %s domain name info for \
                    level %d is %s", pUserInfo0->uid, dwLevel1, 
                     pUserInfo0->pszDN, dwlevel2, pUserInfo1->pszDN);
            
        } 
    
    }
    if (dwLevel1 >= 0)
    {
        PLSA_USER_INFO_0 pUserInfo0 = (PLSA_USER_INFO_0 )pUserInfo_1;
        PLSA_USER_INFO_0 pUserInfo1 = (PLSA_USER_INFO_0 )pUserInfo_2;
        if (strcmp(pUserInfo0->pszName, pUserInfo1->pszName)
        {
            snprintf(pszTestMsg, sizeof(pszTestMsg), 
                   "The user name information for the uid:%s is not consitent \ 
                    The user name info for level %d is %s User name info for \
                    level %d is %s", pUserInfo0->uid, dwLevel1, 
                     pUserInfo0->pszName, dwlevel2, pUserInfo1->pszName);
            
        } 

        if (pUserInfo0->gid != pUserInfo1->gid)
        {
        
            snprintf(pszTestMsg, sizeof(pszTestMsg), 
                    "The gid information for the user%s is not consistent");
            dwError = LW_LOG_ERROR_TEST_FAILED;
            BAIL_ON_TEST_BROKE(dwError);
        }
        if (strcmp(pUserInfo0->pszName, pUserInfo1->pszName)
        {
            snprintf(pszTestMsg, sizeof(pszTestMsg), 
                   "The user name information for the uid:%s is not consitent \ 
                    The user name info for level %d is %s User name info for \
                    level %d is %s", pUserInfo0->uid, dwLevel1, 
                     pUserInfo0->pszName, dwlevel2, pUserInfo1->pszName);
            
        } 
        if ((pUserInfo0->pszHomeDir&&pUserInfo->pszHomeDir) &&
             strcmp(pUserInfo0->pszHomeDir  pUserInfo1->pszHomeDir))
        {
            snprintf(pszTestMsg, sizeof(pszTestMsg), 
                    "The Home Dir information for the user:%s is inconsitent for \
                     Level:%d  is %s and for Level:%d is %s",
                     pUserInfo0->pszName, dwLevel1, pUserInfo0->pszHomeDir,
                     dwLevel2, pUserInfo1->pszHomeDir);

            dwError = LW_LOG_ERROR_TEST_FAILED;
            BAIL_ON_TEST_BROKE(dwError);
       
        }
        if ((pUserInfo0->pszSid && pUserInfo1->pszSid &&) &&
           (strcmp(pUserInfo0->pszSid pUserInfo1->pszSid)))
        {
            snprintf(pszTestMsg, sizeof(pszTestMsg), 
                    "The SID information for the user:%s is inconsitent for \
                     Level:%d  is %s and for Level:%d is %s",
                     pUserInfo0->pszName, dwLevel1, pUserInfo0->pszSid,
                     dwLevel2, pUserInfo1->pszSid);

            dwError = LW_LOG_ERROR_TEST_FAILED;
            BAIL_ON_TEST_BROKE(dwError);
        
        }

         
    
    }
cleanup:
    LWT_LOG_TEST(dwError);
    return dwError;

error:
    goto cleanup;


}*/
