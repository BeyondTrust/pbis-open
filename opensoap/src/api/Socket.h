/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Socket.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef OpenSOAP_IMPL_Socket_H
#define OpenSOAP_IMPL_Socket_H

#include "Stream.h"

#if defined(NONGCC_WIN32)
#include <winsock.h>
#else
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

#ifdef HAVE_SSL
#include <openssl/bio.h>
#include <openssl/ssl.h>
#endif /* HAVE_SSL */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
    typedef
#if defined(NONGCC_WIN32)
    SOCKET
#else
    int
#endif
    SOCKET_TYPE;
    
    typedef struct tagOpenSOAPSecureSocket {
        int     use;  /* a flag if secure socket is used */
#ifdef HAVE_SSL
        BIO     *bio; /* Abstract Input Output */
        SSL     *ssl; /* Secure Socket Layer Connection */
        SSL_CTX *ctx; /* SSL Context */
#endif
        int version; /* ssl protocol version */

        char * ca_file;        /* CA file for Peer */
        char * ca_dir;         /* CA dir for Peer */
        char * certchain_file; /* Certification chain file for local */
        char * privkey_file;   /* Private Key file for local */
        int  verify_level;      /* Certification Verification Level */
    } OpenSOAPSecureSocket;
    typedef OpenSOAPSecureSocket    *OpenSOAPSecureSocketPtr;
    
    typedef struct tagOpenSOAPSocket OpenSOAPSocket;
    typedef OpenSOAPSocket    *OpenSOAPSocketPtr;

    struct tagOpenSOAPSocket {
        OpenSOAPStream  super;
        SOCKET_TYPE     socket;
        OpenSOAPSecureSocket ssl;
    };

    int
    OPENSOAP_API
    OpenSOAPSocketInitialize(OpenSOAPSocketPtr /* [in, out] */ s,
                             OpenSOAPObjectFreeFunc /* [in] */ free_func,
                             OpenSOAPStreamReadFunc  /* [in] */ read_func,
                             OpenSOAPStreamWriteFunc /* [in] */ write_func);

    int
    OpenSOAPSocketWriteImplSecure(OpenSOAPStreamPtr /* [in] */ st,
                                  const unsigned char * /* [in] */ buf,
                                  size_t * /* [in, out] */ buf_sz);
    int
    OpenSOAPSocketReadImplSecure(OpenSOAPStreamPtr /* [in] */ st,
                                 unsigned char * /* [out] */ buf,
                                 size_t * /* [in, out] */ buf_sz);
    
    int
    OPENSOAP_API
    OpenSOAPSocketReleaseMembers(OpenSOAPSocketPtr /* [in] */ s);
    
    int
    OPENSOAP_API
    OpenSOAPSocketCreate(OpenSOAPSocketPtr * /* [out] */ s);
    
    int
    OPENSOAP_API
    OpenSOAPSocketRelease(OpenSOAPSocketPtr /* [in] */ s);
    
    int
    OPENSOAP_API
    OpenSOAPSocketOpen(OpenSOAPSocketPtr /* [in, out] */ s,
                       int /* [in] */ domain,
                       int /* [in] */ type,
                       int /* [in] */ protocol);
    int
    OPENSOAP_API
    OpenSOAPSocketConnect(OpenSOAPSocketPtr /* [in, out] */ s,
                          const struct sockaddr * /* [in] */ serv_addr,
                          size_t /* [in] */ addrlen);
    
    int
    OPENSOAP_API
    OpenSOAPSocketClose(OpenSOAPSocketPtr /* [in] */ s);
    

    int
    OPENSOAP_API
    OpenSOAPSocketRead(OpenSOAPSocketPtr /* [in] */ s,
                       unsigned char * /* [out] */ buf,
                       size_t * /* [in, out] */ buf_sz);
    

    int
    OPENSOAP_API
    OpenSOAPSocketWrite(OpenSOAPSocketPtr /* [in] */ s,
                        const unsigned char * /* [in] */ buf,
                        size_t * /* [in, out] */ buf_sz);
    

    int
    OPENSOAP_API
    OpenSOAPSocketReadToDelm(OpenSOAPSocketPtr /* [in] */ s,
                             const unsigned char * /* [in] */ delm,
                             size_t /* [in] */ delm_sz,
                             OpenSOAPByteArrayPtr /* [out] */ buf);

    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_Socket_H */
