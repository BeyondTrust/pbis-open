/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: soapInterface4ap_dso.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef SOAPINTERFACE4AP_DSO_H
#define SOAPINTERFACE4AP_DSO_H

//#include <OpenSOAP/Envelope.h>

#define DSO_NO_MATCH 774
#define DSO_TOO_BIG 816

#ifdef __cplusplus
extern "C" {
#endif 

extern void SetProcessInfo(void);
extern int WriteLog(int level,const char * comment);

extern int GetNewId(char* num);
extern int CheckLimitSizeMessage(long length, char* num);
extern int GetFileName(const char* num, char* name);
extern int AddHttpHeader(const char* num, const char* key, const char* data);
extern int GetHttpHeader(const char* num, const char* key, const char** data);
extern int InvokeOpenSOAPServer(const char* req_num, char* res_num);
extern int DeleteFiles(const char* num);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //SOAPINTERFACE4AP_DSO_H
