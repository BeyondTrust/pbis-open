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

NTSTATUS
LwioBitVectorCreate(
    ULONG             ulNumBits,
    PLWIO_BIT_VECTOR* ppBitVector
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_BIT_VECTOR pBitVector = NULL;

    if (!ulNumBits)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LwIoAllocateMemory(sizeof(LWIO_BIT_VECTOR), (PVOID*)&pBitVector);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwIoAllocateMemory(
                    (((ulNumBits-1)/(sizeof(ULONG)*8)) + 1) * sizeof(ULONG),
                    (PVOID*)&pBitVector->pVector);
    BAIL_ON_NT_STATUS(ntStatus);

    pBitVector->ulNumBits = ulNumBits;

    *ppBitVector = pBitVector;

cleanup:

    return ntStatus;

error:

    *ppBitVector = NULL;

    if (pBitVector)
    {
        LwioBitVectorFree(pBitVector);
    }

    goto cleanup;
}

BOOLEAN
LwioBitVectorIsSet(
    PLWIO_BIT_VECTOR pBitVector,
    ULONG            iBit
    )
{
    return (pBitVector->pVector &&
            (iBit < pBitVector->ulNumBits) &&
            (pBitVector->pVector[iBit/(sizeof(ULONG)*8)] & (1 << (iBit % (sizeof(ULONG)*8)))));
}

NTSTATUS
LwioBitVectorSetBit(
    PLWIO_BIT_VECTOR pBitVector,
    ULONG            iBit
    )
{
    NTSTATUS ntStatus = 0;

    if (!pBitVector->pVector ||
        (iBit >= pBitVector->ulNumBits))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pBitVector->pVector[iBit/(sizeof(ULONG)*8)] |= (1 << (iBit % (sizeof(ULONG)*8)));

error:

    return ntStatus;
}

NTSTATUS
LwioBitVectorUnsetBit(
    PLWIO_BIT_VECTOR pBitVector,
    ULONG           iBit
    )
{
    NTSTATUS ntStatus = 0;

    if (!pBitVector->pVector || (iBit >= pBitVector->ulNumBits))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pBitVector->pVector[iBit/(sizeof(ULONG)*8)] &= ~(1 << (iBit %(sizeof(ULONG)*8)));

error:

    return ntStatus;
}

NTSTATUS
LwioBitVectorFirstUnsetBit(
    PLWIO_BIT_VECTOR pBitVector,
    PULONG           pulUnsetBit
    )
{
    NTSTATUS ntStatus = 0;
    ULONG    ulUnsetBit = 0;
    ULONG    ulNSets = 0;
    ULONG    iSet = 0;
    BOOLEAN bFound = FALSE;

    if (!pBitVector->pVector)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulNSets = (pBitVector->ulNumBits/(sizeof(ULONG) * 8)) + 1;
    for (; !bFound && (iSet < ulNSets); iSet++)
    {
        ULONG val = pBitVector->pVector[iSet];
        if (val == UINT32_MAX)
        {
            ulUnsetBit += (sizeof(ULONG) * 8);
        }
        else
        {
            ULONG idx = 0;
            for (; idx < sizeof(ULONG) * 8; idx++)
            {
                if (!(val & (1 << idx)))
                {
                    bFound = TRUE;
                    break;
                }
            }
            ulUnsetBit += idx;
        }
    }

    if (!bFound || (ulUnsetBit >= pBitVector->ulNumBits))
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pulUnsetBit = ulUnsetBit;

cleanup:

    return ntStatus;

error:

    *pulUnsetBit = 0;

    goto cleanup;
}

VOID
LwioBitVectorReset(
    PLWIO_BIT_VECTOR pBitVector
    )
{
    if (pBitVector->pVector)
    {
        memset(pBitVector->pVector, 0, ((pBitVector->ulNumBits-1)/(sizeof(ULONG)*8)) + 1);
    }
}

VOID
LwioBitVectorFree(
    PLWIO_BIT_VECTOR pBitVector
    )
{
    LWIO_SAFE_FREE_MEMORY(pBitVector->pVector);
    LwIoFreeMemory(pBitVector);
}

