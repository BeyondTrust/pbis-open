/*-----------------------------------------------------------------------------
 * $RCSfile: Socket.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: Socket.c,v 1.15 2003/12/01 08:31:50 okada Exp $";
#endif  /* _DEBUG */

#ifdef _DEBUG
#  include <stdio.h>
#  include <string.h>
#  ifndef _WIN32
#    include <arpa/inet.h>
#    include <netinet/in.h>
#  endif
#endif /* _DEBUG */

#include "Socket.h"

/* for SSL ver */
#include <OpenSOAP/Transport.h>

# ifdef HAVE_SSL
#  include <openssl/x509.h>
#  ifdef _DEBUG
#    include <openssl/err.h>
#  endif /* _DEBUG */
# endif /* HAVE_SSL */

/*
=begin
= OpenSOAP Socket class
=end
 */

#if defined(NONGCC_WIN32)

#define SOCKET_INITIALIZE(s) ((s) = INVALID_SOCKET)
#define IS_VALID_SOCKET(s)  ((s) != INVALID_SOCKET)

static
int
CLOSE_SOCKET(SOCKET_TYPE s) {
    int ret = OPENSOAP_NO_ERROR;

    if (closesocket(s) == SOCKET_ERROR) {
        int c_err = WSAGetLastError();
        if (c_err == WSANOTINITIALISED) {
            ret = OPENSOAP_IO_ERROR;
        }
        else if (c_err == WSAENETDOWN) { /* network subsystem has
                                          * failed */
            ret = OPENSOAP_IO_ERROR;
        }
        else if (c_err == WSAENOTSOCK) { /* descriptor is not a socket */
            ret = OPENSOAP_IO_ERROR;
        }
        else if (c_err == WSAEINPROGRESS) { /* in progress */
            ret = OPENSOAP_IO_ERROR;
        }
        else if (c_err == WSAEINTR) { /* close interrupted */
            ret = OPENSOAP_IO_ERROR;
        }
        else if (c_err == WSAEWOULDBLOCK) { /* */
            ret = OPENSOAP_IO_ERROR;
        }
        else {
            ret = OPENSOAP_IO_ERROR;
        }
    }

    return ret;
}

/* Winsock initialize */
static
const WORD
winsockRequestVersion = MAKEWORD(1,1);

typedef struct {
    OpenSOAPObject super;
    int errorWSAStartup;
    WSADATA wsaData;
} OpenSOAPSocketSystem;

typedef OpenSOAPSocketSystem *OpenSOAPSocketSystemPtr;

/* Socket system */
static
OpenSOAPSocketSystemPtr
theSocketSystem = NULL;

static
int
OpenSOAPSocketSystemFree(OpenSOAPObjectPtr obj) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (obj) {
        OpenSOAPSocketSystemPtr socketSystem = (OpenSOAPSocketSystemPtr)obj;
        if (!socketSystem->errorWSAStartup) {
            WSACleanup();
        }
		/* */
        ret = OpenSOAPObjectReleaseMembers(obj);
        if (OPENSOAP_SUCCEEDED(ret)) {
            free(obj);
            theSocketSystem = NULL;
        }
    }

    return ret;
}

static
int
OpenSOAPSocketSystemCreate(/* [out] */ OpenSOAPSocketSystemPtr *socketSystem) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (socketSystem) {
        ret = OPENSOAP_MEM_BADALLOC;
        *socketSystem = malloc(sizeof(OpenSOAPSocketSystem));
        if (*socketSystem) {
            ret = OpenSOAPObjectInitialize((OpenSOAPObjectPtr)*socketSystem,
                                           OpenSOAPSocketSystemFree);
            if (OPENSOAP_SUCCEEDED(ret)) {
                /* Initialize WinSock.dll */
                (*socketSystem)->errorWSAStartup
                    = WSAStartup(winsockRequestVersion,
                                 &(*socketSystem)->wsaData);
                if ((*socketSystem)->errorWSAStartup) {
                    /* Winsock Initialize error */
                    int wsa_error
                        = (*socketSystem)->errorWSAStartup;
                    (*socketSystem)->errorWSAStartup = 0;
                    switch (wsa_error) {
                      case WSASYSNOTREADY:
                          ret = OPENSOAP_IO_ERROR;
                          break;
                      case WSAVERNOTSUPPORTED:
                          ret = OPENSOAP_IO_ERROR;
                          break;
                      case WSAEINPROGRESS:
                          ret = OPENSOAP_IO_ERROR;
                          break;
                      case WSAEPROCLIM:
                          ret = OPENSOAP_IO_ERROR;
                          break;
#if defined(WSAFAULT)
                      case WSAFAULT:
                          ret = OPENSOAP_IO_ERROR;
                          break;
#endif /* WSAFAULT */
                      default:
                          ret = OPENSOAP_IO_ERROR;
                          break;
                    }
                }
                else if ((*socketSystem)->wsaData.wVersion
						 != winsockRequestVersion) {
                    /* winsock version mismatch */
                    ret = OPENSOAP_IO_ERROR;
                }
                if (OPENSOAP_FAILED(ret)) {
                    OpenSOAPSocketSystemFree((OpenSOAPObjectPtr)*socketSystem);
                    *socketSystem = NULL;
                }
            }
            else {
                free(*socketSystem);
                *socketSystem = NULL;
            }
        }
    }

    return ret;
}

/* Socket system lock */
static
CRITICAL_SECTION
theSocketSystemLock;

/* Socket system lock flag */
static
int
isSocketSystemLockValid = 0;

extern
void
OpenSOAPSocketSystemLockInitialize() {
	if (!isSocketSystemLockValid) {
		InitializeCriticalSection(&theSocketSystemLock);
		isSocketSystemLockValid = 0;
	}
}

extern
void
OpenSOAPSocketSystemUltimate() {
    if (theSocketSystem) {
        OpenSOAPObjectRelease((OpenSOAPObjectPtr)theSocketSystem);
    }
	if (isSocketSystemLockValid) {
		/* delete Critical Section */
		DeleteCriticalSection(&theSocketSystemLock);
		isSocketSystemLockValid = 0;
	}	
}

extern
void
OpenSOAPSocketSystemLock() {
	if (isSocketSystemLockValid) {
		EnterCriticalSection(&theSocketSystemLock);
	}
}

extern
void
OpenSOAPSocketSystemUnlock() {
	if (isSocketSystemLockValid) {
		LeaveCriticalSection(&theSocketSystemLock);
	}
}

static
int
OpenSOAPSocketSystemInitialize() {
    int ret = OPENSOAP_NO_ERROR;

	OpenSOAPSocketSystemLock();
	if (!theSocketSystem) {
		ret = OpenSOAPSocketSystemCreate(&theSocketSystem);
	}
	OpenSOAPSocketSystemUnlock();

    return ret;
}

#else
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define SOCKET_INITIALIZE(s) ((s) = -1)
#define IS_VALID_SOCKET(s)  ((s) != -1)

static
int
CLOSE_SOCKET(SOCKET_TYPE s) {
    int ret = OPENSOAP_NO_ERROR;

    if (close(s)) {
        int c_err = errno;
        if (c_err == EBADF) { /* invalid fd */
            ret = OPENSOAP_IO_ERROR;
        }
        else if (c_err == EINTR) { /* close interrupt */
            ret = OPENSOAP_IO_ERROR;
        }
        else if (c_err == EIO) { /* I/O error */
            ret = OPENSOAP_IO_ERROR;
        }
        else {
            ret = OPENSOAP_IO_ERROR;
        }
    }

    return ret;
}

#ifdef HAVE_PTHREAD

/* Socket system lock */
static
pthread_mutex_t
theSocketSystemLock;

/* Socket system lock flag */
static
int
isSocketSystemLockValid = 0;

extern
void
OpenSOAPSocketSystemLockInitialize() {
	if (!isSocketSystemLockValid) {
		/* mutex initialize */
		pthread_mutex_init(&theSocketSystemLock, NULL);
		isSocketSystemLockValid = 1;
	}
}

extern
void
OpenSOAPSocketSystemUltimate() {
	if (isSocketSystemLockValid) {
		/* mutex delete */
		pthread_mutex_destroy(&theSocketSystemLock);
		isSocketSystemLockValid = 0;
	}	
}

extern
void
OpenSOAPSocketSystemLock() {
	/* lock */
	pthread_mutex_lock(&theSocketSystemLock);
}

extern
void
OpenSOAPSocketSystemUnlock() {
	/* unlock */
	pthread_mutex_unlock(&theSocketSystemLock);
}

#else /* HAVE_PTHREAD */

extern
void
OpenSOAPSocketSystemLockInitialize() {
}

extern
void
OpenSOAPSocketSystemUltimate() {
}

extern
void
OpenSOAPSocketSystemLock() {
}

extern
void
OpenSOAPSocketSystemUnlock() {
}

#endif /* HAVE_PTHREAD */

static
int
OpenSOAPSocketSystemInitialize() {
    return OPENSOAP_NO_ERROR;
}

#endif

/*
=begin
--- function#OpenSOAPSocketRelease(s)
    Release OpenSOAP Socket

    :Parameters
      :OpenSOAPSocketPtr  [in] ((|s|))
        OpenSOAP Socket
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPSocketReleaseMembers(OpenSOAPSocketPtr s) {
    int ret = OpenSOAPSocketClose(s);

    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPStreamReleaseMembers((OpenSOAPStreamPtr)s);
    }

    return ret;
}

static
int
OpenSOAPSocketFree(OpenSOAPObjectPtr obj) {
    OpenSOAPSocketPtr s = (OpenSOAPSocketPtr)obj;
    int ret = OpenSOAPSocketReleaseMembers(s);

    if (OPENSOAP_SUCCEEDED(ret)) {
        free(obj);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPSocketReadImpl(st, buf, buf_sz)
    Read from OpenSOAP Socket

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|st|))
        OpenSOAP Socket
      :unsigned char * [out] buf,
        Pointer to reading buffer
      :size_t * [in, out] buf_sz
        Size of reading buffer.  Returns the size of data actually read.
    :Returns
      :int
      
=end
 */
static
int
OpenSOAPSocketReadImpl(OpenSOAPStreamPtr /* [in] */ st,
                       unsigned char * /* [out] */ buf,
                       size_t * /* [in, out] */ buf_sz) {
    OpenSOAPSocketPtr s = (OpenSOAPSocketPtr)st;
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s && buf && buf_sz && *buf_sz) {
        ret = OPENSOAP_IO_ERROR; /* not open socket */
        if (IS_VALID_SOCKET(s->socket)) {
            unsigned char *buf_end = buf + *buf_sz;
            unsigned char *buf_i = buf;

            ret = OPENSOAP_NO_ERROR;
            while (buf_i < buf_end) {
                int recv_sz
                    = recv(s->socket,
                           buf_i,
                           buf_end - buf_i,
                           0);
                if (recv_sz < 0) {
                    ret = OPENSOAP_IO_ERROR;
                    break;
                }
                else if (recv_sz == 0) {
                    break;
                }
                buf_i += recv_sz;
            }
            if (OPENSOAP_SUCCEEDED(ret)) {
                *buf_sz = buf_i - buf;
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPSocketWriteImpl(st, buf, buf_sz)
    Write to OpenSOAP Socket

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|st|))
        OpenSOAP Socket
      :const unsigned char * [in] buf,
        Pointer to writing data buffer
      :size_t * [in, out] buf_sz
        Size of writing data.
        After called, returns the size of data actually written.
    :Returns
      :int
      
=end
 */
static
int
OpenSOAPSocketWriteImpl(OpenSOAPStreamPtr /* [in] */ st,
                    const unsigned char * /* [in] */ buf,
                    size_t * /* [in, out] */ buf_sz) {
    OpenSOAPSocketPtr s = (OpenSOAPSocketPtr)st;
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s && buf && buf_sz && *buf_sz) {
        ret = OPENSOAP_IO_ERROR; /* not open socket */
        if (IS_VALID_SOCKET(s->socket)) {
            const unsigned char *buf_end = buf + *buf_sz;
            const unsigned char *buf_i = buf;
            ret = OPENSOAP_NO_ERROR;
            while (buf_i < buf_end) {
                int send_sz = send(s->socket,
                                   buf_i,
                                   buf_end - buf_i,
                                   0);
                if (send_sz < 0) {
                    ret = OPENSOAP_IO_ERROR;
                    break;
                }
                else if (send_sz == 0) {
                    break;
                }
                buf_i += send_sz;
            }
            if (OPENSOAP_SUCCEEDED(ret)) {
                *buf_sz = buf_i - buf;
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPSocketReadImplSecure(st, buf, buf_sz)
    Read from OpenSOAP Socket (SSL)

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|st|))
        OpenSOAP Socket
      :unsigned char * [out] buf,
        Pointer to reading buffer
      :size_t * [in, out] buf_sz
        Size of reading buffer.
        After called, returns the size actually read.
    :Returns
      :int
      
=end
 */
extern
int
OpenSOAPSocketReadImplSecure(OpenSOAPStreamPtr /* [in] */ st,
                       unsigned char * /* [out] */ buf,
                       size_t * /* [in, out] */ buf_sz) {
#ifdef HAVE_SSL
    OpenSOAPSocketPtr s = (OpenSOAPSocketPtr)st;
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s && buf && buf_sz && *buf_sz) {
        ret = OPENSOAP_IO_ERROR; /* not open socket */
        if (s->ssl.use != 0 && s->ssl.ssl) {
            unsigned char *buf_end = buf + *buf_sz;
            unsigned char *buf_i = buf;

            ret = OPENSOAP_NO_ERROR;
            while (buf_i < buf_end) {
                int recv_sz
                    = SSL_read(s->ssl.ssl,
                               buf_i,
                               buf_end - buf_i);
                if (recv_sz < 0) {
                    ret = OPENSOAP_IO_ERROR;
                    break;
                }
                else if (recv_sz == 0) {
                    break;
                }
                buf_i += recv_sz;
            }
            if (OPENSOAP_SUCCEEDED(ret)) {
                *buf_sz = buf_i - buf;
            }
        }
    }

    return ret;
#else /* ! HAVE_SSL */
    return OPENSOAP_UNSUPPORT_PROTOCOL;
#endif
}

/*
=begin
--- function#OpenSOAPSocketWriteImplSecure(st, buf, buf_sz)
    Write to OpenSOAP Socket (SSL)

    :Parameters
      :OpenSOAPStreamPtr  [in] ((|st|))
        OpenSOAP Socket
      :const unsigned char * [in] buf,
        Pointer to writing data buffer.
      :size_t * [in, out] buf_sz
        Size of writing data.
        After called, returns the size of data actually written.
    :Returns
      :int
      
=end
 */
extern
int
OpenSOAPSocketWriteImplSecure(OpenSOAPStreamPtr /* [in] */ st,
                    const unsigned char * /* [in] */ buf,
                    size_t * /* [in, out] */ buf_sz) {
#ifdef HAVE_SSL
    OpenSOAPSocketPtr s = (OpenSOAPSocketPtr)st;
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s && buf && buf_sz && *buf_sz) {
        ret = OPENSOAP_IO_ERROR; /* not open socket */
        if (s->ssl.use != 0 && s->ssl.ssl) {
            const unsigned char *buf_end = buf + *buf_sz;
            const unsigned char *buf_i = buf;
            ret = OPENSOAP_NO_ERROR;
            while (buf_i < buf_end) {
                int send_sz = SSL_write(s->ssl.ssl,
                                        buf_i,
                                        buf_end - buf_i);
                if (send_sz < 0) {
                    ret = OPENSOAP_IO_ERROR;
                    break;
                }
                else if (send_sz == 0) {
                    break;
                }
                buf_i += send_sz;
            }
            if (OPENSOAP_SUCCEEDED(ret)) {
                *buf_sz = buf_i - buf;
            }
        }
    }

    return ret;
#else /* ! HAVE_SSL */
    return OPENSOAP_UNSUPPORT_PROTOCOL;
#endif /* HAVE_SSL */
}

/*
=begin
--- function#OpenSOAPSocketInitialize(s, free_func, read_func, write_func)
    OpenSOAP Socket member initialize.

    :Parameters
      :OpenSOAPSocketPtr * [out] ((|s|))
        OpenSOAP Socket pointer
      :OpenSOAPObjectFreeFunc [in] ((|free_func|))
        OpenSOAP Object free function
      :OpenSOAPStreamReadFunc [in] ((|read_func|))
        OpenSOAP Stream read function.
        If NULL, use default read function.
      :OpenSOAPStreamWriteFunc [in] ((|write_func|))
        OpenSOAPStream write function.
        If NULL, use default write function.
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPSocketInitialize(OpenSOAPSocketPtr /* [in, out] */ s,
                         OpenSOAPObjectFreeFunc /* [in] */ free_func,
                         OpenSOAPStreamReadFunc  /* [in] */ read_func,
                         OpenSOAPStreamWriteFunc /* [in] */ write_func) {
    int ret = OpenSOAPSocketSystemInitialize();
    
    if (OPENSOAP_SUCCEEDED(ret)) {
        OpenSOAPStreamPtr st = (OpenSOAPStreamPtr)s;
        if (!read_func) {
            read_func = OpenSOAPSocketReadImpl;
        }
        if (!write_func) {
            write_func = OpenSOAPSocketWriteImpl;
        }
        ret = OpenSOAPStreamInitialize(st,
                                       free_func,
                                       read_func,
                                       write_func);
        if (OPENSOAP_SUCCEEDED(ret)) {
            SOCKET_INITIALIZE(s->socket);
        }
    }
    s->ssl.use = 0;
#ifdef HAVE_SSL
    s->ssl.bio = NULL;
    s->ssl.ssl = NULL;
    s->ssl.ctx = NULL;
#endif /* HAVE_SSL */
    s->ssl.version = 0;
    s->ssl.ca_file = NULL;
    s->ssl.ca_dir = NULL;
    s->ssl.certchain_file = NULL;
    s->ssl.privkey_file = NULL;
    s->ssl.verify_level = 0;
    
    return ret;
}

/*
=begin
--- function#OpenSOAPSocketCreate(s)
    OpenSOAP Socket instance create

    :Parameters
      :OpenSOAPSocketPtr * [out] ((|s|))
        OpenSOAP Socket pointer
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPSocketCreate(OpenSOAPSocketPtr * /* [out] */ s) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s) {
        *s = malloc(sizeof(OpenSOAPSocket));
        if (*s) {
            ret = OpenSOAPSocketInitialize(*s,
                                           OpenSOAPSocketFree,
                                           NULL, NULL);
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
--- function#OpenSOAPSocketRelease(s)
    Release OpenSOAP Socket

    :Parameters
      :OpenSOAPSocketPtr  [in] ((|s|))
        OpenSOAP Socket
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPSocketRelease(OpenSOAPSocketPtr /* [in] */ s) {
    return OpenSOAPObjectRelease((OpenSOAPObjectPtr)s);
}

/*
=begin
--- function#OpenSOAPSocketOpen(s, node, service)
    Open OpenSOAP Socket

    :Parameters
      :OpenSOAPSocketPtr  [in] ((|s|))
        OpenSOAP Socket
      :int [in] domain
        Domain of communication
      :int [in] type
        Semantics type of communication
      :int [in] protocol
        Protocol used by Socket
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPSocketOpen(OpenSOAPSocketPtr /* [in, out] */ s,
                   int /* [in] */ domain,
                   int /* [in] */ type,
                   int /* [in] */ protocol) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s) {
        SOCKET_TYPE soc = socket(domain, type, protocol);
        ret = OPENSOAP_IO_ERROR;
        if (IS_VALID_SOCKET(soc)) {
            s->socket = soc;
            ret = OPENSOAP_NO_ERROR;
        }
    }

    return ret;
}

#if 1
int verify_callback(int ok, X509_STORE_CTX *store)
{
#ifdef _DEBUG
    char data[256];
 
    if (!ok)
    {
        X509 *cert = X509_STORE_CTX_get_current_cert(store);
        int  depth = X509_STORE_CTX_get_error_depth(store);
        int  err = X509_STORE_CTX_get_error(store);

        fprintf(stderr, "-Error with certificate at depth: %i\n", depth);
        X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
        fprintf(stderr, "  issuer   = %s\n", data);
        X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
        fprintf(stderr, "  subject  = %s\n", data);
        fprintf(stderr, "  err %i:%s\n", err, X509_verify_cert_error_string(err));
    }
#endif /* _DEBUG */
 
    return ok;
}
#endif

static
int
OpenSOAPSocketSSLConnect(OpenSOAPSocketPtr /* [in, out] */ s) {
    int ret = OPENSOAP_NO_ERROR;
#ifdef HAVE_SSL
    OpenSOAPSecureSocketPtr ss = &(s->ssl);
    long opt = SSL_OP_ALL;
    int err = 0;
    /* SSL library initialize checking (or, should be global ?) */
    static int opensoap_ssl_library_init = 0;

    if (! opensoap_ssl_library_init) {
      SSL_library_init();
      opensoap_ssl_library_init = 1;
    }
    err = (int)(ss->ctx = SSL_CTX_new(SSLv23_client_method()));
    if (! err) {
        return OPENSOAP_TRANSPORT_SSL_VERSION_ERROR;
    }
    /* set SSL version options */
    SSL_CTX_set_options(ss->ctx, opt);
    if (ss->version != OPENSOAP_SSL_VER_ALL) {
        opt |=
            ((ss->version & OPENSOAP_SSL_VER_SSL2) ?
             0 : SSL_OP_NO_SSLv2) |
            ((ss->version & OPENSOAP_SSL_VER_SSL3) ?
             0 : SSL_OP_NO_SSLv3) |
            ((ss->version & OPENSOAP_SSL_VER_TLS1) ?
             0 : SSL_OP_NO_TLSv1);
    }
    SSL_CTX_set_options(ss->ctx, opt);

    /* set verification settings */
    if ((ss->ca_file || ss->ca_dir) &&
        SSL_CTX_load_verify_locations(ss->ctx,ss->ca_file, ss->ca_dir) != 1) {
#ifdef _DEBUG
        fprintf(stderr, "Error loading CA file[%s] and/or directory[%s]\n",
                ss->ca_file, ss->ca_dir);
#endif /* _DEBUG */
        return OPENSOAP_TRANSPORT_SSL_CERTFILE_ERROR;
    }
    if (SSL_CTX_set_default_verify_paths(ss->ctx) != 1) {
#ifdef _DEBUG
        fprintf(stderr, "Error loading default CA file and/or directory\n");
#endif /* _DEBUG */
        return OPENSOAP_TRANSPORT_SSL_CERTFILE_ERROR;
    }
    if (ss->certchain_file) {
        char *key;
        if (SSL_CTX_use_certificate_chain_file(ss->ctx, ss->certchain_file) != 1) {
#ifdef _DEBUG
            fprintf(stderr, "Error loading certificate from file[%s]\n",
                    ss->certchain_file);
#endif /* _DEBUG */
            return OPENSOAP_TRANSPORT_SSL_CERTFILE_ERROR;
        }
        key = (ss->privkey_file ?
               ss->privkey_file :
               ss->certchain_file);
        if (SSL_CTX_use_PrivateKey_file(ss->ctx, key, SSL_FILETYPE_PEM) != 1) {
#ifdef _DEBUG
            fprintf(stderr, "Error loading private key from file[%s]\n", key);
#endif /* _DEBUG */
            return OPENSOAP_TRANSPORT_SSL_CERTFILE_ERROR;
        }
    }
    SSL_CTX_set_verify(ss->ctx,
                       (ss->verify_level ?
                        SSL_VERIFY_PEER :
                        SSL_VERIFY_NONE),
                       verify_callback);
            
    SSL_CTX_set_verify_depth(ss->ctx, 9);
            
    if ((ss->ssl = SSL_new(ss->ctx)) == NULL) {
        return OPENSOAP_TRANSPORT_SSL_VERSION_ERROR;
    }
    err = SSL_set_fd(ss->ssl, s->socket);
    if (err == 0) {
        return OPENSOAP_TRANSPORT_SSL_VERSION_ERROR;
    }
    err = SSL_connect(ss->ssl);
    if (err <= 0) {
#ifdef _DEBUG
        unsigned long error = ERR_get_error();
        char e[256];
        int e_lib = ERR_GET_LIB(error);
        int e_func = ERR_GET_FUNC(error);
        int e_reason = ERR_GET_REASON(error);
        SSL_load_error_strings();
        ERR_error_string(error, e);
        fprintf(stderr, "\nSSL-ERROR:%08lx[%s](%d,%d,%d)\n", error, e, e_lib, e_func, e_reason);
#endif /* _DEBUG */
        return (OPENSOAP_TRANSPORT_SSL_CERTFILE_ERROR);
    }
    ret = OPENSOAP_NO_ERROR;
#else /* !HAVE_SSL */
    ret = OPENSOAP_UNSUPPORT_PROTOCOL;
#endif /* HAVE_SSL */
    return ret;
}

/*
=begin
--- function#OpenSOAPSocketConnect(s, node, service)
    Connect OpenSOAP Socket
                      
    :Parameters
      :OpenSOAPSocketPtr  [in] ((|s|))
        OpenSOAP Socket
      :const struct sockaddr * [in] serv_addr,
        server address
      :size_t [in] addrlen
        address length
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPSocketConnect(OpenSOAPSocketPtr /* [in, out] */ s,
                      const struct sockaddr * /* [in] */ serv_addr,
                      size_t /* [in] */ addrlen) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s) {
        ret = OPENSOAP_IO_ERROR; /* socket not open */
        if (IS_VALID_SOCKET(s->socket)) {
            if (!connect(s->socket, serv_addr, addrlen)) {
                /* Connect OK */
                ret = OPENSOAP_NO_ERROR;
            } else {
                /* Connect Error */
#if (!defined(_WIN32)) && defined(_DEBUG)
                char * se = (char *)strerror(errno);
                char *a1;
                a1 = inet_ntoa(((struct sockaddr_in *)serv_addr)->sin_addr);
                fprintf(stderr, "connect to address %s: %s (%d)\n", a1, se, errno);
#endif
#ifndef _WIN32
                switch (errno) {
                    case ECONNREFUSED:
                        /* No one listening on the remote address. */
                        ret = OPENSOAP_TRANSPORT_CONNECTION_REFUSED;
                        break;
                    case ETIMEDOUT:
                        /* Timeout  while attempting connection. The
                         * server may be too busy to accept new
                         * connections. Note that for IP sockets the
                         * timeout may be very long when syncookies
                         * are enabled on the server.
                         */
                        ret = OPENSOAP_TRANSPORT_CONNECTION_TIMEOUT;
                        break;
                    case ENETUNREACH:
                        /* Network is unreachable. */
                        ret = OPENSOAP_TRANSPORT_NETWORK_UNREACH;
                        break;
                    case EHOSTUNREACH:
                        /* Host is unreachable */
                        ret = OPENSOAP_TRANSPORT_HOST_UNREACH;
                        break;
                    default:
                        /* Unknown */
                        ret = OPENSOAP_IO_ERROR;
				}
#else /* Please set a proper error code for WIN32 */
				ret = OPENSOAP_IO_ERROR;
#endif  /* ! _WIN32 */
            }
        }
        if (OPENSOAP_SUCCEEDED(ret) && s->ssl.use) {
            ret = OpenSOAPSocketSSLConnect(s);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPSocketClose(s)
    Close OpenSOAP Socket.  Also disconnect.

    :Parameters
      :OpenSOAPSocketPtr  [in] ((|s|))
        OpenSOAP Socket
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPSocketClose(OpenSOAPSocketPtr /* [in] */ s) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (s) {
        ret = OPENSOAP_NO_ERROR;
        if (IS_VALID_SOCKET(s->socket)) {
            ret = CLOSE_SOCKET(s->socket);
            SOCKET_INITIALIZE(s->socket);
        }

#ifdef HAVE_SSL
        if (s->ssl.ssl) {
            SSL_free(s->ssl.ssl);
            s->ssl.ssl = NULL;
        }
        if (s->ssl.ctx) {
            SSL_CTX_free(s->ssl.ctx);
            s->ssl.ctx = NULL;
        }
        if (s->ssl.bio) {
            BIO_free(s->ssl.bio);
            s->ssl.bio = NULL;
        }
#endif
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPSocketRead(s, buf, buf_sz)
    Read from OpenSOAP Socket

    :Parameters
      :OpenSOAPSocketPtr  [in] ((|s|))
        OpenSOAP Socket
      :unsigned char * [out] buf,
        Pointer to reading buffer
      :size_t * [in, out] buf_sz
        Size of reading buffer.  Returns the size of data actually read.
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPSocketRead(OpenSOAPSocketPtr /* [in] */ s,
                   unsigned char * /* [out] */ buf,
                   size_t * /* [in, out] */ buf_sz) {
    int ret = OpenSOAPStreamRead((OpenSOAPStreamPtr)s,
                                 buf,
                                 buf_sz);

    return ret;
}

/*
=begin
--- function#OpenSOAPSocketWrite(s, buf, buf_sz)
    Write to OpenSOAP Socket

    :Parameters
      :OpenSOAPSocketPtr  [in] ((|s|))
        OpenSOAP Socket
      :const unsigned char * [in] buf,
        Pointer to writing data buffer
      :size_t * [in, out] buf_sz
        Size of writing data.
        After called, returns the size of data actually written.
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPSocketWrite(OpenSOAPSocketPtr /* [in] */ s,
                    const unsigned char * /* [in] */ buf,
                    size_t * /* [in, out] */ buf_sz) {
    int ret = OpenSOAPStreamWrite((OpenSOAPStreamPtr)s,
                                  buf,
                                  buf_sz);

    return ret;
}

/*
=begin
--- function#OpenSOAPSocketReadToDelm(s, delm, delm_sz, buf)
    Read at delimiter from OpenSOAP ClientSocket.

    :Parameters
      :OpenSOAPSocketPtr  [in] ((|s|))
        OpenSOAP Socket
      :const unsigned char * [in] delm
        Delimiter data array.
      :size_t [in] delm_sz
        Delimiter data size.
      :OpenSOAPByteArrayPtr [out] buf
        Reading Buffer
    :Returns
      :int
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPSocketReadToDelm(OpenSOAPSocketPtr /* [in] */ s,
                         const unsigned char * /* [in] */ delm,
                         size_t /* [in] */ delm_sz,
                         OpenSOAPByteArrayPtr /* [out] */ buf) {
    int ret = OpenSOAPStreamReadToDelm((OpenSOAPStreamPtr)s,
                                       delm,
                                       delm_sz,
                                       buf);

    return ret;
}

