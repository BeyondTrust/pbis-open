/*
 *  LWIAttrValDataQuery.cpp
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/24/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

// dsDoAttributeValueSearchWithData
// Searches for records by attribute type and attribute value.
//
// tDirStatus dsDoAttributeValueSearchWithData(
// tDirNodeReference inDirNodeReference,
// tDataBufferPtr inOutDataBuffer,
// tDataListPtr inRecordTypeList,
// tDataNodePtr inAttributeMatchType,
// tDirPatternMatch inPatternMatchType,
// tDataNodePtr inPatternToMatch,
// tDataListPtr inAttributeTypeRequestList,
// dsBool inAttributeInfoOnly,
// unsigned long *inOutMatchRecordCount,
// tContextData *inOutContinueData);
//
// Parameters
//
// inDirNodeReference
//
// On input, a value of type tDirNodeReference, obtained by calling dsOpenDirNode,
// that identifies the node that is to be searched.
//
// inOutDataBuffer
//
// On input, a value of type tDataBufferPtr created by calling dsDataBufferAllocate
// that points to the tDataBuffer structure in which this function is to place
// search results. On output, if inOutMatchRecordCount points to a value greater
// than zero, call dsGetRecordEntry, dsGetAttributeEntry, and dsGetAttributeValue
// to get the records, attributes, and attribute values from the data buffer.
//
// inRecordTypeList
//
// On input, a value of type tDataListPtr pointing to a tDataList structure allocated
// by calling dsDataListAllocate that contains a list of the record types to search
// for. Set the record type to kDSStdRecordTypeAll to search all records. For other
// possible values, see Standard Record Types.
//
// inAttributeMatchType
// On input, a value of type tDataNodePtr that points to a tDataNode structure
// allocated by calling dsDataNodeAllocateBlock or dsDataNodeAllocateString that
// contains an attribute type to search for. To search all attribute types, set the
// attribute type to kDSAttributesAll. For other possible values, see the attribute
// constants described in the ÒConstantsÓ section for other possible values.
//
// inPatternMatchType
//
// On input, a value of type tDirPatternMatch specifying a pattern type that controls
// the way in which the pattern specified by inPattern2Match is compared with attribute
// values. See Pattern Matching Constants for possible values. The pattern type may
// also be defined by the Open Directory plug-in that handles the directory service
// represented by inDirNodeReference.
//
// inPatternToMatch
//
// On input, a value of type tDataNodePtr that points to a tDataNode structure allocated
// by calling dsDataNodeAllocateBlock or dsDataNodeAllocateString that contains the
// pattern to match.
//
// inAttributeTypeRequestList
//
// On input, a value of type tDataListPtr pointing to a tDataList structure allocated by
// calling dsDataListAllocate that specifies the record attribute types that are to be returned.
//
// inAttributeInfoOnly
//
// On input, a value of type dsBool set to TRUE if the calling application only wants
// information about attributes. To get the values of the attributes as well as information
// about the attributes, set inAttributeInfoOnly to FALSE.
//
// inOutMatchRecordCount
//
// On input, a pointer to a value of type long that specifies the number of matching records
// to get. On output, inOutRecordEntryCount points to the number of records in the data buffer
// pointed to by inOutDataBuffer; the number may be less than the requested number if there
// were not enough matching records to fill the buffer. The caller cannot change the value of
// inOutRecordEntryCount across multiple calls to this function using the value pointed to by
// inOutContinueData.
//
// inOutContinueData
//
// On input, a pointer to a value of type tContextData and set to NULL. On output, if the value
// pointed to by inOutContinueData is NULL, there are no new results in the buffer. If the
// value pointed to by inOutContinueData is not NULL on output, pass the value pointed to by
// inOutContinueData to this function again to get the next entries. You must call
// dsReleaseContinueData if you donÕt want to get the remaining records.
//
// function result
//
// A value of type tDirStatus indicating success (eDSNoErr) or an error.
// For a list of possible result codes, see ÒResult CodesÓ.
//
// Discussion
//
// This function stores in the data buffer pointed to by inOutDataBuffer a list of records having
// attributes of the type specified by the inAttributeMatchType parameter whose values match the
// specified pattern.
//
// Set inOutRecordEntryCount to point to a positive integer value that represents the number of
// records that are to be returned. You cannot change the value pointed to by inOutRecordEntryCount
// if you call this function with inOutContinueData pointing to context data returned by a
// previous call to this function.
//
// If there are too many records to fit in a single buffer, this function returns a non-null value
// in the value pointed to by inOutContinueData. To get more records, call this function again,
// passing the pointer to inOutContinueData that was returned by the previous call to this function.
//
// To get a record from the data buffer pointed to by inOutDataBuffer, call dsGetRecordEntry. To get
// information about the recordÕs attributes, call dsGetAttributeEntry. To get the value of a
// recordÕs attribute, call dsGetAttributeValue.
//
// When you no longer need inOutContinueData, call dsReleaseContinueData to release the memory
// associated with it.
//
//
#include "LWIAttrValDataQuery.h"

long
LWIAttrValDataQuery::QueryUserInformation(LWIQuery* pQuery, sDoAttrValueSearchWithData* pAttrValueSearchWithData)
{
    long macError = eDSNoErr;
    LWIAttrLookup::Index_t attrIndex = LWIAttrLookup::idx_unknown;

    attrIndex = LWIAttrLookup::GetIndex(pAttrValueSearchWithData->fInAttrType->fBufferData);
    switch (attrIndex)
    {
        case LWIAttrLookup::idx_kDSNAttrRecordName:
            macError = pQuery->QueryUserInformationByName(pAttrValueSearchWithData->fInPatt2Match->fBufferData);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        case LWIAttrLookup::idx_kDS1AttrUniqueID:
        {
            uid_t uid = atoi(pAttrValueSearchWithData->fInPatt2Match->fBufferData);
            macError = pQuery->QueryUserInformationById(uid);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        }
        case LWIAttrLookup::idx_kDS1AttrGeneratedUID:
            macError = pQuery->QueryUserInformationByGeneratedUID(pAttrValueSearchWithData->fInPatt2Match->fBufferData);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        default:
            LOG("Unsupported attribute index for user - %d", attrIndex);
            macError = eDSNoErr;
            break;
    }

cleanup:
    return macError;
}

long
LWIAttrValDataQuery::QueryGroupInformation(LWIQuery* pQuery, sDoAttrValueSearchWithData* pAttrValueSearchWithData)
{
    long macError = eDSNoErr;
    LWIAttrLookup::Index_t attrIndex = LWIAttrLookup::idx_unknown;

    attrIndex = LWIAttrLookup::GetIndex(pAttrValueSearchWithData->fInAttrType->fBufferData);
    switch (attrIndex)
    {
        case LWIAttrLookup::idx_kDSNAttrRecordName:
            macError = pQuery->QueryGroupInformationByName(pAttrValueSearchWithData->fInPatt2Match->fBufferData);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        case LWIAttrLookup::idx_kDSNAttrGroupMembership:
            macError = pQuery->QueryGroupsForUserByName(pAttrValueSearchWithData->fInPatt2Match->fBufferData);
            break;
        case LWIAttrLookup::idx_kDSNAttrGroupMembers:
        {
            uid_t uid;
            macError = ExtractUIDFromGeneratedUID(pAttrValueSearchWithData->fInPatt2Match->fBufferData, uid);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = pQuery->QueryGroupsForUserById(uid);
            GOTO_CLEANUP_ON_MACERROR(macError);

            break;
        }
        case LWIAttrLookup::idx_kDS1AttrPrimaryGroupID:
        {
            gid_t gid = atoi(pAttrValueSearchWithData->fInPatt2Match->fBufferData);
            macError = pQuery->QueryGroupInformationById(gid);
            GOTO_CLEANUP_ON_MACERROR(macError);

            break;
        }
        case LWIAttrLookup::idx_kDS1AttrGeneratedUID:
            macError = pQuery->QueryGroupInformationByGeneratedUID(pAttrValueSearchWithData->fInPatt2Match->fBufferData);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        default:
            LOG("Unsupported attribute index for group - %d", attrIndex);
            macError = eDSNoErr;
            break;
    }

cleanup:
    return macError;
}

long
LWIAttrValDataQuery::Run(sDoAttrValueSearchWithData* pAttrValueSearchWithData)
{
    long macError = eDSNoErr;
    long macError_userQuery = eDSNoErr;
    long macError_groupQuery = eDSNoErr;
    unsigned long bytesWritten = 0;
    unsigned long nRecordsWritten = 0;
    LWIQuery* pQuery = NULL;
    int recordTypeCount;
    int recordAttributeCount;
    bool isWithData = (kDoAttributeValueSearchWithData == pAttrValueSearchWithData->fType);

    LOG_ENTER("fType = %d, fResult = %d, fInNodeRef = %d, fInAttrType = %s, fInPattMatchType = 0x%04X, fInPatt2Match = %s, fInAttrInfoOnly = %s, fOutDataBuff => { size = %d }",
              pAttrValueSearchWithData->fType,
              pAttrValueSearchWithData->fResult,
              pAttrValueSearchWithData->fInNodeRef,
              pAttrValueSearchWithData->fInAttrType->fBufferData,
              pAttrValueSearchWithData->fInPattMatchType,
              pAttrValueSearchWithData->fInPatt2Match->fBufferData,
              BOOL_STRING(isWithData && pAttrValueSearchWithData->fInAttrInfoOnly),
              pAttrValueSearchWithData->fOutDataBuff->fBufferSize);

    recordTypeCount = dsDataListGetNodeCount(pAttrValueSearchWithData->fInRecTypeList);
    recordAttributeCount = isWithData ? dsDataListGetNodeCount(pAttrValueSearchWithData->fInAttrTypeRequestList) : 0;

    LOG_PARAM("fInRecTypeList.count = %d, fInAttrTypeRequestList.count = %d",
              recordTypeCount,
              recordAttributeCount);

    pQuery = new LWIQuery(!isWithData || !pAttrValueSearchWithData->fInAttrInfoOnly);
    if (!pQuery)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = LWIRecTypeLookup::GetVector(pAttrValueSearchWithData->fInRecTypeList, &pQuery->_recTypeSet);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (isWithData)
    {
        macError = LWIAttrLookup::GetVector(pAttrValueSearchWithData->fInAttrTypeRequestList, &pQuery->_attributeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = LWIAttrLookup::GetVector(kDSAttributesAll, &pQuery->_attributeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    // We support only exact (including case-insensitive) matches
    switch (pAttrValueSearchWithData->fInPattMatchType)
    {
        case eDSExact:
        case eDSiExact:
            // These are fine
            break;

        default:
            LOG("Unsupported pattern match type: 0x%04X", pAttrValueSearchWithData->fInPattMatchType);
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = eDSInvalidRecordType;

    if (pQuery->ShouldQueryUserInformation())
    {
        macError_userQuery = QueryUserInformation(pQuery, pAttrValueSearchWithData);
        if (macError_userQuery != eDSNoErr)
        {
            LOG("User query failed [Error: %d]", macError_userQuery);
        }
    }

    if (pQuery->ShouldQueryGroupInformation())
    {
        macError_groupQuery = QueryGroupInformation(pQuery, pAttrValueSearchWithData);
        if (macError_groupQuery != eDSNoErr)
        {
            LOG("Group query failed [Error: %d]", macError_groupQuery);
        }
    }

    // If both queries failed, it is a problem
    if ((macError_userQuery != eDSNoErr) && (macError_groupQuery != eDSNoErr))
    {
       macError = (pQuery->ShouldQueryUserInformation() ? macError_userQuery : macError_groupQuery);
       GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
       macError = eDSNoErr;

    // Write the results
    macError = pQuery->WriteResponse(pAttrValueSearchWithData->fOutDataBuff->fBufferData,
                                     pAttrValueSearchWithData->fOutDataBuff->fBufferSize,
                                     bytesWritten,
                                     nRecordsWritten);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pAttrValueSearchWithData->fOutDataBuff->fBufferLength = bytesWritten;
    pAttrValueSearchWithData->fOutMatchRecordCount = nRecordsWritten;

    if ( bytesWritten > 0 )
    {
        LOG_BUFFER(pAttrValueSearchWithData->fOutDataBuff->fBufferData, bytesWritten);
    }

cleanup:
    if (pQuery)
    {
        delete pQuery;
    }

    LOG_LEAVE("fOutMatchRecordCount = %d --> %d", pAttrValueSearchWithData->fOutMatchRecordCount, macError);

    return macError;
}


