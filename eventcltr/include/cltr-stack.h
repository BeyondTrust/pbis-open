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
#ifndef __EVTSTACK_H__
#define __EVTSTACK_H__

typedef DWORD (*PFN_CLTR_FOREACH_STACK_ITEM)(PVOID pItem, PVOID pUserData);

typedef struct __CLTR_STACK
{
    PVOID pItem;
    
    struct __CLTR_STACK * pNext;
    
} CLTR_STACK, *PCLTR_STACK;

DWORD
CltrStackPush(
    PVOID pItem,
    PCLTR_STACK* ppStack
    );

PVOID
CltrStackPop(
    PCLTR_STACK* ppStack
    );

PVOID
CltrStackPeek(
    PCLTR_STACK pStack
    );

DWORD
CltrStackForeach(
    PCLTR_STACK pStack,
    PFN_CLTR_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    );

PCLTR_STACK
CltrStackReverse(
    PCLTR_STACK pStack
    );

VOID
CltrStackFree(
    PCLTR_STACK pStack
    );

#endif /* __EVTSTACK_H__ */
