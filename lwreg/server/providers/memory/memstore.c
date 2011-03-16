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
    MEM_REG_STORE_HANDLE rootNode = NULL;
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

    ghMemRegRoot = phReg;
    *phDb = phReg;

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(phReg->Name);
    LWREG_SAFE_FREE_MEMORY(phReg);
    LWREG_SAFE_FREE_MEMORY(rootKey);
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
    PCWSTR Name,
    DWORD NodeType,
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    OUT PMEM_REG_STORE_HANDLE phNode,
    OUT OPTIONAL PMEM_REG_STORE_HANDLE pRetNewNode)
{
    NTSTATUS status = 0;
    PREGMEM_NODE *pNodesArray = NULL;
    PREGMEM_NODE pNewNode = NULL;
    PWSTR newNodeName = NULL;

    status = NtRegReallocMemory(hDb->SubNodes, 
                                (PVOID) &pNodesArray,
                                (hDb->NodesLen + 1) * sizeof(PREGMEM_NODE));
    BAIL_ON_NT_STATUS(status);
    status = LW_RTL_ALLOCATE(
                 (PVOID*) &pNewNode, PREGMEM_NODE, sizeof(REGMEM_NODE));
    BAIL_ON_NT_STATUS(status);
    memset(pNewNode, 0, sizeof(*pNewNode));

    status = LwRtlWC16StringDuplicate(&newNodeName, Name);
    BAIL_ON_NT_STATUS(status);
  
    hDb->SubNodes = pNodesArray;
    pNodesArray = NULL;

    hDb->SubNodes[hDb->NodesLen] = pNewNode;
    pNewNode->Name = newNodeName;
    newNodeName = NULL;

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
    LWREG_SAFE_FREE_MEMORY(pNodesArray);
    LWREG_SAFE_FREE_MEMORY(pNewNode);
    LWREG_SAFE_FREE_MEMORY(newNodeName);
    goto cleanup;
}


#if 0
typedef struct _REGMEM_VALUE
{
    PWSTR Name;
    DWORD Type;
    PVOID Data;
    DWORD DataLen;
} REGMEM_VALUE, *PREGMEM_VALUE;

/* For reference, remove once done with implementation */
typedef struct _REGMEM_NODE
{
    PWSTR Name;
    DWORD NodeType;
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor;

    struct _REGMEM_NODE **SubNodes;
    DWORD NodesLen;

    PREGMEM_VALUE *Values;
    DWORD ValuesLen;

    PREMEM_VALUE_ATTRIBUTES *Attributes;
    DWORD AttributesLen;
} REGMEM_NODE, *PREGMEM_NODE;
#endif


NTSTATUS
MemRegStoreFindNodeValue(
    IN MEM_REG_STORE_HANDLE hDb,
    IN PCWSTR Name,
    OUT PREGMEM_VALUE *phValue)
{
    NTSTATUS status = 0;
    DWORD valueIndex = 0;
    BOOLEAN bFoundValue = FALSE;

    BAIL_ON_NT_STATUS(status);
    if (!Name)
    {
        Name = (PCWSTR) L"";
    }
    for (valueIndex=0; valueIndex<hDb->ValuesLen; valueIndex++)
    {
        if (LwRtlWC16StringIsEqual(Name, hDb->Values[valueIndex]->Name, FALSE))
        {
            bFoundValue = TRUE;
            break;
        }
    }

    if (bFoundValue)
    {
        *phValue = hDb->Values[valueIndex];
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
MemRegStoreAddNodeValue(
    MEM_REG_STORE_HANDLE hDb,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD dwReserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData)
{
    NTSTATUS status = 0;
    PREGMEM_VALUE pNodeValue = NULL;
    PWSTR pwszName = NULL;
    WCHAR pwszNull[2] = {0};
    BYTE *pbData = NULL;
    PREGMEM_VALUE *newValues;

    status = LW_RTL_ALLOCATE(
                 (PVOID*) &pNodeValue, 
                 REGMEM_VALUE, 
                 sizeof(*pNodeValue));
    BAIL_ON_NT_STATUS(status);

    
    if (!pValueName || *pValueName == '\0')
    {
        pValueName = pwszNull;
    }
    status = LwRtlWC16StringDuplicate(&pwszName, pValueName);
    BAIL_ON_NT_STATUS(status);

    if (cbData > 0)
    {
        status = LW_RTL_ALLOCATE(
                     (PVOID*) &pbData, 
                     BYTE, 
                     sizeof(*pbData) * cbData);
        BAIL_ON_NT_STATUS(status);
        memset(pbData, 0, sizeof(*pbData) * cbData);
        memcpy(pbData, pData, cbData);
    }
    else
    {
        status = LW_RTL_ALLOCATE(
                     (PVOID*) &pbData, 
                     BYTE, 
                     1);
        BAIL_ON_NT_STATUS(status);
        memset(pbData, 0, 1);
    }

    status = NtRegReallocMemory(hDb->Values, 
                                (PVOID) &newValues,
                                (hDb->ValuesLen + 1) * sizeof(PREGMEM_VALUE));
    BAIL_ON_NT_STATUS(status);

    pNodeValue->Name = pwszName;
    pNodeValue->Type = dwType;
    pNodeValue->Data = pbData;
    pNodeValue->DataLen = cbData;

    hDb->Values = newValues;
    hDb->Values[hDb->ValuesLen] = pNodeValue;
    hDb->ValuesLen++;

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pNodeValue);
    LWREG_SAFE_FREE_MEMORY(pwszName);
    LWREG_SAFE_FREE_MEMORY(pbData);
    LWREG_SAFE_FREE_MEMORY(newValues);
    goto cleanup;
}
