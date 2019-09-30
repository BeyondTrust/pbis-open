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

#include "domainjoin.h"

static
VOID
ShowUsage(
    VOID
    );

int
main(
    int   argc,
    char* argv[]
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR  pszDomainName = NULL;
    PSTR  pszComputerName = NULL;

    if (argc < 2)
    {
        ShowUsage();

        ceError = 1;
        goto error;
    }

    ceError = DJInit();
    if (ceError)
        goto error;

    if (!strcmp(argv[1], "query"))
    {
        ceError = DJQueryJoinInformation(&pszComputerName, &pszDomainName);
        if (ceError)
            goto error;

        printf("Computer name: %s\n"
               "Domain name:   %s\n",
               pszComputerName ? pszComputerName : "<null>",
               pszDomainName ? pszDomainName : "<null>");
    }
    else if (!strcmp(argv[1], "join"))
    {
        if (argc < 5)
        {
            ShowUsage();
            ceError = 1;
            goto error;
        }

        ceError = DJJoinDomain(
                        argv[2],                     /* domain    */
                        (argc > 5 ? argv[5] : NULL), /* OU */
                        argv[3],                     /* user name */
                        argv[4]                      /* password  */
                        );
        if (ceError)
            goto error;
    }
    else if (!strcmp(argv[1], "leave"))
    {
        if (argc < 4)
        {
            ShowUsage();
            ceError = 1;
            goto error;
        }

        ceError = DJUnjoinDomain(
                        argv[2], /* username */
                        argv[3]  /* password */
                        );
        if (ceError)
            goto error;
    }
    else
    {
        ShowUsage();

        ceError = 1;
        goto error;
    }
 
cleanup:

    if (pszComputerName)
    {
        DJFreeMemory(pszComputerName);
    }

    if (pszDomainName)
    {
        DJFreeMemory(pszDomainName);
    }

    DJShutdown();

    return ceError;

error:

    fprintf(stderr, "test-join-api failed. Error code: %u\n", ceError);

    goto cleanup;
}

static
VOID
ShowUsage(
    VOID
    )
{
    printf("Usage: test-join-api (query | "
                                  "(join <username> <password> <optional ou>) |"
                                  "(leave <username> <password>))\n");
}
