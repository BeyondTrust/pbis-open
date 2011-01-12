#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

#include "soapInterface4ap_dso.h"

/* Channal Manager library */
#include "ChannelDescriptor.h"
#include "MsgDrvChnlSelector.h"
#include "AppLogger.h"
#include "ProcessInfo.h"
#include "SrvErrorCode.h"

/* Common library */
#include "SrvConf.h"
#include "DataRetainer.h"
#include "ServerCommon.h"
#include "SOAPMessageFunc.h"

#include <OpenSOAP/OpenSOAP.h>

//#define DEBUG

using namespace OpenSOAP;
using namespace std;

AppLogger applog;

void SetProcessInfo()
{
	string hn=ProcessInfo::GetHostName();
	if (hn.length()<=0) {
		char buf[256];
		gethostname(buf,sizeof(buf));
		ProcessInfo::SetHostName(buf);
		ProcessInfo::SetProcessName("soapInterface4ap");
		applog.Update(NULL,"soapInterface4ap");
	}
}

int WriteLog(int level,const char * comment)
{
	applog.Write(level,comment);
	return 0;
}

int GetNewId(char* num)
{
#ifdef DEBUG
	AppLogger::Write(ERR_ERROR ,"soapinterface::GetNewId: in");
#endif /* DEBUG */

    //server configure
	SetProcessInfo();
    SrvConf* srvConf = new SrvConf();

    if (!srvConf) {
		applog.Write(APLOG_ERROR,"GetNewId:Trans I/F: SrvConf create failed");
        return EXIT_FAILURE;
    }

	DataRetainer dr(srvConf->getSoapMessageSpoolPath());
	dr.Create();

	string tmp;
	dr.GetId(tmp);
	strcpy(num, tmp.c_str());
	delete srvConf;
	
#ifdef DEBUG
	AppLogger::Write(ERR_ERROR ,"soapinterface::GetNewId: out");
#endif /* DEBUG */

	return 0;
}

int GetFileName(const char* num, char* name)
{
#ifdef DEBUGa
	AppLogger::Write(ERR_ERROR ,"soapinterface::GetFileName: in");
#endif /* DEBUG */

    //server configure
	SetProcessInfo();
    SrvConf* srvConf = new SrvConf();

    if (!srvConf) {
		applog.Write(APLOG_ERROR,"GetFileName:Trans I/F: SrvConf create failed");
        return EXIT_FAILURE;
    }

	DataRetainer dr(srvConf->getSoapMessageSpoolPath());

#ifdef DEBUG
	AppLogger::Write(ERR_ERROR ,"soapinterface::GetFileName: dr()");
#endif /* DEBUG */

	dr.SetId(num);

#ifdef DEBUG
	AppLogger::Write(ERR_ERROR ,"soapinterface::GetFileName: dr.SetId()");
#endif /* DEBUG */

	string tmp = dr.GetHttpBodyFileName();

#ifdef DEBUG
	AppLogger::Write(ERR_ERROR ,"soapinterface::GetFileName: dr.GetHttpBodyFileName");
#endif /* DEBUG */

	strcpy(name, tmp.c_str());
	delete srvConf;
	return 0;
}

int AddHttpHeader(const char* num, const char* key, const char* data)
{
#ifdef DEBUG
	AppLogger::Write(ERR_ERROR ,"soapinterface::AddHttpHeader: in");
#endif /* DEBUG */

    //server configure
	SetProcessInfo();
    SrvConf* srvConf = new SrvConf();

    if (!srvConf) {
		applog.Write(APLOG_ERROR
			,"AddHttpHeader:Trans I/F: SrvConf create failed");
        return EXIT_FAILURE;
    }

	DataRetainer dr(srvConf->getSoapMessageSpoolPath());

#ifdef DEBUG
	AppLogger::Write(ERR_ERROR ,"soapinterface::AddHttpHeader: end of constructor");
#endif /* DEBUG */

	dr.SetId(num);

#ifdef DEBUG
	AppLogger::Write(ERR_ERROR ,"soapinterface::AddHttpHeader: end of dr.SetId");
#endif /* DEBUG */
	
	dr.AddHttpHeaderElement(key, data);

	delete srvConf;

#ifdef DEBUG
	AppLogger::Write(ERR_ERROR ,"soapinterface::AddHttpHeader: out");
#endif /* DEBUG */
	return 0;
}
	
int GetHttpHeader(const char* num, const char* key, const char** data)
{
    //server configure
	SetProcessInfo();
    SrvConf* srvConf = new SrvConf();

    if (!srvConf) {
	applog.Write(APLOG_ERROR
			,"GetHttpHeader:Trans I/F: SrvConf create failed");
        return EXIT_FAILURE;
    }

	DataRetainer dr(srvConf->getSoapMessageSpoolPath());
	dr.SetId(num);
	string tmp;
	bool rc = dr.GetHttpHeaderElement(key, tmp);
	//true = success, false = failure

	int ret;
	if(rc==true) {
		*data = tmp.c_str();
		ret = EXIT_SUCCESS;
	}
	else
		ret = DSO_NO_MATCH;
	
	delete srvConf;
	return ret;	
}

int InvokeOpenSOAPServer(const char* req_num, char* res_num)
{
	SetProcessInfo();
	AppLogger::Write(APLOG_DEBUG5,"InvokeOpenSOAPServer");

	ChannelDescriptor chnlDesc;
    ChannelSelector* chnlSelector = new MsgDrvChnlSelector();
    
    if (!chnlSelector) {
		applog.Write(APLOG_ERROR,"Trans I/F: channel descriptor create failed");
        return EXIT_FAILURE;
    }

    if (0 != chnlSelector->open(chnlDesc)) {
		applog.Write(APLOG_ERROR,"Trans I/F: channel descriptor open failed");
		delete chnlSelector;
        return EXIT_FAILURE;
    }

	AppLogger::Write(APLOG_DEBUG5,"InvokeOpenSOAPServer: create MsgDrvChnlSelector");

    // write to MsgDrv
	string fileIDOfMsgFromClientMsg = req_num;  
    if (0 > chnlDesc.write(fileIDOfMsgFromClientMsg)) {
		AppLogger::Write(APLOG_ERROR,"Trans I/F: write to MsgDrv failed.");
		delete chnlSelector;
        return EXIT_FAILURE;
    }

	AppLogger::Write(APLOG_DEBUG5,"InvokeOpenSOAPServer: write to  MsgDrv");

	// read from MsgDrv
    string fileIDOfMsgFromMsgDrv;
    if (0 > chnlDesc.read(fileIDOfMsgFromMsgDrv)) {
		AppLogger::Write(APLOG_ERROR,"Trans I/F: read from MsgDrv failed.");
		delete chnlSelector;
        return EXIT_FAILURE;
    }

	AppLogger::Write(APLOG_DEBUG5,"InvokeOpenSOAPServer: read from MsgDrv");
	
    //close
    chnlSelector->close(chnlDesc);
    delete chnlSelector;
	strcpy(res_num, fileIDOfMsgFromMsgDrv.c_str());

	AppLogger::Write(APLOG_DEBUG5,"End of InvokeOpenSOAPServer");
	//AppLogger::Write(APLOG_DEBUG5, "InvokeOpenSOAPServer: res_num = %s", res_num);
	AppLogger::Write(APLOG_DEBUG5, res_num);
	
    return EXIT_SUCCESS;

}
int DeleteFiles(const char* num)
{
	
    //server configure
	SetProcessInfo();
    SrvConf* srvConf = new SrvConf();

    if (!srvConf) {
		applog.Write(ERR_ERROR,"GetFileName:Trans I/F: SrvConf create failed");
        return EXIT_FAILURE;
    }

	DataRetainer dr(srvConf->getSoapMessageSpoolPath());
	dr.SetId(num);
	dr.DeleteFiles();
	delete srvConf;
  
	return EXIT_SUCCESS;
}

int CheckLimitSizeMessage(long length, char* num)
{
	static char METHOD_LABEL[] = "CheckLimitSizeMessage";
    
	SetProcessInfo();
    SrvConf* srvConf = new SrvConf();

    if (!srvConf) {
		applog.Write(ERR_ERROR,"GetFileName:Trans I/F: SrvConf create failed");
        return EXIT_FAILURE;
    }

	long limit = srvConf->getLimitSOAPMessageSize();

	if(limit>=0 && length > limit) {
		AppLogger::Write(APLOG_WARN,"%s %s=(%d) %s=(%d)"
			,"received soap message size limit over."
			,"limit size", limit ,"message size",length);
		
		int ret = OpenSOAPInitialize(NULL);
		if (OPENSOAP_FAILED(ret)) {
			AppLogger::Write(ERR_ERROR,"%s%s=(%d)"
				,METHOD_LABEL, "OpenSOAPInitialize failed. code" ,ret);
		}
    	
		string retStr = makeLimitSizeOverMessage(limit);
        
		DataRetainer dr(srvConf->getSoapMessageSpoolPath());
		dr.SetId(num);
		dr.DeleteFiles();
		
		dr.SetSoapFault(retStr, "200");
		
		ret = OpenSOAPUltimate();
		if (OPENSOAP_FAILED(ret)) {
			AppLogger::Write(ERR_WARN,"%s%s=(%d)", METHOD_LABEL,
				"OpenSOAPUltimate failed. code", ret);
		}

		delete srvConf;
		return EXIT_FAILURE;
	}
						 
	delete srvConf;
	return EXIT_SUCCESS;
}
