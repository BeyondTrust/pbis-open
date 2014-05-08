/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Invoker.cpp,v $
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

//for stdio
#include <sys/wait.h>
//#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include "SOAPMessageFunc.h"

#include "Invoker.h"
#include "ServiceInvoker.h"
#include "ServiceInvokerFactory.h"

#include "ServerCommon.h"
#include "SSMLInfoFunc.h"
#include "FileIDFunc.h"
#include "DataRetainer.h"
#include "SrvConfAttrHandler.h"

//for log, exception
#include "AppLogger.h"
#include "ProcessInfo.h"
#include "Exception.h"
#include "SrvErrorCode.h"
#include "TraceLog.h"

using namespace std;
using namespace OpenSOAP;


//#define DEBUG
static string CLASS_SIG("Invoker");
extern TraceLog *tlog;

void 
Invoker::invoke(SoapMessage& request, SoapMessage& response)
{
#ifdef DEBUG
    static char METHOD_LABEL[] = "Invoker::invoke";
#endif //DEBUG

    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(METHOD_LABEL);
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate();
#endif //DEBUG

    ServiceInvokerFactory factory;
    ServiceInvoker* svInvoker = 
        factory.createServiceInvoker(methodName,
                                     nsUri,
                                     EXTERNAL_SERVICES,
                                     srvConf);

    try {
        //invoke
        svInvoker->invoke(request, response);
        delete svInvoker;
        //trace
        tlog_local.SetComment("SERVICE_INVOKER INVOKED");
        tlog_local.TraceUpdate();
    }
    catch (SoapException& se) {
        delete svInvoker;
        se.AddFuncCallStack();
#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with SoapException <<");
        tlog_local.TraceUpdate();
#endif //DEBUG
        throw se;
    }
    catch (Exception& e) {
        delete svInvoker;
        e.AddFuncCallStack();
#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with Exception <<");
        tlog_local.TraceUpdate();
#endif //DEBUG
        throw e;
    }
    catch (...) {
        delete svInvoker;
        AppLogger::Write(ERR_ERROR, "%s::%s: unknown exception caught",
                         CLASS_SIG.c_str(),
                         __func__);
#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with unknown exception <<");
        tlog_local.TraceUpdate();
#endif //DEBUG
        throw;
    }
#ifdef DEBUG
    //trace
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate();
#endif //DEBUG
}

// End of Invoker.cpp

