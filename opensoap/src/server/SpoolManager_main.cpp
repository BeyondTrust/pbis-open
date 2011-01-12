/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SpoolManager_main.cpp,v $
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
#include <unistd.h>
#endif

#include <fstream>
#include <iostream>
#include <string>
#include <signal.h>

#if !defined(NO_LOGGER)
#include "AppLogger.h"
#endif
#include "ServerCommon.h"
#include "SpoolManager.h"
#include "PIDFactory.h"
#include "SrvConfAttrHandler.h"
#include "ProcessInfo.h"
#include "Exception.h"
#include "SrvErrorCode.h"

#include "TraceLog.h"

using namespace OpenSOAP;
using namespace std;

TraceLog		*tlog=NULL;
PIDFactory		*pidFactory;

extern void signal_term(int i)
{
	AppLogger::Write(LOG_NOTICE, "signal:term termination now");
	ProcessInfo::SetProcessStatus(PSTAT_WAITTERM);

//	delete tlog;
//	delete pidFactory;

//	exit(0);
}

int
main (int argc, char* argv[])
{
  key_t		root_key;
  int		ret_opt;

  root_key = 0;
  while ((ret_opt = getopt(argc, argv, "k:")) > 0){
    switch(ret_opt){
      case 'k' :
        root_key = atoi(optarg);
        break;
      default :
        break;
    }
  }

  tlog = new TraceLog(root_key, "spoolManager");

  ProcessInfo::SetProcessName(argv[0]);
  ProcessInfo::SetHostName(NULL);
#if defined(NO_LOGGER)
	string logFile;
	SrvConfAttrHandler* srvConf = new SrvConfAttrHandler();
	vector<string> values;
	string query = SERVER_CONF_TAG + "/log/path=?";
	if (0 > srvConf->queryXml(query, values)) {
		logFile = OPENSOAP_TMP_PATH + "log/spoolManager";
	}
	else {
		logFile = values[0] + "spoolManager.log";
	}
	delete srvConf;

	FILE* outfp = fopen(logFile.c_str(), "w");
	if (!outfp) {
		delete tlog;
		return 1;
	}
	if (argc <=1) {
		fprintf(outfp, "invalid initial argument. argc=(%d)\n", argc);
		fprintf(outfp, "exit.\n");
		fclose(outfp);
		delete tlog;
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
		delete tlog;
		return EXIT_FAILURE;
	}
	//backup stderr
	olderr = _dup(2);
	//stderr -> outfp
	if (-1 == _dup2(_fileno(outfp), 2)) {
		fprintf(outfp, "_dup2 to stderr failed.\n");
		fprintf(outfp, "exit.\n");
		fclose(outfp);
		delete tlog;
		return EXIT_FAILURE;
	}
#else
  // set log level  
  //AppLogger logger;
  AppLogger logger(NULL,ProcessInfo::GetProcessName().c_str());
  AppLogger::Write("SpoolManger_main start");
#endif

#if !defined(WIN32)
  // make it to daemon & write pid file

//  daemon(0, 1);
  ProcessInfo::AddThreadInfo();


  pidFactory = new PIDFactory();
  if (!pidFactory->makePID(RESSPOOL_PID_FILE)){
    delete pidFactory;
    AppLogger::Write(ERR_ERROR,"error !!: process duplication");
    return EXIT_FAILURE;
  }
  
#endif

  // register signal
  signal(SIGTERM,signal_term);
  // create instance of Spool Manager and run
  ChannelManager* spoolManagerPtr= new SpoolManager();
  try{
    spoolManagerPtr->run();
  }
  catch(...){
    AppLogger::Write(ERR_ERROR,"unknown error !!: process down!!"); 
  }

  AppLogger::Write("SpoolManager_main wait termination");
  while(ProcessInfo::GetThreadCount()>1){
	  sleep(1);
	  ProcessInfo::CheckThreadInfo();
  }

  delete spoolManagerPtr;
  
#if defined(NO_LOGGER)
	fclose(outfp);
	_dup2(old, 1);
	_dup2(olderr, 2);
#else
  //write stop message
  AppLogger::Write("SpoolManger_main stop");
#endif
  ProcessInfo::DelThreadInfo();

  delete tlog;
  delete pidFactory;

  return 0;
}// end of main (int, char**)
