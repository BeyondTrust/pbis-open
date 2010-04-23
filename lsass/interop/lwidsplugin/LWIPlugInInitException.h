/*
 *  LWIPlugInInitException.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/23/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LWIPLUGININITEXCEPTION_H__
#define __LWIPLUGININITEXCEPTION_H__

#include "LWIPlugIn.h"

class LWIPlugInInitException : public LWIException
{
public:

    LWIPlugInInitException()
        : LWIException(ePlugInInitError)
    {
    }
};

#endif /* __LWIPLUGININITEXCEPTION_H__ */


