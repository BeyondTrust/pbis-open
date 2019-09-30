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
 *        rbtree.h
 *
 * Abstract:
 *
 *        BeyondTrust Base Library (LwBase)
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
