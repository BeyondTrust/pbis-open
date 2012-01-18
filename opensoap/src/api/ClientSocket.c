/*-----------------------------------------------------------------------------
 * $RCSfile: ClientSocket.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: ClientSocket.c,v 1.23 2003/11/04 09:50:38 okada Exp $";
#endif  /* _DEBUG */

#include "ClientSocket.h"

#include "AddrInfo.h"

/*
=begin
= OpenSOAP ClientSocket class
=end
 */

static
int
OpenSOAPClientSocketFree(/* [in] */ OpenSOAPObjectPtr obj) {
    OpenSOAPSocketPtr s = (OpenSOAPSocketPtr)obj;
    int ret = OpenSOAPSocketReleaseMembers(s);

    if (OPENSOAP_SUCCEEDED(ret)) {
        free(obj);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPClientSocketCreate(s)
    OpenSOAP ClientSocket instance create

    :Parameters
      :OpenSOAPClientSocketPtr * ((|s|))
        [out] OpenSOAP ClientSocket pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPClientSocketCreate(/* [out] */ OpenSOAPClientSocketPtr *s) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (s) {
        *s = malloc(sizeof(OpenSOAPClientSocket));
        if (*s) {
            ret = OpenSOAPSocketInitialize((OpenSOAPSocketPtr)*s,
                                           OpenSOAPClientSocketFree,
                                           NULL, NULL);
            ((OpenSOAPSocketPtr)*s)->ssl.use = 0;
            if (OPENSOAP_FAILED(ret)) {
                free(*s);
                *s = NULL;
            }
        }
        else {
            ret = OPENSOAP_MEM_BADALLOC;
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPClientSocketCreateSecure(s)
    OpenSOAP ClientSocket instance for SSL create

    :Parameters
      :OpenSOAPClientSocketPtr * ((|s|))
        [out] OpenSOAP ClientSocket pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPClientSocketCreateSecure(/* [out] */ OpenSOAPClientSocketPtr *s) {
#ifdef HAVE_SSL
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (s) {
        *s = malloc(sizeof(OpenSOAPClientSocket));
        if (*s) {
            ret = OpenSOAPSocketInitialize((OpenSOAPSocketPtr)*s,
                                           OpenSOAPClientSocketFree,
                                           OpenSOAPSocketReadImplSecure,
                                           OpenSOAPSocketWriteImplSecure);
            ((OpenSOAPSocketPtr)*s)->ssl.use = 1;
            if (OPENSOAP_FAILED(ret)) {
                free(*s);
                *s = NULL;
            }
        }
        else {
            ret = OPENSOAP_MEM_BADALLOC;
        }
    }
    
    return ret;
#else  /* !HAVE_SSL */
    return OPENSOAP_YET_IMPLEMENTATION;
#endif /* HAVE_SSL */
}
      
/*
=begin
--- function#OpenSOAPClientSocketRelease(s)
    Release OpenSOAP ClientSocket

    :Parameters
      :OpenSOAPClientSocketPtr  ((|s|))
        [in] OpenSOAP ClientSocket
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPClientSocketRelease(/* [in] */ OpenSOAPClientSocketPtr s) {
    return OpenSOAPObjectRelease((OpenSOAPObjectPtr)s);
}

/*
=begin
--- function#OpenSOAPClientSocketOpen(s, node, service)
    Open and Connect OpenSOAP ClientSocket.

    :Parameters
      :OpenSOAPClientSocketPtr ((|s|))
        [in] OpenSOAP ClientSocket
      :const char * ((|node|))
        [in] hostname, and so on.
        node. i.e. "www.isrc.co.jp", "192.168.0.1", ...
      :const char * ((|service|))
        [in] service. i.e. "http", "ftp", "smtp", "8080", ...
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPClientSocketOpen(/* [in] */ OpenSOAPClientSocketPtr cs,
                         /* [in] */ const char *node,
                         /* [in] */ const char *service) {
    OpenSOAPSocketPtr s = (OpenSOAPSocketPtr)cs;
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s && node && *node && service && *service) {
        OpenSOAPAddrInfoPtr res = NULL;
        ret = OpenSOAPAddrInfoCreate(node,
                                     service,
                                     &res);
        if (OPENSOAP_SUCCEEDED(ret)) {
            OpenSOAPAddrInfoPtr ai = res;
            for (; ai; ai = ai->ai_next) {
                ret = OpenSOAPSocketOpen(s,
                                         ai->ai_family,
                                         ai->ai_socktype,
                                         ai->ai_protocol);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPSocketConnect(s,
                                                ai->ai_addr,
                                                ai->ai_addrlen);
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        break;
                    }
                    OpenSOAPSocketClose(s);
                }
            }
            
            OpenSOAPAddrInfoRelease(res);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPClientSocketClose(s)
    Close OpenSOAP ClientSocket.

    :Parameters
      :OpenSOAPClientSocketPtr ((|s|))
        [in] OpenSOAP ClientSocket
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPClientSocketClose(/* [in] */ OpenSOAPClientSocketPtr s) {
    int ret = OpenSOAPSocketClose((OpenSOAPSocketPtr)s);
    
    return ret;
}

/*
=begin
--- function#OpenSOAPClientSocketRead(s, buf, buf_sz)
    Read from OpenSOAP ClientSocket.

    :Parameters
      :OpenSOAPClientSocketPtr ((|s|))
        [in] OpenSOAP ClientSocket
      :unsigned char * ((|buf|))
        [out] buffer for read data.
      :size_t * ((|buf_sz|))
        [in, out] Size of buf's pointer. After this function call,
		          return readed size.
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPClientSocketRead(/* [in] */ OpenSOAPClientSocketPtr s,
						 /* [out] */ unsigned char *buf,
						 /* [in, out] */ size_t *buf_sz) {
    int ret = OpenSOAPSocketRead((OpenSOAPSocketPtr)s,
                                 buf,
                                 buf_sz);

    return ret;
}

/*
=begin
--- function#OpenSOAPClientSocketWrite(s, buf, buf_sz)
    Write to OpenSOAP ClientSocket.

    :Parameters
      :OpenSOAPClientSocketPtr  ((|s|))
        [in] OpenSOAP ClientSocket
      :const unsigned char * ((|buf|)),
        [in] buffer for write data.
      :size_t * ((|buf_sz|))
        [in, out] Size of buf's pointer. After this function call,
				return write size.
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPClientSocketWrite(/* [in] */ OpenSOAPClientSocketPtr s,
						  /* [in] */ const unsigned char *buf,
						  /* [in, out] */ size_t *buf_sz) {
    int ret = OpenSOAPSocketWrite((OpenSOAPSocketPtr)s,
                                  buf,
                                  buf_sz);

    return ret;
}

/*
=begin
--- function#OpenSOAPClientSocketReadToDelm(s, delm, delm_sz, buf)
    Read at delimiter from OpenSOAP ClientSocket.

    :Parameters
      :OpenSOAPClientSocketPtr ((|s|))
        [in] OpenSOAP ClientSocket
      :const unsigned char * ((|delm|))
        [in] Delimiter data array.
      :size_t delm_sz
        [in] Delimiter data size.
      :OpenSOAPByteArrayPtr ((|buf|))
        [out] Buffer for
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPClientSocketReadToDelm(/* [in] */ OpenSOAPClientSocketPtr s,
							   /* [in] */ const unsigned char *delm,
							   /* [in] */ size_t delm_sz,
							   /* [out] */ OpenSOAPByteArrayPtr buf) {
    int ret = OpenSOAPSocketReadToDelm((OpenSOAPSocketPtr)s,
                                       delm,
                                       delm_sz,
                                       buf);

    return ret;
}

/*
=begin
--- function#OpenSOAPClientSocketReadWithSize(s, sz, buf)
    Read with size from OpenSOAP ClientSocket. If read size is zero,
	read as possible.

    :Parameters
      :OpenSOAPClientSocketPtr ((|s|))
        [in] OpenSOAP ClientSocket
      :size_t ((|sz|))
        [in] read size
      :OpenSOAPByteArrayPtr ((|buf|))
        [out] Buffer for read of data.
    :Returns
      :int
      
=end
 */
#if 0
extern
int
/* OPENSOAP_API */
OpenSOAPClientSocketReadWithSize(OpenSOAPClientSocketPtr /* [in] */ s,
                                 size_t /* [in] */ sz,
                                 OpenSOAPByteArrayPtr /* [out] */ buf) {
    int ret = OpenSOAPStreamReadWithSize((OpenSOAPStreamPtr)s, sz, buf);

    return ret;
}
#endif

