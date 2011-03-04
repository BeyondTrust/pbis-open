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
// If there are too many records to fit in a single buffer, this function returns a non-NULL value
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

#include "includes.h"
#include "LWIAttrValDataQuery.h"

#define SHOW_DEBUG_SPEW 0 /* GlennC, quiet down the spew! */

long
LWIAttrValDataQuery::QueryUserInformation(LWIQuery* pQuery, char* pszAttribute, char* pszPattern)
{
    long macError = eDSNoErr;
    LWIAttrLookup::Index_t attrIndex = LWIAttrLookup::idx_unknown;

    attrIndex = LWIAttrLookup::GetIndex(pszAttribute);
    switch (attrIndex)
    {
        case LWIAttrLookup::idx_kDSNAttrRecordName:
        case LWIAttrLookup::idx_kDS1AttrDistinguishedName:
            macError = pQuery->QueryUserInformationByName(pszPattern);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        case LWIAttrLookup::idx_kDS1AttrUniqueID:
        {
            uid_t uid = atoi(pszPattern);
            macError = pQuery->QueryUserInformationById(uid);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        }
        case LWIAttrLookup::idx_kDS1AttrGeneratedUID:
            macError = pQuery->QueryUserInformationByGeneratedUID(pszPattern);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        case LWIAttrLookup::idx_kDS1AttrPrimaryGroupID:
            macError = pQuery->QueryUserInformationByPrimaryGroupID(pszPattern);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        default:
#ifdef SHOW_DEBUG_SPEW
            LOG("Unsupported attribute index for user - %d", attrIndex);
#endif
            macError = eDSNoErr;
            break;
    }

cleanup:

    return macError;
}

long
LWIAttrValDataQuery::QueryGroupInformation(LWIQuery* pQuery, char* pszAttribute, char* pszPattern)
{
    long macError = eDSNoErr;
    LWIAttrLookup::Index_t attrIndex = LWIAttrLookup::idx_unknown;

    attrIndex = LWIAttrLookup::GetIndex(pszAttribute);
    switch (attrIndex)
    {
        case LWIAttrLookup::idx_kDSNAttrRecordName:
        case LWIAttrLookup::idx_kDS1AttrDistinguishedName:
            macError = pQuery->QueryGroupInformationByName(pszPattern);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        case LWIAttrLookup::idx_kDSNAttrGroupMembership:
            macError = pQuery->QueryGroupsForUserByName(pszPattern);
            break;
        case LWIAttrLookup::idx_kDSNAttrGroupMembers:
        {
            uid_t uid;
            macError = ExtractUIDFromGeneratedUID(pszPattern, uid);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = pQuery->QueryGroupsForUserById(uid);
            GOTO_CLEANUP_ON_MACERROR(macError);

            break;
        }
        case LWIAttrLookup::idx_kDS1AttrPrimaryGroupID:
        {
            gid_t gid = atoi(pszPattern);
            macError = pQuery->QueryGroupInformationById(gid);
            GOTO_CLEANUP_ON_MACERROR(macError);

            break;
        }
        case LWIAttrLookup::idx_kDS1AttrGeneratedUID:
            macError = pQuery->QueryGroupInformationByGeneratedUID(pszPattern);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        default:
#ifdef SHOW_DEBUG_SPEW
            LOG("Unsupported attribute index for group - %d", attrIndex);
#endif
            macError = eDSNoErr;
            break;
    }

cleanup:

    return macError;
}

long
LWIAttrValDataQuery::QueryComputerListInformation(LWIQuery* pQuery, char* pszAttribute, char* pszPattern)
{
    long macError = eDSNoErr;
    LWIAttrLookup::Index_t attrIndex = LWIAttrLookup::idx_unknown;

    attrIndex = LWIAttrLookup::GetIndex(pszAttribute);
    switch (attrIndex)
    {
        case LWIAttrLookup::idx_kDSNAttrRecordName:
            macError = pQuery->QueryComputerListInformationByName(pszPattern);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        case LWIAttrLookup::idx_kDSNAttrComputers:
            if (!strcmp(pszPattern, "localhost"))
            {
                macError = pQuery->QueryComputerListInformationByName(kDSRecordsAll);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            }
        default:
#ifdef SHOW_DEBUG_SPEW
            LOG("Unsupported attribute index for group - %d", attrIndex);
#endif
            macError = eDSNoErr;
            break;
    }

cleanup:

    return macError;
}

long
LWIAttrValDataQuery::QueryComputerGroupInformation(LWIQuery* pQuery, char* pszAttribute, char* pszPattern)
{
    long macError = eDSNoErr;
    LWIAttrLookup::Index_t attrIndex = LWIAttrLookup::idx_unknown;
    PSTR pszHostname = NULL;
 
    macError = GetDnsHostName(&pszHostname);
    GOTO_CLEANUP_ON_MACERROR(macError);

    attrIndex = LWIAttrLookup::GetIndex(pszAttribute);
    switch (attrIndex)
    {
        case LWIAttrLookup::idx_kDSNAttrRecordName:
            macError = pQuery->QueryComputerGroupInformationByName(pszPattern);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        case LWIAttrLookup::idx_kDSNAttrComputers:
            if (!strcmp(pszPattern, "localhost") ||
                !strcmp(pszPattern, pszHostname) )
            {
                macError = pQuery->QueryComputerGroupInformationByName(kDSRecordsAll);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            }
            else
            {
                LOG("Got request to search for %s as a (Computers) attribute for ComputerGroup objects, currently not supported and therefore ignoring",
                    pszPattern);
                break;
            }
        case LWIAttrLookup::idx_kDSNAttrGroupMembership:
            if (!strcmp(pszPattern, "localhost") ||
                !strcmp(pszPattern, pszHostname))
            {
                macError = pQuery->QueryComputerGroupInformationByName(kDSRecordsAll);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            }
            else
            {
                LOG("Got request to search for %s as a GroupMembership) attribute for ComputerGroup objects, currently not supported and therefore ignoring",
                    pszPattern);
                break;
            }
        default:
#ifdef SHOW_DEBUG_SPEW
            LOG("Unsupported attribute index for group - %d", attrIndex);
#endif
            macError = eDSNoErr;
            break;
    }

cleanup:

    if (pszHostname)
    {
        LW_SAFE_FREE_STRING(pszHostname);
    }

    return macError;
}

long
LWIAttrValDataQuery::QueryComputerInformation(LWIQuery* pQuery, char* pszAttribute, char* pszPattern)
{
    long macError = eDSNoErr;
    LWIAttrLookup::Index_t attrIndex = LWIAttrLookup::idx_unknown;

    attrIndex = LWIAttrLookup::GetIndex(pszAttribute);
    switch (attrIndex)
    {
        case LWIAttrLookup::idx_kDSNAttrRecordName:
            macError = pQuery->QueryComputerInformationByName(pszPattern);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        case LWIAttrLookup::idx_kDS1AttrENetAddress:
            macError = pQuery->QueryComputerInformationByENetAddress(pszPattern);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        case LWIAttrLookup::idx_kDSNAttrIPAddress:
            macError = pQuery->QueryComputerInformationByIPAddress(pszPattern);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        case LWIAttrLookup::idx_kDS1AttrGeneratedUID:
            macError = pQuery->QueryComputerInformationByGeneratedUID(pszPattern);
            GOTO_CLEANUP_ON_MACERROR(macError);
            break;
        default:
#ifdef SHOW_DEBUG_SPEW
            LOG("Unsupported attribute index for group - %d", attrIndex);
#endif
            macError = eDSNoErr;
            break;
    }

cleanup:

    return macError;
}

long
LWIAttrValDataQuery::Run(sDoMultiAttrValueSearchWithData* pMultiAttrValueSearchWithData, LWE_DS_FLAGS Flags, PNETADAPTERINFO pNetAdapterList)
{
    long macError = eDSNoErr;
    long macError_userQuery = eDSNoErr;
    long macError_groupQuery = eDSNoErr;
    unsigned long bytesWritten = 0;
    unsigned long nRecordsWritten = 0;
    unsigned long TotalRecords = 0;
    LWIQuery* pQuery = NULL;
    int iter = 0;
    int resultCount = 0;
    int patternListCount = 0;
    int recordTypeCount = 0;
    int recordAttributeCount = 0;
    tDataNodePtr pPatternNode = NULL;
    bool isWithData = (kDoMultipleAttributeValueSearchWithData == pMultiAttrValueSearchWithData->fType);
    tContextData HandleId = 0;
 
    LOG_ENTER("LWIAttrValDataQuery::Run - fType = %d, fResult = %d, fInNodeRef = %d, fInAttrType = %s, fInPattMatchType = 0x%04X, fInAttrInfoOnly = %s, fIOContinueData = %d, fOutDataBuff => { size = %d }",
              pMultiAttrValueSearchWithData->fType,
              pMultiAttrValueSearchWithData->fResult,
              pMultiAttrValueSearchWithData->fInNodeRef,
              pMultiAttrValueSearchWithData->fInAttrType->fBufferData,
              pMultiAttrValueSearchWithData->fInPattMatchType,
              BOOL_STRING(isWithData && pMultiAttrValueSearchWithData->fInAttrInfoOnly),
              pMultiAttrValueSearchWithData->fIOContinueData,
              pMultiAttrValueSearchWithData->fOutDataBuff->fBufferSize);

    patternListCount = dsDataListGetNodeCount(pMultiAttrValueSearchWithData->fInPatterns2MatchList);
    recordTypeCount = dsDataListGetNodeCount(pMultiAttrValueSearchWithData->fInRecTypeList);
    recordAttributeCount = isWithData ? dsDataListGetNodeCount(pMultiAttrValueSearchWithData->fInAttrTypeRequestList) : 0;

    LOG_PARAM("fInPatterns2MatchList.count = %d, fInRecTypeList.count = %d, fInAttrTypeRequestList.count = %d",
              patternListCount,
              recordTypeCount,
              recordAttributeCount);

    if (pMultiAttrValueSearchWithData->fIOContinueData != 0)
    {
        macError = GetQueryFromContextList(pMultiAttrValueSearchWithData->fIOContinueData, &pQuery);
        if (macError == eDSNoErr)
        {
            LOG("Already processed this query, handling IO continuation for result record data");
            goto HandleResponse;
        }
    }
    
    macError = LWIQuery::Create(!isWithData || !pMultiAttrValueSearchWithData->fInAttrInfoOnly,
                                true, // The query results will support fIOContinue (split large results over many calls)
                                pMultiAttrValueSearchWithData->fInNodeRef,
                                Flags,
                                pNetAdapterList,
                                &pQuery);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIRecTypeLookup::GetVector(pMultiAttrValueSearchWithData->fInRecTypeList, &pQuery->_recTypeSet);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (isWithData)
    {
        macError = LWIAttrLookup::GetVector(pMultiAttrValueSearchWithData->fInAttrTypeRequestList, &pQuery->_attributeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = LWIAttrLookup::GetVector(kDSAttributesAll, &pQuery->_attributeSet);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    // We support only exact (including case-insensitive) matches
    switch (pMultiAttrValueSearchWithData->fInPattMatchType)
    {
        case eDSExact:
        case eDSiExact:
            // These are fine
            break;

        default:
            LOG("Unsupported pattern match type: 0x%04X", pMultiAttrValueSearchWithData->fInPattMatchType);
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = eDSInvalidRecordType;

    for (iter = 0; iter < patternListCount; iter++)
    {
        macError = dsDataListGetNodeAlloc(pMultiAttrValueSearchWithData->fInNodeRef,
                                          pMultiAttrValueSearchWithData->fInPatterns2MatchList,
                                          iter+1,
                                          &pPatternNode);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        LOG_PARAM("Searching for pattern (%s)...", pPatternNode->fBufferData);
                    
        if (pQuery->ShouldQueryUserInformation())
        {
            macError_userQuery = QueryUserInformation(pQuery,
                                                      pMultiAttrValueSearchWithData->fInAttrType->fBufferData,
                                                      pPatternNode->fBufferData);
            if (macError_userQuery != eDSNoErr)
            {
                LOG("User query failed [Error: %d]", macError_userQuery);
            }
            else
            {
                resultCount++;
            }
        }

        if (pQuery->ShouldQueryGroupInformation())
        {
            macError_groupQuery = QueryGroupInformation(pQuery,
                                                        pMultiAttrValueSearchWithData->fInAttrType->fBufferData,
                                                        pPatternNode->fBufferData);
            if (macError_groupQuery != eDSNoErr)
            {
                LOG("Group query failed [Error: %d]", macError_groupQuery);
            }
            else
            {
                resultCount++;
            }
        }
    
        if (pQuery->ShouldQueryComputerListInformation())
        {
            macError_groupQuery = QueryComputerListInformation(pQuery,
                                                               pMultiAttrValueSearchWithData->fInAttrType->fBufferData,
                                                               pPatternNode->fBufferData);
            if (macError_groupQuery != eDSNoErr)
            {
                LOG("Computer List query failed [Error: %d]", macError_groupQuery);
            }
            else
            {
                resultCount++;
            }
        }

        if (pQuery->ShouldQueryComputerGroupInformation())
        {
            macError_groupQuery = QueryComputerGroupInformation(pQuery,
                                                                pMultiAttrValueSearchWithData->fInAttrType->fBufferData,
                                                                pPatternNode->fBufferData);
            if (macError_groupQuery != eDSNoErr)
            {
                LOG("Computer Group query failed [Error: %d]", macError_groupQuery);
            }
            else
            {
                resultCount++;
            }
        }

        if (pQuery->ShouldQueryComputerInformation())
        {
            macError_groupQuery = QueryComputerInformation(pQuery,
                                                           pMultiAttrValueSearchWithData->fInAttrType->fBufferData,
                                                           pPatternNode->fBufferData);
            if (macError_groupQuery != eDSNoErr)
            {
                LOG("Computer query failed [Error: %d]", macError_groupQuery);
            }
            else
            {
                resultCount++;
            }
        }

        dsDataNodeDeAllocate(NULL, pPatternNode);
        pPatternNode = NULL;
    }

    // If both queries failed, it is a problem
    if (resultCount == 0 && (macError_userQuery != eDSNoErr) && (macError_groupQuery != eDSNoErr))
    {
       macError = (pQuery->ShouldQueryUserInformation() ? macError_userQuery : macError_groupQuery);
       GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
       macError = eDSNoErr;

HandleResponse:

    // Write the results
    macError = pQuery->WriteResponse(pMultiAttrValueSearchWithData->fOutDataBuff->fBufferData,
                                     pMultiAttrValueSearchWithData->fOutDataBuff->fBufferSize,
                                     bytesWritten,
                                     nRecordsWritten,
                                     TotalRecords);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (TotalRecords > nRecordsWritten)
    {
        macError = AddQueryToContextList(pQuery, &HandleId);
        GOTO_CLEANUP_ON_MACERROR(macError);

        pQuery = NULL;
        pMultiAttrValueSearchWithData->fIOContinueData = HandleId;
    }
    else
    {
        pMultiAttrValueSearchWithData->fIOContinueData = 0;
    }

    pMultiAttrValueSearchWithData->fOutDataBuff->fBufferLength = bytesWritten;
    pMultiAttrValueSearchWithData->fInOutMatchRecordCount = nRecordsWritten;

    if ( bytesWritten > 0 )
    {
#ifdef SHOW_ALL_DEBUG_SPEW
        LOG_BUFFER(pMultiAttrValueSearchWithData->fOutDataBuff->fBufferData, bytesWritten);
#endif
    }

cleanup:

    if (pQuery)
    {
        delete pQuery;
    }
    
    if (pPatternNode)
    {
        dsDataNodeDeAllocate(0, pPatternNode);
    }

    LOG_LEAVE("fInOutMatchRecordCount = %d, fIOContinueData = %d --> %d", pMultiAttrValueSearchWithData->fInOutMatchRecordCount, pMultiAttrValueSearchWithData->fIOContinueData, macError);

    return macError;
}

long
LWIAttrValDataQuery::Run(sDoAttrValueSearchWithData* pAttrValueSearchWithData, LWE_DS_FLAGS Flags, PNETADAPTERINFO pNetAdapterList)
{
    long macError = eDSNoErr;
    long macError_userQuery = eDSNoErr;
    long macError_groupQuery = eDSNoErr;
    unsigned long bytesWritten = 0;
    unsigned long nRecordsWritten = 0;
    unsigned long TotalRecords = 0;
    LWIQuery* pQuery = NULL;
    int recordTypeCount;
    int recordAttributeCount;
    bool isWithData = (kDoAttributeValueSearchWithData == pAttrValueSearchWithData->fType);
    tContextData HandleId = 0;
 
    LOG_ENTER("LWIAttrValDataQuery::Run - fType = %d, fResult = %d, fInNodeRef = %d, fInAttrType = %s, fInPattMatchType = 0x%04X, fInPatt2Match = %s, fInAttrInfoOnly = %s, fIOContinueData = %d, fOutDataBuff => { size = %d }",
              pAttrValueSearchWithData->fType,
              pAttrValueSearchWithData->fResult,
              pAttrValueSearchWithData->fInNodeRef,
              pAttrValueSearchWithData->fInAttrType->fBufferData,
              pAttrValueSearchWithData->fInPattMatchType,
              pAttrValueSearchWithData->fInPatt2Match->fBufferData,
              BOOL_STRING(isWithData && pAttrValueSearchWithData->fInAttrInfoOnly),
              pAttrValueSearchWithData->fIOContinueData,
              pAttrValueSearchWithData->fOutDataBuff->fBufferSize);

    recordTypeCount = dsDataListGetNodeCount(pAttrValueSearchWithData->fInRecTypeList);
    recordAttributeCount = isWithData ? dsDataListGetNodeCount(pAttrValueSearchWithData->fInAttrTypeRequestList) : 0;

    LOG_PARAM("fInRecTypeList.count = %d, fInAttrTypeRequestList.count = %d",
              recordTypeCount,
              recordAttributeCount);

    if (pAttrValueSearchWithData->fIOContinueData != 0)
    {
        macError = GetQueryFromContextList(pAttrValueSearchWithData->fIOContinueData, &pQuery);
        if (macError == eDSNoErr)
        {
            LOG("Already processed this query, handling IO continuation for result record data");
            goto HandleResponse;
        }
    }

    macError = LWIQuery::Create(!isWithData || !pAttrValueSearchWithData->fInAttrInfoOnly,
                                true, // The query results will support fIOContinue (split large results over many calls)
                                pAttrValueSearchWithData->fInNodeRef,
                                Flags,
                                pNetAdapterList,
                                &pQuery);
    GOTO_CLEANUP_ON_MACERROR(macError);

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
        macError_userQuery = QueryUserInformation(pQuery,
                                                  pAttrValueSearchWithData->fInAttrType->fBufferData,
                                                  pAttrValueSearchWithData->fInPatt2Match->fBufferData);
        if (macError_userQuery != eDSNoErr)
        {
            LOG("User query failed [Error: %d]", macError_userQuery);
        }
    }

    if (pQuery->ShouldQueryGroupInformation())
    {
        macError_groupQuery = QueryGroupInformation(pQuery,
                                                    pAttrValueSearchWithData->fInAttrType->fBufferData,
                                                    pAttrValueSearchWithData->fInPatt2Match->fBufferData);
        if (macError_groupQuery != eDSNoErr)
        {
            LOG("Group query failed [Error: %d]", macError_groupQuery);
        }
    }
    
    if (pQuery->ShouldQueryComputerListInformation())
    {
        macError_groupQuery = QueryComputerListInformation(pQuery,
                                                           pAttrValueSearchWithData->fInAttrType->fBufferData,
                                                           pAttrValueSearchWithData->fInPatt2Match->fBufferData);
        if (macError_groupQuery != eDSNoErr)
        {
            LOG("Computer List query failed [Error: %d]", macError_groupQuery);
        }
    }

    if (pQuery->ShouldQueryComputerGroupInformation())
    {
        macError_groupQuery = QueryComputerGroupInformation(pQuery,
                                                            pAttrValueSearchWithData->fInAttrType->fBufferData,
                                                            pAttrValueSearchWithData->fInPatt2Match->fBufferData);
        if (macError_groupQuery != eDSNoErr)
        {
            LOG("Computer Group query failed [Error: %d]", macError_groupQuery);
        }
    }

    if (pQuery->ShouldQueryComputerInformation())
    {
        macError_groupQuery = QueryComputerInformation(pQuery,
                                                       pAttrValueSearchWithData->fInAttrType->fBufferData,
                                                       pAttrValueSearchWithData->fInPatt2Match->fBufferData);
        if (macError_groupQuery != eDSNoErr)
        {
            LOG("Computer query failed [Error: %d]", macError_groupQuery);
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

HandleResponse:

    // Write the results
    macError = pQuery->WriteResponse(pAttrValueSearchWithData->fOutDataBuff->fBufferData,
                                     pAttrValueSearchWithData->fOutDataBuff->fBufferSize,
                                     bytesWritten,
                                     nRecordsWritten,
                                     TotalRecords);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (TotalRecords > nRecordsWritten)
    {
        macError = AddQueryToContextList(pQuery, &HandleId);
        GOTO_CLEANUP_ON_MACERROR(macError);

        pQuery = NULL;
  
        pAttrValueSearchWithData->fIOContinueData = HandleId;
    }
    else
    {
        pAttrValueSearchWithData->fIOContinueData = 0;
    }

    pAttrValueSearchWithData->fOutDataBuff->fBufferLength = bytesWritten;
    pAttrValueSearchWithData->fOutMatchRecordCount = nRecordsWritten;

    if ( bytesWritten > 0 )
    {
#ifdef SHOW_ALL_DEBUG_SPEW
        LOG_BUFFER(pAttrValueSearchWithData->fOutDataBuff->fBufferData, bytesWritten);
#endif
    }

cleanup:

    if (pQuery)
    {
        delete pQuery;
    }

    LOG_LEAVE("fOutMatchRecordCount = %d, fIOContinueData = %d --> %d", pAttrValueSearchWithData->fOutMatchRecordCount, pAttrValueSearchWithData->fIOContinueData, macError);

    return macError;
}


