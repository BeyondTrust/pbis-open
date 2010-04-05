/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsabitvector.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (SMBSS)
 *
 *        Bit Vector
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
SMBBitVectorCreate(
    DWORD dwNumBits,
    PSMB_BIT_VECTOR* ppBitVector
    )
{
    DWORD dwError = 0;
    PSMB_BIT_VECTOR pBitVector = NULL;

    if (!dwNumBits)
    {
        dwError = ERANGE;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    dwError = SMBAllocateMemory(
                    sizeof(SMB_BIT_VECTOR),
                    (PVOID*)&pBitVector);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = SMBAllocateMemory(
                    (((dwNumBits-1)/(sizeof(DWORD)*8)) + 1) * sizeof(DWORD),
                    (PVOID*)&pBitVector->pVector);
    BAIL_ON_LWIO_ERROR(dwError);

    pBitVector->dwNumBits = dwNumBits;

    *ppBitVector = pBitVector;

cleanup:

    return dwError;

error:

    *ppBitVector = NULL;

    if (pBitVector)
    {
        SMBBitVectorFree(pBitVector);
    }

    goto cleanup;
}

VOID
SMBBitVectorFree(
    PSMB_BIT_VECTOR pBitVector
    )
{
    LWIO_SAFE_FREE_MEMORY(pBitVector->pVector);
    SMBFreeMemory(pBitVector);
}

BOOLEAN
SMBBitVectorIsSet(
    PSMB_BIT_VECTOR pBitVector,
    DWORD           iBit
    )
{
    return (pBitVector->pVector &&
            (iBit < pBitVector->dwNumBits) &&
            (pBitVector->pVector[iBit/(sizeof(DWORD)*8)] & (1 << (iBit % (sizeof(DWORD)*8)))));
}

DWORD
SMBBitVectorSetBit(
    PSMB_BIT_VECTOR pBitVector,
    DWORD           iBit
    )
{
    DWORD dwError = 0;

    if (!pBitVector->pVector ||
        (iBit >= pBitVector->dwNumBits))
    {
        dwError = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    pBitVector->pVector[iBit/(sizeof(DWORD)*8)] |= (1 << (iBit % (sizeof(DWORD)*8)));

error:

    return dwError;
}

DWORD
SMBBitVectorUnsetBit(
    PSMB_BIT_VECTOR pBitVector,
    DWORD           iBit
    )
{
    DWORD dwError = 0;

    if (!pBitVector->pVector ||
        (iBit >= pBitVector->dwNumBits))
    {
        dwError = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    pBitVector->pVector[iBit/(sizeof(DWORD)*8)] &= ~(1 << (iBit %(sizeof(DWORD)*8)));

error:

    return dwError;
}

DWORD
SMBBitVectorFirstUnsetBit(
    PSMB_BIT_VECTOR pBitVector,
    PDWORD  pdwUnsetBit
    )
{
    DWORD dwError = 0;
    DWORD dwUnsetBit = 0;
    DWORD dwNSets = 0;
    DWORD iSet = 0;
    BOOLEAN bFound = FALSE;

    if (!pBitVector->pVector)
    {
        dwError = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    dwNSets = (pBitVector->dwNumBits/(sizeof(DWORD) * 8)) + 1;
    for (; !bFound && (iSet < dwNSets); iSet++)
    {
        DWORD val = pBitVector->pVector[iSet];
        if (val == DWORD_MAX)
        {
            dwUnsetBit += (sizeof(DWORD) * 8);
        }
        else
        {
            DWORD idx = 0;
            for (; idx < sizeof(DWORD) * 8; idx++)
            {
                if (!(val & (1 << idx)))
                {
                    bFound = TRUE;
                    break;
                }
            }
            dwUnsetBit += idx;
        }
    }

    if (!bFound || (dwUnsetBit >= pBitVector->dwNumBits))
    {
        dwError = LWIO_ERROR_NO_BIT_AVAILABLE;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    *pdwUnsetBit = dwUnsetBit;

cleanup:

    return dwError;

error:

    *pdwUnsetBit = 0;

    goto cleanup;
}

VOID
SMBBitVectorReset(
    PSMB_BIT_VECTOR pBitVector
    )
{
    if (pBitVector->pVector)
    {
        memset(pBitVector->pVector, 0, ((pBitVector->dwNumBits-1)/(sizeof(DWORD)*8)) + 1);
    }
}
