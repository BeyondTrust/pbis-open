/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SSMLAttrMgrWinService.cpp,v $
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
#include "SSMLAttrMgrWinService.h"

#include <iostream>
using namespace std;

using namespace OpenSOAP;

SSMLAttrMgrWinService::SSMLAttrMgrWinService()
{
	//set opensoap server process name
	execProgName = "ssmlAttrMgr";
	//set opensoap server process log file
	logFileName = OPENSOAP_TMP_PATH + "log/ssmlAttrMgr";
	//set opensoap server process initial argument
	extParm = "";
}

LPTSTR SSMLAttrMgrWinService::GetName()
{
	//set Windows Service Name
	return "OpenSOAPSSMLAttrMgr";
}

DWORD SSMLAttrMgrWinService::ServiceInit(int argc, LPTSTR *argv)
{
	if (argc>1) {
		logFileName = argv[1];
	}
   return NO_ERROR;
}

// オブジェクトを作成する（1つだけ）
SSMLAttrMgrWinService ssmlAttrMgrWinService;

