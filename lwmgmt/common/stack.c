/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        evtstack.h
 *
 * Abstract:
 *
 *        Likewise Eventlog
 *
 *        Stack
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#include "includes.h"

DWORD
LWMGMTStackPush(
    PVOID pItem,
    PLWMGMTSTACK* ppStack
    )
{
    DWORD dwError = 0;
    PLWMGMTSTACK pStack = NULL;

    if (!ppStack) {
        dwError = LWMGMT_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    dwError = LWMGMTAllocateMemory(
                    sizeof(LWMGMTSTACK),
                    (PVOID*)&pStack);
    BAIL_ON_LWMGMT_ERROR(dwError);

    pStack->pItem = pItem;

    pStack->pNext = *ppStack;
    *ppStack = pStack;

cleanup:

    return dwError;

error:

    if (pStack) {
        LWMGMTFreeMemory(pStack);
    }

    goto cleanup;
}

PVOID
LWMGMTStackPop(
    PLWMGMTSTACK* ppStack
    )
{
    PVOID pItem = NULL;
    PLWMGMTSTACK pTop = (ppStack && *ppStack ? *ppStack : NULL);

    if (pTop)
    {
        *ppStack = pTop->pNext;

        pItem = pTop->pItem;

        LWMGMTFreeMemory(pTop);
    }

    return pItem;
}

PVOID
LWMGMTStackPeek(
    PLWMGMTSTACK pStack
    )
{
    return (pStack ? pStack->pItem : NULL);
}

DWORD
LWMGMTStackForeach(
    PLWMGMTSTACK pStack,
    PFN_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    )
{
    DWORD dwError = 0;
    PLWMGMTSTACK pIter = pStack;

    if (!pfnAction) {
        goto cleanup;
    }

    for (; pIter; pIter = pIter->pNext)
    {
        dwError = pfnAction(pIter->pItem, pUserData);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

PLWMGMTSTACK
LWMGMTStackReverse(
    PLWMGMTSTACK pStack
    )
{
    PLWMGMTSTACK pP = NULL;
    PLWMGMTSTACK pQ = pStack;
    PLWMGMTSTACK pR = NULL;

    while ( pQ ) {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}

VOID
LWMGMTStackFree(
    PLWMGMTSTACK pStack
    )
{
    while (pStack)
    {
        PLWMGMTSTACK pTmp = pStack;

        pStack = pStack->pNext;

        LWMGMTFreeMemory(pTmp);
    }
}
