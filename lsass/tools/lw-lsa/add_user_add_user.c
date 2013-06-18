/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        add_user.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *        Main file to add a user
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "add_user_add_user.h"

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
    printf("Usage: %s [ --home  dir ]\n"
           "          [ --shell shell ]\n"
           "          [ --uid   id ]\n"
           "          [ --group name ]\n"
           "          user\n", pszProgramName);
}

static
DWORD
ParseArgs(
    int argc,
    PSTR argv[],
    PSTR* ppszHomedir,
    PSTR* ppszShell,
    PSTR* ppszUid,
    PSTR* ppszGroup,
    PSTR* ppszLoginId
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_HOME,
        PARSE_MODE_SHELL,
        PARSE_MODE_UID,
        PARSE_MODE_GROUP,
        PARSE_MODE_DONE
    } ParseMode;

    DWORD dwError = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    PSTR pszHomedir = NULL;
    PSTR pszShell = NULL;
    PSTR pszUid = NULL;
    PSTR pszGroup = NULL;
    PSTR pszLoginId = NULL;

    do {

      pArg = argv[iArg++];
      if (pArg == NULL || *pArg == '\0') {
        break;
      }

      switch(parseMode)
      {
          case PARSE_MODE_OPEN:
          {
              if (strcmp(pArg, "--home") == 0)    {
                parseMode = PARSE_MODE_HOME;
              }
              else if ((strcmp(pArg, "--help") == 0) ||
                       (strcmp(pArg, "-h") == 0)) {
                ShowUsage(LsaGetProgramName(argv[0]));
                exit(0);
              }
              else if (strcmp(pArg, "--shell") == 0) {
                parseMode = PARSE_MODE_SHELL;
              }
              else if (strcmp(pArg, "--uid") == 0) {
                parseMode = PARSE_MODE_UID;
              }
              else if (strcmp(pArg, "--group") == 0) {
                  parseMode = PARSE_MODE_GROUP;
              }
              else {
                  dwError = LwAllocateString(pArg, &pszLoginId);
                  BAIL_ON_LSA_ERROR(dwError);
                  parseMode = PARSE_MODE_DONE;
              }

              break;
          }

          case PARSE_MODE_HOME:
          {
              dwError = LwAllocateString(pArg, &pszHomedir);
              BAIL_ON_LSA_ERROR(dwError);

              parseMode = PARSE_MODE_OPEN;

              break;
          }

          case PARSE_MODE_SHELL:
          {
               dwError = LwAllocateString(pArg, &pszShell);
               BAIL_ON_LSA_ERROR(dwError);

               parseMode = PARSE_MODE_OPEN;

               break;
          }

          case PARSE_MODE_UID:
          {
              if (!IsUnsignedInteger(pArg))
              {
                  fprintf(stderr, "Please use a UID that is an unsigned integer.\n");
                  ShowUsage(LsaGetProgramName(argv[0]));
                  exit(1);
              }

              dwError = LwAllocateString(pArg, &pszUid);
              BAIL_ON_LSA_ERROR(dwError);

              parseMode = PARSE_MODE_OPEN;

              break;
          }

          case PARSE_MODE_GROUP:
          {
               dwError = LwAllocateString(pArg, &pszGroup);
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

    if (LW_IS_NULL_OR_EMPTY_STR(pszLoginId)) {
        fprintf(stderr, "Please specify a valid login id.\n");
        ShowUsage(LsaGetProgramName(argv[0]));
        exit(1);
    }

    *ppszUid = pszUid;
    *ppszGroup = pszGroup;
    *ppszLoginId = pszLoginId;
    *ppszShell = pszShell;
    *ppszHomedir = pszHomedir;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszUid);
    LW_SAFE_FREE_STRING(pszGroup);
    LW_SAFE_FREE_STRING(pszLoginId);
    LW_SAFE_FREE_STRING(pszShell);
    LW_SAFE_FREE_STRING(pszHomedir);

    *ppszUid = NULL;
    *ppszGroup = NULL;
    *ppszLoginId = NULL;
    *ppszShell = NULL;
    *ppszHomedir = NULL;

    goto cleanup;
}

static
DWORD
GetGroupId(
    HANDLE hLsaConnection,
    PCSTR  pszGroupName,
    gid_t* pGid
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 0;
    gid_t gid = 0;

    dwError = LsaFindGroupByName(
                    hLsaConnection,
                    pszGroupName,
                    0,
                    dwGroupInfoLevel,
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    gid = ((PLSA_GROUP_INFO_0)pGroupInfo)->gid;

    *pGid = gid;

cleanup:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    return dwError;

error:

    *pGid = 0;

    goto cleanup;
}

static
DWORD
BuildUserInfo(
    uid_t uid,
    gid_t gid,
    PCSTR pszLoginId,
    PCSTR pszShell,
    PCSTR pszHomedir,
    PLSA_USER_INFO_0* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;

    dwError = LwAllocateMemory(
                   sizeof(LSA_USER_INFO_0),
                   (PVOID*)&pUserInfo
                   );
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->uid = uid;
    pUserInfo->gid = gid;

    dwError = LwAllocateString(pszLoginId, &pUserInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszShell)
    {
        dwError = LwAllocateString(pszShell, &pUserInfo->pszShell);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszHomedir)
    {
        dwError = LwAllocateString(pszHomedir, &pUserInfo->pszHomedir);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // TODO: Gecos

    *ppUserInfo = pUserInfo;

cleanup:

    return dwError;

error:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    goto cleanup;
}

static
DWORD
AddUser(
    PCSTR pszHomedir,
    PCSTR pszShell,
    PCSTR pszUid,
    PCSTR pszGroup,
    PCSTR pszLoginId
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PSTR pszError = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    gid_t gid = 0;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszGroup) {
        dwError = GetGroupId(
                        hLsaConnection,
                        pszGroup,
                        &gid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = BuildUserInfo(
                   LW_IS_NULL_OR_EMPTY_STR(pszUid) ? 0 : (uid_t)atoi(pszUid),
                   gid,
                   pszLoginId,
                   pszShell,
                   pszHomedir,
                   &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAddUser(
                 hLsaConnection,
                 pUserInfo,
                 dwUserInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszError);

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;

error:
    switch(dwError)
    {
        case LW_ERROR_USER_EXISTS:
        {
            fprintf(stderr, "Error: Attempt to add a duplicate user\n");
            break;
        }
        default:
        {
            if (!LW_IS_NULL_OR_EMPTY_STR(pszError)) {
               fprintf(stderr, "Error: %s\n", pszError);
            } else {
               fprintf(stderr,
                    "Error: Failed to add user. code [%u]\n",
                    dwError);
            }
            break;
        }
    }

    goto cleanup;
}

DWORD
LsaAddUserMain(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSTR  pszHomedir = NULL;
    PSTR  pszShell = NULL;
    PSTR  pszUid = NULL;
    PSTR  pszGroup = NULL;
    PSTR  pszLoginId = NULL;
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
                    &pszHomedir,
                    &pszShell,
                    &pszUid,
                    &pszGroup,
                    &pszLoginId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AddUser(
                    pszHomedir,
                    pszShell,
                    pszUid,
                    pszGroup,
                    pszLoginId
                    );
    BAIL_ON_LSA_ERROR(dwError);

    fprintf(stdout, "Successfully added user %s\n", pszLoginId);

cleanup:

    LW_SAFE_FREE_STRING(pszHomedir);
    LW_SAFE_FREE_STRING(pszShell);
    LW_SAFE_FREE_STRING(pszUid);
    LW_SAFE_FREE_STRING(pszGroup);
    LW_SAFE_FREE_STRING(pszLoginId);

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
                        "Failed to add user.  Error code %u (%s).\n%s\n",
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
                "Failed to add user.  Error code %u (%s).\n",
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

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
