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
 *        move.c
 *
 * Abstract:
 *
 *        Methods for moving directory objects.
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
DWORD InitAdtMoveObjectAction(IN AdtActionTP action)
{
    return InitBaseAction(action);
}

/**
 * Actions validate methods.
 */
DWORD ValidateAdtMoveObjectAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR dn = NULL;

    dwError = OpenADSearchConnectionDN(action, &(action->moveObject.from));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = OpenADSearchConnectionDN(action, &(action->moveObject.to));
    ADT_BAIL_ON_ERROR_NP(dwError);

    SwitchToSearchConnection(action);

    if (!action->moveObject.from) {
        dwError = ADT_ERR_ARG_MISSING_FROM;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = ProcessDash(&(action->moveObject.from));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = ResolveDN(appContext, ObjectClassAny, action->moveObject.from, &dn);
    if(dwError && IsMultiForestMode(action)) {
        SwitchConnection(action);
        dwError = ResolveDN(appContext, ObjectClassAny, action->moveObject.from, &dn);
    }
    ADT_BAIL_ON_ERROR_NP(dwError);
    LW_SAFE_FREE_MEMORY(action->moveObject.from);
    action->moveObject.from = dn;

    if (!action->moveObject.to) {
        dwError = ADT_ERR_ARG_MISSING_FROM;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = ProcessDash(&(action->moveObject.to));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = GetRDN(action->moveObject.to, &(appContext->oName));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = GetParentDN(action->moveObject.to, &dn);
    ADT_BAIL_ON_ERROR_NP(dwError);
    LW_SAFE_FREE_MEMORY(action->moveObject.to);
    action->moveObject.to = dn;

    dwError = ResolveDN(appContext, ObjectClassAny, action->moveObject.to, &dn);
    if(dwError && IsMultiForestMode(action)) {
        SwitchConnection(action);
        dwError = ResolveDN(appContext, ObjectClassAny, action->moveObject.to, &dn);
    }
    ADT_BAIL_ON_ERROR_NP(dwError);
    LW_SAFE_FREE_MEMORY(action->moveObject.to);
    action->moveObject.to = dn;

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Actions execute method.
 */
DWORD ExecuteAdtMoveObjectAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    PrintStderr(appContext, LogLevelVerbose, "%s: Moving AD object %s to %s,%s ...\n",
                appContext->actionName, action->moveObject.from,
                appContext->oName, action->moveObject.to);

    dwError = MoveADObject(appContext, action->moveObject.from, appContext->oName, action->moveObject.to);
    if(dwError && IsMultiForestMode(action)) {
        SwitchConnection(action);
        dwError = MoveADObject(appContext, action->moveObject.from, appContext->oName, action->moveObject.to);
    }
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelVerbose, "%s: Done moving AD object\n",
                appContext->actionName);

    if(appContext->gopts.isPrintDN) {
        PrintResult(appContext, LogLevelNone, "%s,%s\n", appContext->oName, action->moveObject.to);
        goto cleanup;
    }

    if(!appContext->gopts.isPrintDN) {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "New DN: %s,%s\n", appContext->oName, action->moveObject.to);
        }
    }

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Actions clean up methods.
 */
DWORD CleanUpAdtMoveObjectAction(IN AdtActionTP action)
{
    return CleanUpBaseAction(action);
}
