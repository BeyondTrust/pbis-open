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
 *        lwmem.c
 *
 * Abstract:
 *
 *        BeyondTrust Advanced API (lwadvapi) Memory Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

LW_DWORD
LwAllocateMemory(
    LW_IN LW_DWORD dwSize,
    LW_OUT LW_PVOID* ppMemory
    )
{
    DWORD dwError = 0;
    PVOID pMemory = NULL;

    if (dwSize == 0)
    {
        dwSize = 1;
    }

    pMemory = calloc(1, dwSize);
    if (!pMemory)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        *ppMemory = NULL;
    }
    else
    {
        *ppMemory = pMemory;
    }

    return dwError;
}

LW_DWORD
LwReallocMemory(
    LW_IN LW_PVOID pMemory,
    LW_OUT LW_PVOID* ppNewMemory,
    LW_IN LW_DWORD dwSize
    )
{
    DWORD dwError = 0;
    PVOID pNewMemory = NULL;

    if (dwSize == 0)
    {
        dwSize = 1;
    }

    if (pMemory == NULL) {
       pNewMemory = malloc(dwSize);
       memset(pNewMemory, 0, dwSize);
    }else {
       pNewMemory = realloc(pMemory, dwSize);
    }
    if (!pNewMemory){
       dwError = LW_ERROR_OUT_OF_MEMORY;
       *ppNewMemory = NULL;
    }else {
       *ppNewMemory = pNewMemory;
    }

    return dwError;
}

LW_VOID
LwFreeMemory(
    LW_IN LW_OUT LW_PVOID pMemory
    )
{
    free(pMemory);
}
