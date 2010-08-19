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
 *        rbtree.h
 *
 * Abstract:
 *
 *        Likewise Base Library (LwBase)
 *
 *        Red Black Tree
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __LW_RBTREE_H__
#define __LW_RBTREE_H__

#include <lw/types.h>
#include <lw/attrs.h>
#include <lw/ntstatus.h>

LW_BEGIN_EXTERN_C

typedef enum
{

    LWRTL_TREE_TRAVERSAL_TYPE_PRE_ORDER = 0,
    LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
    LWRTL_TREE_TRAVERSAL_TYPE_POST_ORDER

} LWRTL_TREE_TRAVERSAL_TYPE;

typedef int   (*PFN_LWRTL_RB_TREE_COMPARE)( PVOID pKey1, PVOID pKey2);
typedef VOID  (*PFN_LWRTL_RB_TREE_FREE_KEY)(PVOID pKey);
typedef VOID  (*PFN_LWRTL_RB_TREE_FREE_DATA)(PVOID pData);

typedef NTSTATUS (*PFN_LWRTL_RB_TREE_VISIT)(
                    PVOID pKey,
                    PVOID pData,
                    PVOID pUserData,
                    PBOOLEAN pbContinue
                    );

typedef struct LWRTL_RB_TREE *PLWRTL_RB_TREE;

NTSTATUS
LwRtlRBTreeCreate(
    PFN_LWRTL_RB_TREE_COMPARE   pfnRBTreeCompare,
    PFN_LWRTL_RB_TREE_FREE_KEY  pfnRBTreeFreeKey,
    PFN_LWRTL_RB_TREE_FREE_DATA pfnRBTreeFreeData,
    PLWRTL_RB_TREE* ppRBTree
    );

// Returns STATUS_NOT_FOUND and sets *ppItem to NULL if the key is not in the
// tree.
NTSTATUS
LwRtlRBTreeFind(
    PLWRTL_RB_TREE pRBTree,
    PVOID        pKey,
    PVOID*       ppItem
    );

NTSTATUS
LwRtlRBTreeAdd(
    PLWRTL_RB_TREE pRBTree,
    PVOID       pKey,
    PVOID       pData
    );

NTSTATUS
LwRtlRBTreeTraverse(
    PLWRTL_RB_TREE pRBTree,
    LWRTL_TREE_TRAVERSAL_TYPE traversalType,
    PFN_LWRTL_RB_TREE_VISIT pfnVisit,
    PVOID                 pUserData
    );

NTSTATUS
LwRtlRBTreeRemove(
    PLWRTL_RB_TREE pRBTree,
    PVOID   pKey);

VOID
LwRtlRBTreeRemoveAll(
    PLWRTL_RB_TREE pRBTree
    );

VOID
LwRtlRBTreeFree(
    PLWRTL_RB_TREE pRBTree
    );

LW_END_EXTERN_C

#endif /* __LW_RBTREE_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
