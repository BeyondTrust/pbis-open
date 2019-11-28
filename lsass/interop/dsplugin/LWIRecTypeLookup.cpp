/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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

           dsDataNodeDeAllocate(0, pDataNode);
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
