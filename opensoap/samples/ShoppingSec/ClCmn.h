/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ClCmn.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ClCmn_H
#define ClCmn_H

#include <OpenSOAP/Envelope.h>

#include <stdio.h>

#define BUFFER_SIZE 1024

#define APPLICATION_ERROR (0x10000001L)

#define LOG_STREAM stderr

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
		"%s function error: Cannot %s\n" \
		" ---> OpenSOAP Error Code: 0x%04x\n", \
		(location),\
		(message),\
		(error));

/* error check macro */
#define ERROR_CHECK(error, location, message); \
if (OPENSOAP_FAILED(error)) { \
	ERROR_MSG(error, location, message) \
}

/* error return macro */
#define ERROR_RETURN(error, location, message); \
if (OPENSOAP_FAILED(error)) { \
	ERROR_MSG(error, location, message) \
	return (error); \
}

int
ReadMessage(FILE *fp,
			const char *charEnc,
			OpenSOAPEnvelopePtr *env);

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
InvokeService(const char *endpoint,
			  const char *soapAction,
			  const char *charEnc,
			  OpenSOAPEnvelopePtr *request,
			  OpenSOAPEnvelopePtr *response);

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
						  const char *NsUri,
						  const char *NsPrefix);

int
CreateRequestCommon(OpenSOAPEnvelopePtr *request,
					const char *requestName,
					OpenSOAPBlockPtr *body,
					const char *nameSpace);

int
GetStringParameter(OpenSOAPBlockPtr body,
				   const char *paramName,
				   char **paramValue);

int
ParseFaultMessage(OpenSOAPBlockPtr body);

int
ParseResponseCommon(OpenSOAPEnvelopePtr response,
					const char *responseName,
					OpenSOAPBlockPtr *body,
					const char *method_ns_uri,
					int *isFault);

#endif /* ClCmn_H */
