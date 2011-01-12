/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SSMLAttrMgr_main.cpp,v $
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
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

#include <fstream>
#include <string>
#include <signal.h>

#if !defined(NO_LOGGER)
#include "AppLogger.h"
#endif
#include "ServerCommon.h"
#include "XmlQuery.h"
#include "SSMLAttrMgr.h"
#include "PIDFactory.h"
#include "SrvConfAttrHandler.h"
#include "ProcessInfo.h"
#include "Exception.h"

using namespace OpenSOAP;
using namespace std;

extern void signal_term(int i){
	AppLogger::Write(LOG_NOTICE,"signal:term termination now");
	ProcessInfo::SetProcessStatus(PSTAT_WAITTERM);
}

int 
main(int argc, char** argv)
{
  ProcessInfo::SetProcessName(argv[0]);
  ProcessInfo::SetHostName(NULL);
#if defined(NO_LOGGER)
  string logFile;
  SrvConfAttrHandler* srvConf = new SrvConfAttrHandler();
  vector<string> values;
  string query = SERVER_CONF_TAG + "/log/path=?";
  if (0 > srvConf->queryXml(query, values)) {
    logFile = OPENSOAP_TMP_PATH + "log/ssmlAttrMgr";
  }
  else {
    logFile = values[0] + "ssmlAttrMgr.log";
  }
  delete srvConf;

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
  AppLogger::Write("SSMLAttrMgr_main start");
#endif

#if !defined(WIN32)
  // make it to daemon & write pid file

//  daemon(0, 1);

  ProcessInfo::AddThreadInfo();

  PIDFactory* pidFactory = new PIDFactory();
  if (!pidFactory->makePID(SSMLATTRMGR_PID_FILE)){
    delete pidFactory;
    AppLogger::Write(ERR_ERROR,"error !!: process duplication");
    return EXIT_FAILURE;
  }
  
#endif
  
  // set signal 
  signal(SIGTERM,signal_term);
  SSMLAttrMgr ssmlMgr;
  try{
    ssmlMgr.run();
  }
  catch(...){
    AppLogger::Write(ERR_ERROR,"unknown error !!: process down!!"); 
  }

	AppLogger::Write("SSMLAttrMgr_main wait termination");
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
  AppLogger::Write("SSMLAttrMgr_main stop");
#endif
  ProcessInfo::DelThreadInfo();

  delete pidFactory;

  return 0;
}
    
  
