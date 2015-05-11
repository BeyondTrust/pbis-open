/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SpoolManagerWinService.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SpoolManagerWinService_H
#define SpoolManagerWinService_H

#include "WinServiceBase.h"

class SpoolManagerWinService : public WinServiceBase
{
   LPTSTR GetName();
   DWORD ServiceInit(int argc, LPTSTR *argv);
public:

   SpoolManagerWinService();
};


#endif //SpoolManagerWinService_H

