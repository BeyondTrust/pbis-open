/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

 *        Driver for program to modify machine (local domain) SID
 *
 * Authors:
 *        Rafal Szczesniak (rafal@likewise.com)
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lsaclient.h"
#include "lsaipc.h"
#include <lw/base.h>

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
DWORD
ParseArgs(
    int argc,
    char *argv[],
    PSTR *ppszSid
    );

static
VOID
ShowUsage();

static
DWORD
ValidateParameters(
    PCSTR pszSid
    );

static
DWORD
SetMachineSid(
    PSTR pszSid
    );

static
DWORD
MapErrorCode(
    DWORD dwError
    );


int
set_machine_sid_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSTR pszMachineSid = NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;

    if (geteuid() != 0) {
        fprintf(stderr, "This program requires super-user privileges.\n");
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ParseArgs(argc, argv,
                        &pszMachineSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ValidateParameters(pszMachineSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SetMachineSid(pszMachineSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pszMachineSid) {
        LW_SAFE_FREE_STRING(pszMachineSid);
    }

    return dwError;

error:

    dwError = MapErrorCode(dwError);

    dwErrorBufferSize = LwGetErrorString(dwError, NULL, 0);

    if (dwErrorBufferSize > 0)
    {
        DWORD dwError2 = 0;
        PSTR  pszErrorBuffer = NULL;

        dwError2 = LwAllocateMemory(
                     dwErrorBufferSize,
                     (PVOID*)&pszErrorBuffer);

        if (!dwError2)
        {
            DWORD dwLen = 0;

            dwLen = LwGetErrorString(dwError,
                                      pszErrorBuffer,
                                      dwErrorBufferSize);
            if ((dwLen == dwErrorBufferSize) &&
                !LW_IS_NULL_OR_EMPTY_STR(pszErrorBuffer))
            {
                fprintf(stderr,
                        "Failed to modify SID.  Error code %u (%s).\n%s\n",
                        dwError,
                        LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                        pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }

        LW_SAFE_FREE_STRING(pszErrorBuffer);
    }

    if (bPrintOrigError)
    {
        fprintf(stderr,
                "Failed to modify SID.  Error code %u (%s).\n",
                dwError,
                LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }

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

static
DWORD
ParseArgs(
    int argc,
    char *argv[],
    PSTR *ppszSid
    )
{
    DWORD dwError = 0;
    PSTR pszSid = NULL;

    if (argc < 2) {
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

    dwError = LwAllocateString(argv[1], &pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszSid = pszSid;

cleanup:
    return dwError;

error:
    if (pszSid) {
        LW_SAFE_FREE_STRING(pszSid);
    }

    *ppszSid = NULL;

    goto cleanup;
}

static
void
ShowUsage()
{
    printf("Usage: set-machine-sid <SID>\n");
}

static
DWORD
ValidateParameters(
    PCSTR pszSid
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pSid = NULL;

    BAIL_ON_INVALID_STRING(pszSid);

    ntStatus = RtlAllocateSidFromCString(&pSid,
                                         pszSid);
    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (RtlValidSid(pSid) &&
        pSid->SubAuthorityCount != 4)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (pSid)
    {
        RTL_FREE(&pSid);
    }

    return dwError;

error:
    goto cleanup;
}

static
DWORD
SetMachineSid(
    PSTR pszSid
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = NULL;

    BAIL_ON_INVALID_STRING(pszSid);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSetMachineSid(hLsaConnection,
                               pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    fprintf(stdout, "Successfully set machine SID to %s\n", pszSid);

cleanup:
    if (hLsaConnection != (HANDLE)NULL) {
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
