/*
 *  LWIAttrValDataQuery.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/24/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __LWIATTRVALDATAQUERY_H__
#define __LWIATTRVALDATAQUERY_H__

#include "LWIQuery.h"

class LWIAttrValDataQuery : public LWIQuery
{
private:
    LWIAttrValDataQuery();
    virtual ~LWIAttrValDataQuery();
    LWIAttrValDataQuery(const LWIAttrValDataQuery& other);
    LWIAttrValDataQuery& operator=(const LWIAttrValDataQuery& other);

public:
    static long Run(sDoAttrValueSearchWithData* pAttrValueSearchWithData, LWE_DS_FLAGS Flags, PNETADAPTERINFO pNetAdapterList);
    static long Run(sDoMultiAttrValueSearchWithData* pMultiAttrValueSearchWithData, LWE_DS_FLAGS Flags, PNETADAPTERINFO pNetAdapterList);
    
private:
    static long QueryUserInformation(LWIQuery* pQuery, char* pszAttribute, char* pszPattern);
    static long QueryGroupInformation(LWIQuery* pQuery, char* pszAttribute, char* pszPattern);
    static long QueryComputerListInformation(LWIQuery* pQuery, char* pszAttribute, char* pszPattern);
    static long QueryComputerGroupInformation(LWIQuery* pQuery, char* pszAttribute, char* pszPattern);
    static long QueryComputerInformation(LWIQuery* pQuery, char* pszAttribute, char* pszPattern);
};

#endif /* __LWIATTRVALDATAQUERY_H__ */


