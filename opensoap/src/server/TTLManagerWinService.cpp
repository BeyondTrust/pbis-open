/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: TTLManagerWinService.cpp,v $
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
#include "TTLManagerWinService.h"

#include <iostream>
using namespace std;

using namespace OpenSOAP;

TTLManagerWinService::TTLManagerWinService()
{
	//set opensoap server process name
	execProgName = "ttlManager";
	//set opensoap server process log file
	logFileName = OPENSOAP_TMP_PATH + "log/ttlManager";
	//set opensoap server process initial argument
	extParm = "";
}

LPTSTR TTLManagerWinService::GetName()
{
	//set Windows Service Name
	return "OpenSOAPTTLManager";
}

DWORD TTLManagerWinService::ServiceInit(int argc, LPTSTR *argv)
{
	if (argc>1) {
		logFileName = argv[1];
	}
   return NO_ERROR;
}

// オブジェクトを作成する（1つだけ）
TTLManagerWinService ttlManagerWinService;

