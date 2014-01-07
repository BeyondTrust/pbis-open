/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: QueueManager_main.cpp,v $
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

#include <string>
#include <fstream>
#include <signal.h>

#if !defined(NO_LOGGER)
#include "AppLogger.h"
#endif
#include "ServerCommon.h"

#include "QueueManager.h"
#include "PIDFactory.h"
#include "SrvConfAttrHandler.h"
#include "ProcessInfo.h"
#include "Exception.h"
#include "SrvErrorCode.h"

#include "TraceLog.h"

using namespace std;
using namespace OpenSOAP;

TraceLog		*tlog=NULL;
PIDFactory		*pidFactory;

extern void signal_term(int i)
{
	AppLogger::Write(LOG_NOTICE,"signal:term termination now");
	ProcessInfo::SetProcessStatus(PSTAT_WAITTERM);

//	delete tlog;
//	delete pidFactory;

//	exit(0);
}

int 
main(int argc, char* argv[])
{
    // if argument exists, it behaves as Queue Manager of Forwarder
    bool isFwdQueue = false;
    int	i;
    
    ProcessInfo::SetProcessName(argv[0]);
    ProcessInfo::SetHostName(NULL);

#if defined(WIN32)
    if (argc > 2 && (atoi(argv[2]) != 0)) {
        isFwdQueue = true;
    }
#else
//  if (argc > 1 && (atoi(argv[1]) != 0)) {
//    isFwdQueue = true;
//  }
    for (i = 1; i < argc; i++){
      if (strcmp(argv[i], "fwd") == 0){
        isFwdQueue = true;
	break;
      }
    }
#endif

    //trace
    key_t	root_key;
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

    if (isFwdQueue) {
        tlog = new TraceLog(root_key, "queueManager_fwd");
    }
    else {
        tlog = new TraceLog(root_key, "queueManager_req");
    }


#if defined(NO_LOGGER)
  string logFile;
  SrvConfAttrHandler* srvConf = new SrvConfAttrHandler();
  vector<string> values;
  string query = SERVER_CONF_TAG + "/log/path=?";
  if (0 > srvConf->queryXml(query, values)) {
    if (isFwdQueue) {
      logFile = OPENSOAP_TMP_PATH + "log/queueManager_fwd";
    }
    else {
      logFile = OPENSOAP_TMP_PATH + "log/queueManager_req";
    }
  }
  else {
    if (isFwdQueue) {
      logFile = values[0] + "queueManager_fwd.log";
    }
    else {
      logFile = values[0] + "queueManager_req.log";
    }
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
  if (isFwdQueue) {
    AppLogger::Write("QueueManger_main (fwd) start");
  }
  else {
    AppLogger::Write("QueueManger_main (req) start");
  }
#endif

#if !defined(WIN32)
  // make it to daemon & write pid file

//  daemon(0, 1);

  ProcessInfo::AddThreadInfo();

  pidFactory = new PIDFactory();
  if (!pidFactory->makePID(isFwdQueue ? FWDQUEUE_PID_FILE : REQQUEUE_PID_FILE)){
    delete pidFactory;
    delete tlog;
    AppLogger::Write(ERR_ERROR,"error !!: process duplication");
    return EXIT_FAILURE;
  }
  
#endif

  // register signal
  signal(SIGTERM,signal_term);
  // create instance of QueueManager and run
  ChannelManager* queueManagerPtr = new QueueManager(isFwdQueue);
  try{
    queueManagerPtr->run();
  }
  catch(...){
    AppLogger::Write(ERR_ERROR,"unknown error !!: process down!!"); 
  }

  if (isFwdQueue){
    AppLogger::Write("QueueManager_main (fwd) wait termination");
  }
  else{
    AppLogger::Write("QueueManager_main (req) wait termination");
  }
  while(ProcessInfo::GetThreadCount()>1){
	  sleep(1);
	  ProcessInfo::CheckThreadInfo();
  }

  delete queueManagerPtr;

#if defined(NO_LOGGER)
	fclose(outfp);
	_dup2(old, 1);
	_dup2(olderr, 2);
#else
  //write stop message
  if (isFwdQueue) {
    AppLogger::Write("QueueManger_main (fwd) stop");
  }
  else {
    AppLogger::Write("QueueManger_main (req) stop");
  }
//  Logger::term();
#endif
  ProcessInfo::DelThreadInfo();

  delete tlog;
  delete pidFactory;

  return 0;
}

