/*
 *  LWIRecordListQuery.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/24/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */
#ifndef __LWIRECORDLISTQUERY_H__
#define __LWIRECORDLISTQUERY_H__

#include "LWIQuery.h"

class LWIRecordListQuery : public LWIQuery
{
private:
    LWIRecordListQuery();
    ~LWIRecordListQuery();
    LWIRecordListQuery(const LWIRecordListQuery& other);
    LWIRecordListQuery& operator=(const LWIRecordListQuery& other);

public:
    static long Run(IN OUT sGetRecordList* pGetRecordList);

    static
    long
    Test(
        IN const char* DsPath,
        IN sGetRecordList* pGetRecordList
        );

protected:
    static
    long
    Test(
        IN const char* DsPath,
        IN tDataListPtr RecNameList,
        IN tDirPatternMatch PatternMatch,
        IN tDataListPtr RecTypeList,
        IN tDataListPtr AttribTypeList,
        IN dsBool AttribInfoOnly,
        IN unsigned long Size
        );
};

#endif /* __LWIRECORDLISTQUERY_H__ */


