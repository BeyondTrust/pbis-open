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
 *        enable.c
 *
 * Abstract:
 *        Methods for enabling/disabling users and computers.
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 17, 2010
 *
 */

#include "includes.h"

/**
 * Enable/disable user account.
 * @param action Action reference.
 * @param isEnable Enable user if TRUE; disable otherwise.
 */
static DWORD ExecuteAdtEnableDisableUser(IN AdtActionTP action, IN BOOL isEnabled)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    INT i = 0;
    INT j = 0;
    PUSER_INFO_4 info = NULL;
    AttrValsT *avp = NULL;

    dwError = LocateADUser(appContext, &(action->disableUser.name));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = LwAllocateMemory(2 * sizeof(AttrValsT), OUT_PPVOID(&avp));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    avp[0].attr = "samAccountName";

    dwError = GetObjectAttrs(appContext, action->disableUser.name, avp);
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!avp[0].vals || !avp[0].vals[0]) {
        dwError = ADT_ERR_FAILED_AD_GET_ATTR;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Reading account properties of user %s ...\n",
                appContext->actionName,
                avp[0].vals[0]);

    dwError = AdtNetUserGetInfo4(appContext, avp[0].vals[0], &info);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Done reading account properties.\n",
                appContext->actionName);

    if(isEnabled) {
        info->usri4_flags &= ~UF_ACCOUNTDISABLE;
    }
    else {
        info->usri4_flags |= UF_ACCOUNTDISABLE;
    }

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Changing account properties of user %s ...\n",
                appContext->actionName,
                avp[0].vals[0]);

    dwError = AdtNetUserSetInfoFlags(appContext,
                                     avp[0].vals[0],
                                     info->usri4_flags);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Done changing account properties.\n",
                appContext->actionName);

    if(appContext->gopts.isPrintDN) {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "%s\n", action->disableUser.name);
        }
    }
    else {
        if (!appContext->gopts.isQuiet) {
            PrintResult(appContext,
                        LogLevelNone,
                        "Account of user %s has been %s\n",
                        avp[0].vals[0], isEnabled ? "enabled" : "disabled");
        }
    }

    cleanup:
        if (avp) {
            for (i = 0; avp[i].vals; ++i) {
                for (j = 0; avp[i].vals[j]; ++j) {
                    LW_SAFE_FREE_MEMORY(avp[i].vals[j]);
                }

                LW_SAFE_FREE_MEMORY(avp[i].vals);
            }

            LW_SAFE_FREE_MEMORY(avp);
        }

        LW_SAFE_FREE_MEMORY(info);

        return dwError;

    error:
        goto cleanup;
}

/**************************************************************/
/*                   Enabling user account                    */
/**************************************************************/

/**
 * Action initialization method.
 */
DWORD InitAdtEnableUserAction(IN AdtActionTP action)
{
    return InitBaseActionWithNetAPI(action);
}

/**
 * Action validate method.
 */
DWORD ValidateAdtEnableUserAction(IN AdtActionTP action)
{
    DWORD dwError = 0;

    if (!action->enableUser.name) {
        dwError = ADT_ERR_ARG_MISSING_NAME;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = ProcessDash(&(action->enableUser.name));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = OpenADSearchConnectionDomain(action, &(action->enableUser.name));
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
DWORD ExecuteAdtEnableUserAction(IN AdtActionTP action)
{
    return ExecuteAdtEnableDisableUser(action, TRUE);
}

/**
 * Action clean up method.
 */
DWORD CleanUpAdtEnableUserAction(IN AdtActionTP action)
{
    return CleanUpBaseWithNetAPIAction(action);
}

/**************************************************************/
/*                  Disabling user account                    */
/**************************************************************/

DWORD InitAdtDisableUserAction(IN AdtActionTP action)
{
    return InitBaseActionWithNetAPI(action);
}

DWORD ValidateAdtDisableUserAction(IN AdtActionTP action)
{
    DWORD dwError = 0;

    if (!action->disableUser.name) {
        dwError = ADT_ERR_ARG_MISSING_NAME;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = ProcessDash(&(action->disableUser.name));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = OpenADSearchConnectionDomain(action, &(action->disableUser.name));
    ADT_BAIL_ON_ERROR_NP(dwError);

    SwitchToSearchConnection(action);

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

DWORD ExecuteAdtDisableUserAction(IN AdtActionTP action)
{
    return ExecuteAdtEnableDisableUser(action, FALSE);
}

DWORD CleanUpAdtDisableUserAction(IN AdtActionTP action)
{
    return CleanUpBaseWithNetAPIAction(action);
}

/**************************************************************/
/*                Enabling computer account                   */
/**************************************************************/

DWORD InitAdtEnableComputerAction(IN AdtActionTP action)
{
    return 0;
}

DWORD ValidateAdtEnableComputerAction(IN AdtActionTP action)
{
    return 0;
}

DWORD ExecuteAdtEnableComputerAction(IN AdtActionTP action)
{
    return 0;
}

DWORD CleanUpAdtEnableComputerAction(IN AdtActionTP action)
{
    return 0;
}

/**************************************************************/
/*               Disabling computer account                   */
/**************************************************************/

DWORD InitAdtDisableComputerAction(IN AdtActionTP action)
{
    return 0;
}

DWORD ValidateAdtDisableComputerAction(IN AdtActionTP action)
{
    return 0;
}

DWORD ExecuteAdtDisableComputerAction(IN AdtActionTP action)
{
    return 0;
}

DWORD CleanUpAdtDisableComputerAction(IN AdtActionTP action)
{
    return 0;
}
