/*-----------------------------------------------------------------------------
 * $RCSfile: SoapingClient.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if defined(HAVE_CONFIG_H)
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Transport.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <math.h>

#if defined(HAVE_GETOPT_H)
#  include <getopt.h>
#endif /* HAVE_GETOPT_H */
#if !defined(HAVE_GETOPT)
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

#if defined(_MSC_VER)
typedef __int64 LLONG;
#include <winsock.h>

#define snprintf _snprintf

static
int
usleep(unsigned long usec) {
	int ret = -1;
	/* get milisec */
	int msec = usec / 1000L;
	/* infinite sleep avoid */
	if (msec == INFINITE) {
		--msec;
	}
	/* sleep milisec */
	Sleep(msec);
	ret = 0;

	return ret;
}


static
int 
gettimeofday(struct timeval *tv, 
			 struct timezone *tz) {
	int ret = -1;

	if (tv) {
		FILETIME ft;
		ULARGE_INTEGER usec;

		/* get 100-nano sec time since January, 1, 1601 (UTC) */
		GetSystemTimeAsFileTime(&ft);

		/* copy */
		usec.LowPart = ft.dwLowDateTime;
		usec.HighPart = ft.dwHighDateTime;

		/* offset to the epoch time */
		usec.QuadPart -= 116444736000000000i64;

		/* set micro sec */
		usec.QuadPart /= 10;

		tv->tv_sec  = (long)(usec.QuadPart / 1000000L);
		tv->tv_usec = (long)(usec.QuadPart % 1000000L);

		ret = 0;
	}

	return ret;
}

#else
#include <sys/time.h>
#include <unistd.h>

typedef long long LLONG;
#endif

/* use atexit flag */
#define USE_ATEXIT

/* one second as micro second unit */
#define SECOND         (1000000L)
/* TimeOut value default 10sec(?) */
#define DEFAULT_TIMEOUT_MSEC (10000L)
/* Character to fill the sending string with */
#define FILL_CHAR      'P'

#define ENDPOINT_MAX_LEN 256

/* Data size limit 1MB from OpenSOAP specification */
#define MESSAGE_LIMIT  (1024L * 1024)

/* Soaping end point */
static char *soapingEndPoint = NULL;

/*
  use at interrupt function
  ping count
*/
static volatile int  count;
static volatile int  count_recv;
/* statistic values [usec] */
static volatile LLONG totalTime;
static volatile LLONG totalTime2;
static volatile long minTime;
static volatile long maxTime;

static const char USAGE_FORMAT[] = 
/* "Usage: %s [-h] [-s size[K|M|G]] [-c count [-i wait]] [-v] [-m ttl] [-t timeout] [-o | URL]\n" */
"Usage: %s [-h] [-s size[K|M]] [-c count [-i wait]] [-v] [-o] [URL | host]\n"
"Options:\n"
"  -h         display this information\n"
"  -s size    size of string data to send [[K|M]byte] (default: 0) \n"
"  -c count   number of times for continuous transmissions (default: 1)\n"
"  -i wait    interval time for continuous transmissions [msec] (default: 0)\n"
"  -v         verbose mode.  display SOAP message\n"
/*
  "  -m ttl     set time-to-live value [msec] (default : ?)\n"
  "  -t timeout set timeout value [msec] (default : 0)\n"
*/
"  -o         via OpenSOAP server (http://host/cgi-bin/soapInterface.cgi)\n"
"URL :  Soaping end-point (default: http://localhost/cgi-bin/SoapingService.cgi)\n"
"host : end-point addressed by hostname (default: localhot)\n"
"       -> http://host/cgi-bin/{SoapingService.cgi,soapInterface.cgi}\n"
;

/*
  print SOAP Message
*/
static
void
PrintEnvelope(OpenSOAPEnvelopePtr env,
			  const char *label) {
  
	OpenSOAPByteArrayPtr envBuf = NULL;
	const unsigned char *envBeg = NULL;
	size_t envSz = 0;
  
	OpenSOAPByteArrayCreate(&envBuf);
	OpenSOAPEnvelopeGetCharEncodingString(env, NULL, envBuf);
	OpenSOAPByteArrayGetBeginSizeConst(envBuf, &envBeg, &envSz);
  
	fprintf(stderr, "\n=== %s envelope begin ===\n", label);
	fwrite(envBeg, 1, envSz, stderr);
	fprintf(stderr,   "=== %s envelope  end  ===\n", label);

	OpenSOAPByteArrayRelease(envBuf);
}

/*
  get current time [usec]
*/
static
struct timeval
SoapingGetCurrentTime(void) {
	static struct timeval current = {0, 0};
	gettimeofday(&current, NULL);
	
	return (current);
}

/*
  get send string
*/
static
char *
SoapingClientGetStringSize(char *optarg) {

	long  sendStringLen = 0;
	char *sendDataString =NULL;

	char *sizeStr = strdup(optarg);
	size_t sizeStrLen = strlen(sizeStr);
	size_t unitSize = 1;
	
	switch (sizeStr[sizeStrLen - 1]) {
		case 'G':
		case 'g':
			unitSize *= 1024L;
		case 'M':
		case 'm':
			unitSize *= 1024L;
		case 'K':
		case 'k':
			unitSize *= 1024L;
			sizeStr[sizeStrLen - 1] = '\0';
			break;
		default:
			break;
	}
	/* */
	sendStringLen = atol(sizeStr) * unitSize;
	free(sizeStr); 
#if 0
	if (sendStringLen > MESSAGE_LIMIT) {
		printf("Invalid Value: SoapingData must be under %ld bytes\n",
			   MESSAGE_LIMIT);
		exit(1);
	}
#endif
	if (sendStringLen < 0) {
		printf("Invalid Value:[-s size] size >= 0\n");
		exit(1);
	}

	sendDataString = malloc(sendStringLen + 1);

	if (sendDataString) {
		memset(sendDataString, FILL_CHAR, sendStringLen);
		sendDataString[sendStringLen] = '\0';
	}
	else {
		printf("System Limit Over Value: %s size string cannot create\n",
			   optarg);
		exit(1);
	}

	return sendDataString;
}

/*
  command line option Ping Count
*/
static
int
SoapingClientGetPingCount(char *optarg) {
	int pc = atoi(optarg);
	if (pc < 1) {
		printf("Invalid Value:[-c count] count >= 1\n");
		exit(1);
	}

	return pc;
}

/*
  command line option 
*/
static
int
SoapingClientGetSleepTime(char *optarg) {
	int st = atoi(optarg);

	if (st < 0) {
		printf("Invalid Value:[-i wait] wait >= 0\n");
		exit(1);
	}

	return st;
}

/*
 */
static
long
SoapingClientGetTimeOut(char *optarg) {
	long to = atol(optarg);
	if (to < 0) {
		printf("Invalid Value:[-t timeout] timeout >= 0\n");
		exit(1);
	}

	return to;
}

/*
  TimeOut judgement
*/
static
int 
SoapingTimeOut(long timeOut,
			   struct timeval startTime) {
	int ret;
	struct timeval current = SoapingGetCurrentTime();
	if ((current.tv_sec - startTime.tv_sec) * SECOND +
		(current.tv_usec - startTime.tv_usec)
		> timeOut * 1000) {
		ret = 1;
	} else {
		ret = 0;
	}
	
	return ret;
}

/*
  Display Help Message
*/
static
void
SoapingClientHelp(char *arg0, FILE *fp) {
	fprintf(fp, USAGE_FORMAT, arg0);
	return;
}

/*
  print result
*/
static
void
SoapingPrintResult(void) {
	long average;
	int loss = 0;
	if (count) {
		loss = 100 * (count - count_recv) / count;
	}
	if (count_recv) {
		average = totalTime / count_recv;
		totalTime2 = totalTime2 / count_recv;
	} else {
		average = 0;
		totalTime2 = 0;
	}
	printf("\n--- %s soaping statistics ---\n", soapingEndPoint);
	printf("%d messages transmitted, %d received, %d%% loss, time %.3fms\n",
		   count,
		   count_recv,
		   loss,
		   totalTime / 1000.0);
	printf("Round-Trip min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f msec\n",
		   minTime / 1000.0,
		   average / 1000.0,
		   maxTime / 1000.0,
   		   sqrt((double)(totalTime2 - (LLONG) average * average))
		   / 1000.0);

	return;
}

/*
  interrupt function
*/
static
void
onint(int sig) {
#if !defined(USE_ATEXIT)	
	SoapingPrintResult();
#endif /* USE_ATEXIT */	
	exit(0);
}

/*
  round trip calculate
*/
static
void
SoapingClientGetRoundTrip(long dt) {
	if (count_recv > 1) {
		if (minTime > dt) {
			minTime = dt;
		}
		if (maxTime < dt) {
			maxTime = dt;
		}
	}
	else {
		minTime = maxTime = dt;
		totalTime2 =	totalTime = 0;
	}
	totalTime += dt;
	totalTime2 += (LLONG) dt * dt;

	return;
}

/*
  Create request message block
*/
static
int
SoapingCreateRequestMessageBlock(/* [out] */ OpenSOAPEnvelopePtr *request,
								 /* [out] */ OpenSOAPBlockPtr	 *block) {
	/* create request message */
	int ret = OpenSOAPEnvelopeCreateMB(NULL, NULL, request);

	if (OPENSOAP_FAILED(ret)) {
		fprintf(stderr,
				"Cannot create SOAP Envelope\n"
				"OpenSOAP Error Code: 0x%04x\n",
				ret);
	}
	else {
		/* add body request block */
		ret = OpenSOAPEnvelopeAddBodyBlockMB(*request, "Soaping", block);
		if (OPENSOAP_FAILED(ret)) {
			fprintf(stderr,
					"Cannot add body request block\n"
					"OpenSOAP Error Code: 0x%04x\n",
					ret);
		}
		else {
			/* set namespace to request block */
			ret = OpenSOAPBlockSetNamespaceMB(*block,
											  "http://services.opensoap.jp/Soaping/",
											  "m");
			if (OPENSOAP_FAILED(ret)) {
				fprintf(stderr,
						"Cannot set namespace to request block\n"
						"OpenSOAP Error Code: 0x%04x\n",
						ret);
			}
		}
		/* if failed, relese request envelope */
		if (OPENSOAP_FAILED(ret)) {
			OpenSOAPEnvelopeRelease(*request);
		}
	}

	return ret;
}

/*
  Create request message
*/
static
int
SoapingCreateRequestMessage(struct timeval startTime,
							const char *sendStringMB,
							OpenSOAPEnvelopePtr *request) {
	OpenSOAPBlockPtr soapingBlock = NULL;
	/* create request message */
	int ret = SoapingCreateRequestMessageBlock(request,
											   &soapingBlock);
	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr sendString = NULL;
		ret = OpenSOAPStringCreateWithMB(sendStringMB, &sendString);
		if (OPENSOAP_FAILED(ret)) {
			fprintf(stderr,
					"Cannot create OpenSOAP String\n"
					"OpenSOAP Error Code: 0x%04x\n",
					ret);
		}
		else {
			/* set request parameter TimeSec */
			ret = OpenSOAPBlockSetChildValueMB(soapingBlock,
											   "TimeSec",
											   "int",
											   &(startTime.tv_sec));
    
			if (OPENSOAP_FAILED(ret)) {
				fprintf(stderr,
						"Cannot set request parameter TimeSec\n"
						"OpenSOAP Error Code: 0x%04x\n",
						ret);
			}
			else {
				/* set request parameter TimeUSec */
				ret = OpenSOAPBlockSetChildValueMB(soapingBlock,
												   "TimeUSec",
												   "int",
												   &(startTime.tv_usec));
    
				if (OPENSOAP_FAILED(ret)) {
					fprintf(stderr,
							"Cannot set request parameter TimeUSec\n"
							"OpenSOAP Error Code: 0x%04x\n",
							ret);
				}
				else {
					/* set request parameter SendString */
					ret = OpenSOAPBlockSetChildValueMB(soapingBlock,
													   "SendString",
													   "string",
													   &sendString);
				}
			}

			/* release */
			OpenSOAPStringRelease(sendString);
		}
	}
		
	return ret;
}

/*
  invoke service
*/
static
int
SoapingInvokeService(/* [in]  */ const char *soapingEndPoint,
					 /* [in]  */ struct timeval startTime,
					 /* [in]  */ long timeOut,
					 /* [in]  */ OpenSOAPEnvelopePtr request,
					 /* [out] */ OpenSOAPEnvelopePtr *response) {
	OpenSOAPTransportPtr transport = NULL;
	int ret = OpenSOAPTransportCreate(&transport);
	if (OPENSOAP_FAILED(ret)) {
	}
	else {
		ret = OpenSOAPTransportSetURL(transport, soapingEndPoint);
		if (OPENSOAP_FAILED(ret)) {
		}
		else {
			ret = OpenSOAPTransportInvoke(transport, request, response);
			if (OPENSOAP_FAILED(ret)) {
                if (OPENSOAP_TRANSPORT_IS_HTTP_ERROR(ret)) {
                    int http_status = OPENSOAP_TRANSPORT_GET_HTTP_ERROR(ret);
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
                }
                else  {
                    switch(ret) {
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
                        default:
                            fprintf(stderr, "TransportInvoke failed: return code=%08x\n", ret);
                    }
                }
			}
			else {
				/*  Soaping is break over TimeOut   */
				if (!*response && SoapingTimeOut(timeOut, startTime)) {
					ret = OPENSOAP_USERDEFINE_ERROR;
					printf("Time Out : Can't Connect OpenSOAP Server \n");
				}
			}
		}

		/* transport release */
		OpenSOAPTransportRelease(transport);
	}

	return ret;
}

/*
  Fault child value output
*/
static
void
SoapingOutputFaultChildValueMB(/* [in]  */ OpenSOAPBlockPtr faultBlock,
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
SoapingXMLElmOutput(/* [in]  */ OpenSOAPXMLElmPtr xmlChildElm,
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
SoapingOutputFaultDetail(/* [in]  */ OpenSOAPBlockPtr faultBlock,
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
					SoapingXMLElmOutput(detailChildElm,
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
SoapingOutputFault(/* [in]  */ OpenSOAPBlockPtr faultBlock,
				   /* [out] */ FILE *fp) {
	OpenSOAPStringPtr valStr = NULL;
	int errorCode = OpenSOAPStringCreate(&valStr);
	if (!fp) {
		fp = stdout;
	}
	
	fputs("\nFault\n", fp);
	if (OPENSOAP_SUCCEEDED(errorCode)) {
		/* fault code */
		SoapingOutputFaultChildValueMB(faultBlock,
									   "faultcode",
									   valStr,
									   fp);
		/* fault string */
		SoapingOutputFaultChildValueMB(faultBlock,
									   "faultstring",
									   valStr,
									   fp);
		/* detail */
		SoapingOutputFaultDetail(faultBlock,
								 valStr,
								 fp);

		/* valStr release */
		OpenSOAPStringRelease(valStr);
	}
}

/*
  Get response message parameters
*/
static
int
SoapingGetResponseParam(/* [in]  */ OpenSOAPEnvelopePtr response,
						/* [out] */ struct timeval *replyTime,
						/* [out] */ char **replyString) {
	OpenSOAPBlockPtr responseBlock = NULL;
	int ret = OpenSOAPEnvelopeGetBodyBlockMB(response,
											 "SoapingResponse",
											 &responseBlock);

	if (OPENSOAP_FAILED(ret) || !responseBlock) {
		/* SoapingResponse not found */
	}
	else {
		ret = OpenSOAPBlockGetChildValueMB(responseBlock,
										   "ReplyTimeSec",
										   "int",
										   &(replyTime->tv_sec));
		if (OPENSOAP_FAILED(ret)) {
			/* get replyTime error */
		}
		else {
			ret = OpenSOAPBlockGetChildValueMB(responseBlock,
											   "ReplyTimeUSec",
											   "int",
											   &(replyTime->tv_usec));
			if (OPENSOAP_FAILED(ret)) {
				/* get replyTime error */
			}
			else {

				OpenSOAPStringPtr replyStr = NULL;
				ret = OpenSOAPStringCreate(&replyStr);
				if (OPENSOAP_FAILED(ret)) {
					/* OpenSOAPString create error */
				}
				else {
					ret = OpenSOAPBlockGetChildValueMB(responseBlock,
													   "ReplyString",
													   "string",
													   &replyStr);
					
					if (OPENSOAP_FAILED(ret)) {
						/* get replyString error */
						ret = OPENSOAP_NO_ERROR;
					}
					else {
						ret = OpenSOAPStringGetStringMBWithAllocator(replyStr,
																	 NULL,
																	 NULL,
																	 replyString);
						if (OPENSOAP_FAILED(ret)) {
							free(*replyString);
							*replyString = NULL;
						}
					}
					/* release string */
					OpenSOAPStringRelease(replyStr);
				}
			}
		}
	}
	
	return ret;
}

/*
  main function
*/
int
main(int argc,
	 char **argv) {
	static const char DEFAULT_HOSTNAME[]
		= "localhost";
	static const char DEFAULT_SERVICE_STDIO[]
		= "cgi-bin/SoapingService.cgi";
	static const char DEFAULT_SERVICE_SERVER[]
		= "cgi-bin/soapInterface.cgi";
	const char *endPointHostname = DEFAULT_HOSTNAME;

	/* option values */
	int soapUseServer = 0;
	int soapCount = 1;
	unsigned long sleepTime = 0;
	int soapingVerbose = 0;
	long timeOut = DEFAULT_TIMEOUT_MSEC;
  
	struct timeval startTime = {0, 0};
	struct timeval replyTime = {0, 0};

	OpenSOAPEnvelopePtr request = NULL;
	OpenSOAPEnvelopePtr response = NULL;

	char *sendString = NULL;
	
	/* OpenSoapPointer For Request Error check */
	int errorCode = OPENSOAP_NO_ERROR;

	/* option val */
	int cc;

	/* get options */

	while ((cc = getopt(argc, argv, "s:c:i:hvt:o")) != -1) {
		switch (cc) {
			case 's':
				sendString = SoapingClientGetStringSize(optarg);
				break;
			case 'c':
				soapCount = SoapingClientGetPingCount(optarg);
				break;
			case 'i':
				sleepTime = SoapingClientGetSleepTime(optarg);
				break;
			case 'v':
				soapingVerbose = 1;
				break;
			case 't':
				timeOut = SoapingClientGetTimeOut(optarg);
				break;
			case 'o':
				soapUseServer = 1;
				break;
			case 'h':
				SoapingClientHelp(argv[0], stdout);
				return 0;
			case '?':
				SoapingClientHelp(argv[0], stderr);
				return 1;
			default:
				break;
		}
	}

	/* set endpoint */

	if (argv[optind] != NULL) {
		if (strchr(argv[optind], '/')) {
			/* if the last argument includes '/', it is URL */
			soapingEndPoint = argv[optind];
		} else {
			/* otherwise, it is a hostname */
			endPointHostname = argv[optind];
		}
	}
	if (! soapingEndPoint) {
		/* endpoint as combination of hostname and service type */
		soapingEndPoint = malloc(sizeof(char) * ENDPOINT_MAX_LEN);
		snprintf(soapingEndPoint, ENDPOINT_MAX_LEN, "http://%s/%s", endPointHostname,
				 (soapUseServer ?
				  DEFAULT_SERVICE_SERVER :
				  DEFAULT_SERVICE_STDIO));
	}

	/* start soaping */

	/* initialize client */

	errorCode = OpenSOAPInitialize(NULL);
    if (OPENSOAP_FAILED(errorCode)) {
		fprintf(stderr,
				"OPENSOAP API Initialize Failed\n"
				"OPENSOAP Error CODE:0x%04x\n",
				errorCode);
		return 1;
    }

	printf("SOAPING %s : %d byte string.\n", soapingEndPoint,
		   (sendString ? strlen(sendString) : 0));

    /* loop soaping soapCount times */
	count = count_recv = 0;

#if defined(USE_ATEXIT)	
	/* */
	atexit(SoapingPrintResult);
#endif /* USE_ATEXIT */
	
	/* set interrupt */
	signal(SIGINT, onint);
#ifdef SIGHUP
	signal(SIGHUP, onint);
#endif /* SIGHUP */
	signal(SIGTERM, onint);
	
	/* loop begin */
	while (1) {
		OpenSOAPBlockPtr responseBlock = NULL;

		/*  SoapingCreateRequestMessage-------------------------------------*/
    

		/* get present time before soaping*/
		startTime = SoapingGetCurrentTime();
		
		/* create request message */
		errorCode = SoapingCreateRequestMessage(startTime,
												sendString,
												&request);
		if (OPENSOAP_FAILED(errorCode)) {
			return 1;
		}

		/* output request message */
		if (soapingVerbose) {
			PrintEnvelope(request, "request");
		}

		/* invoke service */
		errorCode = SoapingInvokeService(soapingEndPoint,
										 startTime,
										 timeOut,
										 request,
										 &response);

		++count;
		
		/* END SoapingClientCreateRequestMessage----------------------------*/

		if (OPENSOAP_FAILED(errorCode)) {
			fprintf(stderr, "invoke failed\n");
		} else {

			/* SoapingClientCreateResponceMessage ------------------------------*/


			if (soapingVerbose) {
				PrintEnvelope(response, "response");
			}

			/* */
			errorCode = OpenSOAPEnvelopeGetBodyBlockMB(response,
													   "Fault",
													   &responseBlock);
		
			if (OPENSOAP_SUCCEEDED(errorCode) && responseBlock) {
				/* is fault */

				/* output fault */
				SoapingOutputFault(responseBlock, NULL);
			}
			else {
				char *replyString = NULL;
				/* how to use replyString? */
				errorCode = SoapingGetResponseParam(response,
													&replyTime,
													&replyString);
				if (OPENSOAP_SUCCEEDED(errorCode)) {
					struct timeval current = SoapingGetCurrentTime();
					long timeDiffer = (current.tv_sec - replyTime.tv_sec) * SECOND
						+ (current.tv_usec - replyTime.tv_usec);

					printf("soaping-seq=%i time=%.3f msec\n", count - 1, timeDiffer / 1000.0);
					++count_recv;
					SoapingClientGetRoundTrip(timeDiffer);

					/*END OpenSOAPCreateResponseMessage--------------------------*/

					/* replyString release */
					free(replyString);
				}
			}
		}

		/* finalize client */

		OpenSOAPEnvelopeRelease(response); response = NULL;
		OpenSOAPEnvelopeRelease(request);  request  = NULL;

		if (count == soapCount) {
			break;
		}
		
		/* interval times */
		usleep(sleepTime * 1000);
	}
	
	/* reset interrupt */
	signal(SIGINT, SIG_DFL);
#ifdef SIGHUP
	signal(SIGHUP, SIG_DFL);
#endif /* SIGHUP */
	signal(SIGTERM, SIG_DFL);
	
	/* output result */
#if !defined(USE_ATEXIT)
	SoapingPrintResult();
#endif /* USE_ATEXIT */
	
	/* sendString release */
	free(sendString);

	/* */
	OpenSOAPUltimate();
      
	return 0;
}
