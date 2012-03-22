/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        datablob.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Data Blob data structure
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *
 */

#include "includes.h"

/**************************************************************
 */

DWORD
LsaDataBlobAllocate(
    PLSA_DATA_BLOB *ppBlob,
    DWORD dwSize
    )
{
    DWORD dwError = LW_ERROR_INTERNAL;
    LSA_DATA_BLOB *pBlob = NULL;    

    BAIL_ON_INVALID_POINTER(ppBlob);

    dwError = LwAllocateMemory(sizeof(LSA_DATA_BLOB),
                                (PVOID*)&pBlob);
    BAIL_ON_LSA_ERROR(dwError);

    pBlob->dwLen = dwSize;
    pBlob->pData = NULL;

    if (pBlob->dwLen > 0) {
        dwError = LwAllocateMemory(pBlob->dwLen,
                        (PVOID*)&pBlob->pData);
        BAIL_ON_LSA_ERROR(dwError);

        memset(pBlob->pData, 0x0, pBlob->dwLen);        
    }

    *ppBlob = pBlob;
    
cleanup:
    return dwError;
    
error:
    if (pBlob) {
        LwFreeMemory(pBlob);
    }
    
    goto cleanup;    
}

/**************************************************************
 */

VOID
LsaDataBlobFree(
    PLSA_DATA_BLOB *ppBlob
    )
{
    if (ppBlob && *ppBlob)
    {
        if ((*ppBlob)->pData)
        {
            LwFreeMemory((*ppBlob)->pData);
        }

        LwFreeMemory(*ppBlob);
        *ppBlob = NULL;
    }    
}

/**************************************************************
 */

DWORD
LsaDataBlobStore(
    PLSA_DATA_BLOB *ppBlob,
    DWORD dwSize,
    const PBYTE pBuffer
    )
{
    DWORD dwError = LW_ERROR_INTERNAL;

    BAIL_ON_INVALID_POINTER(ppBlob);

    dwError = LsaDataBlobAllocate(ppBlob, dwSize);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwSize > 0) {        
        memcpy((*ppBlob)->pData, pBuffer, dwSize);
    }
    
cleanup:
    return dwError;
    
error:
    goto cleanup;    
}

/**************************************************************
 */

DWORD
LsaDataBlobCopy(
    PLSA_DATA_BLOB *ppDst,
    PLSA_DATA_BLOB pSrc
    )
{
    DWORD dwError = LW_ERROR_INTERNAL;

    BAIL_ON_INVALID_POINTER(ppDst);
    BAIL_ON_INVALID_POINTER(pSrc);

    dwError = LsaDataBlobStore(ppDst,
                               pSrc->dwLen,
                               pSrc->pData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;
    
error:
    goto cleanup;    
}

/**************************************************************
 */

DWORD
LsaDataBlobLength(
    PLSA_DATA_BLOB pBlob
    )
{
    return (pBlob != NULL) ? pBlob->dwLen : 0;
}

/**************************************************************
 */

PBYTE
LsaDataBlobBuffer(
    PLSA_DATA_BLOB pBlob
    )
{
    return (pBlob != NULL) ? pBlob->pData : NULL;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
