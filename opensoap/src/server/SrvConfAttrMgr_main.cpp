/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SrvConfAttrMgr_main.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if defined(WIN32)
#include <windows.h>
#include <io.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <signal.h>

#include <string>
#include <fstream>
#if !defined(NO_LOGGER)
#include "AppLogger.h"
#endif
#include "ServerCommon.h"
#include "XmlQuery.h"
#include "SrvConfAttrMgr.h"
#include "PIDFactory.h"
#include "ProcessInfo.h"
#include "Exception.h"

using namespace OpenSOAP;
using namespace std;

extern void signal_term(int i){
	AppLogger::Write(LOG_NOTICE,"signal:term termination now");
	ProcessInfo::SetProcessStatus(PSTAT_WAITTERM);
}

int main(int argc, char** argv)
{
  ProcessInfo::SetProcessName(argv[0]);
  ProcessInfo::SetHostName(NULL);
  SrvConfAttrMgr srvConfAttrMgr;

#if defined(NO_LOGGER)

	string logFile = srvConfAttrMgr.getLogPath() + "srvConfAttrMgr.log";

	FILE* outfp = fopen(logFile.c_str(), "w");
	if (!outfp) {
		return 1;
	}
	if (argc <=1) {
		fprintf(outfp, "invalid initial argument. argc=(%d)\n", argc);
		fprintf(outfp, "exit.\n");
		fclose(outfp);
		return EXIT_FAILURE;
	}

	int old;
	int olderr;
	//backup stdout
	old = _dup(1);
	//stdout -> outfp
	if (-1 == _dup2(_fileno(outfp), 1)) {
		fprintf(outfp, "_dup2 to stdout failed.\n");
		fprintf(outfp, "exit.\n");
		fclose(outfp);
		return EXIT_FAILURE;
	}
	//backup stderr
	olderr = _dup(2);
	//stderr -> outfp
	if (-1 == _dup2(_fileno(outfp), 2)) {
		fprintf(outfp, "_dup2 to stderr failed.\n");
		fprintf(outfp, "exit.\n");
		fclose(outfp);
		return EXIT_FAILURE;
	}
#else
  // set log level  
  AppLogger logger(NULL,ProcessInfo::GetProcessName().c_str());
  AppLogger::Write("SrvConfAttrMgr_main start");
#endif

#if !defined(WIN32)
  // make it to daemon & write pid file

//  daemon(0, 1);

  ProcessInfo::AddThreadInfo();

  PIDFactory* pidFactory = new PIDFactory();
  if (!pidFactory->makePID(SRVCONFATTRMGR_PID_FILE)){
    delete pidFactory;
    AppLogger::Write(ERR_ERROR,"error !!: process duplication");
    return EXIT_FAILURE;
  }

#endif

  // set signal 
  signal(SIGTERM,signal_term);
//  SrvConfAttrMgr srvConfAttrMgr;
  try{
	  srvConfAttrMgr.run();
  }
  catch(Exception e){
    AppLogger::Write(e); 
  }
  catch(...){
    AppLogger::Write(ERR_ERROR,"unknown error !!: process down!!"); 
  }

	AppLogger::Write("SrvConfAttrMgr_main wait termination");
	while(ProcessInfo::GetThreadCount()>1) {
		sleep(1);
		ProcessInfo::CheckThreadInfo();// thread check
	}
#if defined(NO_LOGGER)
	fclose(outfp);
	_dup2(old, 1);
	_dup2(olderr, 2);
#else
  //write stop message
  AppLogger::Write("SrvConfAttrMgr_main stop");
#endif
  ProcessInfo::DelThreadInfo();

  delete pidFactory;

  return 0;
}

