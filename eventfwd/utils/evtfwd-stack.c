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
 *        evtfwd-stack.h
 *
 * Abstract:
 *
 *        Event forwarder from eventlogd to collector service
 * 
 *        Stack
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

DWORD
EfdStackPush(
    PVOID pItem,
    PEFD_STACK* ppStack
    )
{
    DWORD dwError = 0;
    PEFD_STACK pStack = NULL;
    
    if (!ppStack) {
        dwError = EFD_ERROR_INVALID_PARAMETER;
        BAIL_ON_EFD_ERROR(dwError);
    }
    
    dwError = RTL_ALLOCATE(
                    &pStack,
                    EFD_STACK,
                    sizeof(EFD_STACK));
    BAIL_ON_EFD_ERROR(dwError);
    
    pStack->pItem = pItem;
    
    pStack->pNext = *ppStack;
    *ppStack = pStack;
    
cleanup:

    return dwError;
    
error:

    if (pStack) {
        RtlMemoryFree(pStack);
    }

    goto cleanup;
}

PVOID
EfdStackPop(
    PEFD_STACK* ppStack
    )
{
    PVOID pItem = NULL;
    PEFD_STACK pTop = (ppStack && *ppStack ? *ppStack : NULL);
    
    if (pTop)
    {
        *ppStack = pTop->pNext;

        pItem = pTop->pItem;
        
        RtlMemoryFree(pTop);
    }
    
    return pItem;
}

PVOID
EfdStackPeek(
    PEFD_STACK pStack
    )
{
    return (pStack ? pStack->pItem : NULL);
}

DWORD
EfdStackForeach(
    PEFD_STACK pStack,
    PFN_EFD_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    )
{
    DWORD dwError = 0;
    PEFD_STACK pIter = pStack;
    
    if (!pfnAction) {
        goto cleanup;
    }
    
    for (; pIter; pIter = pIter->pNext)
    {
        dwError = pfnAction(pIter->pItem, pUserData);
        BAIL_ON_EFD_ERROR(dwError);
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

PEFD_STACK
EfdStackReverse(
    PEFD_STACK pStack
    )
{
    PEFD_STACK pP = NULL;
    PEFD_STACK pQ = pStack;
    PEFD_STACK pR = NULL;
    
    while( pQ ) {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }
    
    return pP;
}

VOID
EfdStackFree(
    PEFD_STACK pStack
    )
{
    while (pStack)
    {
        PEFD_STACK pTmp = pStack;
        
        pStack = pStack->pNext;
        
        RtlMemoryFree(pTmp);
    }
}
