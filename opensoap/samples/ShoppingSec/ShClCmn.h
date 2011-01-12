/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShClCmn.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ShoppingClientCmn_H
#define ShoppingClientCmn_H

extern const char *DEFAULT_ENDPOINT;
extern const char *DEFAULT_CHARENC;
extern const char *OPTION_PREFIX;

extern const char *OPTION_SOAPENDPOINT;
extern const char *OPTION_SOAPACTION;
extern const char *OPTION_CHARENC;

extern const char *USAGE_COMMON;

typedef  struct {
	char *appName;
	char *soapEndpoint;
	char *soapAction;
	char *charEnc;
	char *nameSpace;
	char *method;
	char *code;
	long qty;
} ClientVariables;

/* get name of this program */
char *
GetAppName(const char *argv0);

/* initialize common options */
int
InitializeOptionsCommon(ClientVariables *appVars,
						char ***argv,
						char **avEnd);

/* initialize method arguments */
int
InitializeMethodArgs(ClientVariables *appVars,
					 char ***argv,
					 char **avEnd,
					 int error);

/* initialize options postprocessing */
int
InitializeOptionsPostProc(ClientVariables *appVars,
						  const char *defaultAppName,
						  int error);

/* call method common part */
int
CallMethodCommon(ClientVariables *appVars);

#endif /* ShoppingClientCmn_H */
