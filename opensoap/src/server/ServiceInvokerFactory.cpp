/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ServiceInvokerFactory.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include "ServiceInvokerFactory.h"
#include "ServiceInvoker.h"
#include "HttpServiceInvoker.h"
#include "StdIOServiceInvoker.h"

#include "SSMLInfo.h"
#include "SSMLInfoFunc.h"

#include "AppLogger.h"
#include "ProcessInfo.h"
#include "Exception.h"
#include "SrvErrorCode.h"
#include "TraceLog.h"

using namespace std;
using namespace OpenSOAP;

static string CLASS_SIG("ServiceInvokerFactory");
extern TraceLog		*tlog;

ServiceInvokerFactory::ServiceInvokerFactory()
{
}

ServiceInvokerFactory::~ServiceInvokerFactory()
{
}

ServiceInvoker* 
ServiceInvokerFactory::createServiceInvoker(const string& methodName,
                                            const string& methodNs,
                                            const SSMLType ssmlType,
                                            SrvConf& srvConf)
{
    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(CLASS_SIG + __func__);
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate();
#endif //DEBUG

    ServiceInvoker* svInvoker = NULL;
    try {
        SSMLInfo ssmlInfo;
        //set pre condition
        ssmlInfo.setOperationName(methodName);
        ssmlInfo.setNamespace(methodNs);
        ssmlInfo.setSSMLType(ssmlType);
        
        //extract SSML data
        bool operationExist = getSSMLInfo(ssmlInfo);
        
        //check SSML data
        string connectionType = ssmlInfo.getConnectionMethod();
        
        AppLogger::Write(ERR_DEBUG, "%s::%s: operationExist=(%d)",
                         CLASS_SIG.c_str(),
                         __func__,
                         operationExist);
        AppLogger::Write(ERR_DEBUG, "%s::%s: connectionType=[%s]",
                         CLASS_SIG.c_str(),
                         __func__,
                         connectionType.c_str());
        
        if (connectionType == SRV_CONNCT_HTTP) {
            HttpServiceInvoker* invoker = new HttpServiceInvoker(srvConf);
            if (!invoker) {
                //
                Exception e(-1, OPENSOAP_MEM_BADALLOC,
                            ERR_ERROR, __FILE__, __LINE__);
            }
            invoker->setEndPointUrl(ssmlInfo.getEndPointUrl());
            svInvoker = invoker;
            
            AppLogger::Write(ERR_DEBUG, "%s::%s: HttpService EndPoint=[%s]",
                             CLASS_SIG.c_str(),
                             __func__,
                             ssmlInfo.getEndPointUrl().c_str());
        }
        else if (connectionType == SRV_CONNCT_STDIO) {
            StdIOServiceInvoker* invoker = new StdIOServiceInvoker(srvConf);
            if (!invoker) {
                //
                Exception e(-1, OPENSOAP_MEM_BADALLOC,
                            ERR_ERROR, __FILE__, __LINE__);
            }
            invoker->setExecProg(ssmlInfo.getExecProg());
            svInvoker = invoker;
            
            AppLogger::Write(ERR_DEBUG, "%s::%s: StdIOService ExecProg=[%s]",
                             CLASS_SIG.c_str(),
                             __func__,
                             ssmlInfo.getExecProg().c_str());
        }
        else if (connectionType == SRV_CONNCT_SOCKET) {
            //not implemented
            Exception e(-1, OPENSOAP_YET_IMPLEMENTATION, 
                        ERR_ERROR, __FILE__, __LINE__);
            e.SetErrText("service socket connection type not implemented");
            throw e;
        }
        else {
            Exception e(-1, OPENSOAP_YET_IMPLEMENTATION, 
                        ERR_ERROR, __FILE__, __LINE__);
            e.SetErrText("not implemented unknown service connection type");
            throw e;
        }
        //common resource
        svInvoker->setTimeoutPeriod(ssmlInfo.getSyncTTL());
    }
    catch (Exception e) {
        e.AddFuncCallStack();
        AppLogger::Write(e);
        if (svInvoker) {
            delete svInvoker;
        }
#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with Exception <<");
        tlog_local.TraceUpdate();
#endif //DEBUG
        throw e;
    }
    catch (...) {
        AppLogger::Write(ERR_ERROR, "unknown error");
        if (svInvoker) {
            delete svInvoker;
        }
        //exception
        Exception e(-1, OPENSOAPSERVER_UNKNOWN_ERROR,
                    ERR_ERROR, __FILE__, __LINE__);
        throw e;
    }
#ifdef DEBUG
    //trace
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate();
#endif //DEBUG

    return svInvoker;
}

// End of ServiceInvokerFactor.cpp

