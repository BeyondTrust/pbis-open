/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: QueueManagerFwdWinService.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <windows.h>
#include <io.h>
#include "ServerCommon.h"
#include "QueueManagerFwdWinService.h"

#include <iostream>
using namespace std;

using namespace OpenSOAP;

QueueManagerFwdWinService::QueueManagerFwdWinService()
{
	//set opensoap server process name
	execProgName = "queueManager";
	//set opensoap server process log file
	logFileName = OPENSOAP_TMP_PATH + "log/queueManager_fwd";
	//set opensoap server process initial argument
	extParm = "1";
}

LPTSTR QueueManagerFwdWinService::GetName()
{
	//set Windows Service Name
	return "OpenSOAPQueueManagerFwd";
}

DWORD QueueManagerFwdWinService::ServiceInit(int argc, LPTSTR *argv)
{
	if (argc>1) {
		logFileName = argv[1];
	}
   return NO_ERROR;
}

// オブジェクトを作成する（1つだけ）
QueueManagerFwdWinService queueManagerFwdWinService;

