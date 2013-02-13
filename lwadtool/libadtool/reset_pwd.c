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
 *        pwd_reset.c
 *
 * Abstract:
 *
 *        Methods for resetting AD passwords.
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 17, 2010
 *
 */

#include "includes.h"

/**************************************************************/
/*                  Resetting user's password                 */
/**************************************************************/

/**
 * Action initialization method.
 */
DWORD InitAdtResetUserPasswordAction(IN AdtActionTP action)
{
    return InitBaseActionWithNetAPI(action);
}

/**
 * Action validate method.
 */
DWORD ValidateAdtResetUserPasswordAction(IN AdtActionTP action)
{
    DWORD dwError = 0;

    if(action->newUser.isNoCanChangePasswd) {
        action->newUser.isNoMustChangePasswd = 1;
    }

    if(action->newUser.isNoPasswdExpires) {
        action->newUser.isNoMustChangePasswd = 1;
    }

    if (!action->resetUserPassword.name) {
        dwError = ADT_ERR_ARG_MISSING_PASSWD;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = ProcessDash(&(action->resetUserPassword.name));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = OpenADSearchConnectionDomain(action, &(action->resetUserPassword.name));
    ADT_BAIL_ON_ERROR_NP(dwError);

    SwitchToSearchConnection(action);

    /*
    if (!action->resetUserPassword.password) {
        dwError = ADT_ERR_ARG_MISSING_PASSWD;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }
    */

    if (action->resetUserPassword.password) {
        dwError = ProcessADUserPassword(&(action->resetUserPassword.name));
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Action execute method.
 */
DWORD ExecuteAdtResetUserPasswordAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    INT i = 0;
    INT j = 0;
    PUSER_INFO_4 info = NULL;
    AttrValsT *avp = NULL;
    AttrValsT *avpTime = NULL;

    dwError = LocateADUser(appContext, &(action->resetUserPassword.name));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = LwAllocateMemory(2 * sizeof(AttrValsT), OUT_PPVOID(&avp));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    avp[0].attr = "samAccountName";

    dwError = GetObjectAttrs(appContext, action->resetUserPassword.name, avp);
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!avp[0].vals || !avp[0].vals[0]) {
        dwError = ADT_ERR_FAILED_AD_GET_ATTR;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Reading password properties of user %s ...\n",
                appContext->actionName,
                avp[0].vals[0]);

    dwError = AdtNetUserGetInfo4(appContext, avp[0].vals[0], &info);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Done reading password properties.\n",
                appContext->actionName);

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Changing password properties of user %s ...\n",
                appContext->actionName,
                avp[0].vals[0]);

    if (action->resetUserPassword.password) {
        dwError = AdtNetUserSetPassword(appContext,
                                        avp[0].vals[0],
                                        action->resetUserPassword.password);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = LwAllocateMemory(2 * sizeof(AttrValsT), OUT_PPVOID(&avpTime));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    dwError = LwAllocateMemory(2 * sizeof(PSTR), OUT_PPVOID(&(avpTime[0].vals)));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    avpTime[0].attr = "pwdLastSet";

    if(action->resetUserPassword.isNoMustChangePasswd) {
        avpTime[0].vals[0] = "-1";
    }
    else {
        avpTime[0].vals[0] = "0";
    }

    dwError = ModifyADObject(appContext, action->resetUserPassword.name, avpTime, 2);
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(action->resetUserPassword.isNoCanChangePasswd) {
        info->usri4_flags |= UF_PASSWD_CANT_CHANGE;
    }

    if (action->resetUserPassword.isNoPasswdExpires) {
        info->usri4_flags |= UF_DONT_EXPIRE_PASSWD;
    }
    else {
        info->usri4_flags &= ~UF_DONT_EXPIRE_PASSWD;
    }

    info->usri4_flags &= ~UF_PASSWD_NOTREQD;

    dwError = AdtNetUserSetInfoFlags(appContext,
                                     avp[0].vals[0],
                                     info->usri4_flags);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Done changing password properties.\n",
                appContext->actionName);

    if(appContext->gopts.isPrintDN) {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "%s\n", action->resetUserPassword.name);
        }
    }
    else {
        if (!appContext->gopts.isQuiet) {
            PrintResult(appContext,
                        LogLevelNone,
                        "Password properties have been changed for user %s\n",
                        avp[0].vals[0]);
        }
    }

    cleanup:
        if (avpTime) {
                for (i = 0; avpTime[i].vals; ++i) {
                    LW_SAFE_FREE_MEMORY(avpTime[i].vals);
                }

                LW_SAFE_FREE_MEMORY(avpTime);
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

        LW_SAFE_FREE_MEMORY(info);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Action clean up method.
 */
DWORD CleanUpAdtResetUserPasswordAction(IN AdtActionTP action)
{
    return CleanUpBaseWithNetAPIAction(action);
}

/**************************************************************/
/*               Resetting computer's password                */
/**************************************************************/

DWORD InitAdtResetComputerPasswordAction(IN AdtActionTP action)
{
    return 0;
}

DWORD ValidateAdtResetComputerPasswordAction(IN AdtActionTP action)
{
    return 0;
}

DWORD ExecuteAdtResetComputerPasswordAction(IN AdtActionTP action)
{
    return 0;
}

DWORD CleanUpAdtResetComputerPasswordAction(IN AdtActionTP action)
{
    return 0;
}
