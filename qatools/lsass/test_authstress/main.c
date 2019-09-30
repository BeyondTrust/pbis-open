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
 *        Test Program for exercising LsaAuthenticateUser
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "stresssys.h"
#include "stressdef.h"
#include <lwstr.h>
#include <lsa/lsa.h>

int
main(
    int   argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    UINT64 llSuccess = 0;
    UINT64 llFailed = 0;
    UINT64 llIter = 0;
    PCSTR  pszLoginName = "";
    PCSTR  pszPassword  = "";
    PSTR pszMessage = NULL;
    
    if (argc < 3)
    {
        fprintf(stdout, "Usage: test_authstress <login id> <password>\n");
        exit(1);
    }

    pszLoginName = argv[1];
    pszPassword  = argv[2];

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_STRESS_ERROR(dwError);
    
    
    while (1) {
       
       llIter += 1;

       dwError = LsaAuthenticateUser(
                     hLsaConnection,
                     pszLoginName,
                     pszPassword,
                     &pszMessage);

       if (pszMessage)
       {
           fprintf(stdout,
                   "%s\n",
                   pszMessage);
       }
       LW_SAFE_FREE_STRING(pszMessage);
       
       if ( dwError == 0 )
       {
          llSuccess += 1;
       }
       else
       {
          llFailed += 1;
       }
          
       if ( ( llIter % 10 ) == 0)
       {
           fprintf(stdout,
                   "success [ %llu ]  failure [ %llu ] \n",
                   (unsigned long long)llSuccess,
                   (unsigned long long)llFailed);
       }

       sleep(1);
    }

 cleanup:
    
    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    return (dwError);

error:

    goto cleanup;
}

