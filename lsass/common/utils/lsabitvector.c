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
 *        lsabitvector.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Bit Vector
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
LsaBitVectorCreate(
    DWORD dwNumBits,
    PLSA_BIT_VECTOR* ppBitVector
    )
{
    DWORD dwError = 0;
    PLSA_BIT_VECTOR pBitVector = NULL;

    if (!dwNumBits)
    {
        dwError = LW_ERROR_ERRNO_ERANGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                    sizeof(LSA_BIT_VECTOR),
                    (PVOID*)&pBitVector);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    (((dwNumBits-1)/(sizeof(DWORD)*8)) + 1) * sizeof(DWORD),
                    (PVOID*)&pBitVector->pVector);
    BAIL_ON_LSA_ERROR(dwError);

    pBitVector->dwNumBits = dwNumBits;

    *ppBitVector = pBitVector;

cleanup:

    return dwError;

error:

    *ppBitVector = NULL;

    if (pBitVector)
    {
        LsaBitVectorFree(pBitVector);
    }

    goto cleanup;
}

VOID
LsaBitVectorFree(
    PLSA_BIT_VECTOR pBitVector
    )
{
    LW_SAFE_FREE_MEMORY(pBitVector->pVector);
    LwFreeMemory(pBitVector);
}

BOOLEAN
LsaBitVectorIsSet(
    PLSA_BIT_VECTOR pBitVector,
    DWORD           iBit
    )
{
    return (pBitVector->pVector &&
            (iBit < pBitVector->dwNumBits) &&
            (pBitVector->pVector[iBit/(sizeof(DWORD)*8)] & (1 << (iBit % (sizeof(DWORD)*8)))));
}

DWORD
LsaBitVectorSetBit(
    PLSA_BIT_VECTOR pBitVector,
    DWORD           iBit
    )
{
    DWORD dwError = 0;

    if (!pBitVector->pVector ||
        (iBit >= pBitVector->dwNumBits))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pBitVector->pVector[iBit/(sizeof(DWORD)*8)] |= (1 << (iBit % (sizeof(DWORD)*8)));

error:

    return dwError;
}

DWORD
LsaBitVectorUnsetBit(
    PLSA_BIT_VECTOR pBitVector,
    DWORD           iBit
    )
{
    DWORD dwError = 0;

    if (!pBitVector->pVector ||
        (iBit >= pBitVector->dwNumBits))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pBitVector->pVector[iBit/(sizeof(DWORD)*8)] &= ~(1 << (iBit %(sizeof(DWORD)*8)));

error:

    return dwError;
}

VOID
LsaBitVectorReset(
    PLSA_BIT_VECTOR pBitVector
    )
{
    if (pBitVector->pVector)
    {
        memset(pBitVector->pVector, 0, ((pBitVector->dwNumBits-1)/(sizeof(DWORD)*8)) + 1);
    }
}
