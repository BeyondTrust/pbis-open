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
 *        Test Program for exercising LsaAllocSecurityIdentifierFromString
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *
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

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszSIDStr
    );

VOID
ShowUsage();



int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSTR  pszSIDHexStr = NULL;
    PSTR  pszSIDStr = NULL;
    PSTR  pszSIDStrFeedback = NULL;

    UCHAR* pucSIDByteArr = NULL;
    DWORD dwSIDByteCount = 0;

    PLSA_SECURITY_IDENTIFIER pSID = NULL;

    dwError = ParseArgs(argc, argv, &pszSIDStr);
    BAIL_ON_LSA_ERROR(dwError);

    printf("Converting SID string: \"%s\"\n", pszSIDStr);

    dwError = LsaAllocSecurityIdentifierFromString(
        pszSIDStr, &pSID);
    BAIL_ON_LSA_ERROR(dwError);

    printf("LsaAllocSecurityIdentifierFromString() successful\n");

    dwError = LsaGetSecurityIdentifierString(pSID, &pszSIDStrFeedback);
    BAIL_ON_LSA_ERROR(dwError);

    if(strcmp(pszSIDStrFeedback, pszSIDStr) != 0)
    {
        fprintf(stderr, "LsaGetSecurityIdentifierString() FAILED!\n");
        fprintf(stderr, "Mismatch between original string and output string:\n");
        fprintf(stderr, "Original: %s\n", pszSIDStr);
        fprintf(stderr, "Output:   %s\n", pszSIDStrFeedback);
        dwError = LW_ERROR_INVALID_SID;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetSecurityIdentifierBinary(pSID, &pucSIDByteArr, &dwSIDByteCount);
    BAIL_ON_LSA_ERROR(dwError);

    printf("LsaGetSecurityIdentifierBinary() successful, byte count = %u\n",
           dwSIDByteCount);

    dwError = LsaByteArrayToHexStr(pucSIDByteArr, dwSIDByteCount, &pszSIDHexStr);
    BAIL_ON_LSA_ERROR(dwError);

    printf("Hex SID String: \"%s\"\n", pszSIDHexStr);



cleanup:

    LW_SAFE_FREE_STRING(pszSIDHexStr);
    LW_SAFE_FREE_STRING(pszSIDStr);
    LW_SAFE_FREE_MEMORY(pucSIDByteArr);

    if (pSID)
    {
        LsaFreeSecurityIdentifier(pSID);
    }

    return (dwError);

error:

    fprintf(stderr, "Failed to convert SID. Error code [%u]\n", dwError);

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszSIDStr
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0
        } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszSIDStr = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }

        switch (parseMode)
        {
            case PARSE_MODE_OPEN:

                if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
                {
                    ShowUsage();
                    exit(0);
                }
                else
                {

                    dwError = LwAllocateString(pszArg, &pszSIDStr);
                    BAIL_ON_LSA_ERROR(dwError);
                }
                break;

        }

    } while (iArg < argc);

    if (LW_IS_NULL_OR_EMPTY_STR(pszSIDStr)) {
       fprintf(stderr, "Please specify a SID string, i.e. S-1-5-532.\n");
       ShowUsage();
       exit(1);
    }

    *ppszSIDStr = pszSIDStr;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszSIDStr);

    *ppszSIDStr = NULL;

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: test-lsa_alloc_security_identifier_from_string <sid_string>\n");
}


