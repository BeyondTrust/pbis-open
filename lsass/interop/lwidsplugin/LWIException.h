/*
 *  LWIException.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/23/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __LWIEXCEPTION_H__
#define __LWIEXCEPTION_H__

#include "LWIPlugIn.h"

class LWIException
{
public:

    LWIException(int errCode)
        : _errCode(errCode)
    {
    }

protected:
    LWIException(const LWIException& other);
    LWIException& operator=(const LWIException& other);

public:

    inline int getErrorCode() { return _errCode; }

private:

    int _errCode;
};

#endif /* __LWIEXCEPTION_H__ */


