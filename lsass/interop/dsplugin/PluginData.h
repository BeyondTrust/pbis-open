/*
  File: PluginData.h

  Version:      Directory Services 1.0

  Copyright:    © 1999-2001 by Apple Computer, Inc., all rights reserved.

  IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc. ("Apple") in
  consideration of your agreement to the following terms, and your use, installation,
  modification or redistribution of this Apple software constitutes acceptance of these
  terms.  If you do not agree with these terms, please do not use, install, modify or
  redistribute this Apple software.

  In consideration of your agreement to abide by the following terms, and subject to these
  terms, Apple grants you a personal, non-exclusive license, under Apple’s copyrights in
  this original Apple software (the "Apple Software"), to use, reproduce, modify and
  redistribute the Apple Software, with or without modifications, in source and/or binary
  forms; provided that if you redistribute the Apple Software in its entirety and without
  modifications, you must retain this notice and the following text and disclaimers in all
  such redistributions of the Apple Software.  Neither the name, trademarks, service marks
  or logos of Apple Computer, Inc. may be used to endorse or promote products derived from
  the Apple Software without specific prior written permission from Apple. Except as expressly
  stated in this notice, no other rights or licenses, express or implied, are granted by Apple
  herein, including but not limited to any patent rights that may be infringed by your
  derivative works or by other works in which the Apple Software may be incorporated.

  The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO WARRANTIES,
  EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS
  USE AND OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.

  IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE,
  REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND
  WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR
  OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __PluginData_H__
#define __PluginData_H__ 1


// System headers
#include "PlugInShell.h"
#include <DirectoryService/DirServicesTypes.h>


// Used to denote if the attribute is read-only or read-write
typedef enum {
    keAttrReadOnly = 0x00000001,
    keAttrReadWrite = 0x00000002
} eAttributeFlags;

// Used by the server and plug-in as state for the plug-in

typedef enum {
    kUnknownState = 0x00000000,
    kActive = 0x00000001,
    kInactive = 0x00000002,
    kInitialized = 0x00000004,
    kUninitialized = 0x00000008,
    kFailedToInit = 0x00000010
} ePluginState;

// Node type registration types

typedef enum {
    kUnknownNodeType = 0x00000000,
    kDirNodeType = 0x00000001,
    kLocalNodeType = 0x00000002
} eDirNodeType;


//-------------------------------------------------
// dsReleaseContinueData

#if 0
typedef struct {
    unsigned long fType;
    long          fResult;
    tDirReference fInDirReference;
    tContextData  fInContinueData;
} sReleaseContinueData;
#endif

typedef struct {
    uint32_t      fType;
    int32_t       fResult;
    tDirReference fInDirReference;
    tContextData  fInContinueData;
} sReleaseContinueData;


//-------------------------------------------------
// dsOpenDirNode

#if 0
typedef struct {
    unsigned long     fType;
    long              fResult;
    tDirReference     fInDirRef;
    tDataListPtr      fInDirNodeName;
    tDirNodeReference fOutNodeRef;
} sOpenDirNode;
#endif

typedef struct {
    uint32_t          fType;
    int32_t           fResult;
    tDirReference     fInDirRef;
    tDataListPtr      fInDirNodeName;
    tDirNodeReference fOutNodeRef;
    uid_t             fInUID;
    uid_t             fInEffectiveUID;
} sOpenDirNode;


//-------------------------------------------------
// dsCloseDirNode

#if 0
typedef struct {
    unsigned long fType;
    long          fResult;
    tDirReference fInNodeRef;
} sCloseDirNode;
#endif

typedef struct {
    uint32_t      fType;
    int32_t       fResult;
    tDirReference fInNodeRef;
} sCloseDirNode;


//-------------------------------------------------
// dsGetDirNodeInfo

#if 0
typedef struct {
    unsigned long     fType;
    long              fResult;
    tDirNodeReference fInNodeRef;
    tDataListPtr      fInDirNodeInfoTypeList;
    tDataBufferPtr    fOutDataBuff;
    dsBool            fInAttrInfoOnly;
    unsigned long     fOutAttrInfoCount;
    tAttributeListRef fOutAttrListRef;
    tContextData      fOutContinueData;
} sGetDirNodeInfo;
#endif

typedef struct {
    uint32_t          fType;
    int32_t           fResult;
    tDirNodeReference fInNodeRef;
    tDataListPtr      fInDirNodeInfoTypeList;
    tDataBufferPtr    fOutDataBuff;
    bool              fInAttrInfoOnly;
    uint32_t          fOutAttrInfoCount;
    tAttributeListRef fOutAttrListRef;
    tContextData      fOutContinueData;
} sGetDirNodeInfo;


//-------------------------------------------------
// dsGetRecordList

#if 0
typedef struct {
    unsigned long     fType;
    long              fResult;
    tDirNodeReference fInNodeRef;
    tDataBufferPtr    fInDataBuff;
    tDataListPtr      fInRecNameList;
    tDirPatternMatch  fInPatternMatch;
    tDataListPtr      fInRecTypeList;
    tDataListPtr      fInAttribTypeList;
    dsBool            fInAttribInfoOnly;
    unsigned long     fOutRecEntryCount;
    tContextData      fIOContinueData;
} sGetRecordList;
#endif

typedef struct {
    uint32_t          fType;
    int32_t           fResult;
    tDirNodeReference fInNodeRef;
    tDataBufferPtr    fInDataBuff;
    tDataListPtr      fInRecNameList;
    tDirPatternMatch  fInPatternMatch;
    tDataListPtr      fInRecTypeList;
    tDataListPtr      fInAttribTypeList;
    bool              fInAttribInfoOnly;
    uint32_t          fOutRecEntryCount;
    tContextData      fIOContinueData;
} sGetRecordList;


//-------------------------------------------------
// dsGetRecordEntry

#if 0
typedef struct {
    unsigned long     fType;
    long              fResult;
    tDirNodeReference fInNodeRef;
    tDataBufferPtr    fInOutDataBuff;
    unsigned long     fInRecEntryIndex;
    tAttributeListRef fOutAttrListRef;
    tRecordEntryPtr   fOutRecEntryPtr;
} sGetRecordEntry;
#endif

typedef struct {
    uint32_t          fType;
    int32_t           fResult;
    tDirNodeReference fInNodeRef;
    tDataBufferPtr    fInOutDataBuff;
    uint32_t          fInRecEntryIndex;
    tAttributeListRef fOutAttrListRef;
    tRecordEntryPtr   fOutRecEntryPtr;
} sGetRecordEntry;



//-------------------------------------------------
// dsGetAttributeEntry

#if 0
typedef struct {
    unsigned long          fType;
    long                   fResult;
    tDirNodeReference      fInNodeRef;
    tDataBufferPtr         fInOutDataBuff;
    tAttributeListRef      fInAttrListRef;
    unsigned long          fInAttrInfoIndex;
    tAttributeValueListRef fOutAttrValueListRef;
    tAttributeEntryPtr     fOutAttrInfoPtr;
} sGetAttributeEntry;
#endif

typedef struct {
    uint32_t               fType;
    int32_t                fResult;
    tDirNodeReference      fInNodeRef;
    tDataBufferPtr         fInOutDataBuff;
    tAttributeListRef      fInAttrListRef;
    uint32_t               fInAttrInfoIndex;
    tAttributeValueListRef fOutAttrValueListRef;
    tAttributeEntryPtr     fOutAttrInfoPtr;
} sGetAttributeEntry;


//-------------------------------------------------
// dsGetAttributeValue

#if 0
typedef struct {
    unsigned long           fType;
    long                    fResult;
    tDirNodeReference       fInNodeRef;
    tDataBufferPtr          fInOutDataBuff;
    unsigned long           fInAttrValueIndex;
    tAttributeValueListRef  fInAttrValueListRef;
    tAttributeValueEntryPtr fOutAttrValue;
} sGetAttributeValue;
#endif

typedef struct {
    uint32_t                fType;
    int32_t                 fResult;
    tDirNodeReference       fInNodeRef;
    tDataBufferPtr          fInOutDataBuff;
    uint32_t                fInAttrValueIndex;
    tAttributeValueListRef  fInAttrValueListRef;
    tAttributeValueEntryPtr fOutAttrValue;
} sGetAttributeValue;


//-------------------------------------------------
// dsCloseAttributeList

#if 0
typedef struct {
    unsigned long     fType;
    long              fResult;
    tAttributeListRef fInAttributeListRef;
} sCloseAttributeList;
#endif

typedef struct {
    uint32_t          fType;
    int32_t           fResult;
    tAttributeListRef fInAttributeListRef;
} sCloseAttributeList;


//-------------------------------------------------
// dsCloseAttributeValueList

#if 0
typedef struct {
    unsigned long          fType;
    long                   fResult;
    tAttributeValueListRef fInAttributeValueListRef;
} sCloseAttributeValueList;
#endif

typedef struct {
    uint32_t               fType;
    int32_t                fResult;
    tAttributeValueListRef fInAttributeValueListRef;
} sCloseAttributeValueList;


//-------------------------------------------------
// dsOpenRecord

#if 0
typedef struct {
    unsigned long     fType;
    long              fResult;
    tDirNodeReference fInNodeRef;
    tDataNodePtr      fInRecType;
    tDataNodePtr      fInRecName;
    tRecordReference  fOutRecRef;
} sOpenRecord;
#endif

typedef struct {
    uint32_t          fType;
    int32_t           fResult;
    tDirNodeReference fInNodeRef;
    tDataNodePtr      fInRecType;
    tDataNodePtr      fInRecName;
    tRecordReference  fOutRecRef;
} sOpenRecord;


//-------------------------------------------------
// dsGetRecordReferenceInfo

#if 0
typedef struct {
    unsigned long    fType;
    long             fResult;
    tRecordReference fInRecRef;
    tRecordEntryPtr  fOutRecInfo;
} sGetRecRefInfo;
#endif

typedef struct {
    uint32_t         fType;
    int32_t          fResult;
    tRecordReference fInRecRef;
    tRecordEntryPtr  fOutRecInfo;
} sGetRecRefInfo;


//-------------------------------------------------
// dsGetRecordAttributeInfo

#if 0
typedef struct {
    unsigned long      fType;
    long               fResult;
    tRecordReference   fInRecRef;
    tDataNodePtr       fInAttrType;
    tAttributeEntryPtr fOutAttrInfoPtr;
} sGetRecAttribInfo;
#endif

typedef struct {
    uint32_t           fType;
    int32_t            fResult;
    tRecordReference   fInRecRef;
    tDataNodePtr       fInAttrType;
    tAttributeEntryPtr fOutAttrInfoPtr;
} sGetRecAttribInfo;


//-------------------------------------------------
// dsGetRecordAttributeValueByID

#if 0
typedef struct {
    unsigned long           fType;
    long                    fResult;
    tRecordReference        fInRecRef;
    tDataNodePtr            fInAttrType;
    unsigned long           fInValueID;
    tAttributeValueEntryPtr fOutEntryPtr;
} sGetRecordAttributeValueByID;
#endif

typedef struct {
    uint32_t                fType;
    int32_t                 fResult;
    tRecordReference        fInRecRef;
    tDataNodePtr            fInAttrType;
    uint32_t                fInValueID;
    tAttributeValueEntryPtr fOutEntryPtr;
} sGetRecordAttributeValueByID;


//-------------------------------------------------
// dsGetRecordAttributeValueByIndex

#if 0
typedef struct {
    unsigned long           fType;
    long                    fResult;
    tRecordReference        fInRecRef;
    tDataNodePtr            fInAttrType;
    unsigned long           fInAttrValueIndex;
    tAttributeValueEntryPtr fOutEntryPtr;
} sGetRecordAttributeValueByIndex;
#endif

typedef struct {
    uint32_t                fType;
    int32_t                 fResult;
    tRecordReference        fInRecRef;
    tDataNodePtr            fInAttrType;
    uint32_t                fInAttrValueIndex;
    tAttributeValueEntryPtr fOutEntryPtr;
} sGetRecordAttributeValueByIndex;


//-------------------------------------------------
// dsGetRecordAttributeValueByValue;

typedef struct {
    uint32_t                fType;
    int32_t                 fResult;
    tRecordReference        fInRecPtr;
    tDataNodePtr            fInAttrType;
    tDataNodePtr            fInAttrValue;
    tAttributeValueEntryPtr fOutEntryPtr;
} sGetRecordAttributeValueByValue;


//-------------------------------------------------
// dsFlushRecord

#if 0
typedef struct {
    unsigned long    fType;
    long             fResult;
    tRecordReference fInRecRef;
} sFlushRecord;
#endif

typedef struct {
    uint32_t         fType;
    int32_t          fResult;
    tRecordReference fInRecRef;
} sFlushRecord;


//-------------------------------------------------
// dsCloseRecord

#if 0
typedef struct {
    unsigned long    fType;
    long             fResult;
    tRecordReference fInRecRef;
} sCloseRecord;
#endif

typedef struct {
    uint32_t         fType;
    int32_t          fResult;
    tRecordReference fInRecRef;
} sCloseRecord;


//-------------------------------------------------
// dsSetRecordName

#if 0
typedef struct {
    unsigned long    fType;
    long             fResult;
    tRecordReference fInRecRef;
    tDataNodePtr     fInNewRecName;
} sSetRecordName;
#endif

typedef struct {
    uint32_t         fType;
    int32_t          fResult;
    tRecordReference fInRecRef;
    tDataNodePtr     fInNewRecName;
} sSetRecordName;


//-------------------------------------------------
// dsSetRecordType

#if 0
typedef struct {
    unsigned long    fType;
    long             fResult;
    tRecordReference fInRecRef;
    tDataNodePtr     fInNewRecType;
} sSetRecordType;
#endif

typedef struct {
    uint32_t         fType;
    int32_t          fResult;
    tRecordReference fInRecRef;
    tDataNodePtr     fInNewRecType;
} sSetRecordType;


//-------------------------------------------------
// dsDeleteRecord

#if 0
typedef struct {
    unsigned long    fType;
    long             fResult;
    tRecordReference fInRecRef;
} sDeleteRecord;
#endif

typedef struct {
    uint32_t         fType;
    int32_t          fResult;
    tRecordReference fInRecRef;
} sDeleteRecord;


//-------------------------------------------------
// dsCreateRecord
// dsCreateRecordAndOpen

#if 0
typedef struct {
    unsigned long     fType;
    long              fResult;
    tDirNodeReference fInNodeRef;
    tDataNodePtr      fInRecType;
    tDataNodePtr      fInRecName;
    dsBool            fInOpen;
    tRecordReference  fOutRecRef;
} sCreateRecord;
#endif

typedef struct {
    uint32_t          fType;
    int32_t           fResult;
    tDirNodeReference fInNodeRef;
    tDataNodePtr      fInRecType;
    tDataNodePtr      fInRecName;
    bool              fInOpen;
    tRecordReference  fOutRecRef;
} sCreateRecord;


//-------------------------------------------------
// dsAddAttribute

#if 0
typedef struct {
    unsigned long          fType;
    long                   fResult;
    tRecordReference       fInRecRef;
    tDataNodePtr           fInNewAttr;
    tAccessControlEntryPtr fInNewAttrAccess;
    tDataNodePtr           fInFirstAttrValue;
} sAddAttribute;
#endif

typedef struct {
    uint32_t               fType;
    int32_t                fResult;
    tRecordReference       fInRecRef;
    tDataNodePtr           fInNewAttr;
    tAccessControlEntryPtr fInNewAttrAccess;
    tDataNodePtr           fInFirstAttrValue;
} sAddAttribute;


//-------------------------------------------------
// dsRemoveAttribute

#if 0
typedef struct {
    unsigned long    fType;
    long             fResult;
    tRecordReference fInRecRef;
    tDataNodePtr     fInAttribute;
} sRemoveAttribute;
#endif

typedef struct {
    uint32_t         fType;
    int32_t          fResult;
    tRecordReference fInRecRef;
    tDataNodePtr     fInAttribute;
} sRemoveAttribute;


//-------------------------------------------------
// dsAddAttributeValue

#if 0
typedef struct {
    unsigned long    fType;
    long             fResult;
    tRecordReference fInRecRef;
    tDataNodePtr     fInAttrType;
    tDataNodePtr     fInAttrValue;
} sAddAttributeValue;
#endif

typedef struct {
    uint32_t         fType;
    int32_t          fResult;
    tRecordReference fInRecRef;
    tDataNodePtr     fInAttrType;
    tDataNodePtr     fInAttrValue;
} sAddAttributeValue;


//-------------------------------------------------
// dsRemoveAttributeValue

#if 0
typedef struct {
    unsigned long    fType;
    long             fResult;
    tRecordReference fInRecRef;
    tDataNodePtr     fInAttrType;
    unsigned long    fInAttrValueID;
} sRemoveAttributeValue;
#endif

typedef struct {
    uint32_t         fType;
    int32_t          fResult;
    tRecordReference fInRecRef;
    tDataNodePtr     fInAttrType;
    uint32_t         fInAttrValueID;
} sRemoveAttributeValue;


//-------------------------------------------------
// dsSetAttributeValue

#if 0
typedef struct {
    unsigned long           fType;
    long                    fResult;
    tRecordReference        fInRecRef;
    tDataNodePtr            fInAttrType;
    tAttributeValueEntryPtr fInAttrValueEntry;
} sSetAttributeValue;
#endif

typedef struct {
    uint32_t                fType;
    int32_t                 fResult;
    tRecordReference        fInRecRef;
    tDataNodePtr            fInAttrType;
    tAttributeValueEntryPtr fInAttrValueEntry;
} sSetAttributeValue;


//-------------------------------------------------
// dsSetAttributeValues

#if 0
typedef struct {
    unsigned long    fType;
    long             fResult;
    tRecordReference fInRecRef;
    tDataNodePtr     fInAttrType;
    tDataListPtr     fInAttrValueList;
} sSetAttributeValues;
#endif

typedef struct {
    uint32_t         fType;
    int32_t          fResult;
    tRecordReference fInRecRef;
    tDataNodePtr     fInAttrType;
    tDataListPtr     fInAttrValueList;
} sSetAttributeValues;


//-------------------------------------------------
// dsDoDirNodeAuth

#if 0
typedef struct {
    unsigned long fType;
    long fResult;
    tDirNodeReference fInNodeRef;
    tDataNodePtr fInAuthMethod;
    dsBool fInDirNodeAuthOnlyFlag;
    tDataBufferPtr fInAuthStepData;
    tDataBufferPtr fOutAuthStepDataResponse;
    tContextData fIOContinueData;
} sDoDirNodeAuth;
#endif

typedef struct {
    uint32_t          fType;
    int32_t           fResult;
    tDirNodeReference fInNodeRef;
    tDataNodePtr      fInAuthMethod;
    bool              fInDirNodeAuthOnlyFlag;
    tDataBufferPtr    fInAuthStepData;
    tDataBufferPtr    fOutAuthStepDataResponse;
    tContextData      fIOContinueData;
} sDoDirNodeAuth;


//-------------------------------------------------
// dsDoAttributeValueSearch

#if 0
typedef struct {
    unsigned long fType;
    long fResult;
    tDirNodeReference fInNodeRef;
    tDataBufferPtr fOutDataBuff;
    tDataListPtr fInRecTypeList;
    tDataNodePtr fInAttrType;
    tDirPatternMatch fInPattMatchType;
    tDataNodePtr fInPatt2Match;
    unsigned long fOutMatchRecordCount;
    tContextData fIOContinueData;
} sDoAttrValueSearch;
#endif

typedef struct {
    uint32_t          fType;
    int32_t           fResult;
    tDirNodeReference fInNodeRef;
    tDataBufferPtr    fOutDataBuff;
    tDataListPtr      fInRecTypeList;
    tDataNodePtr      fInAttrType;
    tDirPatternMatch  fInPattMatchType;
    tDataNodePtr      fInPatt2Match;
    uint32_t          fOutMatchRecordCount;
    tContextData      fIOContinueData;
} sDoAttrValueSearch;


//-------------------------------------------------
// dsDoAttributeValueSearchWithData

#if 0
typedef struct {
    unsigned long     fType;
    long              fResult;
    tDirNodeReference fInNodeRef;
    tDataBufferPtr    fOutDataBuff;
    tDataListPtr      fInRecTypeList;
    tDataNodePtr      fInAttrType;
    tDirPatternMatch  fInPattMatchType;
    tDataNodePtr      fInPatt2Match;
    unsigned long     fOutMatchRecordCount;
    tContextData      fIOContinueData;
    tDataListPtr      fInAttrTypeRequestList;
    dsBool            fInAttrInfoOnly;
} sDoAttrValueSearchWithData;
#endif

typedef struct {
    uint32_t          fType;
    int32_t           fResult;
    tDirNodeReference fInNodeRef;
    tDataBufferPtr    fOutDataBuff;
    tDataListPtr      fInRecTypeList;
    tDataNodePtr      fInAttrType;
    tDirPatternMatch  fInPattMatchType;
    tDataNodePtr      fInPatt2Match;
    uint32_t          fOutMatchRecordCount;
    tContextData      fIOContinueData;
    tDataListPtr      fInAttrTypeRequestList;
    bool              fInAttrInfoOnly;
} sDoAttrValueSearchWithData;


//-------------------------------------------------
// dsDoMultiAttrValueSearch

#if 0
typedef struct {
    unsigned long     fType;
    long              fResult;
    tDirNodeReference fInNodeRef;
    tDataBufferPtr    fOutDataBuff;
    tDataListPtr      fInRecTypeList;
    tDataNodePtr      fInAttrType;
    tDirPatternMatch  fInPattMatchType;
    tDataListPtr      fInPatterns2MatchList;
    unsigned long     fInOutMatchRecordCount;
    tContextData      fIOContinueData;
} sDoMultiAttrValueSearch;
#endif

typedef struct {
    uint32_t          fType;
    int32_t           fResult;
    tDirNodeReference fInNodeRef;
    tDataBufferPtr    fOutDataBuff;
    tDataListPtr      fInRecTypeList;
    tDataNodePtr      fInAttrType;
    tDirPatternMatch  fInPattMatchType;
    tDataListPtr      fInPatterns2MatchList;
    uint32_t          fInOutMatchRecordCount;
    tContextData      fIOContinueData;
} sDoMultiAttrValueSearch;


//-------------------------------------------------
// dsDoMultiAttrValueSearchWithData

#if 0
typedef struct {
    unsigned long     fType;
    long              fResult;
    tDirNodeReference fInNodeRef;
    tDataBufferPtr    fOutDataBuff;
    tDataListPtr      fInRecTypeList;
    tDataNodePtr      fInAttrType;
    tDirPatternMatch  fInPattMatchType;
    tDataListPtr      fInPatterns2MatchList;
    unsigned long     fInOutMatchRecordCount;
    tContextData      fIOContinueData;
    tDataListPtr      fInAttrTypeRequestList;
    dsBool            fInAttrInfoOnly;
} sDoMultiAttrValueSearchWithData;
#endif

typedef struct {
    uint32_t          fType;
    int32_t           fResult;
    tDirNodeReference fInNodeRef;
    tDataBufferPtr    fOutDataBuff;
    tDataListPtr      fInRecTypeList;
    tDataNodePtr      fInAttrType;
    tDirPatternMatch  fInPattMatchType;
    tDataListPtr      fInPatterns2MatchList;
    uint32_t          fInOutMatchRecordCount;
    tContextData      fIOContinueData;
    tDataListPtr      fInAttrTypeRequestList;
    bool              fInAttrInfoOnly;
} sDoMultiAttrValueSearchWithData;


//-------------------------------------------------
// dsDoPlugInCustomCall

#if 0
typedef struct {
    unsigned long     fType;
    long              fResult;
    tDirNodeReference fInNodeRef;
    unsigned long     fInRequestCode;
    tDataBufferPtr    fInRequestData;
    tDataBufferPtr    fOutRequestResponse;
} sDoPlugInCustomCall;
#endif

typedef struct {
    uint32_t          fType;
    int32_t           fResult;
    tDirNodeReference fInNodeRef;
    uint32_t          fInRequestCode;
    tDataBufferPtr    fInRequestData;
    tDataBufferPtr    fOutRequestResponse;
} sDoPlugInCustomCall;


//-------------------------------------------------
// Header

#if 0
typedef struct {
    unsigned long fType;
    long          fResult;
} sHeader;
#endif

typedef struct {
    uint32_t     fType;
    int32_t      fResult;
    tContextData fContextData;
} sHeader;

#endif // __PluginData_H__
