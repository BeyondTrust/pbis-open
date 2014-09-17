/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Service.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_Service_H
#define OpenSOAP_Service_H

#include <OpenSOAP/Envelope.h>

/**
 * @file OpenSOAP/Service.h
 * @brief OpenSOAP API Service Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * @typedef struct tagOpenSOAPService OpenSOAPService
     * @brief OpenSOAPService Structure Type Definition
     */
    typedef struct tagOpenSOAPService OpenSOAPService;

    /**
     * @typedef OpenSOAPService    *OpenSOAPServicePtr
     * @brief OpenSOAP Service Ponter Definition
     */
    typedef OpenSOAPService    *OpenSOAPServicePtr;

    /**
      * @typedef int (*OpenSOAPServiceFuncPtr)(OpenSOAPEnvelopePtr request, OpenSOAPEnvelopePtr *response, void *opt)
      * @brief Define Service Function Pointer
      * @param
      *    request OpenSOAPEnvelopePtr [in] ((|request|)) Service Request
      * @param
      *    response OpenSOAPEnvelopePtr * [out] ((|response|)) 
      * @param
      *    opt void * [in, out] ((|opt|))
      * @return
      *    Error Code
      */
    typedef
    int
    (*OpenSOAPServiceFuncPtr)(/* [in]  */ OpenSOAPEnvelopePtr request,
                              /* [out] */ OpenSOAPEnvelopePtr *response,
                              /* [in, out] */ void *opt);

    /**
      * @fn int OpenSOAPServiceCreateMB(OpenSOAPServicePtr *srv, const char *srv_name, const char *connect_type, int is_loop, ...)
      * @brief Create An OpenSOAP Service(MB)
      * @param
      *    srv OpenSOAPServicePtr * [out] ((|srv|)) OpenSOAP Service pointer
      * @param
      *    srv_name const char * [in] ((|srv_name|)) Service Type
      * @param
      *    connect_type const char * [in] ((|connect_type|)) Connection Type [stdio|cgi]
      * @param
      *    is_loop int [in] ((|is_loop|)) 
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPServiceCreateMB(/* [out] */ OpenSOAPServicePtr *srv,
                            /* [in]  */ const char *srv_name,
                            /* [in]  */ const char *connect_type,
                            /* [in]  */ int is_loop,
                            ...);

    /**
      * @fn int OpenSOAPServiceCreateWC(OpenSOAPServicePtr *srv, const wchar_t *srv_name, const char *connect_type, int is_loop, ...)
      * @brief Create An OpenSOAP Service(WC)
      * @param
      *    srv OpenSOAPServicePtr * [out] ((|srv|)) OpenSOAP Service pointer
      * @param
      *    srv_name const wchar_t * [in] ((|srv_name|)) Service Type
      * @param
      *    connect_type const wchar_t * [in] ((|connect_type|)) Connection Type [stdio|cgi]
      * @param
      *    is_loop int [in] ((|is_loop|)) 
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPServiceCreateWC(/* [out] */ OpenSOAPServicePtr *srv,
                            /* [in]  */ const wchar_t *srv_name,
                            /* [in]  */ const char *connect_type,
                            /* [in]  */ int is_loop,
                            ...);

    /**
      * @fn int OpenSOAPServiceRelease(OpenSOAPServicePtr srv)
      * @brief Release an OpenSOAP Service
      * @param
      *    srv OpenSOAPServicePtr [in, out] ((|srv|)) OpenSOAP Service
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPServiceRelease(/* [in, out] */ OpenSOAPServicePtr srv);
    
    /**
      * @fn int OpenSOAPServiceRegisterMB(OpenSOAPServicePtr srv, const char *name, OpenSOAPServiceFuncPtr func, void *opt)
      * @brief Register OpenSOAP Service
      * @param
      *    srv OpenSOAPServicePtr [in] ((|srv|)) OpenSOAP Service
      * @param
      *    name const char * [in, out] ((|name|)) service function name
      * @param
      *    func OpenSOAPServiceFuncPtr [in] ((|func|)) service function pointer
      * @param
      *    opt void * [in] ((|opt|)) service function option parameter
      * @return
      *    Error Code
      * @note
      *    In OpenSOAPServiceRun function, if service function name is equal to ((|name|)), then call func(request_env, response_env, opt) and return value is FAILED, stop OpenSOAPServiceRun and return func's return value.
      */
    int
    OPENSOAP_API
    OpenSOAPServiceRegisterMB(/* [in, out] */ OpenSOAPServicePtr srv,
                              /* [in] */ const char *name,
                              /* [in] */ OpenSOAPServiceFuncPtr func,
                              /* [in] */ void *opt);

    /**
      * @fn int OpenSOAPServiceRegisterWC(OpenSOAPServicePtr srv, const wchar_t *name, OpenSOAPServiceFuncPtr func, void *opt)
      * @brief Register OpenSOAP Service
      * @param
      *    srv OpenSOAPServicePtr [in] ((|srv|)) OpenSOAP Service
      * @param
      *    name const wchar_t * [in, out] ((|name|)) service function name
      * @param
      *    func OpenSOAPServiceFuncPtr [in] ((|func|)) service function pointer
      * @param
      *    opt void * [in] ((|opt|)) service function option parameter
      * @return
      *    Error Code
      * @note
      *    In OpenSOAPServiceRun function, if service function name is equal to ((|name|)), then call func(request_env, response_env, opt) and return value is FAILED, stop OpenSOAPServiceRun and return func's return value.
      */
    int
    OPENSOAP_API
    OpenSOAPServiceRegisterWC(/* [in, out] */ OpenSOAPServicePtr srv,
                              /* [in] */ const wchar_t *name,
                              /* [in] */ OpenSOAPServiceFuncPtr func,
                              /* [in] */ void *opt);

    /**
      * @fn int OpenSOAPServiceRun(OpenSOAPServicePtr srv)
      * @brief Execute OpenSOAP Service
      * @param
      *    srv OpenSOAPServicePtr [in, out] ((|srv|)) OpenSOAP Service
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPServiceRun(/* [in, out] */ OpenSOAPServicePtr srv);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_Service_H */
