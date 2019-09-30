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
 *        lsamem.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS) Memory Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
LsaAppendAndFreePtrs(
    IN OUT PDWORD pdwDestCount,
    IN OUT PVOID** pppDestPtrArray,
    IN OUT PDWORD pdwAppendCount,
    IN OUT PVOID** pppAppendPtrArray
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwCurrentCount = *pdwDestCount;
    DWORD dwAppendSize = *pdwAppendCount * sizeof(PVOID);
    DWORD dwNewSize = dwCurrentCount * sizeof(PVOID) + dwAppendSize;
    DWORD dwNewCount = dwNewSize / sizeof(PVOID);
    PVOID *ppDestPtrArray = *pppDestPtrArray;

    if (dwNewCount < dwCurrentCount)
    {
        dwError = LW_ERROR_ERRNO_ERANGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (ppDestPtrArray == NULL)
    {
        LSA_ASSERT(dwCurrentCount == 0);
        *pppDestPtrArray = *pppAppendPtrArray;
        *pppAppendPtrArray = NULL;
        *pdwDestCount = *pdwAppendCount;
        *pdwAppendCount = 0;
    }
    else
    {
        if (dwNewSize > 0)
        {
            dwError = LwReallocMemory(
                ppDestPtrArray,
                (PVOID*)&ppDestPtrArray,
                dwNewSize);
            BAIL_ON_LSA_ERROR(dwError);
        }

        /* The old pointer was freed and now invalid, so the output parameter
         * needs to be assigned here, even if the rest of the function fails. */
        *pppDestPtrArray = ppDestPtrArray;

        // Append the new data and zero it out from the src array
        memcpy(ppDestPtrArray + dwCurrentCount, 
                *pppAppendPtrArray,
                dwAppendSize);
        *pdwDestCount = dwNewCount;
        LW_SAFE_FREE_MEMORY(*pppAppendPtrArray);
        *pdwAppendCount = 0;
    }

cleanup:

    return dwError;

error:
    // Leave pppDestPtrArray, pdwDestCount, pdwAppendCount, and
    // pppAppendPtrArray as is, so that the passed in data is not lost.
    
    goto cleanup;
}

DWORD
LsaInitializeStringBuffer(
        LSA_STRING_BUFFER *pBuffer,
        size_t sCapacity)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR pszBuffer = NULL;

    pBuffer->sLen = 0;
    pBuffer->sCapacity = 0;

    if (sCapacity > DWORD_MAX - 1)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
        sCapacity + 1,
        (PVOID *)&pszBuffer);
    BAIL_ON_LSA_ERROR(dwError);

    pBuffer->pszBuffer = pszBuffer;
    pBuffer->sCapacity = sCapacity;

cleanup:
    return dwError;

error:
    pBuffer->pszBuffer = NULL;

    goto cleanup;
}

DWORD
LsaAppendStringBuffer(
        LSA_STRING_BUFFER *pBuffer,
        PCSTR pszAppend)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t sAppendLen = 0;
    size_t sNewCapacity = 0;

    sAppendLen = strlen(pszAppend);

    if (sAppendLen + pBuffer->sLen > pBuffer->sCapacity ||
            pBuffer->pszBuffer == NULL)
    {
        sNewCapacity = (pBuffer->sCapacity + sAppendLen) * 2;

        if (sNewCapacity > DWORD_MAX - 1)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (sNewCapacity < pBuffer->sCapacity)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LwReallocMemory(
            pBuffer->pszBuffer,
            (PVOID *)&pBuffer->pszBuffer,
            sNewCapacity + 1);
        BAIL_ON_LSA_ERROR(dwError);

        pBuffer->sCapacity = sNewCapacity;
    }

    memcpy(
        pBuffer->pszBuffer + pBuffer->sLen,
        pszAppend,
        sAppendLen);
    pBuffer->sLen += sAppendLen;
    pBuffer->pszBuffer[pBuffer->sLen] = '\0';

cleanup:
    return dwError;

error:
    goto cleanup;
}

void
LsaFreeStringBufferContents(
        LSA_STRING_BUFFER *pBuffer)
{
    LW_SAFE_FREE_MEMORY(pBuffer->pszBuffer);

    pBuffer->sLen = 0;
    pBuffer->sCapacity = 0;
}
