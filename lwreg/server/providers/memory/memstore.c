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


NTSTATUS
MemRegStoreOpen(
    OUT PMEM_REG_STORE_HANDLE phDb)
{

    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE phReg = NULL;
    PWSTR rootKey = NULL;
    MEM_REG_STORE_HANDLE rootNode = NULL;
    

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

    status = LwRtlWC16StringAllocateFromCString(
                 &rootKey,
                 HKEY_THIS_MACHINE);
    BAIL_ON_NT_STATUS(status);

    status = MemRegStoreAddNode(
                 phReg,
                 rootKey,
                 REGMEM_TYPE_HIVE,
                 NULL,  // SD parameter
                 &rootNode);
    BAIL_ON_NT_STATUS(status);

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(phReg->Name);
    LWREG_SAFE_FREE_MEMORY(phReg);
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


NTSTATUS
MemRegStoreAddNode(
    IN MEM_REG_STORE_HANDLE hDb,
    PWSTR Name,
    DWORD NodeType,
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    OUT PMEM_REG_STORE_HANDLE phNode)
{
    NTSTATUS status = 0;
    PREGMEM_NODE *pNodesArray = NULL;
    PREGMEM_NODE pNewNode = NULL;

    status = NtRegReallocMemory(hDb->SubNodes, 
                                (PVOID) &pNodesArray,
                                hDb->NodesLen * sizeof(PREGMEM_NODE));
    BAIL_ON_NT_STATUS(status);
    status = LW_RTL_ALLOCATE(
                 (PVOID*)&pNewNode, PREGMEM_NODE, sizeof(PREGMEM_NODE));
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
    *phNode = hDb;

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(hDb->SubNodes);
    LWREG_SAFE_FREE_MEMORY(pNodesArray);
    LWREG_SAFE_FREE_MEMORY(pNewNode);
    LWREG_SAFE_FREE_MEMORY(pNewNode->Name);
    goto cleanup;
}
