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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Tool to enumerate NSS Maps
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"

#include "lsaclient.h"
#include "lsaipc.h"

#define YPMATCH_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "" )

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
DWORD
ParseArgs(
    int      argc,
    char*    argv[],
    PSTR*    ppszKeyName,
    PSTR*    ppszMapName,
    PSTR*    ppszDomain,
    PBOOLEAN pbPrintKeys,
    PBOOLEAN pbPrintNicknameTable,
    PBOOLEAN pbUseNicknameTable
    );

static
VOID
ShowUsage();

static
DWORD
FindUserByName(
    HANDLE  hLsaConnection,
    PCSTR   pszUserName,
    BOOLEAN bPrintKeys
    );

static
DWORD
FindGroupByName(
    HANDLE  hLsaConnection,
    PCSTR   pszGroupName,
    BOOLEAN bPrintKeys
    );

static
DWORD
FindMapByName(
    HANDLE  hLsaConnection,
    PCSTR   pszMapName,
    PCSTR   pszKeyName,
    BOOLEAN bPrintKeys
    );

static
VOID
PrintUserInfo_2(
    PLSA_USER_INFO_2 pUserInfo,
    BOOLEAN bPrintKeys
    );

static
VOID
PrintGroupInfo_1(
    PLSA_GROUP_INFO_1 pGroupInfo,
    BOOLEAN bPrintKeys
    );

static
VOID
PrintMapInfo_0(
    PLSA_NSS_ARTEFACT_INFO_0 pMapInfo,
    BOOLEAN bPrintKeys
    );

static
VOID
PrintNicknameTable(
    PDLINKEDLIST pNicknameList
    );

static
DWORD
MapErrorCode(
    DWORD dwError
    );

int
lw_ypmatch_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    PSTR    pszKeyName = NULL;
    PSTR    pszMapName = NULL;
    PSTR    pszDomain = NULL;
    BOOLEAN bPrintKeys = FALSE;
    BOOLEAN bPrintNicknameTable = FALSE;
    BOOLEAN bUseNicknameTable = TRUE;
    PDLINKEDLIST pNISNicknameList = NULL;
    PCSTR   pszNicknameFilePath = "/var/yp/nicknames";
    BOOLEAN bNoNicknameFile = FALSE;

    dwError = ParseArgs(
                    argc,
                    argv,
                    &pszKeyName,
                    &pszMapName,
                    &pszDomain,
                    &bPrintKeys,
                    &bPrintNicknameTable,
                    &bUseNicknameTable);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNISGetNicknames(
                    pszNicknameFilePath,
                    &pNISNicknameList);
    if (dwError == ENOENT)
    {
        bNoNicknameFile = TRUE;
        dwError = 0;
    }

    if (bPrintNicknameTable)
    {
        if (bNoNicknameFile)
        {
           printf("nickname file %s does not exist.\n", pszNicknameFilePath);
        }
        else if (pNISNicknameList)
        {
            PrintNicknameTable(pNISNicknameList);
        }

        goto cleanup;
    }

    if (bUseNicknameTable)
    {
        PCSTR pszLookupName = NULL;

        if (bNoNicknameFile)
        {
            printf("nickname file %s does not exist.\n", pszNicknameFilePath);
        }
        else if (pNISNicknameList)
        {
            pszLookupName = LsaNISLookupAlias(
                                pNISNicknameList,
                                pszMapName);

            if (pszLookupName)
            {
                LW_SAFE_FREE_STRING(pszMapName);

                dwError = LwAllocateString(
                                pszLookupName,
                                &pszMapName);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    if (!strcasecmp(pszMapName, "passwd.byname") ||
        !strcasecmp(pszMapName, "passwd"))
    {
        dwError = FindUserByName(
                        hLsaConnection,
                        pszKeyName,
                        bPrintKeys);
    }
    else if (!strcasecmp(pszMapName, "group.byname") ||
             !strcasecmp(pszMapName, "group"))
    {
        dwError = FindGroupByName(
                        hLsaConnection,
                        pszKeyName,
                        bPrintKeys);
    }
    else
    {
        dwError = FindMapByName(
                        hLsaConnection,
                        pszMapName,
                        pszKeyName,
                        bPrintKeys);
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    if (pNISNicknameList)
    {
        LsaNISFreeNicknameList(pNISNicknameList);
    }

    LW_SAFE_FREE_STRING(pszKeyName);
    LW_SAFE_FREE_STRING(pszMapName);
    LW_SAFE_FREE_STRING(pszDomain);

    return (dwError);

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
                        "Failed to find key in map.  Error code %u (%s).\n%s\n",
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
                "Failed to find key in map.  Error code %u (%s).\n",
                dwError,
                LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }

    dwError = 1;

    goto cleanup;
}

static
DWORD
ParseArgs(
    int      argc,
    char*    argv[],
    PSTR*    ppszKeyName,
    PSTR*    ppszMapName,
    PSTR*    ppszDomain,
    PBOOLEAN pbPrintKeys,
    PBOOLEAN pbPrintNicknameTable,
    PBOOLEAN pbUseNicknameTable
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_DOMAIN,
            PARSE_MODE_DONE
        } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    PSTR pszKeyName = NULL;
    PSTR pszMapName = NULL;
    PSTR* ppszValues[2] = {0};
    DWORD iValue = 0;
    DWORD dwMaxIndex = 1;
    BOOLEAN bPrintKeys = FALSE;
    BOOLEAN bUseNicknameTable = TRUE;
    BOOLEAN bPrintNicknameTable = FALSE;
    PSTR    pszDomain = NULL;

    ppszValues[0] = &pszKeyName;
    ppszValues[1] = &pszMapName;

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
                    ShowUsage();
                    exit(0);
                }
                else if (!strcmp(pszArg, "-d")) {
                    parseMode = PARSE_MODE_DOMAIN;
                }
                else if (!strcmp(pszArg, "-k"))
                {
                    bPrintKeys = TRUE;
                }
                else if (!strcmp(pszArg, "-t"))
                {
                    bUseNicknameTable = FALSE;
                }
                else if (!strcmp(pszArg, "-x"))
                {
                    bPrintNicknameTable = TRUE;
                }
                else
                {
                    if (iValue > dwMaxIndex)
                    {
                        ShowUsage();
                        exit(1);
                    }

                    dwError = LwAllocateString(
                                pszArg,
                                ppszValues[iValue]);
                    BAIL_ON_LSA_ERROR(dwError);

                    // 1. Key Name
                    // 2. Map Name
                    iValue++;
                }
                break;

            case PARSE_MODE_DOMAIN:

                LW_SAFE_FREE_STRING(pszDomain);

                dwError = LwAllocateString(
                              pszArg,
                              &pszDomain);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_DONE:
                ShowUsage();
                exit(1);
        }

    } while (iArg < argc);

    if (parseMode != PARSE_MODE_OPEN && parseMode != PARSE_MODE_DONE)
    {
        ShowUsage();
        exit(1);
    }

    if (!bPrintNicknameTable &&
        (LW_IS_NULL_OR_EMPTY_STR(pszMapName) ||
         LW_IS_NULL_OR_EMPTY_STR(pszKeyName)))
    {
        ShowUsage();
        exit(1);
    }

    *ppszMapName = pszMapName;
    *ppszKeyName = pszKeyName;
    *pbPrintKeys = bPrintKeys;
    *ppszDomain = pszDomain;
    *pbPrintNicknameTable = bPrintNicknameTable;
    *pbUseNicknameTable = bUseNicknameTable;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszKeyName);
    LW_SAFE_FREE_STRING(pszMapName);
    LW_SAFE_FREE_STRING(pszDomain);

    goto cleanup;
}

static
void
ShowUsage()
{
    printf("Usage: lw-ypmatch [-d domain] [-x] [-t] [-k] key-name map-name\n");
    printf("\n");
    printf("-k : print keys.\n");
    printf("-x : print nis nickname table.\n");
    printf("-t : do not use nickname table.\n");
}

static
DWORD
FindUserByName(
    HANDLE  hLsaConnection,
    PCSTR   pszUserName,
    BOOLEAN bPrintKeys
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoLevel = 2;
    PVOID pUserInfo = NULL;

    dwError = LsaFindUserByName(
                    hLsaConnection,
                    pszUserName,
                    dwUserInfoLevel,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    PrintUserInfo_2((PLSA_USER_INFO_2)pUserInfo, bPrintKeys);

cleanup:

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
FindGroupByName(
    HANDLE  hLsaConnection,
    PCSTR   pszGroupName,
    BOOLEAN bPrintKeys
    )
{
    DWORD dwError = 0;
    DWORD dwGroupInfoLevel = 1;
    PVOID pGroupInfo = NULL;
    LSA_FIND_FLAGS FindFlags = 0;

    dwError = LsaFindGroupByName(
                    hLsaConnection,
                    pszGroupName,
                    FindFlags,
                    dwGroupInfoLevel,
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    PrintGroupInfo_1((PLSA_GROUP_INFO_1)pGroupInfo, bPrintKeys);

cleanup:

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
FindMapByName(
    HANDLE  hLsaConnection,
    PCSTR   pszMapName,
    PCSTR   pszKeyName,
    BOOLEAN bPrintKeys
    )
{
    DWORD dwError = 0;
    DWORD dwMapInfoLevel = 0;
    PVOID  pNSSArtefactInfo = NULL;
    LSA_NIS_MAP_QUERY_FLAGS dwFlags = LSA_NIS_MAP_QUERY_ALL;

    dwError = LsaFindNSSArtefactByKey(
                    hLsaConnection,
                    dwMapInfoLevel,
                    pszKeyName,
                    pszMapName,
                    dwFlags,
                    &pNSSArtefactInfo);
    BAIL_ON_LSA_ERROR(dwError);

    PrintMapInfo_0(
            (PLSA_NSS_ARTEFACT_INFO_0)pNSSArtefactInfo,
            bPrintKeys);

cleanup:

    if (pNSSArtefactInfo) {
       LsaFreeNSSArtefactInfo(dwMapInfoLevel, pNSSArtefactInfo);
    }

    return dwError;

error:

    goto cleanup;
}

static
VOID
PrintUserInfo_2(
    PLSA_USER_INFO_2 pUserInfo,
    BOOLEAN bPrintKeys
    )
{
    if (bPrintKeys)
    {
        printf("%s ", pUserInfo->pszName);
    }

    printf("%s:%s:%u:%u:%s:%s:%s\n",
           YPMATCH_SAFE_LOG_STRING(pUserInfo->pszName),
           pUserInfo->bAccountDisabled ? "**DISABLED**" :
           pUserInfo->bAccountLocked ? "**LOCKED**" :
           pUserInfo->bAccountExpired ? "**EXPIRED**" :
           YPMATCH_SAFE_LOG_STRING(pUserInfo->pszPasswd),
           (unsigned int)pUserInfo->uid,
           (unsigned int)pUserInfo->gid,
           YPMATCH_SAFE_LOG_STRING(pUserInfo->pszGecos),
           YPMATCH_SAFE_LOG_STRING(pUserInfo->pszHomedir),
           YPMATCH_SAFE_LOG_STRING(pUserInfo->pszShell));
}

static
VOID
PrintGroupInfo_1(
    PLSA_GROUP_INFO_1 pGroupInfo,
    BOOLEAN bPrintKeys
    )
{
    PSTR* ppszMembers = NULL;

    if (bPrintKeys)
    {
        printf("%s ", pGroupInfo->pszName);
    }

    printf("%s:%s:%u:",
           YPMATCH_SAFE_LOG_STRING(pGroupInfo->pszName),
           YPMATCH_SAFE_LOG_STRING(pGroupInfo->pszPasswd),
           (unsigned int)pGroupInfo->gid);

    ppszMembers = pGroupInfo->ppszMembers;

    if (ppszMembers)
    {
        DWORD iMember = 0;

        while (!LW_IS_NULL_OR_EMPTY_STR(*ppszMembers))
        {
          if (iMember)
          {
             printf(",%s", *ppszMembers);
          }
          else
          {
             printf("%s", *ppszMembers);
          }
          iMember++;
          ppszMembers++;
       }
    }

    printf("\n");
}

static
VOID
PrintMapInfo_0(
    PLSA_NSS_ARTEFACT_INFO_0 pMapInfo,
    BOOLEAN bPrintKeys
    )
{
    if (bPrintKeys)
    {
        printf("%s ", YPMATCH_SAFE_LOG_STRING(pMapInfo->pszName));
    }
    
    printf("%s\n", YPMATCH_SAFE_LOG_STRING(pMapInfo->pszValue));
}

static
VOID
PrintNicknameTable(
    PDLINKEDLIST pNicknameList
    )
{
    PDLINKEDLIST pIter = pNicknameList;

    for (; pIter; pIter = pIter->pNext)
    {
        PLSA_NIS_NICKNAME pNickname = (PLSA_NIS_NICKNAME)pIter->pItem;

        printf("Use \"%s\" for map \"%s\"\n", pNickname->pszMapAlias, pNickname->pszMapName);
    }
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

