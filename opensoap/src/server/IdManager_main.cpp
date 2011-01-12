/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: IdManager_main.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#if defined(WIN32)
#include <windows.h>
#include <io.h>
#include <process.h>
#include <time.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

#include <signal.h>
//#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>

#include "IdManager.h"

#if !defined(NO_LOGGER)
#include "AppLogger.h"
#endif
#include "ServerCommon.h"
#include "IdManager.h"
#include "PIDFactory.h"
#include "SrvConfAttrHandler.h"
#include "ProcessInfo.h"
#include "Exception.h"
#include "SrvErrorCode.h"

using namespace OpenSOAP;
using namespace std;

//for LOG
static const std::string CLASS_SIG = "IdManager";

extern void signal_term(int i){
	AppLogger::Write(LOG_NOTICE,"signal:term termination now");
	ProcessInfo::SetProcessStatus(PSTAT_WAITTERM);
}

int 
main(int argc, char* argv[])
{
  int i=0;
  ProcessInfo::SetProcessName(argv[0]);
  ProcessInfo::SetHostName(NULL);
#if defined(NO_LOGGER)
  string logFile;
  SrvConfAttrHandler* srvConf = new SrvConfAttrHandler();
  vector<string> values;
  string query = SERVER_CONF_TAG + "/log/path=?";
  if (0 > srvConf->queryXml(query, values)) {
    logFile = OPENSOAP_TMP_PATH + "log/idManager";
  }
  else {
    logFile = values[0] + "idManager.log";
  }
  delete srvConf;

  FILE* outfp = fopen(logFile.c_str(), "w");
  if (!outfp) {
    cerr << "** E **: file open failed.[" << logFile << "]" << endl;
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
  AppLogger::Write("IdManager start");
#endif


#if !defined(WIN32)
  // make it to daemon & write pid file

//  daemon(0, 1);

  ProcessInfo::AddThreadInfo();

  PIDFactory* pidFactory = new PIDFactory();
  if (!pidFactory->makePID(IDMGR_PID_FILE)){
    delete pidFactory;
    AppLogger::Write(ERR_ERROR, "error !!: process duplication");
    return EXIT_FAILURE;
  }

#endif

  // set signal 
  signal(SIGTERM,signal_term);
  // create instance of IdManager & run
  // 3 times retry
  AppLogger::Write("IdManager_main wait termination");
  for (i=0 ; i<3 && ProcessInfo::GetProcessStatus()!=PSTAT_WAITTERM ; i++){
    IdManager idMngr;
    try{
      idMngr.run();
    }
    catch(Exception e){
      AppLogger::Write(e); 
    }
    catch(...){
      AppLogger::Write(ERR_ERROR,"unknown error !!: process down!!"); 
    }
  }

#if defined(NO_LOGGER)
  fclose(outfp);
  _dup2(old, 1);
  _dup2(olderr, 2);
#else
  //write stop message
  AppLogger::Write("IdManager_main stop");
#endif
  ProcessInfo::DelThreadInfo();

  delete pidFactory;

  return EXIT_SUCCESS;
}
