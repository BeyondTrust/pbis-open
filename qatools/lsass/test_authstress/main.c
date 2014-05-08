/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
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

