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
 *        lsastack.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
