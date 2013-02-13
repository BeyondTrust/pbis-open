/*-----------------------------------------------------------------------------
 * $RCSfile: Service.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: Service.c,v 1.18 2003/11/20 07:03:20 okada Exp $";
#endif  /* _DEBUG */

#include <OpenSOAP/CStdio.h>

#include "Service.h"

#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/*
=begin
= OpenSOAP ServiceConnecter class
=end
 */

typedef struct tagOpenSOAPServiceConnecterFuncTable
OpenSOAPServiceConnecterFuncTable;
typedef OpenSOAPServiceConnecterFuncTable
*OpenSOAPServiceConnecterFuncTablePtr;

struct tagOpenSOAPServiceConnecterFuncTable {
    const char *connectType;
    int (*initializer)(/* [out] */ OpenSOAPServiceConnecterPtr c,
                       /* [in]  */ va_list ap);
                   
    int (*receiveHeader)(/* [in]  */ OpenSOAPServiceConnecterPtr c,
                         /* [out] */ size_t *content_length,
                         /* [out] */ OpenSOAPByteArrayPtr charset);
    
    int (*receive)(/* [in]  */ OpenSOAPServiceConnecterPtr c,
                   /* [in]  */ size_t content_length,
                   /* [out] */ OpenSOAPByteArrayPtr request);

    int (*send)(/* [in] */ OpenSOAPServiceConnecterPtr c,
                /* [in] */ OpenSOAPByteArrayPtr charset,
                /* [in] */ OpenSOAPByteArrayPtr response);

    int canLoop;
};

struct tagOpenSOAPServiceConnecter {
    OpenSOAPObject      super;

    OpenSOAPStreamPtr   serviceStream;
    const OpenSOAPServiceConnecterFuncTable *funcTable;
};

static
int
OpenSOAPServiceConnecterFree(/* [in] */ OpenSOAPObjectPtr obj) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    OpenSOAPServiceConnecterPtr c = (OpenSOAPServiceConnecterPtr)obj;

    if (c) {
        OpenSOAPStreamRelease(c->serviceStream);
        ret = OpenSOAPObjectReleaseMembers(obj);
        if (OPENSOAP_SUCCEEDED(ret)) {
            free(c);
        }
    }

    return ret;
}

static
int
OpenSOAPServiceConnecterStdioInitialize(/* [out] */ OpenSOAPServiceConnecterPtr
                                        c,
                                        /* [in]  */ va_list ap) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (c) {
        OpenSOAPCStdioPtr   serviceStream = NULL;
        ret = OpenSOAPCStdioCreateForStdio(&serviceStream);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPCStdioSetBinaryMode(serviceStream);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPObjectInitialize((OpenSOAPObjectPtr)c,
                                               OpenSOAPServiceConnecterFree);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    c->serviceStream = (OpenSOAPStreamPtr)serviceStream;
                }
            }
            if (OPENSOAP_FAILED(ret)) {
                OpenSOAPCStdioRelease(serviceStream);
            }
        }
    }

    return ret;
}

static
int
OpenSOAPServiceConnecterGetCharset(/* [in]  */ const unsigned char *
                                   content_type_val,
                                   /* [in]  */ size_t content_type_sz,
                                   /* [out] */ OpenSOAPByteArrayPtr charset) {
    static const unsigned char NULL_CHAR = '\0';
    static const char QUOTATIONS[] = "\'\"";
    static const char CHARSET_HEADER[] = "charset=";
    static const size_t CHARSET_HEADER_SIZE = sizeof(CHARSET_HEADER) - 1;
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (charset) {
        if (content_type_val) {
            unsigned char *buf = malloc(content_type_sz + 1);
            ret = OPENSOAP_MEM_BADALLOC;
            if (buf) {
                unsigned char *charset_beg = NULL;
                unsigned char *buf_end = buf + content_type_sz;
                strncpy(buf, content_type_val, content_type_sz);
                *buf_end = NULL_CHAR;
                charset_beg = strstr(buf,
                                     CHARSET_HEADER);
                if (charset_beg) {
                    size_t charset_sz = 0;
                    unsigned char *c_i = NULL;
                    const unsigned char *q_i = QUOTATIONS;
                    
                    charset_beg += CHARSET_HEADER_SIZE;
                    for (; *q_i && *q_i != *charset_beg; ++q_i) {
                    }
                    if (*q_i) {
                        ++charset_beg;
                        for (c_i = charset_beg;
                             c_i != buf_end && *c_i != *q_i;
                             ++c_i) {
                        }
                    }
                    else {
                        for (c_i = charset_beg;
                             c_i != buf_end && !isspace(*c_i);
                             ++c_i) {
                        }
                    }
                    *c_i++ = NULL_CHAR;
                    charset_sz = c_i - charset_beg;
                    ret = OpenSOAPByteArraySetData(charset,
                                                   charset_beg,
                                                   charset_sz);
                }
                else {
                    content_type_val = NULL;
                }

                free(buf);
            }
        }
        if (!content_type_val) {
            ret = OpenSOAPByteArraySetData(charset,
                                           &NULL_CHAR,
                                           1);
        }
    }

    return ret;
}
                                   

static
int
OpenSOAPServiceConnecterCGIReceiveHeader(OpenSOAPServiceConnecterPtr
                                         /* [in] */ c,
                                         size_t *
                                         /* [out] */ content_length,
                                         OpenSOAPByteArrayPtr
                                         /* [out] */ charset) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (c && content_length && charset) {
        char *c_l_env = getenv("CONTENT_LENGTH");
        char *c_t_env = getenv("CONTENT_TYPE");
        size_t c_t_sz = (c_t_env) ? strlen(c_t_env) : 0;

        *content_length = (c_l_env) ? atol(c_l_env) : 0;

        ret = OpenSOAPServiceConnecterGetCharset(c_t_env,
                                                 c_t_sz,
                                                 charset);
    }

    return ret;
}

static
int
OpenSOAPServiceConnecterStdioReceiveHeader(/* [in]  */
										   OpenSOAPServiceConnecterPtr c,
										   /* [out] */ size_t *content_length,
										   /* [out] */ OpenSOAPByteArrayPtr
										   charset) {
    const unsigned char NULL_CHAR = '\0';
    static const char QUOTATIONS[] = "\'\"";
    static const char HTTP_HEADER_TERM[] = "\r\n\r\n";
    static const size_t HTTP_HEADER_TERM_SIZE = 4;
    static const char CONTENT_LENGTH_HEADER[] = "\r\nContent-Length:";
    static const size_t CONTENT_LENGTH_HEADER_SIZE
        = sizeof(CONTENT_LENGTH_HEADER) - 1;
    static const char CHARSET_HEADER[] = "charset=";
    static const size_t CHARSET_HEADER_SIZE = sizeof(CHARSET_HEADER) - 1;
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (c && content_length && charset) {
        OpenSOAPByteArrayPtr http_hdr = NULL;
        ret = OpenSOAPByteArrayCreate(&http_hdr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPStreamReadToDelm(c->serviceStream,
                                           HTTP_HEADER_TERM,
                                           HTTP_HEADER_TERM_SIZE,
                                           http_hdr);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPByteArrayAppend(http_hdr,
                                              &NULL_CHAR,
                                              1);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    const unsigned char *http_hdr_beg = NULL;
                    size_t http_hdr_sz = 0;
                    ret = OpenSOAPByteArrayGetBeginSizeConst(http_hdr,
                                                             &http_hdr_beg,
                                                             &http_hdr_sz);
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        const char *charset_beg = NULL;
                        size_t charset_sz = 0;
                        const char *cl_i
                            = strstr(http_hdr_beg,
                                     CONTENT_LENGTH_HEADER);
                        if (cl_i) {
                            *content_length
                                = atol(cl_i + CONTENT_LENGTH_HEADER_SIZE);
                        }
                        else {
                            *content_length = 0;
                        }
                        charset_beg = strstr(http_hdr_beg,
                                             CHARSET_HEADER);
                        if (charset_beg) {
                            charset_beg += CHARSET_HEADER_SIZE;
                            for (cl_i = QUOTATIONS;
                                 *cl_i && *cl_i != *charset_beg;
                                 ++cl_i) {
                            }
                        
                            if (*cl_i) {
                                char quate = *cl_i;
                                ++charset_beg;
                                for (cl_i = charset_beg;
                                     *cl_i != quate;
                                     ++cl_i) {
                                }
                            }
                            else {
                                for (cl_i = charset_beg;
                                     !isspace(*cl_i);
                                     ++cl_i) {
                                }
                            }
                            charset_sz = cl_i - charset_beg;
                            ret = OpenSOAPByteArraySetData(charset,
                                                           charset_beg,
                                                           charset_sz);
                        }
                        else {
                            ret = OpenSOAPByteArrayClear(charset);
                        }
                        if (OPENSOAP_SUCCEEDED(ret)) {
                            ret = OpenSOAPByteArrayAppend(charset,
                                                          &NULL_CHAR,
                                                          1);
                        }
                    }
                }
            }

            OpenSOAPByteArrayRelease(http_hdr);
        }   
    }

    return ret;
}

static
int
OpenSOAPServiceConnecterStdioReceive(OpenSOAPServiceConnecterPtr
                                    /* [in] */ c,
                                    size_t
                                    /* [in] */ content_length,
                                    OpenSOAPByteArrayPtr
                                    /* [out] */ request) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (c) {
        ret = OpenSOAPStreamReadWithSize(c->serviceStream,
                                         content_length,
                                         request);
    }

    return ret;
}

#if defined(NONGCC_WIN32)
# define STRING_FORMAT L"%hs"
#else /* !NONGCC_WIN32 */
# define STRING_FORMAT L"%s"
#endif /* !NONGCC_WIN32 */

static
int
OpenSOAPHTTPMessageCreate(/* [in]  */ OpenSOAPByteArrayPtr http_body,
                          /* [in]  */ OpenSOAPByteArrayPtr charset,
                          /* [out] */ OpenSOAPByteArrayPtr *http_msg) {
    static const wchar_t HTTP_HEADER_FORMAT[]
        = L"Content-Type: text/xml; charset=\""
        STRING_FORMAT
        L"\"\r\n"
        L"Content-Length: %d\r\n\r\n";
    static const wchar_t HTTP_HEADER_NOCHARSET_FORMAT[]
        = L"Content-Type: text/xml\r\n"
        L"Content-Length: %d\r\n\r\n";
    
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (http_msg && http_body) {
        const unsigned char *body_beg = NULL;
        size_t body_sz = 0;
        ret = OpenSOAPByteArrayGetBeginSizeConst(http_body,
                                                 &body_beg,
                                                 &body_sz);
        if (OPENSOAP_SUCCEEDED(ret)) {
            const unsigned char *charset_beg = NULL;
            size_t charset_sz = 0;
            if (charset) {
                ret = OpenSOAPByteArrayGetBeginSizeConst(charset,
                                                         &charset_beg,
                                                         &charset_sz);
            }
            if (OPENSOAP_SUCCEEDED(ret)) {
                OpenSOAPStringPtr   hdr_str = NULL;
                ret = OpenSOAPStringCreate(&hdr_str);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = (charset_sz == 0 || charset_sz == 1)
                        ? OpenSOAPStringFormatWC(hdr_str,
                                                 HTTP_HEADER_NOCHARSET_FORMAT,
                                                 body_sz)
                        : OpenSOAPStringFormatWC(hdr_str,
                                                 HTTP_HEADER_FORMAT,
                                                 charset_beg,
                                                 body_sz);
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        ret = OpenSOAPByteArrayCreate(http_msg);
                        if (OPENSOAP_SUCCEEDED(ret)) {
                            ret = OpenSOAPStringGetStringUSASCII(hdr_str,
																 *http_msg);
                            if (OPENSOAP_SUCCEEDED(ret)) {
                                ret = OpenSOAPByteArrayAppend(*http_msg,
                                                              body_beg,
                                                              body_sz);
                            }
                        }
                    }
                    OpenSOAPStringRelease(hdr_str);
                }
            }
        }
    }

    return ret;
}

static
int
OpenSOAPServiceConnecterCGISend(/* [in] */ OpenSOAPServiceConnecterPtr c,
								/* [in] */ OpenSOAPByteArrayPtr charset,
								/* [in] */ OpenSOAPByteArrayPtr response) {
    /*
    static const wchar_t HTTP_HEADER_FORMAT[]
        = L"Content-Type: text/xml; charset=\""
        STRING_FORMAT
        L"\"\r\n"
        L"Content-Length: %d\r\n\r\n";
    static const wchar_t HTTP_HEADER_NOCHARSET_FORMAT[]
        = L"Content-Type: text/xml\r\n"
        L"Content-Length: %d\r\n\r\n";
    */
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (c) {
        OpenSOAPByteArrayPtr http_msg = NULL;
        ret = OpenSOAPHTTPMessageCreate(response,
                                        charset,
                                        &http_msg);
        if (OPENSOAP_SUCCEEDED(ret)) {
            const unsigned char *msg_beg = NULL;
            size_t msg_sz = 0;
            ret = OpenSOAPByteArrayGetBeginSizeConst(http_msg,
                                                     &msg_beg,
                                                     &msg_sz);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPStreamWrite(c->serviceStream,
                                          msg_beg,
                                          &msg_sz);
            }
            OpenSOAPByteArrayRelease(http_msg);
        }
    }

    return ret;
}

static
int
OpenSOAPServiceConnecterStdioSend(/* [in] */ OpenSOAPServiceConnecterPtr c,
								  /* [in] */ OpenSOAPByteArrayPtr charset,
								  /* [in] */ OpenSOAPByteArrayPtr response) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (c && response) {
        static const unsigned char HTTP_RESPONSE_STATUS[]
            = "HTTP/1.1 200 OK\r\n";
        static const size_t HTTP_RESPONSE_STATUS_SIZE
            = sizeof(HTTP_RESPONSE_STATUS) - 1;
        size_t sz = HTTP_RESPONSE_STATUS_SIZE;
        ret = OpenSOAPStreamWrite(c->serviceStream,
                                  HTTP_RESPONSE_STATUS,
                                  &sz);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPServiceConnecterCGISend(c,
                                                  charset,
                                                  response);
        }
    }

    return ret;
}


const OpenSOAPServiceConnecterFuncTable 
OpenSOAPServiceConnecterFuncTableMap[] = {
    {"stdio",
     OpenSOAPServiceConnecterStdioInitialize,
     OpenSOAPServiceConnecterStdioReceiveHeader,
     OpenSOAPServiceConnecterStdioReceive,
     OpenSOAPServiceConnecterStdioSend,
    1},
    {"cgi",
     OpenSOAPServiceConnecterStdioInitialize,
     OpenSOAPServiceConnecterCGIReceiveHeader,
     OpenSOAPServiceConnecterStdioReceive,
     OpenSOAPServiceConnecterCGISend,
    0},
    {NULL, NULL, NULL, NULL, 0}
};

static
const OpenSOAPServiceConnecterFuncTable *
OpenSOAPServiceConecterFuncTableGet(/* [in] */ const char *connect_type) {
    const OpenSOAPServiceConnecterFuncTable *ret = NULL;

    if (connect_type && *connect_type) {
        for (ret = OpenSOAPServiceConnecterFuncTableMap;
             ret->connectType && strcmp(ret->connectType, connect_type) != 0;
             ++ret) {
        }
        if (!ret->connectType) {
            ret = NULL;
        }
    }

    return ret;
}

static
int
OpenSOAPServiceConnecterCreate(/* [in]  */ const char *connect_type,
                               /* [in]  */ int is_loop,
                               /* [in]  */ va_list ap,
                               /* [out] */ OpenSOAPServiceConnecterPtr *c) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (c) {
        const OpenSOAPServiceConnecterFuncTable *func_table
            = OpenSOAPServiceConecterFuncTableGet(connect_type);
        if (func_table && (func_table->canLoop || !is_loop)) {
            ret = OPENSOAP_MEM_BADALLOC;
            *c = malloc(sizeof(OpenSOAPServiceConnecter));
            if (*c) {
                (*c)->funcTable = func_table;
                ret = (func_table->initializer)(*c, ap);
                if (OPENSOAP_FAILED(ret)) {
                    free(*c);
                    *c = NULL;
                }
            }
        }
    }

    return ret;
}

static
int
OpenSOAPServiceConnecterRelease(/* [in] */ OpenSOAPServiceConnecterPtr s) {
    int ret = OpenSOAPObjectRelease((OpenSOAPObjectPtr)s);

    return ret;
}

static
int
OpenSOAPServiceConnecterReceiveHeader(/* [in]  */ OpenSOAPServiceConnecterPtr c,
                                      /* [out] */ size_t *content_length,
                                      /* [out] */ OpenSOAPByteArrayPtr charset) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (c && c->funcTable && c->funcTable->receiveHeader) {
        ret = (c->funcTable->receiveHeader)(c, content_length, charset);
    }

    return ret;
}

static
int
OpenSOAPServiceConnecterReceive(/* [in] */ OpenSOAPServiceConnecterPtr c,
                                /* [in] */ size_t content_length,
                                /* [out] */ OpenSOAPByteArrayPtr request) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (c && c->funcTable && c->funcTable->receive) {
        ret = (c->funcTable->receive)(c, content_length, request);
    }

    return ret;
}

static
int
OpenSOAPServiceConnecterSend(/* [in] */ OpenSOAPServiceConnecterPtr c,
                             /* [in] */ OpenSOAPByteArrayPtr charset,
                             /* [in] */ OpenSOAPByteArrayPtr response) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (c && c->funcTable && c->funcTable->send) {
        ret = (c->funcTable->send)(c, charset, response);
    }

    return ret;
}

/*
=begin
= OpenSOAP Service class
=end
 */

struct tagOpenSOAPServiceMapItem {
    OpenSOAPServiceFuncPtr serviceFunc;
    void *serviceOpt;
};

static
int
OpenSOAPServiceMapItemFree(void *val,
                           void *opt) {
    free(val);
    return OPENSOAP_NO_ERROR;
}

/*
=begin
--- function#OpenSOAPServiceReleaseMembers(srv)
    Release OpenSOAP Service members
    
    :Parameters
      :OpenSOAPServicePtr [in] ((|srv|))
        OpenSOAP Service pointer
    :Returns
      :int
	    Return Code Is ErrorCode
=end
 */
static
int
OpenSOAPServiceReleaseMembers(/* [in] */ OpenSOAPServicePtr srv) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (srv) {
        OpenSOAPByteArrayRelease(srv->charsetBuffer);
        OpenSOAPByteArrayRelease(srv->responseBuffer);
        OpenSOAPByteArrayRelease(srv->requestBuffer);
        OpenSOAPServiceConnecterRelease(srv->serviceConnecter);
        free(srv->defaultService);
        OpenSOAPStringHashApplyToValues(srv->serviceMap,
                                        OpenSOAPServiceMapItemFree,
                                        NULL);
        OpenSOAPStringHashRelease(srv->serviceMap);
        OpenSOAPStringRelease(srv->serviceName);
        ret = OpenSOAPObjectReleaseMembers((OpenSOAPObjectPtr)srv);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPServiceFree(obj)
    Release OpenSOAP Service
    
    :Parameters
      :OpenSOAPObjectPtr [in] ((|obj|))
        OpenSOAP Service pointer
    :Returns
      :int
	    Return Code Is ErrorCode
=end
 */
static
int
OpenSOAPServiceFree(/* [in] */ OpenSOAPObjectPtr obj) {
    int ret = OpenSOAPServiceReleaseMembers((OpenSOAPServicePtr)obj);
    if (OPENSOAP_SUCCEEDED(ret)) {
        free(obj);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPServiceInitialize(srv, free_func)
    Initialize OpenSOAP Service
    
    :Parameters
      :OpenSOAPServicePtr [in] ((|srv|))
        OpenSOAP Service pointer
      :OpenSOAPObjectFreeFunc [in] ((|free_func|))
        OpenSOAP Object Free Function pointer.
    :Returns
      :int
	    Return Code Is ErrorCode
=end
 */
static
int
OpenSOAPServiceInitialize(/* [out] */ OpenSOAPServicePtr srv,
                          /* [in]  */ OpenSOAPStringPtr srv_name,
                          /* [in]  */ const char *connect_type,
                          /* [in]  */ int is_loop,
                          /* [in]  */ va_list ap) {
    OpenSOAPServiceConnecterPtr connecter;
    int ret = OpenSOAPServiceConnecterCreate(connect_type,
                                             is_loop,
                                             ap,
                                             &connecter);
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPObjectInitialize((OpenSOAPObjectPtr)srv,
                                       OpenSOAPServiceFree);
        if (OPENSOAP_SUCCEEDED(ret)) {
            srv->serviceName = srv_name;
            srv->isLoop = is_loop;
            srv->defaultService = NULL;
            srv->serviceConnecter = connecter;
            srv->requestBuffer = NULL;
            srv->responseBuffer = NULL;
            srv->charsetBuffer = NULL;
            ret = OpenSOAPStringHashCreate(&srv->serviceMap);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPByteArrayCreate(&srv->requestBuffer);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPByteArrayCreate(&srv->responseBuffer);
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        ret = OpenSOAPByteArrayCreate(&srv->charsetBuffer);
                    }
                }
            }
            if (OPENSOAP_FAILED(ret)) {
                OpenSOAPServiceReleaseMembers(srv);
            }
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPServiceCreateString(srv, srv_name, connect_type, is_loop, ap)
    Create OpenSOAP Service
    
    :Parameters
      :OpenSOAPServicePtr * [out] ((|srv|))
        OpenSOAP Service pointer
    :Returns
      :int
	    Return Code Is ErrorCode
=end
 */
static
int
/* OPENSOAP_API */
OpenSOAPServiceCreateString(/* [out] */ OpenSOAPServicePtr *srv,
                            /* [in]  */ OpenSOAPStringPtr srv_name,
                            /* [in]  */ const char *connect_type,
                            /* [in]  */ int is_loop,
                            /* [in]  */ va_list ap) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (srv && srv_name && connect_type && *connect_type) {
        ret = OPENSOAP_MEM_BADALLOC;
        *srv = malloc(sizeof(OpenSOAPService));
        if (*srv) {
            ret = OpenSOAPServiceInitialize(*srv,
                                            srv_name,
                                            connect_type,
                                            is_loop,
                                            ap);
            if (OPENSOAP_FAILED(ret)) {
                free(*srv);
                *srv = NULL;
            }
        }
    }
    if (OPENSOAP_FAILED(ret)) {
        OpenSOAPStringRelease(srv_name);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPServiceCreateMB(srv, srv_name, connect_type, is_loop, ...)
    Create OpenSOAP Service
    
    :Parameters
      :OpenSOAPServicePtr * [out] ((|srv|))
        OpenSOAP Service pointer
    :Returns
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPServiceCreateMB(/* [out] */ OpenSOAPServicePtr *srv,
                        /* [in]  */ const char *srv_name,
                        /* [in]  */ const char *connect_type,
                        /* [in]  */ int is_loop,
                        ...) {
	OpenSOAPStringPtr srv_name_str = NULL;
	int ret = OpenSOAPStringCreateWithMB(srv_name, &srv_name_str);
	va_list ap;
	
	if (OPENSOAP_SUCCEEDED(ret)) {
		va_start(ap, is_loop);
		ret = OpenSOAPServiceCreateString(srv,
										  srv_name_str,
										  connect_type,
										  is_loop,
										  ap);
		va_end(ap);
	}
   
	return ret;
}

/*
=begin
--- function#OpenSOAPServiceCreateMB(srv, srv_name, connect_type,
is_loop, ...)
    Create OpenSOAP Service
    
    :Parameters
      :OpenSOAPServicePtr * [out] ((|srv|))
        OpenSOAP Service pointer
    :Returns
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPServiceCreateWC(/* [out] */ OpenSOAPServicePtr *srv,
						/* [in]  */ const wchar_t *srv_name,
						/* [in]  */ const char *connect_type,
						/* [in]  */ int is_loop,
						...) {
	OpenSOAPStringPtr srv_name_str = NULL;
	int ret = OpenSOAPStringCreateWithWC(srv_name, &srv_name_str);
	va_list ap;

	if (OPENSOAP_SUCCEEDED(ret)) {
		va_start(ap, is_loop);
		ret = OpenSOAPServiceCreateString(srv,
										  srv_name_str,
										  connect_type,
										  is_loop,
										  ap);
		va_end(ap);
	}
   
	return ret;
}

/*
=begin
--- function#OpenSOAPServiceRelease(srv)
    Release OpenSOAP Service
    
    :Parameters
      :[in, out] OpenSOAPServicePtr ((|srv|))
        OpenSOAP Service pointer
    :Returns
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPServiceRelease(/* [in, out] */ OpenSOAPServicePtr srv) {
	return OpenSOAPObjectRelease((OpenSOAPObjectPtr)srv);
}

static
int
OpenSOAPServiceRegisterString(/* [out] */ OpenSOAPServicePtr srv,
                              /* [in]  */ OpenSOAPStringPtr name,
                              /* [in]  */ OpenSOAPServiceFuncPtr func,
                              /* [in]  */ void *opt) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (srv && func) {
        OpenSOAPServiceMapItem *item
            = malloc(sizeof(OpenSOAPServiceMapItem));
        ret = OPENSOAP_MEM_BADALLOC;
        if (item) {
            item->serviceFunc = func;
            item->serviceOpt  = opt;

            if (name) {
                void *old_item = NULL;
                ret = OpenSOAPStringHashGetValue(srv->serviceMap,
                                                 name,
                                                 &old_item);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    OpenSOAPServiceMapItemFree(old_item, NULL);
                }
                ret = OpenSOAPStringHashSetValue(srv->serviceMap,
                                                 name,
                                                 item);
            }
            else {
                free(srv->defaultService);
                srv->defaultService = item;

                ret = OPENSOAP_NO_ERROR;
            }
            if (OPENSOAP_FAILED(ret)) {
                free(item);
            }
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPServiceRegisterMB(srv, name, func, opt)
    Register Service function in OpenSOAP Service
	In OpenSOAPServiceRun function, if service function name
	is equal to ((|name|)), then call	func(request_env,
	response_env,	opt) and return value is FAILED,
	stop OpenSOAPServiceRun and	return func's return value.
    
    :Parameters
      :[in, out] OpenSOAPServicePtr ((|obj|))
        OpenSOAP Service pointer
	  :[in] const char * ((|name|))
	    service function name.
	  :[in] OpenSOAPServiceFuncPtr ((|func|))
	    service function pointer,
	  :[in] void * ((|opt|))
        service function option parameter.
    :Returns
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPServiceRegisterMB(/* [out] */ OpenSOAPServicePtr srv,
                          /* [in]  */ const char *name,
                          /* [in]  */ OpenSOAPServiceFuncPtr func,
                          /* [in]  */ void *opt) {
    OpenSOAPStringPtr name_str = NULL;
    int ret = name ? OpenSOAPStringCreateWithMB(name, &name_str)
        : OPENSOAP_NO_ERROR;
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPServiceRegisterString(srv, name_str, func, opt);

        OpenSOAPStringRelease(name_str);
    }

    return ret;
}


/*
=begin
--- function#OpenSOAPServiceRegisterWC(srv, name, func, opt)
    Register Service function in OpenSOAP Service
	In OpenSOAPServiceRun function, if service function name
	is equal to ((|name|)), then call	func(request_env,
	response_env,	opt) and return value is FAILED,
	stop OpenSOAPServiceRun and	return func's return value.
    
    :Parameters
      :[in, out] OpenSOAPServicePtr ((|obj|))
        OpenSOAP Service pointer
	  :[in] const char * ((|name|))
	    service function name.
	  :[in] OpenSOAPServiceFuncPtr ((|func|))
	    service function pointer,
	  :[in] void * ((|opt|))
        service function option parameter.
    :Returns
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPServiceRegisterWC(/* [out] */ OpenSOAPServicePtr srv,
                          /* [in]  */ const wchar_t *name,
                          /* [in]  */ OpenSOAPServiceFuncPtr func,
                          /* [in]  */ void *opt) {
    OpenSOAPStringPtr name_str = NULL;
    int ret = name ? OpenSOAPStringCreateWithWC(name, &name_str)
        : OPENSOAP_NO_ERROR;
    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPServiceRegisterString(srv, name_str, func, opt);

        OpenSOAPStringRelease(name_str);
    }

    return ret;
}

static
int
OpenSOAPServiceFaultResponseCreate(/* [in, out] */ OpenSOAPServicePtr srv,
                                   /* [in] */ int errorCode) {
    static const char FAULT_FORMAT[]
        = "<?xml version=\"1.0\"?>\n"
		"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\n"
        "  <SOAP-ENV:Body>\n"
        "    <SOAP-ENV:Fault>\n"
        "      <faultcode>SOAP-ENV:Server</faultcode>\n"
        "      <faultstring>OPENSOAP Service error, Error code:%08x</faultstring>\n"
        "      <detail>OPENSOAP Service Internal error</detail>\n"
        "    </SOAP-ENV:Fault>\n"
        "  </SOAP-ENV:Body>\n"
        "</SOAP-ENV:Envelope>\n";
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (srv) {
        size_t resSz = sizeof(FAULT_FORMAT) + 8 - 4 - 1;
        ret = OpenSOAPByteArrayResize(srv->responseBuffer,
                                      resSz + 1);
        if (OPENSOAP_SUCCEEDED(ret)) {
            unsigned char *resBeg = NULL;
            ret = OpenSOAPByteArrayBegin(srv->responseBuffer, &resBeg);
            if (OPENSOAP_SUCCEEDED(ret)) {
                sprintf(resBeg, FAULT_FORMAT, errorCode);
            }
            ret = OpenSOAPByteArrayResize(srv->responseBuffer,
                                          resSz);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPServiceRegisterRun(srv)
    Execute OpenSOAP Service
    
    :Parameters
      :[in, out] OpenSOAPServicePtr ((|obj|))
        OpenSOAP Service pointer
    :Returns
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPServiceRun(/* [in, out] */ OpenSOAPServicePtr srv) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (srv) {
        do {
            size_t content_length = 0;
            ret = OpenSOAPServiceConnecterReceiveHeader(srv->serviceConnecter,
                                                        &content_length,
                                                        srv->charsetBuffer);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPServiceConnecterReceive(srv->serviceConnecter,
                                                      content_length,
                                                      srv->requestBuffer);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    const unsigned char *charset_beg = NULL;
                    ret = OpenSOAPByteArrayBeginConst(srv->charsetBuffer,
                                                      &charset_beg);
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        OpenSOAPEnvelopePtr request_env = NULL;
						if (charset_beg && *charset_beg == '\0') {
							charset_beg = NULL;
						}
                        ret = OpenSOAPEnvelopeCreateCharEncoding(charset_beg,
                                                                 srv->requestBuffer,
                                                                 &request_env);
                        if (OPENSOAP_SUCCEEDED(ret)) {
                            OpenSOAPBlockPtr body_block = NULL;
                            ret = OpenSOAPEnvelopeGetNextBodyBlock(request_env,
                                                                   &body_block);
                            if (OPENSOAP_SUCCEEDED(ret)) {
                                OpenSOAPStringPtr block_name = NULL;
                                ret = OpenSOAPBlockGetName(body_block,
                                                           &block_name);
                                if (OPENSOAP_SUCCEEDED(ret)) {
                                    OpenSOAPEnvelopePtr response_env = NULL;
                                    OpenSOAPServiceMapItem *item = NULL;
                                    ret = OpenSOAPStringHashGetValue(srv->serviceMap,
                                                                     block_name,
                                                                     (void **)&item);
                                    if (OPENSOAP_FAILED(ret)) {
                                        item = srv->defaultService;
                                    }
                                    if (item) {
                                        ret = (item->serviceFunc)(request_env,
                                                                  &response_env,
                                                                  (item->serviceOpt));
                                    }
                                    else {
                                        /* service not found */
                                        ret = OPENSOAP_NOT_CATEGORIZE_ERROR;
                                    }
                                    if (OPENSOAP_SUCCEEDED(ret)) {
                                        ret = OpenSOAPEnvelopeGetCharEncodingString(response_env,
                                                                                    charset_beg,
                                                                                    srv->responseBuffer);
                                        OpenSOAPEnvelopeRelease(response_env);
                                    }

                                    OpenSOAPStringRelease(block_name);
                                }
                            }

                            OpenSOAPEnvelopeRelease(request_env);
                        }
                    }
                }

                if (OPENSOAP_FAILED(ret)) {
                    OpenSOAPServiceFaultResponseCreate(srv, ret);
                }
                ret = OpenSOAPServiceConnecterSend(srv->serviceConnecter,
                                                   srv->charsetBuffer,
                                                   srv->responseBuffer);
            }
            
        } while (OPENSOAP_SUCCEEDED(ret) && srv->isLoop);
    }

    return ret;
}
