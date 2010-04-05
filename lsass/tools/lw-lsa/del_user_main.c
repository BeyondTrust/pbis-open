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
 *        Driver for program to delete a user
 *
 * Authors:
 *
 *        Krishna Ganugapati (krishnag@likewisesoftware.com)
 *        Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lsaclient.h"
#include "lsaipc.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
DWORD
LsaDelUserMain(
    int   argc,
    char* argv[]
    );

static
PSTR
GetProgramName(
    PSTR pszFullProgramPath
    );

static
VOID
ShowUsage(
    PCSTR pszProgramName
    );

int
del_user_main(
    int argc,
    char* argv[]
    )
{
    return LsaDelUserMain(argc, argv);
}

static
DWORD
MapErrorCode(
    DWORD dwError
    );

static
BOOLEAN
IsUnsignedInteger(
    PCSTR pszIntegerCandidate
    );

VOID
ParseArgs(
    int argc,
    char* argv[]
    )
{
    int iArg = 1;
    const char* pArg = NULL;

    if (argc != 2)
    {
        ShowUsage(GetProgramName(argv[0]));
        exit(1);
    }

    while (iArg < argc)
    {
        pArg = argv[iArg++];
        if (pArg[0] == '-')
        {
            if ( (strcmp(pArg, "--help") == 0) ||
                 (strcmp(pArg, "-h") == 0))
            {
                ShowUsage(GetProgramName(argv[0]));
                exit(0);
            }
            else
            {
                fprintf(stderr, "Error: Invalid option \n");
                ShowUsage(GetProgramName(argv[0]));
                exit(1);
            }
        }
    }
    
}

DWORD
LsaDelUserMain(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    uid_t uid = 0;
    int   nRead = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    
    ParseArgs(argc,argv);
    
    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);
    
    nRead = sscanf(argv[1], "%u", (unsigned int*)&uid);
    if ((EOF == nRead) || (0 == nRead)) {
        dwError = LsaDeleteUserByName(
                        hLsaConnection,
                        argv[1]);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!IsUnsignedInteger(argv[1]))
    {
        fprintf(stderr, "Please use a UID which is an unsigned integer.\n");
        ShowUsage(GetProgramName(argv[0]));
        exit(0);
    }
    else
    {
        dwError = LsaDeleteUserById(
                        hLsaConnection,
                        uid);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    fprintf(stdout, "The user has been deleted successfully.\n");

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
                        "Failed to delete user.  Error code %u (%s).\n%s\n",
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
                "Failed to delete user.  Error code %u (%s).\n",
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
VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    fprintf(stdout, "Usage: %s ( <user login id> | <uid> )\n", pszProgramName);
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

static
BOOLEAN
IsUnsignedInteger(
    PCSTR pszIntegerCandidate
    )
{
    typedef enum {
        PARSE_MODE_LEADING_SPACE = 0,
        PARSE_MODE_INTEGER,
        PARSE_MODE_TRAILING_SPACE
    } ParseMode;

    ParseMode parseMode = PARSE_MODE_LEADING_SPACE;
    BOOLEAN bIsUnsignedInteger = TRUE;
    INT iLength = 0;
    INT iCharIdx = 0;
    CHAR cNext = '\0';
    
    if (LW_IS_NULL_OR_EMPTY_STR(pszIntegerCandidate))
    {
        bIsUnsignedInteger = FALSE;
        goto error;
    }
    
    iLength = strlen(pszIntegerCandidate);
    
    do {

      cNext = pszIntegerCandidate[iCharIdx++];
      
      switch(parseMode) {

          case PARSE_MODE_LEADING_SPACE:
          {
              if (isdigit((int)cNext))
              {
                  parseMode = PARSE_MODE_INTEGER;
              }
              else if (!isspace((int)cNext))
              {
                  bIsUnsignedInteger = FALSE;
              }
              break;
          }
          
          case PARSE_MODE_INTEGER:
          {
              if (isspace((int)cNext))
              {
                  parseMode = PARSE_MODE_TRAILING_SPACE;
              }
              else if (!isdigit((int)cNext))
              {
                  bIsUnsignedInteger = FALSE;
              }
              break;
          }
          
          case PARSE_MODE_TRAILING_SPACE:
          {
              if (!isspace((int)cNext))
              {
                  bIsUnsignedInteger = FALSE;
              }
              break;
          }    
      }

    } while (iCharIdx < iLength && bIsUnsignedInteger == TRUE);

    
error:

    return bIsUnsignedInteger;   
}
