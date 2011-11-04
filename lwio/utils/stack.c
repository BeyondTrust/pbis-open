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
 *        stack.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Utilities
 *
 *        Stack
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
SMBStackPush(
    PVOID pItem,
    PSMB_STACK* ppStack
    )
{
    DWORD dwError = 0;
    PSMB_STACK pStack = NULL;

    if (!ppStack) {
        dwError = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    dwError = LwIoAllocateMemory(
                    sizeof(SMB_STACK),
                    (PVOID*)&pStack);
    BAIL_ON_LWIO_ERROR(dwError);

    pStack->pItem = pItem;

    pStack->pNext = *ppStack;
    *ppStack = pStack;

cleanup:

    return dwError;

error:

    if (pStack) {
        LwIoFreeMemory(pStack);
    }

    goto cleanup;
}

/** Pushes an element to the head of the stack without allocating it.
 *
 *  This can be used to impelement a free-list allocator where the free list
 *  is embedded within each free buffer.
 */
VOID
SMBStackPushNoAlloc(
    PSMB_STACK* ppStack,
    PSMB_STACK  pStack
    )
{
    pStack->pItem = NULL;

    pStack->pNext = *ppStack;
    *ppStack = pStack;
}

PVOID
SMBStackPop(
    PSMB_STACK* ppStack
    )
{
    PVOID pItem = NULL;
    PSMB_STACK pTop = (ppStack && *ppStack ? *ppStack : NULL);

    if (pTop)
    {
        *ppStack = pTop->pNext;

        pItem = pTop->pItem;

        LwIoFreeMemory(pTop);
    }

    return pItem;
}

/** Pops and element from the head of the stack without deleting it.
 *
 *  This can be used to impelement a free-list allocator where the free list
 *  is embedded within each free buffer.
 */
VOID
SMBStackPopNoFree(
    PSMB_STACK* ppStack
    )
{
    PSMB_STACK pTop = (ppStack && *ppStack ? *ppStack : NULL);

    if (pTop)
    {
        *ppStack = pTop->pNext;
    }
}

VOID
SMBDLinkedListPop(
    PSMBDLINKEDLIST *ppListHead
    )
{
    PSMBDLINKEDLIST pList = *ppListHead;

    if (pList->pNext) {
        // Connect the next neighbor to our previous neighbor
        pList->pNext->pPrev = pList->pPrev;
    }

    if (pList->pPrev) {
        // Connect the previous neighbor to our next neighbor
        pList->pPrev->pNext = pList->pNext;
    }

    *ppListHead = pList->pNext;
}

PVOID
SMBStackPeek(
    PSMB_STACK pStack
    )
{
    return (pStack ? pStack->pItem : NULL);
}

DWORD
SMBStackForeach(
    PSMB_STACK pStack,
    PFNSMB_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    )
{
    DWORD dwError = 0;
    PSMB_STACK pIter = pStack;

    if (!pfnAction) {
        goto cleanup;
    }

    for (; pIter; pIter = pIter->pNext)
    {
        dwError = pfnAction(pIter->pItem, pUserData);
        BAIL_ON_LWIO_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

PSMB_STACK
SMBStackReverse(
    PSMB_STACK pStack
    )
{
    PSMB_STACK pP = NULL;
    PSMB_STACK pQ = pStack;
    PSMB_STACK pR = NULL;

    while( pQ ) {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}

VOID
SMBStackFree(
    PSMB_STACK pStack
    )
{
    while (pStack)
    {
        PSMB_STACK pTmp = pStack;

        pStack = pStack->pNext;

        LwIoFreeMemory(pTmp);
    }
}
