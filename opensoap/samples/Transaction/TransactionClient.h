/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: TransactionClient.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef CalcClient_H
#define CalcClient_H

#include <stdio.h>

typedef  struct {
    char *result;
    char *paymentResult;
    char *paymentComment;
    char *paymentTransactionID;
    char *paymentActionResponse;
    char *depositResult;
    char *depositComment;
    char *depositTransactionID;
    char *depositActionResponse;
} ResultVariables;

int
Transfer(const char *endpoint,
	 const char *soapAction,
	 const char *charEnc,
	 const char *endpoint1,
	 const char *endpoint2,
	 const char *account1,
	 const char *account2,
	 const char *money,
	 ResultVariables *rsltVars);


int
TransferAsync(const char *endpoint,
	      const char *soapAction,
	      const char *charEnc,
	      const char *endpoint1,
	      const char *endpoint2,
	      const char *account1,
	      const char *account2,
	      const char *money,
	      char **messageID,
	      FILE *fp);

int
GetResult(FILE *fp,
	  const char *endpoint,
	  const char *soapAction,
	  const char *charEnc,
	  ResultVariables *rsltVars);

#endif /* CalcClient_H */
