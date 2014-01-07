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
 *        lwnet-stack.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Stack
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

DWORD
LWNetStackPush(
    PVOID pItem,
    PLWNET_STACK* ppStack
    )
{
    DWORD dwError = 0;
    PLWNET_STACK pStack = NULL;
    
    if (!ppStack) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dwError = LWNetAllocateMemory(
                    sizeof(LWNET_STACK),
                    (PVOID*)&pStack);
    BAIL_ON_LWNET_ERROR(dwError);
    
    pStack->pItem = pItem;
    
    pStack->pNext = *ppStack;
    *ppStack = pStack;
    
cleanup:

    return dwError;
    
error:

    if (pStack) {
        LWNetFreeMemory(pStack);
    }

    goto cleanup;
}

PVOID
LWNetStackPop(
    PLWNET_STACK* ppStack
    )
{
    PVOID pItem = NULL;
    PLWNET_STACK pTop = (ppStack && *ppStack ? *ppStack : NULL);
    
    if (pTop)
    {
        *ppStack = pTop->pNext;

        pItem = pTop->pItem;
        
        LWNetFreeMemory(pTop);
    }
    
    return pItem;
}

PVOID
LWNetStackPeek(
    PLWNET_STACK pStack
    )
{
    return (pStack ? pStack->pItem : NULL);
}

DWORD
LWNetStackForeach(
    PLWNET_STACK pStack,
    PFN_LWNET_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    )
{
    DWORD dwError = 0;
    PLWNET_STACK pIter = pStack;
    
    if (!pfnAction) {
        goto cleanup;
    }
    
    for (; pIter; pIter = pIter->pNext)
    {
        dwError = pfnAction(pIter->pItem, pUserData);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

PLWNET_STACK
LWNetStackReverse(
    PLWNET_STACK pStack
    )
{
    PLWNET_STACK pP = NULL;
    PLWNET_STACK pQ = pStack;
    PLWNET_STACK pR = NULL;
    
    while( pQ ) {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }
    
    return pP;
}

VOID
LWNetStackFree(
    PLWNET_STACK pStack
    )
{
    while (pStack)
    {
        PLWNET_STACK pTmp = pStack;
        
        pStack = pStack->pNext;
        
        LWNetFreeMemory(pTmp);
    }
}
