/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MsgDrvCreatorWinService.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef MsgDrvCreatorWinService_H
#define MsgDrvCreatorWinService_H

#include "WinServiceBase.h"

class MsgDrvCreatorWinService : public WinServiceBase
{
   LPTSTR GetName();
   DWORD ServiceInit(int argc, LPTSTR *argv);
public:

   MsgDrvCreatorWinService();
};


#endif //MsgDrvCreatorWinService_H

