#include "includes.h"


static
void
ParseArgs(
    int argc,
    char *argv[],
    PSTR *ppUsersFilename,
    PSTR *ppGroupsFilename,
    PSTR *ppInvalidFilename,
    PSTR *ppszLogFilename,
    int *pnLogLevel,
    int *pnAppend
    );

static
void
ShowUsage(
    );


DWORD
Lwt_LsaTestSetup(
    int argc,
    char *argv[],
    HANDLE *phLsaConnection,
    PTESTDATA *ppTestData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    HANDLE hLsaConnection = NULL;

    PSTR pszUsersFilename = NULL;
    PSTR pszGroupsFilename = NULL;
    PSTR pszInvalidFileName = NULL;
    PSTR pszLogFilename = NULL;
    int nLogLevel = 0;
    int nAppend = 0;

    PLWTDATAIFACE pLwtUserIface = NULL;
    PLWTDATAIFACE pLwtGroupIface = NULL;
    PLWTDATAIFACE pLwtInvalidDataIface = NULL;
    DWORD dwMaxGroup = 0;
    DWORD dwMaxUser = 0;
    DWORD dwMaxInvalidDataSet = 0;
    int nDataFormat = -1;    /* Update it from the config file */

    PLWTCSV  pGroupInfo = NULL;

    PTESTDATA pTestData = NULL;

    ParseArgs(
        argc,
        argv,
        &pszUsersFilename,
        &pszGroupsFilename,
        &pszInvalidFileName,
        &pszLogFilename,
        &nLogLevel,
        &nAppend);
    nDataFormat = LWT_DATA_CSV; /* FIXME -- hardcoding to CSV. */

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LWT_ERROR(dwError);

    if(nDataFormat == LWT_DATA_CSV)
    {
        if ( !IsNullOrEmpty(pszUsersFilename) )
        {
            dwError = Csv_LoadInterface(&pLwtUserIface);
            BAIL(dwError);

            dwError = InitialiseUserInfo(pszUsersFilename, &pLwtUserIface, &dwMaxUser);
            BAIL(dwError);
        }
        if ( !IsNullOrEmpty(pszGroupsFilename))
        {
            dwError = Csv_LoadInterface(&pLwtGroupIface);
            BAIL(dwError);

            dwError = InitializeGroupInfo(pszGroupsFilename, &pLwtGroupIface, &dwMaxGroup);
            BAIL(dwError);
        }
        if ( !IsNullOrEmpty(pszInvalidFileName) )
        {
            dwError = Csv_LoadInterface(&pLwtInvalidDataIface);
            BAIL(dwError);

            dwError = InitialiseInvalidDataSet(pszInvalidFileName, &pLwtInvalidDataIface, &dwMaxInvalidDataSet);
            BAIL(dwError);
        }

    }
    else if (nDataFormat == LWT_DATA_LDIF)
    {
        if ( !IsNullOrEmpty(pszUsersFilename) )
        {

            dwError = Ldif_LoadInterface(&pLwtUserIface);
            BAIL(dwError);

            InitialiseUserInfo(pszUsersFilename, &pLwtUserIface, &dwMaxUser);
            BAIL(dwError);
        }
        if ( !IsNullOrEmpty(pszGroupsFilename))
        {
            dwError = Ldif_LoadInterface(&pLwtGroupIface);
            BAIL(dwError);

            dwError = InitializeGroupInfo(
                        pszGroupsFilename,
                        &pLwtGroupIface,
                        &dwMaxGroup);
            BAIL(dwError);
        }
    }
    else
    {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL(dwError);
    }

    dwError = LwtInitLogging(pszLogFilename, nAppend, nLogLevel);
    BAIL(dwError);

    dwError = LwAllocateMemory(sizeof(TESTDATA), (PVOID)&pTestData);
    BAIL_ON_LWT_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszUsersFilename);
    LW_SAFE_FREE_STRING(pszGroupsFilename);

    if ( pTestData )
    {
        pTestData->pUserIface =  pLwtUserIface;
        pTestData->pGroupIface = pLwtGroupIface;
        pTestData->pInvalidDataIface = pLwtInvalidDataIface;
        pTestData->dwNumGroups = dwMaxGroup;
        pTestData->dwNumUsers = dwMaxUser;
        pTestData->dwNumInvalidDataSet = dwMaxInvalidDataSet;
    }

    *phLsaConnection = hLsaConnection;
    *ppTestData = pTestData;

    return dwError;

error:

    if(pGroupInfo)
    {
        DestroyGroupInfo(pLwtGroupIface);
        pGroupInfo = NULL;
    }

    if ( hLsaConnection != (HANDLE)NULL) 
    {
        LsaCloseServer(hLsaConnection);
        hLsaConnection = NULL;
    }

    LW_SAFE_FREE_MEMORY(pTestData);

    LwtShutdownLogging();

    goto cleanup;
}


DWORD
Lwt_LsaTestTeardown(
    HANDLE *phLsaConnection,
    PTESTDATA *ppTestData
    )
{
    if ( ppTestData && *ppTestData != NULL )
    {
        PTESTDATA pTestData = *ppTestData;
    
        if ( pTestData->pUserIface)
        {
            DestroyUserInfo(pTestData->pUserIface);
            LW_SAFE_FREE_MEMORY(pTestData->pUserIface);
        }

        if (pTestData->pGroupIface)
        {
            DestroyGroupInfo(pTestData->pGroupIface);
            LW_SAFE_FREE_MEMORY(pTestData->pGroupIface);
        }

        LW_SAFE_FREE_MEMORY(pTestData);
        *ppTestData = NULL;
    }

    if ( phLsaConnection && *phLsaConnection != (HANDLE)NULL) 
    {
        LsaCloseServer(*phLsaConnection);
        *phLsaConnection = NULL;
    }

    LwtShutdownLogging();

    return LW_ERROR_SUCCESS;
}


static
void
ShowUsage(
    )
{
    /* FIXME */
    fprintf(stderr,  "[--users <user-csv-file>] [--groups <group-csv-file>] [--invalid <invalid-csv-file>] [--log-level <[0, 1, 2]>] [--log-file <file> [--append]]\n");
}

static
void
ParseArgs(
    int argc,
    char *argv[],
    PSTR *ppszUsersFilename,
    PSTR *ppszGroupsFilename,
    PSTR *ppszInvalidFilename,
    PSTR *ppszLogFilename,
    int *pnLogLevel,
    int *pnAppend
    )
{
    DWORD dwError = 0;
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_USERS_FILE,
        PARSE_MODE_GROUPS_FILE,
        PARSE_MODE_INVALID_FILE,
        PARSE_MODE_LOG_FILE,
        PARSE_MODE_LOG_LEVEL
    } ParseMode;

    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pszArg = NULL;

    PSTR pszUsersFilename = NULL;
    PSTR pszGroupsFilename = NULL;
    PSTR pszInvalidFilename = NULL;
    PSTR pszLogFilename = NULL;
    int nLogLevel = 0;
    int nAppend = 0;

    if ( argv[1] == NULL )
    {
        ShowUsage();
        exit(0);
    }

    do {
        pszArg = argv[iArg++];
        if ( pszArg == NULL || *pszArg == '\0' )
        {
            break;
        }

        switch ( parseMode )
        {
            case PARSE_MODE_OPEN:
                if ((strcmp(pszArg, "--help") == 0 ) ||
                    (strcmp(pszArg, "-h") == 0))
                {
                    ShowUsage();
                    exit(0);
                }
                else if ( strcmp(pszArg, "--users") == 0 )
                {
                    parseMode = PARSE_MODE_USERS_FILE;
                }
                else if ( strcmp(pszArg, "--groups") == 0 )
                {
                    parseMode = PARSE_MODE_GROUPS_FILE;
                }
                else if ( strcmp(pszArg, "--invalid") == 0 )
                {
                    parseMode = PARSE_MODE_INVALID_FILE;
                }
                else if ( strcmp(pszArg, "--log-file") == 0 )
                {
                    parseMode = PARSE_MODE_LOG_FILE;
                }
                else if ( strcmp(pszArg, "--log-level") == 0 )
                {
                    parseMode = PARSE_MODE_LOG_LEVEL;
                }
                else if ( strcmp(pszArg, "--append") == 0 )
                {
                    nAppend = 1;
                    parseMode = PARSE_MODE_OPEN;
                }
                else
                {
                    ShowUsage();
                    exit(0);
                }
                break;
            case PARSE_MODE_USERS_FILE:
                dwError = LwAllocateString(pszArg, &pszUsersFilename);
                BAIL_ON_LWT_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;
                break;

            case PARSE_MODE_GROUPS_FILE:
                dwError = LwAllocateString(pszArg, &pszGroupsFilename);
                BAIL_ON_LWT_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;
                break;
            
            case PARSE_MODE_INVALID_FILE:
                dwError = LwAllocateString(pszArg, &pszInvalidFilename);
                BAIL_ON_LWT_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;
                break;

            case PARSE_MODE_LOG_FILE:
                dwError = LwAllocateString(pszArg, &pszLogFilename);
                BAIL_ON_LWT_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;
                break;
            case PARSE_MODE_LOG_LEVEL:
                nLogLevel = atoi(pszArg);
                if ( nLogLevel < 0 || nLogLevel > 2 )
                {
                    ShowUsage();
                    goto error;
                }
                parseMode = PARSE_MODE_OPEN;
                break;
        }
    } while ( iArg < argc );

    *ppszUsersFilename = pszUsersFilename;
    *ppszGroupsFilename = pszGroupsFilename;
    *ppszInvalidFilename = pszInvalidFilename;
    *ppszLogFilename = pszLogFilename;
    *pnLogLevel = nLogLevel;
    *pnAppend = nAppend;
    return;

error:
    LW_SAFE_FREE_STRING(pszUsersFilename);
    LW_SAFE_FREE_STRING(pszGroupsFilename);
    LW_SAFE_FREE_STRING(pszInvalidFilename);
    LW_SAFE_FREE_STRING(pszLogFilename);
    exit(0);
}
