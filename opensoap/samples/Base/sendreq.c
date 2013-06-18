/*-----------------------------------------------------------------------------
 * $RCSfile: sendreq.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: sendreq.c,v 1.11 2002/01/30 11:27:37 bandou Exp $";
#endif  /* _DEBUG */

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/ClientSocket.h>
#include <OpenSOAP/String.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif /* EXIT_SUCCESS */

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif /* EXIT_FAILURE */

/*======================================================================*/
static
const char URI_PRE_HOSTNAME[] = "://";

static
const char URI_DELIMITER[] = "/";

static
const char URI_PORT_DELIMITER[] = ":";

static
const char DEFAULT_PROTOCOL[] = "http";

static
const char DEFAULT_CHARSET[] = "UTF-8";

typedef struct {
    char    *hostname;
    char    *protocol;
    char    *port;
    char    *location;

    OpenSOAPClientSocketPtr socket;
} OpenSOAPEndPointInfo;
typedef OpenSOAPEndPointInfo *OpenSOAPEndPointInfoPtr;

static
const char *
OpenSOAPEndPointInfoSupportProtocols[] = {
    "http",
    NULL
};

static
int
OpenSOAPEndPointInfoIsSupportedProtocol(const char *proto_begin,
                                        const char *proto_end) {
    int ret = OPENSOAP_IO_ERROR; /* not supported protocol */

    if (proto_begin && proto_end && proto_begin < proto_end) {
        size_t proto_len = proto_end - proto_begin;
        const char **i = OpenSOAPEndPointInfoSupportProtocols;
        for (; *i && (strncmp(*i, proto_begin, proto_len) != 0); ++i) {
        }
        if (*i) {
            ret = OPENSOAP_NO_ERROR;
        }
    }
    
    return ret;
}

static
int
OpenSOAPEndPointInfoReleaseMembers(OpenSOAPEndPointInfoPtr ep_info) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    
    if (ep_info) {
        free(ep_info->hostname); ep_info->hostname = NULL;
        free(ep_info->protocol); ep_info->protocol = NULL;
        free(ep_info->port);     ep_info->port = NULL;
        free(ep_info->location); ep_info->location = NULL;

        OpenSOAPClientSocketRelease(ep_info->socket);
        ep_info->socket = NULL;

        ret = OPENSOAP_NO_ERROR;
    }
    
    return ret;
}

static
int
OpenSOAPEndPointInfoInitialize(OpenSOAPEndPointInfoPtr ep_info,
                               const char *end_point) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    
    if (ep_info && end_point && *end_point) {
        const char *protocol_begin = end_point;
        const char *protocol_end = NULL;
        const char *hostname_begin = end_point;
        const char *hostname_end = NULL;
        const char *port_begin = NULL;
        const char *port_end = NULL;
        const char *location_begin = NULL;
        
        memset(ep_info, 0, sizeof(OpenSOAPEndPointInfo));
        
        protocol_end = strstr(protocol_begin, URI_PRE_HOSTNAME);
        if (protocol_end) {
            hostname_begin = protocol_end + strlen(URI_PRE_HOSTNAME);
        }
        else {
            protocol_begin = DEFAULT_PROTOCOL;
            protocol_end   = protocol_begin + strlen(protocol_begin);
        }
        hostname_end = strstr(hostname_begin, URI_DELIMITER);
        location_begin = hostname_end;

        if (!hostname_end) {
            hostname_end = hostname_begin + strlen(hostname_begin);
            location_begin = URI_DELIMITER;
        }
        
        port_begin = strstr(hostname_begin, URI_PORT_DELIMITER);
        if (port_begin) {
            if (port_begin < hostname_end) {
                port_end = hostname_end;
                hostname_end = port_begin;
                ++port_begin;
            }
            else {
                port_begin = NULL;
            }
        }

        if (!port_begin) {
            port_begin = protocol_begin;
            port_end   = protocol_end;
        }

        ret = OpenSOAPEndPointInfoIsSupportedProtocol(protocol_begin,
                                                      protocol_end);
        if (OPENSOAP_SUCCEEDED(ret)) {

            ret = OpenSOAPClientSocketCreate(&ep_info->socket);

            if (OPENSOAP_SUCCEEDED(ret)) {
                size_t hostname_len = hostname_end - hostname_begin;
                size_t protocol_len = protocol_end - protocol_begin;
                size_t port_len = port_end - port_begin;
                size_t location_len = strlen(location_begin);
        
                ep_info->hostname = malloc(hostname_len + 1);
                ep_info->protocol = malloc(protocol_len + 1);
                ep_info->port = malloc(port_len + 1);
                ep_info->location = malloc(location_len + 1);
                if (ep_info->hostname && ep_info->protocol
                    && ep_info->port && ep_info->location) {
                    strncpy(ep_info->hostname,
                            hostname_begin,
                            hostname_len);
                    ep_info->hostname[hostname_len] = '\0';
                    strncpy(ep_info->protocol,
                            protocol_begin,
                            protocol_len);
                    ep_info->protocol[protocol_len] = '\0';
                    strncpy(ep_info->port,
                            port_begin,
                            port_len);
                    ep_info->port[port_len] = '\0';
                    strncpy(ep_info->location,
                            location_begin,
                            location_len);
                    ep_info->location[location_len] = '\0';

                    ret = OPENSOAP_NO_ERROR;
                }
                else {
                    OpenSOAPEndPointInfoReleaseMembers(ep_info);
                    ret = OPENSOAP_MEM_BADALLOC;
                }
            }
        }
    }
    
    return ret;
}

static
int
OpenSOAPEndPointInfoCreatePostHTTPHeader(OpenSOAPEndPointInfoPtr /* [in] */ ep_info,
                                         const char * /* [in] */ soap_action,
                                         const char * /* [in] */ charset,
                                         size_t /* [in] */ req_sz,
                                         OpenSOAPByteArrayPtr * /* [out] */ http_hdr) {
    static const char DEFUALT_SOAPACTION[] = "";
#if !defined(__GNUC__) && defined(WIN32)
    static const wchar_t HEADER_FORMAT[] =
        L"POST %hs HTTP/1.0\r\n"
        L"SOAPAction: %hs\r\n"
        L"Content-Type: text/xml; charset=%hs\r\n"
        L"Content-Length:%d\r\n"
        L"\r\n";
#else
    static const wchar_t HEADER_FORMAT[] =
        L"POST %hs HTTP/1.0\r\n"
        L"SOAPAction: %hs\r\n"
        L"Content-Type: text/xml; charset=%hs\r\n"
        L"Content-Length:%d\r\n"
        L"\r\n";
#endif
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (!soap_action) {
        soap_action = DEFUALT_SOAPACTION;
    }
    if (!charset) {
        charset = DEFAULT_CHARSET;
    }

    if (ep_info && http_hdr) {
        OpenSOAPStringPtr http_hdr_str = NULL;
        ret = OpenSOAPStringCreate(&http_hdr_str);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPStringFormatWC(http_hdr_str,
                                         HEADER_FORMAT,
                                         ep_info->location,
                                         soap_action,
                                         charset,
                                         req_sz);
            if (OPENSOAP_SUCCEEDED(ret)) {
                int is_created = !*http_hdr;
                if (is_created) {
                    ret = OpenSOAPByteArrayCreate(http_hdr);
                }
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPStringGetCharEncodingString(http_hdr_str,
                                                              "US-ASCII",
                                                              *http_hdr);
                }
                if (OPENSOAP_FAILED(ret) && is_created) {
                    OpenSOAPByteArrayRelease(*http_hdr);
                    *http_hdr = NULL;
                }
            }
            OpenSOAPStringRelease(http_hdr_str);
        }
    }

    return ret;
}

static
int
OpenSOAPEndPointInfoSendHTTPRequest(OpenSOAPEndPointInfoPtr /* [in] */ ep_info,
                                    const char * /* [in] */ soap_action,
                                    const char * /* [in] */ req_charset,
                                    OpenSOAPByteArrayPtr /* [in] */ request) {
    const unsigned char *req_beg = NULL;
    size_t req_sz = 0;
    int ret = OpenSOAPByteArrayGetBeginSizeConst(request,
                                                 &req_beg,
                                                 &req_sz);
    if (OPENSOAP_SUCCEEDED(ret)) {
        OpenSOAPByteArrayPtr http_hdr = NULL;
        ret = OpenSOAPEndPointInfoCreatePostHTTPHeader(ep_info,
                                                       soap_action,
                                                       req_charset,
                                                       req_sz,
                                                       &http_hdr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            const unsigned char *http_hdr_beg = NULL;
            size_t http_hdr_sz = 0;
            ret = OpenSOAPByteArrayGetBeginSizeConst(http_hdr,
                                                     &http_hdr_beg,
                                                     &http_hdr_sz);
            if (OPENSOAP_SUCCEEDED(ret)) {
                size_t w_sz = http_hdr_sz;
                ret = OpenSOAPClientSocketWrite(ep_info->socket,
                                                http_hdr_beg,
                                                &w_sz);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    if (w_sz == http_hdr_sz) {
                        w_sz = req_sz;
                        ret = OpenSOAPClientSocketWrite(ep_info->socket,
                                                        req_beg,
                                                        &w_sz);
                        if (OPENSOAP_SUCCEEDED(ret)
                            && w_sz != req_sz) {
                            ret = OPENSOAP_IO_ERROR;
                        }
                    }
                    else {
                        ret = OPENSOAP_IO_ERROR;
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
OpenSOAPEndPointInfoGetHTTPResponseInfo(OpenSOAPEndPointInfoPtr /* [in] */ ep_info,
                                        OpenSOAPByteArrayPtr /* [out] */ charset,
                                        size_t * /* [out] */ res_sz) {
    static const char QUOTATIONS[] = "\'\"";
    static const char HTTP_HEADER_TERM[] = "\r\n\r\n";
    static const size_t HTTP_HEADER_TERM_SIZE = 4;
    static const char CONTENT_LENGTH_HEADER[] = "\r\nContent-Length:";
    static const size_t CONTENT_LENGTH_HEADER_SIZE
        = sizeof(CONTENT_LENGTH_HEADER) - 1;
    static const char CHARSET_HEADER[] = "charset=";
    static const size_t CHARSET_HEADER_SIZE = sizeof(CHARSET_HEADER) - 1;
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (ep_info && charset && res_sz) {
        OpenSOAPByteArrayPtr http_hdr = NULL;
        ret = OpenSOAPByteArrayCreate(&http_hdr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPClientSocketReadToDelm(ep_info->socket,
                                                 HTTP_HEADER_TERM,
                                                 HTTP_HEADER_TERM_SIZE,
                                                 http_hdr);
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
                        *res_sz = atol(cl_i + CONTENT_LENGTH_HEADER_SIZE);
                    }
                    else {
                        *res_sz = 0;
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
                    }
                    else {
                        charset_beg = DEFAULT_CHARSET;
                        charset_sz  = sizeof(DEFAULT_CHARSET) - 1;
                    }
                    ret = OpenSOAPByteArraySetData(charset,
                                                   charset_beg,
                                                   charset_sz);
                }
            }

            OpenSOAPByteArrayRelease(http_hdr);
        }
    }

    return ret;
}
                                        
static
int
OpenSOAPEndPointInfoRecvHTTPResponse(OpenSOAPEndPointInfoPtr /* [in] */ ep_info,
                                     OpenSOAPByteArrayPtr /* [out] */ charset,
                                     OpenSOAPByteArrayPtr /* [out] */ response) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (ep_info) {
        size_t res_sz = 0;
        ret = OpenSOAPEndPointInfoGetHTTPResponseInfo(ep_info,
                                                      charset,
                                                      &res_sz);
        if (OPENSOAP_SUCCEEDED(ret)) {
#define READ_BUF_SZ 128
            unsigned char read_buf[READ_BUF_SZ];
            size_t read_sum_sz = 0;
            ret = OpenSOAPByteArrayClear(response);
            while (OPENSOAP_SUCCEEDED(ret)) {
                size_t read_sz
                    = (res_sz && res_sz < read_sum_sz + READ_BUF_SZ)
                    ? (res_sz - read_sum_sz) : READ_BUF_SZ;
                if (!read_sz) {
                    break;
                }
                ret = OpenSOAPClientSocketRead(ep_info->socket,
                                               read_buf,
                                               &read_sz);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    if (!read_sz) {
                        break;
                    }
                    ret = OpenSOAPByteArrayAppend(response,
                                                  read_buf,
                                                  read_sz);
                    read_sum_sz += read_sz;
                }
            }
#undef READ_BUF_SZ            
        }
    }

    return ret;
}


static
int
OpenSOAPEndPointInfoInvoke(OpenSOAPEndPointInfoPtr /* [in] */ ep_info,
                           const char * /* [in] */ soap_action,
                           const char * /* [in] */ req_charset,
                           OpenSOAPByteArrayPtr /* [in] */ request,
                           OpenSOAPByteArrayPtr /* [out] */ res_charset,
                           OpenSOAPByteArrayPtr /* [out] */ response) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (ep_info && request && response) {
        ret = OpenSOAPClientSocketOpen(ep_info->socket,
                                       ep_info->hostname,
                                       ep_info->port);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPEndPointInfoSendHTTPRequest(ep_info,
                                                      soap_action,
                                                      req_charset,
                                                      request);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPEndPointInfoRecvHTTPResponse(ep_info,
                                                           res_charset,
                                                           response);
            }
            OpenSOAPClientSocketClose(ep_info->socket);
        }
    }

    return ret;
}

/*======================================================================*/

static
const char
DEFAULT_APP_NAME[] = "sendreq";

static
const char
DEFAULT_ENDPOINT[]
= "http://opensoap.cc.hokudai.ac.jp/cgi-bin/soapInterface.cgi";

static
const char
DEFAULT_CHARENC[]
= "UTF-8";

static const char USAGE_FORMAT[] = 
"Usage: %s [-s endpoint] [-a soapaction] [-c char_enc] request.xml [response.xml]\n"
"  -s endpoint    SOAP service endpoint URI\n"
"  -a soapaction  value of SOAPAction HTTP-Header\n"
"  -c char_enc    character encoding of request message (default: UTF-8)\n"
"  request.xml    request XML file\n"
"  response.xml   response XML file\n";

static const int FILE_DELM =
#if defined(__GNUC__)
'/';
#else
// 
'\\';
#endif

static const char OPTION_PREFIX[] = "-";

static const char OPTION_SOAPENDPOINT[] = "s";
static const char OPTION_SOAPACTION[] = "a";
static const char OPTION_CHARENC[] = "c";

typedef  struct {
    char    *app_name;
    
    char    *soap_endpoint;
    char    *soap_action;
    char    *char_enc;
    char    *request_xml;
    char    *response_xml;

    OpenSOAPByteArrayPtr request_buf;
    OpenSOAPByteArrayPtr response_buf;

} SendRequestVariables;

static
char *
SendRequestGetAppName(const char *argv0) {
    char *ret = NULL;
    size_t  app_name_len = 0;
    const char *app_name = strrchr(argv0, FILE_DELM);
    if (app_name) {
        ++app_name;
    }
    else {
        app_name = argv0;
    }
    app_name_len = strlen(app_name);
    ret = strdup(app_name);
    
    return ret;
}

static
int
SendRequestVariablesInitialize(SendRequestVariables *app_vars,
                               int argc,
                               char **argv) {
    int     ret = 0;
    const size_t OPTION_PREFIX_LEN = strlen(OPTION_PREFIX);
    char **av_end = argv + argc;
    memset((void *)app_vars, 0, sizeof(*app_vars)); /* clear */

    app_vars->app_name = SendRequestGetAppName(*argv);

    for (++argv; argv != av_end; ++argv) {
        const char *opt_body = *argv;
        if (strncmp(opt_body, OPTION_PREFIX, OPTION_PREFIX_LEN) == 0) {
            size_t opt_body_len = 0;
            opt_body += OPTION_PREFIX_LEN;
            opt_body_len = strlen(opt_body);
            if (strncmp(OPTION_SOAPENDPOINT,
                        opt_body, opt_body_len) == 0
                && !app_vars->soap_endpoint) {
                ++argv;
                if (argv == av_end) {
                    break;
                }
                app_vars->soap_endpoint = strdup(*argv);
                if (!app_vars->soap_endpoint) {
                    fputs("memory allocate error\n", stderr);
                    argv = av_end;
                    break;
                }
            }
            else if (strncmp(OPTION_SOAPACTION,
                             opt_body, opt_body_len) == 0
                     && !app_vars->soap_action) {
                ++argv;
                if (argv == av_end) {
                    break;
                }
                app_vars->soap_action = strdup(*argv);
                if (!app_vars->soap_action) {
                    fputs("memory allocate error\n", stderr);
                    argv = av_end;
                    break;
                }
            }
            else if (strncmp(OPTION_CHARENC,
                             opt_body, opt_body_len) == 0
                     && !app_vars->char_enc) {
                ++argv;
                if (argv == av_end) {
                    break;
                }
                app_vars->char_enc = strdup(*argv);
                if (!app_vars->char_enc) {
                    fputs("memory allocate error\n", stderr);
                    argv = av_end;
                    break;
                }
            }
            else {
                argv = av_end;
                break;
            }
        }
        else {
            break;
        }
    }

    if (argv != av_end) {
        app_vars->request_xml = strdup(*argv);
        if (app_vars->request_xml) {
            ret = 1;
            ++argv;
            if (argv != av_end) {
                app_vars->response_xml = strdup(*argv);
                if (!app_vars->response_xml) {
                    fputs("memory allocate error\n", stderr);
                    ret = 0;
                }
            }
        }
        else {
            fputs("memory allocate error\n", stderr);
        }
    }

    if (ret && !app_vars->soap_endpoint) {
        app_vars->soap_endpoint
            = strdup(DEFAULT_ENDPOINT);
        if (!app_vars->soap_endpoint) {
            fputs("memory allocate error\n", stderr);
            ret = 0;
        }
    }

    if (ret && !app_vars->char_enc) {
        app_vars->char_enc
            = strdup(DEFAULT_CHARENC);
        if (!app_vars->char_enc) {
            fputs("memory allocate error\n", stderr);
            ret = 0;
        }
    }

    if (ret) {
        int error_code = OpenSOAPInitialize(NULL);
        if (OPENSOAP_FAILED(error_code)) {
            fputs("OpenSOAP initialize error\n", stderr);
            ret = 0;
        }
    }

    if (!ret) {
        printf(USAGE_FORMAT,
               app_vars->app_name
               ? app_vars->app_name
               : DEFAULT_APP_NAME);
    }

    return ret;
}

static
int
SendRequestReadRequestFile(SendRequestVariables *app_vars) {
    int ret = 0;
    int error_code = OpenSOAPByteArrayCreate(&app_vars->request_buf);
    
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "OpenSOAP error: %x\n", error_code);
    }
    else {
        FILE *req_fp = fopen(app_vars->request_xml, "rb");
        if (!req_fp) {
            fprintf(stderr,
                    "Cannot file open: %s\n", app_vars->request_xml);
        }
        else {
#define READ_BUF_SZ 256
            unsigned char read_buf[READ_BUF_SZ];
            size_t read_size = 0;

            while (OPENSOAP_SUCCEEDED(error_code)
                   && (read_size = fread(read_buf, 1, READ_BUF_SZ, req_fp))) {
                error_code
                    = OpenSOAPByteArrayAppend(app_vars->request_buf,
                                              read_buf,
                                              read_size);
            }
            if (OPENSOAP_FAILED(error_code)) {
                fprintf(stderr,
                        "OpenSOAP error: %x\n", error_code);
            }
            else {
                ret = 1;
            }
            fclose(req_fp);
#undef  READ_BUF_SZ
        }
        if (!ret) {
            OpenSOAPByteArrayRelease(app_vars->request_buf);
            app_vars->request_buf = NULL;
        }
    }

    return ret;
}

static
int
SendRequestInvokeMessage(SendRequestVariables *app_vars) {
    int ret = 0;
    int error_code = OpenSOAPByteArrayCreate(&app_vars->response_buf);
    
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "OpenSOAP error: %x\n", error_code);
    }
    else {
        OpenSOAPEndPointInfo ep_info;
        error_code
            = OpenSOAPEndPointInfoInitialize(&ep_info,
                                             app_vars->soap_endpoint);
        if (OPENSOAP_FAILED(error_code)) {
            fprintf(stderr,
                    "OpenSOAP error: %x\n", error_code);
        }
        else {
            OpenSOAPByteArrayPtr res_charset = NULL;
            error_code
                = OpenSOAPByteArrayCreate(&res_charset);
            if (OPENSOAP_FAILED(error_code)) {
                fprintf(stderr,
                        "OpenSOAP error: %x\n", error_code);
            }
            else {
                error_code
                    = OpenSOAPEndPointInfoInvoke(&ep_info,
                                                 app_vars->soap_action,
                                                 app_vars->char_enc,
                                                 app_vars->request_buf,
                                                 res_charset,
                                                 app_vars->response_buf);
                if (OPENSOAP_FAILED(error_code)) {
                    fprintf(stderr,
                            "OpenSOAP error: %x\n", error_code);
                }
                else {
                    const unsigned char *b_ary_beg = NULL;
                    size_t b_ary_sz = 0;
                    ret = 1;
                    error_code
                        = OpenSOAPByteArrayGetBeginSizeConst(res_charset,
                                                             &b_ary_beg,
                                                             &b_ary_sz);
                    if (OPENSOAP_SUCCEEDED(error_code)) {
                        fputs("Response charset: ", stderr);
                        fwrite(b_ary_beg, 1, b_ary_sz, stderr);
                        fputc('\n', stderr);
                    }
                }
                OpenSOAPByteArrayRelease(res_charset);
            }
            
            OpenSOAPEndPointInfoReleaseMembers(&ep_info);
        }
        if (!ret) {
            OpenSOAPByteArrayRelease(app_vars->response_buf);
            app_vars->response_buf = NULL;
        }
    }

    return ret;
}

static
int
SendRequestWriteReponseFile(SendRequestVariables *app_vars) {
    int ret = 0;
    const unsigned char *res_beg = NULL;
    size_t res_sz = 0;
    int error_code
        = OpenSOAPByteArrayGetBeginSizeConst(app_vars->response_buf,
                                             &res_beg,
                                             &res_sz);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "OpenSOAP error: %x\n", error_code);
    }
    else {
        FILE *res_fp = stdout;
        if (app_vars->response_xml) {
            res_fp = fopen(app_vars->response_xml, "wb");
            if (!res_fp) {
                fprintf(stderr,
                        "Cannot open file: %s\n",
                        app_vars->response_xml);
            }
        }
        if (res_fp) {
            fwrite(res_beg, 1, res_sz, res_fp);
            if (res_fp != stdout) {
                fclose(res_fp);
            }
        }
    }
    
    return ret;
}

int
main(int argc,
     char **argv) {
    SendRequestVariables    app_vars;

    if (!SendRequestVariablesInitialize(&app_vars,
                                        argc,
                                        argv)) {
        return EXIT_FAILURE;
    }

    if (!SendRequestReadRequestFile(&app_vars)) {
        return EXIT_FAILURE;
    }
    
    if (!SendRequestInvokeMessage(&app_vars)) {
        return EXIT_FAILURE;
    }

    if (!SendRequestWriteReponseFile(&app_vars)) {
        return EXIT_FAILURE;
    }
    
    OpenSOAPUltimate();
    
    return EXIT_SUCCESS;
}

