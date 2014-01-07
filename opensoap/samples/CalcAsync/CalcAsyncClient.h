/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: CalcAsyncClient.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef CalcAsyncClient_H
#define CalcAsyncClient_H

#include <stdio.h>

int
CalcSync(const char *end_point,
		 const char *soap_action,
		 const char *char_enc,
		 const char *operator,
		 double operand_a,
		 double operand_b,
		  int hop_count,
		  const char **forward_path,
		  int forward_path_num,
		 double *result);

int
CalcAsync(const char *end_point,
		  const char *soap_action,
		  const char *char_enc,
		  const char *operator,
		  double operand_a,
		  double operand_b,
		  int hop_count,
		  const char **forward_path,
		  int forward_path_num,
		  char **messageID,
		  FILE *fp);

int
GetResult(FILE *fp,
		  const char *end_point,
		  const char *soap_action,
		  const char *char_enc,
		  double *result);

char**
getForwardPathListString(char *argv, int* path_num);

/* file pointer for debug output */
extern FILE *debug_fp;

#endif /* CalcAsyncClient_H */



