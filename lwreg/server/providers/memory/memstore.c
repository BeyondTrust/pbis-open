/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *        memstore.c
 *
 * Abstract:
 *        Registry memory provider data storage backend
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */
#include "includes.h"
#include "memstore_p.h"


static CHAR *gRootKeys[] = 
{
    HKEY_THIS_MACHINE,
    "HKEY_CURRENT_USER", // Just for testing...
    NULL
};


NTSTATUS
MemRegStoreOpen(
    OUT PMEM_REG_STORE_HANDLE phDb)
{
    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE phReg = NULL;
    PWSTR rootKey = NULL;
    PWSTR subKey = NULL;
    MEM_REG_STORE_HANDLE rootNode = NULL;
    MEM_REG_STORE_HANDLE subKeyNode = NULL;
    DWORD i = 0;

    status = LW_RTL_ALLOCATE(
                 (PVOID*)&phReg, 
                 MEM_REG_STORE_HANDLE, 
                 sizeof(*phReg));
    BAIL_ON_NT_STATUS(status);
    memset(phReg, 0, sizeof(*phReg));

    phReg->NodeType = REGMEM_TYPE_ROOT;
    status = LwRtlWC16StringAllocateFromCString(
                 &phReg->Name, "\\");
    BAIL_ON_NT_STATUS(status);

    ghMemRegRoot = phReg;
    *phDb = phReg;

    for (i=0; gRootKeys[i]; i++)
    {
        status = LwRtlWC16StringAllocateFromCString(
                     &rootKey,
                     gRootKeys[i]);
        BAIL_ON_NT_STATUS(status);

        status = MemRegStoreAddNode(
                     phReg,
                     rootKey,
                     REGMEM_TYPE_HIVE,
                     NULL,  // SD parameter
                     &rootNode,
                     NULL);
        BAIL_ON_NT_STATUS(status);
        LWREG_SAFE_FREE_MEMORY(rootKey);
       
    }

#if 1
    /* Add a test subtree until AddKey is functional */
    status = LwRtlWC16StringAllocateFromCString(
                 &subKey,
                 HKEY_THIS_MACHINE);
    BAIL_ON_NT_STATUS(status);
    status = MemRegStoreFindNode(
                 phReg,
                 subKey,
                 &subKeyNode);
    BAIL_ON_NT_STATUS(status);
    LWREG_SAFE_FREE_MEMORY(subKey);

    
    status = LwRtlWC16StringAllocateFromCString(
                 &subKey,
                 "Services");
    BAIL_ON_NT_STATUS(status);

    status = MemRegStoreAddNode(
                     subKeyNode,
                     subKey,
                     REGMEM_TYPE_KEY,
                     NULL,  // SD parameter
                     NULL,
                     &rootNode);
    BAIL_ON_NT_STATUS(status);
    LWREG_SAFE_FREE_MEMORY(subKey);

    status = LwRtlWC16StringAllocateFromCString(
                 &subKey,
                 "Software");
    BAIL_ON_NT_STATUS(status);

    status = MemRegStoreAddNode(
                     subKeyNode,
                     subKey,
                     REGMEM_TYPE_KEY,
                     NULL,  // SD parameter
                     NULL,
                     NULL);
    BAIL_ON_NT_STATUS(status);
    LWREG_SAFE_FREE_MEMORY(subKey);

    status = LwRtlWC16StringAllocateFromCString(
                 &subKey,
                 "lsass");
    BAIL_ON_NT_STATUS(status);

    subKeyNode = rootNode;  // Services node
    status = MemRegStoreAddNode(
                     subKeyNode,
                     subKey,
                     REGMEM_TYPE_KEY,
                     NULL,  // SD parameter
                     NULL,
                     NULL);
    BAIL_ON_NT_STATUS(status);
    LWREG_SAFE_FREE_MEMORY(subKey);

#endif

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(phReg->Name);
    LWREG_SAFE_FREE_MEMORY(phReg);
    LWREG_SAFE_FREE_MEMORY(rootKey);
    LWREG_SAFE_FREE_MEMORY(subKey);
    goto cleanup;
}


NTSTATUS
MemRegStoreClose(
    IN MEM_REG_STORE_HANDLE hDb)
{
    NTSTATUS status = 0;

    // Need to traverse memory hierarchy and free resources
    BAIL_ON_NT_STATUS(status);

    LWREG_SAFE_FREE_MEMORY(ghMemRegRoot);

cleanup:
    return status;

error:
    goto cleanup;
}

#if 0
typedef struct _REGMEM_NODE
{
    PWSTR Name;
    DWORD NodeType;
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor;

    struct _REGMEM_NODE **SubNodes;
    DWORD NodesLen;

    REGMEM_VALUE *Values;
    DWORD ValuesLen;

    PREMEM_VALUE_ATTRIBUTES *Attributes;
    DWORD AttributesLen;
} REGMEM_NODE, *PREGMEM_NODE;
#endif


NTSTATUS
MemRegStoreFindNode(
    IN MEM_REG_STORE_HANDLE hDb,
    IN PCWSTR Name,
    OUT PMEM_REG_STORE_HANDLE phNode)
{
    NTSTATUS status = 0;
    DWORD nodeIndex = 0;
    BOOLEAN bFoundNode = FALSE;

    BAIL_ON_NT_STATUS(status);

    if (!Name)
    {
        Name = (PCWSTR) L"";
    }
    for (nodeIndex=0; nodeIndex<hDb->NodesLen; nodeIndex++)
    {
        if (LwRtlWC16StringIsEqual(Name, hDb->SubNodes[nodeIndex]->Name, FALSE))
        {
            bFoundNode = TRUE;
            break;
        }
    }

    if (bFoundNode)
    {
        *phNode = hDb->SubNodes[nodeIndex];
    }
    else
    {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemRegStoreAddNode(
    IN MEM_REG_STORE_HANDLE hDb,
    PWSTR Name,
    DWORD NodeType,
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    OUT PMEM_REG_STORE_HANDLE phNode,
    OUT OPTIONAL PMEM_REG_STORE_HANDLE pRetNewNode)
{
    NTSTATUS status = 0;
    PREGMEM_NODE *pNodesArray = NULL;
    PREGMEM_NODE pNewNode = NULL;

    status = NtRegReallocMemory(hDb->SubNodes, 
                                (PVOID) &pNodesArray,
                                (hDb->NodesLen + 1) * sizeof(PREGMEM_NODE));
    BAIL_ON_NT_STATUS(status);
    status = LW_RTL_ALLOCATE(
                 (PVOID*)&pNewNode, PREGMEM_NODE, sizeof(REGMEM_NODE));
    BAIL_ON_NT_STATUS(status);
    memset(pNewNode, 0, sizeof(*pNewNode));
  
    hDb->SubNodes = pNodesArray;
    pNodesArray = NULL;
    hDb->SubNodes[hDb->NodesLen] = pNewNode;

    status = LwRtlWC16StringDuplicate(&pNewNode->Name, Name);
    BAIL_ON_NT_STATUS(status);
    pNewNode->NodeType = NodeType;
    pNewNode->SecurityDescriptor = SecurityDescriptor;
    hDb->NodesLen++;

    if (phNode)
    {
        *phNode = hDb;
    }
    if (pRetNewNode)
    {
        *pRetNewNode = pNewNode;
    }

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(hDb->SubNodes);
    LWREG_SAFE_FREE_MEMORY(pNodesArray);
    LWREG_SAFE_FREE_MEMORY(pNewNode);
    LWREG_SAFE_FREE_MEMORY(pNewNode->Name);
    goto cleanup;
}
