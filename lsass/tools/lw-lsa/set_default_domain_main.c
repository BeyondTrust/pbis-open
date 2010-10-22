/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        set_default_domain_main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Driver for program to change default joined domain
 *
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lsa/ad.h"
#include "lsaclient.h"
#include "lsaipc.h"
#include <lw/base.h>

static
DWORD
ParseArgs(
    int argc,
    char *argv[],
    PSTR *ppszName
    );

static
VOID
ShowUsage();

static
DWORD
SetDefaultDomain(
    PSTR pszName
    );

static
DWORD
MapErrorCode(
    DWORD dwError
    );

int
set_default_domain_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    DWORD dwRetError = 0;
    PSTR pszDomainName = NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;

    if (geteuid() != 0) {
        fprintf(stderr, "This program requires super-user privileges.\n");
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ParseArgs(argc, argv,
                        &pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_STRING(pszDomainName);

    dwError = SetDefaultDomain(pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszDomainName);

    return dwError;

error:
    dwRetError = MapErrorCode(dwError);
    dwErrorBufferSize = LwGetErrorString(dwRetError, NULL, 0);
    if (dwErrorBufferSize > 0)
    {
        PSTR  pszErrorBuffer = NULL;

        dwError = LwAllocateMemory(
                     dwErrorBufferSize,
                     (PVOID*)&pszErrorBuffer);
        if (!dwError)
        {
            DWORD dwLen = 0;

            dwLen = LwGetErrorString(dwRetError,
                                      pszErrorBuffer,
                                      dwErrorBufferSize);
            if ((dwLen == dwErrorBufferSize) &&
                !LW_IS_NULL_OR_EMPTY_STR(pszErrorBuffer))
            {
                fprintf(stderr, "Failed to set default joined domain.  %s\n", pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }

        LW_SAFE_FREE_STRING(pszErrorBuffer);
    }

    if (bPrintOrigError)
    {
        fprintf(stderr, "Failed to set default joined domain. Error code [%u]\n", dwRetError);
    }

    dwError = dwRetError;

    goto cleanup;
}


static
DWORD
MapErrorCode(
    DWORD dwError
    )
{
    DWORD dwRetError = dwError;

    switch (dwError)
    {
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:
            dwRetError = LW_ERROR_LSA_SERVER_UNREACHABLE;
            break;

        default:
            break;
    }

    return dwRetError;
}


DWORD
ParseArgs(
    int argc,
    char *argv[],
    PSTR *ppszName
    )
{
    DWORD dwError = 0;
    PSTR pszName = NULL;

    if (argc < 2)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        ShowUsage();
        exit(1);
    }

    if (strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0)
    {
        ShowUsage();
        exit(0);
    }

    dwError = LwAllocateString(argv[1], &pszName);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszName = pszName;

cleanup:
    return dwError;

error:
    if (pszName)
    {
        LW_SAFE_FREE_STRING(pszName);
    }

    *ppszName = NULL;

    goto cleanup;
}

static
void
ShowUsage()
{
    printf("Usage: lw-set-default-domain <domain name>\n");
}

static
DWORD
SetDefaultDomain(
    PSTR pszName
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = NULL;

    BAIL_ON_INVALID_STRING(pszName);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pszName);

    dwError = LsaAdSetDefaultDomain(
                  hLsaConnection,
                  pszName);
    BAIL_ON_LSA_ERROR(dwError);

    fprintf(stdout, "Successfully set default joined domain to %s\n", pszName);

cleanup:
    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
