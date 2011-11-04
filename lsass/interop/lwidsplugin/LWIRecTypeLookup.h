/*
 *  LWIRecTypeLookup.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/24/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */
#ifndef __LWIRECTYPELOOKUP_H__
#define __LWIRECTYPELOOKUP_H__

#include "LWIPlugIn.h"
#include "LWIMutexLock.h"

class LWIRecTypeLookup
{
public:

    typedef enum
    {
        idx_unknown,                        //  0
        idx_kDSStdRecordTypeUsers,          //  1
        idx_kDSStdRecordTypeGroups,         //  2
        idx_kDSStdRecordTypeComputerLists,  //  3
        idx_kDSStdRecordTypeComputerGroups, //  4
        idx_kDSStdRecordTypeComputers,      //  5
        idx_sentinel                        //  6
    }  Index_t;

private:

    LWIRecTypeLookup();
    ~LWIRecTypeLookup();

    LWIRecTypeLookup(const LWIRecTypeLookup& other);
    LWIRecTypeLookup& operator=(const LWIRecTypeLookup& other);

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

#endif /* __LWIRECTYPELOOKUP_H__ */


