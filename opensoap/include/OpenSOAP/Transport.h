/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Transport.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_Transport_H
#define OpenSOAP_Transport_H

#include <OpenSOAP/ByteArray.h>
#include <OpenSOAP/Envelope.h>
#include <OpenSOAP/Stream.h>

/**
 * @file OpenSOAP/Transport.h
 * @brief OpenSOAP API Transport Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * @typedef struct tagOpenSOAPTransport OpenSOAPTransport
     * @brief OpenSOAPTransport Structure Type Definition
     */
    typedef struct tagOpenSOAPTransport OpenSOAPTransport;

    /**
     * @typedef OpenSOAPTransport    *OpenSOAPTransportPtr
     * @brief OpenSOAPTransport Pointer Type Definition
     */
    typedef OpenSOAPTransport    *OpenSOAPTransportPtr;

    /**
      * @fn int OpenSOAPTransportCreate(OpenSOAPTransportPtr * t)
      * @brief Create Transport instance
      * @param
      *    t OpenSOAPTransportPtr * [out] ((|t|)) OpenSOAP Transport pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportCreate(OpenSOAPTransportPtr * /* [out] */ t);

    /**
      * @fn int OpenSOAPTransportRelease(OpenSOAPTransportPtr t)
      * @brief Release OpenSOAP Transport
      * @param
      *    t OpenSOAPTransportPtr [in] ((|t|)) OpenSOAP Transport pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportRelease(OpenSOAPTransportPtr /* [in] */ t);

    /**
      * @fn int OpenSOAPTransportSend(OpenSOAPTransportPtr t, OpenSOAPEnvelopePtr soap_env)
      * @brief Send SOAP Envelope as ByteArray
      * @param
      *    t OpenSOAPTransportPtr [in] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    soap_env OpenSOAPEnvelopePtr [in] ((|soap_env|)) SOAP Envelope
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSend(OpenSOAPTransportPtr /* [in] */ t,
                          OpenSOAPEnvelopePtr /* [in] */ soap_env);

    /**
      * @fn int OpenSOAPTransportReceive(OpenSOAPTransportPtr t, OpenSOAPEnvelopePtr * soap_env)
      * @brief Receive SOAP Message
      * @param
      *    t OpenSOAPTransportPtr [in] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    soap_env OpenSOAPEnvelopePtr * [in, out] ((|soap_env|)) SOAP Envelope
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportReceive(OpenSOAPTransportPtr /* [in] */ t,
                             OpenSOAPEnvelopePtr * /* [in, out] */ soap_env);

    /**
      * @fn int OpenSOAPTransportConnect(OpenSOAPTransportPtr t)
      * @brief Connect to end point
      * @param
      *    t OpenSOAPTransportPtr [in] ((|t|)) OpenSOAP Transport pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportConnect(OpenSOAPTransportPtr /* [in] */ t);

    /**
      * @fn int OpenSOAPTransportDisconnect(OpenSOAPTransportPtr t)
      * @brief Disconnect from end point
      * @param
      *    t OpenSOAPTransportPtr [in] ((|t|)) OpenSOAP Transport pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportDisconnect(OpenSOAPTransportPtr /* [in] */ t);

    /**
      * @fn int OpenSOAPTransportInvokeStream(OpenSOAPTransportPtr t,
    OpenSOAPStreamPtr stream, const char *int * tp_status)
      * @brief SOAP Service call using OpenSOAPStream
      * @param
      *    t OpenSOAPTransportPtr ((|t|)) OpenSOAP Transport pointer
      * @param
      *    request OpenSOAPStreamPtr [in] ((|request|)) SOAP request data
      * @param
      *    input_size size_t [in] ((|input_size|)) input stream size.
           if size_t <=0, size is unknown.
      * @param
      *    tp_status int * [out] ((|tp_status|)) Transport status value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportInvokeStream(OpenSOAPTransportPtr /* [in] */ t,
                                  OpenSOAPStreamPtr /* [in] */ stream,
                                  int /* [in] */ input_size,
                                  int * /* [out] */ tp_status);

    /**
      * @fn int OpenSOAPTransportInvokeByteArray(OpenSOAPTransportPtr t, OpenSOAPByteArrayPtr request, OpenSOAPByteArrayPtr response, int * tp_status)
      * @brief SOAP Service call using OpenSOAPByteArray
      * @param
      *    t OpenSOAPTransportPtr ((|t|)) OpenSOAP Transport pointer
      * @param
      *    request OpenSOAPEnvelopePtr [in] ((|request|)) SOAP request data
      * @param
      *    response OpenSOAPByteArrayPtr [out] ((|response|)) SOAP response data
      * @param
      *    tp_status int * [out] ((|tp_status|)) Transport status value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportInvokeByteArray(OpenSOAPTransportPtr /* [in] */ t,
                                     OpenSOAPByteArrayPtr /* [in] */ request,
                                     OpenSOAPByteArrayPtr /* [out] */ response,
                                     int * /* [out] */ tp_status);

    /**
      * @fn int OpenSOAPTransportInvoke(OpenSOAPTransportPtr t, OpenSOAPEnvelopePtr request, OpenSOAPEnvelopePtr * response)
      * @brief SOAP call
      * @param
      *    t OpenSOAPTransportPtr [in] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    request OpenSOAPEnvelopePtr [in] ((|request|)) SOAP request Envelope
      * @param
      *    response OpenSOAPEnvelopePtr * [in, out] ((|response|)) SOAP response Envelope
      * @return
      *    Error Code

      *    OPENSOAP_TRANSPORT_ERROR
            (getaddrinfo)
            - OPENSOAP_TRANSPORT_HOST_NOT_FOUND
              Hostname not found (DNS error)
            (connect)
            - OPENSOAP_TRANSPORT_NETWORK_UNREACH
              Network is unreachable (Network problem on local?)
            - OPENSOAP_TRANSPORT_HOST_UNREACH
              Hostname is unreachable (Network problem on remote?)
            - OPENSOAP_TRANSPORT_CONNECTION_REFUSED
              Connection Refused (The daemon doesn't exist)
            - OPENSOAP_TRANSPORT_CONNECTION_TIMEOUT
              Connection Timed out (The server may be busy)
            HTTP
            - OPENSOAP_TRANSPORT_IS_HTTP_ERROR()
            - OPENSOAP_TRANSPORT_GET_HTTP_ERROR()

            - OPENSOAP_UNSUPPORT_PROTOCOL
    */
    int
    OPENSOAP_API
    OpenSOAPTransportInvoke(OpenSOAPTransportPtr /* [in] */ t,
                            OpenSOAPEnvelopePtr /* [in] */ request,
                            OpenSOAPEnvelopePtr * /* [in, out] */ response);
    
    /**
      * @fn int OpenSOAPTransportSetSOAPAction(OpenSOAPTransportPtr t, const char * soap_action)
      * @brief Set SOAP-Action Header (Some-URI)
      * @param
      *    t OpenSOAPTransportPtr [in, out] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    soap_action const char * [in] ((|soap_action|)) soap-action
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSetSOAPAction(OpenSOAPTransportPtr /* [in, out] */ t,
                                   const char * /* [in] */ soap_action);

    /**
      * @fn int OpenSOAPTransportSetURL(OpenSOAPTransportPtr t, const char * url)
      * @brief Set request URL
      * @param
      *    t OpenSOAPTransportPtr [in, out] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    url const char * [in] ((|url|)) URL
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSetURL(OpenSOAPTransportPtr /* [in, out] */ t,
                            const char * /* [in] */ url);

    /**
      * @fn int OpenSOAPTransportSetHeader(OpenSOAPTransportPtr t, const char * header_name, const char * header_value)
      * @brief Set Header
      * @param
      *    t OpenSOAPTransportPtr [in, out] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    header_name const char * [in] ((|header_name|)) header name
      * @param
      *    header_value const char * [in] ((|header_value|)) header's value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSetHeader(OpenSOAPTransportPtr /* [in, out] */ t,
                               const char * /* [in] */ header_name,
                               const char * /* [in] */ header_value);

    /**
      * @fn int OpenSOAPTransportGetHeader(OpenSOAPTransportPtr t, const char * header_name, char ** header_value)
      * @brief Get a header value of the response.
      * @param
      *    t OpenSOAPTransportPtr [in] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    header_name const char * [in] ((|header_name|)) header name
      * @param
      *    header_value char ** [out] ((|header_value|)) header's value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportGetHeader(OpenSOAPTransportPtr /* [in] */ t,
                               const char * /* [in] */ header_name,
                               char ** /* [out] */ header_value);
    
    /**
      * @fn int OpenSOAPTransportSetCharset(OpenSOAPTransportPtr t, const char * charset)
      * @brief Set Character Set
      * @param
      *    t OpenSOAPTransportPtr [in, out] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    charset const char * [in] ((|charset|)) character set
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSetCharset(OpenSOAPTransportPtr /* [out] */ t,
                                const char * /* [in] */ charset);

    /**
      * @fn int OpenSOAPTransportSetContentType(OpenSOAPTransportPtr t, const char * content_type)
      * @brief Set Content-Type (Overwrite OpenSOAPTransportSetCharset())
      * @param
      *    t OpenSOAPTransportPtr [in, out] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    content_type const char * [in] ((|content_type|)) content type
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSetContentType(OpenSOAPTransportPtr /* [out] */ t,
                                    const char * /* [in] */ content_type);

    /**
      * @fn int OpenSOAPTransportSMTPSetHostname(OpenSOAPTransportPtr s)
      * @brief Set SMTP Host
      * @param
      *    s OpenSOAPTransportPtr [out] ((|s|)) OpenSOAP Transport pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSMTPSetHostname(OpenSOAPTransportPtr /* [in, out] */ s);

    /**
      * @fn int OpenSOAPTransportSMTPSetFrom(OpenSOAPTransportPtr t)
      * @brief Set SMTP From Header
      * @param
      *    t OpenSOAPTransportPtr [out] ((|t|)) OpenSOAP Transport pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSMTPSetFrom(OpenSOAPTransportPtr /* [in, out] */ t);

    /**
      * @fn int OpenSOAPTransportSMTPSetTo(OpenSOAPTransportPtr t)
      * @brief Set SMTP To Header
      * @param
      *    t OpenSOAPTransportPtr [out] ((|t|)) OpenSOAP Transport pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSMTPSetTo(OpenSOAPTransportPtr /* [in, out] */ t);

    /**
      * @fn int OpenSOAPTransportSetAuthUserPass(OpenSOAPTransportPtr t, const char * user, const char * passwd)
      * @brief Set User/Passwd for Authentification
      * @param
      *    t OpenSOAPTransportPtr [in, out] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    user   const char * [in] ((|user|)) username for authentication
      * @param
      *    passwd const char * [in] ((|passwd|)) password for authentication
	  * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSetAuthUserPass(OpenSOAPTransportPtr /* [out] */ t,
									 const char * /* [in] */ user,
									 const char * /* [in] */ passwd);

    /**
     * @def OPENSOAP_AUTH_TYPE_BASIC    (1)
     * @brief Transport Basic Authentication
     */
#define OPENSOAP_AUTH_TYPE_BASIC    (1)  /* BASIC */

    /**
     * @def OPENSOAP_AUTH_TYPE_DIGEST    (2)
     * @brief Transport Digest Authentification
     */
#define OPENSOAP_AUTH_TYPE_DIGEST    (2)  /* DIGEST */

    /**
     * @def OPENSOAP_AUTH_TYPE_ANY     (0)
     * @brief Any supported type for Transport Authentification
     */
#define OPENSOAP_AUTH_TYPE_ANY (0)

    /**
      * @fn int OpenSOAPTransportSetAuthType(OpenSOAPTransportPtr t, int auth_type)
      * @brief Set Authentification type
      * @param
      *    t OpenSOAPTransportPtr [in, out] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    auth_type int [in] ((|auth_type|)) acceptable type for
      authentication in choice of 
          OPENSOAP_AUTH_TYPE_BASIC | OPENSOAP_AUTH_TYPE_DIGEST |
          OPENSOAP_AUTH_TYPE_ANY
		  
		  If only OPENSOAP_AUTH_TYPE_BASIC is set, directly send
		  UserPassword for Basic Authentication in first request.
	  * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSetAuthType(OpenSOAPTransportPtr /* [out] */ t,
								 int /* [in] */ auth_type );

    /**
      * @fn int OpenSOAPTransportSetProxy(OpenSOAPTransportPtr t,
                           const char * host, int port, const char * user,
                           const char * passwd, int auth_type)
      * @brief Set Proxy Server Information
      * @param
      *    t OpenSOAPTransportPtr [in, out] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    host const char * [in] ((|host|)) Hostname of Proxy Server
      * @param
      *    port int [in] ((|port|)) Port Number of Proxy Server
      * @param
      *    user  const char * [in] ((|user|)) Proxy Server username (NULL for non-user/passwd auth)
      * @param
      *    passwd const char * [in] ((|passwd|)) Proxy Server passwd
      * @param
      *    auth_type int [in] ((|auth_type|)) acceptable type for
      authentication in choice of 
          OPENSOAP_AUTH_TYPE_BASIC | OPENSOAP_AUTH_TYPE_DIGEST | OPENSOAP_AUTH_TYPE_ANY
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSetProxy(OpenSOAPTransportPtr /* [out] */ t,
                              const char * /* [in] */ host,
                              int /* [in] */ port,
                              const char * /* [in] */ user,
                              const char * /* [in] */ passwd,
                              int /* [in] */ auth_type);

    /**
     * @def OPENSOAP_SSL_VER_SSL2    (1)
     * @brief Transport SSL Version SSL2.0 ONLY
     */
#define OPENSOAP_SSL_VER_SSL2    (1)  /* SSL_V2.0 ONLY */
    
    /**
     * @def OPENSOAP_SSL_VER_SSL3    (2)
     * @brief Transport SSL Version SSL3.0 ONLY
     */
#define OPENSOAP_SSL_VER_SSL3    (2)  /* SSL_V3.0 ONLY */
    
    /**
     * @def OPENSOAP_SSL_VER_TLS1    (4)
     * @brief Transport SSL Version TLS1.0 ONLY
     */
#define OPENSOAP_SSL_VER_TLS1    (4)  /* TLS_V1.0 ONLY */

    /**
     * @def OPENSOAP_SSL_VER_ALL    (0)
     * @brief Transport SSL Version accepts all Support Version
     *  (SSLv2/SSLv3/TLSv1)
     */
#define OPENSOAP_SSL_VER_ALL (0) /* SSL all version */
    
    /**
      * @fn int OpenSOAPTransportSetSSLVersion(OpenSOAPTransportPtr t,
            int ssl_version )
      * @brief Set acceptable SSL versions
      * @param
      *    t OpenSOAPTransportPtr [in, out] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    ssl_version int [in] ((|ssl_version|)) SSL version choice from
          (OPENSOAP_SSL_VER_SSL2 | OPENSOAP_SSL_VER_SSL3 |
    OPENSOAP_SSL_VER_TLS1 | OPENSOAP_SSL_VER_ALL)
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSetSSLVersion(OpenSOAPTransportPtr /* [out] */ t,
                                int /* [in] */ ssl_version );

    /**
      * @fn int OpenSOAPTransportSetSSLCert(OpenSOAPTransportPtr t,
            const char * ca_file, const char * ca_dir,
            const char * certchain_file, const char * privkey_file,
            int verify_level )
      * @brief Set SSL Certification files
      * @param
      *    t OpenSOAPTransportPtr [in, out] ((|t|)) OpenSOAP Transport pointer
      * @param
      *    ca_file const char * [in] ((|ca_file|)) filename of Certification Authority for Peer (NULL for no certification)
      * @param
      *    ca_dir  const char * [in] ((|ca_dir|))  directory name which contains Peer's CA keys (if NULL, look up no directory)
      * @param
      *    certchain_file const char * [in] ((|certchain_file|)) filename of SSL Certification Chains for local (NULL for no certification)
      * @param
      *    privkey_file  const char * [in] ((|privkey_file|))   filename of Private Key, which is a pair of certchain_file (NULL for no certification)
      * @param
      *    verify_level  int [in] ((|verify_level|))   Verification Level of Peer Certification
      *                                                0(default) =
    session continues even if the certification is invalid, 1 = quit
    and returns as failed
      *
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPTransportSetSSLCert(OpenSOAPTransportPtr /* [out] */ t,
                                      const char * /* [in] */ ca_file,
                                      const char * /* [in] */ ca_dir,
                                      const char * /* [in] */ certchain_file,
                                      const char * /* [in] */ privkey_file,
                                      int /* [in] */ verify_level );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_Transport_H */
