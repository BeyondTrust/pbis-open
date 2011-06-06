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
 *        del_group.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *        Main file to delete a group
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "del_group_del_group.h"

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
    printf("Usage: %s <group>\n"
           "   or: %s --gid  <id>\n",
           pszProgramName,
           pszProgramName);
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

    if (!pszGid && !pszGroup)
    {
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

DWORD
LsaDelGroupMain(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    PSTR pszGid = NULL;
    PSTR pszGroup = NULL;
    
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
    
    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);
    

    if (!LW_IS_NULL_OR_EMPTY_STR(pszGid))
    {
        int   nRead = 0;
        gid_t gid   = 0;
        
        nRead = sscanf(pszGid, "%u", (unsigned int*)&gid);
        if ((nRead == EOF) || (nRead == 0))
        {
            fprintf(stderr, "An invalid group id [%s] was specified.", pszGid);

            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaDeleteGroupById(
                        hLsaConnection,
                        gid);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!LW_IS_NULL_OR_EMPTY_STR(pszGroup))
    {
        dwError = LsaDeleteGroupByName(
                        hLsaConnection,
                        pszGroup);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        ShowUsage(LsaGetProgramName(argv[0]));
        exit(1);
    }
    
    fprintf(stdout, "The group has been deleted successfully.\n");

cleanup:

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

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
                        "Failed to delete group.  Error code %u (%s).\n%s\n",
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
                "Failed to delete group.  Error code %u (%s).\n",
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
