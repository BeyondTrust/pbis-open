/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SpoolManagerWinService.cpp,v $
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
#include "SpoolManagerWinService.h"

#include <iostream>
using namespace std;

using namespace OpenSOAP;

SpoolManagerWinService::SpoolManagerWinService()
{
	//set opensoap server process name
	execProgName = "spoolManager";
	//set opensoap server process log file
	logFileName = OPENSOAP_TMP_PATH + "log/spoolManager";
	//set opensoap server process initial argument
	extParm = "";
}

LPTSTR SpoolManagerWinService::GetName()
{
	//set Windows Service Name
	return "OpenSOAPSpoolManager";
}

DWORD SpoolManagerWinService::ServiceInit(int argc, LPTSTR *argv)
{
	if (argc>1) {
		logFileName = argv[1];
	}
   return NO_ERROR;
}

// オブジェクトを作成する（1つだけ）
SpoolManagerWinService spoolManagerWinService;

