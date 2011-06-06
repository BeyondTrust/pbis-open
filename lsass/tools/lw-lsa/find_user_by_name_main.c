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
 *        Test Program for exercising LsaFindUserByName
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
#include "common.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszLoginId,
    PDWORD pdwInfoLevel
    );

static
VOID
ShowUsage();

static
VOID
PrintUserInfo_0(
    PLSA_USER_INFO_0 pUserInfo,
    BOOLEAN bAllowedLogon
    );

static
VOID
PrintUserInfo_1(
    PLSA_USER_INFO_1 pUserInfo,
    BOOLEAN bAllowedLogon
    );

static
VOID
PrintUserInfo_2(
    PLSA_USER_INFO_2 pUserInfo,
    BOOLEAN bAllowedLogon
    );

static
DWORD
MapErrorCode(
    DWORD dwError
    );

int
find_user_by_name_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSTR  pszLoginId = NULL;
    DWORD dwInfoLevel = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PVOID pUserInfo = NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    BOOLEAN bAllowedLogon = TRUE;

    dwError = ParseArgs(argc, argv, &pszLoginId, &dwInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaFindUserByName(
                    hLsaConnection,
                    pszLoginId,
                    dwInfoLevel,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckUserInList(
                      hLsaConnection,
                      ((PLSA_USER_INFO_0)pUserInfo)->pszName,
                      NULL);
    if (dwError)
    {
        bAllowedLogon = FALSE;
        dwError = 0;
    }
    
    switch(dwInfoLevel)
    {
        case 0:
            PrintUserInfo_0((PLSA_USER_INFO_0)pUserInfo,
                            bAllowedLogon);
            break;
        case 1:
            PrintUserInfo_1((PLSA_USER_INFO_1)pUserInfo,
                            bAllowedLogon);
            break;
        case 2:
            PrintUserInfo_2((PLSA_USER_INFO_2)pUserInfo,
                            bAllowedLogon);
            break;
        default:
            
            fprintf(stderr, "Error: Invalid user info level [%u]\n", dwInfoLevel);
            break;
    }

cleanup:

    if (pUserInfo) {
       LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }
    
    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    LW_SAFE_FREE_STRING(pszLoginId);

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
                        "Failed to locate user.  Error code %u (%s).\n%s\n",
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
                "Failed to locate user.  Error code %u (%s).\n",
                dwError,
                LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszLoginId,
    PDWORD pdwInfoLevel
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_LEVEL,
            PARSE_MODE_DONE
        } ParseMode;
        
    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszLoginId = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    DWORD dwInfoLevel = 0;

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
                else if (!strcmp(pszArg, "--level")) {
                    parseMode = PARSE_MODE_LEVEL;
                }
                else
                {
                    dwError = LwAllocateString(pszArg, &pszLoginId);
                    BAIL_ON_LSA_ERROR(dwError);
                }
                break;
                
            case PARSE_MODE_LEVEL:
                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "Please enter an info level which is an unsigned integer.\n");
                    ShowUsage();
                    exit(1); 
                }
                dwInfoLevel = atoi(pszArg);
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
    
    if (LW_IS_NULL_OR_EMPTY_STR(pszLoginId)) {
       fprintf(stderr, "Please specify a user login id to query for.\n");
       ShowUsage();
       exit(1);
    }

    *ppszLoginId = pszLoginId;
    *pdwInfoLevel = dwInfoLevel;

cleanup:
    
    return dwError;

error:

    *ppszLoginId = NULL;
    *pdwInfoLevel = 0;

    LW_SAFE_FREE_STRING(pszLoginId);

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: lw-find-user-by-name {--level [0, 1, 2]} <user login id>\n");
}

VOID
PrintUserInfo_0(
    PLSA_USER_INFO_0 pUserInfo,
    BOOLEAN bAllowedLogon
    )
{
    fprintf(stdout, "User info (Level-0):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:              %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszName) ? "<null>" : pUserInfo->pszName);
    fprintf(stdout, "SID:               %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszSid) ? "<null>" : pUserInfo->pszSid);
    fprintf(stdout, "Uid:               %u\n", (unsigned int)pUserInfo->uid);
    fprintf(stdout, "Gid:               %u\n", (unsigned int)pUserInfo->gid);
    fprintf(stdout, "Gecos:             %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszGecos) ? "<null>" : pUserInfo->pszGecos);
    fprintf(stdout, "Shell:             %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszShell) ? "<null>" : pUserInfo->pszShell);
    fprintf(stdout, "Home dir:          %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszHomedir) ? "<null>" : pUserInfo->pszHomedir);
    fprintf(stdout, "Logon restriction: %s\n",
            bAllowedLogon ? "NO" : "YES");
}

VOID
PrintUserInfo_1(
    PLSA_USER_INFO_1 pUserInfo,
    BOOLEAN bAllowedLogon
    )
{
    fprintf(stdout, "User info (Level-1):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:              %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszName) ? "<null>" : pUserInfo->pszName);
    fprintf(stdout, "SID:               %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszSid) ? "<null>" : pUserInfo->pszSid);
    fprintf(stdout, "UPN:               %s\n",
                    LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszUPN) ? "<null>" : pUserInfo->pszUPN);
    fprintf(stdout, "Generated UPN:     %s\n", pUserInfo->bIsGeneratedUPN ? "YES" : "NO");
    fprintf(stdout, "Uid:               %u\n", (unsigned int)pUserInfo->uid);
    fprintf(stdout, "Gid:               %u\n", (unsigned int)pUserInfo->gid);
    fprintf(stdout, "Gecos:             %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszGecos) ? "<null>" : pUserInfo->pszGecos);
    fprintf(stdout, "Shell:             %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszShell) ? "<null>" : pUserInfo->pszShell);
    fprintf(stdout, "Home dir:          %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszHomedir) ? "<null>" : pUserInfo->pszHomedir);
    fprintf(stdout, "LMHash length:     %u\n", pUserInfo->dwLMHashLen);
    fprintf(stdout, "NTHash length:     %u\n", pUserInfo->dwNTHashLen);
    fprintf(stdout, "Local User:        %s\n", pUserInfo->bIsLocalUser ? "YES" : "NO");
    fprintf(stdout, "Logon restriction: %s\n", bAllowedLogon ? "NO" : "YES");
}

VOID
PrintUserInfo_2(
    PLSA_USER_INFO_2 pUserInfo,
    BOOLEAN bAllowedLogon
    )
{
    fprintf(stdout, "User info (Level-2):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:                         %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszName) ? "<null>" : pUserInfo->pszName);
    fprintf(stdout, "SID:                          %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszSid) ? "<null>" : pUserInfo->pszSid);
    fprintf(stdout, "UPN:                          %s\n",
                    LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszUPN) ? "<null>" : pUserInfo->pszUPN);
    fprintf(stdout, "Generated UPN:                %s\n", pUserInfo->bIsGeneratedUPN ? "YES" : "NO");
    fprintf(stdout, "DN:                           %s\n",
                    LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszDN) ? "<null>" : pUserInfo->pszDN);
    fprintf(stdout, "Uid:                          %u\n", (unsigned int)pUserInfo->uid);
    fprintf(stdout, "Gid:                          %u\n", (unsigned int)pUserInfo->gid);
    fprintf(stdout, "Gecos:                        %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszGecos) ? "<null>" : pUserInfo->pszGecos);
    fprintf(stdout, "Shell:                        %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszShell) ? "<null>" : pUserInfo->pszShell);
    fprintf(stdout, "Home dir:                     %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszHomedir) ? "<null>" : pUserInfo->pszHomedir);
    fprintf(stdout, "LMHash length:                %u\n", pUserInfo->dwLMHashLen);
    fprintf(stdout, "NTHash length:                %u\n", pUserInfo->dwNTHashLen);
    fprintf(stdout, "Local User:                   %s\n", pUserInfo->bIsLocalUser ? "YES" : "NO");
    fprintf(stdout, "Account disabled (or locked): %s\n",
            pUserInfo->bAccountDisabled ? "TRUE" : "FALSE");
    fprintf(stdout, "Account expired:              %s\n",
            pUserInfo->bAccountExpired ? "TRUE" : "FALSE");
    fprintf(stdout, "Password never expires:       %s\n",
            pUserInfo->bPasswordNeverExpires ? "TRUE" : "FALSE");
    fprintf(stdout, "Password expired:             %s\n",
            pUserInfo->bPasswordExpired ? "TRUE" : "FALSE");
    fprintf(stdout, "Prompt for password change:   %s\n",
            pUserInfo->bPromptPasswordChange ? "YES" : "NO");
    fprintf(stdout, "User can change password:     %s\n",
            pUserInfo->bUserCanChangePassword ? "YES" : "NO");
    fprintf(stdout, "Days till password expires:   %u\n",
            pUserInfo->dwDaysToPasswordExpiry);
    fprintf(stdout, "Logon restriction:            %s\n",
            bAllowedLogon ? "NO" : "YES");
}

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
