/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: IdManagerWinService.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef IdManagerWinService_H
#define IdManagerWinService_H

#include "WinServiceBase.h"

class IdManagerWinService : public WinServiceBase
{
   LPTSTR GetName();
   DWORD ServiceInit(int argc, LPTSTR *argv);
public:

   IdManagerWinService();
};

#endif //IdManagerWinService_H

