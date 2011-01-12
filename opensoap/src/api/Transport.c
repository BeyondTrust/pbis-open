/*-----------------------------------------------------------------------------
 * $RCSfile: Transport.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: Transport.c,v 1.37.4.1 2004/06/04 07:23:11 okada Exp $";
#endif  /* _DEBUG */

#include <stdio.h>
#include "Socket.h"

#include "Transport.h"

#include <OpenSOAP/ClientSocket.h>
#include <OpenSOAP/Serializer.h>

#include <string.h>
#include <ctype.h>

/*
 * printf string format
 */
#if defined(NONGCC_WIN32)
# define STRING_WFORMAT L"%hs"
#else /* !NONGCC_WIN32 */
# define STRING_WFORMAT L"%s"
#endif /* !NONGCC_WIN32 */

#ifdef _MSC_VER
/* strcasecmp definition */
#  define strncasecmp(s1, s2, n) _strnicmp((s1), (s2), (n))
#endif /* !_MSC_VER */

static
const char NULL_CHAR = '\0';

static
const unsigned char
HTTP_LINE_TERM[] = "\r\n";

#define HTTP_LINE_TERM_SZ (sizeof(HTTP_LINE_TERM) - 1)

static
const unsigned char
HTTP_HEADER_TERM[] = "\r\n\r\n";

#define HTTP_HEADER_TERM_SZ (sizeof(HTTP_HEADER_TERM) - 1)

static
const unsigned char *
HTTP_LINE_TERM_END = HTTP_LINE_TERM + HTTP_LINE_TERM_SZ;

static
const char 
TRANSPORT_DEFAULT_CHARSET[] = "UTF-8";

/*
  for IPv6 Address
 */
static
const char 
IPV6_REFERENCE_PREFIX[] = "[";

static
const char 
IPV6_REFERENCE_POSTFIX[] = "]";

/*
 */
static
int
IsCaseInsensitiveEqual(/* [in] */ const char *s1,
                       /* [in] */ const char *s2) {
    int cmpResult = 0;

    if (s1 && s2) {
        for (; (cmpResult = toupper(*s1) - toupper(*s2)) == 0
                 && *s1 && *s2; ++s1, ++s2) {
        }
    }

    return cmpResult == 0;
}

/*
=begin
= OpenSOAP ConnectInfo class
=end
 */

struct tagOpenSOAPConnectInfo {
    /* "http://user:pass@host:port/some/where" */
    char    *hostname;      /* "host" */
    char    *protocol;      /* "http" */
    char    *port;          /* "port" */
    char    *location;      /* /some/where */
    char    *userpass;      /* "user:pass" */
    /* ssl */
    int  ssl_version;      /* SSL version */
    char * ca_file;        /* CA file for Peer */
    char * ca_dir;         /* CA dir for Peer */
    char * certchain_file; /* Certification chain file for local */
    char * privkey_file;   /* Private Key file for local */
    int  verify_level;      /* Certification Verification Level */
};

static
const char
URI_PRE_HOSTNAME[] = "://";

/* Separater for user-passwd (if exists) and hostname */
static
const char
URI_USERPASS_DLM[] = "@";

static
const char
URI_USERPASS_SEP[] = ":";

static
const char
URI_DELIMITER[] = "/";

static
const char
URI_PORT_DELIMITER[] = ":";

struct PORT_MAP {
    const char *portName;
    const char *portNumber;
};

static
struct PORT_MAP
WELL_KNOWN_PORT_MAP[] = {
    {"ftp",     "21"},
    {"smtp",    "25"},
    {"mail",    "25"},
    {"http",    "80"},
    {"www",     "80"},
    {"nntp",    "109"},
    {"news",    "109"},
    {"usenet",  "109"},
    {"readnews",    "109"},
    {"https",   "443"},
    {NULL, NULL}
};

/*
=begin
--- function#OpenSOAPConnectInfoConvertWellKnownPort(portBeg, portEnd)
    Convert well known port no.
    :Parameters
      :const char ** ((|portBeg|))
        [in, out] port strig begin address's pointer.
      :const char ** ((|portEnd|))
        [in, out] port strig end address's pointer.
=end
 */
static
void
OpenSOAPConnectInfoConvertWellKnownPort(/* [in, out] */ const char **portBeg,
                                        /* [in, out] */ const char **portEnd) {
    if (portBeg && portEnd && *portBeg && *portEnd) {
        const struct PORT_MAP *wkportmap = WELL_KNOWN_PORT_MAP;
        for (; wkportmap->portName; ++wkportmap) {
            const char *wkportnameI = wkportmap->portName;
            const char *wkportnum = wkportmap->portNumber;
            const char *portI = *portBeg;
            /* compare */
            for (; portI != *portEnd && *portI == *wkportnameI;
                 ++portI, ++wkportnameI) {
            }
            /* find */
            if (portI == *portEnd) {
                *portBeg = wkportnum;
                *portEnd = wkportnum;
                /* find string term */
                for (; **portEnd; ++(*portEnd)) {
                }
                break;
            }
        }
    }
}

static
int
OpenSOAPConnectInfoReleaseMembers(OpenSOAPConnectInfoPtr
                                  /* [in, out] */ cInfo) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (cInfo) {
        free(cInfo->hostname); cInfo->hostname = NULL;
        free(cInfo->protocol); cInfo->protocol = NULL;
        free(cInfo->port);     cInfo->port = NULL;
        free(cInfo->location); cInfo->location = NULL;
        free(cInfo->userpass); cInfo->userpass = NULL;

        free(cInfo->ca_file); cInfo->ca_file = NULL;
        free(cInfo->ca_dir);  cInfo->ca_dir = NULL;
        free(cInfo->certchain_file); cInfo->certchain_file = NULL;
        free(cInfo->privkey_file);   cInfo->privkey_file = NULL;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
OpenSOAPConnectInfoFree(OpenSOAPConnectInfoPtr
                        /* [in, out] */ cInfo) {
    int ret = OpenSOAPConnectInfoReleaseMembers(cInfo);

    if (OPENSOAP_SUCCEEDED(ret)) {
        free(cInfo);
    }

    return ret;
}

static
int
OpenSOAPConnectInfoInitialize(OpenSOAPConnectInfoPtr
                              /* [in, out] */ cInfo) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (cInfo) {
        cInfo->hostname = NULL;
        cInfo->protocol = NULL;
        cInfo->port = NULL;
        cInfo->location = NULL;
        cInfo->userpass = NULL;

        cInfo->ssl_version = 0;

        cInfo->ca_file = NULL;
        cInfo->ca_dir = NULL;
        cInfo->certchain_file = NULL;
        cInfo->privkey_file = NULL;
        cInfo->verify_level = 0;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
OpenSOAPConnectInfoCreate(OpenSOAPConnectInfoPtr * /* [out] */ cInfo) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (cInfo) {
        *cInfo = malloc(sizeof(OpenSOAPConnectInfo));
        if (*cInfo) {
            ret = OpenSOAPConnectInfoInitialize(*cInfo);
            if (OPENSOAP_FAILED(ret)) {
                free(*cInfo);
                *cInfo = NULL;
            }
        }
    }

    return ret;
}

#if 1
# define OpenSOAPConnectInfoRelease(cInfo) OpenSOAPConnectInfoFree(cInfo)
#else
static
int
OpenSOAPConnectInfoRelease(/* [in, out] */ OpenSOAPConnectInfoPtr cInfo) {
    int ret = OpenSOAPConnectInfoFree(cInfo);

    return ret;
}
#endif

static
int
OpenSOAPConnectInfoSetURL(/* [out] */ OpenSOAPConnectInfoPtr connectInfo,
                          /* [in]  */ const char * url) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (connectInfo && url && *url) {
        const char *protocolBegin = url;
        const char *protocolEnd = NULL;

        protocolEnd = strstr(protocolBegin, URI_PRE_HOSTNAME);
        if (protocolEnd) {
            const char *hostnameBegin
                = protocolEnd + strlen(URI_PRE_HOSTNAME);
            const char *hostnameEnd = strstr(hostnameBegin, URI_DELIMITER);
            const char *locationBegin = hostnameEnd;
            const char *userpassBegin = NULL;
            const char *userpassEnd = NULL;
            const char *portBegin = NULL;
            const char *portEnd = NULL;
            const char *portDelmSearchBegin = NULL;
            size_t hostnameLen = 0;
            size_t protocolLen = 0;
            size_t portLen = 0;
            size_t locationLen = 0;
            size_t userpassLen = 0;
            int isIPv6 = 0;

            if (!hostnameEnd) {
                hostnameEnd = hostnameBegin + strlen(hostnameBegin);
                locationBegin = URI_DELIMITER;
            }
            userpassBegin = hostnameBegin;
            if ((userpassEnd = strstr(userpassBegin, URI_USERPASS_DLM)) != NULL) {
                hostnameBegin = userpassEnd + strlen(URI_USERPASS_DLM);
            } else {
                userpassEnd = userpassBegin;
            }

            portDelmSearchBegin = hostnameBegin;
            /* check IPv6 Address */
            if (strncmp(hostnameBegin,
                        IPV6_REFERENCE_PREFIX,
                        sizeof(IPV6_REFERENCE_PREFIX) - 1) == 0) {
                portDelmSearchBegin
                    = strstr(hostnameBegin, IPV6_REFERENCE_POSTFIX);
                if (portDelmSearchBegin
                    && portDelmSearchBegin
                    + sizeof(IPV6_REFERENCE_POSTFIX) - 1 <= hostnameEnd) {
                    /* IPv6 Address */
                    isIPv6 = 1;
                    if (portDelmSearchBegin
                        + sizeof(IPV6_REFERENCE_POSTFIX) - 1 == hostnameEnd) {
                        /* no port part */
                        hostnameBegin += (sizeof(IPV6_REFERENCE_PREFIX) - 1);
                        hostnameEnd = portDelmSearchBegin;
                    }
                    else {
                        if (portDelmSearchBegin
                            + (sizeof(IPV6_REFERENCE_POSTFIX) - 1)
                            + (sizeof(URI_PORT_DELIMITER) - 1) < hostnameEnd
                            && strncmp(portDelmSearchBegin
                                       + sizeof(IPV6_REFERENCE_POSTFIX) - 1,
                                       URI_PORT_DELIMITER,
                                       sizeof(URI_PORT_DELIMITER) - 1) == 0) {
                            /* have port */
                            hostnameBegin
                                += (sizeof(IPV6_REFERENCE_PREFIX) - 1);
                            portEnd = hostnameEnd;
                            hostnameEnd = portDelmSearchBegin;
                            portBegin = portDelmSearchBegin
                                + (sizeof(IPV6_REFERENCE_POSTFIX) - 1)
                                + (sizeof(URI_PORT_DELIMITER) - 1);
                        }
                        else {
                            isIPv6 = 0;
                        }
                    }
                }
                else {
                    /* not IPv6 Address */
                    portDelmSearchBegin = NULL;
                }
            }

            /* not IPv6 and check port */
            if (!isIPv6 && portDelmSearchBegin) {
                portBegin = strstr(portDelmSearchBegin,
                                   URI_PORT_DELIMITER);
                if (portBegin
                    && portBegin + sizeof(URI_PORT_DELIMITER) - 1
                    < hostnameEnd) {
                    portEnd = hostnameEnd;
                    hostnameEnd = portBegin;
                    portBegin += (sizeof(URI_PORT_DELIMITER) - 1);
                }
                else {
                    portBegin = NULL;
                }
            }

            /* */
            if (!portBegin) {
                portBegin = protocolBegin;
                portEnd   = protocolEnd;
            }

            hostnameLen = hostnameEnd - hostnameBegin;
            protocolLen = protocolEnd - protocolBegin;
            /* */
            OpenSOAPConnectInfoConvertWellKnownPort(&portBegin,
                                                    &portEnd);
            /* */
            portLen = portEnd - portBegin;
            locationLen = strlen(locationBegin);

            userpassLen = userpassEnd - userpassBegin;

            OpenSOAPConnectInfoReleaseMembers(connectInfo);

            connectInfo->hostname = malloc(hostnameLen + 1);
            connectInfo->protocol = malloc(protocolLen + 1);
            connectInfo->port = malloc(portLen + 1);
            connectInfo->location = malloc(locationLen + 1);
            connectInfo->userpass = malloc(userpassLen + 1);

            if (connectInfo->hostname && connectInfo->protocol
                && connectInfo->port && connectInfo->location) {
                strncpy(connectInfo->hostname,
                        hostnameBegin,
                        hostnameLen);
                connectInfo->hostname[hostnameLen] = '\0';
                strncpy(connectInfo->protocol,
                        protocolBegin,
                        protocolLen);
                connectInfo->protocol[protocolLen] = '\0';
                strncpy(connectInfo->port,
                        portBegin,
                        portLen);
                connectInfo->port[portLen] = '\0';
                strncpy(connectInfo->location,
                        locationBegin,
                        locationLen);
                connectInfo->location[locationLen] = '\0';
                strncpy(connectInfo->userpass,
                        userpassBegin,
                        userpassLen);
                connectInfo->userpass[userpassLen] = '\0';

                ret = OPENSOAP_NO_ERROR;
            }
            else {
                OpenSOAPConnectInfoReleaseMembers(connectInfo);
                ret = OPENSOAP_MEM_BADALLOC;
            }
        }
    }

    return ret;
}

static
int
OpenSOAPConnectInfoGetHTTPPOSTRequest(/* [in, out] */ OpenSOAPConnectInfoPtr
                                      cInfo,
                                      /* [in]  */ const char *httpVer,
                                      /* [out] */ OpenSOAPByteArrayPtr
                                      httpHdr) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    static const unsigned char DEFAULT_HTTP_VER[] = "1.0";
    static const wchar_t POST_REQUEST_FORMAT[]
        =
        L"POST "
        STRING_WFORMAT  /* Request */
        L" HTTP/"
        STRING_WFORMAT  /* HTTP version */
        STRING_WFORMAT; /* line term */

    /* HTTP Version can be set from Environment Variable */
    if (!httpVer) {
        if (getenv("OPENSOAP_HTTP_VERSION")) {
            httpVer = getenv("OPENSOAP_HTTP_VERSION");
        } else {
            httpVer = DEFAULT_HTTP_VER;
        }
    }
    if (cInfo && cInfo->location && *(cInfo->location) && httpHdr) {
        OpenSOAPStringPtr tmp = NULL;
        ret = OpenSOAPStringCreate(&tmp);

        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPStringFormatWC(tmp,
                                         POST_REQUEST_FORMAT,
                                         cInfo->location,
                                         httpVer,
                                         HTTP_LINE_TERM);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPStringGetStringUSASCII(tmp,
                                                     httpHdr);
            }

            OpenSOAPStringRelease(tmp);
        }
    }

    return ret;
}

static
int
OpenSOAPConnectInfoIsSameProtocol(/* [in]  */ OpenSOAPConnectInfoPtr cInfo,
                                  /* [in]  */ const char *protocol,
                                  /* [out] */ int *isSame) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (cInfo && protocol && *protocol && isSame) {
        *isSame = IsCaseInsensitiveEqual(protocol, cInfo->protocol);
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
OpenSOAPConnectInfoCreateClientSocketStream(OpenSOAPConnectInfoPtr
                                            /* [in] */ cInfo,
                                            OpenSOAPStreamPtr *
                                            /* [out] */ s) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (cInfo && s) {
        /* check SSL protocol */
        int isValid = 0;
        ret = OpenSOAPConnectInfoIsSameProtocol(cInfo,
                                                "https",
                                                &isValid);
        if (OPENSOAP_SUCCEEDED(ret) && isValid) {
            ret = OpenSOAPClientSocketCreateSecure((OpenSOAPClientSocketPtr *)s);
            if (OPENSOAP_SUCCEEDED(ret)) {
                (*(OpenSOAPSocketPtr *)s)->ssl.version = cInfo->ssl_version;
                (*(OpenSOAPSocketPtr *)s)->ssl.ca_file = cInfo->ca_file;
                (*(OpenSOAPSocketPtr *)s)->ssl.ca_dir  = cInfo->ca_dir;
                (*(OpenSOAPSocketPtr *)s)->ssl.certchain_file = cInfo->certchain_file;
                (*(OpenSOAPSocketPtr *)s)->ssl.privkey_file = cInfo->privkey_file;
                (*(OpenSOAPSocketPtr *)s)->ssl.verify_level = cInfo->verify_level;
            }
        } else {
            ret = OpenSOAPClientSocketCreate((OpenSOAPClientSocketPtr *)s);
        }
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPClientSocketOpen((OpenSOAPClientSocketPtr)*s,
                                           cInfo->hostname,
                                           cInfo->port);
            if (OPENSOAP_FAILED(ret)) {
                OpenSOAPStreamRelease(*s);
                *s = NULL;
            }
        }
    }

    return ret;
}

#include <OpenSOAP/StringHash.h>

/*
=begin
= OpenSOAP TransportHeader class
=end
 */

struct tagOpenSOAPTransportHeader {
    OpenSOAPObject  super;
    OpenSOAPStringHashPtr   headerMap;
    OpenSOAPByteArrayPtr    charset;
    size_t                  contentLength;
};

static
const unsigned char
CONTENT_LENGTH_NAME[] = "Content-Length";

static
const unsigned char
CONTENT_TYPE_NAME[] = "Content-Type";

static
const unsigned char
CONTENT_TYPE_KEY[] = "CONTENT-TYPE";

static
const unsigned char
SOAPACTION_NAME[] = "SOAPAction";

static
const size_t
SOAPACTION_NAME_SZ = sizeof(SOAPACTION_NAME) - 1;

static
const unsigned char
SOAPACTION_KEY[] = "SOAPACTION";

/* Host HTTP Header */
static
const unsigned char
HOST_HTTPHEADER_NAME[] = "Host";

static
const size_t
HOST_HTTPHEADER_NAME_SZ = sizeof(HOST_HTTPHEADER_NAME) - 1;

/* User-Agent Header */
static
const unsigned char
USERAGENT_HTTP_HEADER[] = "User-Agent";

static
const unsigned char
USERAGENT_VALUE[] = "OpenSOAP " VERSION;

/* Transfer-Encoding HTTP Header */
static
const unsigned char
TRANSFER_ENCODING_HTTPHEADER_NAME[] = "TRANSFER-ENCODING";

static
const size_t
TRANSFER_ENCODING_HTTPHEADER_NAME_SZ = sizeof(TRANSFER_ENCODING_HTTPHEADER_NAME) - 1;

/* Transfer-Encoding Chunked data */
static
const unsigned char
TRANSFER_ENCODING_CHUNKED_NAME[] = "chunked";

static
const size_t
TRANSFER_ENCODING_CHUNKED_NAME_SZ = sizeof(TRANSFER_ENCODING_CHUNKED_NAME) - 1;

/*
 */
typedef struct {
    char *name;
    char *value;
} OpenSOAPTransportHeaderMapValue;
typedef OpenSOAPTransportHeaderMapValue *OpenSOAPTransportHeaderMapValuePtr;

/*
=begin
--- function#OpenSOAPTransportHeaderMapValueFree(value, noused)
    Free header_map of TransportHeader.

    :Parameters
      :void * ((|value|))
        [in] value header_map of TransportHeader.
      :void * ((|noused|))
        [in, out] no use option.
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
static
int
OpenSOAPTransportHeaderMapValueFree(/* [in] */ void *value,
                                    /* [in, out] */ void *noused) {
    int ret = OPENSOAP_NO_ERROR;

    if (value) {
        OpenSOAPTransportHeaderMapValuePtr v
            = (OpenSOAPTransportHeaderMapValuePtr)value;
        free(v->name);
        free(v->value);
        free(value);
    }

    return ret;
}

/*
 */
static
int
OpenSOAPTransportHeaderMapValueCreate(OpenSOAPTransportHeaderMapValuePtr *v) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (v) {
        ret = OPENSOAP_MEM_BADALLOC;
        *v = malloc(sizeof(OpenSOAPTransportHeaderMapValue));
        if (*v) {
            (*v)->name  = NULL;
            (*v)->value = NULL;

            ret = OPENSOAP_NO_ERROR;
        }
    }

    return ret;
}

/*
 */
static
int
OpenSOAPTransportHeaderClear(/* [in, out] */ OpenSOAPTransportHeaderPtr th) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (th) {
        ret = OpenSOAPStringHashApplyToValues(th->headerMap,
                                              OpenSOAPTransportHeaderMapValueFree,
                                              NULL);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPStringHashClear(th->headerMap);
            if (OPENSOAP_SUCCEEDED(ret)) {
                th->contentLength = 0;
                ret = OpenSOAPByteArraySetData(th->charset,
                                               &NULL_CHAR,
                                               1);
            }
        }
    }

    return ret;
}

static
int
OpenSOAPTransportHeaderReleaseMembers(/* [in, out] */
                                      OpenSOAPTransportHeaderPtr th) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (th) {
        ret = OpenSOAPStringHashApplyToValues(th->headerMap,
                                              OpenSOAPTransportHeaderMapValueFree,
                                              NULL);
        if (OPENSOAP_SUCCEEDED(ret)) {
            OpenSOAPStringHashRelease(th->headerMap);
            OpenSOAPByteArrayRelease(th->charset);
            ret = OpenSOAPObjectReleaseMembers((OpenSOAPObjectPtr)th);
        }
    }

    return ret;
}

static
int
OpenSOAPTransportHeaderFree(/* [in, out] */ OpenSOAPObjectPtr obj) {
    int ret = OpenSOAPTransportHeaderReleaseMembers((OpenSOAPTransportHeaderPtr)obj);
    if (OPENSOAP_SUCCEEDED(ret)) {
        free(obj);
    }

    return ret;
}

static
int
OpenSOAPTransportHeaderInitialize(/* [in, out] */ 
                                  OpenSOAPTransportHeaderPtr th) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (th) {
        OpenSOAPStringHashPtr hdrMap = NULL;
        th->headerMap = NULL;
        th->charset = NULL;
        th->contentLength = 0;
        ret = OpenSOAPStringHashCreate(&hdrMap);
        if (OPENSOAP_SUCCEEDED(ret)) {
            OpenSOAPByteArrayPtr chset = NULL;
            ret = OpenSOAPByteArrayCreateWithData(&NULL_CHAR, 1, &chset);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPObjectInitialize((OpenSOAPObjectPtr)th,
                                               OpenSOAPTransportHeaderFree);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    th->headerMap = hdrMap;
                    th->charset = chset;
                    th->contentLength = 0;
                }
                if (OPENSOAP_FAILED(ret)) {
                    OpenSOAPByteArrayRelease(chset);
                }
            }
            if (OPENSOAP_FAILED(ret)) {
                OpenSOAPStringHashRelease(hdrMap);
            }
        }
    }

    return ret;
}

static
int
OpenSOAPTransportHeaderCreate(/* [out] */ 
                              OpenSOAPTransportHeaderPtr *th) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (th) {
        ret = OPENSOAP_MEM_BADALLOC;
        *th = malloc(sizeof(OpenSOAPTransportHeader));
        if (*th) {
            ret = OpenSOAPTransportHeaderInitialize(*th);
            if (OPENSOAP_FAILED(ret)) {
                free(*th);
                *th = NULL;
            }
        }
    }

    return ret;
}

static
int
OpenSOAPTransportHeaderRelease(/* [in, out] */ 
                               OpenSOAPTransportHeaderPtr th) {
    int ret = OpenSOAPObjectRelease((OpenSOAPObjectPtr)th);

    return ret;
}

static
int
OpenSOAPTransportHeaderSetHeader(/* [in, out] */ 
                                 OpenSOAPTransportHeaderPtr th,
                                 /* [in] */ const char *name,
                                 /* [in] */ size_t nameSz,
                                 /* [in] */ const char *val,
                                 /* [in] */ size_t valSz) {
    static const char CHARSET_EQUAL[] = "charset=";
    static const size_t CHARSET_EQUAL_SIZE = sizeof(CHARSET_EQUAL) - 1;
    static const char QUOTATIONS[] = "\'\"";

    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (th && name && nameSz) {
        char *nameMbs = malloc(nameSz + 1);
        char *keyMbs  = malloc(nameSz + 1);
        char *valMbs = malloc(valSz + 1);
        ret = OPENSOAP_MEM_BADALLOC;
        if (nameMbs && keyMbs && valMbs) {
            OpenSOAPByteArrayPtr    charset = NULL;
            int is_content_length = 0;
            size_t content_length = 0;
            const char *nameItr = nameMbs;
            char *keyItr  = keyMbs;
            ret = OPENSOAP_NO_ERROR;
            memcpy(nameMbs, name, nameSz);
            nameMbs[nameSz] = '\0';
            /* toupper */
            while (*keyItr++ = toupper(*nameItr++)) {
            }
            memcpy(valMbs, val, valSz);
            valMbs[valSz]  = '\0';
            if (IsCaseInsensitiveEqual(CONTENT_TYPE_NAME, keyMbs)) {
                char *valMbs_end = valMbs + valSz;
                char *charset_beg = strstr(valMbs,
                                           CHARSET_EQUAL);
                if (charset_beg) {
                    ret = OpenSOAPByteArrayCreate(&charset);
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        char tmp = NULL_CHAR;
                        size_t charsetSz = 0;
                        char *c_i = NULL;
                        const char *q_i = QUOTATIONS;

                        for (charset_beg += CHARSET_EQUAL_SIZE;
                             *q_i && *q_i != *charset_beg; ++q_i) {
                        }
                        if (*q_i) {
                            ++charset_beg;
                            for (c_i = charset_beg;
                                 c_i != valMbs_end && *c_i != *q_i;
                                 ++c_i) {
                            }
                        }
                        else {
                            for (c_i = charset_beg;
                                 c_i != valMbs_end && !isspace(*c_i);
                                 ++c_i) {
                            }
                        }
                        tmp = *c_i;
                        *c_i = NULL_CHAR;
                        charsetSz = c_i - charset_beg + 1;
                        ret = OpenSOAPByteArraySetData(charset,
                                                       charset_beg,
                                                       charsetSz);
                        *c_i = tmp;
                        if (OPENSOAP_FAILED(ret)) {
                            OpenSOAPByteArrayRelease(charset);
                            charset = NULL;
                        }
                    }
                }
            }
            else if (IsCaseInsensitiveEqual(CONTENT_LENGTH_NAME,
                                            keyMbs)) {
                content_length
                    = (valMbs) ? atol(valMbs) : 0;
                is_content_length = 1;
            }
            if (OPENSOAP_SUCCEEDED(ret)) {
                OpenSOAPTransportHeaderMapValuePtr oldVal = NULL;
                ret = OpenSOAPStringHashGetValueMB(th->headerMap,
                                                   keyMbs,
                                                   (void **)&oldVal);
                if (OPENSOAP_SUCCEEDED(ret)) { /* oldVal found */
                    /* swap */
                    char *tmp = NULL;
                    tmp = oldVal->name;
                    oldVal->name = nameMbs;
                    nameMbs  = tmp;
                    tmp = oldVal->value;
                    oldVal->value = valMbs;
                    valMbs  = tmp;
                }
                else {
                    OpenSOAPTransportHeaderMapValuePtr newVal = NULL;
                    ret = OpenSOAPTransportHeaderMapValueCreate(&newVal);
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        newVal->name  = nameMbs;
                        newVal->value = valMbs;
                        ret = OpenSOAPStringHashSetValueMB(th->headerMap,
                                                           keyMbs,
                                                           newVal);
                        if (OPENSOAP_SUCCEEDED(ret)) {
                            nameMbs = NULL;
                            valMbs  = NULL;
                            OpenSOAPTransportHeaderMapValueFree(oldVal, NULL);
                        }
                    }
                }
                if (OPENSOAP_SUCCEEDED(ret)) {
                    if (charset) {
                        OpenSOAPByteArrayRelease(th->charset);
                        th->charset = charset;
                    }
                    if (is_content_length) {
                        th->contentLength = content_length;
                    }
                }
            }
        }
        free(valMbs);
        free(keyMbs);
        free(nameMbs);
    }

    return ret;
}

static
int
OpenSOAPTransportHeaderSetContentLength(/* [in, out] */ 
                                        OpenSOAPTransportHeaderPtr  th,
                                        /* [in] */ size_t content_length) {

    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (th && content_length > 0) {
        OpenSOAPByteArrayPtr tmp_buf = NULL;
        ret = OpenSOAPByteArrayCreate(&tmp_buf);
        if (OPENSOAP_SUCCEEDED(ret)) {
            OpenSOAPStringPtr tmp_str = NULL;
            ret = OpenSOAPStringCreate(&tmp_str);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPStringFormatWC(tmp_str,
                                             L"%ld",
                                             content_length);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPStringGetCharEncodingString(tmp_str,
                                                              NULL,
                                                              tmp_buf);
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        const unsigned char *c_l_beg = NULL;
                        size_t c_l_sz = 0;
                        ret = OpenSOAPByteArrayGetBeginSizeConst(tmp_buf,
                                                                 &c_l_beg,
                                                                 &c_l_sz);

                        if (OPENSOAP_SUCCEEDED(ret)) {
                            ret = OpenSOAPTransportHeaderSetHeader(th,
                                                                   CONTENT_LENGTH_NAME,
                                                                   sizeof(CONTENT_LENGTH_NAME) - 1,
                                                                   c_l_beg,
                                                                   c_l_sz);

                        }
                    }
                }
                OpenSOAPStringRelease(tmp_str);
            }
            OpenSOAPByteArrayRelease(tmp_buf);
        }
    }

    return ret;
}

#if 0
static
int
OpenSOAPTransportHeaderSetCharset(/* [in, out] */ 
                                  OpenSOAPTransportHeaderPtr  th,
                                  /* [in] */ const unsigned char *charset) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (th && charset && *charset) {
        ret = OPENSOAP_YET_IMPLEMENTATION;
    }

    return ret;
}
#endif

static
int
OpenSOAPTransportHeaderSetFromHTTPHeader(/* [in, out] */ 
                                         OpenSOAPTransportHeaderPtr  th,
                                         /* [in] */ 
                                         OpenSOAPByteArrayPtr http_hdr,
                                         /* [out] */ int *http_code) {
    static const int HTTP_OK_CODE = 200;
    static const unsigned char HEADER_DELM = ':';
    static const unsigned char HTTP_RESPONSE_PREFIX[] = "HTTP/";
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (th && http_hdr) {
        int dummy_http_code = 0;
        const unsigned char *http_hdr_beg = NULL;
        const unsigned char *http_hdr_end = NULL;
        if (!http_code) {
            http_code = &dummy_http_code;
        }
        *http_code = HTTP_OK_CODE;
        ret = OpenSOAPByteArrayGetBeginEndConst(http_hdr,
                                                &http_hdr_beg,
                                                &http_hdr_end);
        if (OPENSOAP_SUCCEEDED(ret)) {
            const unsigned char *d = NULL;
            const unsigned char *l_t_i = HTTP_LINE_TERM;
            const unsigned char *i = http_hdr_beg;
            const unsigned char *l_beg = i;
            int is_checked_res = 0;

            for (; i != http_hdr_end; ++i) {
                if (*i == *l_t_i) {
                    ++l_t_i;
                }
                else {
                    l_t_i = HTTP_LINE_TERM;
                }
                if (*i == HEADER_DELM) {
                    d = i;
                }
                if (l_t_i == HTTP_LINE_TERM_END) {
                    const unsigned char *l_end
                        = i - (HTTP_LINE_TERM_END - HTTP_LINE_TERM) + 1;
                    if (l_beg == l_end) {
                        break;
                    }
                    if (!is_checked_res) {
                        if (!d) {
                            if (strncmp(l_beg,
                                        HTTP_RESPONSE_PREFIX,
                                        sizeof(HTTP_RESPONSE_PREFIX) - 1)
                                == 0) {
                                const unsigned char *h_r_i
                                    = l_beg
                                    + sizeof(HTTP_RESPONSE_PREFIX) - 1;
                                for (; h_r_i != l_end
                                         && (isdigit(*h_r_i)
                                             || *h_r_i == '.'); ++h_r_i) {
                                }
                                for (; h_r_i != l_end && isspace(*h_r_i);
                                     ++h_r_i) {
                                }
                                if (h_r_i != l_end) {
                                    for (*http_code = 0;
                                         h_r_i != l_end && isdigit(*h_r_i);
                                         ++h_r_i) {
                                        *http_code *= 10;
                                        *http_code += (*h_r_i - '0');
                                    }
                                }
                            }
                        }
                        is_checked_res = 1;
                    }
                    if (d) {
                        const unsigned char *v = d + 1;

                        for (; v != l_end && isspace(*v); ++v) {
                        }
                        ret = OpenSOAPTransportHeaderSetHeader(th,
                                                               l_beg,
                                                               d - l_beg,
                                                               v,
                                                               l_end - v);
                        if (OPENSOAP_FAILED(ret)) {
                            break;
                        }
                    }

                    l_t_i = HTTP_LINE_TERM;
                    l_beg = i + 1;
                    d = NULL;
                }
            }
        }
    }

    return ret;
}

/*
 */
static
int
OpenSOAPByteArrayAppendHTTPHeader(/* [in, out] */ 
                                  OpenSOAPByteArrayPtr httpHdr,
                                  /* [in] */ const char *name,
                                  /* [in] */ const char *val) {
    static const unsigned char HTTP_HEADER_DELM[] = ": ";
    static const size_t HTTP_HEADER_DELM_SZ
        = sizeof(HTTP_HEADER_DELM) - 1;
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (httpHdr && name && *name) {
        OpenSOAPByteArrayPtr lineBuf = NULL;
        ret = OpenSOAPByteArrayCreate(&lineBuf);

        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPByteArrayAppend(lineBuf,
                                          name,
                                          strlen(name));
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPByteArrayAppend(lineBuf,
                                              HTTP_HEADER_DELM,
                                              HTTP_HEADER_DELM_SZ);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    size_t valSz = val ? strlen(val) : 0;

                    if (valSz) {
                        ret = OpenSOAPByteArrayAppend(lineBuf,
                                                      val,
                                                      valSz);
                    }
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        ret = OpenSOAPByteArrayAppend(lineBuf,
                                                      HTTP_LINE_TERM,
                                                      HTTP_LINE_TERM_SZ);
                    }
                }
            }

            if (OPENSOAP_SUCCEEDED(ret)) {
                const unsigned char *lineBeg = NULL;
                size_t lineSz = 0;
                ret = OpenSOAPByteArrayGetBeginSizeConst(lineBuf,
                                                         &lineBeg,
                                                         &lineSz);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPByteArrayAppend(httpHdr,
                                                  lineBeg,
                                                  lineSz);
                }
            }
            OpenSOAPByteArrayRelease(lineBuf);
        }
    }

    return ret;
}

/*
 */
static
int
OpenSOAPTransportHeaderMapValueAppendHTTPHeader(/* [in]  */ void *value,
                                                /* [out] */ void *httpHdrPtr) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (value && httpHdrPtr) {
        OpenSOAPTransportHeaderMapValuePtr v
            = (OpenSOAPTransportHeaderMapValuePtr)value;
        OpenSOAPByteArrayPtr httpHdr
            = (OpenSOAPByteArrayPtr)httpHdrPtr;

        ret = OpenSOAPByteArrayAppendHTTPHeader(httpHdr,
                                                v->name,
                                                v->value);
    }

    return ret;
}

/*
 */
static
int
OpenSOAPTransportHeaderAppendToHTTPHeader(/* [in] */ 
                                          OpenSOAPTransportHeaderPtr th,
                                          /* [out] */ 
                                          OpenSOAPByteArrayPtr httpHdr) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (th && httpHdr) {
        /* add SOAPAction header */
        OpenSOAPTransportHeaderMapValuePtr soapactionVal = NULL;
        ret = OpenSOAPStringHashGetValueMB(th->headerMap,
                                           SOAPACTION_KEY,
                                           (void **)&soapactionVal);
        if (OPENSOAP_FAILED(ret)) {
            ret = OpenSOAPTransportHeaderMapValueCreate(&soapactionVal);
            if (OPENSOAP_SUCCEEDED(ret)) {
                soapactionVal->name = strdup(SOAPACTION_NAME);
                ret = OpenSOAPStringHashSetValueMB(th->headerMap,
                                                   SOAPACTION_KEY,
                                                   soapactionVal);
            }
        }
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPStringHashApplyToValues(th->headerMap,
                                                  OpenSOAPTransportHeaderMapValueAppendHTTPHeader,
                                                  httpHdr);

            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPByteArrayAppend(httpHdr,
                                              HTTP_LINE_TERM,
                                              HTTP_LINE_TERM_SZ);
            }
        }
    }

    return ret;
}

static
int
OpenSOAPTransportHeaderGetCharsetBeginConst(/* [in] */ 
                                            OpenSOAPTransportHeaderPtr th,
                                            /* [out] */ const unsigned char **
                                            charset_beg) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (th && charset_beg) {
        ret = OpenSOAPByteArrayBeginConst(th->charset,
                                          charset_beg);
    }

    return ret;
}

/*
 */
static
int
OpenSOAPTransportHeaderIsContentTypeXML(/* [in]  */ OpenSOAPTransportHeaderPtr
                                        th,
                                        /* [out] */ int *isXml) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (th && isXml) {
        OpenSOAPTransportHeaderMapValuePtr contentType = NULL;
        *isXml = 0;
        ret = OpenSOAPStringHashGetValueMB(th->headerMap,
                                           CONTENT_TYPE_KEY,
                                           (void **)&contentType);
        if (OPENSOAP_SUCCEEDED(ret)) {
            static const char CONTENT_TYPE_TEXT_XML[] = "text/xml";
            const char *textXmlBeg = strstr(contentType->value,
                                            CONTENT_TYPE_TEXT_XML);
            if (textXmlBeg) {
                *isXml = 1;
            }
        }
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
OpenSOAPTransportHeaderGetContentLength(/* [in] */ 
                                        OpenSOAPTransportHeaderPtr th,
                                        /* [out] */ 
                                        size_t *content_length) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (th && content_length) {
        *content_length = th->contentLength;
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/*
=begin
= OpenSOAP Transport Authentication class
=end
 */

struct tagOpenSOAPTransportAuth {
    char *user;
    char *passwd;
    int type;
};

static
int
OpenSOAPTransportAuthReleaseMembers(/* [in, out] */ 
                                    OpenSOAPTransportAuthPtr ta) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (ta) {
        free(ta->user);
        ta->user = NULL;
        free(ta->passwd);
        ta->passwd = NULL;
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
OpenSOAPTransportAuthRelease(/* [in, out] */ 
                               OpenSOAPTransportAuthPtr ta) {
    int ret = OpenSOAPTransportAuthReleaseMembers(ta);

    if (OPENSOAP_SUCCEEDED(ret)) {
        free(ta);
    }

    return ret;
}

static
int
OpenSOAPTransportAuthInitialize(/* [out] */ 
                                OpenSOAPTransportAuthPtr ta) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (ta) {
        ta->user = NULL;
        ta->passwd = NULL;
        ta->type = 0;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
OpenSOAPTransportAuthCreate(/* [out] */ 
                            OpenSOAPTransportAuthPtr *ta) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (ta) {
        *ta = malloc(sizeof(OpenSOAPTransportAuth));
        if (*ta) {
            ret = OpenSOAPTransportAuthInitialize(*ta);
            if (OPENSOAP_FAILED(ret)) {
                free(*ta);
                *ta = NULL;
            }
        }
    }

    return ret;
}

/*
=begin
= OpenSOAP Transport Proxy class
=end
 */

struct tagOpenSOAPTransportProxy {
    OpenSOAPObject  super;
    char *host;
    int port;
    char *user;
    char *passwd;
    int auth_type;
    /*
      int http_version; /* HTTP_1.0 / HTTP_1.1 / ?
    */
};

static
int
OpenSOAPTransportProxyReleaseMembers(/* [in, out] */ 
                               OpenSOAPTransportProxyPtr tp) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (tp) {
        if (tp->host) {
            free(tp->host);
            tp->host = NULL;
        }
        if (tp->user) {
            free(tp->user);
            tp->user = NULL;
        }
        if (tp->passwd) {
            free(tp->passwd);
            tp->passwd = NULL;
        }
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
OpenSOAPTransportProxyRelease(/* [in, out] */ 
                               OpenSOAPTransportProxyPtr tp) {
    int ret = OpenSOAPTransportProxyReleaseMembers(tp);

    if (OPENSOAP_SUCCEEDED(ret)) {
        free(tp);
    }

    return ret;
}

static
int
OpenSOAPTransportProxyInitialize(/* [out] */ 
                                OpenSOAPTransportProxyPtr tp) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (tp) {
        tp->host = NULL;
        tp->port = 80;
        tp->user = NULL;
        tp->passwd = NULL;
        tp->auth_type = 0;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
OpenSOAPTransportProxyCreate(/* [out] */ 
                             OpenSOAPTransportProxyPtr * tp) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (tp) {
        *tp = malloc(sizeof(OpenSOAPTransportProxy));
        if (*tp) {
            ret = OpenSOAPTransportProxyInitialize(*tp);
            if (OPENSOAP_FAILED(ret)) {
                free(*tp);
                *tp = NULL;
            }
        }
    }

    return ret;
}

/*
=begin
= OpenSOAP Transport class
=end
 */

/*
=begin
--- function#OpenSOAPTransportReleaseMembers(t)
    Release Transport member.

    :Parameters
      :OpenSOAPTransportPtr * ((|t|))
        [in] OpenSOAP Transport pointer
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
static
int
OpenSOAPTransportReleaseMembers(/* [in] */ OpenSOAPTransportPtr t) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t) {
        OpenSOAPTransportHeaderRelease(t->requestHeaders);
        OpenSOAPTransportHeaderRelease(t->responseHeaders);
        OpenSOAPStreamRelease(t->transportStream);
        OpenSOAPByteArrayRelease(t->serviceEndPoint);
        OpenSOAPConnectInfoRelease(t->serviceConnectInfo);
        OpenSOAPTransportAuthRelease(t->auth);
        OpenSOAPTransportProxyRelease(t->proxy);

        ret = OpenSOAPObjectReleaseMembers((OpenSOAPObjectPtr)t);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportFree(t)
    Free Transport.

    :Parameters
      :OpenSOAPTransportPtr * ((|t|))
        [in] OpenSOAP Transport pointer
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
static
int
OpenSOAPTransportFree(/* [in] */ OpenSOAPObjectPtr obj) {
    OpenSOAPTransportPtr t = (OpenSOAPTransportPtr)obj;
    int ret = OpenSOAPTransportReleaseMembers(t);

    if (OPENSOAP_SUCCEEDED(ret)) {
        free(obj);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportInitialize(t)
    Initialize Transport.

    :Parameters
      :OpenSOAPTransportPtr * ((|t|))
        [out] OpenSOAP Transport pointer
    :Returns
      :int
        Return Code is ErrorCode
=end
 */
static
int
OpenSOAPTransportInitialize(/* [out] */ OpenSOAPTransportPtr t) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t) {
        t->requestHeaders  = NULL;
        t->responseHeaders = NULL;
        t->transportStream = NULL;
        t->serviceEndPoint = NULL;
        t->serviceConnectInfo = NULL;
        t->auth = NULL;
        t->proxy = NULL;

        ret = OpenSOAPObjectInitialize((OpenSOAPObjectPtr)t,
                                       OpenSOAPTransportFree);
        if (OPENSOAP_SUCCEEDED(ret)) {
            OpenSOAPTransportHeaderPtr req_hdr = NULL;
            ret = OpenSOAPTransportHeaderCreate(&req_hdr);
            if (OPENSOAP_SUCCEEDED(ret)) {
                OpenSOAPTransportHeaderPtr res_hdr = NULL;
                ret = OpenSOAPTransportHeaderCreate(&res_hdr);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    OpenSOAPByteArrayPtr s_e_p = NULL;
                    ret = OpenSOAPByteArrayCreate(&s_e_p);
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        OpenSOAPConnectInfoPtr cInfo = NULL;
                        ret = OpenSOAPConnectInfoCreate(&cInfo);
                        if (OPENSOAP_SUCCEEDED(ret)) {
                            OpenSOAPTransportAuthPtr ta = NULL;
                            ret = OpenSOAPTransportAuthCreate(&ta);
                            if (OPENSOAP_SUCCEEDED(ret)) {
                                OpenSOAPTransportProxyPtr tp = NULL;
                                ret = OpenSOAPTransportProxyCreate(&tp);
                                if (OPENSOAP_SUCCEEDED(ret)) {
                                    t->requestHeaders  = req_hdr;
                                    t->responseHeaders = res_hdr;
                                    t->serviceEndPoint = s_e_p;
                                    t->serviceConnectInfo = cInfo;
                                    t->auth = ta;
                                    t->proxy = tp;
                                }
                                if (OPENSOAP_FAILED(ret)) {
                                    OpenSOAPTransportProxyRelease(tp);
                                }
                            }
                            if (OPENSOAP_FAILED(ret)) {
                                OpenSOAPTransportAuthRelease(ta);
                            }
                        }
                        if (OPENSOAP_FAILED(ret)) {
                            OpenSOAPByteArrayRelease(s_e_p);
                        }
                    }
                    if (OPENSOAP_FAILED(ret)) {
                        OpenSOAPTransportHeaderRelease(res_hdr);
                    }
                }
                if (OPENSOAP_FAILED(ret)) {
                    OpenSOAPTransportHeaderRelease(req_hdr);
                }
            }
            if (OPENSOAP_FAILED(ret)) {
                OpenSOAPObjectReleaseMembers((OpenSOAPObjectPtr)t);
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportCreate(t)
    Create Transport instance.

    :Parameters
      :OpenSOAPTransportPtr * ((|t|))
        [out] OpenSOAP Transport pointer
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportCreate(/* [out] */ OpenSOAPTransportPtr *t) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    ret = OPENSOAP_PARAMETER_BADVALUE;
    if (t) {
        ret = OPENSOAP_MEM_BADALLOC;
        *t = malloc(sizeof(OpenSOAPTransport));
        if (*t) {
            ret = OpenSOAPTransportInitialize(*t);
            if (OPENSOAP_FAILED(ret)) {
                free(*t);
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportRelease(t)
    Release OpenSOAP Transport.
    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportRelease(/* [in] */ OpenSOAPTransportPtr t) {
    int ret = OpenSOAPObjectRelease((OpenSOAPObjectPtr)t);

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportSendHeader(t, contentLength)
    Transport header send.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
      :size_t ((|contentLength|))
        [in] content length. if 0, then not add Content-Length Header
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
static
int
OpenSOAPTransportSendHeader(/* [in] */ OpenSOAPTransportPtr t,
                            /* [in] */ size_t contentLength) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t && t->transportStream) {
        if (contentLength > 0) {
            ret = OpenSOAPTransportHeaderSetContentLength(t->requestHeaders,
                                                          contentLength);
        } else {
            ret = OPENSOAP_NO_ERROR;
        }
        if (OPENSOAP_SUCCEEDED(ret)) {
            OpenSOAPByteArrayPtr httpHdr = NULL;
            ret = OpenSOAPByteArrayCreate(&httpHdr);

            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPConnectInfoGetHTTPPOSTRequest(t->serviceConnectInfo,
                                                            NULL,
                                                            httpHdr);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPTransportHeaderAppendToHTTPHeader(t->requestHeaders,
                                                                    httpHdr);

                    if (OPENSOAP_SUCCEEDED(ret)) {
                        ret = OpenSOAPStreamWriteByteArray(t->transportStream,
                                                           httpHdr,
                                                           NULL);
                    }
                }

                OpenSOAPByteArrayRelease(httpHdr);
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportSendStream(t, in_stream)
    Send Data from Input Stream.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
      :OpenSOAPStreamPtr ((|in_stream|))
        [in] Input Stream
      :size_t ((|input_size|))
        [in] size of input data. If <=0, the size is unknown.
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
static
int
OpenSOAPTransportSendStream(/* [in] */ OpenSOAPTransportPtr t,
                            /* [in] */ OpenSOAPStreamPtr in_stream,
                            /* [in] */ size_t input_size) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t && t->transportStream && in_stream) {
        /* send Transport Header */
        ret = OpenSOAPTransportSendHeader(t, input_size);
        if (OPENSOAP_FAILED(ret)) {
            return ret;
        }
        /* get address and size */
        while(OPENSOAP_SUCCEEDED(ret)) {
#define READ_BUF_SZ 128
            unsigned char buf[READ_BUF_SZ];
            size_t buf_sz = READ_BUF_SZ;

            ret = OpenSOAPStreamRead(in_stream, buf, &buf_sz);
            if (OPENSOAP_SUCCEEDED(ret)) {
                if (buf_sz == 0) {
                    /* completed reading input Stream */
                    break;
                }
                ret = OpenSOAPStreamWrite(t->transportStream,
                                          buf,
                                          &buf_sz);
            }
#undef READ_BUF_SZ
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportSendByteArray(t, soap_env)
    Send SOAP Envelope as ByteArray.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
      :OpenSOAPByteArrayPtr ((|soap_env|))
        [in] SOAP Envelope as ByteArray
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
/* extern */
static
int
/* OPENSOAP_API */
OpenSOAPTransportSendByteArray(/* [in] */ OpenSOAPTransportPtr t,
                               /* [in] */ OpenSOAPByteArrayPtr soap_env) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t && t->transportStream && soap_env) {
        const unsigned char *soap_env_beg = NULL;
        size_t soap_env_sz = 0;
        /* get address and size */
        ret = OpenSOAPByteArrayGetBeginSizeConst(soap_env,
                                                 &soap_env_beg,
                                                 &soap_env_sz);
        if (OPENSOAP_SUCCEEDED(ret)) {
            /* Transport Header ‘—M */
            ret = OpenSOAPTransportSendHeader(t, soap_env_sz);
            if (OPENSOAP_SUCCEEDED(ret)) {
                /* Envelope Data ‘—M */
                ret = OpenSOAPStreamWrite(t->transportStream,
                                          soap_env_beg,
                                          &soap_env_sz);
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportSend(t, soap_env)
    Send SOAP Message.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
      :OpenSOAPEnvelopePtr ((|soap_env|))
        [in] SOAP Envelope
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportSend(/* [in] */ OpenSOAPTransportPtr t,
                      /* [in] */ OpenSOAPEnvelopePtr soap_env) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t && t->transportStream && soap_env) {
        OpenSOAPByteArrayPtr soap_env_data = NULL;
        ret = OpenSOAPByteArrayCreate(&soap_env_data);
        if (OPENSOAP_SUCCEEDED(ret)) {
            const unsigned char *charset_beg = NULL;
            ret = OpenSOAPTransportHeaderGetCharsetBeginConst(t->requestHeaders,
                                                              &charset_beg);
            if (OPENSOAP_SUCCEEDED(ret)) {
                if (!charset_beg || !*charset_beg) {
                    charset_beg = TRANSPORT_DEFAULT_CHARSET;
                    /* charset add to header */
                    ret = OpenSOAPTransportSetCharset(t, charset_beg);
                }
                if (OPENSOAP_SUCCEEDED(ret)) {
                    /* Envelope -> ByteArray */
                    ret = OpenSOAPEnvelopeGetCharEncodingString(soap_env,
                                                                charset_beg,
                                                                soap_env_data);
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        ret = OpenSOAPTransportSendByteArray(t, soap_env_data);
                    }
                }
            }
            OpenSOAPByteArrayRelease(soap_env_data);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportReceiveHeader(t, contentLength,
                                            transferEncoding, httpStatus)
    Receive Transport Header.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
      :size_t * ((|contentLength|))
        [out] Content-Length value stored buffer.
        if NULL, then not return Content-Length value.
      :int * ((|transferEncoding|))
        [out] Transfer-Encoding header value
              0: NONE
              1: Chunked
              (others): (to be supported in feauture)
      :int * ((|httpStatus|))
        [out] Transport status value stored buffer.
        if NULL, then not return HTTP status value.
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
static
int
OpenSOAPTransportReceiveHeader(/* [in] */ OpenSOAPTransportPtr t,
                               /* [out] */ size_t *contentLength,
                               /* [out] */ int *transferEncoding,
                               /* [out] */ int *httpStatus) {
    static const int HTTP_CONTINUE_CODE = 100;
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t && t->transportStream) {
        OpenSOAPByteArrayPtr httpHdr = NULL;
        ret = OpenSOAPByteArrayCreate(&httpHdr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            int dummyHttpStatus = 0;
            size_t dummyContentLength = 0;
            size_t dummyTransferEncoding = 0;
            /* */
            if (!httpStatus) {
                httpStatus = &dummyHttpStatus;
            }
            if (!contentLength) {
                contentLength = &dummyContentLength;
            }
            if (!transferEncoding) {
                transferEncoding = &dummyTransferEncoding;
            }
            OpenSOAPTransportHeaderClear(t->responseHeaders);
            while (1) {
                ret = OpenSOAPStreamReadToDelm(t->transportStream,
                                               HTTP_HEADER_TERM,
                                               HTTP_HEADER_TERM_SZ,
                                               httpHdr);
                if (OPENSOAP_FAILED(ret)) {
                    break;
                }
                ret = OpenSOAPTransportHeaderSetFromHTTPHeader(t->responseHeaders,
                                                               httpHdr,
                                                               httpStatus);
                if (OPENSOAP_FAILED(ret)) {
                    break;
                }
                if (*httpStatus != HTTP_CONTINUE_CODE) {
                    break;
                }
            }
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPTransportHeaderGetContentLength(t->responseHeaders,
                                                              contentLength);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    char * h = NULL;
                    ret = OpenSOAPTransportGetHeader(t,
                                                     TRANSFER_ENCODING_HTTPHEADER_NAME,
                                                     &h);
                    if (OPENSOAP_SUCCEEDED(ret) &&
                        !strncasecmp(h, TRANSFER_ENCODING_CHUNKED_NAME,
                                     TRANSFER_ENCODING_CHUNKED_NAME_SZ)) {
                        *transferEncoding = 1;
                    } else {
                        *transferEncoding = 0;
                    }
                    free(h);
                    ret = OPENSOAP_NO_ERROR;

                }
            }
            OpenSOAPByteArrayRelease(httpHdr);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportReceiveStream(t, out_stream, tp_status)
    Receive SOAP Message and write out Output Stream.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
      :OpenSOAPStreamPtr ((|out_stream|))
        [out] Output Stream
      :int * ((|tp_status|))
        [out] Transport status value stored buffer.
        if NULL, then not return HTTP status value.
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
/* extern */
static
int
OpenSOAPTransportReceiveStream(/* [in] */  OpenSOAPTransportPtr t,
                               /* [out] */ OpenSOAPStreamPtr out_stream,
                               /* [out] */ int *tp_status) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t) {
        size_t content_length = 0;
        int transfer_encoding = 0;
        /* Transport Header Receive */
        ret = OpenSOAPTransportReceiveHeader(t,
                                             &content_length,
                                             &transfer_encoding,
                                             tp_status);
        if (OPENSOAP_SUCCEEDED(ret)) {
            if (transfer_encoding == 1) {
                /* Transport Receive Body by Chunked Data */
                ret = OpenSOAPStreamReadChunkedDataStream(t->transportStream,
                                                          out_stream);
            }
            else {
                /* Transport body Receive */
                ret = OpenSOAPStreamReadWithSizeStream(t->transportStream,
                                                       content_length,
                                                       out_stream);
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportReceiveByteArray(t, soap_env, tp_status)
    Receive SOAP Message.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
      :OpenSOAPByteArrayPtr ((|soap_env|))
        [out] SOAP Envelope data stored buffer.
      :int * ((|tp_status|))
        [out] Transport status value stored buffer.
        if NULL, then not return HTTP status value.
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
/* extern */
static
int
/* OPENSOAP_API */
OpenSOAPTransportReceiveByteArray(/* [in] */  OpenSOAPTransportPtr t,
                                  /* [out] */ OpenSOAPByteArrayPtr soap_env,
                                  /* [out] */ int *tp_status) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t) {
        size_t content_length = 0;
        int transfer_encoding = 0;
        /* Transport Header Receive */
        ret = OpenSOAPTransportReceiveHeader(t,
                                             &content_length,
                                             &transfer_encoding,
                                             tp_status);
        if (OPENSOAP_SUCCEEDED(ret)) {
            if (transfer_encoding == 1) {
                /* Transport Receive Body by Chunked Data */
                ret = OpenSOAPStreamReadChunkedData(t->transportStream,
                                                    soap_env);
            }
            else {
                /* Transport body Receive */
                ret = OpenSOAPStreamReadWithSize(t->transportStream,
                                                 content_length,
                                                 soap_env);
            }
        }
    }

    return ret;
}

static
int
OpenSOAPEnvelopeCreateFaultBlock(/* [out] */ OpenSOAPBlockPtr *faultBlock,
                                 /* [out] */ OpenSOAPEnvelopePtr *faultEnv) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (faultBlock && faultEnv) {
        ret = OpenSOAPEnvelopeCreateMB(NULL, NULL, faultEnv);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPEnvelopeAddBodyBlockMB(*faultEnv,
                                                 "Fault",
                                                 faultBlock);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPBlockSetNamespaceMB(*faultBlock,
                                                  NULL,
                                                  "SOAP-ENV");
            }
            if (OPENSOAP_FAILED(ret)) {
                OpenSOAPEnvelopeRelease(*faultEnv);
                *faultEnv = NULL;
            }
        }
    }

    return ret;
}

static
int
OpenSOAPEnvelopeCreateFault(/* [in]  */ OpenSOAPStringPtr faultCode,
                            /* [in]  */ OpenSOAPStringPtr faultString,
                            /* [in]  */ OpenSOAPStringPtr detail,
                            /* [out] */ OpenSOAPEnvelopePtr *faultEnv) {
    int ret = OpenSOAPEnvelopeCreateMB(NULL, NULL, faultEnv);

    if (OPENSOAP_SUCCEEDED(ret)) {
        OpenSOAPBlockPtr faultBlock = NULL;
        OpenSOAPXMLElmPtr faultElm = NULL;

        ret = OpenSOAPEnvelopeAddFaultString(*faultEnv,
                                             faultCode,
                                             faultString,
                                             0,
                                             &faultBlock);
        faultElm = (OpenSOAPXMLElmPtr)faultBlock;
        if (detail && OPENSOAP_SUCCEEDED(ret)) {
            OpenSOAPStringPtr detailName = NULL;
            ret = OpenSOAPStringCreateWithMB("detail",
                                             &detailName);

            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPXMLElmSetChildValueAsString(faultElm,
                                                          detailName,
                                                          0,
                                                          detail);

                OpenSOAPStringRelease(detailName);
            }
        }

        if (OPENSOAP_FAILED(ret)) {
            OpenSOAPEnvelopeRelease(*faultEnv);
            *faultEnv = NULL;
        }
    }

    return ret;
}

static
int
OpenSOAPTransportCreateHTTPFaultValues(/* [in]  */ int httpStatus,
                                       /* [in]  */ OpenSOAPByteArrayPtr detail,
                                       /* [in]  */ const char *detailCharSet,
                                       /* [out] */ OpenSOAPStringPtr *faultCodeStr,
                                       /* [out] */ OpenSOAPStringPtr *faultStringStr,
                                       /* [out] */ OpenSOAPStringPtr *detailStr) {
    static const wchar_t FAULTCODE_VALUE[] = L"SOAP-ENV:Server";
    static const wchar_t FAULTSTRING_FORMAT[]
        = L"HTTP Response receive error, HTTP STATUS %d";

    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (detail && faultCodeStr && faultStringStr && detailStr) {
        *faultCodeStr = NULL;
        *faultStringStr = NULL;
        *detailStr = NULL;
        /* */
        ret = OpenSOAPStringCreateWithWC(FAULTCODE_VALUE, faultCodeStr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPStringCreate(faultStringStr);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPStringFormatWC(*faultStringStr,
                                             FAULTSTRING_FORMAT,
                                             httpStatus);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPStringCreateWithCharEncodingString(detailCharSet,
                                                                     detail,
                                                                     detailStr);
                }
            }
        }
        /* */
        if (OPENSOAP_FAILED(ret)) {
            OpenSOAPStringRelease(*faultCodeStr);
            *faultCodeStr = NULL;
            OpenSOAPStringRelease(*faultStringStr);
            *faultStringStr = NULL;
            OpenSOAPStringRelease(*detailStr);
            *detailStr = NULL;
        }
    }

    return ret;
}

static
int
OpenSOAPTransportCreateHTTPFault(/* [in]  */ int httpStatus,
                                 /* [in]  */ OpenSOAPByteArrayPtr detail,
                                 /* [in]  */ const char *detailCharSet,
                                 /* [out] */ OpenSOAPEnvelopePtr *soapEnv) {
    /*
      static const wchar_t FAULTSTRING_FORMAT[]
        = L"HTTP Response receive error, HTTP STATUS %d";
      static const wchar_t FAULTCODE_VALUE[] = L"SOAP-ENV:Server";
      */
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (detail && soapEnv) {
        OpenSOAPStringPtr faultCode = NULL;
        OpenSOAPStringPtr faultString = NULL;
        OpenSOAPStringPtr detailStr = NULL;

        ret = OpenSOAPTransportCreateHTTPFaultValues(httpStatus,
                                                     detail,
                                                     detailCharSet,
                                                     &faultCode,
                                                     &faultString,
                                                     &detailStr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPEnvelopeCreateFault(faultCode,
                                              faultString,
                                              detailStr,
                                              soapEnv);
        }
        if (OPENSOAP_FAILED(ret)) {
            OpenSOAPStringRelease(faultCode);
            OpenSOAPStringRelease(faultString);
            OpenSOAPStringRelease(detailStr);
            if ((ret == OPENSOAP_ICONV_NOT_IMPL || /* no iconv */
                 ret == OPENSOAP_INVALID_MB_SEQUENCE) && /* multibyte */
                /* Those above are for multibyte characters error in
                   HTTP body.  This should be reconsidered. */
                httpStatus == 401) {
                ret = OPENSOAP_NO_ERROR;
            }
        }
    }

    return ret;
}

static
int
OpenSOAPTransportCreateHTTPEnvelope(/* [in] */ OpenSOAPTransportPtr t,
                                    /* [in] */ OpenSOAPByteArrayPtr response,
                                    /* [in] */ int httpStatus,
                                    /* [out] */ OpenSOAPEnvelopePtr *soapEnv) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (t && response && soapEnv) {
        const unsigned char *charsetBeg = NULL;
        ret = OpenSOAPTransportHeaderGetCharsetBeginConst(t->responseHeaders,
                                                          &charsetBeg);
        if (OPENSOAP_SUCCEEDED(ret)) {
            int isXml = 0;
            if (!charsetBeg || !*charsetBeg) {
                charsetBeg = TRANSPORT_DEFAULT_CHARSET;
            }
            ret = OpenSOAPTransportHeaderIsContentTypeXML(t->responseHeaders,
                                                          &isXml);
            if (OPENSOAP_SUCCEEDED(ret) && isXml) {
                /* normal */
                ret = OpenSOAPEnvelopeCreateCharEncoding(charsetBeg,
                                                         response,
                                                         soapEnv);
            }
            else {
                /* http error */
                ret = OpenSOAPTransportCreateHTTPFault(httpStatus,
                                                       response,
                                                       charsetBeg,
                                                       soapEnv);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OPENSOAP_TRANSPORT_SET_HTTP_ERROR(httpStatus);
                }
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportSetAuthBasicHeader(t)
    create header:
    "Authorization: Basic *******"
    ******* <= base64encoded string of "user:passwd"
=end
*/

static
int
OpenSOAPTransportSetAuthBasicHeader(OpenSOAPTransportPtr /* [out] */ t) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    char *user = t->auth->user;
    char *passwd = t->auth->passwd;
    int userpass_len = 1; /* ":" */
    char *userpass_str = NULL;
    OpenSOAPSerializerFunc serializer = NULL;
    OpenSOAPStringPtr userpassStr = NULL;
    OpenSOAPByteArrayPtr userpassBA = NULL;

    if (!user && !passwd) {
        return ret;
    }
    if (OPENSOAP_SUCCEEDED(ret = OpenSOAPStringCreate(&userpassStr)) &&
        OPENSOAP_SUCCEEDED(ret = OpenSOAPGetSerializerMB("base64Binary", &serializer))) {
        /* prepare userpass_str <= "user:passwd" */
        if (user) {
            userpass_len += strlen(user);
        }
        if (passwd) {
            userpass_len += strlen(passwd);
        }

        userpass_str = (char *)malloc(userpass_len + 1);
        userpass_str[0] = '\0';
        if (user) {
            strcat(userpass_str, user);
        }
        strcat(userpass_str, ":");
        if (passwd) {
            strcat(userpass_str, passwd);
        }

        ret = OpenSOAPByteArrayCreateWithData(userpass_str, userpass_len, &userpassBA);
        if (OPENSOAP_SUCCEEDED(ret))  {
            free(userpass_str);
                if (OPENSOAP_SUCCEEDED(ret = serializer((void *)&userpassBA, userpassStr)) &&
                    OPENSOAP_SUCCEEDED(ret = OpenSOAPStringGetLengthMB(userpassStr, &userpass_len))) {
                    /* "Basic "(6) + *user:pass* + NULL(1) */
                    userpass_str = (char *)malloc(userpass_len + 7);
                    strcpy(userpass_str, "Basic ");
                    OpenSOAPStringGetStringMB(userpassStr, &userpass_len, userpass_str + 6);
                    ret = OpenSOAPTransportSetHeader(t, "Authorization", userpass_str);
                    free(userpass_str);
                }
        }
        OpenSOAPByteArrayRelease(userpassBA);
    }
    OpenSOAPStringRelease(userpassStr);

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportReceive(t, soap_env)
    Receive SOAP Message.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
      :OpenSOAPEnvelopePtr * ((|soap_env|))
        [in, out] SOAP Envelope
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportReceive(/* [in] */ OpenSOAPTransportPtr t,
                         /* [in, out] */ OpenSOAPEnvelopePtr *soap_env) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t) {
        OpenSOAPByteArrayPtr soap_env_data = NULL;
        ret = OpenSOAPByteArrayCreate(&soap_env_data);
        if (OPENSOAP_SUCCEEDED(ret)) {
            int tp_status = 0;
            /* receive Envelope ByteArray */
            ret = OpenSOAPTransportReceiveByteArray(t,
                                                    soap_env_data,
                                                    &tp_status);
            if (OPENSOAP_SUCCEEDED(ret)) {
                /* ByteArray -> Envelope */
                ret = OpenSOAPTransportCreateHTTPEnvelope(t,
                                                          soap_env_data,
                                                          tp_status,
                                                          soap_env);
            }

            OpenSOAPByteArrayRelease(soap_env_data);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportConnect(t)
    Connect to end point.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportConnect(/* [in] */ OpenSOAPTransportPtr t) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t) {
        OpenSOAPStreamPtr s = NULL;
        ret = OpenSOAPConnectInfoCreateClientSocketStream(t
                                                          ->serviceConnectInfo,
                                                          &s);
        if (OPENSOAP_SUCCEEDED(ret)) {
            OpenSOAPStreamRelease(t->transportStream);
            t->transportStream = s;
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportDisconnect(t)
    Disconnect to end point.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportDisconnect(OpenSOAPTransportPtr /* [in] */ t) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t) {
        OpenSOAPStreamRelease(t->transportStream);
        t->transportStream = NULL;
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportInvokeStream(t, stream)
    SOAP Service call using OpenSOAPStream.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
      :OpenSOAPStreamPtr ((|stream|))
        [in/Out] Input/Out Stream for SOAP, DIME request/response data.
      :int input_size
        [size_t] input stream size.  if size_t<=0, size is unknown.
      :int *tp_status
        [out] http status code
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportInvokeStream(/* [in] */ OpenSOAPTransportPtr t,
                              /* [in, out] */ OpenSOAPStreamPtr stream,
                              /* [in] */ int input_size,
                              /* [out] */ int *tp_status) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t && stream) {
        /* check auth-type's AUTH_TYPE_BASIC bit is ON ? */
        if (t->auth->type & OPENSOAP_AUTH_TYPE_BASIC) {
            ret = OpenSOAPTransportSetAuthBasicHeader(t);
        } else {
            ret = OPENSOAP_NO_ERROR;
        }
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPTransportConnect(t);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPTransportSendStream(t, stream, input_size);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPTransportReceiveStream(t, stream,
                                                            tp_status);
                }
                OpenSOAPTransportDisconnect(t);
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportInvokeByteArray(t, request, response)
    SOAP Service call using OpenSOAPByteArray.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
      :OpenSOAPEnvelopePtr ((|request|))
        [in] SOAP request data.
      :OpenSOAPByteArrayPtr ((|response|))
        [out] SOAP response data.
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportInvokeByteArray(/* [in] */ OpenSOAPTransportPtr t,
                                 /* [in] */ OpenSOAPByteArrayPtr request,
                                 /* [out] */ OpenSOAPByteArrayPtr response,
                                 /* [out] */ int *tp_status) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t && request && response) {
        /* check auth-type's AUTH_TYPE_BASIC bit is ON ? */
        if (t->auth->type & OPENSOAP_AUTH_TYPE_BASIC) {
            ret = OpenSOAPTransportSetAuthBasicHeader(t);
        } else {
            ret = OPENSOAP_NO_ERROR;
        }
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPTransportConnect(t);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPTransportSendByteArray(t, request);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPTransportReceiveByteArray(t, response,
                                                            tp_status);
                }
                OpenSOAPTransportDisconnect(t);
            }
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportInvoke(t, request, response)
    SOAP call.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in] OpenSOAP Transport pointer
      :OpenSOAPEnvelopePtr ((|request|))
        [in] SOAP request Envelope
      :OpenSOAPEnvelopePtr * ((|response|))
        [in, out] SOAP response Envelope
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportInvoke(/* [in] */ OpenSOAPTransportPtr t,
                        /* [in] */ OpenSOAPEnvelopePtr request,
                        /* [in, out] */ OpenSOAPEnvelopePtr *response) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    int auth = 0; /* flag if already tried authenticates */
    int ret_orig = ret;
    char *h = NULL;

    while (t && request && response) {
        /* if AUTH_TYPE_BASIC bit is ON, force header */
        if (t->auth->type == OPENSOAP_AUTH_TYPE_BASIC) {
            ret = OpenSOAPTransportSetAuthBasicHeader(t);
        }
        ret = OpenSOAPTransportConnect(t);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPTransportSend(t, request);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPTransportReceive(t, response);
            }
            OpenSOAPTransportDisconnect(t);
            if (OPENSOAP_TRANSPORT_GET_HTTP_ERROR(ret) != 401) {
                /* HTTP Unauthorized Error */
                break;
            }
            ret_orig = ret;
            if (auth == 1) {
                break;
            }
            if (h) {
                free(h);
                h = NULL;
            }
            ret = OpenSOAPTransportGetHeader(t,
                                             "WWW-AUTHENTICATE",
                                             &h);
            if (OPENSOAP_FAILED(ret) || !h) {
                ret = ret_orig;
                break;
            }
            if ((t->auth->type == OPENSOAP_AUTH_TYPE_ANY ||
                 t->auth->type & OPENSOAP_AUTH_TYPE_BASIC) &&
                !strncmp(h, "Basic", 5)) {
                ret = OpenSOAPTransportSetAuthBasicHeader(t);
                if (OPENSOAP_FAILED(ret)) {
                    ret = ret_orig;
                    break;
                }
                auth = 1;
            } else if ((t->auth->type == OPENSOAP_AUTH_TYPE_ANY ||
                        t->auth->type & OPENSOAP_AUTH_TYPE_DIGEST) &&
                       !strncmp(h, "Digest", 6)) {
                ret = ret_orig;
                break;
            }
            else {
                ret = ret_orig;
                break;
            }
        } else {
            break;
        }
    }
    if (h) {
        free(h);
    }
    return ret;
}

/*
=begin
--- function#OpenSOAPTransportSetSOAPAction(s, soap_action)
    Set SOAP-Action Header (Some-URI)

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in, out] OpenSOAP Transport pointer
      :const char * ((|soap_action|))
        [in] soap-action.
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportSetSOAPAction(/* [in, out] */ OpenSOAPTransportPtr t,
                               /* [in] */ const char *soap_action) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t) {
        if (!soap_action) {
            soap_action = "";
        }
        ret = OpenSOAPTransportHeaderSetHeader(t->requestHeaders,
                                               SOAPACTION_NAME,
                                               SOAPACTION_NAME_SZ,
                                               soap_action,
                                               strlen(soap_action));
    }

    return ret;
}

/*
 */
static
int
OpenSOAPTransportCheckProtocolAndSetURL(/* [out] */ OpenSOAPTransportPtr t,
                                        /* [in]  */ const char *url,
                                        /* [in, out] */ OpenSOAPConnectInfoPtr
                                        *cInfo) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t && url && *url && cInfo) {
        /* check valid protocol */
        int isValid = 0;
        ret = OpenSOAPConnectInfoIsSameProtocol(*cInfo,
                                                "http",
                                                &isValid);
        if (OPENSOAP_SUCCEEDED(ret)) {
            if (!isValid) {
                ret = OpenSOAPConnectInfoIsSameProtocol(*cInfo,
                                                        "https",
                                                        &isValid);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    if (!isValid) {
                        /* unknown protocol */
                        ret = OPENSOAP_UNSUPPORT_PROTOCOL;
                    }
                }
            }
            if (OPENSOAP_SUCCEEDED(ret)) {
                /* set sefrvice end point */
                ret = OpenSOAPByteArraySetData(t->serviceEndPoint,
                                               url,
                                               strlen(url) + 1);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    const char *hostname = (*cInfo)->hostname;
                    ret = OpenSOAPTransportHeaderSetHeader(t->requestHeaders,
                                                           HOST_HTTPHEADER_NAME,
                                                           HOST_HTTPHEADER_NAME_SZ,
                                                           hostname,
                                                           strlen(hostname));
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        /* swap */
                        OpenSOAPConnectInfoPtr tmp = *cInfo;
                        *cInfo = t->serviceConnectInfo;
                        t->serviceConnectInfo = tmp;

                        /* Add User-Agent Header */
                        OpenSOAPTransportSetHeader(t,
                                                   USERAGENT_HTTP_HEADER,
                                                   USERAGENT_VALUE);
                    }                   
                }
            }
        }
    }

    return ret;
}

/*
  url_decode(char [in, out] *str)
  in URL: "%[0-9A-Fa-f][0-9A-Fa-f]" -> '(Ascii-code)'
 */
static
void
url_decode(char /* [in, out] */ * str)
{
    char * cp = str;
    char * cp2;
    while(*cp) {
        if (cp[0] == '%' && isxdigit(*(cp+1)) && isxdigit(*(cp+2))) {
            int i;
            for(i = 1; i<=2; i++) {
                if ('A'<=cp[i] && cp[i]<='F') {
                    cp[i] -= 'A' - '9' - 1;
                }
                if ('a'<=cp[i] && cp[i]<='f') {
                    cp[i] -= 'a' - '9' - 1;
                }
                cp[i] -= '0';
            }
            cp[0] = cp[1] * 16 + cp[2];
            cp2 = cp;
            do {
                cp2++;
                *cp2 = *(cp2 + 2);
            } while(*(cp2 + 2));
        }
        cp++;
    }
    return;
}

/*
=begin
--- function#OpenSOAPTransportSetURL(t, url)
    Set request URL

    :Parameters
      :OpenSOAPTransportPtr * ((|t|))
        [in, out] OpenSOAP Transport pointer
      :const char * ((|url|))
        [in] URL
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportSetURL(/* [in, out] */ OpenSOAPTransportPtr t,
                        /* [in] */ const char *url) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t && url && *url) {
        OpenSOAPConnectInfoPtr cInfo = NULL;
        if (OPENSOAP_SUCCEEDED(ret = OpenSOAPConnectInfoCreate(&cInfo))) {
            if (OPENSOAP_SUCCEEDED(ret = OpenSOAPConnectInfoSetURL(cInfo, url)) &&
                OPENSOAP_SUCCEEDED(ret =
                                   OpenSOAPTransportCheckProtocolAndSetURL(t, url,
                                                                           &cInfo))) {
                if (t->serviceConnectInfo->userpass && *(t->serviceConnectInfo->userpass)) {
                    /* "user:pass" set */
                    char *user = t->serviceConnectInfo->userpass;
                    char *pass = strstr(t->serviceConnectInfo->userpass, URI_USERPASS_SEP);
                    if (pass) {
                        *pass = '\0';
                        pass ++;
                    }
                    url_decode(user);
                    if (pass) {
                        url_decode(pass);
                    }
                    ret = OpenSOAPTransportSetAuthUserPass(t, user, pass);
                }
            }
            OpenSOAPConnectInfoRelease(cInfo);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportSetHeader(t, header_name, header_value)
    Set Header.

    :Parameters
      :OpenSOAPTransportPtr * ((|t|))
        [out] OpenSOAP Transport pointer
      :const char * ((|header_name|))
        [in] header name.
      :const char * ((|header_value|))
        [in] header's value.
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportSetHeader(/* [in, out] */ OpenSOAPTransportPtr t,
                           /* [in] */ const char *header_name,
                           /* [in] */ const char *header_value) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t && header_name && *header_name) {
        size_t val_sz = (header_value) ? strlen(header_value) : 0;
        /* Add String to StringHash */
        ret = OpenSOAPTransportHeaderSetHeader(t->requestHeaders,
                                               header_name,
                                               strlen(header_name),
                                               header_value,
                                               val_sz);
    }

    return ret;
}

extern
int
OPENSOAP_API
OpenSOAPTransportGetHeader(/* [in] */ OpenSOAPTransportPtr t,
                           /* [in] */ const char *header_name,
                           /* [out] */ char **header_value) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t && header_name) {
        OpenSOAPTransportHeaderMapValuePtr header = NULL;
        ret = OpenSOAPStringHashGetValueMB(t->responseHeaders->headerMap,
                                           header_name,
                                           (void **)&header);
        if (OPENSOAP_SUCCEEDED(ret)) {
            *header_value = header->value;
        } else {
            ret = OPENSOAP_PARAMETER_BADVALUE;
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportSetCharset(t, charset)
    Set charset.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in, out] OpenSOAP Transport pointer
      :const char * charset
        [in] character set.
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportSetCharset(/* [out] */ OpenSOAPTransportPtr t,
                            /* [in] */  const char *charset) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    static const wchar_t CONTENT_TYPE_FORMAT[]
        = L"text/xml; charset=\""
        STRING_WFORMAT
        L"\"";

    if (t && charset && *charset) {
        OpenSOAPStringPtr tmp = NULL;
        ret = OpenSOAPStringCreate(&tmp);
        if (OPENSOAP_SUCCEEDED(ret)) {
            OpenSOAPByteArrayPtr content_type = NULL;
            ret = OpenSOAPByteArrayCreate(&content_type);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPStringFormatWC(tmp,
                                             CONTENT_TYPE_FORMAT,
                                             charset);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPStringGetStringUSASCII(tmp,
                                                         content_type);
                    if (OPENSOAP_SUCCEEDED(ret)) {
                        const unsigned char *content_type_beg = NULL;
                        size_t content_type_sz = 0;
                        ret = OpenSOAPByteArrayGetBeginSizeConst(content_type,
                                                                 &content_type_beg,
                                                                 &content_type_sz);
                        if (OPENSOAP_SUCCEEDED(ret)) {
                            ret = OpenSOAPTransportHeaderSetHeader(t->requestHeaders,
                                                                   CONTENT_TYPE_NAME,
                                                                   sizeof(CONTENT_TYPE_NAME) - 1,
                                                                   content_type_beg,
                                                                   content_type_sz);
                        }
                    }
                }
                OpenSOAPByteArrayRelease(content_type);
            }

            OpenSOAPStringRelease(tmp);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportSetContentType(t, content_type)
    Set charset.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [in, out] OpenSOAP Transport pointer
      :const char * content_type
        [in] content-type string.
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportSetContentType(/* [out] */ OpenSOAPTransportPtr t,
                                /* [in] */  const char *content_type) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (t && content_type && *content_type) {
        ret = OpenSOAPTransportHeaderSetHeader(t->requestHeaders,
                                               CONTENT_TYPE_NAME,
                                               sizeof(CONTENT_TYPE_NAME) - 1,
                                               content_type,
                                               strlen(content_type));
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportSMTPSetHostname(s)
    Set SMTP Host.

    :Parameters
      :OpenSOAPTransportPtr * ((|s|))
        [out] OpenSOAP Transport pointer
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportSMTPSetHostname(/* [out] */ OpenSOAPTransportPtr t) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportSMTPSetFrom(t)
    Set SMTP From Header.

    :Parameters
      :OpenSOAPTransportPtr ((|t|))
        [out] OpenSOAP Transport pointer
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportSMTPSetFrom(OpenSOAPTransportPtr /* [out] */ t) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

    return ret;
}

/*
=begin
--- function#OpenSOAPTransportSMTPSetTo(t)
    Set SMTP To Header.

    :Parameters
      :OpenSOAPTransportPtr * ((|t|))
        [out] OpenSOAP Transport pointer
    :Returns
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPTransportSMTPSetTo(/* [out] */ OpenSOAPTransportPtr t) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

    return ret;
}

extern
int
OPENSOAP_API
OpenSOAPTransportSetAuthUserPass(OpenSOAPTransportPtr /* [out] */ t,
                                 const char * /* [in] */ user,
                                 const char * /* [in] */ passwd) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    OpenSOAPTransportAuthPtr ta = t->auth;

    if (user != NULL || passwd != NULL) {
        if (ta->user) {
            free(ta->user);
        }
        if (user) {
            ta->user = strdup(user);
        }
        if (ta->passwd) {
            free(ta->passwd);
        }
        if (passwd) {
            ta->passwd = strdup(passwd);
        }
        ret = OPENSOAP_NO_ERROR;
    }
    return ret;
}

extern
int
OPENSOAP_API
OpenSOAPTransportSetAuthType(OpenSOAPTransportPtr /* [out] */ t,
                             int /* [in] */ auth_type ) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    OpenSOAPTransportAuthPtr ta = t->auth;

    if (auth_type & OPENSOAP_AUTH_TYPE_DIGEST ) {
        /* not implemented */
        ret = OPENSOAP_UNSUPPORT_PROTOCOL;
    } else {
        ta->type = auth_type;
        ret = OPENSOAP_NO_ERROR;
    }
    return ret;
}

extern
int
OPENSOAP_API
OpenSOAPTransportSetProxy(OpenSOAPTransportPtr /* [out] */ t,
                          const char * /* [in] */ host,
                          int /* [in] */ port,
                          const char * /* [in] */ user,
                          const char * /* [in] */ passwd,
                          int /* [in] */ auth_type) {
    OpenSOAPTransportProxyPtr tp = t->proxy;
    if (tp->host) {
        free(tp->host);
    }
    tp->host = strdup(host);
    tp->port = port;
    if (tp->user) {
        free(tp->user);
    }
    tp->user = strdup(user);
    if (tp->passwd) {
        free(tp->passwd);
    }
    tp->passwd = strdup(passwd);
    tp->auth_type = auth_type;

    return 0;
}

extern
int
OPENSOAP_API
OpenSOAPTransportSetSSLCert(OpenSOAPTransportPtr /* [out] */ t,
                            const char * /* [in] */ ca_file,
                            const char * /* [in] */ ca_dir,
                            const char * /* [in] */ certchain_file,
                            const char * /* [in] */ privkey_file,
                            int /* [in] */ verify_level ) {
#ifdef HAVE_SSL
    OpenSOAPConnectInfoPtr ci = t->serviceConnectInfo;
    free (ci->ca_file);
    free (ci->ca_dir);
    free (ci->certchain_file);
    free (ci->privkey_file);

    ci->ca_file = (ca_file ? strdup(ca_file) : NULL);
    ci->ca_dir  = (ca_dir  ? strdup(ca_dir)  : NULL);
    ci->certchain_file = (certchain_file ? strdup(certchain_file) : NULL);
    ci->privkey_file   = (privkey_file   ? strdup(privkey_file) : NULL);

    ci->verify_level = verify_level;

    return OPENSOAP_NO_ERROR;
#else /* ! HAVE_SSL */
    return OPENSOAP_UNSUPPORT_PROTOCOL;
#endif /* HAVE_SSL */
}

extern
int
OPENSOAP_API
OpenSOAPTransportSetSSLVersion(OpenSOAPTransportPtr /* [out] */ t,
                               int /* [in] */ ssl_version ) {
#ifdef HAVE_SSL
    t->serviceConnectInfo->ssl_version = ssl_version;
    return OPENSOAP_NO_ERROR;
#else /* ! HAVE_SSL */
    return OPENSOAP_UNSUPPORT_PROTOCOL;
#endif /* HAVE_SSL */
}
