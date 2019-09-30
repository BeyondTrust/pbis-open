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
