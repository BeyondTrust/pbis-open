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
        dwError = LwAllocateMemory(2 * sizeof(AttrValsT), (PVOID) &avp);
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
