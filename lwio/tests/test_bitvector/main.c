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
 *        Likewise IO (LWIO)
 *
 *        Test Program for exercising SMB BitVector
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lwiosys.h"
#include "lwio/lwio.h"
#include "lwiodef.h"
#include "lwioutils.h"

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
    PLWIO_BIT_VECTOR pBitVector = NULL;
    DWORD dwNumBits = 100;
    DWORD dwTestBit = 51;
    DWORD iBit = 0;
    DWORD dwUnsetBit = 0;

    dwError = ParseArgs(argc, argv);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = LwioBitVectorCreate(
                  dwNumBits,
                  &pBitVector);
    BAIL_ON_LWIO_ERROR(dwError);

    // Index starts from 0
    //
    dwError = LwioBitVectorSetBit(pBitVector, 0);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = LwioBitVectorSetBit(pBitVector, dwNumBits);
    if (dwError != LWIO_ERROR_INVALID_PARAMETER)
    {
       BAIL_ON_LWIO_ERROR(dwError);
    }

    dwError = LwioBitVectorSetBit(pBitVector, dwTestBit);
    BAIL_ON_LWIO_ERROR(dwError);

    if (LwioBitVectorIsSet(pBitVector, dwTestBit))
    {
       printf("Bit Set succeeded\n");
    }
    else
    {
       printf("Bit Set failed\n");
    }

    dwError = LwioBitVectorUnsetBit(pBitVector, dwTestBit);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = LwioBitVectorUnsetBit(pBitVector, 0);
    BAIL_ON_LWIO_ERROR(dwError);

    if (LwioBitVectorIsSet(pBitVector, dwTestBit))
    {
        printf("Bit Unset failed\n");
    }
    else
    {
        printf("Bit Unset succeeded\n");
    }

    dwError = LwioBitVectorFirstUnsetBit(
                pBitVector,
                &dwUnsetBit);
    if ((dwError == LWIO_ERROR_NO_BIT_AVAILABLE) ||
        (!dwError && (dwUnsetBit != 0)))
    {
        printf("Error: Expected (0)\n");
    }
    else
    {
        printf("Bit 0 is available as expected\n");
    }

    for (iBit = 0; iBit < dwNumBits; iBit++)
    {
        if (iBit != dwTestBit)
        {
            LwioBitVectorSetBit(pBitVector, iBit);
        }
    }

    dwError = LwioBitVectorFirstUnsetBit(
                pBitVector,
                &dwUnsetBit);
    if ((dwError == LWIO_ERROR_NO_BIT_AVAILABLE) ||
        (!dwError && (dwUnsetBit != dwTestBit)))
    {
        printf("Error: Expected (%d) Found (%d)\n", dwTestBit, dwUnsetBit);
    }
    else
    {
        printf("Bit %d is available as expected\n", dwTestBit);
    }

    dwError = LwioBitVectorSetBit(pBitVector, dwTestBit);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = LwioBitVectorFirstUnsetBit(
                pBitVector,
                &dwUnsetBit);
    if (dwError != LWIO_ERROR_NO_BIT_AVAILABLE)
    {
        printf("Error: Expected no bits available. Found (%d)\n", dwUnsetBit);
    }
    else
    {
        dwError = 0;
    }

cleanup:

    if (pBitVector)
    {
        LwioBitVectorFree(pBitVector);
    }

    return (dwError);

error:

    goto cleanup;
}
