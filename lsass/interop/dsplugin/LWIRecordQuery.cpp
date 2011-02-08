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

#include "LWIDirNodeQuery.h"
#include "LWIRecordQuery.h"
#include "LWIQuery.h"
#include "LWICRC.h"
#include <assert.h>

#ifndef kDSStdRecordTypeComputerGroups /* For when building on Tiger based build machines */
/*!
 * @defined kDSStdRecordTypeComputerGroups
 * @discussion Identifies computer group records.
 */
#define kDSStdRecordTypeComputerGroups "dsRecTypeStandard:ComputerGroups"
#endif


LWIRecordQuery::RecordRefMap * LWIRecordQuery::_recordRefMap = NULL;
LWIRecordQuery::AttributeRefMap * LWIRecordQuery::_attributeRefMap = NULL;

long
LWIRecordQuery::Initialize()
{
    long macError = eDSNoErr;
    _recordRefMap = new RecordRefMap();
    if (!_recordRefMap)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP();
    }

    _attributeRefMap = new AttributeRefMap();
    if (!_attributeRefMap)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP();
    }

cleanup:

    if (macError)
    {
        Cleanup();
    }

    return macError;
}

void
LWIRecordQuery::Cleanup()
{
    if (_recordRefMap)
    {
        RecordRefMapIter iter;
        PDSRECORD pRecord = NULL;

        for(iter = _recordRefMap->begin(); iter != _recordRefMap->end(); ++iter)
        {
            pRecord = iter->second;
            if (pRecord)
            {
               LWIQuery::FreeRecordList(pRecord);
            }
        }

        delete _recordRefMap;
        _recordRefMap = NULL;
    }

    if (_attributeRefMap)
    {
        delete _attributeRefMap;
        _attributeRefMap = NULL;
    }
}

long
LWIRecordQuery::GetDataNodeString(tDataNodePtr pDataNode, char** ppszString)
{
    long macError = eDSNoErr;
    char* pszString = NULL;

    if (pDataNode && pDataNode->fBufferLength)
    {
       macError = LwAllocateMemory(pDataNode->fBufferLength+1, (PVOID*)&pszString);
       GOTO_CLEANUP_ON_MACERROR(macError);

       strncpy(pszString, pDataNode->fBufferData, pDataNode->fBufferLength);
    }

    *ppszString = pszString;
    pszString = NULL;

cleanup:

    if (pszString)
    {
       LwFreeMemory(pszString);
    }

    return macError;
}

long
LWIRecordQuery::GetDataNodeValue(tDataNodePtr pDataNode, char** ppData, long * pLength)
{
    long macError = eDSNoErr;
    char* pData = NULL;

    if (pDataNode && pDataNode->fBufferLength)
    {
       macError = LwAllocateMemory(pDataNode->fBufferLength, (PVOID*)&pData);
       GOTO_CLEANUP_ON_MACERROR(macError);

       memcpy(pData, pDataNode->fBufferData, pDataNode->fBufferLength);
    }

    *ppData = pData;
    pData = NULL;
	*pLength = pDataNode->fBufferLength;

cleanup:

    if (pData)
    {
       LwFreeMemory(pData);
    }

    return macError;
}

long
LWIRecordQuery::Open(sOpenRecord* pOpenRecord, LWE_DS_FLAGS Flags, PNETADAPTERINFO pNetAdapterList)
{
    long macError = eDSNoErr;
    LWIQuery* pQuery = NULL;
    PDSRECORD pRecord = NULL;
    char* pszRecType = NULL;
    char* pszRecName = NULL;

    LOG_ENTER("fType = %d, fResult = %d, fInNodeRef = %u, fInRecType = %.*s, "
              "fInRecName = %.*s, fOutRecRef = %u",
              pOpenRecord->fType,
              pOpenRecord->fResult,
              pOpenRecord->fInNodeRef,
              pOpenRecord->fInRecType->fBufferLength,
              pOpenRecord->fInRecType->fBufferData,
              pOpenRecord->fInRecName->fBufferLength,
              pOpenRecord->fInRecName->fBufferData,
              pOpenRecord->fOutRecRef);

    macError = GetDataNodeString(pOpenRecord->fInRecType, &pszRecType);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = GetDataNodeString(pOpenRecord->fInRecName, &pszRecName);
    GOTO_CLEANUP_ON_MACERROR(macError);
	
    if (!strcmp(pOpenRecord->fInRecType->fBufferData, kDSStdRecordTypeUsers))
    {
        macError = LWIQuery::Create(true,
                                    false, // Rely on our error eDSBufferTooSmall to cause the caller to retry
                                    pOpenRecord->fInNodeRef,
                                    Flags,
                                    pNetAdapterList,
                                    &pQuery);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = LWIRecTypeLookup::GetVector(pszRecType, &pQuery->_recTypeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = LWIAttrLookup::GetVector((tDataListPtr)NULL, &pQuery->_attributeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);

        LWI_BITVECTOR_SET(pQuery->_attributeSet, LWIAttrLookup::idx_kDSAttributesAll);

        macError = pQuery->QueryUserInformationByName(pszRecName);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else if (!strcmp(pOpenRecord->fInRecType->fBufferData, kDSStdRecordTypeGroups))
    {
        macError = LWIQuery::Create(true,
                                    false, // Rely on our error eDSBufferTooSmall to cause the caller to retry
                                    pOpenRecord->fInNodeRef,
                                    Flags,
                                    pNetAdapterList,
                                    &pQuery);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = LWIRecTypeLookup::GetVector(pszRecType, &pQuery->_recTypeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = LWIAttrLookup::GetVector((tDataListPtr)NULL, &pQuery->_attributeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);

        LWI_BITVECTOR_SET(pQuery->_attributeSet, LWIAttrLookup::idx_kDSAttributesAll);

        macError = pQuery->QueryGroupInformationByName(pszRecName);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else if (!strcmp(pOpenRecord->fInRecType->fBufferData, kDSStdRecordTypeComputerLists))
    {
        macError = LWIQuery::Create(true,
                                    false, // Rely on our error eDSBufferTooSmall to cause the caller to retry
                                    pOpenRecord->fInNodeRef,
                                    Flags,
                                    pNetAdapterList,
                                    &pQuery);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = LWIRecTypeLookup::GetVector(pszRecType, &pQuery->_recTypeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = LWIAttrLookup::GetVector((tDataListPtr)NULL, &pQuery->_attributeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);

        LWI_BITVECTOR_SET(pQuery->_attributeSet, LWIAttrLookup::idx_kDSAttributesAll);

        macError = pQuery->QueryComputerListInformationByName(pszRecName);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else if (!strcmp(pOpenRecord->fInRecType->fBufferData, kDSStdRecordTypeComputerGroups))
    {
        macError = LWIQuery::Create(true,
                                    false, // Rely on our error eDSBufferTooSmall to cause the caller to retry
                                    pOpenRecord->fInNodeRef,
                                    Flags,
                                    pNetAdapterList,
                                    &pQuery);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = LWIRecTypeLookup::GetVector(pszRecType, &pQuery->_recTypeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = LWIAttrLookup::GetVector((tDataListPtr)NULL, &pQuery->_attributeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);

        LWI_BITVECTOR_SET(pQuery->_attributeSet, LWIAttrLookup::idx_kDSAttributesAll);

        macError = pQuery->QueryComputerGroupInformationByName(pszRecName);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else if (!strcmp(pOpenRecord->fInRecType->fBufferData, kDSStdRecordTypeComputers))
    {
        macError = LWIQuery::Create(true,
                                    false, // Rely on our error eDSBufferTooSmall to cause the caller to retry
                                    pOpenRecord->fInNodeRef,
                                    Flags,
                                    pNetAdapterList,
                                    &pQuery);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = LWIRecTypeLookup::GetVector(pszRecType, &pQuery->_recTypeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = LWIAttrLookup::GetVector((tDataListPtr)NULL, &pQuery->_attributeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);

        LWI_BITVECTOR_SET(pQuery->_attributeSet, LWIAttrLookup::idx_kDSAttributesAll);

        macError = pQuery->QueryComputerInformationByName(pszRecName);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = eDSInvalidRecordType;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pRecord = pQuery->GetRecordList(true /* remove */);
    if (!pRecord)
    {
        LOG_ERROR("Unexpected lack of record");
        macError = ePlugInError;
        GOTO_CLEANUP();
    }

    LWIRecordQuery::_recordRefMap->insert(std::make_pair(pOpenRecord->fOutRecRef, pRecord));
    pRecord = NULL;

cleanup:

    if (pQuery)
    {
        pQuery->Release();
    }

    if (pRecord)
    {
        LWIQuery::FreeRecord(pRecord);
    }

    if (pszRecName)
    {
       LwFreeMemory(pszRecName);
    }

    if (pszRecType)
    {
       LwFreeMemory(pszRecType);
    }

    LOG_LEAVE("--> %d", macError);

    return macError;
}

long
LWIRecordQuery::Close(sCloseRecord* pCloseRecord)
{
    long macError = eDSNoErr;
    PDSRECORD pRecord = NULL;
    RecordRefMapIter iter;

    LOG_ENTER("fType = %d, fResult = %d, fInRecRef = %d",
              pCloseRecord->fType,
              pCloseRecord->fResult,
              pCloseRecord->fInRecRef);

    iter = _recordRefMap->find(pCloseRecord->fInRecRef);
    if (iter != _recordRefMap->end())
    {
        pRecord = iter->second;
        if (pRecord)
        {
           LWIQuery::FreeRecordList(pRecord);
        }
        _recordRefMap->erase(iter);
    }

    LOG_LEAVE("--> %d", macError);

    return macError;
}

static
void
BufferAppendShort(
    OUT PVOID Buffer,
    IN OUT unsigned long* Offset,
    IN uint16_t Value
    )
{
    memcpy((char*)Buffer + *Offset, &Value, sizeof(Value));
    *Offset += sizeof(Value);
}

static
void
BufferAppendData(
    OUT PVOID Buffer,
    IN OUT unsigned long* Offset,
    IN PVOID Data,
    IN unsigned long Length
    )
{
    memcpy((char*)Buffer + *Offset, Data, Length);
    *Offset += Length;
}

long
LWIRecordQuery::GetReferenceInfo(sGetRecRefInfo* pGetRecRefInfo)
{
    long macError = eDSNoErr;
    PDSRECORD pRecord = NULL;
    tRecordEntryPtr pRecordEntry = NULL;
    RecordRefMapIter iter;
    unsigned long offset;
    uint16_t attrCount = 0;

    LOG_ENTER("fType = %d, fResult = %d, fInRecRef = %d, fOutRecInfo = %p",
              pGetRecRefInfo->fType,
              pGetRecRefInfo->fResult,
              pGetRecRefInfo->fInRecRef,
              pGetRecRefInfo->fOutRecInfo);

    iter = LWIRecordQuery::_recordRefMap->find(pGetRecRefInfo->fInRecRef);
    if (iter != LWIRecordQuery::_recordRefMap->end())
    {
        pRecord = iter->second;
    }

    if (!pRecord)
    {
        LOG_ERROR("Invalid record reference: %d", pGetRecRefInfo->fInRecRef);
        macError = eDSInvalidRecordRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    // NI plug-in code had +8 instead of +4 for some reason...
    macError = LwAllocateMemory(sizeof(*pRecordEntry) + 2 + pRecord->nameLen + 2 + pRecord->typeLen, (PVOID*)&pRecordEntry);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::GetNumberOfAttributes(pRecord, attrCount);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecordEntry->fRecordAttributeCount = attrCount;

    offset = 0;
    BufferAppendShort(pRecordEntry->fRecordNameAndType.fBufferData, &offset, pRecord->nameLen);
    BufferAppendData(pRecordEntry->fRecordNameAndType.fBufferData, &offset, pRecord->pszName, pRecord->nameLen);
    BufferAppendShort(pRecordEntry->fRecordNameAndType.fBufferData, &offset, pRecord->typeLen);
    BufferAppendData(pRecordEntry->fRecordNameAndType.fBufferData, &offset, pRecord->pszType, pRecord->typeLen);

    pRecordEntry->fRecordNameAndType.fBufferLength = offset;
    pRecordEntry->fRecordNameAndType.fBufferSize = offset;

    pGetRecRefInfo->fOutRecInfo = pRecordEntry;
    pRecordEntry = NULL;

cleanup:

    if (pRecordEntry)
    {
        LwFreeMemory(pRecordEntry);
    }

    LOG_LEAVE("fOutRecInfo = @%p --> %d", pGetRecRefInfo->fOutRecInfo, macError);

    return macError;
}

long
LWIRecordQuery::GetAttributeInfo(sGetRecAttribInfo* pGetRecAttribInfo)
{
    long macError = eDSNoErr;
    RecordRefMapIter iter;
    PDSRECORD pRecord = NULL;
    PDSATTRIBUTE pAttribute = NULL;
    tAttributeEntryPtr pAttributeEntry = NULL;

    LOG_ENTER("fType = %d, fResult = %d, fInRecRef = %d, fInAttrType = @%p => { \"%.*s\" }, fOutAttrInfoPtr = @%p",
              pGetRecAttribInfo->fType,
              pGetRecAttribInfo->fResult,
              pGetRecAttribInfo->fInRecRef,
              pGetRecAttribInfo->fInAttrType,
              pGetRecAttribInfo->fInAttrType ? pGetRecAttribInfo->fInAttrType->fBufferLength : 0,
              pGetRecAttribInfo->fInAttrType ? pGetRecAttribInfo->fInAttrType->fBufferData : NULL,
              pGetRecAttribInfo->fOutAttrInfoPtr);

    if (pGetRecAttribInfo == NULL)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    iter = LWIRecordQuery::_recordRefMap->find(pGetRecAttribInfo->fInRecRef);
    if (iter != LWIRecordQuery::_recordRefMap->end())
    {
        pRecord = iter->second;
    }

    if (!pRecord)
    {
        LOG_ERROR("Invalid record reference: %d", pGetRecAttribInfo->fInRecRef);
        macError = eDSInvalidRecordRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = LWIQuery::FindAttributeByType(pRecord, pGetRecAttribInfo->fInAttrType->fBufferData, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::CreateAttributeEntry(&pAttributeEntry, pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pGetRecAttribInfo->fOutAttrInfoPtr = pAttributeEntry;
    pAttributeEntry = NULL;

cleanup:

    if (pAttributeEntry)
    {
        LwFreeMemory(pAttributeEntry);
    }

    LOG_LEAVE("fOutAttrInfoPtr = @%p --> %d",
              pGetRecAttribInfo->fOutAttrInfoPtr,
              macError);

    return macError;
}

long
LWIRecordQuery::GetAttributeValueByID(sGetRecordAttributeValueByID* pGetRecAttribValueByID)
{
    long macError = eDSNoErr;
    RecordRefMapIter iter;
    PDSRECORD pRecord = NULL;
    PDSATTRIBUTE pAttribute = NULL;
    PDSATTRIBUTEVALUE pAttributeValue = NULL;
    tAttributeValueEntryPtr pAttributeValueEntry = NULL;

    LOG_ENTER("fType = %d, fResult = %d, fInRecRef = %d, fInAttrType = @%p => { \"%.*s\" }, fInValueID = %d, fOutEntryPtr = @%p",
              pGetRecAttribValueByID->fType,
              pGetRecAttribValueByID->fResult,
              pGetRecAttribValueByID->fInRecRef,
              pGetRecAttribValueByID->fInAttrType,
              pGetRecAttribValueByID->fInAttrType ? pGetRecAttribValueByID->fInAttrType->fBufferLength : 0,
              pGetRecAttribValueByID->fInAttrType ? pGetRecAttribValueByID->fInAttrType->fBufferData : NULL,
              pGetRecAttribValueByID->fInValueID,
              pGetRecAttribValueByID->fOutEntryPtr);

    if (pGetRecAttribValueByID == NULL)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    iter = LWIRecordQuery::_recordRefMap->find(pGetRecAttribValueByID->fInRecRef);
    if (iter != LWIRecordQuery::_recordRefMap->end())
    {
        pRecord = iter->second;
    }

    if (!pRecord)
    {
        LOG_ERROR("Invalid record reference: %d", pGetRecAttribValueByID->fInRecRef);
        macError = eDSInvalidRecordRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = LWIQuery::FindAttributeByType(pRecord, pGetRecAttribValueByID->fInAttrType->fBufferData, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::FindAttributeValueByID(pAttribute, pGetRecAttribValueByID->fInValueID, &pAttributeValue);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::CreateAttributeValueEntry(&pAttributeValueEntry, pAttributeValue);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pGetRecAttribValueByID->fOutEntryPtr = pAttributeValueEntry;
    pAttributeValueEntry = NULL;

cleanup:

    if (pAttributeValueEntry)
    {
        LwFreeMemory(pAttributeValueEntry);
    }

    LOG_LEAVE("fOutAttrInfoPtr = @%p --> %d",
              pGetRecAttribValueByID->fOutEntryPtr,
              macError);

    return macError;
}

long
LWIRecordQuery::GetAttributeValueByIndex(sGetRecordAttributeValueByIndex* pGetRecAttribValueByIndex)
{
    long macError = eDSNoErr;
    RecordRefMapIter iter;
    PDSRECORD pRecord = NULL;
    PDSATTRIBUTE pAttribute = NULL;
    PDSATTRIBUTEVALUE pAttributeValue = NULL;
    tAttributeValueEntryPtr pAttributeValueEntry = NULL;

    LOG_ENTER("fType = %d, fResult = %d, fInRecRef = %d, fInAttrType = @%p => { \"%.*s\" }, fInAttrValueIndex = %d, fOutAttrInfoPtr = @%p",
              pGetRecAttribValueByIndex->fType,
              pGetRecAttribValueByIndex->fResult,
              pGetRecAttribValueByIndex->fInRecRef,
              pGetRecAttribValueByIndex->fInAttrType,
              pGetRecAttribValueByIndex->fInAttrType ? pGetRecAttribValueByIndex->fInAttrType->fBufferLength : 0,
              pGetRecAttribValueByIndex->fInAttrType ? pGetRecAttribValueByIndex->fInAttrType->fBufferData : NULL,
              pGetRecAttribValueByIndex->fInAttrValueIndex,
              pGetRecAttribValueByIndex->fOutEntryPtr);

    if (pGetRecAttribValueByIndex == NULL)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    iter = LWIRecordQuery::_recordRefMap->find(pGetRecAttribValueByIndex->fInRecRef);
    if (iter != LWIRecordQuery::_recordRefMap->end())
    {
        pRecord = iter->second;
    }

    if (!pRecord)
    {
        LOG_ERROR("Invalid record reference: %d", pGetRecAttribValueByIndex->fInRecRef);
        macError = eDSInvalidRecordRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = LWIQuery::FindAttributeByType(pRecord, pGetRecAttribValueByIndex->fInAttrType->fBufferData, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::FindAttributeValueByIndex(pAttribute, pGetRecAttribValueByIndex->fInAttrValueIndex, &pAttributeValue);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::CreateAttributeValueEntry(&pAttributeValueEntry, pAttributeValue);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pGetRecAttribValueByIndex->fOutEntryPtr = pAttributeValueEntry;
    pAttributeValueEntry = NULL;

cleanup:

    if (pAttributeValueEntry)
    {
        LwFreeMemory(pAttributeValueEntry);
    }

    LOG_LEAVE("fOutAttrInfoPtr = @%p --> %d",
              pGetRecAttribValueByIndex->fOutEntryPtr,
              macError);

    return macError;
}

long
LWIRecordQuery::AddAttribute(sAddAttribute* pAddAttribute)
{
    long macError = eDSNoErr;
	char* pszAttribute = NULL;
	char* pValue = NULL;
	long length = 0;
    PDSRECORD pRecord = NULL;
    RecordRefMapIter iter;
	PDSATTRIBUTE pAttribute = NULL;

    macError = GetDataNodeString(pAddAttribute->fInNewAttr, &pszAttribute);
	GOTO_CLEANUP_ON_MACERROR(macError);

    macError = GetDataNodeValue(pAddAttribute->fInFirstAttrValue, &pValue, &length);
	GOTO_CLEANUP_ON_MACERROR(macError);
	
    LOG_ENTER("fInRecRef = %d, Attribute = %s, Value len = %d",
              pAddAttribute->fInRecRef,
			  pszAttribute,
			  length);

    iter = LWIRecordQuery::_recordRefMap->find(pAddAttribute->fInRecRef);
    if (iter != LWIRecordQuery::_recordRefMap->end())
    {
        pRecord = iter->second;
    }

    if (!pRecord)
    {
        LOG_ERROR("Invalid record reference: %d", pAddAttribute->fInRecRef);
        macError = eDSInvalidRecordRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    LOG("Removing old attribute (%s) settings on record (%s)",
        pszAttribute, pRecord->pszName);

    LWIQuery::RemoveAttributeAndValues(pszAttribute, pRecord);
		  
    LOG("Saving off new attribute (%s) value len (%d) settings on record (%s)",
        pszAttribute, length, pRecord->pszName);

    macError = LWIQuery::AddAttribute(pszAttribute, pRecord, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::SetAttributeValue(pAttribute, pValue, length);
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* Mark this pRecord dirty so that we know to save off changes to AD */
    pRecord->fDirty = TRUE;

cleanup:

    if (pszAttribute)
    {
        LwFreeMemory(pszAttribute);
    }

    if (pValue)
    {
        LwFreeMemory(pValue);
    }

    LOG_LEAVE("--> %d", macError);

    return macError;
}

long
LWIRecordQuery::AddAttributeValue(sAddAttributeValue* pAddAttributeValue)
{
    long macError = eDSNoErr;
	char* pszAttribute = NULL;
	char* pszValue = NULL;
    PDSRECORD pRecord = NULL;
    RecordRefMapIter iter;
	PDSATTRIBUTE pAttribute = NULL;
	
    macError = GetDataNodeString(pAddAttributeValue->fInAttrType, &pszAttribute);
	GOTO_CLEANUP_ON_MACERROR(macError);

    LOG("AddAttributeValue processing: Attribute = %s",
		pszAttribute);

    macError = GetDataNodeString(pAddAttributeValue->fInAttrValue, &pszValue);
	GOTO_CLEANUP_ON_MACERROR(macError);
			
	LOG_ENTER("fInRecRef = %d", pAddAttributeValue->fInRecRef);

    iter = LWIRecordQuery::_recordRefMap->find(pAddAttributeValue->fInRecRef);
    if (iter != LWIRecordQuery::_recordRefMap->end())
    {
        pRecord = iter->second;
    }

    if (!pRecord)
    {
        LOG_ERROR("Invalid record reference: %d", pAddAttributeValue->fInRecRef);
        macError = eDSInvalidRecordRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
	
    macError = LWIQuery::FindAttributeByType(pRecord, pszAttribute, &pAttribute);
	if (macError == eDSAttributeNotFound)
	{
	    macError = LWIQuery::AddAttribute(pszAttribute, pRecord, &pAttribute);
	}
    GOTO_CLEANUP_ON_MACERROR(macError);
	
	macError = LWIQuery::SetAttributeValue(pAttribute, pszValue);
	GOTO_CLEANUP_ON_MACERROR(macError);
	
	LOG("Saving off additional attribute (%s) setting on object (%s)",
	    pszAttribute, pRecord->pszName);

    /* Mark this pRecord dirty so that we know to save off changes to AD */
	pRecord->fDirty = TRUE;

cleanup:

    if (pszAttribute)
	{
	    LwFreeMemory(pszAttribute);
	}
	
	if (pszValue)
	{
	    LwFreeMemory(pszValue);
	}
	
    LOG_LEAVE("--> %d", macError);

    return macError;
}

long
LWIRecordQuery::SetAttributeValues(sSetAttributeValues* pSetAttributeValues)
{
    long  macError = eDSNoErr;
    char* pszAttribute = NULL;
    char* pValue = NULL;
    long length = 0;
    PDSRECORD pRecord = NULL;
    RecordRefMapIter iter;
    tDataNodePtr pDataNode = NULL;
    int valueCount = 0;
    int i = 0;
    PDSDIRNODE pDirNode = NULL;
    PDSATTRIBUTE pAttribute = NULL;
    PMCXVALUE pMCXValueList = NULL;
    DWORD dwPolicyType = UNKNOWN_GROUP_POLICY;

    valueCount = dsDataListGetNodeCount(pSetAttributeValues->fInAttrValueList);

    macError = GetDataNodeString(pSetAttributeValues->fInAttrType, &pszAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);
	
    LOG_ENTER("fType = %d, fResult = %d, fInRecRef = %d, Attribute = %s, Number of values = %d",
              pSetAttributeValues->fType,
              pSetAttributeValues->fResult,
              pSetAttributeValues->fInRecRef,
			  pszAttribute,
              valueCount);
			  
    iter = LWIRecordQuery::_recordRefMap->find(pSetAttributeValues->fInRecRef);
    if (iter != LWIRecordQuery::_recordRefMap->end())
    {
        pRecord = iter->second;
    }

    if (!pRecord)
    {
        LOG_ERROR("Invalid record reference: %d", pSetAttributeValues->fInRecRef);
        macError = eDSInvalidRecordRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (!strcmp(pRecord->pszType, kDSStdRecordTypeComputerGroups) ||
        !strcmp(pRecord->pszType, kDSStdRecordTypeComputerLists))
    {
       dwPolicyType = MACHINE_GROUP_POLICY;
    }
    else if (!strcmp(pRecord->pszType, kDSStdRecordTypeGroups))
    {
        dwPolicyType = USER_GROUP_POLICY;
    }
    else
    {
        macError = eDSInvalidRecordType;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
	
    LOG("Replacing attribute (%s) settings on record (%s)", pszAttribute, pRecord->pszName);

    LWIQuery::RemoveAttributeAndValues(pszAttribute, pRecord);
    
    macError = LWIQuery::AddAttribute(pszAttribute, pRecord, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    for (i = 0; i < valueCount; i++)
    {
        macError = dsDataListGetNodeAlloc( pSetAttributeValues->fInRecRef,
                                           pSetAttributeValues->fInAttrValueList,
                                           i+1,
                                           &pDataNode );
        GOTO_CLEANUP_ON_MACERROR( macError );
        
        macError = GetDataNodeValue(pDataNode, &pValue, &length);
        GOTO_CLEANUP_ON_MACERROR( macError );
        
        // Add to record
        macError = LWIQuery::SetAttributeValue(pAttribute, pValue, length);
        GOTO_CLEANUP_ON_MACERROR(macError);

        LwFreeMemory(pValue);
        pValue = NULL;
        
        dsDataNodeDeAllocate(NULL, pDataNode);
        pDataNode = NULL;
    }

    macError = LWIDirNodeQuery::GetDsDirNodeRef(pRecord->dirNodeRef, &pDirNode);
    GOTO_CLEANUP_ON_MACERROR(macError);

    LOG("Flushing attributes that are changed for record (%s) of node (%s) auth is (%s)",
        pRecord->pszName, pDirNode->pszDirNodePath, pDirNode->pszDirNodeUserUPN ? pDirNode->pszDirNodeUserUPN : "Unknown");

    pAttribute = pRecord->pAttributeListHead;

    while (pAttribute)
    {
        if (!strcmp(pAttribute->pszName, kDS1AttrMCXSettings))
        {
            macError = ConvertDSAttributeValuesToMCXValues(pAttribute->pValueListHead, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        pAttribute = pAttribute->pNext;
    }

    macError = SaveMCXValuesForGPOSettingType(pMCXValueList, pDirNode->pDirNodeGPO, dwPolicyType, pDirNode->pszDirNodeUserUPN);
    if (macError)
    {
        LOG("Error saving MCX values to GPO, record (%s) of node (%s) auth is (%s). Got actual error %d, going to return eDSNotAuthorized",
            pRecord->pszName, pDirNode->pszDirNodePath, pDirNode->pszDirNodeUserUPN ? pDirNode->pszDirNodeUserUPN : "Unknown", macError);
        macError = eDSNotAuthorized;
    }
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* Mark this pRecord not dirty since we have saved off changes to AD */
    pRecord->fDirty = FALSE;

cleanup:

    FreeMCXValueList(pMCXValueList);
    
    if (pszAttribute)
    {
        LW_SAFE_FREE_STRING(pszAttribute);
    }
    
    if (pDataNode)
    {
        dsDataNodeDeAllocate(0, pDataNode);
    }
    
    if (pValue)
    {
        LwFreeMemory(pValue);
    }
		
    LOG_LEAVE("--> %d", macError);

    return macError;
}

long
LWIRecordQuery::SetAttributeValue(sSetAttributeValue* pSetAttributeValue)
{
    long  macError = eDSNoErr;
    char* pszAttribute = NULL;
    char* pValue = NULL;
    long length = 0;
    PDSRECORD pRecord = NULL;
    RecordRefMapIter iter;
    PDSDIRNODE pDirNode = NULL;
    PDSATTRIBUTE pAttribute = NULL;
    PMCXVALUE pMCXValueList = NULL;
    DWORD dwPolicyType = UNKNOWN_GROUP_POLICY;

    macError = GetDataNodeString(pSetAttributeValue->fInAttrType, &pszAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    macError = GetDataNodeValue(&pSetAttributeValue->fInAttrValueEntry->fAttributeValueData, &pValue, &length);
    GOTO_CLEANUP_ON_MACERROR(macError);
	
    LOG_ENTER("fType = %d, fResult = %d, fInRecRef = %d, Attribute = %s",
              pSetAttributeValue->fType,
              pSetAttributeValue->fResult,
              pSetAttributeValue->fInRecRef,
              pszAttribute);
  
    iter = LWIRecordQuery::_recordRefMap->find(pSetAttributeValue->fInRecRef);
    if (iter != LWIRecordQuery::_recordRefMap->end())
    {
        pRecord = iter->second;
    }

    if (!pRecord)
    {
        LOG_ERROR("Invalid record reference: %d", pSetAttributeValue->fInRecRef);
        macError = eDSInvalidRecordRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (!strcmp(pRecord->pszType, kDSStdRecordTypeComputerGroups) ||
        !strcmp(pRecord->pszType, kDSStdRecordTypeComputerLists))
    {
       dwPolicyType = MACHINE_GROUP_POLICY;
    }
    else if (!strcmp(pRecord->pszType, kDSStdRecordTypeGroups))
    {
        dwPolicyType = USER_GROUP_POLICY;
    }
    else
    {
        macError = eDSInvalidRecordType;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
	
    LOG("Replacing attribute (%s) settings on record (%s)", pszAttribute, pRecord->pszName);

    LWIQuery::RemoveAttributeAndValues(pszAttribute, pRecord);
    
    macError = LWIQuery::AddAttribute(pszAttribute, pRecord, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::SetAttributeValue(pAttribute, pValue, length);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIDirNodeQuery::GetDsDirNodeRef(pRecord->dirNodeRef, &pDirNode);
    GOTO_CLEANUP_ON_MACERROR(macError);

    LOG("Flushing attributes that are changed for record (%s) of node (%s) auth is (%s)",
        pRecord->pszName, pDirNode->pszDirNodePath, pDirNode->pszDirNodeUserUPN ? pDirNode->pszDirNodeUserUPN : "Unknown");

    pAttribute = pRecord->pAttributeListHead;

    while (pAttribute)
    {
        if (!strcmp(pAttribute->pszName, kDS1AttrMCXSettings))
        {
            macError = ConvertDSAttributeValuesToMCXValues(pAttribute->pValueListHead, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        pAttribute = pAttribute->pNext;
    }

    macError = SaveMCXValuesForGPOSettingType(pMCXValueList, pDirNode->pDirNodeGPO, dwPolicyType, pDirNode->pszDirNodeUserUPN);
    if (macError)
    {
        LOG("Error saving MCX values to GPO, record (%s) of node (%s) auth is (%s). Got actual error %d, going to return eDSNotAuthorized",
            pRecord->pszName, pDirNode->pszDirNodePath, pDirNode->pszDirNodeUserUPN ? pDirNode->pszDirNodeUserUPN : "Unknown", macError);
        macError = eDSNotAuthorized;
    }
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* Mark this pRecord not dirty since we have saved off changes to AD */
    pRecord->fDirty = FALSE;

cleanup:

    FreeMCXValueList(pMCXValueList);

    if (pszAttribute)
    {
        LwFreeMemory(pszAttribute);
    }
    
    if (pValue)
    {
        LwFreeMemory(pValue);
    }
		
    LOG_LEAVE("--> %d", macError);

    return macError;
}

long 
LWIRecordQuery::RemoveAttribute(sRemoveAttribute* pRemoveAttribute)
{
    long  macError = eDSNoErr;
    char* pszAttribute = NULL;
    PDSRECORD pRecord = NULL;
    RecordRefMapIter iter;

    macError = GetDataNodeString(pRemoveAttribute->fInAttribute, &pszAttribute);
	GOTO_CLEANUP_ON_MACERROR(macError);
	
    LOG_ENTER("fType = %d, fResult = %d, fInRecRef = %d, Attribute = %s",
              pRemoveAttribute->fType,
              pRemoveAttribute->fResult,
              pRemoveAttribute->fInRecRef,
			  pszAttribute);
			  
    iter = LWIRecordQuery::_recordRefMap->find(pRemoveAttribute->fInRecRef);
    if (iter != LWIRecordQuery::_recordRefMap->end())
    {
        pRecord = iter->second;
    }

    if (!pRecord)
    {
        LOG_ERROR("Invalid record reference: %d", pRemoveAttribute->fInRecRef);
        macError = eDSInvalidRecordRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
	
    LOG("Removing attribute (%s) settings on record (%s)", pszAttribute, pRecord->pszName);

    LWIQuery::RemoveAttributeAndValues(pszAttribute, pRecord);

    /* Mark this pRecord dirty so that we know to save off changes to AD */
    pRecord->fDirty = TRUE;

cleanup:

    if (pszAttribute)
    {
        LwFreeMemory(pszAttribute);
    }
		
    LOG_LEAVE("--> %d", macError);

    return macError;
}

long 
LWIRecordQuery::FlushRecord(sFlushRecord* pFlushRecord)
{
    long macError = eDSNoErr;
    PDSRECORD pRecord = NULL;
    RecordRefMapIter iter;
    PDSDIRNODE pDirNode = NULL;
    PDSATTRIBUTE pAttribute = NULL;
    PMCXVALUE pMCXValueList = NULL;
    DWORD dwPolicyType = UNKNOWN_GROUP_POLICY;

    LOG_ENTER("fType = %d, fResult = %d, fInRecRef = %d",
              pFlushRecord->fType,
              pFlushRecord->fResult,
              pFlushRecord->fInRecRef);
  
    iter = LWIRecordQuery::_recordRefMap->find(pFlushRecord->fInRecRef);
    if (iter != LWIRecordQuery::_recordRefMap->end())
    {
        pRecord = iter->second;
    }

    if (!pRecord)
    {
        LOG_ERROR("Invalid record reference: %d", pFlushRecord->fInRecRef);
        macError = eDSInvalidRecordRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (!strcmp(pRecord->pszType, kDSStdRecordTypeComputerGroups) ||
        !strcmp(pRecord->pszType, kDSStdRecordTypeComputerLists))
    {
       dwPolicyType = MACHINE_GROUP_POLICY;
    }
    else if (!strcmp(pRecord->pszType, kDSStdRecordTypeGroups))
    {
        dwPolicyType = USER_GROUP_POLICY;
    }
    else
    {
        macError = eDSInvalidRecordType;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = LWIDirNodeQuery::GetDsDirNodeRef(pRecord->dirNodeRef, &pDirNode);
    GOTO_CLEANUP_ON_MACERROR(macError);

    LOG("Flushing attributes that are changed for record (%s) of node (%s) auth is (%s)",
        pRecord->pszName, pDirNode->pszDirNodePath, pDirNode->pszDirNodeUserUPN ? pDirNode->pszDirNodeUserUPN : "Unknown");

    pAttribute = pRecord->pAttributeListHead;

    while (pAttribute)
    {
        if (!strcmp(pAttribute->pszName, kDS1AttrMCXSettings))
        {
            macError = ConvertDSAttributeValuesToMCXValues(pAttribute->pValueListHead, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        pAttribute = pAttribute->pNext;
    }

    macError = SaveMCXValuesForGPOSettingType(pMCXValueList, pDirNode->pDirNodeGPO, dwPolicyType, pDirNode->pszDirNodeUserUPN);
    if (macError)
    {
        LOG("Error saving MCX values to GPO, record (%s) of node (%s) auth is (%s). Got actual error %d, going to return eDSNotAuthorized",
            pRecord->pszName, pDirNode->pszDirNodePath, pDirNode->pszDirNodeUserUPN ? pDirNode->pszDirNodeUserUPN : "Unknown", macError);
        macError = eDSNotAuthorized;
    }
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    FreeMCXValueList(pMCXValueList);

    LOG_LEAVE("--> %d", macError);

    return macError;
}

