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
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Test Program for exercising LsaAllocSecurityIdentifierFromBinary
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
    PSTR*  ppszSIDHexStr
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

    UCHAR* pucSIDByteArr = NULL;
    DWORD dwSIDByteCount = 0;

    PLSA_SECURITY_IDENTIFIER pSID = NULL;

    dwError = ParseArgs(argc, argv, &pszSIDHexStr);
    BAIL_ON_LSA_ERROR(dwError);

    printf("Converting hex SID string: \"%s\"\n", pszSIDHexStr);

    dwError = LsaHexStrToByteArray(
        pszSIDHexStr,
        NULL,
        &pucSIDByteArr,
        &dwSIDByteCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocSecurityIdentifierFromBinary(
        pucSIDByteArr, dwSIDByteCount, &pSID);
    BAIL_ON_LSA_ERROR(dwError);

    printf("LsaAllocSecurityIdentifierFromBinary() successful\n");

    dwError = LsaGetSecurityIdentifierString(pSID, &pszSIDStr);
    BAIL_ON_LSA_ERROR(dwError);

    printf("LsaGetSecurityIdentifierString() returns: \"%s\"\n", pszSIDStr);



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
    PSTR*  ppszSIDHexStr
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0
        } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszSIDHexStr = NULL;
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

                    dwError = LwAllocateString(pszArg, &pszSIDHexStr);
                    BAIL_ON_LSA_ERROR(dwError);
                }
                break;

        }

    } while (iArg < argc);

    if (LW_IS_NULL_OR_EMPTY_STR(pszSIDHexStr)) {
       fprintf(stderr, "Please specify a hex string, i.e. 01FA020101.\n");
       ShowUsage();
       exit(1);
    }

    *ppszSIDHexStr = pszSIDHexStr;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszSIDHexStr);

    *ppszSIDHexStr = NULL;

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: test-lsa_alloc_security_identifier_from_binary <hex_value_of_sid>\n");
}


