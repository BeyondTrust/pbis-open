/*-----------------------------------------------------------------------------
 * $RCSfile: CalcClient.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: CalcClient.c,v 1.29 2001/12/18 02:24:58 bandou Exp $";
#endif  /* _DEBUG */

#include <OpenSOAP/Envelope.h>
#include <OpenSOAP/XMLAttr.h>
#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Transport.h>

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

static
const unsigned char
NULL_CHAR = '\0';

static const int FILE_DELM =
#if defined(__GNUC__)
'/';
#else
// 
'\\';
#endif

static const char DEFAULT_ENDPOINT[]
= "http://opensoap.jp/cgi-bin/CalcService.cgi";

static
const char
DEFAULT_CHARENC[]
= "UTF-8";

static const char OPTION_PREFIX[] = "-";

static const char OPTION_SOAPENDPOINT[] = "s";
static const char OPTION_SOAPACTION[] = "a";
static const char OPTION_CHARENC[] = "c";

static const char USAGE_FORMAT[] = "\
Usage: %s [-s endpoint] [-a soapaction] [-c char_enc] operand_A operator operand_B\n\
  -s endpoint    SOAP service endpoint URI\n\
  -a soapaction  SOAPAction Header\'s value Prefix\n\
  -c char_enc    request character encoding. If not set, this as UTF-8, \n\
                 not estimate.\n\
  operand_A      operand A\'s value\n\
  operator       operator +, -, x, or / \n\
  operand_B      operand B\'s value\n\
";

static  const   char    CALC_METHOD_NAMESPACE_URI[] =
"http://tempuri.org/message/";

static  const   char    CALC_METHOD_NAMESPACE_PREFIX[] =
"m";

const int RequestNamesHashValue = '*';
const int ProxyMultiplyChar = 'x';

static const char *RequestNames[] = {
    "Multiply",                         /* * */
    "Add",                              /* + */
    NULL,                               /* , */
    "Subtract",                         /* - */
    NULL,                               /* . */
    "Divide"                            /* / */
};

static const char *ResponseNames[] = {
    "MultiplyResponse",                 /* * */
    "AddResponse",                      /* + */
    NULL,                               /* , */
    "SubtractResponse",                 /* - */
    NULL,                               /* . */
    "DivideResponse"                    /* / */
};

static const size_t RequestNamesSize
= sizeof(RequestNames) / sizeof(const char *);

static const char SERVICE_OPERAND_A_NAME[] = "A";
static const char SERVICE_OPERAND_B_NAME[] = "B";

static const char SERVICE_RESPONSE_RESULT_NAME[] = "Result";

typedef  struct {
    char        *app_name;
    
    char        *soap_endpoint;
    char        *soap_action;
    char        *char_enc;
    const char  *service_method_name;
    double      service_operand_A;
    double      service_operand_B;
    const char  *service_response_name;

    OpenSOAPEnvelopePtr soap_request_env;
    OpenSOAPEnvelopePtr soap_response_env;
} CalcClientVariables;


static
int
CalcClientVariablesRelease(CalcClientVariables *app_vars) {
	int ret = 0;

	if (app_vars) {
		int error_code = OpenSOAPEnvelopeRelease(app_vars->soap_response_env);
		ret = 1;
		if (OPENSOAP_FAILED(error_code)) {
			fprintf(stderr,
					"Response Envelope Release error."
					" error code = %x\n",
					error_code);
			ret = 0;
		}
		error_code = OpenSOAPEnvelopeRelease(app_vars->soap_request_env);
		if (OPENSOAP_FAILED(error_code)) {
			fprintf(stderr,
					"Request Envelope Release error."
					" error code = %x\n",
					error_code);
			ret = 0;
		}

		free(app_vars->soap_endpoint);
		free(app_vars->soap_action);
		free(app_vars->char_enc);
		free(app_vars->app_name);
	}

	return ret;
}

static
char *
CalcClientGetAppName(const char *argv0) {
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
    ret = malloc(app_name_len + 1);
    
    return ret ? strcpy(ret, app_name) : NULL;
}

static
int
CalcClientVariablesInitialize(CalcClientVariables *app_vars,
                              int argc,
                              char **argv) {
    int     ret = 1;
    const size_t OPTION_PREFIX_LEN = strlen(OPTION_PREFIX);
    char **av_end = argv + argc;
    memset((void *)app_vars, 0, sizeof(*app_vars)); /* clear */

    app_vars->app_name = CalcClientGetAppName(*argv);

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
            }
            else if (strncmp(OPTION_SOAPACTION,
                               opt_body, opt_body_len) == 0
                && !app_vars->soap_action) {
                ++argv;
                if (argv == av_end) {
                    break;
                }
                app_vars->soap_action = strdup(*argv);
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

    ret = 0;
    if (argv != av_end) {
        app_vars->service_operand_A = atof(*argv);
        ++argv;
        if (argv != av_end) {
            int req_hash =  **argv;
            if (ProxyMultiplyChar == req_hash) {
                req_hash = RequestNamesHashValue;
            }
            else if (RequestNamesHashValue == req_hash) {
                req_hash = ProxyMultiplyChar;
            }
            req_hash -= RequestNamesHashValue;
            if (0 <= req_hash
                && (unsigned)req_hash < RequestNamesSize
                && RequestNames[req_hash]) {
                app_vars->service_method_name = RequestNames[req_hash];
                app_vars->service_response_name = ResponseNames[req_hash];
                ++argv;
                if (argv != av_end) {
                    app_vars->service_operand_B = atof(*argv);
                    ret = 1;
                }
            }
        }        
    }

    if (!app_vars->soap_endpoint) {
        app_vars->soap_endpoint =
            strdup(DEFAULT_ENDPOINT);
    }

    if (ret && !app_vars->char_enc) {
        app_vars->char_enc
            = strdup(DEFAULT_CHARENC);
        if (!app_vars->char_enc) {
            fputs("memory allocate error\n", stderr);
            ret = 0;
        }
    }
    
    if (!ret) {
        printf(USAGE_FORMAT,
               app_vars->app_name ? app_vars->app_name : "CalcClient");
    }


    return ret;
}

static
int
CalcClientCreateRequestMessage(CalcClientVariables *app_vars) {
    int ret = 1;

    OpenSOAPBlockPtr tr_h_block = NULL;
    OpenSOAPBlockPtr soap_request_method = NULL;
    long    t_value = 5;
    OpenSOAPXMLAttrPtr nanika_attr = NULL;
    OpenSOAPStringPtr   nanika_value = NULL;
    OpenSOAPXMLElmPtr   param_a_elm = NULL;
    OpenSOAPXMLElmPtr   param_b_elm = NULL;
    OpenSOAPXMLAttrPtr  param_a_encrypt_attr = NULL;
    OpenSOAPXMLAttrPtr  param_b_encrypt_attr = NULL;
    int param_a_encrypt_yes = 1;
    int param_b_encrypt_yes = 0;
    int error_code = OPENSOAP_NO_ERROR;

    error_code = OpenSOAPStringCreateWithMB("", &nanika_value);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot create nanika attribute value string\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }

    error_code
        = OpenSOAPEnvelopeCreateMB("1.1",
                                   NULL,
                                   &app_vars->soap_request_env);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot create SOAP Envelope\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }
    
    /* Add Header block */
    error_code
        = OpenSOAPEnvelopeAddHeaderBlockMB(app_vars->soap_request_env,
                                           "Transaction",
                                           &tr_h_block);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot add Transaction header\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }
    /* Set header block's namespace */
    error_code
        = OpenSOAPBlockSetNamespaceMB(tr_h_block,
                                        "some-URI",
                                      "t");
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot set Transaction\'s namespace\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }
    /* Set Header block value */
    error_code
        = OpenSOAPBlockSetValueMB(tr_h_block,
                                  "int",
                                  &t_value);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot set Transaction\'s value\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }
    /* Set mustunderstand attribute */
    error_code
        = OpenSOAPBlockSetMustunderstandAttr(tr_h_block);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot set mustUnderstand attribute\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }
    /* Set actor attribute to next */
    error_code
        = OpenSOAPBlockSetActorAttrNext(tr_h_block);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot set actor attribute to next\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }
    /* Set nanika attirbute */
    error_code
        = OpenSOAPBlockAddAttributeMB(tr_h_block,
                                      "nanika",
                                      "string",
                                      &nanika_value,
                                      &nanika_attr);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot add nanika attribute to Transaction block\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }

    /* add body request block */
    error_code
        = OpenSOAPEnvelopeAddBodyBlockMB(app_vars->soap_request_env,
                                         app_vars->service_method_name,
                                         &soap_request_method); 
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot add body request block\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }

    /* set namespace to request block */
    error_code
        = OpenSOAPBlockSetNamespaceMB(soap_request_method,
                                      CALC_METHOD_NAMESPACE_URI,
//                                      "");
                                      NULL);
//                                      CALC_METHOD_NAMESPACE_PREFIX);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot set namespace to request block\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }

    /* set request parameter A */
    error_code
        = OpenSOAPBlockSetChildValueMB(soap_request_method,
                                       SERVICE_OPERAND_A_NAME,
                                       "double",
                                       &app_vars->service_operand_A);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot set request parameter A\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }

    /* set request parameter B */
    error_code
        = OpenSOAPBlockSetChildValueMB(soap_request_method,
                                       SERVICE_OPERAND_B_NAME,
                                       "double",
                                       &app_vars->service_operand_B);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot set request parameter B\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }

    /* get parameter A XML Element */
    error_code
        = OpenSOAPBlockGetChildMB(soap_request_method,
                                  SERVICE_OPERAND_A_NAME,
                                  &param_a_elm);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot get request parameter A\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }

    /* add encrypt attribute to parameter A */
    error_code
        = OpenSOAPXMLElmAddAttributeMB(param_a_elm,
                                       "encrypt",
                                       "boolean",
                                       &param_a_encrypt_yes,
                                       &param_a_encrypt_attr);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot set encrypt attribute to parameter A\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }

    /* set namespace of encrypt attribute */
    error_code
        = OpenSOAPXMLAttrSetNamespaceMB(param_a_encrypt_attr,
                                        "http://opensoap.jp/auth/",
                                        "a");
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot set  namespace of encrypt attribute of parameter A\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }
    
    /* get parameter B XML Element */
    error_code
        = OpenSOAPBlockGetChildMB(soap_request_method,
                                  SERVICE_OPERAND_B_NAME,
                                  &param_b_elm);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot get request parameter B\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }

    /* add encrypt attribute to parameter B */
    error_code
        = OpenSOAPXMLElmAddAttributeMB(param_b_elm,
                                       "encrypt",
                                       "boolean",
                                       &param_b_encrypt_yes,
                                       &param_b_encrypt_attr);
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot set encrypt attribute to parameter B\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }

    /* set namespace of encrypt attribute */
    error_code
        = OpenSOAPXMLAttrSetNamespaceMB(param_b_encrypt_attr,
                                        "http://opensoap.jp/auth/",
                                        "a");
    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "Cannot set  namespace of encrypt attribute of parameter B\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 0;
    }
    
    return ret;
}

static
int
CalcClientProcResponseMessage(CalcClientVariables *app_vars) {
    int ret = 0;
    /* */
    const unsigned char *buf_beg = NULL;
    size_t buf_sz = 0;
    size_t param_len = 0;
    char *param_value = NULL;
    int is_same_ns = 0;
    OpenSOAPBlockPtr    soap_response_blk = NULL;
    int error_code
        = OpenSOAPEnvelopeGetBodyBlockMB(app_vars->soap_response_env,
                                         "Fault",
                                         &soap_response_blk);
    if (OPENSOAP_SUCCEEDED(error_code) && soap_response_blk) {
        OpenSOAPStringPtr   fault_param = NULL;
        OpenSOAPXMLElmPtr   detail_elm  = NULL;
		
        OpenSOAPStringCreate(&fault_param);
        fputs("Fault\n", stdout);
        if (OPENSOAP_SUCCEEDED(error_code
                               = OpenSOAPBlockGetChildValueMB(soap_response_blk,
                                                              "faultcode",
                                                              "string",
                                                              &fault_param))
            && OPENSOAP_SUCCEEDED(error_code
                                  = OpenSOAPStringGetLengthMB(fault_param,
                                                              &param_len))
            && (param_value = malloc(param_len + 1))
            && OPENSOAP_SUCCEEDED(error_code
                                  = OpenSOAPStringGetStringMB(fault_param,
                                                              &param_len,
                                                              param_value))) {
            printf("faultcode: %s\n", 
                   param_value);
        }
        else {
            fputs("faultcode not found\n", stdout);
        }
        free(param_value); param_value = NULL;
        if (OPENSOAP_SUCCEEDED(error_code
                               = OpenSOAPBlockGetChildValueMB(soap_response_blk,
															  "faultstring",
															  "string",
															  &fault_param))
            && OPENSOAP_SUCCEEDED(error_code
								  = OpenSOAPStringGetLengthMB(fault_param,
															  &param_len))
            && (param_value = malloc(param_len + 1))
            && OPENSOAP_SUCCEEDED(error_code
								  = OpenSOAPStringGetStringMB(fault_param,
															  &param_len,
															  param_value))) {
            printf("faultstring: %s\n", 
                   param_value);
        }
        else {
            fputs("faultstring not found\n", stdout);
        }
        free(param_value); param_value = NULL;
        if (OPENSOAP_SUCCEEDED(error_code
                               = OpenSOAPBlockGetChildMB(soap_response_blk,
                                                         "detail",
                                                         &detail_elm))) {
			OpenSOAPXMLElmPtr   detail_child_elm  = NULL;
            error_code
				= OpenSOAPXMLElmGetNextChild(detail_elm,
											 &detail_child_elm);
			if (OPENSOAP_SUCCEEDED(error_code) && detail_child_elm) {
				OpenSOAPByteArrayPtr    detail_buf = NULL;
				/* detail child fuound */
				error_code = OpenSOAPByteArrayCreate(&detail_buf);
				if (OPENSOAP_SUCCEEDED(error_code)) {
					size_t detail_buf_sz = 0;
					do {
						char *realloc_buf = NULL;
						error_code
							= OpenSOAPXMLElmGetCharEncodingString(detail_child_elm,
																  NULL,
																  detail_buf);
						if (OPENSOAP_SUCCEEDED(error_code)
							&& OPENSOAP_SUCCEEDED(error_code
												  = OpenSOAPByteArrayGetBeginSizeConst(detail_buf,
																					   &buf_beg,
																					   &buf_sz))
							&& (realloc_buf
								= realloc(param_value,
										  detail_buf_sz + buf_sz + 1))) {
							param_value = realloc_buf;
							memcpy(param_value + detail_buf_sz, buf_beg, buf_sz);
							detail_buf_sz += buf_sz;
						}
						else {
							break;
						}
						error_code
							= OpenSOAPXMLElmGetNextChild(detail_elm,
														 &detail_child_elm);
					} while (OPENSOAP_SUCCEEDED(error_code)
							 && detail_child_elm);
					/* null terminated */
					if (param_value) {
						param_value[detail_buf_sz] = NULL_CHAR;
					}
					/* release buffer */
					OpenSOAPByteArrayRelease(detail_buf);
				}
			}
			else {
				/* detail is value */
				error_code
					= OpenSOAPXMLElmGetValueMB(detail_elm,
											   "string",
											   &fault_param);
				if (OPENSOAP_SUCCEEDED(error_code) 
					&& OPENSOAP_SUCCEEDED(error_code
										  = OpenSOAPStringGetLengthMB(fault_param,
																	  &param_len))
					&& (param_value = malloc(param_len + 1))) {
					error_code
						= OpenSOAPStringGetStringMB(fault_param,
													&param_len,
													param_value);
				}
				if (OPENSOAP_FAILED(error_code)) {
					free(param_value); param_value = NULL;
				}
			}
        }
		if (param_value) {
			printf("detail: %s\n", 
				   param_value);
		}
        else {
            fputs("detail not found\n", stdout);
        }
        free(param_value); param_value = NULL;
    }
	else if (OPENSOAP_SUCCEEDED(error_code
                                = OpenSOAPEnvelopeGetBodyBlockMB(app_vars->soap_response_env,
                                                                 app_vars
                                                                 ->service_response_name,
                                                                 &soap_response_blk))
             && OPENSOAP_SUCCEEDED(error_code
                                   = OpenSOAPBlockIsSameNamespaceMB(soap_response_blk,
                                                                    CALC_METHOD_NAMESPACE_URI,
                                                                    &is_same_ns))
             && is_same_ns) {
        double result_value = 0;
        error_code
            = OpenSOAPBlockGetChildValueMB(soap_response_blk,
                                           SERVICE_RESPONSE_RESULT_NAME,
                                           "double",
                                           &result_value);
        if (OPENSOAP_SUCCEEDED(error_code)) {
            printf("%s: %f\n",
                   SERVICE_RESPONSE_RESULT_NAME,
                   result_value);
        }
    }
    else {
        printf("%s not found\n",
               SERVICE_RESPONSE_RESULT_NAME);
    }
    ret = 1;
    return ret;
}

static
void
CalcClientOutputEnvelope(OpenSOAPEnvelopePtr env,
						 const char *char_enc,
						 const char *msg,
						 FILE *fp) {
	if (!msg) {
		msg = "";
	}
	if (env && fp) {
		OpenSOAPByteArrayPtr env_buf = NULL;
		int error_code = OpenSOAPByteArrayCreate(&env_buf);
		if (OPENSOAP_SUCCEEDED(error_code)) {
			error_code
				= OpenSOAPEnvelopeGetCharEncodingString(env,
														char_enc,
														env_buf);
			if (OPENSOAP_SUCCEEDED(error_code)) {
				const unsigned char *env_beg = NULL;
				size_t env_sz = 0;
				error_code
					= OpenSOAPByteArrayGetBeginSizeConst(env_buf,
														 &env_beg,
														 &env_sz);
				if (OPENSOAP_SUCCEEDED(error_code)) {
					fprintf(fp,
							"\n"
							"=== %s soap envelope begin ===\n",
							msg);
					fwrite(env_beg, 1, env_sz, fp);
					fprintf(fp,
							"\n"
							"=== %s soap envelope  end  ===\n",
							msg);
				}
			}

			OpenSOAPByteArrayRelease(env_buf);
		}
	}
}

static
int
CalcClientInvokeMessage(CalcClientVariables *app_vars) {
    int ret = 0;

    OpenSOAPTransportPtr    transport = NULL;
    int error_code = OpenSOAPTransportCreate(&transport);
    if (OPENSOAP_SUCCEEDED(error_code)) {
        error_code
            = OpenSOAPTransportSetURL(transport,
                                      app_vars->soap_endpoint);
        if (OPENSOAP_SUCCEEDED(error_code)) {
            error_code
                = OpenSOAPTransportSetSOAPAction(transport,
                                                 app_vars->soap_action);
            if (OPENSOAP_SUCCEEDED(error_code)) {
                error_code
                    = OpenSOAPTransportSetCharset(transport,
                                                  app_vars->char_enc);
                if (OPENSOAP_SUCCEEDED(error_code)) {
					/* */
					CalcClientOutputEnvelope(app_vars
											 ->soap_request_env,
											 app_vars->char_enc,
											 "request",
											 stderr);
					/* */
                    error_code
                        = OpenSOAPTransportInvoke(transport,
                                                  app_vars
                                                  ->soap_request_env,
                                                  &app_vars
                                                  ->soap_response_env);
                    if (OPENSOAP_SUCCEEDED(error_code)) {
						/* */
						CalcClientOutputEnvelope(app_vars
												 ->soap_response_env,
												 NULL,
												 "response",
												 stderr);
						/* */
                        ret = 1;
                    }
                    else {
                        fprintf(stderr,
                                "Cannot Invoke. ErrorCode=%x\n",
                                error_code);
                    }
                }
                else {
                    fprintf(stderr,
                            "Cannot Set Charset. ErrorCode=%x\n",
                            error_code);
                }
            }
            else {
                fprintf(stderr,
                        "Cannot Set SOAPAction. ErrorCode=%x\n",
                        error_code);
            }
        }
        else {
            fprintf(stderr,
                    "Cannot Set URL. ErrorCode=%x\n",
                    error_code);
        }

        OpenSOAPTransportRelease(transport);
    }
    else {
        fprintf(stderr,
                "Cannot create Transport. ErrorCode=%x\n",
                error_code);
    }
    
    return ret;
}

int
main(int argc,
     char **argv) {
    CalcClientVariables app_vars;       /* application variables */
    /* initialize opensoap API */
    int error_code = OpenSOAPInitialize(NULL);

    if (OPENSOAP_FAILED(error_code)) {
        fprintf(stderr,
                "OpenSOAP API Initialize Failed.\n"
                "OpenSOAP Error Code: 0x%04x\n",
                error_code);
        
        return 1;
    }
    
    if (!CalcClientVariablesInitialize(&app_vars, argc, argv)) {
        OpenSOAPUltimate();
        return 1;
    }

    printf("SOAP EndPoint: %s\nSOAPAction: %s\n"
           "request name: %s\nA: %f\nB: %f\n",
           app_vars.soap_endpoint ? app_vars.soap_endpoint : "",
           app_vars.soap_action ? app_vars.soap_action : "",
           app_vars.service_method_name,
           app_vars.service_operand_A,
           app_vars.service_operand_B);

    if (!CalcClientCreateRequestMessage(&app_vars)) {
        OpenSOAPUltimate();
        return 1;
    }

    if (!CalcClientInvokeMessage(&app_vars)) {
        OpenSOAPUltimate();
        return 1;
    }

    if (!CalcClientProcResponseMessage(&app_vars)) {
        OpenSOAPUltimate();
        return 1;
    }

	if (!CalcClientVariablesRelease(&app_vars)) {
        OpenSOAPUltimate();
		return 1;
	}
    
    OpenSOAPUltimate();

    return 0;
}
