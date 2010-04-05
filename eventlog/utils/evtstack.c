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
EVTStackPush(
    PVOID pItem,
    PEVT_STACK* ppStack
    )
{
    DWORD dwError = 0;
    PEVT_STACK pStack = NULL;

    if (!ppStack) {
        dwError = EVT_ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = EVTAllocateMemory(
                    sizeof(EVT_STACK),
                    (PVOID*)&pStack);
    BAIL_ON_EVT_ERROR(dwError);

    pStack->pItem = pItem;

    pStack->pNext = *ppStack;
    *ppStack = pStack;

cleanup:

    return dwError;

error:

    if (pStack) {
        EVTFreeMemory(pStack);
    }

    goto cleanup;
}

PVOID
EVTStackPop(
    PEVT_STACK* ppStack
    )
{
    PVOID pItem = NULL;
    PEVT_STACK pTop = (ppStack && *ppStack ? *ppStack : NULL);

    if (pTop)
    {
        *ppStack = pTop->pNext;

        pItem = pTop->pItem;

        EVTFreeMemory(pTop);
    }

    return pItem;
}

PVOID
EVTStackPeek(
    PEVT_STACK pStack
    )
{
    return (pStack ? pStack->pItem : NULL);
}

DWORD
EVTStackForeach(
    PEVT_STACK pStack,
    PFN_EVT_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    )
{
    DWORD dwError = 0;
    PEVT_STACK pIter = pStack;

    if (!pfnAction) {
        goto cleanup;
    }

    for (; pIter; pIter = pIter->pNext)
    {
        dwError = pfnAction(pIter->pItem, pUserData);
        BAIL_ON_EVT_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

PEVT_STACK
EVTStackReverse(
    PEVT_STACK pStack
    )
{
    PEVT_STACK pP = NULL;
    PEVT_STACK pQ = pStack;
    PEVT_STACK pR = NULL;

    while ( pQ ) {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}

VOID
EVTStackFree(
    PEVT_STACK pStack
    )
{
    while (pStack)
    {
        PEVT_STACK pTmp = pStack;

        pStack = pStack->pNext;

        EVTFreeMemory(pTmp);
    }
}
