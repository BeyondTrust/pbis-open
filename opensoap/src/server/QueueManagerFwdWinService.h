/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: QueueManagerFwdWinService.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef QueueManagerFwdWinService_H
#define QueueManagerFwdWinService_H

#include "WinServiceBase.h"

class QueueManagerFwdWinService : public WinServiceBase
{
   LPTSTR GetName();
   DWORD ServiceInit(int argc, LPTSTR *argv);
public:

   QueueManagerFwdWinService();
};


#endif //QueueManagerFwdWinService_H

