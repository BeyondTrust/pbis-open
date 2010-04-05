#include "includes.h"

#define ACTION_NONE        0
#define ACTION_LIST        1
#define ACTION_LIST_ALL    2
#define ACTION_DOWNLOAD    3
#define ACTION_UPLOAD      4
#define ACTION_LIST_ALL_GP 5

#define MAX_ARGS    12

#define MACHINE_GROUP_POLICY 1
#define USER_GROUP_POLICY    2

typedef struct _GUID_LIST {
    DWORD dwPolicyID;
    PSTR pszClientGUID;
    PSTR pszPolicyName;
    DWORD dwPolicyType;
    struct _GUID_LIST *pNext;
}GUID_LIST,*PGUID_LIST;

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PDWORD pdwAction,
    PDWORD pdwPolicy,
    PSTR*  ppszGPO,
    PSTR*  ppszPath
    );

VOID
ShowUsage();

DWORD
MapPolicyToGUID(
    PGUID_LIST    pGUIDList,
    DWORD         dwPolicy,
    PSTR          *ppszClientGUID,
    PSTR          *ppszPolicyName,
    PDWORD        pdwPolicyType
    );

VOID
MapErrorCodes(
    DWORD dwError
    );

DWORD
BuildGUIDList(
    PGUID_LIST *ppGUIDList,
    PDWORD pdwGUIDListCount
    );

VOID
FreeGUIDList(
    PGUID_LIST *ppGUIDList
    );

DWORD
ListAllGPSettings(
    PGUID_LIST pGUIDList,
    PSTR       pszDomain
    );

VOID
ListAllEnabledGPSettings(
    PGUID_LIST pGUIDList
    );

DWORD
ListSpecificGPSettings(
    PGROUP_POLICY_OBJECT    pGPOList,
    PSTR                    pszPolicyName,
    DWORD                   dwPolicy
    );

DWORD
DownloadSpecificGPSettings(
    PGROUP_POLICY_OBJECT    pGPOList,
    PSTR                    pszGUID,
    PSTR                    pszPolicyName,
    DWORD                   dwPolicy,
    DWORD                   dwPolicyType,
    PSTR                    pszgpo,
    PSTR                    pszPath
    );

DWORD
UploadSpecificGPSettings(
    PGROUP_POLICY_OBJECT    pGPOList,
    PSTR                    pszGUID,
    PSTR                    pszPolicyName,
    DWORD                   dwPolicy,
    DWORD                   dwPolicyType,
    PSTR                    pszgpo,
    PSTR                    pszPath
    );

DWORD
ExecuteCommand(
   DWORD        dwAction,
   PSTR         pszDomain,
   DWORD        dwPolicy,
   PSTR         pszgpo,
   PSTR         pszPath
   );

int
main(
   int argc,
   char* argv[]
   )
{
    DWORD dwError = 0;
    PSTR pszDomain = NULL;
    DWORD dwAction = ACTION_NONE;
    DWORD dwPolicy = 0;
    PSTR pszgpo = NULL;
    PSTR pszPath = NULL;

    dwError = ParseArgs(
                argc,
                argv,
                &dwAction,
                &dwPolicy,
                &pszgpo,
                &pszPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = GetADDomain(&pszDomain);
    if(dwError)
    {
        fprintf(stderr, "Failed to get AD domain name\n");
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = ExecuteCommand(
                dwAction,
                pszDomain,
                dwPolicy,
                pszgpo,
                pszPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszPath);
    LW_SAFE_FREE_STRING(pszDomain);
    LW_SAFE_FREE_STRING(pszgpo);


    return dwError;

error:

    MapErrorCodes(dwError);

    goto cleanup;
}


DWORD
ExecuteCommand(
   DWORD        dwAction,
   PSTR         pszDomain,
   DWORD        dwPolicy,
   PSTR         pszgpo,
   PSTR         pszPath
   )
{
    DWORD dwError = 0;
    PSTR pszGUID = NULL;
    PSTR pszPolicyName = NULL;
    PGROUP_POLICY_OBJECT pGPOList = NULL;
    PGUID_LIST pGUIDList = NULL;
    DWORD dwGUIDListCount = 0;

    dwError = BuildGUIDList(
                     &pGUIDList,
                     &dwGUIDListCount);
    BAIL_ON_LWUTIL_ERROR(dwError);


    if (dwAction == ACTION_LIST_ALL)
    {
        dwError = ListAllGPSettings(
                        pGUIDList,
                        pszDomain);
        BAIL_ON_LWUTIL_ERROR(dwError);

    }
    else if (dwAction == ACTION_LIST_ALL_GP)
    {
        ListAllEnabledGPSettings(
            pGUIDList);

    }
    else
    {
        DWORD dwPolicyType = 0;

        if( dwPolicy <= 0 || dwPolicy > dwGUIDListCount)
        {
            fprintf(stderr, "Error: an invalid policy id was specified.\n");
            fprintf(stdout, "Please enter a policy id in the range [1, %d]\n",
                   dwGUIDListCount);
            goto error;
        }

        dwError = MapPolicyToGUID(
                    pGUIDList,
                    dwPolicy,
                    &pszGUID,
                    &pszPolicyName,
                    &dwPolicyType);
        BAIL_ON_LWUTIL_ERROR(dwError);

        dwError = EnumEnabledGPOs(
                     dwPolicyType,
                     pszDomain,
                     pszGUID,
                     &pGPOList);
        BAIL_ON_LWUTIL_ERROR(dwError);

        if(!pGPOList)
        {
            if(pszgpo)
                fprintf(stderr, "Error: %s is not defined in %s\n",
                        pszPolicyName,
                        pszgpo);
            else
                fprintf(stderr,
                        "Error: no group policy by name [%s] was found.\n",
                        pszPolicyName);

            goto cleanup;
        }

        switch(dwAction)
        {
            case ACTION_LIST:
                dwError = ListSpecificGPSettings(
                            pGPOList,
                            pszPolicyName,
                            dwPolicy);
                BAIL_ON_LWUTIL_ERROR(dwError);
                break;

            case ACTION_DOWNLOAD:
                dwError = DownloadSpecificGPSettings(
                            pGPOList,
                            pszGUID,
                            pszPolicyName,
                            dwPolicy,
                            dwPolicyType,
                            pszgpo,
                            pszPath);
                BAIL_ON_LWUTIL_ERROR(dwError);
                break;

            case ACTION_UPLOAD:
                dwError = UploadSpecificGPSettings(
                            pGPOList,
                            pszGUID,
                            pszPolicyName,
                            dwPolicy,
                            dwPolicyType,
                            pszgpo,
                            pszPath);
                BAIL_ON_LWUTIL_ERROR(dwError);
                break;
        }

    }

cleanup:

    LW_SAFE_FREE_STRING(pszGUID);
    LW_SAFE_FREE_STRING(pszPolicyName);

    FreeGUIDList(&pGUIDList);

    return dwError;

error:

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PDWORD pdwAction,
    PDWORD pdwPolicy,
    PSTR*  ppszgpo,
    PSTR*  ppszPath
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_GP,
            PARSE_MODE_GPO,
            PARSE_MODE_PATH
        } ParseMode;

    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    int iArg = 1;
    DWORD dwAction = ACTION_NONE;
    DWORD dwPolicy = 0;
    PSTR pszgpo = NULL;
    PSTR pszPath = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;


    for (; iArg < argc; iArg++)
    {
        PSTR pszArg = argv[iArg];

        switch (parseMode)
        {
            case PARSE_MODE_OPEN:

                if ((strcasecmp(pszArg, "--help") == 0) ||
                    (strcasecmp(pszArg, "-h") == 0))
                {
                    ShowUsage();
                    exit(0);
                }
                else if ((strcasecmp(pszArg, "--listall") == 0)||
                         (strcasecmp(pszArg, "-la") == 0))
                {
                    dwAction = ACTION_LIST_ALL;
                }
                else if ((strcasecmp(pszArg, "--listgpcses") == 0)||
                         (strcasecmp(pszArg, "-lgp") == 0))
                {
                    dwAction = ACTION_LIST_ALL_GP;
                }
                else if ((strcasecmp(pszArg, "--list") == 0)||
                         (strcasecmp(pszArg, "-l") == 0))
                {
                    dwAction = ACTION_LIST;
                }
                else if ((strcasecmp(pszArg, "--download") == 0)||
                         (strcasecmp(pszArg, "-d") == 0))
                {
                    dwAction = ACTION_DOWNLOAD;
                }
                else if ((strcasecmp(pszArg, "--upload") == 0)||
                         (strcasecmp(pszArg, "-u") == 0))
                {
                    dwAction = ACTION_UPLOAD;
                }
                else if ((strcasecmp(pszArg, "--path") == 0)||
                         (strcasecmp(pszArg, "-p") == 0))
                {
                    parseMode = PARSE_MODE_PATH;
                }
                else if ((strcasecmp(pszArg, "--gpolicy") == 0) ||
                         (strcasecmp(pszArg, "-gp") == 0))
                {
                    parseMode = PARSE_MODE_GP;
                }
                else if ((strcasecmp(pszArg, "--gpobject") == 0) ||
                         (strcasecmp(pszArg, "-gpo") == 0))
                {
                    parseMode = PARSE_MODE_GPO;
                }
                else
                {
                    fprintf(stderr,
                            "Error: the option [%s] is invalid\n",
                            pszArg);
                    ShowUsage();
                    exit(1);
                }
                break;
            case PARSE_MODE_GP:

                dwPolicy = atoi(pszArg);

                parseMode = PARSE_MODE_OPEN;

                break;
            case PARSE_MODE_GPO:

                LW_SAFE_FREE_STRING(pszgpo);

                dwError = LWAllocateString(pszArg, &pszgpo);
                BAIL_ON_LWUTIL_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_PATH:

                LW_SAFE_FREE_STRING(pszPath);

                dwError = LWAllocateString(pszArg, &pszPath);
                BAIL_ON_LWUTIL_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;
        }
    }

    if (argc == 1)
	{
       fprintf(stderr, "Error: Missing the options to be passed\n");
       ShowUsage();
       exit(1);
	}

    if(dwAction == ACTION_LIST_ALL || dwAction == ACTION_LIST_ALL_GP)
	{
        if (dwPolicy || pszgpo || pszPath)
        {
            fprintf(stderr, "Error: Too many arguments passed\n");
            ShowUsage();
            exit(1);
        }
	}
    else if (dwAction == ACTION_LIST)
    {
        if (!dwPolicy)
        {
            fprintf(stderr, "Error: Group policy ID is  NULL \n");
            dwError = LWUTIL_ERROR_INVALID_PARAMETER;
            BAIL_ON_LWUTIL_ERROR(dwError);
        }
        else if (pszgpo || pszPath)
        {
            fprintf(stderr, "Error: Too many arguments passed\n");
            ShowUsage();
            exit(1);
        }
    }
    else if (dwAction == ACTION_DOWNLOAD || dwAction == ACTION_UPLOAD)
    {
        if (!dwPolicy)
        {
            fprintf(stderr, "Error: Group policy ID is  NULL \n");
            dwError = LWUTIL_ERROR_INVALID_PARAMETER;
            BAIL_ON_LWUTIL_ERROR(dwError);
        }

        if (!pszgpo)
        {
            fprintf(stderr, "Error: Group policy object name is NULL \n");
            dwError = LWUTIL_ERROR_INVALID_PARAMETER;
            BAIL_ON_LWUTIL_ERROR(dwError);
        }

        if (!pszPath)
        {
            fprintf(stderr, "Error: Invalid GPO path was specified\n");
            dwError = LWUTIL_ERROR_INVALID_PARAMETER;
            BAIL_ON_LWUTIL_ERROR(dwError);
        }
    }

    *pdwAction = dwAction;
    *pdwPolicy = dwPolicy;
    *ppszgpo = pszgpo;
    *ppszPath = pszPath;

cleanup:

    return dwError;

error:

    *ppszgpo = NULL;
    *ppszPath = NULL;

    LW_SAFE_FREE_STRING(pszgpo);
    LW_SAFE_FREE_STRING(pszPath);

    goto cleanup;
}


void
ShowUsage()
{
    printf("Usage: lw-gp-admin --list --gpolicy <group policy> \n");
    printf("\t--help        |    -h     Show help\n");
    printf("\t--listgpcses  |    -lgp   List all the group policy extensions\n");
    printf("\t--listall     |    -la    List all the enabled policies in all the GPO's\n");
    printf("\t--list        |    -l     List the GPO's where the specified policy is set\n");
    printf("\t--download    |    -d     Download the specified gp setting to the specified path\n");
    printf("\t--upload      |    -u     Upload the specified gp setting from the specified path\n");
    printf("\t--gpolicy     |    -gp    Specify the desired group policy\n");
    printf("\t                          This should be set with the option '-l' '-d' or '-u'\n");
    printf("\t--gpobject    |    -gpo   Specify the desired group policy object from which policy \n");
    printf("\t                          to be downloaded or uploaded. This should be set only with\n");
    printf("\t                          the option '-d' or '-u'\n");
    printf("\t--path        |    -p     Specify the desired path to download or upload policies\n");
    printf("\t                          from or to AD. This should be set only with the option '-d' or '-u'.\n");
    printf("\t                          Please provide the directory path where GPT.INI is present\n");
    printf("\nExamples:\n");
    printf("\tlw-gp-admin -lgp \n");
    printf("\tlw-gp-admin -la \n");
    printf("\tlw-gp-admin -l -gp <ID> \n");
    printf("\tlw-gp-admin -d -gp <ID> -gpo <gpo name> -p <path>\n");
}

DWORD
BuildGUIDList(
    PGUID_LIST *ppGUIDList,
    PDWORD pdwGUIDListCount
    )
{
    DWORD dwError = 0;
    PGUID_LIST pGUIDList = NULL;
    PGUID_LIST pGUIDTmpList = NULL;
    DWORD dwPolicyID = 0;
    PGROUP_POLICY_CLIENT_EXTENSION pGroupPolicyClientExtensions = NULL;
    PGROUP_POLICY_CLIENT_EXTENSION pTempGroupPolicyClientExtensions = NULL;

    
    dwError = GetCSEListFromRegistry("Services\\gpagent\\GPExtensions",
                            &pGroupPolicyClientExtensions );
    BAIL_ON_LWUTIL_ERROR(dwError);

    pTempGroupPolicyClientExtensions = pGroupPolicyClientExtensions; 

    while (pTempGroupPolicyClientExtensions)
    {
        if(pGUIDList == NULL)
        {
            dwError = LWAllocateMemory(
                        sizeof(GUID_LIST),
                        (PVOID*)&(pGUIDList));
            BAIL_ON_LWUTIL_ERROR(dwError);

            pGUIDTmpList = pGUIDList;
        }
        else
        {
            while(pGUIDTmpList->pNext != NULL)
                pGUIDTmpList = pGUIDTmpList->pNext;

            dwError = LWAllocateMemory(
                        sizeof(GUID_LIST),
                        (PVOID*)&pGUIDTmpList->pNext);
            BAIL_ON_LWUTIL_ERROR(dwError);

            pGUIDTmpList = pGUIDTmpList->pNext;
        }

        if(pTempGroupPolicyClientExtensions->pszName)
        {
            dwError = LWAllocateString(
                        pTempGroupPolicyClientExtensions->pszName,
                        &pGUIDTmpList->pszPolicyName);
            BAIL_ON_LWUTIL_ERROR(dwError);
        }
            
        if(pTempGroupPolicyClientExtensions->pszGUID)
        {
            dwError = LWAllocateString(
                        pTempGroupPolicyClientExtensions->pszGUID,
                        &pGUIDTmpList->pszClientGUID);
            BAIL_ON_LWUTIL_ERROR(dwError);
        }
        
        if(pTempGroupPolicyClientExtensions->dwNoMachinePolicy)
        {
            pGUIDTmpList->dwPolicyType = USER_GROUP_POLICY;
        }
        else
        {
            pGUIDTmpList->dwPolicyType = MACHINE_GROUP_POLICY; 
        }
        //UPdate the policy number
        dwPolicyID += 1;
        pGUIDTmpList->dwPolicyID = dwPolicyID;

        pGUIDTmpList->pNext = NULL;
        pTempGroupPolicyClientExtensions = pTempGroupPolicyClientExtensions->pNext;
    }


    *ppGUIDList = pGUIDList;
    *pdwGUIDListCount = dwPolicyID;

cleanup:
    FreeClientExtensionList(pGroupPolicyClientExtensions);
    return dwError;

error:
    
    FreeGUIDList(&pGUIDList);
    *ppGUIDList = NULL;

    goto cleanup;
}

DWORD
MapPolicyToGUID(
    PGUID_LIST pGUIDList,
    DWORD dwPolicy,
    PSTR *ppszClientGUID,
    PSTR *ppszPolicyName,
    PDWORD pdwPolicyType
    )
{

    DWORD dwError = 0;
    PGUID_LIST pGUIDTmpList = pGUIDList;
    PSTR pszClientGUID = NULL;
    PSTR pszPolicyName = NULL;
    DWORD dwPolicyType = 0;

    while(pGUIDTmpList)
    {
        if(dwPolicy == pGUIDTmpList->dwPolicyID)
        {
            dwError = LWAllocateString(
                        pGUIDTmpList->pszClientGUID,
                        &pszClientGUID);
            BAIL_ON_LWUTIL_ERROR(dwError);

            dwError = LWAllocateString(
                        pGUIDTmpList->pszPolicyName,
                        &pszPolicyName);
            BAIL_ON_LWUTIL_ERROR(dwError);

            dwPolicyType = pGUIDTmpList->dwPolicyType;

            break;
        }

        pGUIDTmpList = pGUIDTmpList->pNext;
    }

    *ppszClientGUID = pszClientGUID;
    *ppszPolicyName = pszPolicyName;
    *pdwPolicyType = dwPolicyType;

cleanup:

    return dwError;

error:

    *ppszClientGUID = NULL;
    *ppszPolicyName = NULL;

    LW_SAFE_FREE_STRING(pszClientGUID);
    LW_SAFE_FREE_STRING(pszPolicyName);

    goto cleanup;
}

VOID
FreeGUIDList(
    PGUID_LIST *ppGUIDList
    )
{

    PGUID_LIST pGUIDList = *ppGUIDList;
    PGUID_LIST pGUIDOld = NULL;

    while(pGUIDList)
    {
        pGUIDOld = pGUIDList;

        LW_SAFE_FREE_STRING(pGUIDList->pszClientGUID);
        LW_SAFE_FREE_STRING(pGUIDList->pszPolicyName);

        pGUIDList = pGUIDList->pNext;

        LWFreeMemory(pGUIDOld);
    }

    *ppGUIDList = NULL;
}

VOID
ListAllEnabledGPSettings(
    PGUID_LIST  pGUIDList
    )
{
    PGUID_LIST pGUIDTmpList = pGUIDList;

    fprintf(stdout,"Computer Policy Settings\n");

    while(pGUIDTmpList)
    {
        if(pGUIDTmpList->dwPolicyType == MACHINE_GROUP_POLICY)
            fprintf(stdout,"  ID = %2d    %-60s        {%s}\n",
                      pGUIDTmpList->dwPolicyID,
                      pGUIDTmpList->pszPolicyName,
                      pGUIDTmpList->pszClientGUID);

        pGUIDTmpList = pGUIDTmpList->pNext;
    }

    pGUIDTmpList = pGUIDList;

    fprintf(stdout,"User Policy Settings\n");

    while(pGUIDTmpList)
    {
        if(pGUIDTmpList->dwPolicyType == USER_GROUP_POLICY)
            fprintf(stdout,"  ID = %2d    %-60s        {%s}\n",
                      pGUIDTmpList->dwPolicyID,
                      pGUIDTmpList->pszPolicyName,
                      pGUIDTmpList->pszClientGUID);

        pGUIDTmpList = pGUIDTmpList->pNext;
    }

}

DWORD
ListAllGPSettings(
    PGUID_LIST  pGUIDList,
    PSTR         pszDomain
    )
{
    DWORD dwError = 0;
    PGUID_LIST pGUIDTmpList = pGUIDList;
    PSTR pszPolicyIdentifier = NULL;
    PSTR pszGUID = NULL;
    PSTR pszPolicyName = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    PGROUP_POLICY_OBJECT pGPOList = NULL;
    DWORD dwPolicyType = 0;

    while(pGUIDTmpList)
    {
        dwPolicyType = 0;

        dwError = MapPolicyToGUID(
                    pGUIDTmpList,
                    pGUIDTmpList->dwPolicyID,
                    &pszGUID,
                    &pszPolicyName,
                    &dwPolicyType);
        BAIL_ON_LWUTIL_ERROR(dwError);

        dwError = EnumEnabledGPOs(
                     dwPolicyType,
                     pszDomain,
                     pszGUID,
                     &pGPOList);
        BAIL_ON_LWUTIL_ERROR(dwError);

        if(pGPOList)
        {
            fprintf(stdout, "%s is enabled in the GPO's\n",pszPolicyName);
            for (pGPO = pGPOList; pGPO; pGPO = pGPO->pNext)
            {
                dwError =  ADUCrackFileSysPath(
                                pGPO->pszgPCFileSysPath,
                                NULL,
                                NULL,
                                &pszPolicyIdentifier);
                BAIL_ON_LWUTIL_ERROR(dwError);

                fprintf(stdout, "\tGPO name:%20s \t PolicyIdentifier: %s\n\n", pGPO->pszDisplayName,pszPolicyIdentifier);

                LW_SAFE_FREE_STRING(pszPolicyIdentifier);
            }
        }

        pGUIDTmpList = pGUIDTmpList->pNext;

        LW_SAFE_FREE_STRING(pszGUID);
        LW_SAFE_FREE_STRING(pszPolicyName);
    }

error:

    LW_SAFE_FREE_STRING(pszGUID);
    LW_SAFE_FREE_STRING(pszPolicyName);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    return dwError;
}

DWORD
ListSpecificGPSettings(
    PGROUP_POLICY_OBJECT    pGPOList,
    PSTR                    pszPolicyName,
    DWORD                   dwPolicy
    )
{
    DWORD dwError = 0;
    PSTR pszPolicyIdentifier = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;

    fprintf(stdout, "%s enabled in the below mentioned GPO's\n", pszPolicyName);

    for (pGPO = pGPOList; pGPO; pGPO = pGPO->pNext)
    {
        dwError =  ADUCrackFileSysPath(
                        pGPO->pszgPCFileSysPath,
                        NULL,
                        NULL,
                        &pszPolicyIdentifier);
        BAIL_ON_LWUTIL_ERROR(dwError);

        fprintf(stdout, "\tGPO name:%20s \t PolicyIdentifier: %s\n\n", pGPO->pszDisplayName,pszPolicyIdentifier);

        LW_SAFE_FREE_STRING(pszPolicyIdentifier);
    }

error:

    LW_SAFE_FREE_STRING(pszPolicyIdentifier);
    return dwError;

}

DWORD
DownloadSpecificGPSettings(
    PGROUP_POLICY_OBJECT    pGPOList,
    PSTR                    pszGUID,
    PSTR                    pszPolicyName,
    DWORD                   dwPolicy,
    DWORD                   dwPolicyType,
    PSTR                    pszgpo,
    PSTR                    pszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPolicyIdentifier = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    CHAR szClientGUID[PATH_MAX+1];
    BOOLEAN bEnabled = 0;
    BOOLEAN bDirExists = 0;
    BOOLEAN bMatchFound = 0;

    for (pGPO = pGPOList; pGPO; pGPO = pGPO->pNext)
    {
        if(!strcmp(pGPO->pszDisplayName,pszgpo))
        {
            memset(szClientGUID, 0, sizeof(szClientGUID));
            sprintf(szClientGUID, "{%s}", pszGUID);

            dwError = IsSettingEnabledForGPO(
                        pGPO,
                        dwPolicyType,
                        szClientGUID,
                        &bEnabled);
            BAIL_ON_LWUTIL_ERROR(dwError);

            if (bEnabled)
            {
                dwError = LWCheckDirectoryExists(
                                pszPath,
                                &bDirExists);
                BAIL_ON_LWUTIL_ERROR(dwError);

                if (!bDirExists)
                {
                    mode_t perms = S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

                    dwError = LWCreateDirectory(
                                    pszPath,
                                    perms);
                    BAIL_ON_LWUTIL_ERROR(dwError);
                }

                dwError = GetValuesForGPOSettingType(
                            pGPO,
                            dwPolicyType,
                            szClientGUID,
                            pszPath);
                BAIL_ON_LWUTIL_ERROR(dwError);

                dwError =  ADUCrackFileSysPath(
                                pGPO->pszgPCFileSysPath,
                                NULL,
                                NULL,
                                &pszPolicyIdentifier);
                BAIL_ON_LWUTIL_ERROR(dwError);

                fprintf(stdout, "Downloaded %s to %s/%s folder\n", pszPolicyName,pszPath,pszPolicyIdentifier);

                LW_SAFE_FREE_STRING(pszPolicyIdentifier);
            }

            bMatchFound = 1;
        }
    }

    if(!bMatchFound)
    {
        fprintf(stderr,"Error:Policy may be disabled or Invalid GPO passed\n");
        goto error;
    }

error:

    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    return dwError;
}

DWORD
UploadSpecificGPSettings(
    PGROUP_POLICY_OBJECT    pGPOList,
    PSTR                    pszGUID,
    PSTR                    pszPolicyName,
    DWORD                   dwPolicy,
    DWORD                   dwPolicyType,
    PSTR                    pszgpo,
    PSTR                    pszPath
    )
{
    DWORD dwError = 0;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    CHAR szClientGUID[PATH_MAX+1];
    CHAR szPath[PATH_MAX+1];
    BOOLEAN bMatchFound = 0;
    BOOLEAN bFileExists = 0;


    for (pGPO = pGPOList; pGPO; pGPO = pGPO->pNext)
    {
        if(!strcmp(pGPO->pszDisplayName,pszgpo))
        {
            memset(szClientGUID, 0, sizeof(szClientGUID));
            sprintf(szClientGUID, "{%s}", pszGUID);

            sprintf(szPath, "%s/GPT.INI", pszPath);
            dwError = LWCheckFileExists(szPath,&bFileExists);

            if(!bFileExists)
            {
                fprintf(stderr,"Error:Please verify the path provided\n");
                fprintf(stderr,"Error:Please provide the directory path where GPT.INI is present\n");

                goto error;
            }

            dwError = SaveValuesForGPOSettingType(
                        pGPO,
                        szClientGUID,
                        dwPolicyType,
                        pszPath);

            BAIL_ON_LWUTIL_ERROR(dwError);

            bMatchFound = 1;

            fprintf(stdout, "Uploaded %s to AD \n", pszPolicyName);
        }
    }

    if(!bMatchFound)
    {
        fprintf(stderr,"Error:Invalid GPO passed\n");
        goto error;
    }

error:

    return dwError;
}

VOID
MapErrorCodes(
    DWORD dwError
    )
{
    switch (dwError)
    {
        case LWUTIL_ERROR_NULL_PARAMETER:
        case LWUTIL_ERROR_NO_SUCH_ATTRIBUTE:
        case LWUTIL_ERROR_INVALID_PARAMETER:
            break;
        case LWUTIL_ERROR_INVALID_ATTRIBUTE_TYPE:
            fprintf(stderr,"Error:Parameters attribute passed is not valid\n");
            break;
        case LWUTIL_ERROR_UPN_NOT_FOUND:
            fprintf(stderr,"Error:UPN not found \n");
            break;
        case LWUTIL_ERROR_KRB5_PASSWORD_MISMATCH:
        case LWUTIL_ERROR_KRB5_PASSWORD_EXPIRED:
        case LWUTIL_ERROR_KRB5_CLOCK_SKEW:
        case LWUTIL_ERROR_KRB5_ERROR:
        case LWUTIL_ERROR_GSS_API_FAILED:
            fprintf(stderr,"Error:Kerberos authentication failed\n");
            break;
        case LWUTIL_ERROR_NO_SUCH_POLICY:
            fprintf(stderr,"Error:Specified policy is not available\n");
            break;
        case LWUTIL_ERROR_LDAP_OPEN:
        case LWUTIL_ERROR_LDAP_SET_OPTION:
        case LWUTIL_ERROR_LDAP_QUERY_FAILED:
            fprintf(stderr,"Error:LDAP query failed\n");
            break;
        case LWUTIL_ERROR_UID_NOT_FOUND:
            fprintf(stderr,"Error:UID not found\n");
            break;
        case LWUTIL_ERROR_ACCESS_DENIED:
            fprintf(stderr,"Error: Access denied to upload the directory\n");
            fprintf(stdout,"Try logging in as a domain user having an admin privilege\n");
            break;
        case LWUTIL_ERROR_CREATE_FAILED:
            fprintf(stderr,"Error:Remote open failed\n");
            break;
        case LWUTIL_ERROR_READ_FAILED:
            fprintf(stderr,"Error:Remote file read failed\n");
            break;
        case LWUTIL_ERROR_WRITE_FAILED:
            fprintf(stderr,"Error:Remote file write failed\n");
            break;
        default:
            fprintf(stderr,"Error:Please verify the command option\n");
            break;

	}

}
