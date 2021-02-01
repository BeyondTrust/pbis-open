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
 *        datablob.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
