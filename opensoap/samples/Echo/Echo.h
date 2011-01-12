/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Echo.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef OPENSOAP_ECHO_H
#  define OPENSOAP_ECHO_H

enum _echo_service {
    ECHO_ALL           = 0,
    ECHO_STRING        = 1,
    ECHO_STRING_ARRAY  = 2,
    ECHO_INTEGER       = 3,
    ECHO_INTEGER_ARRAY = 4,
    ECHO_FLOAT         = 5,
    ECHO_FLOAT_ARRAY   = 6,
    ECHO_STRUCT        = 7,
    ECHO_STRUCT_ARRAY  = 8,
    ECHO_VOID          = 9,
    ECHO_BASE64        = 10,
    ECHO_DATE          = 11,
    ECHO_DOUBLE        = 12
};
typedef enum _echo_service EchoService;
#define ECHO_SERVICE_NUM (ECHO_DOUBLE + 1)

struct _service_table {
    const EchoService service;
    const char * method;
    const char * req;
    const char * res;
    const char * type;
};
typedef struct _service_table ServiceTable;

static const ServiceTable SERVICE_TABLE [ECHO_SERVICE_NUM] = {
    { ECHO_ALL          , "Echo"            , ""                 , ""            },
    { ECHO_STRING       , "echoString"      , "inputString"      , "echoStringResponse"      , "string"      },
    { ECHO_STRING_ARRAY , "echoStringArray" , "inputStringArray" , "echoStringArrayResponse" , ""            },
    { ECHO_INTEGER      , "echoInteger"     , "inputInteger"     , "echoIntegerResponse"     , "int"         },
    { ECHO_INTEGER_ARRAY, "echoIntegerArray", "inputIntegerArray", "echoIntegerArrayResponse", ""            },
    { ECHO_FLOAT        , "echoFloat"       , "inputFloat"       , "echoFloatResponse"       , "float"       },
    { ECHO_FLOAT_ARRAY  , "echoFloatArray"  , "inputFloatArray"  , "echoFloatArrayResponse"  , ""            },
    { ECHO_STRUCT       , "echoStruct"      , "inputStruct"      , "echoStructResponse"      , ""            },
    { ECHO_STRUCT_ARRAY , "echoStructArray" , "inputStructArray" , "echoStructArrayResponse" , ""            },
    { ECHO_VOID         , "echoVoid"        , "inputVoid"        , "echoVoidResponse"        , "void"        },
    { ECHO_BASE64       , "echoBase64"      , "inputBase64"      , "echoBase64Response"      , "base64Binary"},
    { ECHO_DATE         , "echoDate"        , "inputDate"        , "echoDateResponse"        , "dateTime"    },
    { ECHO_DOUBLE       , "echoDouble"      , "inputDouble"      , "echoDoubleResponse"        , "double"    }
};

#define ServiceName(service) (SERVICE_TABLE[service].method)
#define ECHO_SERVICE_NOT_IMPLEMENTED(service) \
fprintf(stderr, "Service type %d:%s not implemented.\n", service, ServiceName(service))

#endif /* OPENSOAP_ECHO_H */
