/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        evtstack.h
 *
 * Abwcstract:
 *
 *        Likewise Eventlog
 *
 *        Stack
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "cltr-base.h"

DWORD
CltrStackPush(
    PVOID pItem,
    PCLTR_STACK* ppStack
    )
{
    DWORD dwError = 0;
    PCLTR_STACK pStack = NULL;

    if (!ppStack) {
        dwError = CLTR_ERROR_INVALID_PARAMETER;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    dwError = CltrAllocateMemory(
                    sizeof(CLTR_STACK),
                    (PVOID*)&pStack);
    BAIL_ON_CLTR_ERROR(dwError);

    pStack->pItem = pItem;

    pStack->pNext = *ppStack;
    *ppStack = pStack;

cleanup:

    return dwError;

error:

    if (pStack) {
        CltrFreeMemory(pStack);
    }

    goto cleanup;
}

PVOID
CltrStackPop(
    PCLTR_STACK* ppStack
    )
{
    PVOID pItem = NULL;
    PCLTR_STACK pTop = (ppStack && *ppStack ? *ppStack : NULL);

    if (pTop)
    {
        *ppStack = pTop->pNext;

        pItem = pTop->pItem;

        CltrFreeMemory(pTop);
    }

    return pItem;
}

PVOID
CltrStackPeek(
    PCLTR_STACK pStack
    )
{
    return (pStack ? pStack->pItem : NULL);
}

DWORD
CltrStackForeach(
    PCLTR_STACK pStack,
    PFN_CLTR_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    )
{
    DWORD dwError = 0;
    PCLTR_STACK pIter = pStack;

    if (!pfnAction) {
        goto cleanup;
    }

    for (; pIter; pIter = pIter->pNext)
    {
        dwError = pfnAction(pIter->pItem, pUserData);
        BAIL_ON_CLTR_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

PCLTR_STACK
CltrStackReverse(
    PCLTR_STACK pStack
    )
{
    PCLTR_STACK pP = NULL;
    PCLTR_STACK pQ = pStack;
    PCLTR_STACK pR = NULL;

    while ( pQ ) {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}

VOID
CltrStackFree(
    PCLTR_STACK pStack
    )
{
    while (pStack)
    {
        PCLTR_STACK pTmp = pStack;

        pStack = pStack->pNext;

        CltrFreeMemory(pTmp);
    }
}
