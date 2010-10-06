/*
 *  LWIRecordListQuery.cpp
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/24/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

// sGetRecordList
// Structure called when an Open Directory client calls dsGetRecordList.
//
// typedef struct {
// uInt32              fType;
// sInt32              fResult;
// tDirNodeReference   fInNodeRef;
// tDataBufferPtr      fInDataBuff;
// tDataListPtr        fInRecNameList;
// tDirPatternMatch    fInPatternMatch;
// tDataListPtr        fInRecTypeList;
// tDataListPtr        fInAttribTypeList;
// bool                fInAttribInfoOnly;
// unsigned long       fOutRecEntryCount;
// tContextData        fIOContinueData;
// } sGetRecordList;
//
//
// Fields
//
// fType
//
// Always kGetRecordList.
//
// fResult
//
// Value of type sInt32 that the plug-in sets to eDSNoErr before
// returning to indicate that it was able to get the requested list
// of records for the node identified by fInNodeRef. If an error occurs,
// the plug-in sets fResult to a value listed in ÒResult CodesÓ.
// If no matches are found, the plug-in should set fResult to eDSNoErr,
// fOutRecEntryCount to zero, and fIOContinueData to NULL.
//
// fInNodeRef
//
// Value of type tDirNodeReference that identifies the directory node for
// which the record list is to be obtained. The directory node reference
// was created when the client application opened the directory node.
//
// fInDataBuff
//
// Value of type tDataBufferPtr pointing to the tDataBuffer structure in
// which the plug-in is to return the record list.
//
// fInRecNameList
//
// Value of type tDataListPtr that points to a tDataList structure
// containing patterns in UTF-8 encoding that are to be compared with
// record names. If fInRecNameList is kDSRecordsAll, the plug-in should
// ignore fInPatternMatch and include all records for the directory node
// identified by fInNodeRef.
//
// fInPatternMatch
//
// Value of type tDirPatternMatch that describes the way in which the
// patterns specified by fInRecNameList are to be compared. See Pattern
// Matching Constants for possible constants. The pattern match type may
// also be a type defined by the Open Directory plug-in that handles the
// directory system represented by inDirReference.
//
// fInRecTypeList
//
// Value of type tDataListPtr that points to atDataList structure
// containing the types of records to get. See Standard Record Types and
// Meta Record Type Constants for possible values.
//
// fInAttribTypeList
//
// Value of type tDataListPtr that points to a tDataList structure
// containing the attribute types of records to get. See the attribute
// constants described in the ÒConstantsÓ section for possible values.
//
// fInAttribInfoOnly
//
// Value of type bool. If fInAttribInfoOnly is TRUE, the plug-in should
// include in the buffer pointed to by fInDataBuff attribute information
// for matching records. If fInAttribInfoOnly is FALSE, the plug-in should
// include in the buffer pointed to by fInDataBuff attribute information as
// well as attribute values for matching records.
//
// fOutRecEntryCount
//
// Value of type unsigned long. The first time the client application calls
// dsGetRecordList, fOutRecEntryCount is zero to receive all matching
// records or is a positive integer value that specifies the total number
// of records the client application wants to receive across what may be a
// series of dsGetRecordList calls. If the latter, the plug-in should use
// the initial input value of fOutRecEntryCount to limit the total number
// of matching records it returns. Before returning, the plug-in should set
// fOutRecEntryCount to the number of records it has placed in the buffer
// pointed to by fInDataBuff. The plug-in should ignore the input value of
// fOutRecEntryCount whenever it is processing a sGetRecordList structure
// that has an fIOContinueData field that is not NULL.
//
// fIOContinueData
//
// Value of type tContextData containing continuation data. For the first
// in a series of calls to dsGetRecordList, the input value is NULL. If the
// plug-in can store all of the matching records in the buffer pointed to by
// fInDataBuff, it sets fIOContinueData to NULL before returning. If there
// more records than can be stored in the buffer, the plug-in stores as much
// data as possible and sets fIOContinueData to a plug-inÐdefined value that
// the plug-in can use when the client application calls dsGetRecordList
// again to get another buffer of data. You may want to include a timestamp
// in the continuation data and return an error if you determine that
// fOutContinueData is out of date.
//
// Discussion
//
// The DirectoryService daemon calls a plug-inÕs ProcessRequest entry point
// and passes an sGetRecordList structure when an Open Directory client
// calls dsGetRecordList to get a list of records for a directory node.
// The plug-in uses the fInNodeRef field of the sGetRecordList structure to
// determine the directory node for which the record list is requested, the
// data list pointed to by fInRecNameList to get the names of records for
// which information is requested, the data list pointed to by fInRecTypeList
// to determine the types of records for which information is requested, and
// the data list pointed to by fInAttributeTypeList to determine the
// attributes for which information is requested. The plug-in should return
// only those records whose names match the pattern specified by
// fInRecNameList. The value of the fInAttributeInfoOnly field determines
// whether the plug-in should also return attribute values.
// Depending on the size of the data buffer pointed to by fInDataBuff and the
// length of the list of records, the plug-inÕs routine for processing
// sGetRecordList structures may be called multiple times in order to return
// the complete list. The first time the plug-inÕs routine for processing
// sGetRecordList structures is called, the input value of fIOContinueData
// is NULL and input value of fInOutRecEntryCount specifies the total number
// of records that the plug-in should return even if the plug-inÕs routine for
// processing sGetRecordList structures must be called more than once.
// If there are records that match the criteria specified by fInRecNameList,
// fInPatternMatch, fInRecTypeList, and fInAttributeTypeList, plug-in puts the
// record entries, attribute entries, and attribute values
// (if fInAttributeInfoOnly is FALSE) in the buffer pointed to by fInDataBuff.
// It also sets fInOutRecEntryCount to the number of records that have been
// placed in fInDataBuff and sets fResult to eDSNoErr. If the buffer pointed
// to by fInDataBuff is too small to hold all of the records, the plug-in sets
// fIOContinueData to a plug-inÐdefined value that the plug-in can use when the
// client application calls dsGetRecordList again to get another buffer of data.
// If the buffer pointed to by fInDataBuff contains all of the records or
// contains the last records in the record list, the plug-in sets
// fIOContinueData to NULL. If the plug-in returns before it can get records to
// place in the buffer pointed to by fInDataBuff, it should set fOutRecEntryCount
// to zero, set fResult to eDSNoErr, set fIOContinueData to a plug-inÐdefined
// value that is not NULL. These settings indicate to the client application that
// it should call dsGetRecordList again to get the records. If there are no
// matching records, the plug-in sets fOutRecEntryCount to zero, fIOContinueData
// to NULL, and fResult to eDSNoErr, and returns.

#include "LWIRecordListQuery.h"

long
LWIRecordListQuery::Run(IN OUT sGetRecordList* pGetRecordList)
{
    long macError = eDSNoErr;
    long macError_userQuery = eDSNoErr;
    long macError_groupQuery = eDSNoErr;
    tDataNodePtr pDataNode = NULL;
    const char* szNames;
    unsigned long bytesWritten = 0;
    unsigned long nRecordsWritten = 0;
    unsigned long TotalRecords = 0;
    LWIQuery* pQuery = NULL;
    int recordNameCount;
    int recordTypeCount;
    int recordAttributeCount;
    tContextData HandleId = 0;

    LOG_ENTER("LWIRecordListQuery::Run - fType = %d, fResult = %d, fInNodeRef = %d, "
              "fInDataBuf => { len = %d, size = %d }, fInPatternMatch = 0x%04X, "
              "fInAttribInfoOnly = %s",
              "fIOContinueData = %d",
              pGetRecordList->fType,
              pGetRecordList->fResult,
              pGetRecordList->fInNodeRef,
              pGetRecordList->fInDataBuff->fBufferLength,
              pGetRecordList->fInDataBuff->fBufferSize,
              pGetRecordList->fInPatternMatch,
              BOOL_STRING(pGetRecordList->fInAttribInfoOnly),
              pGetRecordList->fIOContinueData);

    recordNameCount = dsDataListGetNodeCount(pGetRecordList->fInRecNameList);
    recordTypeCount = dsDataListGetNodeCount(pGetRecordList->fInRecTypeList);
    recordAttributeCount = dsDataListGetNodeCount(pGetRecordList->fInAttribTypeList);

    LOG_PARAM("fInRecNameList.count = %d, fInRecTypeList.count = %d, "
              "fInAttribTypeList.count = %d",
              recordNameCount,
              recordTypeCount,
             recordAttributeCount);

    if (pGetRecordList->fIOContinueData != 0 &&
        pGetRecordList->fIOContinueData != SPECIAL_DS_CONTINUE_HANDLE)
    {
        macError = GetQueryFromContextList(pGetRecordList->fIOContinueData, &pQuery);
        if (macError == eDSNoErr)
        {
            LOG("Already processed this query, handling IO continuation for result record data");
            goto HandleResponse;
        }
    }

    pQuery = new LWIQuery(!pGetRecordList->fInAttribInfoOnly,
                          true /* The query results will support fIOContinue (split large results over many calls) */);
    if (!pQuery)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = LWIRecTypeLookup::GetVector(pGetRecordList->fInRecTypeList, &pQuery->_recTypeSet);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIAttrLookup::GetVector(pGetRecordList->fInAttribTypeList, &pQuery->_attributeSet);
    GOTO_CLEANUP_ON_MACERROR(macError);

    // Build list of names
    if ( recordNameCount != 1 ) // I believe that we only get one item in this list
    {
        LOG("GetRecordList got unexpected name list with count equal to: %d", recordNameCount);
        macError = eDSRecordNotFound;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = dsDataListGetNodeAlloc( pGetRecordList->fInNodeRef,
                                       pGetRecordList->fInRecNameList,
                                       1 /* First Item */,
                                       &pDataNode );
    GOTO_CLEANUP_ON_MACERROR( macError );
    szNames = pDataNode->fBufferData;

    macError = eDSInvalidRecordType;

    if (pQuery->ShouldQueryUserInformation())
    {
        macError_userQuery = pQuery->QueryUserInformationByName(szNames);
        if (macError_userQuery != eDSNoErr)
        {
           LOG("User query failed [Error: %d]", macError_userQuery);
        }
    }

    if (pQuery->ShouldQueryGroupInformation())
    {
        macError_groupQuery = pQuery->QueryGroupInformationByName(szNames);
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

HandleResponse:

    // Write the results
    macError = pQuery->WriteResponse(pGetRecordList->fInDataBuff->fBufferData,
                                     pGetRecordList->fInDataBuff->fBufferSize,
                                     bytesWritten,
                                     nRecordsWritten,
                                     TotalRecords);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (TotalRecords > nRecordsWritten)
    {
        macError = AddQueryToContextList(pQuery, &HandleId);
        GOTO_CLEANUP_ON_MACERROR(macError);

        pQuery = NULL;

        pGetRecordList->fIOContinueData = HandleId;
    }
    else
    {
        pGetRecordList->fIOContinueData = 0;
    }

    pGetRecordList->fInDataBuff->fBufferLength = bytesWritten;
    pGetRecordList->fOutRecEntryCount = nRecordsWritten;

    if ( bytesWritten > 0 )
    {
#ifdef SHOW_ALL_DEBUG_SPEW
        LOG_BUFFER(pGetRecordList->fInDataBuff->fBufferData, bytesWritten);
#endif
    }

cleanup:

    if (pDataNode)
    {
        dsDataNodeDeAllocate(0, pDataNode);
    }

    if (pQuery)
    {
        delete pQuery;
    }

    if (macError == eDSBufferTooSmall)
    {
        pGetRecordList->fIOContinueData = (tContextData)SPECIAL_DS_CONTINUE_HANDLE;
    }

    LOG_LEAVE("fOutRecEntryCount = %d, fInDataBuff => { length = %d, size = %d }, fIOContinueData = %d, macError = %d",
              pGetRecordList->fOutRecEntryCount,
              pGetRecordList->fInDataBuff->fBufferLength,
              pGetRecordList->fInDataBuff->fBufferSize,
              pGetRecordList->fIOContinueData,
              macError);

    return macError;
}

long
LWIRecordListQuery::ReleaseContinueData(IN OUT sReleaseContinueData* pReleaseContinueData)
{
    long macError = eDSNoErr;
    LWIQuery* pQuery = NULL;

    LOG_ENTER("fType = %d, fResult = %d, fInNodeRef = %d, "
              "fInContinueData = %d",
              pReleaseContinueData->fType,
              pReleaseContinueData->fResult,
              pReleaseContinueData->fInDirReference,
              pReleaseContinueData->fInContinueData);

    if (pReleaseContinueData->fInContinueData)
    {
        // Special continue handle value, no actual cached query for this one.
        goto cleanup;
    }

    macError = GetQueryFromContextList(pReleaseContinueData->fInContinueData, &pQuery);
    if (macError == eDSNoErr && pQuery)
    {
        /* This is our continue value that we used. Free the pQuery object. */
        pQuery->Release();
    }
    else
    {
        macError = eDSInvalidContinueData;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    LOG_LEAVE("fInContinueData = %d, macError = %d",
              pReleaseContinueData->fInContinueData,
              macError);

    return macError;
}

long
LWIRecordListQuery::Test(
    IN const char* DsPath,
    IN sGetRecordList* pGetRecordList
    )
{
    return Test(DsPath,
                pGetRecordList->fInRecNameList,
                pGetRecordList->fInPatternMatch,
                pGetRecordList->fInRecTypeList,
                pGetRecordList->fInAttribTypeList,
                pGetRecordList->fInAttribInfoOnly,
                pGetRecordList->fInDataBuff->fBufferSize);
}

long
LWIRecordListQuery::Test(
    IN const char* DsPath,
    IN tDataListPtr RecNameList,
    IN tDirPatternMatch PatternMatch,
    IN tDataListPtr RecTypeList,
    IN tDataListPtr AttribTypeList,
    IN dsBool AttribInfoOnly,
    IN unsigned long Size
    )
{
    long macError = eDSNoErr;
    tDirReference dirRef = 0;
    tDirNodeReference dirNode = 0;
    tDataListPtr dirNodeName = NULL;
    tDataBufferPtr pData = NULL;
    UInt32 outCount;
    tContextData continueData = NULL;

    LOG_ENTER("");

    macError = dsOpenDirService( &dirRef );
    GOTO_CLEANUP_ON_MACERROR( macError );

    pData = dsDataBufferAllocate(dirRef, Size);
    if (!pData)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP();
    }

    dirNodeName = dsBuildFromPath( dirRef, DsPath, "/" );
    if (!dirNodeName)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP();
    }

    macError = dsOpenDirNode( dirRef, dirNodeName, &dirNode );
    GOTO_CLEANUP_ON_MACERROR( macError );

    macError = dsGetRecordList( dirNode,
                                pData,
                                RecNameList,
                                PatternMatch,
                                RecTypeList,
                                AttribTypeList,
                                AttribInfoOnly,
                                &outCount,
                                &continueData);
    GOTO_CLEANUP_ON_MACERROR( macError );

    LOG("Got %d records", outCount);

    if (pData->fBufferLength > 0)
    {
#ifdef SHOW_ALL_DEBUG_SPEW
        LOG_BUFFER(pData->fBufferData, pData->fBufferLength);
#endif
    }

cleanup:
    if ( pData )
    {
        dsDataBufferDeAllocate( dirRef, pData );
    }

    if ( dirNodeName )
    {
        dsDataListDeallocate( dirRef, dirNodeName );
    }

    if ( dirNode )
    {
        dsCloseDirNode( dirNode );
    }

    if ( dirRef )
    {
        dsCloseDirService( dirRef );
    }

    LOG_LEAVE("--> %d", macError);

    return macError;
}

