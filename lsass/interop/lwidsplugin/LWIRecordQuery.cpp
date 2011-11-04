#include "LWIRecordQuery.h"
#include "LWIQuery.h"
#include "LWICRC.h"
#include <assert.h>

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
       macError = LWIAllocateMemory(pDataNode->fBufferLength+1, (PVOID*)&pszString);
       GOTO_CLEANUP_ON_MACERROR(macError);

       strncpy(pszString, pDataNode->fBufferData, pDataNode->fBufferLength);
    }

    *ppszString = pszString;
    pszString = NULL;

cleanup:

    if (pszString)
    {
       LWIFreeMemory(pszString);
    }

    return macError;
}

long
LWIRecordQuery::Open(sOpenRecord* pOpenRecord)
{
    long macError = eDSNoErr;
    LWIQuery* pQuery = NULL;
    PDSRECORD pRecord = NULL;
    char* pszRecType = NULL;
    char* pszRecName = NULL;

    LOG_ENTER("fType = %d, fResult = %d, fInNodeRef = %d, fInRecType = %.*s, "
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
                                    false, // Rely on our error eDSBufferTooSmall to cause the caller to retry,
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
                                    false, // Rely on our error eDSBufferTooSmall to cause the caller to retry,
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
       LWIFreeMemory(pszRecName);
    }

    if (pszRecType)
    {
       LWIFreeMemory(pszRecType);
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
    unsigned long offset = 0;
    unsigned long size = 0;
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
    macError = LWIAllocateMemory(sizeof(*pRecordEntry) + 2 + pRecord->nameLen + 2 + pRecord->typeLen, (PVOID*)&pRecordEntry);
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

    assert(size >= offset);

    pGetRecRefInfo->fOutRecInfo = pRecordEntry;
    pRecordEntry = NULL;

cleanup:
    if (pRecordEntry)
    {
        LWIFreeMemory(pRecordEntry);
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
        LWIFreeMemory(pAttributeEntry);
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
        LWIFreeMemory(pAttributeValueEntry);
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
        LWIFreeMemory(pAttributeValueEntry);
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
	char* pszValue = NULL;
    PDSRECORD pRecord = NULL;
    RecordRefMapIter iter;

/*
typedef struct {
    unsigned long fType;
    long fResult;
    tRecordReference fInRecRef;
    tDataNodePtr fInNewAttr;
    tAccessControlEntryPtr fInNewAttrAccess;
    tDataNodePtr fInFirstAttrValue;
} sAddAttribute;
*/
    macError = GetDataNodeString(pAddAttribute->fInNewAttr, &pszAttribute);
	GOTO_CLEANUP_ON_MACERROR(macError);

    macError = GetDataNodeString(pAddAttribute->fInFirstAttrValue, &pszValue);
	GOTO_CLEANUP_ON_MACERROR(macError);
	
    LOG_ENTER("fType = %d, fResult = %d, fInRecRef = %d, Attribute = %s, Value = %s",
              pAddAttribute->fType,
              pAddAttribute->fResult,
              pAddAttribute->fInRecRef,
			  pszAttribute,
			  pszValue);

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
			  
	LOG("This function needs to be implemented to save off new attribute settings on object (%s)", pRecord->pszName);

	GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    if (pszAttribute)
	{
	    LWIFreeMemory(pszAttribute);
	}
	
	if (pszValue)
	{
	    LWIFreeMemory(pszValue);
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

/*
typedef struct {
    unsigned long fType;
    long fResult;
    tRecordReference fInRecRef;
    tDataNodePtr fInAttrType;
    tDataNodePtr fInAttrValue;
} sAddAttributeValue;
*/
    macError = GetDataNodeString(pAddAttributeValue->fInAttrType, &pszAttribute);
	GOTO_CLEANUP_ON_MACERROR(macError);

    macError = GetDataNodeString(pAddAttributeValue->fInAttrValue, &pszValue);
	GOTO_CLEANUP_ON_MACERROR(macError);
	
    LOG_ENTER("fType = %d, fResult = %d, fInRecRef = %d, Attribute = %s, Value = %s",
              pAddAttributeValue->fType,
              pAddAttributeValue->fResult,
              pAddAttributeValue->fInRecRef,
			  pszAttribute,
			  pszValue);

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
	
	LOG("This function needs to be implemented to save off new attribute value settings on object (%s)", pRecord->pszName);

	GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    if (pszAttribute)
	{
	    LWIFreeMemory(pszAttribute);
	}
	
	if (pszValue)
	{
	    LWIFreeMemory(pszValue);
	}
	
    LOG_LEAVE("--> %d", macError);

    return macError;
}

long 
LWIRecordQuery::RemoveAttribute(sRemoveAttribute* pRemoveAttribute)
{
    long  macError = eDSNoErr;
	char* pszAttribute = NULL;

/*
typedef struct {
    unsigned long fType;
    long fResult;
    tRecordReference fInRecRef;
    tDataNodePtr fInAttribute;
} sRemoveAttribute;
*/
    macError = GetDataNodeString(pRemoveAttribute->fInAttribute, &pszAttribute);
	GOTO_CLEANUP_ON_MACERROR(macError);
	
    LOG_ENTER("fType = %d, fResult = %d, fInRecRef = %d, Attribute = %s",
              pRemoveAttribute->fType,
              pRemoveAttribute->fResult,
              pRemoveAttribute->fInRecRef,
			  pszAttribute);
			  
	LOG("This function needs to be implemented to remove attribute settings from object");

    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    if (pszAttribute)
	{
	    LWIFreeMemory(pszAttribute);
	}
	
    LOG_LEAVE("--> %d", macError);

    return macError;
}

long 
LWIRecordQuery::FlushRecord(sFlushRecord* pFlushRecord)
{
    long macError = eDSNoErr;

/*
typedef struct {
    unsigned long fType;
    long fResult;
    tRecordReference fInRecRef;
} sFlushRecord;
*/
    LOG_ENTER("fType = %d, fResult = %d, fInRecRef = %d",
              pFlushRecord->fType,
              pFlushRecord->fResult,
              pFlushRecord->fInRecRef);
			  
	LOG("This function needs to be implemented to flush record data (save) for an object");

    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    LOG_LEAVE("--> %d", macError);

    return macError;
}

