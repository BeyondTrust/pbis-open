/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS) 
 *        
 *        Test Program for exercising getgrnam
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwsecurityidentifier.h"

      
#include "lsautils.h"

static
void
ShowUsage()
{
    printf("Usage: test-getgrnam <group id>\n");
}

static
DWORD
ParseArgs(
    int argc,
    char* argv[],
    PSTR* ppszGroupId
    )
{
    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszGroupId = NULL;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }
        
        if ((strcmp(pszArg, "--help") == 0) || (strcmp(pszArg, "-h") == 0))
        {
            ShowUsage();
            exit(0);
        }
        else
        {
            dwError = LwAllocateString(pszArg, &pszGroupId);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
    } while (iArg < argc);

    if (LW_IS_NULL_OR_EMPTY_STR(pszGroupId)) {
       fprintf(stderr, "Please specify a group id to query for.\n");
       ShowUsage();
       exit(1);
    }

    *ppszGroupId = pszGroupId;

cleanup:
    
    return dwError;

error:

    *ppszGroupId = NULL;

    LW_SAFE_FREE_STRING(pszGroupId);

    goto cleanup;
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    struct group grp = {0};
    struct group* pGrp = NULL;
    char   szBuf[1024];
    PSTR   pszGroupId = NULL;
    PSTR*  ppszMembers = NULL;
    int    iMember = 0;

    dwError = ParseArgs(argc, argv, &pszGroupId);
    BAIL_ON_LSA_ERROR(dwError);

    if (getgrnam_r(pszGroupId, &grp, szBuf, sizeof(szBuf), &pGrp) < 0) {
       fprintf(stderr, "Failed to lookup group [Group Id:%s]", pszGroupId);
       dwError = LwMapErrnoToLwError(errno);
       BAIL_ON_LSA_ERROR(dwError);
    }

    printf("Group info:\n");
    printf("==========\n");
    
    if(pGrp) 
    {
    
        printf("Name:     %s\n", LW_IS_NULL_OR_EMPTY_STR(pGrp->gr_name) ? "<null>" : pGrp->gr_name);    
        printf("Gid:      %u\n", (unsigned int)pGrp->gr_gid);    
        printf("Password: %s\n", LW_IS_NULL_OR_EMPTY_STR(pGrp->gr_passwd) ? "<null>" : pGrp->gr_passwd);
        printf("Members: ");
        ppszMembers = pGrp->gr_mem;
        while (ppszMembers && *ppszMembers)
        {
            printf("%s%s", (iMember ? "," : ""), *ppszMembers);
            ppszMembers++;
            iMember++;
        }
    }
    else
    {
        printf("pGrp == NULL\n");
    }
    printf("\n");

cleanup:

    LW_SAFE_FREE_STRING(pszGroupId);

    return (dwError);

error:

    goto cleanup;
}
