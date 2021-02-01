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
 *        BeyondTrust Netlogon
 *
 *        Client Test Program - LWNetGetDomainController
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#include "config.h"
#include "lwnet-system.h"
#include "lwnet-def.h"
#include "lwnet.h"
#include "lwnet-utils.h"

static
void
ShowUsage()
{
    printf("Usage: netlogon-get-domain-controller <target domain FQDN>\n");
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszTargetFQDN
    )
{

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszTargetFQDN = NULL;

    do {
        pszArg = argv[iArg++];
        if (IsNullOrEmptyString(pszArg))
        {
            break;
        }
        if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
        {
            ShowUsage();
            exit(0);
        }
        else
        {
            dwError = LWNetAllocateString(pszArg, &pszTargetFQDN);
            BAIL_ON_LWNET_ERROR(dwError);
        }
        
    } while (iArg < argc);

    
    if(IsNullOrEmptyString(pszTargetFQDN))
    {
        ShowUsage();
        exit(0);
    }

    cleanup:
        *ppszTargetFQDN = pszTargetFQDN;
    return dwError;

    error:
        LWNET_SAFE_FREE_STRING(pszTargetFQDN);
        *ppszTargetFQDN = NULL;
    goto cleanup;
}

void safePrintString(
        PSTR pszStringName, 
        PSTR pszStringValue
        )
{
    if(IsNullOrEmptyString(pszStringName))
    {
        return;
    }
    else if(pszStringValue == NULL)
    {
        printf("%s = <NULL>\n", pszStringName);
    }
    else if(*pszStringValue == '\0')
    {
        printf("%s = <EMPTY>\n", pszStringName);
    }
    else
    {
        printf("%s = %s\n", pszStringName, pszStringValue);
    }
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;

    PSTR pszTargetFQDN = NULL;
    PSTR pszDomainControllerFQDN = NULL;
    
    dwError = ParseArgs(argc,
            argv,
            &pszTargetFQDN
            );
    BAIL_ON_LWNET_ERROR(dwError);

    lwnet_init_logging_to_file(LWNET_LOG_LEVEL_VERBOSE, TRUE, "");

    dwError = LWNetGetDomainController(
                pszTargetFQDN,
                &pszDomainControllerFQDN
                );
    BAIL_ON_LWNET_ERROR(dwError); 

    safePrintString("Domain Controller", pszDomainControllerFQDN);
    
    cleanup:

        LWNET_SAFE_FREE_STRING(pszTargetFQDN);
        LWNET_SAFE_FREE_STRING(pszDomainControllerFQDN);
        return (dwError);

    error:

        LWNET_LOG_ERROR("Failed communication with the LWNET Agent. Error code [%d]\n", dwError);

        goto cleanup;

}
