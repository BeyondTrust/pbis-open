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
    static long Run(sDoAttrValueSearchWithData* pAttrValueSearchWithData);

private:
    static long QueryUserInformation(LWIQuery* pQuery, sDoAttrValueSearchWithData* pAttrValueSearchWithData);
    static long QueryGroupInformation(LWIQuery* pQuery, sDoAttrValueSearchWithData* pAttrValueSearchWithData);
};

#endif /* __LWIATTRVALDATAQUERY_H__ */


