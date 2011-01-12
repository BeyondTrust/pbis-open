/*-----------------------------------------------------------------------------
 * $RCSfile: EchoClient.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Transport.h>

#if defined(HAVE_CONFIG_H)
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#if defined(HAVE_UNISTD_H)
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if defined(HAVE_GETOPT_H)
#  include <getopt.h>
#endif /* HAVE_GETOPT_H */
#if !defined(HAVE_GETOPT_LONG) && !defined(HAVE_GETOPT)
/*
  get options
*/
static char *optarg;
static int optind = 0;
int getopt(int argc, char * const argv[], const char *optstring) {
	char *cp;
	if (++optind >= argc) {
		optind = argc;
		return -1;
	}
	if (argv[optind][0] != '-') {
		return -1;
	}
	if ((cp = strchr(optstring, argv[optind][1])) == NULL) {
		return '?';
	}
	if (*(cp + 1) != ':') {
		optarg = NULL;
	} else if (argv[optind][2] != '\0') {
		optarg = &(argv[optind][2]);
	} else {
		optarg = argv[optind + 1];
		optind ++;
	}
	return *cp;
}
#endif /* HAVE_GETOPT */

#if (defined(LINUX_))
#  define _XOPEN_SOURCE
#  define __USE_XOPEN
/* for strptime() */
#endif
#include <time.h>

#include "Echo.h"

int Echo_debug = 0;
int Echo_verbose = 0;
char *Echo_user = NULL;
char *Echo_pass = NULL;
int Echo_auth = -1;
int Echo_sslver = -1;

char *Echo_ca_file = NULL;
char *Echo_ca_dir = NULL;
char *Echo_certchain_file = NULL;
char *Echo_privkey_file = NULL;
int Echo_verify_level = -1;

char *Echo_charset = NULL;
char *Echo_soapaction = NULL;

static const char DEFAULT_ENDPOINT[] = "http://localhost/cgi-bin/EchoService.cgi";

/* error message macro */
#define ERROR_MSG(error, location, message); \
fprintf((stderr), \
        "(Line:%d) Failed on %s\n" \
        " ---> OpenSOAP Error Code: 0x%08x\n", \
        (location),\
        (message),\
        (error));

/* error check macro */
#define ERROR_CHECK(error, location, message); \
if (OPENSOAP_FAILED(error)) { \
    ERROR_MSG((error), (location), (message)) \
}

/* error return macro */
#define ERROR_RETURN(error, location, message); \
if (OPENSOAP_FAILED(error)) { \
    ERROR_MSG((error), (location), (message)) \
    return (error); \
}

static
void
PrintEnvelope(OpenSOAPEnvelopePtr env,
              const char *label) {
    OpenSOAPByteArrayPtr envBuf = NULL;
    const unsigned char *envBeg = NULL;
    size_t envSz = 0;
    
    OpenSOAPByteArrayCreate(&envBuf);
    OpenSOAPEnvelopeGetCharEncodingString(env,
                                          (Echo_charset ?
                                           Echo_charset :
                                           "UTF-8"),
                                          envBuf);
    OpenSOAPByteArrayGetBeginSizeConst(envBuf, &envBeg, &envSz);
    
    fprintf(stderr, "\n=== %s envelope begin ===\n", label);
    fwrite(envBeg, 1, envSz, stderr);
    fprintf(stderr, "\n=== %s envelope  end  ===\n", label);
    
    OpenSOAPByteArrayRelease(envBuf);
}

/*
  Fault child value output
*/
static
void
EchoOutputFaultChildValueMB(/* [in]  */ OpenSOAPBlockPtr faultBlock,
                            /* [in]  */ const char *elmName,
                            /* [out] */ OpenSOAPStringPtr valStr,
                            /* [out] */ FILE *fp) {
    int errorCode = OpenSOAPBlockGetChildValueMB(faultBlock,
                                                 elmName,
                                                 "string",
                                                 &valStr);
    if (OPENSOAP_SUCCEEDED(errorCode)) {
        char *val = NULL;
        errorCode = OpenSOAPStringGetStringMBWithAllocator(valStr,
                                                           NULL,
                                                           NULL,
                                                           &val);
        if (OPENSOAP_SUCCEEDED(errorCode)) {
            fprintf(fp,
                    "%s: %s\n",
                    elmName,
                    val);
            free(val);
        }
    }
    if (OPENSOAP_FAILED(errorCode)) {
        fprintf(fp,
                "%s not found\n",
                elmName);
    }
}

/*
  XML Element output
*/
static
void
EchoXMLElmOutput(/* [in]  */ OpenSOAPXMLElmPtr xmlChildElm,
                 /* [out] */ OpenSOAPByteArrayPtr tmpBuf,
                 /* [out] */ FILE *fp) {
    int errorCode = OpenSOAPXMLElmGetCharEncodingString(xmlChildElm,
                                                        NULL,
                                                        tmpBuf);
    if (OPENSOAP_SUCCEEDED(errorCode)) {
        const unsigned char *tmpBufBegin = NULL;
        size_t tmpBufSize = 0;
        errorCode
            = OpenSOAPByteArrayGetBeginSizeConst(tmpBuf,
                                                 &tmpBufBegin,
                                                 &tmpBufSize);
        if (OPENSOAP_SUCCEEDED(errorCode)) {
            fwrite(tmpBufBegin,
                   1,
                   tmpBufSize,
                   fp);
        }
    }
}

/*
  Fault detail output
*/
static
void
EchoOutputFaultDetail(/* [in]  */ OpenSOAPBlockPtr faultBlock,
                      /* [out] */ OpenSOAPStringPtr valStr,
                      /* [out] */ FILE *fp) {
    const char *elmName = "detail";
    OpenSOAPXMLElmPtr detailElm = NULL;
    int errorCode = OpenSOAPBlockGetChildMB(faultBlock,
                                            elmName,
                                            &detailElm);
    if (OPENSOAP_SUCCEEDED(errorCode)) {
        OpenSOAPXMLElmPtr detailChildElm = NULL;
        errorCode = OpenSOAPXMLElmGetNextChild(detailElm, &detailChildElm);
        if (OPENSOAP_SUCCEEDED(errorCode) && detailChildElm) {
            OpenSOAPByteArrayPtr detailChild = NULL;
            errorCode = OpenSOAPByteArrayCreate(&detailChild);
            if (OPENSOAP_SUCCEEDED(errorCode)) {
                fprintf(fp,
                        "%s:\n",
                        elmName);
                
                while (1) {
                    /* */
                    EchoXMLElmOutput(detailChildElm,
                                        detailChild,
                                        fp);
                    
                    errorCode = OpenSOAPXMLElmGetNextChild(detailElm,
                                                           &detailChildElm);
                    if (OPENSOAP_FAILED(errorCode) || !detailChildElm) {
                        break;
                    }
                }
                errorCode = OPENSOAP_NO_ERROR;
                /* */
                OpenSOAPByteArrayRelease(detailChild);
            }
        }
        else {
            /* no child elm */
            errorCode = OpenSOAPXMLElmGetValueMB(detailElm,
                                                 "string",
                                                 &valStr);
            if (OPENSOAP_SUCCEEDED(errorCode)) {
                char *val = NULL;
                errorCode = OpenSOAPStringGetStringMBWithAllocator(valStr,
                                                                   NULL,
                                                                   NULL,
                                                                   &val);
                if (OPENSOAP_SUCCEEDED(errorCode)) {
                    fprintf(fp,
                            "%s: %s\n",
                            elmName,
                            val);
                    free(val);
                }
            }
        }
    }
    if (OPENSOAP_FAILED(errorCode)) {
        fprintf(fp,
                "%s not found\n",
                elmName);
    }
}

/*
  Fault message output
*/
static
void
EchoOutputFault(/* [in]  */ OpenSOAPBlockPtr faultBlock,
                   /* [out] */ FILE *fp) {
    OpenSOAPStringPtr valStr = NULL;
    int errorCode = OpenSOAPStringCreate(&valStr);
    if (!fp) {
        fp = stdout;
    }
    
    fputs("\nFault\n", fp);
    if (OPENSOAP_SUCCEEDED(errorCode)) {
        /* fault code */
        EchoOutputFaultChildValueMB(faultBlock,
                                       "faultcode",
                                       valStr,
                                       fp);
        /* fault string */
        EchoOutputFaultChildValueMB(faultBlock,
                                       "faultstring",
                                       valStr,
                                       fp);
        /* detail */
        EchoOutputFaultDetail(faultBlock,
                                 valStr,
                                 fp);

        /* valStr release */
        OpenSOAPStringRelease(valStr);
    }
}

int
EchoSetRequest(/* [in]  */ EchoService service,
               /* [in]  */ void * input,
               /* [out] */ OpenSOAPEnvelopePtr *request)
{
    OpenSOAPBlockPtr body = NULL;
    OpenSOAPStringPtr echoString = NULL;

    /* create request message */
    ERROR_RETURN(OpenSOAPEnvelopeCreateMB("1.1", NULL, request), __LINE__, "EnvelopeCreate");

    ERROR_RETURN(OpenSOAPEnvelopeAddBodyBlockMB(*request,
                                                SERVICE_TABLE[service].method,
                                                &body),
                 __LINE__, "EnvelopeAddBodyBlock - method");
    ERROR_RETURN(OpenSOAPBlockSetNamespaceMB(body,
                                             "http://soapinterop.org/",
                                             "namesp1"),
                 __LINE__, "BlockSetNamespace");
    switch(service) {
    case ECHO_STRING:
        ERROR_RETURN(OpenSOAPStringCreateWithMB((char *)input, &echoString),
                     __LINE__, "StringCreate");
        ERROR_RETURN(OpenSOAPBlockSetChildValueMB(body,
                                                  SERVICE_TABLE[service].req,
                                                  SERVICE_TABLE[service].type,
                                                  &echoString),
                     __LINE__, "SetChildValue");
        ERROR_RETURN(OpenSOAPStringRelease(echoString),
                     __LINE__, "StringRelease");
        break;
    case ECHO_INTEGER:
        ERROR_RETURN(OpenSOAPBlockSetChildValueMB(body,
                                                  SERVICE_TABLE[service].req,
                                                  SERVICE_TABLE[service].type,
                                                  (long *)input),
                     __LINE__, "SetChildValue");
        break;
    case ECHO_FLOAT:
        ERROR_RETURN(OpenSOAPBlockSetChildValueMB(body,
                                                  SERVICE_TABLE[service].req,
                                                  SERVICE_TABLE[service].type,
                                                  (float *)input),
                     __LINE__, "SetChildValue");
        break;
    case ECHO_DATE:
        ERROR_RETURN(OpenSOAPBlockSetChildValueMB(body,
                                                  SERVICE_TABLE[service].req,
                                                  SERVICE_TABLE[service].type,
                                                  (struct tm *)input),
                     __LINE__, "SetChildValue");
        break;
    case ECHO_DOUBLE:
        ERROR_RETURN(OpenSOAPBlockSetChildValueMB(body,
                                                  SERVICE_TABLE[service].req,
                                                  SERVICE_TABLE[service].type,
                                                  (double *)input),
                     __LINE__, "SetChildValue");
        break;
    default:
        ECHO_SERVICE_NOT_IMPLEMENTED(service);
        return -1;
    }

    return 0;
}

/*
  invoke service
 */
static
int
EchoInvokeService(/* [in]  */ const char *echoEndPoint,
                     /* [in]  */ OpenSOAPEnvelopePtr request,
                     /* [out] */ OpenSOAPEnvelopePtr *response) {
    int error;
    OpenSOAPTransportPtr transport = NULL;

    if (Echo_debug) fprintf(stderr, "OpenSOAPTransportCreate:");
    error = OpenSOAPTransportCreate(&transport);
    if (Echo_debug) fprintf(stderr, "%08x\n", error);
    ERROR_RETURN(error, __LINE__, "TransportCreate");
    
    if (Echo_debug) fprintf(stderr, "OpenSOAPTransportSetURL[%s]:", echoEndPoint);
    error = OpenSOAPTransportSetURL(transport, echoEndPoint);
    if (Echo_debug) fprintf(stderr, "%08x\n", error);
    ERROR_RETURN(error, __LINE__, "TransportSetURL");

    if (Echo_charset) {
        if (Echo_debug) {
            fprintf(stderr, "OpenSOAPTransportSetCharset(%s):", Echo_charset);
        }
        error = OpenSOAPTransportSetCharset(transport, Echo_charset);
        if (Echo_debug) fprintf(stderr, "%08x\n", error);
        ERROR_RETURN(error, __LINE__, "TransportSetCharset");
    }

    if (Echo_soapaction) {
        if (Echo_debug) {
            fprintf(stderr, "OpenSOAPTransportSetSOAPAction(%s):", Echo_soapaction);
        }
        error = OpenSOAPTransportSetSOAPAction(transport, Echo_soapaction);
        if (Echo_debug) fprintf(stderr, "%08x\n", error);
        ERROR_RETURN(error, __LINE__, "TransportSetSOAPAction");
    }

    if (Echo_user || Echo_pass) {
        if (Echo_debug) {
            fprintf(stderr, "OpenSOAPTransportSetAuthUserPass(user:%s,passwd:%s):", Echo_user, Echo_pass);
        }
        error = OpenSOAPTransportSetAuthUserPass(transport, Echo_user, Echo_pass);
        if (Echo_debug) fprintf(stderr, "%08x\n", error);
        ERROR_RETURN(error, __LINE__, "TransportSetAuthUserPass");
    }
    if (Echo_auth != -1) {
        if (Echo_debug) {
            fprintf(stderr, "OpenSOAPTransportSetAuthType(type:%d):", Echo_auth);
        }
        error = OpenSOAPTransportSetAuthType(transport, Echo_auth);
        if (Echo_debug) fprintf(stderr, "%08x\n", error);
        ERROR_RETURN(error, __LINE__, "TransportSetAuthType");
    }
    if (Echo_sslver != -1) {
        if (Echo_debug) {
            fprintf(stderr, "OpenSOAPTransportSetSSLVersion(version:%d):", Echo_sslver);
        }
        error = OpenSOAPTransportSetSSLVersion(transport, Echo_sslver);
        if (Echo_debug) fprintf(stderr, "%08x\n", error);
        ERROR_RETURN(error, __LINE__, "TransportSetSSLVersion");
    }
    if (Echo_ca_file || Echo_ca_dir || Echo_certchain_file ||
        Echo_privkey_file || Echo_verify_level != -1) {
        if (Echo_debug) {
            if (Echo_verify_level == -1) {
                Echo_verify_level = 0;
            }
            fprintf(stderr,
                    "OpenSOAPTransportSetSSLCert(ca_file:%s,ca_dir:%s,certchain_file:%s,privkey_file:%s,version:%d):",
                    Echo_ca_file, Echo_ca_dir, Echo_certchain_file,
                    Echo_privkey_file, Echo_verify_level);
        }
        error =
            OpenSOAPTransportSetSSLCert(transport, Echo_ca_file, Echo_ca_dir,
                                        Echo_certchain_file,
                                        Echo_privkey_file,
                                        Echo_verify_level);
        if (Echo_debug) fprintf(stderr, "%08x\n", error);
        ERROR_RETURN(error, __LINE__, "TransportSetSSLCert");
    }

    if (Echo_debug) fprintf(stderr, "OpenSOAPTransportInvoke:");
    error = OpenSOAPTransportInvoke(transport, request, response);
    if (Echo_debug) fprintf(stderr, "%08x\n", error);
    ERROR_CHECK(error, __LINE__, "TransportInvoke");

    if (OPENSOAP_TRANSPORT_IS_HTTP_ERROR(error)) {
        int http_status = OPENSOAP_TRANSPORT_GET_HTTP_ERROR(error);
        switch(http_status) {
        case 400:
            fprintf(stderr, "400 HTTP Bad Request - Could be a wrong endpoint\n");
            break;
        case 401:
            fprintf(stderr, "401 HTTP Unauthorized - Authentication failed\n");
            break;
        case 402:
            fprintf(stderr, "402 HTTP Payment Required - Please pay something.\n");
            break;
        case 403:
            fprintf(stderr, "403 HTTP Forbidden - You don't have a permission.\n");
            break;
        case 404:
            fprintf(stderr, "404 HTTP Not Found - Endpoint must be wrong.\n");
            break;
        case 405:
            fprintf(stderr, "405 HTTP Method Not Allowed");
            break;
        case 406:
            fprintf(stderr, "406 HTTP Not Acceptable");
            break;
        case 407:
            fprintf(stderr, "407 HTTP Proxy Authentication Required");
            break;
        case 408:
            fprintf(stderr, "408 HTTP Request Timeout");
            break;
        case 409:
            fprintf(stderr, "409 HTTP Conflict");
            break;
        default:
            fprintf(stderr, "HTTP Error (Status:%d)\n", http_status);
        }
        return error;
    }
    else if (OPENSOAP_FAILED(error)) {
        switch(error) {
        case OPENSOAP_TRANSPORT_HOST_NOT_FOUND:
            fprintf(stderr, "Host not found\n");
            break;
        case OPENSOAP_TRANSPORT_NETWORK_UNREACH:
            fprintf(stderr, "Network is unreachable\n");
            break;
        case OPENSOAP_TRANSPORT_HOST_UNREACH:
            fprintf(stderr, "Host is unreachable\n");
            break;
        case OPENSOAP_TRANSPORT_CONNECTION_REFUSED:
            fprintf(stderr, "Connection Refused\n");
            break;
        case OPENSOAP_TRANSPORT_CONNECTION_TIMEOUT:
            fprintf(stderr, "Connection Timed Out\n");
            break;
        }
        return error;
    }
    
    if (Echo_debug) fprintf(stderr, "OpenSOAPTransportRelease:");
    error = OpenSOAPTransportRelease(transport);
    if (Echo_debug) fprintf(stderr, "%08x\n", error);
    ERROR_RETURN(error, __LINE__, "TransportRelease");
    return error;
}

/*
  Get response message parameters
 */
static
int
EchoGetResponse(/* [in]  */ EchoService service,
                /* [out] */ void * output,
                /* [in] */ OpenSOAPEnvelopePtr response) {
    OpenSOAPBlockPtr responseBlock = NULL;

    OpenSOAPStringPtr replyStr = NULL;

    switch(service) {
    case ECHO_STRING:
    case ECHO_INTEGER:
    case ECHO_FLOAT:
    case ECHO_DATE:
    case ECHO_DOUBLE:
        OpenSOAPEnvelopeGetBodyBlockMB(response,
                                       SERVICE_TABLE[service].res,
                                       &responseBlock);
        if (service == ECHO_STRING) {
            OpenSOAPStringCreate(&replyStr);
            OpenSOAPBlockGetChildValueMB(responseBlock,
                                         "return",
                                         "string",
                                         &replyStr);
            OpenSOAPStringGetStringMBWithAllocator(replyStr,
                                                   NULL,
                                                   NULL,
                                                   (char **)output);
            OpenSOAPStringRelease(replyStr);
        } else {
            OpenSOAPBlockGetChildValueMB(responseBlock,
                                         "return",
                                         SERVICE_TABLE[service].type,
                                         output);
        }
        break;
    default:
        ECHO_SERVICE_NOT_IMPLEMENTED(service);
        break;
    }
    return 0;
}

/*
  Display Help Message
*/
static const char USAGE_FORMAT[] = 
"Usage: %s [options] test# data [data2 [data3 ..]]\n"
"Options:\n"
#if defined(HAVE_GETOPT_LONG)
"  -h --help              Display this message\n"
"  -v --verbose           Verbose mode.  Display SOAP Messages\n"
"  -d --debug             Debug mode.  Display debug messages\n"
"  -s --endpoint=ENDPOINT The EchoService endpoint\n"
"                         (default: http://localhost/cgi-bin/EchoService.cgi)\n"
"  -c --charset=CHARSET   Specify charset (default: UTF-8)\n"
"  -a --soapaction=ACTION Specify SOAPAction (default: \"\")\n"
"  -u --http-user=USER    HTTP Authentication username\n"
"                         (default: env ECHO_HTTP_USER)\n"
"  -p --http-passwd=PASS  HTTP Authentication password\n"
"                         (default: env ECHO_HTTP_PASS)\n"
"  -t --http-type=AUTH    HTTP Auth type (default:0=ANY, 1=BASIC, 2=DIGEST)\n"
"  -S --sslver=VER        Specify SSL Version used\n"
"                         (default:0=ALL, 1=SSLv2,2=SSLv3,4=TLSv1)\n"
"  -V --verify=LEVEL      Specify the SSL Server Certification Level\n"
"                         (default:0=DON'T CARE, 1=STRICT)\n"
"  --ca_dir=DIR           Specify the directory containing the CA certificates\n"
"  --ca_file=FILE         Specify the filename containing the CA certificate\n"
"  --certchain_file=FILE  Client Certification chain file name\n"
"  --privkey_file=FILE    Client Authentication Private Key file name\n"
#else /* !HAVE_GETOPT_LONG */
"  -h          Display this message\n"
"  -v          Verbose mode.  Display SOAP Messages\n"
"  -d          Debug mode.  Display debug messages\n"
"  -s ENDPOINT The EchoService endpoint\n"
"              (default: http://localhost/cgi-bin/EchoService.cgi)\n"
"  -c CHARSET  Specify charset (default: UTF-8)\n"
"  -a ACTION   Specify SOAPAction (default: \"\")\n"
"  -u USER     HTTP Authentication username\n"
"              (default: env ECHO_HTTP_USER)\n"
"  -p PASS     HTTP Authentication password\n"
"              (default: env ECHO_HTTP_PASS)\n"
"  -t AUTH     HTTP Auth type (default:0=ANY, 1=BASIC, 2=DIGEST)\n"
"  -S VER      Specify SSL Version used\n"
"              (default:0=ALL, 1=SSLv2,2=SSLv3,4=TLSv1)\n"
"  -V LEVEL    Specify the SSL Server Certification Level\n"
"              (default:0=DON'T CARE, 1=STRICT)\n"
#endif /* HAVE_GETOPT_LONG */
;

static
void
EchoClientHelp(char *arg0, FILE *fp) {
    int service;
    fprintf(fp, USAGE_FORMAT, arg0);
    for(service = 0; service < ECHO_SERVICE_NUM; service++) {
        fprintf(stderr, " test:%d = %s\n", service, ServiceName(service));
    }
}

int
main(int argc,
     char **argv) {
    OpenSOAPEnvelopePtr request = NULL;
    OpenSOAPEnvelopePtr response = NULL;
    OpenSOAPBlockPtr responseBlock = NULL;
    
    time_t tmp_time_t;

    EchoService service;
    char *endpoint = (char *)DEFAULT_ENDPOINT;
 
    void *input_data;
    void *output_data;
    
    long input_data_int;
    char *input_data_string = NULL;
    float input_data_float;
    struct tm input_data_dateTime;
    double input_data_double;

    long output_data_int;
    char *output_data_string = NULL;
    float output_data_float;
    struct tm output_data_dateTime;
    double output_data_double;

    int ret;
    int pass = 0;

    /* option val */
    int cc;
#if defined(HAVE_GETOPT_LONG)
    int option_index = 0;
    static struct option  long_options[] = {
        { "debug"   , 0, 0, 'd' },
        { "verbose" , 0, 0, 'v' },
        { "help"    , 0, 0, 'h' },
        { "endpoint", 0, 0, 's' },
        { "http-user", 1, 0, 'u' },
        { "http-passwd", 1, 0, 'p' },
        { "http-type", 1, 0, 't' },
        { "sslver", 1, 0, 'S' },
        { "verify", 1, 0, 'V' },
        { "ca_dir", 1, 0, 0 },
        { "ca_file", 1, 0, 0 },
        { "certchain_file", 1, 0, 0 },
        { "privkey_file", 1, 0, 0 },
        { "charset", 1, 0, 'c'},
        { "soapaction", 1, 0, 'a'},
        { 0, 0, 0, 0}
    };
    while ((cc = getopt_long(argc, argv, "dvhs:u:p:t:S:V:c:a:", long_options,
                             &option_index)) != -1) {
        switch (cc) {
        case 0:
            switch (option_index) {
            case 9: /* ca_dir */
                Echo_ca_dir = optarg;
                break;
            case 10: /* ca_file */
                Echo_ca_file = optarg;
                break;
            case 11: /* certchain_file */
                Echo_certchain_file = optarg;
                break;
            case 12: /* privkey_file */
                Echo_privkey_file = optarg;
                break;
            }
            break;
#else /* !HAVE_GETOPT_LONG */
    while ((cc = getopt(argc, argv, "dvhs:u:p:t:S:V:c:a:")) != -1) {
        switch (cc) {
#endif /* HAVE_GETOPT_LONG */
        case 'd':
            Echo_debug = 1;
            break;
        case 'v':
            Echo_verbose = 1;
            break;
        case 's':
            endpoint = optarg;
            break;
        case 'u':
            Echo_user = optarg;
            break;
        case 'p':
            Echo_pass = optarg;
            break;
        case 't':
            Echo_auth = atoi(optarg);
            break;
        case 'S':
            Echo_sslver = atoi(optarg);
            break;
        case 'V':
            Echo_verify_level = atoi(optarg);
            break;
        case 'c':
            Echo_charset = optarg;
            break;
        case 'a':
            Echo_soapaction = optarg;
            break;
        case '?':
            EchoClientHelp(argv[0], stderr);
            return 1;
        case 'h':
            EchoClientHelp(argv[0], stdout);
            return 0;
        default:
            break;
        }
    }
    
    if (optind + 1 > argc) {
        EchoClientHelp(argv[0], stderr);
        exit(1);
    }

    if (Echo_user == NULL) {
        Echo_user = getenv("ECHO_HTTP_USER");
    }
    if (Echo_pass == NULL) {
        Echo_pass = getenv("ECHO_HTTP_PASSWD");
    }
    if (getenv("ECHO_HTTP_AUTHTYPE")) {
        Echo_auth = atoi(getenv("ECHO_HTTP_AUTHTYPE"));
    }

    service = atoi(argv[optind]);
    if (service < 0 || ECHO_SERVICE_NUM <= service ) {
        fprintf(stderr, "invalid service type\n");
        exit(1);
    }
    switch(service) {
    case ECHO_STRING:
        if (optind + 2 != argc ) {
            fprintf(stderr, "Only 1 arguement required.\n");
            exit(1);
        }
        input_data_string = argv[optind + 1];
        input_data = input_data_string;
        output_data = &output_data_string;
        break;
    case ECHO_INTEGER:
        if (optind + 2 != argc ) {
            fprintf(stderr, "Only 1 arguement required.\n");
            exit(1);
        }
        input_data_int = atol(argv[optind + 1]);
        input_data = &input_data_int;
        output_data = &output_data_int;
        break;
    case ECHO_FLOAT:
        if (optind + 2 != argc ) {
            fprintf(stderr, "Only 1 arguement required.\n");
            exit(1);
        }
        input_data_float = atof(argv[optind + 1]);
        input_data = &input_data_float;
        output_data = &output_data_float;
        break;
    case ECHO_DATE:
        if (optind + 2 <=  argc ) {
            if (!strptime(argv[optind + 1], "%c", &input_data_dateTime)) {
                fprintf(stderr, "Error! dateTime format is invalid.\n");
                exit(1);
            }
            input_data_dateTime.tm_isdst = 0;
        } else {
            time(&tmp_time_t);
            localtime_r(&tmp_time_t, &input_data_dateTime);
        }
        input_data = &input_data_dateTime;
        output_data = &output_data_dateTime;
        break;
    case ECHO_DOUBLE:
        if (optind + 2 != argc ) {
            fprintf(stderr, "Only 1 arguement required.\n");
            exit(1);
        }
        input_data_double = atof(argv[optind + 1]);
        input_data = &input_data_double;
        output_data = &output_data_double;
        break;
    default:
        ECHO_SERVICE_NOT_IMPLEMENTED(service);
        exit(1);
    }

    /* initialize client */
    OpenSOAPInitialize(NULL);
    
    /* create request message */
    ret = EchoSetRequest(service, input_data, &request);
    if (Echo_debug) fprintf(stderr, "EchoSetRequest:%08x\n", ret);
    if (Echo_verbose) PrintEnvelope(request, "request");

    /* invoke service */
    ret = EchoInvokeService(endpoint, request, &response);
    if (Echo_debug) fprintf(stderr, "EchoInvokeService:%08x\n", ret);
    if (OPENSOAP_SUCCEEDED(ret)) {
        if (Echo_verbose) PrintEnvelope(response, "response");

        /* */
        ret = OpenSOAPEnvelopeGetBodyBlockMB(response,
                                                   "Fault",
                                                   &responseBlock);
        if (OPENSOAP_SUCCEEDED(ret) && responseBlock) {
            /* is fault */
            
            /* output fault */
            EchoOutputFault(responseBlock, NULL);
        } else {
            /* parse response message */
            ret = EchoGetResponse(service, output_data, response);
            if (Echo_debug) fprintf(stderr, "EchoGetResponse:%08x\n", ret);
            
            switch(service) {
            case ECHO_STRING:
                printf("send=[%s],receive=[%s]", input_data_string, output_data_string);
                if (!strcmp(input_data_string, output_data_string)) {
                    pass = 1;
                } else {
                    pass = 0;
                }
                free(output_data_string);
                break;
            case ECHO_INTEGER:
                printf("send=[%ld],receive=[%ld]  : ", input_data_int, output_data_int);
                if (input_data_int == output_data_int) {
                    pass = 1;
                } else {
                    pass = 0;
                }
                break;
            case ECHO_FLOAT:
                printf("send=[%f],receive=[%f]", input_data_float, output_data_float);
                if (input_data_float == output_data_float) {
                    pass = 1;
                } else {
                    pass = 0;
                }
                break;
            case ECHO_DATE:
                printf("send=%sreceive=%s", asctime(&input_data_dateTime), asctime(&output_data_dateTime));
                if (difftime(mktime(&input_data_dateTime),
                             mktime(&output_data_dateTime)) == 0) {
                    pass = 1;
                } else {
                    pass = 0;
                }
                break;
            case ECHO_DOUBLE:
                printf("DOUBLE: send=[%f],receive=[%f]", input_data_double, output_data_double);
                if (input_data_double == output_data_double) {
                    pass = 1;
                } else {
                    pass = 0;
                }
                break;
            default:
                ECHO_SERVICE_NOT_IMPLEMENTED(service);
                exit(1);
            }
        }
        if (pass == 1) {
            printf(" - PASSED\n");
        } else {
            printf(" - FAILED\n");
        }
    }
    
    /* finalize client */
    
    OpenSOAPEnvelopeRelease(response);
    OpenSOAPEnvelopeRelease(request);
    OpenSOAPUltimate();
    
    return 0;
}
