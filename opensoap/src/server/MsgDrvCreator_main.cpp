/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MsgDrvCreator_main.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <iostream>
#if defined(WIN32)
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include <fstream>
#include <string>
#include <signal.h>

#include "ServerCommon.h"
#include "MsgDrvCreator.h"
#if !defined(NO_LOGGER)
#include "AppLogger.h"
#endif
#include "PIDFactory.h"
#include "SrvConfAttrHandler.h"
#include "ProcessInfo.h"
#include "Exception.h"
#include "SrvErrorCode.h"

#include "TraceLog.h"

using namespace std;
using namespace OpenSOAP;

TraceLog		*tlog=NULL;

extern void signal_term(int i){
	AppLogger::Write(LOG_NOTICE,"signal:term termination now");
	ProcessInfo::SetProcessStatus(PSTAT_WAITTERM);
}

int
main(int argc, char** argv) {
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

  tlog = new TraceLog(root_key, "msgDrvCreator");
  
  ProcessInfo::SetProcessName(argv[0]);
  ProcessInfo::SetHostName(NULL);

#if defined(NO_LOGGER)
  string logFile;
  SrvConfAttrHandler* srvConf = new SrvConfAttrHandler();
  vector<string> values;
  string query = SERVER_CONF_TAG + "/log/path=?";
  if (0 > srvConf->queryXml(query, values)) {
    logFile = OPENSOAP_TMP_PATH + "log/msgDrvCreator";
  }
  else {
    logFile = values[0] + "msgDrvCreator.log";
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
  AppLogger logger(NULL,ProcessInfo::GetProcessName().c_str());
  AppLogger::Write("MsgDrvCreator_main start");
#endif

#if !defined(WIN32)
  // make it to daemon & write pid file

//  daemon(0, 1);

  ProcessInfo::AddThreadInfo();
  PIDFactory* pidFactory = new PIDFactory();
  if (!pidFactory->makePID(MSGDRV_PID_FILE)){
    delete pidFactory;
    delete tlog;
    AppLogger::Write(ERR_ERROR,"error !!: process duplication");
    return EXIT_FAILURE;
  }

#endif

  // set signal 
  signal(SIGTERM,signal_term);

  // create instance of Message Driver and run  
  ChannelManager* chnlMngr = new MsgDrvCreator();
  chnlMngr->setMask(0x000);
  //chnlMngr->setMask(0x077);
  try{
      chnlMngr->run();
  }
  catch(Exception e){
    e.AddFuncCallStack();
    AppLogger::Write(e); 
  }
  catch(...){
    AppLogger::Write(ERR_ERROR,"unknown error !!: process down!!"); 
  }
  
	AppLogger::Write("MsgDrvCreator_main wait termination");
	while(ProcessInfo::GetThreadCount()>1) {
		sleep(1);
		ProcessInfo::CheckThreadInfo();// thread check
	}
  delete chnlMngr;

#if defined(NO_LOGGER)
	fclose(outfp);
	_dup2(old, 1);
	_dup2(olderr, 2);
#else
  AppLogger::Write("MsgDrvCreator_main stop");
#endif

  ProcessInfo::DelThreadInfo();

  delete tlog;
  delete pidFactory;

  return EXIT_SUCCESS;
}
