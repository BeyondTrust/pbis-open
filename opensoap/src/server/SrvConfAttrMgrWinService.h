/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SrvConfAttrMgrWinService.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SrvConfAttrMgrWinService_H
#define SrvConfAttrMgrWinService_H

#include "WinServiceBase.h"

class SrvConfAttrMgrWinService : public WinServiceBase
{
   LPTSTR GetName();
   DWORD ServiceInit(int argc, LPTSTR *argv);
public:

   SrvConfAttrMgrWinService();
};


#endif //SrvConfAttrMgrWinService_H

