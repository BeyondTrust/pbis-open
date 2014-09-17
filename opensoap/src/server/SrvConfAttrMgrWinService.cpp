/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SrvConfAttrMgrWinService.cpp,v $
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
#include "SrvConfAttrMgrWinService.h"

#include <iostream>
using namespace std;

using namespace OpenSOAP;

SrvConfAttrMgrWinService::SrvConfAttrMgrWinService()
{
	//set opensoap server process name
	execProgName = "srvConfAttrMgr";
	//set opensoap server process log file
	logFileName = OPENSOAP_TMP_PATH + "log/srvConfAttrMgr";
	//set opensoap server process initial argument
	extParm = "";
}

LPTSTR SrvConfAttrMgrWinService::GetName()
{
	//set Windows Service Name
	return "OpenSOAPSrvConfAttrMgr";
}

DWORD SrvConfAttrMgrWinService::ServiceInit(int argc, LPTSTR *argv)
{
	if (argc>1) {
		logFileName = argv[1];
	}
   return NO_ERROR;
}

// オブジェクトを作成する（1つだけ）
SrvConfAttrMgrWinService srvConfAttrMgrWinService;

