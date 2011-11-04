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
 *        Tool to manage LSASS trace flags at runtime
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwsecurityidentifier.h"
#include "lsautils.h"
#include "lsaclient.h"

#define LSA_TRACE_FLAG_USER_GROUP_QUERIES_TEXT        "user-group-queries"
#define LSA_TRACE_FLAG_AUTHENTICATION_TEXT            "authentication"
#define LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION_TEXT "user-group-administration"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
DWORD
ParseArgs(
    int              argc,
    char*            argv[],
    PLSA_TRACE_INFO* pTraceFlagArrayToSet,
    PDWORD           pdwNumFlagsToSet,
    PDWORD           pdwTraceFlag
    );

static
DWORD
ParseTraceFlagArray(
    PCSTR            pszArg,
    PLSA_TRACE_INFO* ppTraceFlagArray,
    PDWORD           pdwNumFlags
    );

static
DWORD
ParseTraceFlag(
    PCSTR  pszArg,
    PDWORD pdwTraceFlag
    );

static
VOID
ShowUsage();

static
DWORD
PrintTraceInfo(
    PLSA_TRACE_INFO pTraceInfo
    );

static
DWORD
MapErrorCode(
    DWORD dwError
    );

int
trace_info_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    DWORD dwTraceFlag = 0;
    PLSA_TRACE_INFO pTraceFlag = NULL;
    PLSA_TRACE_INFO pTraceFlagArray = NULL;
    DWORD dwNumFlags = 0;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;

    if (argc > 1)
    {
        dwError = ParseArgs(
                        argc,
                        argv,
                        &pTraceFlagArray,
                        &dwNumFlags,
                        &dwTraceFlag);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pTraceFlagArray)
    {
        if (geteuid() != 0) {
            fprintf(stderr, "This program requires super-user privileges.\n");
            dwError = LW_ERROR_ACCESS_DENIED;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    if (pTraceFlagArray)
    {
        dwError = LsaSetTraceFlags(
                    hLsaConnection,
                    pTraceFlagArray,
                    dwNumFlags);
        BAIL_ON_LSA_ERROR(dwError);

        printf("The trace levels were set successfully\n\n");
    }

    if (dwTraceFlag)
    {
        dwError = LsaGetTraceFlag(
                    hLsaConnection,
                    dwTraceFlag,
                    &pTraceFlag);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = PrintTraceInfo(pTraceFlag);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwTraceFlag && pTraceFlagArray)
    {
       DWORD iFlag = 0;

       dwError = LsaEnumTraceFlags(
                    hLsaConnection,
                    &pTraceFlagArray,
                    &dwNumFlags);
       BAIL_ON_LSA_ERROR(dwError);

       for(; iFlag < dwNumFlags; iFlag++)
       {
           PLSA_TRACE_INFO pInfo = &pTraceFlagArray[iFlag];

           dwError = PrintTraceInfo(pInfo);
           BAIL_ON_LSA_ERROR(dwError);
       }
    }

cleanup:

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    LW_SAFE_FREE_MEMORY(pTraceFlag);
    LW_SAFE_FREE_MEMORY(pTraceFlagArray);

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
                fprintf(
                    stderr,
                    "Failed to manage trace flags.  Error code %u (%s).\n%s\n",
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
        fprintf(
            stderr,
            "Failed to manage trace flags.  Error code %u (%s).\n",
            dwError,
            LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }

    goto cleanup;
}

static
DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PLSA_TRACE_INFO* ppTraceFlagArrayToSet,
    PDWORD           pdwNumFlagsToSet,
    PDWORD           pdwTraceFlag
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_SET,
        PARSE_MODE_GET
    } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    DWORD dwTraceFlag = 0;
    PLSA_TRACE_INFO pTraceFlagArray = NULL;
    DWORD dwNumFlags = 0;

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
                else if (!strcasecmp(pszArg, "--get"))
                {
                    parseMode = PARSE_MODE_GET;
                }
                else if (!strcasecmp(pszArg, "--set"))
                {
                    parseMode = PARSE_MODE_SET;
                }
                else
                {
                   ShowUsage();
                   exit(1);
                }
                break;



           case PARSE_MODE_SET:

                LW_SAFE_FREE_MEMORY(pTraceFlagArray);
                dwNumFlags = 0;

                dwError = ParseTraceFlagArray(
                              pszArg,
                              &pTraceFlagArray,
                              &dwNumFlags);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;

           case PARSE_MODE_GET:

                dwTraceFlag = 0;

                dwError = ParseTraceFlag(
                              pszArg,
                              &dwTraceFlag);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;
        }

    } while (iArg < argc);

    *ppTraceFlagArrayToSet = pTraceFlagArray;
    *pdwNumFlagsToSet = dwNumFlags;
    *pdwTraceFlag = dwTraceFlag;

cleanup:

    return dwError;

error:

    *ppTraceFlagArrayToSet = NULL;
    *pdwNumFlagsToSet = 0;
    *pdwTraceFlag = 0;

    LW_SAFE_FREE_MEMORY(pTraceFlagArray);

    goto cleanup;
}

static
DWORD
ParseTraceFlagArray(
    PCSTR            pszArg,
    PLSA_TRACE_INFO* ppTraceFlagArray,
    PDWORD           pdwNumFlags
    )
{
    DWORD dwError = 0;
    DWORD dwNumFlags = 0;
    DWORD iFlag = 0;
    size_t sFlagLen = 0;
    PCSTR pszInput = pszArg;
    PLSA_TRACE_INFO pTraceFlagArray = NULL;
    PSTR pszFlagAndValue = NULL;
    PSTR pszFlag = NULL;

    if (*pszArg == ',')
    {
        fprintf(stderr, "Error: Invalid argument [%s]\n", pszArg);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Figure out how many flags there are
    while ((sFlagLen = strcspn(pszInput, ",")) != 0)
    {
          size_t sDelimiterLen = 0;

          pszInput += sFlagLen;

          sDelimiterLen = strspn(pszInput, ",");

          pszInput += sDelimiterLen;

          dwNumFlags++;
    }

    dwError = LwAllocateMemory(
                    dwNumFlags * sizeof(LSA_TRACE_INFO),
                    (PVOID*)&pTraceFlagArray);
    BAIL_ON_LSA_ERROR(dwError);

    pszInput = pszArg;
    while ((sFlagLen = strcspn(pszInput, ",")) != 0)
    {
          size_t sDelimiterLen = 0;
          PCSTR pszIndex = NULL;

          LW_SAFE_FREE_STRING(pszFlagAndValue);

          dwError = LwStrndup(
                        pszInput,
                        sFlagLen,
                        &pszFlagAndValue);
          BAIL_ON_LSA_ERROR(dwError);

          if (!(pszIndex = strchr(pszFlagAndValue, ':')) ||
              LW_IS_NULL_OR_EMPTY_STR(pszIndex+1) ||
              (strcmp(pszIndex+1, "0") && strcmp(pszIndex + 1, "1")))
          {
              fprintf(stderr,
                      "Error: Invalid value specified for trace flag [%s]\n",
                      pszFlagAndValue);

              dwError = LW_ERROR_INVALID_PARAMETER;
              BAIL_ON_LSA_ERROR(dwError);
          }
          else if (pszIndex == pszFlagAndValue)
          {
              fprintf(stderr,
                      "Error: No name specified for trace flag [%s]\n",
                      pszFlagAndValue);

              dwError = LW_ERROR_INVALID_PARAMETER;
              BAIL_ON_LSA_ERROR(dwError);
          }
          else
          {
              PLSA_TRACE_INFO pTraceFlag = NULL;

              LW_SAFE_FREE_STRING(pszFlag);

              dwError = LwStrndup(
                            pszFlagAndValue,
                            (pszIndex - pszFlagAndValue),
                            &pszFlag);
              BAIL_ON_LSA_ERROR(dwError);

              pTraceFlag = &pTraceFlagArray[iFlag];

              dwError = ParseTraceFlag(
                            pszFlag,
                            &pTraceFlag->dwTraceFlag);
              BAIL_ON_LSA_ERROR(dwError);

              pTraceFlag->bStatus = atoi(pszIndex + 1);
          }

          pszInput += sFlagLen;

          sDelimiterLen = strspn(pszInput, ",");

          pszInput += sDelimiterLen;

          iFlag++;
    }

    *ppTraceFlagArray = pTraceFlagArray;
    *pdwNumFlags = dwNumFlags;

cleanup:

    LW_SAFE_FREE_STRING(pszFlag);
    LW_SAFE_FREE_STRING(pszFlagAndValue);

    return dwError;

error:

    *ppTraceFlagArray = NULL;
    *pdwNumFlags = 0;

    LW_SAFE_FREE_MEMORY(pTraceFlagArray);

    goto cleanup;
}

static
DWORD
ParseTraceFlag(
    PCSTR  pszArg,
    PDWORD pdwTraceFlag
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlag = 0;

    if (!strcasecmp(pszArg, LSA_TRACE_FLAG_USER_GROUP_QUERIES_TEXT))
    {
       dwTraceFlag = LSA_TRACE_FLAG_USER_GROUP_QUERIES;
    }
    else if (!strcasecmp(pszArg, LSA_TRACE_FLAG_AUTHENTICATION_TEXT))
    {
       dwTraceFlag = LSA_TRACE_FLAG_AUTHENTICATION;
    }
    else if (!strcasecmp(pszArg, LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION_TEXT))
    {
       dwTraceFlag = LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION;
    }
    else
    {
       dwError = LW_ERROR_INVALID_PARAMETER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    *pdwTraceFlag = dwTraceFlag;

cleanup:

    return dwError;

error:

    *pdwTraceFlag = 0;

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: lw-trace-info {--set (flag-name:(0|1))(,flag-name:(0|1))*}\n"
           "                      {--get flag-name}\n");
    printf("\nValid flag names: {user-group-queries, authentication, user-group-administration}\n");
    printf("\nExample: lw-trace-info --set user-group-queries:0,authentication:1 --get user-group-administration\n");
}

DWORD
PrintTraceInfo(
    PLSA_TRACE_INFO pTraceInfo
    )
{
    switch(pTraceInfo->dwTraceFlag)
    {
        case LSA_TRACE_FLAG_USER_GROUP_QUERIES:

            fprintf(stdout, "%s\t\t: %d\n",
                    LSA_TRACE_FLAG_USER_GROUP_QUERIES_TEXT,
                    pTraceInfo->bStatus);

            break;

        case LSA_TRACE_FLAG_AUTHENTICATION:

            fprintf(stdout, "%s\t\t\t: %d\n",
                    LSA_TRACE_FLAG_AUTHENTICATION_TEXT,
                    pTraceInfo->bStatus);

            break;

        case LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION:

            fprintf(stdout, "%s\t: %d\n",
                    LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION_TEXT,
                    pTraceInfo->bStatus);

            break;

        default:

            fprintf(stdout, "<unknown>\t: %d\n",
                    pTraceInfo->bStatus);

            break;
    }

    return 0;
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
