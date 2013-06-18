/*-----------------------------------------------------------------------------
 * $RCSfile: CalcService.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>

#include "OpenSOAP/OpenSOAP.h"
#include "OpenSOAP/Service.h"

#if !defined(CONNECT_TYPE)
# define CONNECT_TYPE "stdio"
#endif /* CONNECT_TYPE */

static
const   char
CALC_METHOD_NAMESPACE_URI[] =
"http://tempuri.org/message/";
static
const   char
CALC_METHOD_NAMESPACE_PREFIX[] =
"m";

static
const char
SERVICE_OPERAND_A_NAME[] = "A";
static
const char
SERVICE_OPERAND_B_NAME[] = "B";

static
const char
SERVICE_RESPONSE_RESULT_NAME[] = "Result";


typedef 
int
(*CalcFunctionType)(double a, double b, double *r);

static
int
Add(double a,
    double b,
    double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (r) {
        *r = a + b;
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
Subtract(double a,
         double b,
         double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (r) {
        *r = a - b;
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
Multiply(double a,
         double b,
         double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (r) {
        *r = a * b;
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
Divide(double a,
       double b,
       double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (r) {
        if (b) {
            *r = a / b;
            ret = OPENSOAP_NO_ERROR;
        }
    }

    return ret;
}

typedef struct {
    const char *requestName;
    const char *responseName;
    CalcFunctionType calcFunc;
} CalcServiceMethodMapItem;

static
int
CalcServiceGetParameter(OpenSOAPEnvelopePtr /* [in] */ request,
                        const char * /* [in] */ request_name,
                        double * /* [out] */ a,
                        double * /* [out] */ b) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (a && b) {
        OpenSOAPBlockPtr body_block = NULL;
        ret = OpenSOAPEnvelopeGetBodyBlockMB(request,
                                             request_name,
                                             &body_block);
        if (OPENSOAP_SUCCEEDED(ret)) {
            int is_same_ns = 0;
            ret = OpenSOAPBlockIsSameNamespaceMB(body_block,
                                                 CALC_METHOD_NAMESPACE_URI,
                                                 &is_same_ns);
            if (OPENSOAP_SUCCEEDED(ret) && is_same_ns) {
                ret = OpenSOAPBlockGetChildValueMB(body_block,
                                                   SERVICE_OPERAND_A_NAME,
                                                   "double",
                                                   a);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPBlockGetChildValueMB(body_block,
                                                       SERVICE_OPERAND_B_NAME,
                                                       "double",
                                                       b);
                }
            }
        }
    }

    return ret;
}

static
int
CalcServiceCreateResponse(const char * /* [in] */ response_name,
                          double /* [in] */ r,
                          OpenSOAPEnvelopePtr * /* [out] */ response) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (response_name && *response_name) {
        ret = OpenSOAPEnvelopeCreateMB(NULL, NULL, response);
        if (OPENSOAP_SUCCEEDED(ret)) {
            OpenSOAPBlockPtr body_block = NULL;
            ret = OpenSOAPEnvelopeAddBodyBlockMB(*response,
                                                 response_name,
                                                 &body_block);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPBlockSetNamespaceMB(body_block,
                                                  CALC_METHOD_NAMESPACE_URI,
                                                  CALC_METHOD_NAMESPACE_PREFIX);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPBlockSetChildValueMB(body_block,
                                                       SERVICE_RESPONSE_RESULT_NAME,
                                                       "double",
                                                       &r);
                }
            }
        }
    }

    return ret;
}

static
int
CalcServiceFunc(OpenSOAPEnvelopePtr /* [in] */ request,
                OpenSOAPEnvelopePtr * /* [out] */ response,
                void * /* [in, out] */ opt) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    CalcServiceMethodMapItem *m_map = (CalcServiceMethodMapItem *)opt;

    if (m_map) {
        double a = 0;
        double b = 0;
        ret = CalcServiceGetParameter(request,
                                      m_map->requestName,
                                      &a,
                                      &b);
        if (OPENSOAP_SUCCEEDED(ret)) {
            double r = 0;
            ret = (m_map->calcFunc)(a, b, &r);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = CalcServiceCreateResponse(m_map->responseName,
                                                r,
                                                response);
            }
        }
    }

    return ret;
}

static
const CalcServiceMethodMapItem
CalcServiceMethodMap[] = {
    {"Add", "AddResponse", Add},
    {"Subtract", "SubtractResponse", Subtract},
    {"Multiply", "MultiplyResponse", Multiply},
    {"Divide", "DivideResponse", Divide},
    {NULL, NULL, NULL}
};

/*
 */
int
main(void) {
    int ret = 0;
    OpenSOAPServicePtr calc_service = NULL;
    int error_code
        = OpenSOAPInitialize(NULL);
    if (OPENSOAP_SUCCEEDED(error_code)) {
        const CalcServiceMethodMapItem *m_map_i = CalcServiceMethodMap;
        error_code 
            = OpenSOAPServiceCreateMB(&calc_service,
                                      "CalcService",
                                      CONNECT_TYPE,
                                      0);
        for (; OPENSOAP_SUCCEEDED(error_code) && m_map_i->requestName;
            ++m_map_i) {
            error_code
                = OpenSOAPServiceRegisterMB(calc_service,
                                            m_map_i->requestName,
                                            CalcServiceFunc,
                                            (void *)m_map_i);
        }
        if (OPENSOAP_SUCCEEDED(error_code)) {
            error_code 
                = OpenSOAPServiceRun(calc_service);
        }

        OpenSOAPServiceRelease(calc_service);

        OpenSOAPUltimate();
    }

    return ret;
}
