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
 *        stack.c
 *
 * Abstract:
 *
 *        BeyondTrust IO (LWIO)
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
