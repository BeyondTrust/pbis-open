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
    OUT PMEMREG_STORE_NODE *pphDb)
{
    NTSTATUS status = 0;
    PMEMREG_STORE_NODE phReg = NULL;
    PWSTR rootKey = NULL;
    PMEMREG_STORE_NODE rootNode = NULL;
    DWORD i = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    ULONG ulSecDescLen = 0;

    /* This is the ROOT node (\) of the registry */
    status = LW_RTL_ALLOCATE(
                 (PVOID*)&phReg, 
                 PMEMREG_STORE_NODE, 
                 sizeof(*phReg));
    BAIL_ON_NT_STATUS(status);
    memset(phReg, 0, sizeof(*phReg));

    phReg->NodeType = REGMEM_TYPE_ROOT;
    status = LwRtlWC16StringAllocateFromCString(
                 &phReg->Name, "\\");
    BAIL_ON_NT_STATUS(status);

    status = RegSrvCreateDefaultSecDescRel(
                 &pSecDescRel,
                 &ulSecDescLen);
    BAIL_ON_NT_STATUS(status);

    /* Populate the subkeys under the root node */
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
                     pSecDescRel,  // SD parameter
                     ulSecDescLen,
                     &rootNode,
                     NULL);
        BAIL_ON_NT_STATUS(status);
        LWREG_SAFE_FREE_MEMORY(rootKey);
    }

    *pphDb = phReg;

cleanup:
    LWREG_SAFE_FREE_MEMORY(pSecDescRel);
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(phReg->Name);
    LWREG_SAFE_FREE_MEMORY(phReg);
    LWREG_SAFE_FREE_MEMORY(rootKey);
    goto cleanup;
}


NTSTATUS
MemRegStoreFindNodeSubkey(
    IN PMEMREG_STORE_NODE hDb,
    IN PCWSTR pwszSubKeyPath,
    OUT PMEMREG_STORE_NODE * phNode)
{
    NTSTATUS status = 0;
    PWSTR pwszTmpFullPath = NULL;
    PWSTR pwszSubKey = NULL;
    PWSTR pwszPtr = NULL;
    PMEMREG_STORE_NODE hParentKey = NULL;
    PMEMREG_STORE_NODE hSubKey = NULL;
    BOOLEAN bEndOfString = FALSE;


    BAIL_ON_NT_STATUS(status);

    if (!pwszSubKeyPath)
    {
        pwszSubKeyPath = (PCWSTR) L"";
    }


    status = LwRtlWC16StringDuplicate(&pwszTmpFullPath, pwszSubKeyPath);
    BAIL_ON_NT_STATUS(status);

    hParentKey = hDb;
    pwszSubKey = pwszTmpFullPath;
    do
    {
        pwszPtr = pwstr_wcschr(pwszSubKey, L'\\');
        if (pwszPtr)
        {
            *pwszPtr++ = L'\0';
        }
        else
        {
            pwszPtr = pwszSubKey;
            bEndOfString = TRUE;
        }

        /*
         * Iterate over subkeys in \ sepearated path.
         */
        status = MemRegStoreFindNode(
                     hParentKey,
                     pwszSubKey,
                     &hSubKey);
        hParentKey = hSubKey;
        pwszSubKey = pwszPtr;
    } while (status == 0 && !bEndOfString);
    if (status == 0)
    {
        *phNode = hParentKey;
    }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszTmpFullPath);
    return status;
error:
    goto cleanup;
}


NTSTATUS
MemRegStoreFindNode(
    IN PMEMREG_STORE_NODE hDb,
    IN PCWSTR Name,
    OUT PMEMREG_STORE_NODE *pphNode)
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
        if (hDb->SubNodes[nodeIndex] &&
            LwRtlWC16StringIsEqual(Name, hDb->SubNodes[nodeIndex]->Name, FALSE))
        {
            bFoundNode = TRUE;
            break;
        }
    }

    if (bFoundNode)
    {
        *pphNode = hDb->SubNodes[nodeIndex];
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
MemRegStoreDeleteNode(
    IN PMEMREG_STORE_NODE hDb)
{
    NTSTATUS status = 0;
    DWORD index = 0;
    BOOLEAN bNodeFound = FALSE;

    if (!hDb->ParentNode)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    /* Delete memory for this node here */
    for (index=0; index < hDb->ValuesLen; index++)
    {
        LWREG_SAFE_FREE_MEMORY(hDb->Values[index]->Name);
        LWREG_SAFE_FREE_MEMORY(hDb->Values[index]->Data);
        hDb->Values[index]->DataLen = 0;
        LWREG_SAFE_FREE_MEMORY(hDb->Values[index]->Attributes.pDefaultValue);
        LWREG_SAFE_FREE_MEMORY(hDb->Values[index]->Attributes.pwszDocString);
        if (hDb->Values[index]->Attributes.RangeType == 
            LWREG_VALUE_RANGE_TYPE_ENUM)
        {
            _MemDbFreeWC16Array(
                hDb->Values[index]->Attributes.Range.ppwszRangeEnumStrings);
        }
        LWREG_SAFE_FREE_MEMORY(hDb->Values[index]);
    }
    LWREG_SAFE_FREE_MEMORY(hDb->Values);

    /* Remove this node from parent SubNodes list */
    for (index=0; index < hDb->ParentNode->NodesLen; index++)
    {
        if (hDb->ParentNode->SubNodes[index] == hDb)
        {
            bNodeFound = TRUE;
            break;
        }
    }
    if (bNodeFound)
    {
        hDb->ParentNode->SubNodes[index] = NULL;

        /* Shift all pointers right of node just removed left over empty slot */
        if (index+1 < hDb->ParentNode->NodesLen)
        {
            memmove(&hDb->ParentNode->SubNodes[index], 
                    &hDb->ParentNode->SubNodes[index+1], 
                    (hDb->ParentNode->NodesLen-index-1) * 
                        sizeof(hDb->ParentNode->SubNodes[index]));
            hDb->ParentNode->SubNodes[hDb->ParentNode->NodesLen-1] = NULL;
            hDb->ParentNode->NodesLen--;
        }
        else if (hDb->ParentNode->NodesLen == 1)
        {
            LWREG_SAFE_FREE_MEMORY(hDb->ParentNode->SubNodes);
            hDb->ParentNode->NodesLen = 0;
        }
        else if (index+1 == hDb->ParentNode->NodesLen)
        {
            /* Last entry on the list. Value is nulled out, decrement len */
            hDb->ParentNode->NodesLen--;
        }
    }
    if (hDb->pNodeSd && hDb->pNodeSd->SecurityDescriptorAllocated)
    {
        LWREG_SAFE_FREE_MEMORY(hDb->pNodeSd->SecurityDescriptor);
    }
    LWREG_SAFE_FREE_MEMORY(hDb->pNodeSd);
    LWREG_SAFE_FREE_MEMORY(hDb->Name);
    LWREG_SAFE_FREE_MEMORY(hDb);

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemRegStoreAddNode(
    IN PMEMREG_STORE_NODE hParentNode,
    PCWSTR Name,
    DWORD NodeType,
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    ULONG SecurityDescriptorLen,
    OUT PMEMREG_STORE_NODE * phNode,
    OUT OPTIONAL PMEMREG_STORE_NODE *ppRetNewNode)
{
    NTSTATUS status = 0;
    PREGMEM_NODE *pNodesArray = NULL;
    PREGMEM_NODE pNewNode = NULL;
    PWSTR newNodeName = NULL;
    DWORD index = 0;
    PREGMEM_NODE_SD pUpdatedNodeSd = NULL;

    status = NtRegReallocMemory(
                 hParentNode->SubNodes, 
                 (PVOID) &pNodesArray,
                 (hParentNode->NodesLen + 1) * sizeof(PREGMEM_NODE));
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE(
                 (PVOID*) &pNewNode, PREGMEM_NODE, sizeof(REGMEM_NODE));
    BAIL_ON_NT_STATUS(status);
    memset(pNewNode, 0, sizeof(*pNewNode));

    status = LwRtlWC16StringDuplicate(&newNodeName, Name);
    BAIL_ON_NT_STATUS(status);
  
    pNodesArray[hParentNode->NodesLen] = NULL;
    hParentNode->SubNodes = pNodesArray;
    pNodesArray = NULL;

    if (NodeType > 1)
    {
        /*
         * Point new node to parent, if not root. Needed by some operations
         * that would manipulate parent structure (e.g. MemDeleteKey)
         */
        pNewNode->ParentNode = hParentNode;
    }

    /* Insert new node in sorted order */
    if (hParentNode->NodesLen > 0)
    {
        for (index=0; 
             index<hParentNode->NodesLen &&
             LwRtlWC16StringCompare(Name, hParentNode->SubNodes[index]->Name)>0;
             index++)
        {
            ;
        }
        if (index < (hParentNode->NodesLen+1))
        {
            memmove(&hParentNode->SubNodes[index+1],
                    &hParentNode->SubNodes[index],
                    sizeof(PREGMEM_NODE) * (hParentNode->NodesLen - index));
            hParentNode->SubNodes[index] = pNewNode;
        }
        else
        {
            hParentNode->SubNodes[hParentNode->NodesLen] = pNewNode;
        }
    }
    else
    {
        hParentNode->SubNodes[hParentNode->NodesLen] = pNewNode;
    }

    pNewNode->NodeType = NodeType;
    pNewNode->Name = newNodeName;
    newNodeName = NULL;

    status = MemRegStoreCreateSecurityDescriptor(
                 hParentNode->pNodeSd,
                 SecurityDescriptor,
                 SecurityDescriptorLen,
                 &pUpdatedNodeSd);
    BAIL_ON_NT_STATUS(status);
    if (pNewNode->pNodeSd)
    {
        LWREG_SAFE_FREE_MEMORY(pNewNode->pNodeSd->SecurityDescriptor);
    }
    LWREG_SAFE_FREE_MEMORY(pNewNode->pNodeSd);
    pNewNode->pNodeSd = pUpdatedNodeSd;

    hParentNode->NodesLen++;

    if (phNode)
    {
        *phNode = hParentNode;
    }
    if (ppRetNewNode)
    {
        *ppRetNewNode = pNewNode;
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
    IN PMEMREG_STORE_NODE hDb,
    IN PCWSTR Name,
    OUT PREGMEM_VALUE *phValue)
{
    NTSTATUS status = 0;
    DWORD valueIndex = 0;
    BOOLEAN bFoundValue = FALSE;

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

    return status;

}


NTSTATUS
MemRegStoreChangeNodeValue(
    IN PREGMEM_VALUE pNodeValue,
    IN const BYTE *pData,
    DWORD cbData)
{
    NTSTATUS status = 0;

    if (pNodeValue->Data)
    {
        LWREG_SAFE_FREE_MEMORY(pNodeValue->Data);
        pNodeValue->DataLen = 0;
    }
    status = LW_RTL_ALLOCATE(
                 (PVOID*) &pNodeValue->Data, 
                 BYTE, 
                 sizeof(*pData) * cbData);
    BAIL_ON_NT_STATUS(status);

    memcpy(pNodeValue->Data, pData, cbData);
    pNodeValue->DataLen = cbData;

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pNodeValue->Data);
    goto cleanup;
}


NTSTATUS
MemRegStoreAddNodeValue(
    PMEMREG_STORE_NODE hDb,
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
    DWORD index = 0;

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
        if (pData)
        {
            memcpy(pbData, pData, cbData);
        }
    }

    status = NtRegReallocMemory(hDb->Values, 
                                (PVOID) &newValues,
                                (hDb->ValuesLen + 1) * sizeof(PREGMEM_VALUE));
    BAIL_ON_NT_STATUS(status);
    hDb->Values = newValues;

    pNodeValue->Name = pwszName;
    pNodeValue->Type = dwType;

    /* Clear out existing data before overwriting with new buffer */
    LWREG_SAFE_FREE_MEMORY(pNodeValue->Data);
    pNodeValue->Data = pbData;
    pNodeValue->DataLen = cbData;

    /* Insert new value in sorted order */
    if (hDb->ValuesLen > 0 && pValueName)
    {
        for (index=0;
             index<hDb->ValuesLen &&
             LwRtlWC16StringCompare(pValueName, hDb->Values[index]->Name)>0;
             index++)
        {
            ;
        }
        if (index < (hDb->ValuesLen+1))
        {
            memmove(&hDb->Values[index+1],
                    &hDb->Values[index],
                    sizeof(PREGMEM_NODE) * (hDb->ValuesLen - index));
            hDb->Values[index] = pNodeValue;
        }
        else
        {
            hDb->Values[hDb->ValuesLen] = pNodeValue;
        }
    }
    else
    {
        hDb->Values[hDb->ValuesLen] = pNodeValue;
    }

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
MemRegStoreDeleteNodeValue(
    IN PMEMREG_STORE_NODE hDb,
    IN PCWSTR Name)
{
    NTSTATUS status = 0;
    PREGMEM_VALUE nodeValue = NULL;

    status = MemRegStoreFindNodeValue(
                 hDb,
                 Name,
                 &nodeValue);
    BAIL_ON_NT_STATUS(status);
   
    if (nodeValue->Data)
    {
        LWREG_SAFE_FREE_MEMORY(nodeValue->Data);
        nodeValue->DataLen = 0;
    }
    else
    {
        status = STATUS_CANNOT_DELETE;
    }

cleanup:
    return status;

error:
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
    hValue->Attributes = *pAttributes;
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
    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;
    PWSTR *ppwszEnumStrings = NULL;
    PWSTR pwszEnumString = NULL;
    PWSTR pwszDocString = NULL;
    PLWREG_CURRENT_VALUEINFO pCurrentValue = NULL;
    PLWREG_VALUE_ATTRIBUTES pValueAttributes = NULL;
    PBYTE pRetCurrentValueData = NULL;
    PBYTE pRetAttributeValueData = NULL;
    DWORD dwValueLen = 0;

    dwValueLen = hValue->DataLen;
    if (ppCurrentValue && dwValueLen > 0 &&
        hValue->Attributes.DefaultValueLen > 0)
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
                                     sizeof(*pCurrentValue));
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

    if (ppValueAttributes && hValue->Attributes.DefaultValueLen > 0)
    {
        /* Always allocate a return attributes block */
        status = LW_RTL_ALLOCATE((PVOID*) &pValueAttributes,
                                 LWREG_VALUE_ATTRIBUTES,
                                 sizeof(*pValueAttributes));
        BAIL_ON_NT_STATUS(status);
        memset(pValueAttributes, 0, sizeof(*pValueAttributes));

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
        /* 
         * The default type must always be returned by the
         * attributes structure. Seems like a bug with value
         * handling on the client-side.
         */
        if (hValue->Attributes.ValueType == 0)
        {
            pValueAttributes->ValueType = hValue->Type;
        }
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


NTSTATUS
MemRegStoreCreateNodeSdFromSddl(
    IN PSTR SecurityDescriptor,
    IN ULONG SecurityDescriptorLen,
    PREGMEM_NODE_SD *ppRetNodeSd)
{
    NTSTATUS status = 0;
    PREGMEM_NODE_SD pNodeSd = NULL;

    if (!SecurityDescriptor || SecurityDescriptorLen == 0)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }
    
    status = LW_RTL_ALLOCATE((PVOID*) &pNodeSd,
                                      PREGMEM_NODE_SD,
                                      sizeof(*pNodeSd));
    BAIL_ON_NT_STATUS(status);

    /* Add security descriptor to current key node */
    status = RtlAllocateSecurityDescriptorFromSddlCString(
                 &pNodeSd->SecurityDescriptor,
                 &pNodeSd->SecurityDescriptorLen,
                 SecurityDescriptor,
                 SDDL_REVISION_1);
    BAIL_ON_NT_STATUS(status);
    pNodeSd->SecurityDescriptorAllocated = TRUE;
  
    *ppRetNodeSd = pNodeSd;

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pNodeSd->SecurityDescriptor);
    LWREG_SAFE_FREE_MEMORY(pNodeSd);
    goto cleanup;
}


NTSTATUS
MemRegStoreCreateSecurityDescriptor(
    PREGMEM_NODE_SD pParentSd,
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    ULONG SecurityDescriptorLen,
    PREGMEM_NODE_SD *ppUpdatedNodeSd)
{
    NTSTATUS status = 0;
    PSECURITY_DESCRIPTOR_RELATIVE NewSecurityDescriptor = NULL;
    PREGMEM_NODE_SD pNodeSd = NULL;
    PREGMEM_NODE_SD pNewNodeSd = NULL;
    BOOLEAN bInheritParent = FALSE;

    if (pParentSd)
    {
        if (SecurityDescriptor && SecurityDescriptorLen)
        {
            if (pParentSd->SecurityDescriptorLen == SecurityDescriptorLen &&
            memcmp(pParentSd->SecurityDescriptor,
                   SecurityDescriptor,
                   SecurityDescriptorLen) == 0)
            {
                bInheritParent = TRUE;
            }
        }
        else
        {
            bInheritParent = TRUE;
        }
    }

    status = LW_RTL_ALLOCATE(
                 (PVOID *) &pNodeSd,
                 PREGMEM_NODE_SD,
                 sizeof(*pNewNodeSd));
    BAIL_ON_NT_STATUS(status);

    if (bInheritParent)
    {
        /* Inherit SD from parent node if one is not provided */
        pNodeSd->SecurityDescriptor = pParentSd->SecurityDescriptor;
        pNodeSd->SecurityDescriptorLen = pParentSd->SecurityDescriptorLen;
    }
    else
    {
        status = LW_RTL_ALLOCATE((PVOID *) &NewSecurityDescriptor,
                                 PSECURITY_DESCRIPTOR_RELATIVE,
                                 SecurityDescriptorLen);
        BAIL_ON_NT_STATUS(status);
    
        pNodeSd->SecurityDescriptor = NewSecurityDescriptor;
        memcpy(pNodeSd->SecurityDescriptor,
               SecurityDescriptor,
               SecurityDescriptorLen);
        pNodeSd->SecurityDescriptorLen = SecurityDescriptorLen;
        pNodeSd->SecurityDescriptorAllocated = TRUE;
    }

    *ppUpdatedNodeSd = pNodeSd;
cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pNodeSd);
    LWREG_SAFE_FREE_MEMORY(NewSecurityDescriptor);
    goto cleanup;
}
