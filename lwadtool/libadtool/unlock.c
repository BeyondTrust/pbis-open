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
 *        unlock.c
 *
 * Abstract:
 *        Methods for unlocking user and computer accounts in AD.
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 17, 2010
 *
 */

#include "includes.h"

/**
 * Action initialization method.
 */
DWORD InitAdtUnlockAccountAction(IN AdtActionTP action)
{
    return InitBaseAction(action);
}

/**
 * Action validate method.
 */
DWORD ValidateAdtUnlockAccountAction(IN AdtActionTP action)
{
    DWORD dwError = 0;

    if (!action->unlockAccount.user && !action->unlockAccount.computer) {
        dwError = ADT_ERR_INVALID_ARG_USER_COMPUTER;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    if (action->unlockAccount.user && action->unlockAccount.computer) {
        dwError = ADT_ERR_INVALID_ARG_USER_COMPUTER;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    if(action->unlockAccount.user) {
        dwError = ProcessDash(&(action->unlockAccount.user));
        ADT_BAIL_ON_ERROR_NP(dwError);

        dwError = OpenADSearchConnectionDomain(action, &(action->unlockAccount.user));
    }
    else {
        dwError = ProcessDash(&(action->unlockAccount.computer));
        ADT_BAIL_ON_ERROR_NP(dwError);

        dwError = OpenADSearchConnectionDomain(action, &(action->unlockAccount.computer));
    }
    ADT_BAIL_ON_ERROR_NP(dwError);

    SwitchToSearchConnection(action);

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Action execute method.
 */
DWORD ExecuteAdtUnlockAccountAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    INT i = 0;
    INT j = 0;
    AttrValsT *avp = NULL;
    AttrValsT *avpMod = NULL;

    if(action->unlockAccount.user) {
        dwError = LocateADUser(appContext, &(action->unlockAccount.user));
    }
    else {
        dwError = LocateADComputer(appContext, &(action->unlockAccount.computer));
    }
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = LwAllocateMemory(2 * sizeof(AttrValsT), OUT_PPVOID(&avp));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    avp[0].attr = "samAccountName";

    dwError = GetObjectAttrs(appContext,
                             action->unlockAccount.user ?
                             action->unlockAccount.user :
                             action->unlockAccount.computer,
                             avp);
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!avp[0].vals || !avp[0].vals[0]) {
        dwError = ADT_ERR_FAILED_AD_GET_ATTR;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Modifying account properties of %s ...\n",
                appContext->actionName,
                avp[0].vals[0]);

    dwError = LwAllocateMemory(2 * sizeof(AttrValsT), OUT_PPVOID(&avpMod));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    dwError = LwAllocateMemory(2 * sizeof(PSTR), OUT_PPVOID(&(avpMod[0].vals)));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    avpMod[0].attr = "lockoutTime";
    avpMod[0].vals[0] = "0";

    dwError = ModifyADObject(appContext,
                             action->unlockAccount.user ?
                             action->unlockAccount.user :
                             action->unlockAccount.computer,
                             avpMod,
                             2);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Done modifying account properties.\n",
                appContext->actionName);

    if(appContext->gopts.isPrintDN) {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "%s\n",
                        action->unlockAccount.user ?
                        action->unlockAccount.user :
                        action->unlockAccount.computer
                        );
        }
    }
    else {
        if (!appContext->gopts.isQuiet) {
            PrintResult(appContext,
                        LogLevelNone,
                        "Account %s has been unlocked\n",
                        avp[0].vals[0]);
        }
    }

    cleanup:
        if (avpMod) {
                for (i = 0; avpMod[i].vals; ++i) {
                    LW_SAFE_FREE_MEMORY(avpMod[i].vals);
                }

                LW_SAFE_FREE_MEMORY(avpMod);
        }

        if (avp) {
            for (i = 0; avp[i].vals; ++i) {
                for (j = 0; avp[i].vals[j]; ++j) {
                    LW_SAFE_FREE_MEMORY(avp[i].vals[j]);
                }

                LW_SAFE_FREE_MEMORY(avp[i].vals);
            }

            LW_SAFE_FREE_MEMORY(avp);
        }

        return dwError;

    error:
        goto cleanup;
}

/**
 * Action clean up method.
 */
DWORD CleanUpAdtUnlockAccountAction(IN AdtActionTP action)
{
    return CleanUpBaseAction(action);
}

