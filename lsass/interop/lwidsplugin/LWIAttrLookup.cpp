/*
 *  LWIAttrLookup.cpp
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/24/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "LWIAttrLookup.h"
#include "LWIMutexLockGuard.h"

static inline long AddToDictionary(
    IN OUT CFMutableDictionaryRef Dictionary,
    IN CFStringRef Key,
    IN LWIAttrLookup::Index_t Value
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

#define _INIT_KEY(StringLiteral) \
    do { \
        macError = AddToDictionary(_dictionary, CFSTR(StringLiteral), idx_ ## StringLiteral); \
        GOTO_CLEANUP_ON_MACERROR(macError); \
    } while (0)

CFMutableDictionaryRef LWIAttrLookup::_dictionary = NULL;
const char* LWIAttrLookup::_type = "attribute";

long LWIAttrLookup::Initialize()
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

    // RealName
    _INIT_KEY(kDS1AttrDistinguishedName);
    _INIT_KEY(kDS1AttrGeneratedUID);
    _INIT_KEY(kDS1AttrNFSHomeDirectory);
    _INIT_KEY(kDS1AttrPassword);
    _INIT_KEY(kDS1AttrPasswordPlus);
    _INIT_KEY(kDS1AttrPasswordPolicyOptions);
    _INIT_KEY(kDS1AttrPrimaryGroupID);
    _INIT_KEY(kDS1AttrUniqueID);
    _INIT_KEY(kDS1AttrUserShell);
    _INIT_KEY(kDSNAttrAuthenticationAuthority);
    _INIT_KEY(kDSNAttrGroupMembership);
    _INIT_KEY(kDSNAttrHomeDirectory);
    _INIT_KEY(kDSNAttrRecordName);
    _INIT_KEY(kDS1AttrPwdAgingPolicy);
    _INIT_KEY(kDS1AttrChange);
    _INIT_KEY(kDS1AttrExpire);
    // AppleMetaNodeLocation
    _INIT_KEY(kDSNAttrMetaNodeLocation);
    _INIT_KEY(kDSNAttrGroupMembers);
    _INIT_KEY(kDS1AttrTimeToLive);
    _INIT_KEY(kDSAttributesAll);
    _INIT_KEY(kDSAttributesStandardAll);
    _INIT_KEY(kDSNAttrAuthMethod);
    _INIT_KEY(kDS1AttrReadOnlyNode);
    _INIT_KEY(kDSNAttrNodePath);
    _INIT_KEY(kDSNAttrRecordType);
    _INIT_KEY(kDSNAttrSubNodes);
    _INIT_KEY(kDS1AttrDataStamp);
    _INIT_KEY(K_DS_ATTR_TRUST_INFORMATION);
    _INIT_KEY(kDS1AttrMCXFlags);
    _INIT_KEY(kDS1AttrMCXSettings);
    _INIT_KEY(kDSNAttrMCXSettings);

cleanup:
    if (macError)
    {
        Cleanup();
    }

    return macError;
}

void LWIAttrLookup::Cleanup()
{
    if (_dictionary)
    {
        CFRelease(_dictionary);
        _dictionary = NULL;
    }
}

long
LWIAttrLookup::GetVector(tDataListPtr List, PLWIBITVECTOR* ppVector)
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
LWIAttrLookup::GetVector(tDataNodePtr Node, PLWIBITVECTOR* ppVector)
{
    return GetVector(Node->fBufferData, ppVector);
}

long
LWIAttrLookup::GetVector(const char* Item, PLWIBITVECTOR* ppVector)
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

LWIAttrLookup::Index_t
LWIAttrLookup::GetIndex(const char* Item)
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
        LOG("Unsupported %s - %s", _type, Item);
    }

    if (key)
    {
        CFRelease(key);
    }

    return index;
}
