/*-----------------------------------------------------------------------------
 * $RCSfile: SvCmn.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SvCmn_H
#define SvCmn_H

#include <OpenSOAP/Envelope.h>

#include <stdio.h>

#define BUFFER_SIZE 1024
#define NAME_SZ 256

#define APPLICATION_ERROR (0x10000001L)

FILE *fpLog;
# define LOG_STREAM fpLog

#if defined(CONNECT_TYPE_CGI)
# define CONNECT_TYPE "cgi"
#else
# define CONNECT_TYPE "stdio"
#endif /* CONNECT_TYPE_CGI */

/* log message macro */
#define LOG_MSG(location, message); \
fprintf(LOG_STREAM, \
		"%s: %s\n", \
		(location),\
		(message)); \
fflush(LOG_STREAM);

/* log message macro */
#define LOG_STRING(location, name, value); \
fprintf(LOG_STREAM, \
		"%s: %s=\"%s\"\n", \
		(location),\
		(name),\
		(value)); \
fflush(LOG_STREAM);

/* log message macro */
#define LOG_INT(location, name, value); \
fprintf(LOG_STREAM, \
		"%s: %s=%d\n", \
		(location),\
		(name),\
		(value)); \
fflush(LOG_STREAM);

/* log envelope macro */
#define LOG_ENVELOPE(location, label, env); \
WriteLabeledMessage((env), \
					NULL, \
					(label), \
					LOG_STREAM);

/* error message macro */
#define ERROR_MSG(error, location, message); \
fprintf(LOG_STREAM, \
		"%s: Cannot %s\n" \
		" ---> OpenSOAP Error Code: 0x%04x\n", \
		(location),\
		(message),\
		(error)); \
fflush(LOG_STREAM);

/* error check macro */
#define ERROR_CHECK(error, location, message); \
if (OPENSOAP_FAILED((error))) { \
	ERROR_MSG(error, location, message) \
}

/* error return macro */
#define ERROR_RETURN(error, location, message); \
if (OPENSOAP_FAILED((error))) { \
	ERROR_MSG(error, location, message) \
	return (error); \
}

typedef struct {
	char nameSpace[NAME_SZ];
    char opt1[NAME_SZ];
    char opt2[NAME_SZ];
} Options;

int
WriteMessage(OpenSOAPEnvelopePtr env,
			 const char *charEnc,
			 FILE *fp);

int
WriteLabeledMessage(OpenSOAPEnvelopePtr env,
					const char *charEnc,
					const char *label,
					FILE *fp);

int
AddAttribute(OpenSOAPBlockPtr block,
			 const char *elmName,
			 const char *attrName,
			 const char *attrType,
			 void *attrValue,
			 OpenSOAPXMLAttrPtr *attr);

int
AddAttributeWithNamespace(OpenSOAPBlockPtr block,
						  const char *elmName,
						  const char *attrName,
						  const char *attrType,
						  void *attrValue,
						  const char *nsUri,
						  const char *nsPrefix);
int
GetStringParameter(OpenSOAPBlockPtr body,
				   const char *paramName,
				   char **paramValue);

int
ParseRequestCommon(OpenSOAPEnvelopePtr request,
				   const char *requestName,
				   OpenSOAPBlockPtr *body,
				   const char *nameSpace);

int
CreateResponseCommon(OpenSOAPEnvelopePtr *response,
					 const char *responseName,
					 OpenSOAPBlockPtr *body,
					 const char *nameSpace);

int
CreateFaultMessage(OpenSOAPEnvelopePtr *response,
				   const char *nameSpace,
				   const char *faultcode,
				   const char *faultstring,
				   const char *faultactor,
				   const char *detail);

#endif /* SvCmn_H */
