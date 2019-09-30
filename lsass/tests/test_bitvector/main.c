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
 *        Test Program for exercising LsaBitVector
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
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

static
void
ShowUsage()
{
    printf("Usage: test-bitvector\n");
}

static
int
ParseArgs(
    int argc,
    char* argv[]
    )
{
    int iArg = 1;
    PSTR pszArg = NULL;

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
    } while (iArg < argc);

    return 0;
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PLSA_BIT_VECTOR pBitVector = NULL;
    DWORD dwNumBits = 100;
    DWORD dwTestBit = 51;

    dwError = ParseArgs(argc, argv);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBitVectorCreate(
                  dwNumBits,
                  &pBitVector);
    BAIL_ON_LSA_ERROR(dwError);

    // Index starts from 0
    //
    dwError = LsaBitVectorSetBit(pBitVector, 0);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBitVectorSetBit(pBitVector, dwNumBits);
    if (dwError != LW_ERROR_INVALID_PARAMETER)
    {
       BAIL_ON_LSA_ERROR(dwError);
    }
       
    dwError = LsaBitVectorSetBit(pBitVector, dwTestBit);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (LsaBitVectorIsSet(pBitVector, dwTestBit))
    {
       printf("Bit Set succeeded\n");
    }
    else
    {
       printf("Bit Set failed\n");
    }

    dwError = LsaBitVectorUnsetBit(pBitVector, dwTestBit);
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaBitVectorIsSet(pBitVector, dwTestBit))
    {
        printf("Bit Unset failed\n");
    }
    else
    {
        printf("Bit Unset succeeded\n");
    }

cleanup:

    if (pBitVector)
    {
        LsaBitVectorFree(pBitVector);
    }

    return (dwError);

error:

    goto cleanup;
}
