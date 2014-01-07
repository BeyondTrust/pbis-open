/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MsgDrvCreatorWinService.cpp,v $
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
#include "MsgDrvCreatorWinService.h"

#include <iostream>
using namespace std;

using namespace OpenSOAP;

MsgDrvCreatorWinService::MsgDrvCreatorWinService()
{
	//set opensoap server process name
	execProgName = "msgDrvCreator";
	//set opensoap server process log file
	logFileName = OPENSOAP_TMP_PATH + "log/msgDrvCreator";
	//set opensoap server process initial argument
	extParm = "";
}

LPTSTR MsgDrvCreatorWinService::GetName()
{
	//set Windows Service Name
	return "OpenSOAPMsgDrvCreator";
}

DWORD MsgDrvCreatorWinService::ServiceInit(int argc, LPTSTR *argv)
{
	if (argc>1) {
		logFileName = argv[1];
	}
   return NO_ERROR;
}

// オブジェクトを作成する（1つだけ）
MsgDrvCreatorWinService msgDrvCreatorWinService;

