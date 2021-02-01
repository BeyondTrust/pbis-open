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

#include "lsaclient.h"
#include "lsaipc.h"

static
void
ShowUsage()
{
    printf("Usage: test-validate <user name> <passwd>\n");
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

    dwError = LsaValidateUser(
               hLsaConnection,
               pszUserId,
               pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    fprintf(stdout, "Successfully validate user %s\n", pszUserId);

cleanup:
    return dwError;

error:
    goto cleanup;
}
