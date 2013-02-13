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
 *        Test Program for exercising LsaFindUserById
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
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
#include "lsaclient.h"

VOID
ShowUsage();

int
main(
    int argc,
    const char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PLSA_SID_INFO pNameList = NULL;
    size_t sIndex = 0;

    if (argc < 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))
    {
        ShowUsage();
        return 1;
    }

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaGetNamesBySidList(
                    hLsaConnection,
                    argc - 1,
                    (PSTR *)argv + 1,
                    &pNameList,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < argc - 1; sIndex++)
    {
        printf("Info for sid '%s':\n", argv[sIndex + 1]);
        switch(pNameList[sIndex].accountType)
        {
            case AccountType_NotFound:
                printf("Not found\n");
                break;
            case AccountType_Group:
                printf("Group\nDomain '%s'\nSamAccount '%s'\n",
                        pNameList[sIndex].pszDomainName,
                        pNameList[sIndex].pszSamAccountName);
                break;
            case AccountType_User:
                printf("User\nDomain '%s'\nSamAccount '%s'\n",
                        pNameList[sIndex].pszDomainName,
                        pNameList[sIndex].pszSamAccountName);
                break;
            default:
                printf("Unknown type\n");
        }
    }
    
cleanup:

    if (pNameList) {
       LsaFreeSIDInfoList(pNameList, argc - 1);
    }
    
    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    return (dwError);

error:

    fprintf(stderr, "Failed to get name list. Error code [%u]\n", dwError);

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: test-lsagetnamesbysidlist <sid1> <sid2> ...\n");
}
