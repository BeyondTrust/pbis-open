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
            PrintResult(appContext, LogLevelNone, "New DN: %s,%s", appContext->oName, action->moveObject.to);
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
