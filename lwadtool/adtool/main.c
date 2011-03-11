/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        This is the main class of ADTool. It populates application context
 *        and executes requested actions.
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 17, 2010
 *
 */

#include "includes.h"

/**
 * Main method.
 *
 * @param argc Number of arguments
 * @param argv Arguments
 */
int main(int argc, const char **argv) {
    DWORD dwError = 0;
    AdtActionTP action;
    HANDLE context;
    /* PCSTR errs; */
    AdtResultBaseTP result;

    dwError = AdtOpen(&context);
    ADT_BAIL_ON_ERROR(dwError);

    AdtSetOutputMode(context, AdtOutputModeStdout);

    dwError = AdtCreateActionArgV(context, argc, argv, &action);
    ADT_BAIL_ON_ERROR(dwError);

    dwError = AdtExecuteAction(action);
    ADT_BAIL_ON_ERROR(dwError);

    dwError = AdtGetExecResult(action, (AdtResultTPP) &result);
    ADT_BAIL_ON_ERROR(dwError);

    if(result->resultStr) {
        fprintf(stdout, "%s", result->resultStr);
    }

    cleanup:
      AdtDisposeAction(action);
      AdtClose(context);
      exit(dwError);

    error:
      fprintf(stderr, "\nERROR: %d: %s\n", dwError, AdtGetErrorMsg(dwError));
      /*
      AdtGetStderrStr(context, &errs);
      fprintf(stdout, "Stderr:\n%s\n", errs);
      */
      goto cleanup;
}
