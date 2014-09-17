/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: soapInterface.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>

#include <fstream>

/* Channal Manager library */
#include "ChannelDescriptor.h"
#include "MsgDrvChnlSelector.h"

/* Common library */
#include "FileIDFunc.h"
#include "connection.h"
#include "SOAPMessageFunc.h"
#include "StringUtil.h"
#include "SrvConf.h"
#include "DataRetainer.h"

#include "ServerCommon.h"
#include "AppLogger.h"
#include "ProcessInfo.h"

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Service.h>

using namespace OpenSOAP;
using namespace std;

//#define DEBUG

#ifdef DEBUG
static
void
PrintEnvelope(OpenSOAPEnvelopePtr env,
              const char *label,
              const char *charEnc) 
{
    OpenSOAPByteArrayPtr envBuf = NULL;
    const unsigned char *envBeg = NULL;
    size_t envSz = 0;
    
    OpenSOAPByteArrayCreate(&envBuf);
    OpenSOAPEnvelopeGetCharEncodingString(env, charEnc, envBuf);
    OpenSOAPByteArrayGetBeginSizeConst(envBuf, &envBeg, &envSz);
    
    fprintf(stderr, "\n=== %s envelope begin ===\n", label);
    fwrite(envBeg, 1, envSz, stderr);
    fprintf(stderr, "\n=== %s envelope  end  ===\n", label);
    
    OpenSOAPByteArrayRelease(envBuf);
}
#endif //DEBUG

#include <sys/stat.h>

static
int
ServiceFunc(OpenSOAPEnvelopePtr request,
            OpenSOAPEnvelopePtr *response,
            void *opt) 
{
    int ret = OPENSOAP_NO_ERROR;
    
    OpenSOAPByteArrayPtr envBuf = NULL;
    const unsigned char *envBeg = NULL;
    size_t envSz = 0;

    //server configure
    SrvConf* srvConf = new SrvConf();

#if 0 //temp.
    mode_t mk = 0x202;
    mode_t curmk = umask(mk);
    //fprintf(stderr, "== curmk=[%x] ==\n", curmk);
#endif    

    //message/id converter
    DataRetainer* dataRetainer 
        = new DataRetainer(srvConf->getSoapMessageSpoolPath());
    
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"SrvConf::getSoapMessageSpoolPath"
                    ,srvConf->getSoapMessageSpoolPath().c_str());
#endif

    ret = OpenSOAPByteArrayCreate(&envBuf);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s %s %s=(%d)"
                        ,"Read from CGI:"
                        ,"OpenSOAPByteArrayCreate() failed."
                        ,"ErrorCode",ret);
    }

    string charEnc = srvConf->getDefaultCharEncoding();

#ifdef DEBUG
    PrintEnvelope(request, "Trans I/F request", charEnc.c_str());
#endif //DEBUG

    ret = OpenSOAPEnvelopeGetCharEncodingString(request, 
                                                charEnc.c_str(), 
                                                envBuf);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s %s %s=(%d)"
                        ,"Read from CGI:"
                        ,"OpenSOAPEnvelopeGetCharEncodingString() failed."
                        ,"ErrorCode",ret);
    }
    ret = OpenSOAPByteArrayGetBeginSizeConst(envBuf, &envBeg, &envSz);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s %s %s=(%d)"
                        ,"Read from CGI:"
                        ,"OpenSOAPByteArrayGetBeginSizeConst() failed."
                        ,"ErrorCode",ret);
    }
    string soapMsgFromClient((const char*)envBeg, envSz);

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s %s=[%s]"
                    ,"Tran I/F:","from",soapMsgFromClient.c_str());
    AppLogger::Write(APLOG_DEBUG5,"%s %s=(%d)"
                    ,"Tran I/F:","envSz",envSz);
    AppLogger::Write(APLOG_DEBUG5,"%s %s=(%d)"
                    ,"Tran I/F:","length",soapMsgFromClient.length());
#endif

    ret = OpenSOAPByteArrayRelease(envBuf);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s %s %s=(%d)"
                        ,"Read from CGI:"
                        ,"OpenSOAPByteArrayRelease() failed."
                        ,"ErrorCode",ret);
    }

    //check message size
    long limitSize = srvConf->getLimitSOAPMessageSize();
    if ( limitSize >= 0 && soapMsgFromClient.length() > limitSize) {
        AppLogger::Write(APLOG_WARN,"%s %s=(%d) %s=(%d)"
                        ,"received soap message size limit over."
                        ,"limit size",limitSize
                        ,"message size",soapMsgFromClient.length());

        string retStr(makeLimitSizeOverMessage(limitSize));

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG5,"%s[%s]"
                        ,"FaultMessage::LIMIT OVER:",retStr.c_str());
#endif

        // convert Envelope -> string
        ret = OpenSOAPByteArrayCreateWithData((const unsigned char*)
                                              retStr.c_str(),
                                              retStr.length(),
                                              &envBuf);
        if (OPENSOAP_FAILED(ret)) {
            AppLogger::Write(APLOG_ERROR,"%s %s %s=(%d)"
                            ,"LimitOverFaultMessage:"
                            ,"OpenSOAPByteArrayCreateWithData() failed."
                            ,"ErrorCode",ret);
        }
#ifdef DEBUG
        else {
            AppLogger::Write(APLOG_DEBUG5,"%s %s"
                            ,"LimitOverFaultMessage:"
                            ,"OpenSOAPByteArrayCreateWithData() done.");
        }
#endif //DEBUG
	      
        ret = OpenSOAPEnvelopeCreateCharEncoding(charEnc.c_str(),//NULL,
                                                 envBuf,
                                                 response);
        if (OPENSOAP_FAILED(ret)) {
            AppLogger::Write(APLOG_ERROR,"%s %s %s=(%d)"
                            ,"LimitOverFaultMessage:"
                            ,"OpenSOAPEnvelopeCreateCharEncoding() failed."
                            ,"ErrorCode",ret);
        }
#ifdef DEBUG
        else {
            AppLogger::Write(APLOG_DEBUG5,"%s %s"
                            ,"LimitOverFaultMessage:"
                            ,"OpenSOAPEnvelopeCreateCharEncoding() done.");
            PrintEnvelope(*response, "Trans I/F response", charEnc.c_str());
        }
#endif //DEBUG

        ret = OpenSOAPByteArrayRelease(envBuf);
        if (OPENSOAP_FAILED(ret)) {
            AppLogger::Write(APLOG_ERROR,"%s %s %s=(%d)"
                            ,"LimitOverFaultMessage:"
                            ,"OpenSOAPByteArrayRelease() failed."
                            ,"ErrorCode",ret);
        }
#ifdef DEBUG
        else {
            AppLogger::Write(APLOG_DEBUG5,"%s %s"
                            ,"LimitOverFaultMessage:"
                            ,"OpenSOAPByteArrayRelease() done.");
        }
#endif //DEBUG

        // send fault message 
        return 0;
    }
  
    //get fileID
    dataRetainer->Create();
    string reqFile = dataRetainer->GetHttpBodyFileName();
    AppLogger::Write(APLOG_DEBUG,"%s=[%s]","### reqFile",reqFile.c_str());
    ofstream ofst(reqFile.c_str());
    ofst << soapMsgFromClient << endl;
    ofst.close();
    //dataRetainer->UpdateSoapEnvelope(soapMsgFromClient);
    string fileIDOfMsgFromClientMsg;
    dataRetainer->GetId(fileIDOfMsgFromClientMsg);
    AppLogger::Write(APLOG_DEBUG,"%s=[%s]"
                    ,"### id",fileIDOfMsgFromClientMsg.c_str());

#ifdef DEBUG_
    AppLogger::Write(APLOG_DEBUG5,"%s=[%d]"
                    ,"fileID fromClientToMsgDrv"
                    ,fileIDOfMsgFromClientMsg.c_str());
#endif
  
    //clean up
    soapMsgFromClient = ""; //.clear();

    //communication descriptor
    ChannelDescriptor chnlDesc;
    ChannelSelector* chnlSelector = new MsgDrvChnlSelector();
    
    if (0 != chnlSelector->open(chnlDesc)) {
        AppLogger::Write(APLOG_ERROR
                        ,"Trans I/F: channel descriptor open failed.");
        //2003/11/12
        //create fault message and convert to response envelope
        return EXIT_FAILURE;
    }
  
    // write to MsgDrv
    if (0 > chnlDesc.write(fileIDOfMsgFromClientMsg)) {
        AppLogger::Write(APLOG_ERROR,"Trans I/F: write to MsgDrv failed.");
        //2003/11/12
        //create fault message and convert to response envelope
        return EXIT_FAILURE;
    }
  
    // read from MsgDrv
    string fileIDOfMsgFromMsgDrv;
    if (0 > chnlDesc.read(fileIDOfMsgFromMsgDrv)) {
        AppLogger::Write(APLOG_ERROR,"Trans I/F: read from MsgDrv failed.");
        //2003/11/12
        //create fault message and convert to response envelope
        return EXIT_FAILURE;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%d]"
                    ,"fileID From MsgDrv",fileIDOfMsgFromMsgDrv.c_str());
#endif

    //==============================
    //communication END
    //==============================
    //close
    chnlSelector->close(chnlDesc);
    delete chnlSelector;

    DataRetainer responseDr(srvConf->getSoapMessageSpoolPath());
    responseDr.SetId(fileIDOfMsgFromMsgDrv);
    responseDr.Decompose();
    string soapMsgFromMsgDrv;
    responseDr.GetSoapEnvelope(soapMsgFromMsgDrv);
    
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s %s=(%d):str[%d]"
                    ,"msg From MsgDrv:"
                    ,"size",soapMsgFromMsgDrv.length()
                    ,"str",soapMsgFromMsgDrv.c_str());
#endif //DEBUG

    //destory SrvConf
    delete srvConf;
  
    // convert Envelope
    ret = OpenSOAPByteArrayCreateWithData((const unsigned char*)
                                          soapMsgFromMsgDrv.c_str(),
                                          soapMsgFromMsgDrv.length(),
                                          &envBuf);

    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s %s %s=(%d)"
                        ,"Write to CGI:"
                        ,"OpenSOAPByteArrayCreateWithData() done."
                        ,"ErrorCode",ret);
    }
#ifdef DEBUG
    else {
        AppLogger::Write(APLOG_DEBUG5,"%s %s"
                        ,"Write to CGI:"
                        ,"OpenSOAPByteArrayCreateWithData() done.");
    }
#endif //DEBUG
	      
    //ret = OpenSOAPEnvelopeCreateCharEncoding(NULL,
    ret = OpenSOAPEnvelopeCreateCharEncoding(charEnc.c_str(),//ENC,
                                             envBuf,
                                             response);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s %s %s=(%d)"
                        ,"Write to CGI:"
                        ,"OpenSOAPEnvelopeCreateCharEncoding() done."
                        ,"ErrorCode",ret);
    }
#ifdef DEBUG
    else {
        AppLogger::Write(APLOG_DEBUG5,"%s %s"
                        ,"Write to CGI:"
                        ,"OpenSOAPEnvelopeCreateCharEncoding() done.");
        PrintEnvelope(*response, "Trans I/F response", charEnc.c_str());
    }
#endif //DEBUG

    ret = OpenSOAPByteArrayRelease(envBuf);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s %s=(%d)"
                        ,"Write to CGI: OpenSOAPByteArrayRelease() done."
                        ,"ErrorCode",ret);
    }
#ifdef DEBUG
    else {
        AppLogger::Write(APLOG_DEBUG5,"%s"
                        ,"Write to CGI: OpenSOAPByteArrayRelease() done.");
    }
#endif //DEBUG

    //clean up message...
    //dataRetainer->del(fileIDOfMsgFromClientMsg);
    dataRetainer->DeleteFiles();
    responseDr.DeleteFiles();

    delete dataRetainer;

    return ret;
}

int
main (int argc, char* argv[])
{
    int ret = OPENSOAP_NO_ERROR;
    char hoststr[256];
    ProcessInfo::SetProcessName(basename(argv[0]));
    gethostname(hoststr,sizeof(hoststr));
    ProcessInfo::SetHostName(hoststr);
	AppLogger logger(NULL,ProcessInfo::GetProcessName().c_str());

    //==============================
    //communication START
    //==============================
    OpenSOAPServicePtr service = NULL;
  
    ret = OpenSOAPInitialize(NULL);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s %s=(%d)"
                        ,"OpenSOAPInitialize failed.","ErrorCode",ret);
    }
    ret = OpenSOAPServiceCreateMB(&service, "OpenSOAPServerCGIIF", "cgi", 0);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s %s=(%d)"
                        ,"OpenSOAPServiceCreateMB failed.","ErrorCode",ret);
    }

    ret = OpenSOAPServiceRegisterMB(service, NULL, ServiceFunc, NULL);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s %s=(%d)"
                        ,"OpenSOAPServiceRegisterMB failed.","ErrorCode",ret);
    }

    ret = OpenSOAPServiceRun(service);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s %s=(%d)"
                        ,"OpenSOAPServiceRun failed.","ErrorCode",ret);
    }

    ret = OpenSOAPServiceRelease(service);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s %s=(%d)"
                        ,"OpenSOAPServiceRelease failed.","ErrorCode",ret);
    }
	
    ret = OpenSOAPUltimate();
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s %s=(%d)"
                        ,"OpenSOAPUltimate failed.","ErrorCode",ret);
    }
    
    return EXIT_SUCCESS;
}

// End of soapInterface.cpp
