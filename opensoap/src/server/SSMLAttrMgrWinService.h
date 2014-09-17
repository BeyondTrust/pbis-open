/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SSMLAttrMgrWinService.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SSMLAttrMgrWinService_H
#define SSMLAttrMgrWinService_H

#include "WinServiceBase.h"

class SSMLAttrMgrWinService : public WinServiceBase
{
   LPTSTR GetName();
   DWORD ServiceInit(int argc, LPTSTR *argv);
public:

   SSMLAttrMgrWinService();
};


#endif //SSMLAttrMgrWinService_H

