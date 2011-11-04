/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        ecp.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) ECP (Extra Create Parameters)
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "includes.h"
#include "lwlist.h"
#include "lw/rtlgoto.h"
#include "ntlogmacros.h"
#include "lwio/iortl.h"

typedef ULONG IO_ECP_FLAGS, *PIO_ECP_FLAGS;

#define IO_ECP_FLAG_ACKNOWLEDGED    0x00000001

typedef struct _IO_ECP_LIST {
    LW_LIST_LINKS Head;
} IO_ECP_LIST;

typedef struct _IO_ECP_NODE {
    LW_LIST_LINKS Links;
    IO_ECP_FLAGS Flags;
    PSTR pszType;
    PVOID pContext;
    ULONG ContextSize;
    PIO_ECP_FREE_CONTEXT_CALLBACK pfnFreeContextCallback;
} IO_ECP_NODE, *PIO_ECP_NODE;

static
VOID
IopRtlEcpNodeFree(
    IN OUT PIO_ECP_NODE* ppEcpNode
    )
{
    PIO_ECP_NODE pEcpNode = *ppEcpNode;
    if (pEcpNode)
    {
        if (pEcpNode->pContext && pEcpNode->pfnFreeContextCallback)
        {
            pEcpNode->pfnFreeContextCallback(pEcpNode->pContext);
        }
        RtlCStringFree(&pEcpNode->pszType);
        RtlMemoryFree(pEcpNode);
        *ppEcpNode = NULL;
    }
}

static
NTSTATUS
IopRtlEcpNodeAllocate(
    OUT PIO_ECP_NODE* ppEcpNode,
    IN PCSTR pszType,
    IN PVOID pContext,
    IN ULONG ContextSize,
    IN OPTIONAL PIO_ECP_FREE_CONTEXT_CALLBACK pfnFreeContextCallback
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PIO_ECP_NODE pEcpNode = NULL;

    status = RTL_ALLOCATE(&pEcpNode, IO_ECP_NODE, sizeof(*pEcpNode));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlCStringDuplicate(&pEcpNode->pszType, pszType);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // Set the context at point where failure is impossible.
    pEcpNode->pContext = pContext;
    pEcpNode->ContextSize = ContextSize;
    pEcpNode->pfnFreeContextCallback = pfnFreeContextCallback;

cleanup:
    if (status)
    {
        IopRtlEcpNodeFree(&pEcpNode);
    }

    *ppEcpNode = pEcpNode;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IoRtlEcpListAllocate(
    OUT PIO_ECP_LIST* ppEcpList
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIO_ECP_LIST pEcpList = NULL;

    status = RTL_ALLOCATE(&pEcpList, IO_ECP_LIST, sizeof(*pEcpList));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LwListInit(&pEcpList->Head);

cleanup:
    if (status)
    {
        IoRtlEcpListFree(&pEcpList);
    }

    *ppEcpList = pEcpList;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

// Will automatically clean up ECPs in the list.
VOID
IoRtlEcpListFree(
    IN OUT PIO_ECP_LIST* ppEcpList
    )
{
    PIO_ECP_LIST pEcpList = *ppEcpList;

    if (pEcpList)
    {
        while (!LwListIsEmpty(&pEcpList->Head))
        {
            PLW_LIST_LINKS pLinks = LwListRemoveTail(&pEcpList->Head);
            PIO_ECP_NODE pNode = LW_STRUCT_FROM_FIELD(pLinks, IO_ECP_NODE, Links);
            IopRtlEcpNodeFree(&pNode);
        }
        RtlMemoryFree(pEcpList);
        *ppEcpList = NULL;
    }
}

static
NTSTATUS
IopRtlEcpListFindNode(
    IN PIO_ECP_LIST pEcpList,
    IN PCSTR pszType,
    OUT PIO_ECP_NODE* ppEcpNode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_LIST_LINKS pLinks = NULL;
    PIO_ECP_NODE pNode = NULL;

    if (pEcpList)
    {
        for (pLinks = pEcpList->Head.Next;
             pLinks != &pEcpList->Head;
             pLinks = pLinks->Next)
        {
            pNode = LW_STRUCT_FROM_FIELD(pLinks, IO_ECP_NODE, Links);
            if (LwRtlCStringIsEqual(pszType, pNode->pszType, FALSE))
            {
                break;
            }
        }

        if (pLinks == &pEcpList->Head)
        {
            status = STATUS_NOT_FOUND;
            pNode = NULL;
        }
        else
        {
            status = STATUS_SUCCESS;
            assert(pNode);
        }
    }
    else
    {
        status = STATUS_NOT_FOUND;
    }

    *ppEcpNode = pNode;

    return status;
}

ULONG
IoRtlEcpListGetCount(
    IN OPTIONAL PIO_ECP_LIST pEcpList
    )
{
    ULONG count = 0;
    PLW_LIST_LINKS pLinks = NULL;

    if (pEcpList)
    {
        for (pLinks = pEcpList->Head.Next;
             pLinks != &pEcpList->Head;
             pLinks = pLinks->Next)
        {
            count++;
        }
    }

    return count;
}

NTSTATUS
IoRtlEcpListGetNext(
    IN PIO_ECP_LIST pEcpList,
    IN OPTIONAL PCSTR pszCurrentType,
    OUT PCSTR* ppszNextType,
    OUT OPTIONAL PVOID* ppNextContext,
    OUT OPTIONAL PULONG pNextContextSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PLW_LIST_LINKS pLinks = NULL;
    PIO_ECP_NODE pNode = NULL;
    PCSTR nextType = NULL;
    PVOID nextContext = NULL;
    ULONG nextContextSize = 0;

    if (!pszCurrentType)
    {
        pLinks = pEcpList->Head.Next;
    }
    else
    {
        status = IopRtlEcpListFindNode(pEcpList, pszCurrentType, &pNode);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        pLinks = pNode->Links.Next;
    }

    if (&pEcpList->Head == pLinks)
    {
        status = STATUS_NO_MORE_ENTRIES;
        GOTO_CLEANUP_EE(EE);
    }

    pNode = LW_STRUCT_FROM_FIELD(pLinks, IO_ECP_NODE, Links);

    nextType = pNode->pszType;
    nextContext = pNode->pContext;
    nextContextSize = pNode->ContextSize;

cleanup:
    *ppszNextType = nextType;
    if (ppNextContext)
    {
        *ppNextContext = nextContext;
    }
    if (pNextContextSize)
    {
        *pNextContextSize = nextContextSize;
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IoRtlEcpListFind(
    IN PIO_ECP_LIST pEcpList,
    IN PCSTR pszType,
    OUT OPTIONAL PVOID* ppContext,
    OUT OPTIONAL PULONG pContextSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_ECP_NODE pNode = NULL;
    PVOID context = NULL;
    ULONG contextSize = 0;

    status = IopRtlEcpListFindNode(pEcpList, pszType, &pNode);
    GOTO_CLEANUP_ON_STATUS(status);

    context = pNode->pContext;
    contextSize = pNode->ContextSize;

cleanup:
    if (ppContext)
    {
        *ppContext = context;
    }
    if (pContextSize)
    {
        *pContextSize = contextSize;
    }

    return status;
}

NTSTATUS
IoRtlEcpListInsert(
    IN PIO_ECP_LIST pEcpList,
    IN PCSTR pszType,
    IN PVOID pContext,
    IN ULONG ContextSize,
    IN OPTIONAL PIO_ECP_FREE_CONTEXT_CALLBACK pfnFreeContextCallback
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIO_ECP_NODE pEcpNode = NULL;

    status = IoRtlEcpListFind(pEcpList, pszType, NULL, NULL);
    if (STATUS_SUCCESS == status)
    {
        status = STATUS_OBJECT_NAME_EXISTS;
        GOTO_CLEANUP_EE(EE);
    }
    else if (STATUS_NOT_FOUND == status)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        GOTO_CLEANUP_EE(EE);
    }

    status = IopRtlEcpNodeAllocate(&pEcpNode, pszType, pContext, ContextSize, pfnFreeContextCallback);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LwListInsertTail(&pEcpList->Head, &pEcpNode->Links);

cleanup:
    if (status)
    {
        IopRtlEcpNodeFree(&pEcpNode);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IoRtlEcpListRemove(
    IN PIO_ECP_LIST pEcpList,
    IN PCSTR pszType,
    OUT PVOID* ppContext,
    OUT OPTIONAL PULONG pContextSize,
    OUT PIO_ECP_FREE_CONTEXT_CALLBACK* ppfnFreeContextCallback
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_ECP_NODE pNode = NULL;
    PVOID context = NULL;
    ULONG contextSize = 0;
    PIO_ECP_FREE_CONTEXT_CALLBACK freeContextCallback = NULL;

    status = IopRtlEcpListFindNode(pEcpList, pszType, &pNode);
    GOTO_CLEANUP_ON_STATUS(status);

    context = pNode->pContext;
    contextSize = pNode->ContextSize;
    freeContextCallback = pNode->pfnFreeContextCallback;

    LwListRemove(&pNode->Links);

    // Let the caller free the context
    pNode->pfnFreeContextCallback = NULL;
    IopRtlEcpNodeFree(&pNode);

cleanup:
    if (ppContext)
    {
        *ppContext = context;
    }
    if (pContextSize)
    {
        *pContextSize = contextSize;
    }
    if (ppfnFreeContextCallback)
    {
        *ppfnFreeContextCallback = freeContextCallback;
    }

    return status;
}

NTSTATUS
IoRtlEcpListAcknowledge(
    IN PIO_ECP_LIST pEcpList,
    IN PCSTR pszType
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_ECP_NODE pNode = NULL;

    status = IopRtlEcpListFindNode(pEcpList, pszType, &pNode);
    GOTO_CLEANUP_ON_STATUS(status);

    SetFlag(pNode->Flags, IO_ECP_FLAG_ACKNOWLEDGED);

cleanup:
    return status;
}

BOOLEAN
IoRtlEcpListIsAcknowledged(
    IN PIO_ECP_LIST pEcpList,
    IN PCSTR pszType
    )
{
    BOOLEAN isAcknowledged = FALSE;
    NTSTATUS status = STATUS_SUCCESS;
    PIO_ECP_NODE pNode = NULL;

    status = IopRtlEcpListFindNode(pEcpList, pszType, &pNode);
    GOTO_CLEANUP_ON_STATUS(status);

    isAcknowledged = IsSetFlag(pNode->Flags, IO_ECP_FLAG_ACKNOWLEDGED);

cleanup:
    return isAcknowledged;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
