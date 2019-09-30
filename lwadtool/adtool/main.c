/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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
    AdtActionTP action = NULL;
    HANDLE context;
    /* PCSTR errs; */
    AdtResultBaseTP result;

    setlocale(LC_ALL, "");

    dwError = AdtOpen(&context);
    ADT_BAIL_ON_ERROR(dwError);

    AdtSetOutputMode(context, AdtOutputModeStdout);

    dwError = AdtCreateActionArgV(context, argc, argv, &action);
    ADT_BAIL_ON_ERROR(dwError);

    dwError = AdtExecuteAction(action);
    ADT_BAIL_ON_ERROR(dwError);

    dwError = AdtGetExecResult(action, (AdtResultTPP) (PVOID) &result);
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
