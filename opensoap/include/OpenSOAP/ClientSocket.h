/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ClientSocket.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_ClientSocket_H
#define OpenSOAP_ClientSocket_H

#include <OpenSOAP/ByteArray.h>

/**
 * @file OpenSOAP/ClientSocket.h
 * @brief OpenSOAP API Client Socket Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * @typedef struct tagOpenSOAPClientSocket OpenSOAPClientSocket
     * @brief OpenSOAPClientSocket Structure Type Definition
     */
    typedef struct tagOpenSOAPClientSocket OpenSOAPClientSocket;

    /**
     * @typedef OpenSOAPClientSocket    *OpenSOAPClientSocketPtr
     * @brief OpenSOAPClientSocket Pointer Type Definition
     */
    typedef OpenSOAPClientSocket    *OpenSOAPClientSocketPtr;

    /**
      * @fn int OpenSOAPClientSocketCreate(OpenSOAPClientSocketPtr * s)
      * @brief OpenSOAP ClientSocket instance create
      * @param
      *    s OpenSOAPClientSocketPtr * [out] ((|s|)) OpenSOAP ClientSocket pointer
      * @return
      *    Error Code
      */
    extern
    int
    OPENSOAP_API
    OpenSOAPClientSocketCreate(OpenSOAPClientSocketPtr * /* [out] */ s);
    
    /**
      * @fn int OpenSOAPClientSocketCreateSecure(OpenSOAPClientSocketPtr * s)
      * @brief OpenSOAP ClientSocket instance create in SSL
      * @param
      *    s OpenSOAPClientSocketPtr * [out] ((|s|)) OpenSOAP ClientSocket pointer
      * @return
      *    Error Code
      */
    extern
    int
    OPENSOAP_API
    OpenSOAPClientSocketCreateSecure(/* [out] */ OpenSOAPClientSocketPtr *s);

    /**
      * @fn int OpenSOAPClientSocketRelease(OpenSOAPClientSocketPtr s)
      * @brief Release OpenSOAP ClientSocket
      * @param
      *    s OpenSOAPClientSocketPtr [in] ((|s|)) OpenSOAP ClientSocket
      * @return
      *    Error Code
      */
    extern
    int
    OPENSOAP_API
    OpenSOAPClientSocketRelease(OpenSOAPClientSocketPtr /* [in] */ s);
    
    /**
      * @fn int OpenSOAPClientSocketOpen(OpenSOAPClientSocketPtr s, const char * node, const char * service)
      * @brief Open and Connect OpenSOAP ClientSocket
      * @param
      *    s OpenSOAPClientSocketPtr [in] ((|s|)) OpenSOAP ClientSocket
      * @param
      *    node const char * [in] ((|node|)) hostname, and so on node. (i.e. "www.isrc.co.jp", "192.168.0.1", ...)
      * @param
      *    service const char * [in] ((|service|)) service. (i.e. "http", "ftp", "smtp", "8080", ...)
      * @return
      *    Error Code
      */
    extern
    int
    OPENSOAP_API
    OpenSOAPClientSocketOpen(OpenSOAPClientSocketPtr /* [in] */ s,
                             const char * /* [in] */ node,
                             const char * /* [in] */ service);
    
    /**
      * @fn int OpenSOAPClientSocketClose(OpenSOAPClientSocketPtr s)
      * @brief Close OpenSOAP ClientSocket
      * @param
      *    s OpenSOAPClientSocketPtr [in] ((|s|)) OpenSOAP ClientSocket
      * @return
      *    Error Code
      */
    extern
    int
    OPENSOAP_API
    OpenSOAPClientSocketClose(OpenSOAPClientSocketPtr /* [in] */ s);
    
    /**
      * @fn int OpenSOAPClientSocketRead(OpenSOAPClientSocketPtr s, unsigned char * buf, size_t * buf_sz)
      * @brief Read from OpenSOAP ClientSocket
      * @param
      *    s OpenSOAPClientSocketPtr [in] ((|s|)) OpenSOAP ClientSocket
      * @param
      *    buf unsigned char * [in] ((|buf|)) buffer for read data
      * @param
      *    buf_sz size_t * [in, out] ((|buf_sz|)) Size of buffer. After this function call, returns actual size read.
      * @return
      *    Error Code
      */
    extern
    int
    OPENSOAP_API
    OpenSOAPClientSocketRead(OpenSOAPClientSocketPtr /* [in] */ s,
                             unsigned char * /* [out] */ buf,
                             size_t * /* [in, out] */ buf_sz);
    
    /**
      * @fn int OpenSOAPClientSocketWrite(OpenSOAPClientSocketPtr s, const unsigned char * buf, size_t * buf_sz)
      * @brief Write to OpenSOAP ClientSocket
      * @param
      *    s OpenSOAPClientSocketPtr [in] ((|s|)) OpenSOAP ClientSocket
      * @param
      *    buf const unsigned char * [in] ((|buf|)) buffer for write data
      * @param
      *    buf_sz size_t * ((|buf_sz|)) Size of buffer. After this function call, return write size.
      * @return
      *    Error Code
      */
    extern
    int
    OPENSOAP_API
    OpenSOAPClientSocketWrite(OpenSOAPClientSocketPtr /* [in] */ s,
                              const unsigned char * /* [in] */ buf,
                              size_t * /* [in, out] */ buf_sz);
    
    /**
      * @fn int OpenSOAPClientSocketReadToDelm(OpenSOAPClientSocketPtr s, const unsigned char * delm, size_t delm_sz, OpenSOAPByteArrayPtr buf)
      * @brief Read up to delimiter from OpenSOAP ClientSocket
      * @param
      *    s OpenSOAPClientSocketPtr [in] ((|s|)) OpenSOAP ClientSocket
      * @param
      *    delm const unsigned char * [in] ((|delm|)) Delimiter data array
      * @param
      *    delm_sz size_t [in] ((|delm_sz|)) Delimiter data size
      * @param
      *    buf OpenSOAPByteArrayPtr [out] ((|buf|)) Buffer for read data
      * @return
      *    Error Code
      */
    extern
    int
    OPENSOAP_API
    OpenSOAPClientSocketReadToDelm(OpenSOAPClientSocketPtr /* [in] */ s,
                                   const unsigned char * /* [in] */ delm,
                                   size_t /* [in] */ delm_sz,
                                   OpenSOAPByteArrayPtr /* [out] */ buf);

#if 0    
    /**
      * @fn int OpenSOAPClientSocketReadWithSize(OpenSOAPClientSocketPtr s, size_t sz, OpenSOAPByteArrayPtr buf)
      * @brief Read with size from OpenSOAP ClientSocket. If read size is zero, read as much as possible.(??)
      * @param
      *    s OpenSOAPClientSocketPtr [in] ((|s|)) OpenSOAP ClientSocket
      * @param
      *    sz size_t [in] ((|sz|)) read size
      * @param
      *    buf OpenSOAPByteArrayPtr [out] ((|buf|)) Buffer for read data
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPClientSocketReadWithSize(OpenSOAPClientSocketPtr /* [in] */ s,
                                     size_t /* [in] */ sz,
                                     OpenSOAPByteArrayPtr /* [out] */ buf);
#endif
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_ClientSocket_H */
