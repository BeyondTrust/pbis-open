/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: IdManagerWinService.cpp,v $
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
#include "IdManagerWinService.h"
#include "IdManager.h"

#include <iostream>
using namespace std;

using namespace OpenSOAP;

IdManagerWinService::IdManagerWinService()
{
	//set opensoap server process name
	execProgName = "IdManager";
	//set opensoap server process log file
	logFileName = OPENSOAP_TMP_PATH + "log/idManager";
	//set opensoap server process initial argument
	extParm = "";
}

LPTSTR IdManagerWinService::GetName()
{
	//set Windows Service Name
	return "OpenSOAPIdManager";
}

DWORD IdManagerWinService::ServiceInit(int argc, LPTSTR *argv)
{
	if (argc>1) {
		logFileName = argv[1];
	}
   return NO_ERROR;
}

// オブジェクトを作成する（1つだけ）
IdManagerWinService idManagerWinService;

