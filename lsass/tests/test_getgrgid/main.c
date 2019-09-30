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
 *        Test Program for exercising getgrgid
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
    printf("Usage: test-getgrgid <gid>\n");
}

static
int
ParseArgs(
    int argc,
    char* argv[],
    gid_t* pGID
    )
{
    int iArg = 1;
    PSTR pszArg = NULL;
    gid_t gid = 0;
    BOOLEAN bGIDSpecified = FALSE;

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
            gid = (gid_t) atoi(pszArg);
            bGIDSpecified = TRUE;
            break;
        }
    } while (iArg < argc);

    if (!bGIDSpecified) {
       fprintf(stderr, "Please specify a group id (gid) to query for.\n");
       ShowUsage();
       exit(1);
    }

    *pGID = gid;
    
    return 0;
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    struct group grp = {0};
    struct group* pGroup = NULL;
    char   szBuf[1024];
    gid_t  gid = 0;
    DWORD  iMember = 0;
    PSTR*  ppszMembers = NULL;

    dwError = ParseArgs(argc, argv, &gid);
    BAIL_ON_LSA_ERROR(dwError);

    if (getgrgid_r(gid, &grp, szBuf, sizeof(szBuf), &pGroup) < 0) {
        fprintf(stderr, "Failed to lookup group [gid:%u]", (unsigned int)gid);
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }


    printf("Group info:\n");
    printf("==========\n");
    
    if(pGroup)
    {
        printf("Name:     %s\n", LW_IS_NULL_OR_EMPTY_STR(pGroup->gr_name) ? "<null>" : pGroup->gr_name);
        printf("Gid:      %u\n", (unsigned int)pGroup->gr_gid);
        printf("Passwd:   %s\n", LW_IS_NULL_OR_EMPTY_STR(pGroup->gr_passwd) ? "<null>" : pGroup->gr_passwd);
        printf("Members: ");
        ppszMembers = pGroup->gr_mem;
        while (ppszMembers && *ppszMembers)
        {
            printf("%s%s", (iMember ? "," : ""), *ppszMembers);
            ppszMembers++;
            iMember++;
        }
        printf("\n");
    }
    else
    {
        printf("pGroup == NULL\n");
    }

cleanup:

    return (dwError);

error:

    goto cleanup;
}

