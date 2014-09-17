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

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"

#include "lsaclient.h"
#include "lsaipc.h"

static
void
ShowUsage()
{
    printf("Usage: test-authenticate <user name> <passwd>\n");
}

static
int
ParseArgs(
    int argc,
    const char* argv[],
    PCSTR* ppszUserId,
    PCSTR* ppszPasswd
    )
{
    PCSTR pszArg = NULL;
    int  ret    = 0;

    if( argc != 3 ) {
        ShowUsage();
        exit(0);
    }

    pszArg = argv[1];

    if ((strcmp(pszArg, "--help") == 0) || (strcmp(pszArg, "-h") == 0))
    {
        ShowUsage();
        exit(0);
    }

    *ppszUserId = argv[1];
    *ppszPasswd = argv[2];

    return ret;
}

int
main(
    int argc,
    const char* argv[]
    )
{
    DWORD dwError     = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PCSTR pszUserId   = NULL;
    PCSTR pszPassword   = NULL;

    dwError = ParseArgs(argc,
                        argv,
                        &pszUserId,
                        &pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAuthenticateUser(
               hLsaConnection,
               pszUserId,
               pszPassword,
               NULL);
    BAIL_ON_LSA_ERROR(dwError);

    fprintf(stdout, "Successfully authenticated user %s\n", pszUserId);

cleanup:
    return dwError;

error:
    goto cleanup;
}
