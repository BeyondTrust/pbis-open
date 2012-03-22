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
 *        delete.c
 *
 * Abstract:
 *        Methods for delete AD objects.
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
DWORD InitAdtDeleteObjectAction(IN AdtActionTP action)
{
    return InitBaseAction(action);
}

/**
 * Actions validate methods.
 */
DWORD ValidateAdtDeleteObjectAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR dn = NULL;

    dwError = OpenADSearchConnectionDN(action, &(action->deleteObject.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    SwitchToSearchConnection(action);

    if (!action->deleteObject.dn) {
        dwError = ADT_ERR_ARG_MISSING_DN;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = ProcessDash(&(action->deleteObject.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = ResolveDN(appContext, ObjectClassAny, action->deleteObject.dn, &dn);
    ADT_BAIL_ON_ERROR_NP(dwError);
    LW_SAFE_FREE_MEMORY(action->deleteObject.dn);
    action->deleteObject.dn = dn;

    cleanup:
        return dwError;

    error:
        LW_SAFE_FREE_MEMORY(dn);
        goto cleanup;
}

/**
 * Actions execute method.
 */
DWORD ExecuteAdtDeleteObjectAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    PrintStderr(appContext, LogLevelVerbose, "%s: Calling delete object operation ...\n",
                appContext->actionName);

    dwError = DeleteADObject(appContext, action->deleteObject.dn, action->deleteObject.isDeleteMembers);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelVerbose, "%s: Calling delete object operation - done\n",
                appContext->actionName);

    if(appContext->gopts.isPrintDN) {
        PrintResult(appContext, LogLevelNone, "%s\n", action->deleteObject.dn);
    }
    else {
        PrintResult(appContext, LogLevelNone, "Object %s has been deleted.\n", action->deleteObject.dn);
    }

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Actions clean up methods.
 */
DWORD CleanUpAdtDeleteObjectAction(IN AdtActionTP action)
{
    return CleanUpBaseAction(action);
}
