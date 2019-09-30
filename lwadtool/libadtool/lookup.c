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
 *        lookup.c
 *
 * Abstract:
 *        Methods for looking up object attributes.
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 17, 2010
 *
 */

#include "includes.h"

/**
 * Actions initialization methods.
 */
DWORD InitAdtLookupObjectAction(IN AdtActionTP action)
{
    return InitBaseAction(action);
}


/**
 * Actions validate methods.
 */
DWORD ValidateAdtLookupObjectAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR cell = NULL;

    dwError = OpenADSearchConnectionDN(action, &(action->lookupObject.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    SwitchToSearchConnection(action);

    if (!action->lookupObject.dn) {
        dwError = ADT_ERR_ARG_MISSING_DN;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    if (!action->lookupObject.attr) {
        action->lookupObject.isAll = 1;
    }

    dwError = ProcessDash(&(action->lookupObject.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = ResolveDN(appContext, ObjectClassAny, action->lookupObject.dn, &cell);
    ADT_BAIL_ON_ERROR_NP(dwError);
    LW_SAFE_FREE_MEMORY(action->lookupObject.dn);
    action->lookupObject.dn = cell;

    cleanup:
        return dwError;

    error:
        goto cleanup;
}


/**
 * Actions execute method.
 */
DWORD ExecuteAdtLookupObjectAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    INT i, j;
    AttrValsT *avp = NULL;

    if(appContext->gopts.isPrintDN) {
        PrintResult(appContext, LogLevelNone, "%s\n", action->lookupObject.dn);
        goto cleanup;
    }

    PrintStderr(appContext, LogLevelVerbose, "%s: Looking up object attributes ...\n",
                appContext->actionName);

    if(action->lookupObject.attr) {
        dwError = LwAllocateMemory(2 * sizeof(AttrValsT), OUT_PPVOID(&avp));
        ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

        dwError = LwStrDupOrNull((PCSTR) action->lookupObject.attr, &(avp[0].attr));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

        dwError = GetObjectAttrs(appContext, action->lookupObject.dn, avp);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }
    else {
        dwError = GetAllObjectAttrs(appContext, action->lookupObject.dn, &avp);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }


    if (!appContext->gopts.isQuiet) {
        if (action->lookupObject.attr) {
            for (j = 0; avp && avp[0].vals && avp[0].vals[j]; ++j) {
                PrintResult(appContext, LogLevelNone, "%s\n",
                            (PSTR) avp[0].vals[j]);
            }
        }
        else {
            for (i = 0; avp && avp[i].attr; ++i) {
                PrintResult(appContext, LogLevelNone, "%s: ",
                            (PSTR) avp[i].attr);

                for (j = 0; avp[i].vals && avp[i].vals[j]; ++j) {
                    PrintResult(appContext, LogLevelNone, j ? ";%s" : "%s",
                                (PSTR) avp[i].vals[j]);
                }

                PrintResult(appContext, LogLevelNone, "\n");
            }
        }
    }

    PrintStderr(appContext, LogLevelVerbose, "%s: Looking up object attributes - done\n",
                appContext->actionName);

    cleanup:
        if (avp) {
            for (i = 0; avp[i].attr; ++i) {
                LW_SAFE_FREE_MEMORY(avp[i].attr);

                if(avp[i].vals) {
                    for (j = 0; avp[i].vals[j]; ++j) {
                        LW_SAFE_FREE_MEMORY(avp[i].vals[j]);
                    }

                    LW_SAFE_FREE_MEMORY(avp[i].vals);
                }
            }

            LW_SAFE_FREE_MEMORY(avp);
        }

        return dwError;

    error:
        goto cleanup;
}


/**
 * Actions clean up methods.
 */
DWORD CleanUpAdtLookupObjectAction(IN AdtActionTP action)
{
    return CleanUpBaseAction(action);
}
