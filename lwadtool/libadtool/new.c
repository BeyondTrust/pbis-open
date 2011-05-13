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
 *        new.c
 *
 * Abstract:
 *
 *        Methods for creating new directory objects.
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 17, 2010
 *
 */

#include "includes.h"

/***************************************************************************/
/*                          New OU action                                  */
/***************************************************************************/

DWORD InitAdtNewOuAction(IN AdtActionTP action)
{
    return InitBaseAction(action);
}

DWORD ValidateAdtNewOuAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR dn = NULL;
    PSTR name = NULL;

    dwError = OpenADSearchConnectionDN(action, &(action->newOu.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    SwitchToSearchConnection(action);

    if (!action->newOu.dn) {
        dwError = ADT_ERR_ARG_MISSING_DN;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = ProcessDash(&(action->newOu.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!IsNVP(action->newOu.dn)) {
        dwError = LwAllocateStringPrintf(&dn, "OU=%s", action->newOu.dn);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        LW_SAFE_FREE_MEMORY(action->newOu.dn);
        action->newOu.dn = dn;
    }

    if(action->newOu.name) {
        dwError = ResolveDN(appContext, ObjectClassAny, action->newOu.dn, &dn);
        ADT_BAIL_ON_ERROR_NP(dwError);
        LW_SAFE_FREE_MEMORY(action->newOu.dn);
        action->newOu.dn = dn;

        dwError = ProcessDash(&(action->newOu.name));
        ADT_BAIL_ON_ERROR_NP(dwError);

        if(!IsNVP(action->newOu.name)) {
            dwError = LwAllocateStringPrintf(&dn, "OU=%s", action->newOu.name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
            LW_SAFE_FREE_MEMORY(action->newOu.name);
            action->newOu.name = dn;
        }
    }
    else {
        dwError = GetRDN(action->newOu.dn, &(action->newOu.name));
        ADT_BAIL_ON_ERROR_NP(dwError);

        dwError = GetParentDN(action->newOu.dn, &dn);
        ADT_BAIL_ON_ERROR_NP(dwError);
        LW_SAFE_FREE_MEMORY(action->newOu.dn);
        action->newOu.dn = dn;

        dwError = ResolveDN(appContext, ObjectClassAny, action->newOu.dn, &dn);
        ADT_BAIL_ON_ERROR_NP(dwError);
        LW_SAFE_FREE_MEMORY(action->newOu.dn);
        action->newOu.dn = dn;
    }

    dwError = LwAllocateStringPrintf(&dn, "%s,%s", action->newOu.name, action->newOu.dn);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    LW_SAFE_FREE_MEMORY(action->newOu.dn);
    action->newOu.dn = dn;

    if(!action->newOu.desc) {
        dwError = LwStrDupOrNull("lw-adtool created", &(action->newOu.desc));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    dn = NULL;
    name = NULL;

    cleanup:
        return dwError;

    error:
        LW_SAFE_FREE_MEMORY(dn);
        LW_SAFE_FREE_MEMORY(name);
        goto cleanup;
}

DWORD ExecuteAdtNewOuAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    PSTR desc[] = {
        action->newOu.desc,
        NULL
    };

    PSTR name[] = {
        action->newOu.name,
        NULL
    };

    AttrValsT avp[] = {
        { "objectClass", GetObjectClassVals(ObjectClassOU) },
        { "name", name },
        { "description", desc },
        { NULL, NULL }
    };

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating OU %s ...\n",
                appContext->actionName, action->newOu.dn);

    dwError = CreateADObject(appContext, action->newOu.dn, avp);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating OU - done \n",
                appContext->actionName);

    if(appContext->gopts.isPrintDN) {
        PrintResult(appContext, LogLevelNone, "%s\n", action->newOu.dn);
    }
    else {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "Organizational unit %s has been created.\n", action->newOu.dn);
        }
    }

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

DWORD CleanUpAdtNewOuAction(IN AdtActionTP action)
{
    return CleanUpBaseAction(action);
}

/***************************************************************************/
/*                          New user action                                */
/***************************************************************************/

/**
 * Actions initialization methods.
 */
DWORD InitAdtNewUserAction(IN AdtActionTP action)
{
    return InitBaseActionWithNetAPI(action);
}

/**
 * Actions validate methods.
 */
DWORD ValidateAdtNewUserAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR dn = NULL;
    PSTR name = NULL;
    PSTR tmp = NULL;

    dwError = OpenADSearchConnectionDN(action, &(action->newUser.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    SwitchToSearchConnection(action);

    if (!action->newUser.dn) {
        dwError = LwAllocateStringPrintf(&(action->newUser.dn), "CN=Users");
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if(action->newUser.isNoCanChangePasswd) {
        action->newUser.isNoMustChangePasswd = 1;
    }

    if(action->newUser.isNoPasswdExpires) {
        action->newUser.isNoMustChangePasswd = 1;
    }

    dwError = ProcessDash(&(action->newUser.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!IsNVP(action->newUser.dn)) {
        dwError = LwAllocateStringPrintf(&dn, "OU=%s", action->newUser.dn);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

        dwError = ResolveDN(appContext, ObjectClassAny, dn, &tmp);

        if(dwError) {
            LW_SAFE_FREE_MEMORY(dn);
            dwError = LwAllocateStringPrintf(&dn, "CN=%s", action->newUser.dn);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        LW_SAFE_FREE_MEMORY(action->newUser.dn);
        action->newUser.dn = dn;
    }

    dwError = ResolveDN(appContext, ObjectClassAny, action->newUser.dn, &dn);
    ADT_BAIL_ON_ERROR_NP(dwError);
    LW_SAFE_FREE_MEMORY(action->newUser.dn);
    action->newUser.dn = dn;

    dwError = ProcessDash(&(action->newUser.cn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = ProcessDash(&(action->newUser.name));
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!action->newUser.name) {
        dwError = ADT_ERR_ARG_MISSING_NAME;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    if(!action->newGroup.namePreWin2000) {
        dwError = LwStrDupOrNull(action->newUser.name, &(action->newUser.namePreWin2000));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if(!action->newUser.cn) {
        if(!action->newUser.nameFirst && !action->newUser.nameLast) {
            dwError = ADT_ERR_ARG_MISSING_CN;
            ADT_BAIL_ON_ERROR_NP(dwError);
        }

        if(!action->newUser.nameFirst) {
            dwError = LwStrDupOrNull(action->newUser.nameLast, &(action->newUser.cn));
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }
        else {
            if(!action->newUser.nameLast) {
                dwError = LwStrDupOrNull(action->newUser.nameFirst, &(action->newUser.cn));
                ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
            }
            else {
                dwError = LwAllocateStringPrintf(&(action->newUser.cn), "%s %s", action->newUser.nameFirst,
                                                 action->newUser.nameLast);
                ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
            }
        }
    }

    dwError = LwAllocateStringPrintf(&dn, "CN=%s,%s", action->newUser.cn,
                                     action->newUser.dn);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    LW_SAFE_FREE_MEMORY(action->newUser.dn);
    action->newUser.dn = dn;

    if(!action->newUser.desc) {
        dwError = LwStrDupOrNull("lw-adtool created", &(action->newUser.desc));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    dwError = ProcessADUserPassword(&(action->newUser.password));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dn = NULL;
    name = NULL;

    cleanup:
        return dwError;

    error:
        LW_SAFE_FREE_MEMORY(tmp);
        LW_SAFE_FREE_MEMORY(dn);
        LW_SAFE_FREE_MEMORY(name);

        goto cleanup;
}

/**
 * Actions execute method.
 */
DWORD ExecuteAdtNewUserAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR principal = NULL;
    PUSER_INFO_4 info = NULL;
    BOOL isSet = FALSE;
    AttrValsT *avpTime = NULL;
    INT i = 0;

    dwError = LwAllocateStringPrintf(&principal,
                                     "%s@%s",
                                     action->newUser.name,
                                     appContext->workConn->domainName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    PSTR cn[] = {
        action->newUser.cn,
        NULL
    };

    PSTR name[] = {
        action->newUser.cn,
        NULL
    };

    PSTR nameFirst[] = {
        action->newUser.nameFirst ? action->newUser.nameFirst : action->newUser.cn,
        NULL
    };

    PSTR nameLast[] = {
        action->newUser.nameLast ? action->newUser.nameLast : action->newUser.cn,
        NULL
    };

    PSTR samAccountName[] = {
        action->newUser.namePreWin2000,
        NULL
    };

    PSTR principalName[] = {
        principal,
        NULL
    };

    PSTR desc[] = {
        action->newUser.desc,
        NULL
    };

    AttrValsT avp[] = {
        { "objectClass", GetObjectClassVals(ObjectClassUser) },
        { "cn", cn },
        { "name", name },
        { "givenName", nameFirst },
        { "sn", nameLast },
        { "sAMAccountName", samAccountName },
        { "userPrincipalName", principalName },
        { "description", desc },
        { NULL, NULL }
    };

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating user %s ...\n",
                appContext->actionName, action->newUser.dn);

    dwError = CreateADObject(appContext, action->newUser.dn, avp);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating user - done \n",
                appContext->actionName);

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Reading account properties of user %s ...\n",
                appContext->actionName,
                action->newUser.namePreWin2000);

    if(!appContext->gopts.isReadOnly) {
        dwError = AdtNetUserGetInfo4(appContext, action->newUser.namePreWin2000, &info);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Done reading account properties.\n",
                appContext->actionName);

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Setting account properties of user %s ...\n",
                appContext->actionName,
                action->newUser.namePreWin2000);

    if(action->newUser.isNoCanChangePasswd) {
        info->usri4_flags |= UF_PASSWD_CANT_CHANGE;
        isSet = TRUE;
    }

    if(action->newUser.isAccountEnabled) {
        info->usri4_flags &= ~UF_ACCOUNTDISABLE;
        isSet = TRUE;
    }

    if(action->newUser.isNoPasswdExpires) {
        info->usri4_flags |= UF_DONT_EXPIRE_PASSWD;
        isSet = TRUE;
    }
    else {
        info->usri4_flags &= ~UF_DONT_EXPIRE_PASSWD;
    }

    if(action->newUser.password) {
        info->usri4_flags &= ~UF_PASSWD_NOTREQD;
        isSet = TRUE;

        dwError = AdtNetUserSetPassword(appContext,
                                        action->newUser.namePreWin2000,
                                        action->newUser.password);
        ADT_BAIL_ON_ERROR_NP(dwError);

        if(action->newUser.isNoMustChangePasswd) {
            dwError = LwAllocateMemory(2 * sizeof(AttrValsT), (PVOID) &avpTime);
            ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

            dwError = LwAllocateMemory(2 * sizeof(PSTR), (PVOID) &(avpTime[0].vals));
            ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

            avpTime[0].attr = "pwdLastSet";
            avpTime[0].vals[0] = "-1";

            dwError = ModifyADObject(appContext, action->newUser.dn, avpTime, 2);
            //fprintf(stderr, "Setting time to %s\n", avpTime[0].vals[0]);
            ADT_BAIL_ON_ERROR_NP(dwError);
        }

    }

    if(isSet) {
        dwError = AdtNetUserSetInfoFlags(appContext,
                                         action->newUser.namePreWin2000,
                                         info->usri4_flags);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Account properties of user %s successfully set.\n",
                appContext->actionName,
                action->newUser.name);

    if(appContext->gopts.isPrintDN) {
        PrintResult(appContext, LogLevelNone, "%s\n", action->newUser.dn);
    }
    else {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext,
                        LogLevelNone,
                        "%s: Account has been created for user %s.\n",
                        appContext->actionName, action->newUser.name);
        }
    }

    cleanup:
        if (avpTime) {
            for (i = 0; avpTime[i].vals; ++i) {
                LW_SAFE_FREE_MEMORY(avpTime[i].vals);
            }

            LW_SAFE_FREE_MEMORY(avpTime);
        }

        LW_SAFE_FREE_MEMORY(principal);
        LW_SAFE_FREE_MEMORY(info);
        return dwError;

    error:
        goto cleanup;
}

/**
 * Actions clean up methods.
 */
DWORD CleanUpAdtNewUserAction(IN AdtActionTP action)
{
    return CleanUpBaseWithNetAPIAction(action);
}

/***************************************************************************/
/*                          New group action                               */
/***************************************************************************/

DWORD InitAdtNewGroupAction(IN AdtActionTP action)
{
    return InitBaseAction(action);
}

DWORD ValidateAdtNewGroupAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR dn = NULL;
    PSTR name = NULL;
    PSTR tmp = NULL;

    if (action->newGroup.scope) {
        if(
           !IsEqual(action->newGroup.scope, GROUP_TYPE_NAME_DOMAIN_LOCAL, 1) &&
           !IsEqual(action->newGroup.scope, GROUP_TYPE_NAME_GLOBAL, 1) &&
           !IsEqual(action->newGroup.scope, GROUP_TYPE_NAME_UNIVERSAL, 1)
        ) {
            dwError = ADT_ERR_INVALID_GROUP_SCOPE;
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
    }
    else {
        dwError = LwAllocateStringPrintf(&(action->newGroup.scope), GROUP_TYPE_NAME_GLOBAL);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    dwError = OpenADSearchConnectionDN(action, &(action->newGroup.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    SwitchToSearchConnection(action);

    if (!action->newGroup.dn) {
        dwError = LwAllocateStringPrintf(&(action->newGroup.dn), "CN=Users");
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    dwError = ProcessDash(&(action->newGroup.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!IsNVP(action->newGroup.dn)) {
        dwError = LwAllocateStringPrintf(&dn, "OU=%s", action->newGroup.dn);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

        dwError = ResolveDN(appContext, ObjectClassAny, dn, &tmp);

        if(dwError) {
            LW_SAFE_FREE_MEMORY(dn);
            dwError = LwAllocateStringPrintf(&dn, "CN=%s", action->newGroup.dn);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        LW_SAFE_FREE_MEMORY(action->newGroup.dn);
        action->newGroup.dn = dn;
    }

    dwError = ResolveDN(appContext, ObjectClassAny, action->newGroup.dn, &dn);
    ADT_BAIL_ON_ERROR_NP(dwError);
    LW_SAFE_FREE_MEMORY(action->newGroup.dn);
    action->newGroup.dn = dn;

    dwError = ProcessDash(&(action->newGroup.name));
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!action->newGroup.name) {
        dwError = ADT_ERR_ARG_MISSING_NAME;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    if(!action->newGroup.namePreWin2000) {
        dwError = LwStrDupOrNull(action->newGroup.name, &(action->newGroup.namePreWin2000));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    dwError = LwAllocateStringPrintf(&dn, "CN=%s,%s", action->newGroup.name, action->newGroup.dn);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    LW_SAFE_FREE_MEMORY(action->newGroup.dn);
    action->newGroup.dn = dn;

    if(!action->newGroup.desc) {
        dwError = LwStrDupOrNull("lw-adtool created", &(action->newGroup.desc));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    dn = NULL;
    name = NULL;

    cleanup:
        return dwError;

    error:
        LW_SAFE_FREE_MEMORY(tmp);
        LW_SAFE_FREE_MEMORY(dn);
        LW_SAFE_FREE_MEMORY(name);

        goto cleanup;
}

DWORD ExecuteAdtNewGroupAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;


    PSTR desc[] = {
        action->newGroup.desc,
        NULL
    };

    PSTR name[] = {
        action->newGroup.name,
        NULL
    };

    PSTR cn[] = {
        action->newGroup.name,
        NULL
    };

    PSTR samAccountName[] = {
        action->newGroup.namePreWin2000,
        NULL
    };

    PSTR gType[] = {
        NULL,
        NULL
    };

    AttrValsT avp[] = {
        { "objectClass", GetObjectClassVals(ObjectClassGroup) },
        { "name", name },
        { "cn", cn },
        { "sAMAccountName", samAccountName },
        { "description", desc },
        { "groupType", gType },
        { NULL, NULL }
    };

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating group %s ...\n",
                appContext->actionName, action->newGroup.dn);

    if(IsEqual(action->newGroup.scope, GROUP_TYPE_NAME_DOMAIN_LOCAL, 1)) {
        gType[0] = GROUP_TYPE_DOMAIN_LOCAL;
    }
    else {
        if(IsEqual(action->newGroup.scope, GROUP_TYPE_NAME_GLOBAL, 1)) {
            gType[0] = GROUP_TYPE_GLOBAL;
        }
        else {
            gType[0] = GROUP_TYPE_UNIVERSAL;
        }
    }

    dwError = CreateADObject(appContext, action->newGroup.dn, avp);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating group - done \n",
                appContext->actionName);

    // TODO: Set account controls and password here via RPC

    if(appContext->gopts.isPrintDN) {
        PrintResult(appContext, LogLevelNone, "%s\n", action->newGroup.dn);
    }
    else {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "Group %s has been created.\n", action->newGroup.name);
        }
    }

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

DWORD CleanUpAdtNewGroupAction(IN AdtActionTP action)
{
    return CleanUpBaseAction(action);
}

/***************************************************************************/
/*                          New computer action                            */
/***************************************************************************/

DWORD InitAdtNewComputerAction(IN AdtActionTP action)
{
    return InitBaseAction(action);
}


DWORD ValidateAdtNewComputerAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR dn = NULL;
    PSTR name = NULL;
    PSTR tmp = NULL;

    dwError = OpenADSearchConnectionDN(action, &(action->newComputer.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    SwitchToSearchConnection(action);

    if (!action->newComputer.dn) {
        dwError = LwAllocateStringPrintf(&(action->newComputer.dn), "CN=Computers");
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    dwError = ProcessDash(&(action->newComputer.dn));
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!IsNVP(action->newComputer.dn)) {
        dwError = LwAllocateStringPrintf(&dn, "OU=%s", action->newComputer.dn);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

        dwError = ResolveDN(appContext, ObjectClassAny, dn, &tmp);

        if(dwError) {
            LW_SAFE_FREE_MEMORY(dn);
            dwError = LwAllocateStringPrintf(&dn, "CN=%s", action->newComputer.dn);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        LW_SAFE_FREE_MEMORY(action->newComputer.dn);
        action->newComputer.dn = dn;
    }

    dwError = ResolveDN(appContext, ObjectClassAny, action->newComputer.dn, &dn);
    ADT_BAIL_ON_ERROR_NP(dwError);
    LW_SAFE_FREE_MEMORY(action->newComputer.dn);
    action->newComputer.dn = dn;

    dwError = ProcessDash(&(action->newComputer.name));
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!action->newComputer.name) {
        dwError = ADT_ERR_ARG_MISSING_NAME;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    if(!action->newComputer.namePreWin2000) {
        dwError = LwStrDupOrNull(action->newComputer.name, &(action->newComputer.namePreWin2000));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    dwError = LwAllocateStringPrintf(&dn, "CN=%s,%s", action->newComputer.name, action->newComputer.dn);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    LW_SAFE_FREE_MEMORY(action->newComputer.dn);
    action->newComputer.dn = dn;

    LW_SAFE_FREE_MEMORY(name);
    dwError = LwAllocateStringPrintf(&name, "%s$", action->newComputer.namePreWin2000);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    LwStrToUpper(name);
    LW_SAFE_FREE_MEMORY(action->newComputer.namePreWin2000);
    action->newComputer.namePreWin2000 = name;

    if(!action->newComputer.desc) {
        dwError = LwStrDupOrNull("lw-adtool created", &(action->newComputer.desc));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    dn = NULL;
    name = NULL;

    cleanup:
        return dwError;

    error:
        LW_SAFE_FREE_MEMORY(tmp);
        LW_SAFE_FREE_MEMORY(dn);
        LW_SAFE_FREE_MEMORY(name);

        goto cleanup;
}


DWORD ExecuteAdtNewComputerAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    PSTR desc[] = {
        action->newComputer.desc,
        NULL
    };

    PSTR name[] = {
        action->newComputer.name,
        NULL
    };

    PSTR cn[] = {
        action->newComputer.name,
        NULL
    };

    PSTR samAccountName[] = {
        action->newComputer.namePreWin2000,
        NULL
    };

    AttrValsT avp[] = {
        { "objectClass", GetObjectClassVals(ObjectClassComputer) },
        { "name", name },
        { "cn", cn },
        { "sAMAccountName", samAccountName },
        { "description", desc },
        { NULL, NULL }
    };

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating computer %s ...\n",
                appContext->actionName, action->newComputer.dn);

    dwError = CreateADObject(appContext, action->newComputer.dn, avp);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating computer - done \n",
                appContext->actionName);

    // TODO: Set account controls and password here via RPC

    if(appContext->gopts.isPrintDN) {
        PrintResult(appContext, LogLevelNone, "%s\n", action->newComputer.dn);
    }
    else {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "Computer %s has been created.\n", action->newComputer.name);
        }
    }

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

DWORD CleanUpAdtNewComputerAction(IN AdtActionTP action)
{
    return CleanUpBaseAction(action);
}
