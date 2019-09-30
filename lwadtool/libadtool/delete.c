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
