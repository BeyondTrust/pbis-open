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


static VOID
_MemDbFreeWC16Array(
    IN PWSTR *ppwszArray)
{
    DWORD index = 0;

    if (ppwszArray)
    {
        for (index=0; ppwszArray[index]; index++)
        {
            LWREG_SAFE_FREE_MEMORY(ppwszArray[index]);
        }
        LWREG_SAFE_FREE_MEMORY(ppwszArray);
    }
}


static NTSTATUS
_MemRegDuplicateWC16Array(
    IN PWSTR *ppwszArray,
    OUT PWSTR **pppwszCopyArray)
{
    NTSTATUS status = 0;
    PWSTR *ppwszRetStrings = NULL;
    PWSTR pwszRetString = NULL;
    DWORD index = 0;

    /* Count number of entries first */
    for (index=0; ppwszArray[index]; index++)
        ;
    index++;

    /* Allocate array of pointers to wide strings */
    status = LW_RTL_ALLOCATE(
                 (PVOID*) &ppwszRetStrings,
                 PWSTR,
                 sizeof(PWSTR) * index);
    BAIL_ON_NT_STATUS(status);
    memset(ppwszRetStrings, 0, sizeof(PWSTR) * index);

    /* Duplicate the strings */
     
    for (index=0; ppwszArray[index]; index++)
    {
        status = LwRtlWC16StringDuplicate(
                     &pwszRetString,
                     ppwszArray[index]); 
                     BAIL_ON_NT_STATUS(status);
        ppwszRetStrings[index] = pwszRetString;
        pwszRetString = NULL;
    }

    *pppwszCopyArray = ppwszRetStrings;
cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(ppwszRetStrings);
    LWREG_SAFE_FREE_MEMORY(pwszRetString);
    goto cleanup;
}

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

/* Probably should set default SD here */
        status = MemRegStoreAddNode(
                     phReg,
                     rootKey,
                     REGMEM_TYPE_HIVE,
                     NULL,  // SD parameter
                     0,
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
    ULONG SecurityDescriptorLen,
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

    /* Inherit SD from parent node if one is not provided */
    pNewNode->SecurityDescriptor = hDb->SecurityDescriptor;
    pNewNode->SecurityDescriptorLen = hDb->SecurityDescriptorLen;
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

    /* Clear out existing data before overwriting with new buffer */
    LWREG_SAFE_FREE_MEMORY(pNodeValue->Data);
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



NTSTATUS
MemRegStoreAddNodeAttribute(
    PREGMEM_VALUE hValue,
    IN PLWREG_VALUE_ATTRIBUTES pAttributes)
{
    NTSTATUS status = 0;
    BYTE *pbData = NULL;
    PWSTR *ppwszEnumStrings = NULL;
    PWSTR pwszEnumString = NULL;
    PWSTR pwszDocString = NULL;

    /*
     * Assign all scaler types first. Then allocate data and duplicate
     * pointer types.
     */
    hValue->Attributes = *pAttributes;
    if (pAttributes->DefaultValueLen > 0)
    {
        status = LW_RTL_ALLOCATE(
                     (PVOID*) &pbData, 
                     BYTE, 
                     sizeof(*pbData) * pAttributes->DefaultValueLen);
        BAIL_ON_NT_STATUS(status);
        memset(pbData, 0, sizeof(*pbData) * pAttributes->DefaultValueLen);
        memcpy(pbData,
               pAttributes->pDefaultValue,
               pAttributes->DefaultValueLen);
    }

    /*
     * Duplicate the documentation string, if present
     */
    if (pAttributes->pwszDocString) { 
        status = LwRtlWC16StringDuplicate(&pwszDocString, 
                     pAttributes->pwszDocString);
        BAIL_ON_NT_STATUS(status);
    }

    /*
     * Duplicate the multi-string range array when
     * range type is RANGE_ENUM and the array is present.
     */
    if (pAttributes->RangeType == LWREG_VALUE_RANGE_TYPE_ENUM &&
        pAttributes->Range.ppwszRangeEnumStrings)
    {
        status = _MemRegDuplicateWC16Array(
                     pAttributes->Range.ppwszRangeEnumStrings,
                     &ppwszEnumStrings);
        BAIL_ON_NT_STATUS(status);
    }

    /*
     * Assign all allocated memory present in the attributes structure
     * to the value node.
     */
    if (pAttributes->DefaultValueLen > 0)
    {
        hValue->Attributes.pDefaultValue = (PVOID) pbData;
    }
    hValue->Attributes.pwszDocString = pwszDocString;

    if (pAttributes->RangeType == 
            LWREG_VALUE_RANGE_TYPE_ENUM)
    {
        hValue->Attributes.Range.ppwszRangeEnumStrings = ppwszEnumStrings;
    }

cleanup:
    return status;

error:
    /* Free a bunch of stuff here if something fails */
    LWREG_SAFE_FREE_MEMORY(pbData);
    LWREG_SAFE_FREE_MEMORY(pwszDocString);
    LWREG_SAFE_FREE_MEMORY(pwszEnumString);
    _MemDbFreeWC16Array(ppwszEnumStrings);
    goto cleanup;
}


NTSTATUS
MemRegStoreGetNodeValueAttributes(
    PREGMEM_VALUE hValue,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes)
{
    NTSTATUS status = 0;
    PWSTR *ppwszEnumStrings = NULL;
    PWSTR pwszEnumString = NULL;
    PWSTR pwszDocString = NULL;
    PLWREG_CURRENT_VALUEINFO pCurrentValue = NULL;
    PLWREG_VALUE_ATTRIBUTES pValueAttributes = NULL;
    PBYTE pRetCurrentValueData = NULL;
    PBYTE pRetAttributeValueData = NULL;
    DWORD dwValueLen = 0;

    dwValueLen = hValue->DataLen;
    if (ppCurrentValue && dwValueLen > 0)
    {
         /* 
          * Validate set value is different than default. Only return value
          * if is different than the default.
          */
        if (dwValueLen != hValue->Attributes.DefaultValueLen ||
            memcmp(hValue->Data, 
                   hValue->Attributes.pDefaultValue,
                   dwValueLen) != 0)
        {
            status = LW_RTL_ALLOCATE((PVOID*) &pCurrentValue,
                                     LWREG_CURRENT_VALUEINFO,
                                     sizeof (*pCurrentValue));
            BAIL_ON_NT_STATUS(status);
        
            if (dwValueLen > 0)
            {
                status = LW_RTL_ALLOCATE((PVOID*) &pRetCurrentValueData,
                                         BYTE,
                                         dwValueLen);
                BAIL_ON_NT_STATUS(status);
                memset(pRetCurrentValueData, 0, hValue->DataLen);
                memcpy(pRetCurrentValueData, hValue->Data, dwValueLen);
            }
        }
    }

    if (ppValueAttributes)
    {
        if (hValue->Attributes.DefaultValueLen > 0)
        {
            status = LW_RTL_ALLOCATE(
                         (PVOID*) &pRetAttributeValueData, 
                         BYTE, 
                         sizeof(BYTE) * hValue->Attributes.DefaultValueLen);
            BAIL_ON_NT_STATUS(status);
            memset(pRetAttributeValueData, 
                   0, 
                   sizeof(*pRetAttributeValueData) * 
                       hValue->Attributes.DefaultValueLen);
            memcpy(pRetAttributeValueData,
                   hValue->Attributes.pDefaultValue,
                   hValue->Attributes.DefaultValueLen);
        }
    
        /*
         * Duplicate the documentation string, if present
         */
        if (hValue->Attributes.pwszDocString) { 
            status = LwRtlWC16StringDuplicate(&pwszDocString, 
                         hValue->Attributes.pwszDocString);
            BAIL_ON_NT_STATUS(status);
        }

        status = LW_RTL_ALLOCATE((PVOID*) &pValueAttributes,
                                 LWREG_VALUE_ATTRIBUTES,
                                 sizeof (*pValueAttributes));
        BAIL_ON_NT_STATUS(status);
        memset(pValueAttributes, 0, sizeof(*pValueAttributes));


        if (hValue->Attributes.RangeType == LWREG_VALUE_RANGE_TYPE_ENUM &&
            hValue->Attributes.Range.ppwszRangeEnumStrings)
        {
            status = _MemRegDuplicateWC16Array(
                         hValue->Attributes.Range.ppwszRangeEnumStrings,
                         &ppwszEnumStrings);
            BAIL_ON_NT_STATUS(status);
        }
    }

    /* Assign all allocated data to return optional return structures */
    if (pCurrentValue)
    {
         pCurrentValue->dwType = hValue->Type;
         pCurrentValue->pvData = pRetCurrentValueData;
         pCurrentValue->cbData = hValue->DataLen;
         *ppCurrentValue = pCurrentValue;
    }

    if (pValueAttributes)
    {
        *pValueAttributes = hValue->Attributes;
        pValueAttributes->pDefaultValue = pRetAttributeValueData;
        pValueAttributes->pwszDocString = pwszDocString;
        if (ppwszEnumStrings)
        {
            pValueAttributes->Range.ppwszRangeEnumStrings = ppwszEnumStrings;
        }
        *ppValueAttributes = pValueAttributes;
    }

cleanup:
    return status;

error:
    /* Free a bunch of stuff here if something fails */
    LWREG_SAFE_FREE_MEMORY(pRetAttributeValueData);
    LWREG_SAFE_FREE_MEMORY(pwszDocString);
    LWREG_SAFE_FREE_MEMORY(pwszEnumString);
    _MemDbFreeWC16Array(ppwszEnumStrings);
    goto cleanup;
}
