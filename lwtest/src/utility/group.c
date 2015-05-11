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
 * Module Name: group.c
 *
 * Opens, reads and loads the group information from file into group structure
 * 
 */

#include "includes.h"

/*
 * UpdateMembersToGrouplist 
 * 
 * Function reads the group members from the group information file and update to group structure.
 * 
 */
static
DWORD 
UpdateMembersToGrouplist(
    PSTR pszFieldValue,
    PLWTGROUP pGroup
    );

/*
 * ParseAndReplaceSpecialChars 
 * 
 * Function searches for special characters in a string and replaces it with appropriate characters.
 * 
 */
static
void 
ParseAndReplaceSpecialChars(
    PSTR pszStr
    );

/*
 * InitializeGroupInfo
 * 
 * Function opens the group information files and update the file information to PLWTCSV structure 
 * for future use
 * 
 */
DWORD
InitializeGroupInfo(
    PCSTR pszCsvFilename,
    PLWTDATAIFACE  *ppGroupIface,
    DWORD *pdwNumGroups
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwNumUsers = 0;
    PVOID pvGroupInfo = NULL;
    PLWTDATAIFACE pGroupIface = *ppGroupIface;

    //dwError = CsvOpenFile(pszCsvFilename, (PVOID *)&pGroupInfo);
    dwError = pGroupIface->Init(pszCsvFilename, &pvGroupInfo);
    BAIL_ON_LWT_ERROR(dwError);



    dwNumUsers = pGroupIface->GetMaxEntries(pvGroupInfo);

cleanup:
    (*ppGroupIface)->pvDataContext = pvGroupInfo;
    *pdwNumGroups = dwNumUsers;
    return dwError;

error:
    if (pvGroupInfo)
    {
        pGroupIface->ContextFree(pvGroupInfo);
        pvGroupInfo = NULL;
    }
    goto cleanup;
}


/*
 * DestroyGroupInfo
 * 
 * Function closes the group information file and frees the memory allocated for PLWTCSV structure
 * 
 */
void 
DestroyGroupInfo(
    PLWTDATAIFACE  pGroupInfo
    )
{
    if (pGroupInfo)
    {
        pGroupInfo->ContextFree(pGroupInfo->pvDataContext);
        pGroupInfo->pvDataContext = NULL;
    }
}


/*
 * GetGroup
 * 
 * Function reads specified row from the group information file and update to group structure.
 * 
 */
DWORD
GetGroup(
    PTESTDATA pTestContext,
    size_t index,
    PLWTGROUP *ppGroup
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLWTDATA  pLwtData = NULL;
    PSTR  pszFieldValue = NULL;    
    PLWTDATAIFACE  pGroupIface = NULL;
    PLWTGROUP pGroup = NULL;

    pGroupIface = pTestContext->pGroupIface;

    //pLwtData = pGroupIface->pvContext;
    dwError = pGroupIface->GetNext(pGroupIface->pvDataContext, index, &pLwtData);
    BAIL_ON_LWT_ERROR(dwError);

    pGroup = malloc(sizeof(LWTGROUP));
    if (!pGroup)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LWT_ERROR(dwError);
    }
    memset(pGroup, 0, sizeof(LWTGROUP));
    dwError = Lwt_LsaTestGetValue(pLwtData, "NTName", &pGroup->pszName);
    if ( dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD )
    {
        BAIL_ON_LWT_ERROR(dwError);
    }

    if (pGroup->pszName)
    {
        ParseAndReplaceSpecialChars(pGroup->pszName);
    }

    dwError = Lwt_LsaTestGetValue(pLwtData, "Alias", &pGroup->pszAlias);
    if ( dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD )
    {
        BAIL_ON_LWT_ERROR(dwError);
    }


    dwError =Lwt_LsaTestGetValue(pLwtData, "Sid", &pGroup->pszSid);
    if ( dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD )
    {
        BAIL_ON_LWT_ERROR(dwError);
    }

    dwError = Lwt_LsaTestGetValue(pLwtData, "DistinguishedName", &pGroup->pszDN);
    if ( dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD )
    {
        BAIL_ON_LWT_ERROR(dwError);
    }

    dwError = Lwt_LsaTestGetValue(pLwtData, "Password", &pGroup->pszPassword);
    if ( dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD )
    {
        BAIL_ON_LWT_ERROR(dwError);
    }

    dwError = Lwt_LsaTestGetValue(pLwtData, "objectGUID", &pGroup->pszObjectGuid);
    if ( dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD )
    {
        BAIL_ON_LWT_ERROR(dwError);
    }

    dwError = Lwt_LsaTestGetValue(pLwtData, "SamAccountName", &pGroup->pszSamAccountName);
    if ( dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD )
    {
        BAIL_ON_LWT_ERROR(dwError);
    }

    dwError = Lwt_LsaTestGetValue(pLwtData, "NetBiosName", &pGroup->pszNetBiosName);
    if ( dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD )
    {
        BAIL_ON_LWT_ERROR(dwError);
    }

    dwError = Lwt_LsaTestGetValue(pLwtData, "Members", &pszFieldValue);
    if ( dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD )
    {
        BAIL_ON_LWT_ERROR(dwError);
    }
    
    if (pszFieldValue)
    {
        dwError = UpdateMembersToGrouplist(pszFieldValue, pGroup);
        BAIL_ON_LWT_ERROR(dwError);

        free(pszFieldValue);
        pszFieldValue = NULL;
    }

    /* TODO: Gid value need to be calculated from Sid value. Now for testing purpose value is read from csv file and populated to group structure*/        
    dwError = Lwt_LsaTestGetValue(pLwtData, "Gid", &pszFieldValue);
    if ( dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD )
    {
        BAIL_ON_LWT_ERROR(dwError);
    }
        
    if (pszFieldValue)
    {
        pGroup->nGid = atol(pszFieldValue);
        free(pszFieldValue);
        pszFieldValue = NULL;
    }    

    if (dwError == LW_ERROR_CSV_NO_SUCH_FIELD)
    {
        dwError = LW_ERROR_SUCCESS;
    }


cleanup:    
    if (pLwtData)
    {
        Lwt_LsaTestFreeData(pLwtData);
    }

    *ppGroup = pGroup;
    return dwError;

error:
    FreeGroup(&pGroup);
    pGroup = NULL;

    if (pszFieldValue)
    {
        free(pszFieldValue);
        pszFieldValue = NULL;
    }
    goto cleanup;
}


/*
 * FreeGroup 
 * 
 * Function frees the memory allocated for group and its mebers.
 * 
 */
void
FreeGroup(
    PLWTGROUP *ppGroup
    )
{
    if ( *ppGroup )
    {
        PLWTGROUP pGroup = *ppGroup;

        if ( pGroup->pszName )
        {
            free(pGroup->pszName);
            pGroup->pszName = NULL;
        }

        if (pGroup->pszAlias)
        {
            free(pGroup->pszAlias);
            pGroup->pszAlias = NULL;
        }

        if ( pGroup->pszSid )
        {
            free(pGroup->pszSid);
            pGroup->pszSid = NULL;
        }

        if ( pGroup->pszDN )
        {
            free(pGroup->pszDN);
            pGroup->pszDN = NULL;
        }

        if ( pGroup->pszPassword )
        {
            free(pGroup->pszPassword);
            pGroup->pszPassword = NULL;
        }

        if ( pGroup->pszObjectGuid )
        {
            free(pGroup->pszObjectGuid);
            pGroup->pszObjectGuid = NULL;
        }

        if ( pGroup->pszSamAccountName )
        {
            free(pGroup->pszSamAccountName);
            pGroup->pszSamAccountName = NULL;
        }
    
        if (pGroup->pszNetBiosName)
        {
            free(pGroup->pszNetBiosName);
            pGroup->pszNetBiosName = NULL;
        }    

        if (pGroup->ppszMembers)
        {
            DWORD dwMember = 0;
            while (!IsNullOrEmpty(pGroup->ppszMembers[dwMember]))
            {
                free(pGroup->ppszMembers[dwMember]);
                pGroup->ppszMembers[dwMember] = NULL;
                dwMember++;
            }
            free(pGroup->ppszMembers);
            pGroup->ppszMembers = NULL;
        }        
        free(pGroup);
        pGroup = NULL;
        *ppGroup = pGroup;
    }
}


/*
 * UpdateMembersToGrouplist 
 * 
 * Function reads the group members from the group information file and update to group structure.
 * 
 */
static
DWORD 
UpdateMembersToGrouplist(
    PSTR pszFieldValue,
    PLWTGROUP pGroup
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;    
    DWORD dwIndex = 0;
    DWORD dwMemberCnt = 0;
    DWORD dwNetBiosNameLen = 0;
    DWORD dwMemberFieldLen = 0;
    DWORD dwFieldValueLen = 0;        
    PSTR* ppszMembers = NULL;
    PSTR  pszStrIndex1 = NULL;
    PSTR  pszStrIndex2 = NULL;

    dwNetBiosNameLen = strlen(pGroup->pszNetBiosName);

    /* Getting the member count from the field value. Initialize to 1, bacuase there will be atleast one member for the group */
    dwMemberCnt = 1;    
    for (dwIndex = 0; dwIndex < strlen(pszFieldValue); dwIndex++)
    {
        if (pszFieldValue[dwIndex] == ',')
            dwMemberCnt++;
    }

    ppszMembers = calloc(dwMemberCnt+1, sizeof(PSTR));
    if (!ppszMembers)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LWT_ERROR(dwError);
    }
        
    dwIndex = 0;
    pszStrIndex1 = strstr(pszFieldValue, "CN=");

    while (pszStrIndex1 != NULL)
    {
        PSTR pszValue = NULL;

        pszStrIndex2 = strchr(pszStrIndex1, ',');
        if (pszStrIndex2)
        {
            dwFieldValueLen = strlen(pszStrIndex1) - strlen(pszStrIndex2) - 3; /* -3 to strip string CN= from the member name*/            
        }
        else
        {
            dwFieldValueLen = strlen(pszStrIndex1) - 3;
        }

        dwMemberFieldLen= dwNetBiosNameLen + dwFieldValueLen + 2; /* +2 to add /, between netbios name and member name, and \0 at the end of string*/

        pszValue = calloc(dwMemberFieldLen, 1);
        if (!pszValue)
        {
            dwError = LW_ERROR_OUT_OF_MEMORY;
            BAIL_ON_LWT_ERROR(dwError);
        }

        memcpy(pszValue, pGroup->pszNetBiosName, dwNetBiosNameLen);

        pszValue[dwNetBiosNameLen] = '\\';        

        memcpy(&pszValue[dwNetBiosNameLen+1], &pszStrIndex1[3], dwFieldValueLen);

        pszValue[dwMemberFieldLen-1] = '\0';

        ppszMembers[dwIndex++] = pszValue;

        if (!pszStrIndex2)
            break;
        
        pszStrIndex1 = strstr(pszStrIndex2, "CN=");
    }
    ppszMembers[dwIndex] = NULL;

cleanup:
    pGroup->ppszMembers = ppszMembers;
    return dwError;

error:
    goto cleanup;
}


/*
 * ParseAndReplaceSpecialChars 
 * 
 * Function searches for special characters in a string and replaces it with appropriate characters.
 * 
 */
static
void 
ParseAndReplaceSpecialChars(
    PSTR pszStr
    )
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


