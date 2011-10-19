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
