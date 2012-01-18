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
