/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: TTLManagerWinService.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef TTLManagerWinService_H
#define TTLManagerWinService_H

#include "WinServiceBase.h"

class TTLManagerWinService : public WinServiceBase
{
   LPTSTR GetName();
   DWORD ServiceInit(int argc, LPTSTR *argv);
public:

   TTLManagerWinService();
};


#endif //TTLManagerWinService_H

