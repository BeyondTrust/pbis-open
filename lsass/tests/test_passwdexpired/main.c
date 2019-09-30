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
    printf("Usage: test-passwdexpired <user login id>\n");
}

static
DWORD
ParseArgs(
    int argc,
    char* argv[],
    PSTR* ppszLoginId
    )
{
    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszLoginId = NULL;
    BOOLEAN bUIDSpecified = FALSE;

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
            dwError = LwAllocateString(pszArg, &pszLoginId);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
    } while (iArg < argc);

    if (LW_IS_NULL_OR_EMPTY_STR(pszLoginId)) {
       fprintf(stderr, "Please specify a user login id to query for.\n");
       ShowUsage();
       exit(1);
    }

    *ppszLoginId = pszLoginId;

cleanup:
    
    return dwError;

error:

    *ppszLoginId = NULL;

    LW_SAFE_FREE_STRING(pszLoginId);

    goto cleanup;
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError    = 0;
    PSTR  pszMessage = NULL;
    PSTR  pszLoginId = NULL;
    PSTR  pszStatus  = NULL;
    int   ret;

    dwError = ParseArgs(argc, argv, &pszLoginId);
    BAIL_ON_LSA_ERROR(dwError);

    ret = passwdexpired( pszLoginId, &pszMessage );

    if (ret < 0) {
       fprintf(stderr, "Internal Error. Failed to lookup user [Login Id:%s]", pszLoginId);
       dwError = LwMapErrnoToLwError(errno);
       BAIL_ON_LSA_ERROR(dwError);
    }

    switch(ret) {
        case 0:
            pszStatus = "0: Passwd has not expired.";
            break;
        case 1:
            pszStatus = "1: Passwd expired. User must change it.";
            break;
        case 2:
            pszStatus = "2: Passwd expired. Only administrator can change it.";
            break;
    }    

    printf("Passwd expiry status:\n");
    printf("=====================\n");
    printf("Name:     %s\n", LW_IS_NULL_OR_EMPTY_STR(pszLoginId) ? "<null>" : pszLoginId);
    printf("Status:   %s\n", pszStatus);
    printf("Message:  %s\n", pszMessage);

cleanup:

    LW_SAFE_FREE_STRING(pszLoginId);

    LW_SAFE_FREE_STRING(pszMessage);

    lsa_close_log();
 
    return (dwError);

error:

    goto cleanup;
}
