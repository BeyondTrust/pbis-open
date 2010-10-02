#include "includes.h"
#include "LWIDirNodeQuery.h"

#define LIST_SEPARATOR ", "
#define LIST_SEPARATOR_LENGTH (sizeof(LIST_SEPARATOR)-1)

// Ideally, Apple would have definitions for these:
#define DNI_TYPE "dsAttrTypeStandard:DirectoryNodeInfo"
#define DNI_NAME "DirectoryNodeInfo"

#ifndef kDSStdRecordTypeComputerGroups /* For when building on Tiger based build machines */
/*!
 * @defined kDSStdRecordTypeComputerGroups
 * @discussion Identifies computer group records.
 */
#define		kDSStdRecordTypeComputerGroups			"dsRecTypeStandard:ComputerGroups"
#endif

std::map<long, long> * LWIDirNodeQuery::_attributeRefMap = NULL;

static bool IsPluginRootPathPrefix(const char* path)
{
    return ( path &&
             (0 == strncmp(PLUGIN_ROOT_PATH, path, sizeof(PLUGIN_ROOT_PATH)-1)) &&
             ( (0 == path[sizeof(PLUGIN_ROOT_PATH)-1]) ||
               ('/' == path[sizeof(PLUGIN_ROOT_PATH)-1]) ) );
}

long
LWIDirNodeQuery::Initialize()
{
    _attributeRefMap = new std::map<long, long>();
    return _attributeRefMap ? eDSNoErr : eDSAllocationFailed;
}

void
LWIDirNodeQuery::Cleanup()
{
    if (_attributeRefMap)
    {
        delete _attributeRefMap;
        _attributeRefMap = NULL;
    }
}

long
LWIDirNodeQuery::Open(sOpenDirNode * pOpenDirNode)
{
    long macError = eDSNoErr;
    char* dirNodeName = NULL;
    unsigned long count = dsDataListGetNodeCount( pOpenDirNode->fInDirNodeName );

    LOG_ENTER("fType = %d, fResult = %d, fInDirRef = %d, fInDirNodeName.count = [%d]",
              pOpenDirNode->fType,
              pOpenDirNode->fResult,
              pOpenDirNode->fInDirRef,
              count);

    dirNodeName = dsGetPathFromList(0, pOpenDirNode->fInDirNodeName, "/");

    LOG_PARAM("fInDirNodeName = \"%s\"", SAFE_LOG_STR(dirNodeName));

    // Check to see if the path contains our registered top level path
    if ( !IsPluginRootPathPrefix(dirNodeName) )
    {
        macError = eDSOpenNodeFailed;
        GOTO_CLEANUP();
    }

    // We do not need to do anything special here since we do not return any data.
    // If we were to support different nodes, we would need to associate the
    // identity of the node with the node reference.

cleanup:
    if (dirNodeName)
    {
        free(dirNodeName);
    }

    LOG_LEAVE("--> %d", macError);

    return macError;
}

long
LWIDirNodeQuery::Close(sCloseDirNode * pCloseDirNode)
{
    long macError = eDSNoErr;

    LOG_ENTER("fType = %d, fResult = %d, fInDirRef = %d",
              pCloseDirNode->fType,
              pCloseDirNode->fResult,
              pCloseDirNode->fInNodeRef);

    // Nothing to do here since we do not have any data associated with the node reference.

    LOG_LEAVE("--> %d", macError);

    return macError;
}

long
LWIDirNodeQuery::GetInfo(sGetDirNodeInfo * pGetDirNodeInfo)
{
    long macError = eDSNoErr;
    unsigned long bytesWritten = 0;
    unsigned long nRecordsWritten = 0;
    unsigned long TotalRecords = 0;
    unsigned long nAttributesWritten = 0;
    unsigned long attributeCount;
    char* attributesBuffer = NULL;
    char* attributes = NULL;
    LWIQuery* pQuery = NULL;
    PDSRECORD pRecord = NULL;
    bool bSetValue;
    PLWIBITVECTOR attributeSet = NULL;
    PDSATTRIBUTE pAttribute = NULL;
    tDataListPtr nodeNameList = NULL;
    tDataNodePtr pNameNode = NULL;

    LOG_ENTER("fType = %d, fResult = %d, fInNodeRef = %d, fOutAttrListRef = %d, fInAttrInfoOnly = %s",
              pGetDirNodeInfo->fType,
              pGetDirNodeInfo->fResult,
              pGetDirNodeInfo->fInNodeRef,
              pGetDirNodeInfo->fOutAttrListRef,
              BOOL_STRING(pGetDirNodeInfo->fInAttrInfoOnly));

    attributeCount = dsDataListGetNodeCount(pGetDirNodeInfo->fInDirNodeInfoTypeList);

    attributesBuffer = dsGetPathFromList(0, pGetDirNodeInfo->fInDirNodeInfoTypeList, LIST_SEPARATOR);
    attributes = attributesBuffer ? (attributesBuffer + LIST_SEPARATOR_LENGTH) : NULL;

    LOG_PARAM("fInDirNodeInfoTypeList => { count = %d, items = \"%s\"}",
              attributeCount, SAFE_LOG_STR(attributes));

    pQuery = new LWIQuery(!pGetDirNodeInfo->fInAttrInfoOnly,
                           false /* Rely on our error eDSBufferTooSmall to cause the caller to retry */);
    if (!pQuery)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = LWIAttrLookup::GetVector(pGetDirNodeInfo->fInDirNodeInfoTypeList, &attributeSet);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::BuildRecord(DNI_TYPE, DNI_NAME, &pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    bSetValue = !pGetDirNodeInfo->fInAttrInfoOnly;

    if (LWI_BITVECTOR_ISSET(attributeSet, LWIAttrLookup::idx_kDSAttributesAll))
    {
        LWI_BITVECTOR_SET(attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll);
    }

    if (LWI_BITVECTOR_ISSET(attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll))
    {
        LWI_BITVECTOR_SET(attributeSet, LWIAttrLookup::idx_kDSNAttrAuthMethod);
        LWI_BITVECTOR_SET(attributeSet, LWIAttrLookup::idx_kDS1AttrReadOnlyNode);
        LWI_BITVECTOR_SET(attributeSet, LWIAttrLookup::idx_kDSNAttrNodePath);
        LWI_BITVECTOR_SET(attributeSet, LWIAttrLookup::idx_kDSNAttrRecordType);
        LWI_BITVECTOR_SET(attributeSet, LWIAttrLookup::idx_K_DS_ATTR_TRUST_INFORMATION);
        // kDSNAttrSubNodes --> N/A
        // kDS1AttrDataStamp --> N/A
    }

    for (int iAttr = LWIAttrLookup::idx_unknown+1; iAttr < LWIAttrLookup::idx_sentinel; iAttr++)
    {
        if (LWI_BITVECTOR_ISSET(attributeSet, iAttr))
        {
            switch (iAttr)
            {
            case LWIAttrLookup::idx_kDSNAttrAuthMethod:
            {
                macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDSNAttrAuthMethod, NULL);
                GOTO_CLEANUP_ON_MACERROR(macError);

                if (bSetValue)
                {
                    // These are the normal username + password authentication methods:
                    macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdAuthClearText);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdAuthNodeNativeClearTextOK);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdAuthNodeNativeNoClearText);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdAuthCrypt);
                    GOTO_CLEANUP_ON_MACERROR(macError);

                    // We also support change password:
                    macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdAuthChangePasswd);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                }

                nAttributesWritten++;
                break;
            }
            case LWIAttrLookup::idx_kDS1AttrReadOnlyNode:
            {
                macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDS1AttrReadOnlyNode, NULL);
                GOTO_CLEANUP_ON_MACERROR(macError);

                if (bSetValue)
                {
                    macError = LWIQuery::SetAttributeValue(pAttribute, "ReadOnly");
                    GOTO_CLEANUP_ON_MACERROR(macError);
                }

                nAttributesWritten++;
                break;
            }
            case LWIAttrLookup::idx_kDSNAttrNodePath:
            {
                int nNodes;

                macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDSNAttrNodePath, NULL);
                GOTO_CLEANUP_ON_MACERROR(macError);

                if (bSetValue)
                {
                    nodeNameList = dsDataListAllocate(0);
                    if ( !nodeNameList )
                    {
                        macError = eDSAllocationFailed;
                        GOTO_CLEANUP_ON_MACERROR(macError);
                    }

                    macError = dsBuildListFromPathAlloc(0, nodeNameList, PLUGIN_ROOT_PATH, "/");
                    GOTO_CLEANUP_ON_MACERROR(macError);

                    nNodes = dsDataListGetNodeCount(nodeNameList);
                    for (int iNode = 0; iNode < nNodes; iNode++)
                    {
                        macError = dsDataListGetNodeAlloc(0,
                                                          nodeNameList,
                                                          iNode+1,
                                                          &pNameNode);
                        GOTO_CLEANUP_ON_MACERROR(macError);

                        macError = LWIQuery::SetAttributeValue(pAttribute, pNameNode->fBufferData);
                        GOTO_CLEANUP_ON_MACERROR(macError);

                        dsDataNodeDeAllocate(NULL, pNameNode);
                        pNameNode = NULL;
                    }
                }

                nAttributesWritten++;
                break;
            }
            case LWIAttrLookup::idx_kDSNAttrRecordType:
            {
                macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDSNAttrRecordType, NULL);
                GOTO_CLEANUP_ON_MACERROR(macError);

                if (bSetValue)
                {
                    macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdRecordTypeUsers);
                    GOTO_CLEANUP_ON_MACERROR(macError);

                    macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdRecordTypeGroups);
                    GOTO_CLEANUP_ON_MACERROR(macError);
#ifdef ENABLE_WORKGROUP_MANAGER
                    macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdRecordTypeComputerLists);
                    GOTO_CLEANUP_ON_MACERROR(macError);

                    macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdRecordTypeComputerGroups);
                    GOTO_CLEANUP_ON_MACERROR(macError);

                    macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdRecordTypeComputers);
                    GOTO_CLEANUP_ON_MACERROR(macError);
#endif /* ENABLE_WORKGROUP_MANAGER */
                }

                nAttributesWritten++;
                break;
            }
            case LWIAttrLookup::idx_K_DS_ATTR_TRUST_INFORMATION:
            {
                macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, K_DS_ATTR_TRUST_INFORMATION, NULL);
                GOTO_CLEANUP_ON_MACERROR(macError);

                if (bSetValue)
                {
                    // K_DS_ATTR_TRUST_INFORMATION --> Is this correct?
                    macError = LWIQuery::SetAttributeValue(pAttribute, "FullTrust");
                    GOTO_CLEANUP_ON_MACERROR(macError);
                }

                nAttributesWritten++;
                break;

            }
            case LWIAttrLookup::idx_kDSAttributesAll:
            case LWIAttrLookup::idx_kDSAttributesStandardAll:
                // We assume that the other bits are already set.
                break;

            default:
                LOG("Unsupported attribute index - %d", iAttr);
                break;
            }
        }
    }

    macError = pQuery->CommitRecord(pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecord = NULL;

    macError = pQuery->WriteGDNIResponse(pGetDirNodeInfo->fOutDataBuff->fBufferData,
                                         pGetDirNodeInfo->fOutDataBuff->fBufferSize,
                                         bytesWritten,
                                         nRecordsWritten,
                                         TotalRecords);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pGetDirNodeInfo->fOutDataBuff->fBufferLength = bytesWritten;
    pGetDirNodeInfo->fOutAttrInfoCount = nAttributesWritten;

    if ( bytesWritten > 0 )
    {
#ifdef SHOW_ALL_DEBUG_SPEW
        LOG_BUFFER(pGetDirNodeInfo->fOutDataBuff->fBufferData, bytesWritten);
#endif
    }

cleanup:
    if (pNameNode)
    {
        dsDataNodeDeAllocate(0, pNameNode);
    }

    if (nodeNameList)
    {
        dsDataListDeallocate(0, nodeNameList);
        free(nodeNameList);
    }

    if (attributesBuffer)
    {
        free(attributesBuffer);
    }

    if (pRecord)
    {
        LWIQuery::FreeRecord(pRecord);
    }

    if (pQuery)
    {
        delete pQuery;
    }

    LOG_LEAVE("fOutAttrInfoCount = %d, fOutDataBuff => { length = %d, size = %d } --> %d",
              pGetDirNodeInfo->fOutAttrInfoCount,
              pGetDirNodeInfo->fOutDataBuff->fBufferLength,
              pGetDirNodeInfo->fOutDataBuff->fBufferSize,
              macError);

    return macError;
}

long
LWIDirNodeQuery::FindAttribute(
    PDSMESSAGE pMessage,
    unsigned long attrIndex,
    PDSATTRIBUTE * ppAttribute
    )
{
    long macError = eDSNoErr;
    PDSATTRIBUTE currentElement = NULL;
    unsigned long currentIndex = 0;

    if (pMessage && pMessage->pHeader && pMessage->pHeader->nRecords > 0)
    {
        PDSRECORD pRecord = pMessage->pRecordList;

        // we always just use the first record

        currentElement = pRecord->pAttributeListHead;
        currentIndex = 1;
        while (currentElement && (currentIndex < attrIndex))
        {
            currentElement = currentElement->pNext;
            currentIndex++;
        }
    }

    *ppAttribute = currentElement;

    macError = currentElement ? eDSNoErr : eDSAttributeNotFound;

    return macError;
}

long
LWIDirNodeQuery::GetAttributeInfo(
    PDSMESSAGE pMessage,
    unsigned long attrIndex,
    tAttributeEntryPtr* ppAttributeEntryPtr
    )
{
    long macError = eDSNoErr;
    tAttributeEntryPtr pAttributeEntryPtr = NULL;
    PDSATTRIBUTE pAttribute = NULL;

    macError = FindAttribute(pMessage, attrIndex, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::CreateAttributeEntry(&pAttributeEntryPtr, pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:
    if (macError)
    {
       LWIFreeMemory(pAttributeEntryPtr);
       pAttributeEntryPtr = NULL;
    }

    *ppAttributeEntryPtr = pAttributeEntryPtr;

    return macError;
}

long
LWIDirNodeQuery::GetAttributeEntry(sGetAttributeEntry * pGetAttributeEntry)
{
    long macError = eDSNoErr;
    PDSMESSAGE pMessage = NULL;
    tAttributeEntryPtr pAttrInfoPtr = NULL;

    LOG_ENTER("fType = %d, fResult = %d, fInNodeRef = %d, fOutAttrValueListRef = %d, fInAttrInfoIndex = %d, fOutAttrInfoPtr = @%p",
              pGetAttributeEntry->fType,
              pGetAttributeEntry->fResult,
              pGetAttributeEntry->fInNodeRef,
              pGetAttributeEntry->fOutAttrValueListRef,
              pGetAttributeEntry->fInAttrInfoIndex,
              pGetAttributeEntry->fOutAttrInfoPtr);

    if (!pGetAttributeEntry->fInOutDataBuff)
    {
        macError = eDSNullDataBuff;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

#ifdef SHOW_ALL_DEBUG_SPEW
    LOG_BUFFER(pGetAttributeEntry->fInOutDataBuff->fBufferData, pGetAttributeEntry->fInOutDataBuff->fBufferLength);
#endif

    macError = LWIQuery::ReadResponse(pGetAttributeEntry->fInOutDataBuff->fBufferData,
                                      pGetAttributeEntry->fInOutDataBuff->fBufferLength,
                                      &pMessage);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (!LWIDirNodeQuery::_attributeRefMap)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = GetAttributeInfo(pMessage, pGetAttributeEntry->fInAttrInfoIndex, &pAttrInfoPtr);
    GOTO_CLEANUP_ON_MACERROR(macError);

    LWIDirNodeQuery::_attributeRefMap->insert(std::make_pair(pGetAttributeEntry->fOutAttrValueListRef,
                                                                        pGetAttributeEntry->fInAttrInfoIndex));

    pGetAttributeEntry->fOutAttrInfoPtr = pAttrInfoPtr;
    pAttrInfoPtr = NULL;

    LOG("fOutAttrInfoPtr = @%p => { fAttributeValueCount = %d, fAttributeDataSize = %d, fAttributeValueMaxSize = %d, fAttributeSignature.fBufferSize = %d }",
        pGetAttributeEntry->fOutAttrInfoPtr,
        pGetAttributeEntry->fOutAttrInfoPtr->fAttributeValueCount,
        pGetAttributeEntry->fOutAttrInfoPtr->fAttributeDataSize,
        pGetAttributeEntry->fOutAttrInfoPtr->fAttributeValueMaxSize,
        pGetAttributeEntry->fOutAttrInfoPtr->fAttributeSignature.fBufferSize);

#ifdef SHOW_ALL_DEBUG_SPEW
    LOG_BUFFER(pGetAttributeEntry->fOutAttrInfoPtr->fAttributeSignature.fBufferData,
               pGetAttributeEntry->fOutAttrInfoPtr->fAttributeSignature.fBufferLength);
#endif

cleanup:
    if (pMessage)
    {
       LWIQuery::FreeMessage(pMessage);
    }

    if (pAttrInfoPtr)
    {
       LWIFreeMemory(pAttrInfoPtr);
    }

    LOG_LEAVE("fOutAttrValueListRef = %d, fOutAttrInfoPtr = @%p --> %d",
              pGetAttributeEntry->fOutAttrValueListRef,
              pGetAttributeEntry->fOutAttrInfoPtr,
              macError);

    return macError;
}

long
LWIDirNodeQuery::GetAttributeValue(sGetAttributeValue * pGetAttributeValue)
{
    long macError = eDSNoErr;
    PDSMESSAGE pMessage = NULL;
    unsigned long attrIndex = 0;
    tAttributeValueEntryPtr pAttributeValueEntry = NULL;
    PDSATTRIBUTE pAttribute = NULL;
    PDSATTRIBUTEVALUE pAttributeValue = NULL;
    std::map<long,long>::iterator iter;

    LOG_ENTER("fType = %d, fResult = %d, fInNodeRef = %d, fInOutDataBuff = @%p, fInAttrValueIndex = %d, fInAttrValueListRef = %d",
              pGetAttributeValue->fType,
              pGetAttributeValue->fResult,
              pGetAttributeValue->fInNodeRef,
              pGetAttributeValue->fInOutDataBuff,
              pGetAttributeValue->fInAttrValueIndex,
              pGetAttributeValue->fInAttrValueListRef);

    if (!pGetAttributeValue->fInOutDataBuff)
    {
        macError = eDSNullDataBuff;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

#ifdef SHOW_ALL_DEBUG_SPEW
    LOG_BUFFER(pGetAttributeValue->fInOutDataBuff->fBufferData, pGetAttributeValue->fInOutDataBuff->fBufferLength);
#endif

    macError = LWIQuery::ReadResponse(pGetAttributeValue->fInOutDataBuff->fBufferData,
                                      pGetAttributeValue->fInOutDataBuff->fBufferLength,
                                      &pMessage);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (!LWIDirNodeQuery::_attributeRefMap)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    iter = LWIDirNodeQuery::_attributeRefMap->find(pGetAttributeValue->fInAttrValueListRef);
    if (iter != LWIDirNodeQuery::_attributeRefMap->end())
    {
        attrIndex = iter->second;
    }
    else
    {
        LOG_ERROR("Invalid attribute value list reference: %d", pGetAttributeValue->fInAttrValueListRef);
        macError = eDSInvalidAttrListRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    // We found the index of the attribute in which the current value is to be found
    macError = FindAttribute(pMessage, attrIndex, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::FindAttributeValueByIndex(pAttribute, pGetAttributeValue->fInAttrValueIndex, &pAttributeValue);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::CreateAttributeValueEntry(&pAttributeValueEntry, pAttributeValue);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pGetAttributeValue->fOutAttrValue = pAttributeValueEntry;
    pAttributeValueEntry = NULL;

    LOG("fOutAttrValue = @%p => { fAttributeValueID = 0x%08x, fAttributeValueData.fBufferSize = %d }",
        pGetAttributeValue->fOutAttrValue,
        pGetAttributeValue->fOutAttrValue->fAttributeValueID,
        pGetAttributeValue->fOutAttrValue->fAttributeValueData.fBufferSize);

#ifdef SHOW_ALL_DEBUG_SPEW
    LOG_BUFFER(pGetAttributeValue->fOutAttrValue->fAttributeValueData.fBufferData,
               pGetAttributeValue->fOutAttrValue->fAttributeValueData.fBufferLength);
#endif

cleanup:
    if (pMessage)
    {
       LWIQuery::FreeMessage(pMessage);
    }

    if (pAttributeValueEntry)
    {
        LWIFreeMemory(pAttributeValueEntry);
    }

    LOG_LEAVE("fOutAttrValue = @%p --> %d",
              pGetAttributeValue->fOutAttrValue,
              macError);

    return macError;
}

long
LWIDirNodeQuery::CloseValueList(sCloseAttributeValueList * pCloseAttributeValueList)
{
    long macError = eDSNoErr;

    LOG_ENTER("fType = %d, fResult = %d, fInAttributeValueListRef = %d",
              pCloseAttributeValueList->fType,
              pCloseAttributeValueList->fResult,
              pCloseAttributeValueList->fInAttributeValueListRef
              );


    LOG_LEAVE("fInAttributeValueListRef = %d --> %d",
              pCloseAttributeValueList->fInAttributeValueListRef,
              macError);

    return macError;
}

long
LWIDirNodeQuery::CloseAttributeList(sCloseAttributeList * pCloseAttributeList)
{
    long macError = eDSNoErr;

    LOG_ENTER("fType = %d, fResult = %d, fInAttributeListRef = %d",
              pCloseAttributeList->fType,
              pCloseAttributeList->fResult,
              pCloseAttributeList->fInAttributeListRef
              );

    if (LWIDirNodeQuery::_attributeRefMap == NULL)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    LWIDirNodeQuery::_attributeRefMap->erase(pCloseAttributeList->fInAttributeListRef);

cleanup:

    LOG_LEAVE("fInAttributeListRef = %d --> %d",
              pCloseAttributeList->fInAttributeListRef,
              macError);

    return macError;
}

