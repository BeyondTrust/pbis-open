/*
 *  LWIAttrLookup.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/24/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */
#ifndef __LWIATTRLOOKUP_H__
#define __LWIATTRLOOKUP_H__

#include "LWIPlugIn.h"
#include "LWIMutexLock.h"

#define K_DS_ATTR_TRUST_INFORMATION "dsAttrTypeStandard:TrustInformation"

class LWIAttrLookup
{
public:

    typedef enum
    {
        idx_unknown,                         //  0
        idx_kDS1AttrDistinguishedName,       //  1
        idx_kDS1AttrGeneratedUID,            //  2
        idx_kDS1AttrNFSHomeDirectory,        //  3
        idx_kDS1AttrPassword,                //  4
        idx_kDS1AttrPasswordPlus,            //  5
        idx_kDS1AttrPasswordPolicyOptions,   //  6
        idx_kDS1AttrPrimaryGroupID,          //  7
        idx_kDS1AttrUniqueID,                //  8
        idx_kDS1AttrUserShell,               //  9
        idx_kDSNAttrAuthenticationAuthority, // 10
        idx_kDSNAttrGroupMembership,         // 11
        idx_kDSNAttrHomeDirectory,           // 12
        idx_kDSNAttrRecordName,              // 13
        idx_kDS1AttrPwdAgingPolicy,          // 14
        idx_kDS1AttrChange,                  // 15
        idx_kDS1AttrExpire,                  // 16
        idx_kDSNAttrMetaNodeLocation,        // 17
        idx_kDSNAttrGroupMembers,            // 18
        idx_kDS1AttrTimeToLive,              // 19
        idx_kDSAttributesAll,                // 20
        idx_kDSAttributesStandardAll,        // 21
        idx_kDSNAttrAuthMethod,              // 22
        idx_kDS1AttrReadOnlyNode,            // 23
        idx_kDSNAttrNodePath,                // 24
        idx_kDSNAttrRecordType,              // 25
        idx_kDSNAttrSubNodes,                // 26
        idx_kDS1AttrDataStamp,               // 27
        idx_K_DS_ATTR_TRUST_INFORMATION,     // 28
        idx_kDS1AttrMCXFlags,                // 29
        idx_kDS1AttrMCXSettings,             // 30
        idx_kDSNAttrMCXSettings,             // 31
        idx_sentinel                         // 32
    }  Index_t;

private:

    LWIAttrLookup();
    ~LWIAttrLookup();

    LWIAttrLookup(const LWIAttrLookup& other);
    LWIAttrLookup& operator=(const LWIAttrLookup& other);

public:

    static long Initialize();
    static void Cleanup();

    static long GetVector(tDataListPtr List, PLWIBITVECTOR* ppVector);
    static long GetVector(tDataNodePtr Node, PLWIBITVECTOR* ppVector);
    static long GetVector(const char* Item, PLWIBITVECTOR* ppVector);
    static Index_t GetIndex(const char* Item);

private:

    static CFMutableDictionaryRef _dictionary;
    static const char* _type;
};

#endif /* __LWIATTRLOOKUP_H__ */

