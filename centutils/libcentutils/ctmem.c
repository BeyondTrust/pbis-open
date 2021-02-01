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

#include "config.h"
#include "ctbase.h"

DWORD
CTAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    )
{

    DWORD ceError = ERROR_SUCCESS;
    PVOID pMemory = NULL;

    /* malloc is supposed to return a non-NULL pointer when it is asked to
     * allocate 0 bytes of memory. Linux systems usually follow this rule.
     *
     * AIX does not.
     */
    if (dwSize == 0)
        dwSize = 1;

    pMemory = malloc(dwSize);
    if (!pMemory){
        ceError = ERROR_OUTOFMEMORY;
        *ppMemory = NULL;
    }else {
        memset(pMemory,0, dwSize);
        *ppMemory = pMemory;
    }
    return (ceError);
}

DWORD
CTReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PVOID pNewMemory = NULL;

    if (dwSize == 0)
        dwSize = 1;

    if (pMemory == NULL) {
        pNewMemory = malloc(dwSize);
        memset(pNewMemory, 0, dwSize);
    }else {
        pNewMemory = realloc(pMemory, dwSize);
    }
    if (!pNewMemory){
        ceError = ERROR_OUTOFMEMORY;
        *ppNewMemory = NULL;
    }else {
        *ppNewMemory = pNewMemory;
    }

    return(ceError);
}


void
CTFreeMemory(
    PVOID pMemory
    )
{
    free(pMemory);
    return;
}

