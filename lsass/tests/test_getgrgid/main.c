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

