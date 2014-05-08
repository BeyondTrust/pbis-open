#include "includes.h"




/*
* Get the type of the invalid field type
*/
static
DWORD
Lwt_LsaTestGetInValidFieldType(
    PSTR pszInput,
    DWORD *pdwField
    );



/*
 * ExpandHomeDirectory
 *
 * Expand variables in home directory string.
 */
DWORD
ExpandHomeDirectory(
    PCSTR pszHomeDirectory,
    PCSTR pszDefaultHomeDir,
    PCSTR pszDomainName,
    PCSTR pszUserName,
    PSTR *ppszExpandedHomeDirectory
    );

void
FreeUser(
    PLWTUSER *ppUser
    )
{
    if (*ppUser)
    {
        LwFreeMemory((PVOID)(*ppUser)->pszNTName);
        LwFreeMemory((PVOID)(*ppUser)->pszSamAccountName);
        LwFreeMemory((PVOID)(*ppUser)->pszSid);
        LwFreeMemory((PVOID)(*ppUser)->pszUserPrincipalName);
        LwFreeMemory((PVOID)(*ppUser)->pszFQDN);
        LwFreeMemory((PVOID)(*ppUser)->pszNetBiosName);
        LwFreeMemory((PVOID)(*ppUser)->pszPassword);
        LwFreeMemory((PVOID)(*ppUser)->pszUnixUid);
        LwFreeMemory((PVOID)(*ppUser)->pszUnixGid);
        LwFreeMemory((PVOID)(*ppUser)->pszAlias);
        LwFreeMemory((PVOID)(*ppUser)->pszUnixGecos);
        LwFreeMemory((PVOID)(*ppUser)->pszUnixLoginShell);
        LwFreeMemory((PVOID)(*ppUser)->pszUnixHomeDirectory);
        LwFreeMemory((PVOID)(*ppUser)->pszOriginalUnixHomeDirectory);
        LwFreeMemory((PVOID )*ppUser);
        *ppUser = NULL;
    }
}

/*
 *
 * Get the user at index
 *
 */

DWORD
GetUser(
    PTESTDATA pTestData,
    size_t index,
    PLWTUSER *ppUser
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLWTUSER pUser = NULL;
    PLWTDATA pLwtData = NULL;
 
    PLWTDATAIFACE pUserIface = NULL;
    PSTR pszStr = NULL;

    pUserIface = pTestData->pUserIface;


    dwError = pUserIface->GetNext(pUserIface->pvDataContext, index, &pLwtData);
    BAIL(dwError);

    dwError = LwAllocateMemory(sizeof(LWTUSER),(PVOID *) &pUser);
    if (dwError != LW_ERROR_SUCCESS)
        dwError = LW_ERROR_OUT_OF_MEMORY;
    BAIL(dwError);

    dwError = Lwt_LsaTestGetValue(pLwtData, "NTName", &pUser->pszNTName);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;


    dwError = Lwt_LsaTestGetValue(pLwtData, "SamAccountName", &pUser->pszSamAccountName);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    dwError = Lwt_LsaTestGetValue(pLwtData, "Alias", &pUser->pszAlias);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    dwError = Lwt_LsaTestGetValue(pLwtData, "Sid", &pUser->pszSid);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    dwError = Lwt_LsaTestGetValue(pLwtData, "UserPrincipalName", &pUser->pszUserPrincipalName);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    dwError = Lwt_LsaTestGetValue(pLwtData, "FQDN", &pUser->pszFQDN);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    dwError = Lwt_LsaTestGetValue(pLwtData, "NetBiosName", &pUser->pszNetBiosName);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    dwError = Lwt_LsaTestGetValue(pLwtData, "Password", &pUser->pszPassword);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    dwError = Lwt_LsaTestGetValue(pLwtData, "UserEnabled", &pszStr);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;
   
    if (pszStr != NULL)
    {
        pUser->dwEnabled = atoi(pszStr);
        free(pszStr);
        pszStr = NULL;
    }

    dwError = Lwt_LsaTestGetValue(pLwtData, "AccountExpired", &pszStr);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;
    
    if (pszStr)
    {
        pUser->dwAccountExpired = atoi(pszStr);
        free(pszStr);
        pszStr = NULL;
    }
    
    dwError = Lwt_LsaTestGetValue(pLwtData, "PasswordExpired", &pszStr);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;
    
    if (pszStr)
    {
        pUser->dwPasswordExpired = atoi(pszStr);
        free(pszStr);
        pszStr = NULL;
    }

    dwError = Lwt_LsaTestGetValue(pLwtData, "UnixUid", &pUser->pszUnixUid);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;
    if (pUser->pszUnixUid)
        pUser->nUnixUid = strtoul(pUser->pszUnixUid, NULL, 10);

    dwError = Lwt_LsaTestGetValue(pLwtData, "UnixGid", &pUser->pszUnixGid);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;
    if (pUser->pszUnixGid)
        pUser->nUnixGid = strtoul(pUser->pszUnixGid, NULL, 10);

    dwError = Lwt_LsaTestGetValue(pLwtData, "UnixGecos", &pUser->pszUnixGecos);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    /*TODO: Gid need to be calculated from SID value, now assigning to 0*/
    pUser->nUserGid = 0;

    dwError = Lwt_LsaTestGetValue(pLwtData, "UnixLoginShell", &pUser->pszUnixLoginShell);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    dwError = Lwt_LsaTestGetValue(pLwtData, "UnixHomeDirectory", &pUser->pszOriginalUnixHomeDirectory);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    ExpandHomeDirectory(
        pUser->pszOriginalUnixHomeDirectory,
        "/home", /* Fixme */
        pUser->pszNetBiosName,
        pUser->pszSamAccountName,
        &pUser->pszUnixHomeDirectory);

    if (dwError == LW_ERROR_CSV_NO_SUCH_FIELD)
    {
        dwError = LW_ERROR_SUCCESS;
    }
cleanup:
    if (pLwtData)
    {
        Lwt_LsaTestFreeData(pLwtData);
        pLwtData = NULL;
    }
    
    *ppUser = pUser;

    return dwError;

error:

    if (pUser)
    {
        FreeUser(&pUser);
    }
    goto cleanup;
}

/*
 * open CSV file and keep the information in a structure
 * for later access
 *
 */
DWORD
InitialiseUserInfo(
    PCSTR pInputFile, 
    PLWTDATAIFACE *ppUserIface,
    DWORD *pdwNumUsers
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVOID pvDataContext = NULL;
    DWORD dwNumUsers = 0;
    PLWTDATAIFACE pUserIface = *ppUserIface;

    /*dwError = CsvOpenFile(pCsvFilename,(PVOID *) &pCsv);*/
    dwError = pUserIface->Init(pInputFile,&pvDataContext);
    BAIL_ON_LWT_ERROR(dwError);

    dwNumUsers = pUserIface->GetMaxEntries(pvDataContext);

cleanup:
    (*ppUserIface)->pvDataContext = pvDataContext;
    *pdwNumUsers = dwNumUsers; 
    return dwError;

error:
    if (pvDataContext)
    {
        pUserIface->ContextFree(pvDataContext);
    }
    goto cleanup;
}


DWORD
DestroyUserInfo(
    PLWTDATAIFACE pUserIface
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    if (pUserIface)
    {
        dwError = pUserIface->ContextFree(pUserIface->pvDataContext);
    }
    return dwError;
}

/*
 * ExpandHomeDirectory
 *
 * Expand variables in home directory string.
 */
DWORD
ExpandHomeDirectory(
    PCSTR pszHomeDirectory,
    PCSTR pszDefaultHomeDir,
    PCSTR pszDomainName,
    PCSTR pszUserName,
    PSTR *ppszExpandedHomeDirectory
    )
{
    DWORD dwError = 0;

    size_t i;
    int bInEscapeMode = 0;

    PSTR pszExpandedHomeDirectory = NULL;
    size_t len = 0;
                
    char szChar[2];

    if (!pszHomeDirectory)
        goto cleanup;

    /* Calculate new length. */
    bInEscapeMode = 0;
    for (i = 0; pszHomeDirectory[i]; i++)
    {
        int c = pszHomeDirectory[i];

        if (c == '%')
            bInEscapeMode = 1;
        else
        {
            if (bInEscapeMode)
            {
                bInEscapeMode = 0;

                if (c == 'H')
                    len += strlen(pszHomeDirectory);
                else if (c == 'D')
                    len += strlen(pszDomainName);
                else if (c == 'U')
                    len += strlen(pszUserName);
                else if (c == '%') /* FIXME : assumption */
                    len += 1;
                else
                    len += 1;
            }
            else
            {

                len += 1;
            }
        }
    }

    dwError = LwAllocateMemory(len+1,(PVOID *)&pszExpandedHomeDirectory);
    BAIL(dwError);

    bInEscapeMode = 0;
    for (i = 0; pszHomeDirectory[i]; i++)
    {
        int c = pszHomeDirectory[i];

        if (c == '%')
            bInEscapeMode = 1;
        else
        {
            if (bInEscapeMode)
            {
                bInEscapeMode = 0;

                if (c == 'H')
                    strcat(pszExpandedHomeDirectory, pszDefaultHomeDir);
                else if (c == 'D')
                    strcat(pszExpandedHomeDirectory, pszDomainName);
                else if (c == 'U')
                    strcat(pszExpandedHomeDirectory, pszUserName);
                else if (c == '%') /* FIXME : assumption */
                    strcat(pszExpandedHomeDirectory, "%");
                else
                    strcat(pszExpandedHomeDirectory, "%");
            }
            else
            {
                /* ha ha ha */
                szChar[0] = c;
                szChar[1] = '\0';
                strcat(pszExpandedHomeDirectory, szChar);
            }
        }
    }

    /* FIXME : need to look in lsassd.conf for space replacement character. */
    for (i = 0; pszExpandedHomeDirectory[i]; i++)
        if (pszExpandedHomeDirectory[i] == ' ')
            pszExpandedHomeDirectory[i] = '_';

cleanup:

    *ppszExpandedHomeDirectory = pszExpandedHomeDirectory;
    return dwError;

error:

    if (pszExpandedHomeDirectory)
    {
        free(pszExpandedHomeDirectory);
        pszExpandedHomeDirectory = NULL;
    }
    goto cleanup;
}

/*
 *
 * Get the invalid data 
 *
 */

DWORD
GetInvalidDataRecord(
    PTESTDATA pTestData,
    size_t index,
    PLWTFAILDATA *ppData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLWTDATA pLwtData = NULL;
    PLWTFAILDATA pData = NULL;
    PLWTDATAIFACE pIface = NULL;
    PSTR pszStr = NULL;

    pIface = pTestData->pInvalidDataIface;
    dwError = pIface->GetNext(pIface->pvDataContext, index, &pLwtData);
    BAIL_ON_LWT_ERROR(dwError);

    dwError = LwAllocateMemory( sizeof(LWTFAILDATA), (PVOID *)&pData);
    BAIL_ON_LWT_ERROR(dwError);

    dwError = Lwt_LsaTestGetValue(pLwtData, "Test", &pszStr);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    dwError = Lwt_LsaTestGetInValidFieldType(pszStr, &pData->Field);
    LwFreeMemory(pszStr);
    if (dwError != LW_ERROR_SUCCESS)
    {
        goto error;
    }

    dwError = Lwt_LsaTestGetValue(pLwtData, "UserName", &pData->pszUserName);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    dwError = Lwt_LsaTestGetValue(pLwtData, "GroupName", &pData->pszGroupName);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;
    
    dwError = Lwt_LsaTestGetValue(pLwtData, "Password", &pData->pszGroupName);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    dwError = Lwt_LsaTestGetValue(pLwtData, "Sid", &pData->pszSid);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;

    dwError = Lwt_LsaTestGetValue(pLwtData, "Uid", &pszStr);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;
   
    if (pszStr != NULL)
    {
        pData->nUid = strtoul(pszStr, 0, 10);
        LwFreeMemory(pszStr);
        pszStr = NULL;
    }

    dwError = Lwt_LsaTestGetValue(pLwtData, "Gid", &pszStr);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;
    
    if (pszStr)
    {
        pData->nGid = strtoul(pszStr, NULL, 10);
        LwFreeMemory(pszStr);
        pszStr = NULL;
    }

    dwError = Lwt_LsaTestGetValue(pLwtData, "Level", &pszStr);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;
    if (pszStr)
    {
        pData->dwLevel = strtoul(pszStr, NULL, 10);
        LwFreeMemory(pszStr);
        pszStr = NULL;

    }
    
    dwError = Lwt_LsaTestGetValue(pLwtData, "ErrorCode", &pszStr);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;
    if (pszStr)
    {
        pData->dwErrorCode = strtoul(pszStr, NULL, 10);
        LwFreeMemory(pszStr);
        pszStr = NULL;
    }
    
    dwError = Lwt_LsaTestGetValue(pLwtData, "MaxEntries", &pszStr);
    if (dwError && dwError != LW_ERROR_CSV_NO_SUCH_FIELD)
        goto error;
    if (pszStr)
    {
        pData->dwMaxEntries = strtoul(pszStr, NULL, 10);
        LwFreeMemory(pszStr);
        pszStr = NULL;
    }

cleanup:
    if (pLwtData)
    {
        Lwt_LsaTestFreeData(pLwtData);
        pLwtData = NULL;
    }
    *ppData = pData;
    return dwError;

error:
    if (pData)
    {
        FreeInvalidDataRecord(pData);
    }
    goto cleanup;
}



/*
 * open CSV file and keep the information in a structure
 * for later access
 *
 */
DWORD
InitialiseInvalidDataSet(
    PCSTR pInputFile, 
    PLWTDATAIFACE *ppDataIface,
    DWORD *pdwNumRecords
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVOID pvDataContext = NULL;
    DWORD dwNumRecords = 0;
    PLWTDATAIFACE pDataIface = *ppDataIface;

    dwError = pDataIface->Init(pInputFile,&pvDataContext);
    BAIL(dwError);

    dwNumRecords = pDataIface->GetMaxEntries(pvDataContext);

cleanup:
    (*ppDataIface)->pvDataContext = pvDataContext;
    *pdwNumRecords = dwNumRecords; 
    return dwError;

error:
    if (pvDataContext)
    {
        pDataIface->ContextFree(pvDataContext);
    }
    goto cleanup;
}


static
DWORD
Lwt_LsaTestGetInValidFieldType(
    PSTR pszInput,
    DWORD *pdwField
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwField = 0;

    if (!strcmp(pszInput, "UserName"))
    {
        dwField = LWTUSER_INVALID;
    }
    else if (!strcmp(pszInput, "GroupName"))
    {
        dwField = LWTGROUP_INVALID;
    }
    else if (!strcmp(pszInput, "UserInfoLevel"))
    {
        dwField = LWTUSERINFOLEVEL_INVALID;
    }
    else if (!strcmp(pszInput, "GroupInfoLevel"))
    {
        dwField = LWTGROUPINFOLEVEL_INVALID;
    }
    else if (!strcmp(pszInput, "Uid"))
    {
        dwField = LWTUID_INVALID;
    }
    else if (!strcmp(pszInput, "Gid"))
    {
        dwField = LWTGID_INVALID;
    }
    else if(!strcmp(pszInput, "Sid"))
    {
        dwField = LWTSID_INVALID;
    }
    else if (!strcmp(pszInput, "Password"))
    {
        dwField = LWTPASSWORD_INVALID;
    }
    else if (!strcmp(pszInput, "MaxUserInvalid"))
    {
        dwField = LWTMAXUSER_INVALID;
    }
    else if (!strcmp(pszInput, "MaxGroupInvalid"))
    {
        dwField = LWTMAXGROUPS_INVALID;
    }
    else
    {
        dwError = LW_ERROR_UNKNOWN;
        goto error;
    }
cleanup:
    *pdwField = dwField;
    return dwError;

error:
    goto cleanup;
}



VOID
FreeInvalidDataRecord(
    PLWTFAILDATA pData
    )
{
    LW_SAFE_FREE_MEMORY( pData->pszUserName );
    LW_SAFE_FREE_MEMORY( pData->pszGroupName );
    LW_SAFE_FREE_MEMORY( pData->pszSid );
    LW_SAFE_FREE_MEMORY( pData->pszPassword );
    LW_SAFE_FREE_MEMORY( pData );
}


