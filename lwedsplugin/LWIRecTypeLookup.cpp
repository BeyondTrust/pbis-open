/*
 *  LWIRecTypeLookup.cpp
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/24/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "LWIRecTypeLookup.h"
#include "LWIMutexLockGuard.h"

#define SHOW_DEBUG_SPEW 0 /* GlennC, quiet down the spew! */

#ifndef kDSStdRecordTypeComputerGroups /* For when building on Tiger based build machines */
/*!
 * @defined kDSStdRecordTypeComputerGroups
 * @discussion Identifies computer group records.
 */
#define kDSStdRecordTypeComputerGroups "dsRecTypeStandard:ComputerGroups"
#endif

static inline long AddToDictionary(
    IN OUT CFMutableDictionaryRef Dictionary,
    IN CFStringRef Key,
    IN LWIRecTypeLookup::Index_t Value
    )
{
    int value = Value;
    CFNumberRef number = CFNumberCreate(NULL, kCFNumberIntType, &value);
    if (!number)
    {
        return eDSAllocationFailed;
    }
    CFDictionarySetValue(Dictionary, Key, number);
    CFRelease(number);
    return eDSNoErr;
}

#define _INIT_KEY(StringLiteral, Index) \
    do { \
        macError = AddToDictionary(_dictionary, CFSTR(StringLiteral), Index); \
        GOTO_CLEANUP_ON_MACERROR(macError); \
    } while (0)

CFMutableDictionaryRef LWIRecTypeLookup::_dictionary = NULL;
const char* LWIRecTypeLookup::_type = "record type";

long LWIRecTypeLookup::Initialize()
{
    long macError = eDSNoErr;

    _dictionary = CFDictionaryCreateMutable(NULL,
                                            0,
                                            &kCFCopyStringDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    if (!_dictionary)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    _INIT_KEY(kDSStdRecordTypeUsers, idx_kDSStdRecordTypeUsers);
    _INIT_KEY(kDSStdRecordTypeGroups, idx_kDSStdRecordTypeGroups);
    _INIT_KEY(kDSStdRecordTypeComputerLists, idx_kDSStdRecordTypeComputerLists);
    _INIT_KEY(kDSStdRecordTypeComputerGroups, idx_kDSStdRecordTypeComputerGroups);
    _INIT_KEY(kDSStdRecordTypeComputers, idx_kDSStdRecordTypeComputers);

cleanup:

    if (macError)
    {
        Cleanup();
    }

    return macError;
}

void LWIRecTypeLookup::Cleanup()
{
    if (_dictionary)
    {
        CFRelease(_dictionary);
        _dictionary = NULL;
    }
}

long
LWIRecTypeLookup::GetVector(tDataListPtr List, PLWIBITVECTOR* ppVector)
{
    int macError = eDSNoErr;
    PLWIBITVECTOR pResult = NULL;
    tDataNodePtr pDataNode = NULL;
    int nNodes = 0;
    Index_t value = idx_unknown;

    macError = LWIMakeBitVector(idx_sentinel, &pResult);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if (List != NULL)
    {
       nNodes = dsDataListGetNodeCount(List);

       for (int iNode = 0; iNode < nNodes; iNode++)
       {
           macError = dsDataListGetNodeAlloc(0,
                                             List,
                                             iNode+1,
                                             &pDataNode);
           GOTO_CLEANUP_ON_MACERROR(macError);
           value = GetIndex(pDataNode->fBufferData);
           if (value)
           {
              LWI_BITVECTOR_SET(pResult, value);
           }

           dsDataNodeDeAllocate(NULL, pDataNode);
           pDataNode = NULL;
       }
    }
    
    *ppVector = pResult;
    pResult = NULL;

cleanup:

    if (pResult)
    {
        LWIFreeBitVector(pResult);
    }

    if (pDataNode)
    {
        dsDataNodeDeAllocate(0, pDataNode);
    }

    return macError;
}

long
LWIRecTypeLookup::GetVector(tDataNodePtr Node, PLWIBITVECTOR* ppVector)
{
    return GetVector(Node->fBufferData, ppVector);
}

long
LWIRecTypeLookup::GetVector(const char* Item, PLWIBITVECTOR* ppVector)
{
    int macError = eDSNoErr;
    PLWIBITVECTOR pResult = NULL;
    Index_t value = idx_unknown;

    macError = LWIMakeBitVector(idx_sentinel, &pResult);
    GOTO_CLEANUP_ON_MACERROR(macError);

    value = GetIndex(Item);
    if (value)
    {
        LWI_BITVECTOR_SET(pResult, value);
    }

    *ppVector = pResult;
    pResult = NULL;

cleanup:

    if (pResult)
    {
        LWIFreeBitVector(pResult);
    }

    return macError;
}

LWIRecTypeLookup::Index_t
LWIRecTypeLookup::GetIndex(const char* Item)
{
    CFTypeRef num = NULL;
    CFStringRef key = NULL;
    Index_t value = idx_unknown;
    Index_t index = idx_unknown;

    key = CFStringCreateWithCString(NULL, Item, kCFStringEncodingASCII);
    num = CFDictionaryGetValue(_dictionary, key);
    if (num && CFNumberGetValue((CFNumberRef)num, kCFNumberIntType, &value))
    {
        index = value;
    }
    else
    {
#ifdef SHOW_DEBUG_SPEW
        LOG("Unsupported %s - %s", _type, Item);
#endif
    }

    if (key)
    {
        CFRelease(key);
    }

    return index;
}

