/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: StdIOServiceInvoker.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Transport.h>

#include <OpenSOAP/CStdio.h>
#include <string>
#include <sys/stat.h>

//for debug
#include <sstream>
#include <typeinfo>

//for stdio
#include <sys/wait.h>
//#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include "SOAPMessageFunc.h"
#include "StringUtil.h"
#include "StdIOServiceInvoker.h"

#include "ServerCommon.h"
#include "SSMLInfoFunc.h"
#include "FileIDFunc.h"
#include "DataRetainer.h"
#include "SrvConfAttrHandler.h"
#include "SoapMessageSerializer.h"

//for log, exception
#include "AppLogger.h"
#include "ProcessInfo.h"
#include "Exception.h"
#include "SrvErrorCode.h"
#include "TraceLog.h"

using namespace std;
using namespace OpenSOAP;


//#define DEBUG
static string CLASS_SIG("StdIOServiceInvoker");
extern TraceLog *tlog;

//------------------------------------------------------
// StdIO Invoker
//------------------------------------------------------
void
StdIOServiceInvoker::invokeImpl(SoapMessage* request, SoapMessage* response)
{
    static char METHOD_LABEL[] = "StdIOServiceInvoker::invokeImpl:";
    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(CLASS_SIG + __func__);
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG

    //pipe read/write index
    const int R = 0;
    const int W = 1;

    int pipe_c2p[2];
    int pipe_p2c[2];
    int pid = -1;

    int stat = 0;
  
    if (pipe(pipe_c2p) < 0) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "create pipe 1 failed.");

#if 0
        response = createSOAPFaultMessage("Server",
                                          "I/O Error",
                                          myselfUrl_,
                                          "create pipe failed");
#endif
        return;
    }
    if (pipe(pipe_p2c) < 0) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "create pipe 2 failed.");
        
#if 0
        response = createSOAPFaultMessage("Server",
                                          "I/O Error",
                                          myselfUrl_,
                                          "create pipe failed");
#endif
        close(pipe_c2p[R]);
        close(pipe_c2p[W]);

        return;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5, "%s %s=(%d)(%d)",
                     METHOD_LABEL,
                     "+++ pipe_p2c",
                     pipe_p2c[R],
                     pipe_p2c[W]);
    AppLogger::Write(APLOG_DEBUG5, "%s %s=(%d)(%d)",
                     METHOD_LABEL,
                     "+++ pipe_p2c",
                     pipe_c2p[R],
                     pipe_c2p[W]);
#endif

    //create service process
    if ((pid = fork()) < 0) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "can't fork child process.");

#if 0
        response = createSOAPFaultMessage("Server",
                                          "Process Error",
                                          myselfUrl_,
                                          "service process fork failed");
#endif

        close(pipe_c2p[R]);
        close(pipe_c2p[W]);
        close(pipe_p2c[R]);
        close(pipe_p2c[W]);

        return;
    }

    //child process side
    if (0 == pid) {
        close(pipe_p2c[W]);
        close(pipe_c2p[R]);
        dup2(pipe_p2c[R], 0);
        dup2(pipe_c2p[W], 1);
        close(pipe_p2c[R]);
        close(pipe_c2p[W]);

        if (execlp("sh", "sh", "-c", execProg.c_str(), NULL) < 0) {
            AppLogger::Write(APLOG_ERROR, "%s %s : %s",
                             METHOD_LABEL,
                             "can't exec prog : ",
                             execProg .c_str());

            close(pipe_p2c[R]);
            close(pipe_c2p[W]);
            _exit(1);
        }
        _exit(127);
        // end child process
    }
    
    int status = 0;
    
    close(pipe_p2c[R]);
    close(pipe_c2p[W]);
    
    int wfd = pipe_p2c[W];
    int rfd = pipe_c2p[R];

    // send to service

    // attach HTTP header for service

    string charEnc = srvConf.getDefaultCharEncoding();

    //request message
    string reqStr = request->toString();
    string resStr;

    string msgToService = 
        attachHeaderForService(reqStr, charEnc);

    //------------------------------------------------
    // read resStr from service
    //------------------------------------------------
    
    // message send to service.
    int writeLen = OpenSOAP::write(wfd, msgToService);

    if (writeLen <= 0) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "write failed to Service");

#if 0        
        resStr = createSOAPFaultMessage("Server",
                                          "I/O Error",
                                          myselfUrl_,
                                          "write to service stdin failed");
#endif

        close(pipe_p2c[W]);
        close(pipe_c2p[R]);

        Exception e(-1, OPENSOAPSERVER_NETWORK_ERROR,
                    ERR_ERROR, __FILE__, __LINE__);
        e.SetErrText("write failed");
#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with Exception <<");
        tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG

        throw e;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5, "%s %s=[%s]",
                     METHOD_LABEL,
                     "WRITE TO StdioSrv",
                     msgToService.c_str());
#endif //DEBUG

    //------------------------------------------------
    // read resStr from service
    //------------------------------------------------
    
    //int readLen = OpenSOAP::read(rfd, resStr);
    int readLen = OpenSOAP::read2(rfd, resStr);
    
    //parent process wait for SIGCHLD from child process
    int childProcessWaitStatus = int();
    pid_t exitPid = wait(&childProcessWaitStatus);
    if (0 >= exitPid) {
        //error occured
        //perror("wait");
        AppLogger::Write(APLOG_ERROR, "%s %s ret=(%d)",
                         METHOD_LABEL,
                         "wait invalid child process exit status.",
                         exitPid);

        Exception e(-1, OPENSOAPSERVER_FORK_ERROR,
                    ERR_ERROR, __FILE__, __LINE__);
        e.SetErrText("wait child process failed");
#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with Exception <<");
        tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG

        throw e;
    }
#ifdef DEBUG
    else {
        AppLogger::Write(APLOG_DEBUG, "%s %s status=(%d) pid=(%d)",
                         METHOD_LABEL,
                         "wait child process exit done.",
                         childProcessWaitStatus,
                         exitPid);
    }
#endif //DEBUG

    if (readLen < 0) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "read failed from SocketService");

#if 0
        resStr = 
            createSOAPFaultMessage("Server",
                                   "I/O Error",
                                   myselfUrl_,
                                   "read from service stdout failed");
#endif
        
        close(pipe_p2c[W]);
        close(pipe_c2p[R]);

        return ;
    }
    
    close(pipe_p2c[W]);
    close(pipe_c2p[R]);
    //wait(&status);

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG, "%s %s=[%s]",
                     METHOD_LABEL,
                     "READ From StdioSrv",
                     resStr.c_str());
#endif //DEBUG
    
    if (resStr.empty()) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "resStr from Service is Empty");
#if 0
        resStr = 
            createSOAPFaultMessage(
                "Server",
                "Service ResStr Is Empty",
                myselfUrl_,
                "invoke Stdio Service Return Empty");
#endif

        close(pipe_p2c[W]);
        close(pipe_c2p[R]);

        return;
    }

    string fromEnc;
    string toEnc = srvConf.getDefaultCharEncoding();
    if (getEncodingCharset(resStr, fromEnc/*, toEnc*/)) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG, "%s %s fromEnc=[%s] toEnc=[%s]",
                         METHOD_LABEL,
                         "getEncodingCharset: ",
                         fromEnc.c_str(),
                         toEnc.c_str());
#endif //DEBUG
        
        replaceXmlEncoding(resStr, toEnc);
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG, "%s %s=[%s]",
                         METHOD_LABEL,
                         "replaceXmlEncoding",
                         resStr.c_str());
#endif //DEBUG

        // convert enc : FROM Content-Type: charset To UTF-8
        convertEncoding(resStr, fromEnc, toEnc);
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG, "%s %s=[%s]",
                         METHOD_LABEL,
                         "convertEncoding",
                         resStr.c_str());
#endif //DEBUG
    }

    resStr = removeHeaderForService(resStr);
    

    DataRetainer responseDr(srvConf.getSoapMessageSpoolPath());
    responseDr.Create();
    string responseFile = responseDr.GetHttpBodyFileName();
    ofstream ofst(responseFile.c_str());
    ofst << resStr << endl;
    ofst.close();

    //temp.
    responseDr.AddHttpHeaderElement("content-type", "text/xml;");
    //
    //responseDr.Decompose();

    string responseId;
    responseDr.GetSoapEnvelope(resStr);
    responseDr.GetId(responseId);

    response->deserialize(resStr);
    response->setMessageId(responseId);

    //
    responseDr.Compose();

#ifdef DEBUG
    //trace
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG
    
    return;
}


// End of StdIOServiceInvoker.cpp

