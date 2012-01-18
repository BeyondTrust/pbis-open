/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *        Utility to manipulate the AD cache
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lsa/ad.h"

#include "lsaclient.h"
#include "lsaadprovider.h"
#include "lsaipc.h"
#include "common.h"

#define ACTION_NONE          0
#define ACTION_DELETE_ALL    1
#define ACTION_DELETE_USER   2
#define ACTION_DELETE_GROUP  3
#define ACTION_ENUM_USERS    4
#define ACTION_ENUM_GROUPS   5

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
PSTR
GetProgramName(
    PSTR pszFullProgramPath
    );

static
DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    DWORD* pdwAction,
    PSTR*  ppszDomainName,
    PSTR*  ppszName,
    uid_t* pUID,
    gid_t* pGID,
    DWORD* pdwBatchSize
    );

static
VOID
ShowUsage(
    PCSTR pszProgramName
    );

static
DWORD
EnumerateUsers(
    HANDLE hLsaConnection,
    PCSTR pszDomainName,
    DWORD  dwBatchSize
    );

static
DWORD
EnumerateGroups(
    HANDLE hLsaConnection,
    PCSTR pszDomainName,
    DWORD  dwBatchSize
    );

static
DWORD
MapErrorCode(
    DWORD dwError
    );

int
ad_cache_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    PSTR    pszOperation = "complete operation";
    DWORD   dwAction = ACTION_NONE;
    PSTR    pszDomainName = NULL;
    PSTR    pszName = NULL;
    uid_t   uid = 0;
    gid_t   gid = 0;
    DWORD   dwBatchSize = 10;

    if (argc < 2 ||
        (strcmp(argv[1], "--help") == 0) ||
        (strcmp(argv[1], "-h") == 0))
    {
        ShowUsage(GetProgramName(argv[0]));
        exit(0);
    }

    if (geteuid() != 0) {
        fprintf(stderr, "This program requires super-user privileges.\n");
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ParseArgs(
                  argc,
                  argv,
                  &dwAction,
                  &pszDomainName,
                  &pszName,
                  &uid,
                  &gid,
                  &dwBatchSize);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    switch (dwAction)
    {
        case ACTION_DELETE_ALL:
            pszOperation = "empty cache";

            dwError = LsaAdEmptyCache(
                          hLsaConnection,
                          pszDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            fprintf(stdout, "The cache has been emptied successfully.\n");

            break;

        case ACTION_DELETE_USER:
            pszOperation = "delete user";

            if ( pszName )
            {
                dwError = LsaAdRemoveUserByNameFromCache(
                              hLsaConnection,
                              pszDomainName,
                              pszName);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                dwError = LsaAdRemoveUserByIdFromCache(
                              hLsaConnection,
                              pszDomainName,
                              uid);
                BAIL_ON_LSA_ERROR(dwError);
            }

            fprintf(stdout, "The user has been deleted from the cache successfully.\n");

            break;

        case ACTION_DELETE_GROUP:
            pszOperation = "delete group";

            if ( pszName )
            {
                dwError = LsaAdRemoveGroupByNameFromCache(
                              hLsaConnection,
                              pszDomainName,
                              pszName);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                dwError = LsaAdRemoveGroupByIdFromCache(
                              hLsaConnection,
                              pszDomainName,
                              gid);
                BAIL_ON_LSA_ERROR(dwError);
            }

            fprintf(stdout, "The group has been deleted from the cache successfully.\n");

            break;

        case ACTION_ENUM_USERS:
            pszOperation = "enumerate users";

            dwError = EnumerateUsers(
                          hLsaConnection,
                          pszDomainName,
                          dwBatchSize);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case ACTION_ENUM_GROUPS:
            pszOperation = "enumerate groups";

            dwError = EnumerateGroups(
                          hLsaConnection,
                          pszDomainName,
                          dwBatchSize);
            BAIL_ON_LSA_ERROR(dwError);

        break;
    }

cleanup:

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:

    dwError = MapErrorCode(dwError);
    
    dwErrorBufferSize = LwGetErrorString(dwError, NULL, 0);
    
    if (dwErrorBufferSize > 0)
    {
        DWORD dwError2 = 0;
        PSTR   pszErrorBuffer = NULL;
        
        dwError2 = LwAllocateMemory(
                    dwErrorBufferSize,
                    (PVOID*)&pszErrorBuffer);
        
        if (!dwError2)
        {
            DWORD dwLen = LwGetErrorString(dwError, pszErrorBuffer, dwErrorBufferSize);
            
            if ((dwLen == dwErrorBufferSize) && !LW_IS_NULL_OR_EMPTY_STR(pszErrorBuffer))
            {
                fprintf(stderr,
                        "Failed to %s.  Error code %u (%s).\n%s\n",
                        pszOperation,
                        dwError,
                        LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                        pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }
        
        LW_SAFE_FREE_STRING(pszErrorBuffer);
    }
    
    if (bPrintOrigError)
    {
        fprintf(stderr,
                "Failed to %s.  Error code %u (%s).\n",
                pszOperation,
                dwError,
                LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }
    
    goto cleanup;
}

static
PSTR
GetProgramName(
    PSTR pszFullProgramPath
    )
{
    if (pszFullProgramPath == NULL || *pszFullProgramPath == '\0') {
        return NULL;
    }

    // start from end of the string
    PSTR pszNameStart = pszFullProgramPath + strlen(pszFullProgramPath);
    do {
        if (*(pszNameStart - 1) == '/') {
            break;
        }

        pszNameStart--;

    } while (pszNameStart != pszFullProgramPath);

    return pszNameStart;
}

static
DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    DWORD* pdwAction,
    PSTR*  ppszDomainName,
    PSTR*  ppszName,
    uid_t* pUID,
    gid_t* pGID,
    DWORD* pdwBatchSize
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_DOMAIN_NAME,
            PARSE_MODE_NAME,
            PARSE_MODE_UID,
            PARSE_MODE_GID,
            PARSE_MODE_BATCHSIZE,
            PARSE_MODE_DONE
    } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    DWORD dwAction = ACTION_NONE;
    PSTR  pszDomainName = NULL;
    PSTR  pszName = NULL;
    uid_t uid = 0;
    gid_t gid = 0;
    DWORD dwBatchSize = 10;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }

        switch (parseMode)
        {
            case PARSE_MODE_OPEN:

                if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
                {
                    ShowUsage(GetProgramName(argv[0]));
                    exit(0);
                }
                else if (!strcmp(pszArg, "--delete-all")) {
                    dwAction = ACTION_DELETE_ALL;
                }
                else if (!strcmp(pszArg, "--delete-user")) {
                    dwAction = ACTION_DELETE_USER;
                }
                else if (!strcmp(pszArg, "--delete-group")) {
                    dwAction = ACTION_DELETE_GROUP;
                }
                else if (!strcmp(pszArg, "--enum-users")) {
                    dwAction = ACTION_ENUM_USERS;
                }
                else if (!strcmp(pszArg, "--enum-groups")) {
                    dwAction = ACTION_ENUM_GROUPS;
                }
                else if (!strcmp(pszArg, "--domain")) {
                    parseMode = PARSE_MODE_DOMAIN_NAME;
                }
                else if (!strcmp(pszArg, "--name")) {
                    parseMode = PARSE_MODE_NAME;
                }
                else if (!strcmp(pszArg, "--uid")) {
                    parseMode = PARSE_MODE_UID;
                }
                else if (!strcmp(pszArg, "--gid")) {
                    parseMode = PARSE_MODE_GID;
                }
                else if (!strcmp(pszArg, "--batchsize")) {
                    parseMode = PARSE_MODE_BATCHSIZE;
                }
                else
                {
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }
                break;

            case PARSE_MODE_DOMAIN_NAME:
                dwError = LwAllocateString(pszArg, &pszDomainName);
                BAIL_ON_LSA_ERROR(dwError);
                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_NAME:
                dwError = LwAllocateString(pszArg, &pszName);
                BAIL_ON_LSA_ERROR(dwError);
                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_UID:
                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "Please enter a UID which is an unsigned integer.\n");
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }
                uid = atoi(pszArg);
                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_GID:
                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "Please enter a GID which is an unsigned integer.\n");
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }
                gid = atoi(pszArg);
                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_BATCHSIZE:
                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "Please enter a valid batch size.\n");
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }
                dwBatchSize = atoi(pszArg);
                if ((dwBatchSize == 0) ||
                    (dwBatchSize > 1000)) {
                    fprintf(stderr, "Please enter a valid batch size.\n");
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }
                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_DONE:
                ShowUsage(GetProgramName(argv[0]));
                exit(1);
        }

    } while (iArg < argc);

    if (parseMode != PARSE_MODE_OPEN && parseMode != PARSE_MODE_DONE)
    {
        ShowUsage(GetProgramName(argv[0]));
        exit(1);
    }

    if ( dwAction == ACTION_NONE )
    {
        fprintf(stderr, "Please specify a valid action.\n");
        ShowUsage(GetProgramName(argv[0]));
        exit(1);
    }

    if ( dwAction == ACTION_DELETE_USER )
    {
        if ( LW_IS_NULL_OR_EMPTY_STR(pszName) && !uid )
        {
            fprintf(stderr, "Please specify name or UID.\n");
            ShowUsage(GetProgramName(argv[0]));
            exit(1);
        }
    }

    if ( dwAction == ACTION_DELETE_GROUP )
    {
        if ( LW_IS_NULL_OR_EMPTY_STR(pszName) && !gid )
        {
            fprintf(stderr, "Please specify name or GID.\n");
            ShowUsage(GetProgramName(argv[0]));
            exit(1);
        }
    }

    *pdwAction = dwAction;
    *ppszDomainName = pszDomainName;
    *ppszName = pszName;
    *pUID = uid;
    *pGID = gid;
    *pdwBatchSize = dwBatchSize;

cleanup:

    return dwError;

error:

    *pdwAction = ACTION_NONE;
    *ppszDomainName = NULL;
    *ppszName = NULL;
    *pUID = 0;
    *pGID = 0;
    *pdwBatchSize = 0;

    LW_SAFE_FREE_STRING(pszName);
    LW_SAFE_FREE_STRING(pszDomainName);

    goto cleanup;
}

static
VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    fprintf(stdout, "Usage: %s --delete-all [--domain domain]\n", pszProgramName);
    fprintf(stdout, "       %s --delete-user [--domain domain] {--name <user login id> | --uid <uid>} \n", pszProgramName);
    fprintf(stdout, "       %s --delete-group [--domain domain] {--name <group name> | --gid <gid>} \n", pszProgramName);
    fprintf(stdout, "       %s --enum-users [--domain domain] {--batchsize [1..1000]}\n", pszProgramName);
    fprintf(stdout, "       %s --enum-groups [--domain domain] {--batchsize [1..1000]}\n\n", pszProgramName);
    fprintf(stdout, "\t--delete-all        Deletes everything from the cache\n");
    fprintf(stdout, "\t--delete-user       Deletes one user from the cache\n");
    fprintf(stdout, "\t--delete-group      Deletes one group from the cache\n");
    fprintf(stdout, "\t--enum-users        Enumerates users in the cache\n");
    fprintf(stdout, "\t--enum-groups       Enumerates groups in the cache\n\n");
}

static
DWORD
EnumerateUsers(
    HANDLE hLsaConnection,
    PCSTR pszDomainName,
    DWORD  dwBatchSize
    )
{
    DWORD dwError = 0;

    PSTR   pszResume = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD  dwNumUsersFound = 0;
    DWORD  dwTotalUsersFound = 0;

    do
    {
        DWORD iUser = 0;

        LsaFreeSecurityObjectList(dwNumUsersFound, ppObjects);
        ppObjects = NULL;

        dwError = LsaAdEnumUsersFromCache(
                      hLsaConnection,
                      pszDomainName,
                      &pszResume,
                      dwBatchSize,
                      &dwNumUsersFound,
                      &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);

        if (!dwNumUsersFound)
        {
            break;
        }

        for (iUser = 0; iUser < dwNumUsersFound; iUser++)
        {
            PrintSecurityObject(ppObjects[iUser], iUser + dwTotalUsersFound, 0);
            fprintf(stdout, "\n");
        }

        dwTotalUsersFound += dwNumUsersFound;
    } while (pszResume);

    fprintf(stdout, "Total users found: %u\n", dwTotalUsersFound);

cleanup:

    LsaFreeSecurityObjectList(dwNumUsersFound, ppObjects);

    LW_SAFE_FREE_STRING(pszResume);

    return (dwError);

error:

    goto cleanup;
}

static
DWORD
EnumerateGroups(
    HANDLE hLsaConnection,
    PCSTR pszDomainName,
    DWORD  dwBatchSize
    )
{
    DWORD dwError = 0;

    PSTR   pszResume = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD  dwNumGroupsFound = 0;
    DWORD  dwTotalGroupsFound = 0;

    do
    {
        DWORD iGroup = 0;

        LsaFreeSecurityObjectList(dwNumGroupsFound, ppObjects);
        ppObjects = NULL;

        dwError = LsaAdEnumGroupsFromCache(
                      hLsaConnection,
                      pszDomainName,
                      &pszResume,
                      dwBatchSize,
                      &dwNumGroupsFound,
                      &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);

        if (!dwNumGroupsFound)
        {
            break;
        }

        for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
        {
            PrintSecurityObject(ppObjects[iGroup], iGroup + dwTotalGroupsFound, 0);
        }

        dwTotalGroupsFound += dwNumGroupsFound;
    } while (pszResume);

    fprintf(stdout, "Total groups found: %u\n", dwTotalGroupsFound);

cleanup:

    LsaFreeSecurityObjectList(dwNumGroupsFound, ppObjects);

    LW_SAFE_FREE_STRING(pszResume);

    return (dwError);

error:

    goto cleanup;
}

static
DWORD
MapErrorCode(
    DWORD dwError
    )
{
    DWORD dwError2 = dwError;
    
    switch (dwError)
    {
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:
            
            dwError2 = LW_ERROR_LSA_SERVER_UNREACHABLE;
            
            break;
            
        default:
            
            break;
    }
    
    return dwError2;
}
