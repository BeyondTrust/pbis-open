/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: QueueManagerWinService.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef QueueManagerWinService_H
#define QueueManagerWinService_H

#include "WinServiceBase.h"

class QueueManagerWinService : public WinServiceBase
{
   LPTSTR GetName();
   DWORD ServiceInit(int argc, LPTSTR *argv);
public:

   QueueManagerWinService();
};


#endif //QueueManagerWinService_H

