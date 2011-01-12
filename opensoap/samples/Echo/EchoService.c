/*-----------------------------------------------------------------------------
 * $RCSfile: EchoService.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Service.h>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <locale.h>

#include "Echo.h"

#if !defined(CONNECT_TYPE)
# define CONNECT_TYPE "stdio"
#endif /* CONNECT_TYPE */

static
int
EchoParseSimple(int service,
                OpenSOAPEnvelopePtr request,
                OpenSOAPEnvelopePtr *response,
                void *opt) {
    int ret = 0;
    void *data = NULL;
    long data_int = 0;
    float data_float = 0;
    double data_double = 0;
    struct tm data_tm;
    OpenSOAPStringPtr data_string = NULL;
    OpenSOAPBlockPtr body = NULL;
    OpenSOAPStringCreate(&data_string);

    switch(service) {
    case ECHO_STRING:
        data = &data_string;
        break;
    case ECHO_INTEGER:
        data = &data_int;
        break;
    case ECHO_FLOAT:
        data = &data_float;
        break;
    case ECHO_DATE:
        data = &data_tm;
        break;
    case ECHO_DOUBLE:
        data = &data_double;
        break;
    default:
        break;
    }

    /* parse request message */
    OpenSOAPEnvelopeGetBodyBlockMB(request, SERVICE_TABLE[service].method, &body);
    OpenSOAPBlockGetChildValueMB(body, SERVICE_TABLE[service].req,
                                 SERVICE_TABLE[service].type, data);

    /* make response */
    OpenSOAPEnvelopeCreateMB("1.1", NULL, response);
    OpenSOAPEnvelopeAddBodyBlockMB(*response, SERVICE_TABLE[service].res, &body);
    OpenSOAPBlockSetNamespaceMB(body,
                                "http://soapinterop.org/",
                                "namesp1");
    OpenSOAPBlockSetChildValueMB(body, "return", SERVICE_TABLE[service].type, data);

    OpenSOAPStringRelease(data_string);
    return ret;
}

static
int
EchoStringServiceFunc(OpenSOAPEnvelopePtr request,
            OpenSOAPEnvelopePtr *response,
            void *opt) {
    return EchoParseSimple(ECHO_STRING, request, response, opt);
}

static
int
EchoIntegerServiceFunc(OpenSOAPEnvelopePtr request,
                       OpenSOAPEnvelopePtr *response,
                       void *opt) {
    return EchoParseSimple(ECHO_INTEGER, request, response, opt);
}

static
int
EchoFloatServiceFunc(OpenSOAPEnvelopePtr request,
                     OpenSOAPEnvelopePtr *response,
                     void *opt) {
    return EchoParseSimple(ECHO_FLOAT, request, response, opt);
}

static
int
EchoDateTimeServiceFunc(OpenSOAPEnvelopePtr request,
                        OpenSOAPEnvelopePtr *response,
                        void *opt) {
    return EchoParseSimple(ECHO_DATE, request, response, opt);
}

static
int
EchoDoubleServiceFunc(OpenSOAPEnvelopePtr request,
                      OpenSOAPEnvelopePtr *response,
                      void *opt) {
    return EchoParseSimple(ECHO_DOUBLE, request, response, opt);
}

int
main(void) {
    OpenSOAPServicePtr service = NULL;

    OpenSOAPInitialize(NULL);
    setlocale(LC_CTYPE, "japanese"); 

    OpenSOAPServiceCreateMB(&service, "EchoService", CONNECT_TYPE, 0);

    OpenSOAPServiceRegisterMB(service, SERVICE_TABLE[ECHO_STRING].method,  EchoStringServiceFunc, NULL);
    OpenSOAPServiceRegisterMB(service, SERVICE_TABLE[ECHO_INTEGER].method, EchoIntegerServiceFunc, NULL);
    OpenSOAPServiceRegisterMB(service, SERVICE_TABLE[ECHO_FLOAT].method,   EchoFloatServiceFunc, NULL);
    OpenSOAPServiceRegisterMB(service, SERVICE_TABLE[ECHO_DATE].method,    EchoDateTimeServiceFunc, NULL);
    OpenSOAPServiceRegisterMB(service, SERVICE_TABLE[ECHO_DOUBLE].method,    EchoDoubleServiceFunc, NULL);

    OpenSOAPServiceRun(service);

    OpenSOAPServiceRelease(service);

    OpenSOAPUltimate();

    return 0;
}
