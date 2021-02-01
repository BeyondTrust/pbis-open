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
 *        lsastack.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 * 
 *        Stack
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

DWORD
LsaStackPush(
    PVOID pItem,
    PLSA_STACK* ppStack
    )
{
    DWORD dwError = 0;
    PLSA_STACK pStack = NULL;
    
    if (!ppStack) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LwAllocateMemory(
                    sizeof(LSA_STACK),
                    (PVOID*)&pStack);
    BAIL_ON_LSA_ERROR(dwError);
    
    pStack->pItem = pItem;
    
    pStack->pNext = *ppStack;
    *ppStack = pStack;
    
cleanup:

    return dwError;
    
error:

    if (pStack) {
        LwFreeMemory(pStack);
    }

    goto cleanup;
}

PVOID
LsaStackPop(
    PLSA_STACK* ppStack
    )
{
    PVOID pItem = NULL;
    PLSA_STACK pTop = (ppStack && *ppStack ? *ppStack : NULL);
    
    if (pTop)
    {
        *ppStack = pTop->pNext;

        pItem = pTop->pItem;
        
        LwFreeMemory(pTop);
    }
    
    return pItem;
}

PVOID
LsaStackPeek(
    PLSA_STACK pStack
    )
{
    return (pStack ? pStack->pItem : NULL);
}

DWORD
LsaStackForeach(
    PLSA_STACK pStack,
    PFN_LSA_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    )
{
    DWORD dwError = 0;
    PLSA_STACK pIter = pStack;
    
    if (!pfnAction) {
        goto cleanup;
    }
    
    for (; pIter; pIter = pIter->pNext)
    {
        dwError = pfnAction(pIter->pItem, pUserData);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

PLSA_STACK
LsaStackReverse(
    PLSA_STACK pStack
    )
{
    PLSA_STACK pP = NULL;
    PLSA_STACK pQ = pStack;
    PLSA_STACK pR = NULL;
    
    while( pQ ) {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }
    
    return pP;
}

VOID
LsaStackFree(
    PLSA_STACK pStack
    )
{
    while (pStack)
    {
        PLSA_STACK pTmp = pStack;
        
        pStack = pStack->pNext;
        
        LwFreeMemory(pTmp);
    }
}
