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
    OUT PMEMREG_NODE *pphDbNode)
{
    NTSTATUS status = 0;
    PMEMREG_NODE hRegNode = NULL;
    PWSTR rootKey = NULL;
    PMEMREG_NODE hRootNode = NULL;
    DWORD i = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    ULONG ulSecDescLen = 0;

    /* This is the ROOT node (\) of the registry */
    status = LW_RTL_ALLOCATE(
                 (PVOID*)&hRegNode, 
                 PMEMREG_NODE, 
                 sizeof(*hRegNode));
    BAIL_ON_NT_STATUS(status);
    memset(hRegNode, 0, sizeof(*hRegNode));

    hRegNode->NodeType = MEMREG_TYPE_ROOT;
    status = LwRtlWC16StringAllocateFromCString(
                 &hRegNode->Name, "\\");
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
                     hRegNode,
                     rootKey,
                     MEMREG_TYPE_HIVE,
                     pSecDescRel,  // SD parameter
                     ulSecDescLen,
                     &hRootNode,
                     NULL);
        BAIL_ON_NT_STATUS(status);
        LWREG_SAFE_FREE_MEMORY(rootKey);
    }

    *pphDbNode = hRegNode;

cleanup:
    LWREG_SAFE_FREE_MEMORY(pSecDescRel);
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(hRegNode->Name);
    LWREG_SAFE_FREE_MEMORY(hRegNode);
    LWREG_SAFE_FREE_MEMORY(rootKey);
    goto cleanup;
}

NTSTATUS
MemRegStoreClose(
    IN PMEMREG_NODE hRootNode)
{
    NTSTATUS status = 0;

    /* Validate is actually root of registry */
    if (!hRootNode || hRootNode->NodeType != 1)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    if (hRootNode->Name)
    {
        LWREG_SAFE_FREE_MEMORY(hRootNode->Name);
    }

    LWREG_SAFE_FREE_MEMORY(hRootNode);
cleanup:
    return status;

error: 
    goto cleanup;
}


NTSTATUS
MemRegStoreFindNodeSubkey(
    IN PMEMREG_NODE hDbNode,
    IN PCWSTR pwszSubKeyPath,
    OUT PMEMREG_NODE * phNode)
{
    NTSTATUS status = 0;
    PWSTR pwszTmpFullPath = NULL;
    PWSTR pwszSubKey = NULL;
    PWSTR pwszPtr = NULL;
    PMEMREG_NODE hParentKey = NULL;
    PMEMREG_NODE hSubKey = NULL;
    BOOLEAN bEndOfString = FALSE;


    BAIL_ON_NT_STATUS(status);

    if (!pwszSubKeyPath)
    {
        pwszSubKeyPath = (PCWSTR) L"";
    }


    status = LwRtlWC16StringDuplicate(&pwszTmpFullPath, pwszSubKeyPath);
    BAIL_ON_NT_STATUS(status);

    hParentKey = hDbNode;
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
    IN PMEMREG_NODE hDbNode,
    IN PCWSTR Name,
    OUT PMEMREG_NODE *pphNode)
{
    NTSTATUS status = 0;
    DWORD nodeIndex = 0;
    BOOLEAN bFoundNode = FALSE;

    BAIL_ON_NT_STATUS(status);

    if (!Name)
    {
        Name = (PCWSTR) L"";
    }

    for (nodeIndex=0; nodeIndex<hDbNode->NodesLen; nodeIndex++)
    {
        if (hDbNode->SubNodes[nodeIndex] &&
            LwRtlWC16StringIsEqual(Name, hDbNode->SubNodes[nodeIndex]->Name, FALSE))
        {
            bFoundNode = TRUE;
            break;
        }
    }

    if (bFoundNode)
    {
        *pphNode = hDbNode->SubNodes[nodeIndex];
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
    IN PMEMREG_NODE hDbNode)
{
    NTSTATUS status = 0;
    DWORD index = 0;
    BOOLEAN bNodeFound = FALSE;

    if (!hDbNode->ParentNode)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    /* Delete memory for this node here */
    for (index=0; index < hDbNode->ValuesLen; index++)
    {
        LWREG_SAFE_FREE_MEMORY(hDbNode->Values[index]->Name);
        LWREG_SAFE_FREE_MEMORY(hDbNode->Values[index]->Data);
        hDbNode->Values[index]->DataLen = 0;
        LWREG_SAFE_FREE_MEMORY(hDbNode->Values[index]->Attributes.pDefaultValue);
        LWREG_SAFE_FREE_MEMORY(hDbNode->Values[index]->Attributes.pwszDocString);
        if (hDbNode->Values[index]->Attributes.RangeType == 
            LWREG_VALUE_RANGE_TYPE_ENUM)
        {
            _MemDbFreeWC16Array(
                hDbNode->Values[index]->Attributes.Range.ppwszRangeEnumStrings);
        }
        LWREG_SAFE_FREE_MEMORY(hDbNode->Values[index]);
    }
    LWREG_SAFE_FREE_MEMORY(hDbNode->Values);

    /* Remove this node from parent SubNodes list */
    for (index=0; index < hDbNode->ParentNode->NodesLen; index++)
    {
        if (hDbNode->ParentNode->SubNodes[index] == hDbNode)
        {
            bNodeFound = TRUE;
            break;
        }
    }
    if (bNodeFound)
    {
        hDbNode->ParentNode->SubNodes[index] = NULL;

        /* Shift all pointers right of node just removed left over empty slot */
        if (index+1 < hDbNode->ParentNode->NodesLen)
        {
            memmove(&hDbNode->ParentNode->SubNodes[index], 
                    &hDbNode->ParentNode->SubNodes[index+1], 
                    (hDbNode->ParentNode->NodesLen-index-1) * 
                        sizeof(hDbNode->ParentNode->SubNodes[index]));
            hDbNode->ParentNode->SubNodes[hDbNode->ParentNode->NodesLen-1] = NULL;
            hDbNode->ParentNode->NodesLen--;
        }
        else if (hDbNode->ParentNode->NodesLen == 1)
        {
            LWREG_SAFE_FREE_MEMORY(hDbNode->ParentNode->SubNodes);
            hDbNode->ParentNode->NodesLen = 0;
        }
        else if (index+1 == hDbNode->ParentNode->NodesLen)
        {
            /* Last entry on the list. Value is nulled out, decrement len */
            hDbNode->ParentNode->NodesLen--;
        }
    }
    if (hDbNode->pNodeSd && hDbNode->pNodeSd->SecurityDescriptorAllocated)
    {
        LWREG_SAFE_FREE_MEMORY(hDbNode->pNodeSd->SecurityDescriptor);
    }
    LWREG_SAFE_FREE_MEMORY(hDbNode->pNodeSd);
    LWREG_SAFE_FREE_MEMORY(hDbNode->Name);
    LWREG_SAFE_FREE_MEMORY(hDbNode);

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemRegStoreAddNode(
    IN PMEMREG_NODE hParentNode,
    PCWSTR Name,
    DWORD NodeType,
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    ULONG SecurityDescriptorLen,
    OUT PMEMREG_NODE *phRetParentNode,
    OUT OPTIONAL PMEMREG_NODE *ppRetNewNode)
{
    NTSTATUS status = 0;
    PMEMREG_NODE *pNodesArray = NULL;
    PMEMREG_NODE pNewNode = NULL;
    PWSTR newNodeName = NULL;
    DWORD index = 0;
    PMEMREG_NODE_SD pUpdatedNodeSd = NULL;

    if (hParentNode->SubNodeDepth == MEMREG_MAX_SUBNODES)
    {
        status = STATUS_TOO_MANY_NAMES;
        BAIL_ON_NT_STATUS(status);
    }
    status = NtRegReallocMemory(
                 hParentNode->SubNodes, 
                 (PVOID) &pNodesArray,
                 (hParentNode->NodesLen + 1) * sizeof(PMEMREG_NODE));
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE(
                 (PVOID*) &pNewNode, PMEMREG_NODE, sizeof(MEMREG_NODE));
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
                    sizeof(PMEMREG_NODE) * (hParentNode->NodesLen - index));
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
    pNewNode->SubNodeDepth = hParentNode->SubNodeDepth+1;

    if (phRetParentNode)
    {
        *phRetParentNode = hParentNode;
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
    IN PMEMREG_NODE hDbNode,
    IN PCWSTR Name,
    OUT PMEMREG_VALUE *phValue)
{
    NTSTATUS status = 0;
    DWORD valueIndex = 0;
    BOOLEAN bFoundValue = FALSE;

    if (!Name)
    {
        Name = (PCWSTR) L"";
    }
    for (valueIndex=0; valueIndex<hDbNode->ValuesLen; valueIndex++)
    {
        if (LwRtlWC16StringIsEqual(Name, hDbNode->Values[valueIndex]->Name, FALSE))
        {
            bFoundValue = TRUE;
            break;
        }
    }

    if (bFoundValue)
    {
        *phValue = hDbNode->Values[valueIndex];
    }
    else
    {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

    return status;

}


NTSTATUS
MemRegStoreChangeNodeValue(
    IN PMEMREG_VALUE pNodeValue,
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
    PMEMREG_NODE hDbNode,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD dwReserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData)
{
    NTSTATUS status = 0;
    PMEMREG_VALUE pNodeValue = NULL;
    PWSTR pwszName = NULL;
    WCHAR pwszNull[2] = {0};
    BYTE *pbData = NULL;
    PMEMREG_VALUE *newValues;
    DWORD index = 0;

    status = LW_RTL_ALLOCATE(
                 (PVOID*) &pNodeValue, 
                 MEMREG_VALUE, 
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

    status = NtRegReallocMemory(hDbNode->Values, 
                                (PVOID) &newValues,
                                (hDbNode->ValuesLen + 1) * sizeof(PMEMREG_VALUE));
    BAIL_ON_NT_STATUS(status);
    hDbNode->Values = newValues;

    pNodeValue->Name = pwszName;
    pNodeValue->Type = dwType;

    /* Clear out existing data before overwriting with new buffer */
    LWREG_SAFE_FREE_MEMORY(pNodeValue->Data);
    pNodeValue->Data = pbData;
    pNodeValue->DataLen = cbData;

    /* Insert new value in sorted order */
    if (hDbNode->ValuesLen > 0 && pValueName)
    {
        for (index=0;
             index<hDbNode->ValuesLen &&
             LwRtlWC16StringCompare(pValueName, hDbNode->Values[index]->Name)>0;
             index++)
        {
            ;
        }
        if (index < (hDbNode->ValuesLen+1))
        {
            memmove(&hDbNode->Values[index+1],
                    &hDbNode->Values[index],
                    sizeof(PMEMREG_NODE) * (hDbNode->ValuesLen - index));
            hDbNode->Values[index] = pNodeValue;
        }
        else
        {
            hDbNode->Values[hDbNode->ValuesLen] = pNodeValue;
        }
    }
    else
    {
        hDbNode->Values[hDbNode->ValuesLen] = pNodeValue;
    }

    hDbNode->ValuesLen++;

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
    IN PMEMREG_NODE hDbNode,
    IN PCWSTR Name)
{
    NTSTATUS status = 0;
    BOOLEAN bFoundValue = FALSE;
    BOOLEAN bValueDeleted = FALSE;
    DWORD valueIndex = 0;

    if (!Name)
    {
        Name = (PCWSTR) L"";
    }
    for (valueIndex=0; valueIndex<hDbNode->ValuesLen; valueIndex++)
    {
        if (LwRtlWC16StringIsEqual(Name, hDbNode->Values[valueIndex]->Name, FALSE))
        {
            bFoundValue = TRUE;
            break;
        }
    }
    if (bFoundValue)
    {
        if (hDbNode->Values[valueIndex]->Data)
        {
            LWREG_SAFE_FREE_MEMORY(hDbNode->Values[valueIndex]->Data);
            hDbNode->Values[valueIndex]->DataLen = 0;
            bValueDeleted = TRUE;
        }

        if (hDbNode->Values[valueIndex]->Attributes.ValueType == 0)
        {
            if (valueIndex+1 < hDbNode->ValuesLen)
            {
                memmove(
                    &hDbNode->Values[valueIndex],
                    &hDbNode->Values[valueIndex+1],
                    (hDbNode->ValuesLen - valueIndex - 1) * sizeof(PMEMREG_VALUE));
            }
            hDbNode->Values[hDbNode->ValuesLen-1] = NULL;
            hDbNode->ValuesLen--;
            if (hDbNode->ValuesLen == 0)
            {
                LWREG_SAFE_FREE_MEMORY(hDbNode->Values);
                hDbNode->Values = NULL;
            }
        }
        else
        {
            if (!bValueDeleted)
            {
                status = STATUS_CANNOT_DELETE;
            }
        }
    }
    else
    {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    return status;
}


NTSTATUS
MemRegStoreAddNodeAttribute(
    PMEMREG_VALUE hValue,
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
    if (pbData)
    {
        LWREG_SAFE_FREE_MEMORY(hValue->Attributes.pDefaultValue);
    }
    if (pwszDocString)
    {
         LWREG_SAFE_FREE_MEMORY(hValue->Attributes.pwszDocString);
    }

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
    PMEMREG_VALUE hValue,
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
    BOOLEAN bHasAttributes = FALSE;

    dwValueLen = hValue->DataLen;
    if (ppCurrentValue)
    {
        if ((dwValueLen > 0 && hValue->Data &&
            hValue->Attributes.DefaultValueLen &&
            hValue->Attributes.pDefaultValue && 
            (dwValueLen != hValue->Attributes.DefaultValueLen ||
             memcmp(hValue->Data,
                    hValue->Attributes.pDefaultValue,
                    dwValueLen))) ||
             (dwValueLen > 0 && hValue->Data))
        {
            status = LW_RTL_ALLOCATE((PVOID*) &pCurrentValue,
                                     LWREG_CURRENT_VALUEINFO,
                                     sizeof(*pCurrentValue));
            BAIL_ON_NT_STATUS(status);


            status = LW_RTL_ALLOCATE((PVOID*) &pRetCurrentValueData,
                                     BYTE,
                                     dwValueLen);
            BAIL_ON_NT_STATUS(status);
            memset(pRetCurrentValueData, 0, dwValueLen);
            memcpy(pRetCurrentValueData, hValue->Data, dwValueLen);
        }
    }

    if (ppValueAttributes)
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
            bHasAttributes = TRUE;
        }
    
        /*
         * Duplicate the documentation string, if present
         */
        if (hValue->Attributes.pwszDocString) { 
            status = LwRtlWC16StringDuplicate(&pwszDocString, 
                         hValue->Attributes.pwszDocString);
            BAIL_ON_NT_STATUS(status);
            bHasAttributes = TRUE;
        }

        if (hValue->Attributes.RangeType == LWREG_VALUE_RANGE_TYPE_ENUM &&
            hValue->Attributes.Range.ppwszRangeEnumStrings)
        {
            status = _MemRegDuplicateWC16Array(
                         hValue->Attributes.Range.ppwszRangeEnumStrings,
                         &ppwszEnumStrings);
            BAIL_ON_NT_STATUS(status);
            bHasAttributes = TRUE;
        }
    }

    if (!bHasAttributes && ppValueAttributes)
    {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

    /* Assign all allocated data to return optional return structures */
    if (pCurrentValue)
    {
         pCurrentValue->dwType = hValue->Type;
         pCurrentValue->pvData = pRetCurrentValueData;
         pCurrentValue->cbData = dwValueLen;
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

    LWREG_SAFE_FREE_MEMORY(pValueAttributes);
    LWREG_SAFE_FREE_MEMORY(pCurrentValue);
    LWREG_SAFE_FREE_MEMORY(pRetCurrentValueData);
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
    PMEMREG_NODE_SD *ppRetNodeSd)
{
    NTSTATUS status = 0;
    PMEMREG_NODE_SD pNodeSd = NULL;

    if (!SecurityDescriptor || SecurityDescriptorLen == 0)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }
    
    status = LW_RTL_ALLOCATE((PVOID*) &pNodeSd,
                                      PMEMREG_NODE_SD,
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
    PMEMREG_NODE_SD pParentSd,
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    ULONG SecurityDescriptorLen,
    PMEMREG_NODE_SD *ppUpdatedNodeSd)
{
    NTSTATUS status = 0;
    PSECURITY_DESCRIPTOR_RELATIVE NewSecurityDescriptor = NULL;
    PMEMREG_NODE_SD pNodeSd = NULL;
    PMEMREG_NODE_SD pNewNodeSd = NULL;
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
                 PMEMREG_NODE_SD,
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
