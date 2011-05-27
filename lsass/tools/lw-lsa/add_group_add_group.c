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
 *        add_group.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *        Main file to add a group
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "add_group_add_group.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
DWORD
MapErrorCode(
    DWORD dwError
    );

static
PSTR
LsaGetProgramName(
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
void
ShowUsage(
    PCSTR pszProgramName
    )
{
    printf("Usage: %s [ --gid  <id> ]\n"
           "          <group>\n", pszProgramName);
}

static
DWORD
ParseArgs(
    int argc,
    PSTR argv[],
    PSTR* ppszGid,
    PSTR* ppszGroup
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_GID,
        PARSE_MODE_DONE
    } ParseMode;

    DWORD dwError = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    PSTR pszGid = NULL;
    PSTR pszGroup = NULL;

    do {

      pArg = argv[iArg++];
      if (pArg == NULL || *pArg == '\0') {
        break;
      }

      switch(parseMode) {

          case PARSE_MODE_OPEN:
          {
              if ((strcmp(pArg, "--help") == 0) ||
                     (strcmp(pArg, "-h") == 0)) {
                ShowUsage(LsaGetProgramName(argv[0]));
                exit(0);
              }
              else if (strcmp(pArg, "--gid") == 0) {
                parseMode = PARSE_MODE_GID;
              }
              else {
                  dwError = LwAllocateString(pArg, &pszGroup);
                  BAIL_ON_LSA_ERROR(dwError);
                  parseMode = PARSE_MODE_DONE;
              }

              break;
          }

          case PARSE_MODE_GID:
          {
              if (!IsUnsignedInteger(pArg))
              {
                fprintf(stderr, "Please specifiy a gid that is an unsigned integer\n");
                ShowUsage(LsaGetProgramName(argv[0]));
                exit(1);
              }

              dwError = LwAllocateString(pArg, &pszGid);
              BAIL_ON_LSA_ERROR(dwError);

              parseMode = PARSE_MODE_OPEN;

              break;
          }

          case PARSE_MODE_DONE:
          {
              ShowUsage(LsaGetProgramName(argv[0]));
              exit(1);
          }
      }

    } while (iArg < argc);

    if (parseMode != PARSE_MODE_OPEN && parseMode != PARSE_MODE_DONE)
    {
        ShowUsage(LsaGetProgramName(argv[0]));
        exit(1);
    }

    if (LW_IS_NULL_OR_EMPTY_STR(pszGroup)) {
        fprintf(stderr, "Please specify a valid group name.\n");
        ShowUsage(LsaGetProgramName(argv[0]));
        exit(1);
    }

    *ppszGid = pszGid;
    *ppszGroup = pszGroup;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszGid);
    LW_SAFE_FREE_STRING(pszGroup);

    *ppszGid = NULL;
    *ppszGroup = NULL;

    goto cleanup;
}

static
DWORD
BuildGroupInfo(
    gid_t gid,
    PCSTR pszGroupName,
    PLSA_GROUP_INFO_1* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 1;

    dwError = LwAllocateMemory(
                    sizeof(LSA_GROUP_INFO_1),
                    (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    pGroupInfo->gid = gid;

    dwError = LwAllocateString(pszGroupName, &pGroupInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);

    *ppGroupInfo = pGroupInfo;

cleanup:

    return dwError;

error:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    *ppGroupInfo = NULL;

    goto cleanup;
}

static
DWORD
AddGroup(
    PCSTR pszGid,
    PCSTR pszGroup
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PSTR pszError = NULL;
    DWORD dwGroupInfoLevel = 1;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;

    dwError = BuildGroupInfo(
                    (LW_IS_NULL_OR_EMPTY_STR(pszGid) ? 0 : (gid_t)atoi(pszGid)),
                    pszGroup,
                    &pGroupInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAddGroup(
                    hLsaConnection,
                    pGroupInfo,
                    dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszError);

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    return dwError;

error:

    switch(dwError)
    {
        case LW_ERROR_GROUP_EXISTS:
        {
            fprintf(stderr, "Error: Attempt to add a duplicate group\n");
            break;
        }
        default:
        {
            if (!LW_IS_NULL_OR_EMPTY_STR(pszError)) {
               fprintf(stderr, "Error: %s\n", pszError);
            } else {
               fprintf(stderr,
                    "Error: Failed to add group. code [%u]\n",
                    dwError);
            }
            break;
        }
    }

    goto cleanup;
}

DWORD
LsaAddGroupMain(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSTR  pszGid = NULL;
    PSTR  pszGroup = NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;

    if (geteuid() != 0) {
        fprintf(stderr, "This program requires super-user privileges.\n");
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ParseArgs(
                    argc,
                    argv,
                    &pszGid,
                    &pszGroup);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AddGroup(
                    pszGid,
                    pszGroup);
    BAIL_ON_LSA_ERROR(dwError);

    fprintf(stdout, "Successfully added group %s\n", pszGroup);

cleanup:

    LW_SAFE_FREE_STRING(pszGid);
    LW_SAFE_FREE_STRING(pszGroup);

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
                        "Failed to add group.  Error code %u (%s).\n%s\n",
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
                "Failed to add group.  Error code %u (%s).\n",
                dwError,
                LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }

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
