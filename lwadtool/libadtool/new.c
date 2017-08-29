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

static
DWORD  SetMachinePassword(IN AdtActionTP action);

static
DWORD SetNewComputerSPNAttribute(IN AdtActionTP action);

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
        dwError = LwStrDupOrNull("adtool created", &(action->newOu.desc));
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
        dwError = LwStrDupOrNull("adtool created", &(action->newUser.desc));
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
    }

    dwError = LwAllocateMemory(2 * sizeof(AttrValsT), OUT_PPVOID(&avpTime));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    dwError = LwAllocateMemory(2 * sizeof(PSTR), OUT_PPVOID(&(avpTime[0].vals)));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    avpTime[0].attr = "pwdLastSet";

    avpTime[0].vals[0] = action->newUser.isNoMustChangePasswd ? "-1" : "0";

    dwError = ModifyADObject(appContext, action->newUser.dn, avpTime, 2);

    ADT_BAIL_ON_ERROR_NP(dwError);

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

    if (action->newUser.keytab)
    {
       dwError = CreateNewUserKeytabFile(action);
       if (dwError)
       {
           PrintResult(appContext,
                       LogLevelNone,
                       "%s: Failed to create keytab file for user %s.\n",
                       appContext->actionName, action->newUser.name);
           // Ignore the failure. Leave it up to the admin to delete new user and retry or use
           // adtool reset-user-password to try and create a keytab file for the user.
           dwError = 0; 
       }
       else
       {
          if(!appContext->gopts.isQuiet) 
          {
              PrintResult(appContext,
                          LogLevelNone,
                          "%s: Keytab file created for user %s.\n",
                          appContext->actionName, action->newUser.name);

              if (!action->newUser.isNoMustChangePasswd)
              {
                  PrintResult(appContext,
                              LogLevelInfo,
                              "\tNote: Since --no-must-change-password option was not specified,\n"\
                              "\tthe new user AD account was created with \"User must change password\n"\
                              "\tat next logon\" enabled. The entries in the keytab file was generated\n"\
                              "\tusing the password given on the command line.\n");
              }
          }
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
        dwError = LwStrDupOrNull("adtool created", &(action->newGroup.desc));
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

// Input:  nfs, NFS, http/linuxhostbox, host, HOST/, http/linuxhostbox2
// Output: NFS, HOST, http/linuxhostbox, http/linuxhostbox2
static
VOID GroomSpnList(AdtActionTP action, PSTR *ppGroomedServicePrincipalList)
{
   BOOLEAN bIsServiceClass = FALSE;
   PSTR pszServicePrincipalNameList = NULL;
   PSTR saveStrPtr = NULL;
   PSTR aStr = NULL;
   PSTR isStrThere = NULL;
   PSTR pszTempStr = NULL;
   PSTR pszNewServicePrincipalNameList = NULL;
   PSTR pszFqdnList = NULL;
   PSTR pszServiceClassList = NULL;

   if (action->newComputer.servicePrincipalNameList)
     LwAllocateString(action->newComputer.servicePrincipalNameList, &pszServicePrincipalNameList);
   else
     LwAllocateString(DEFAULT_SERVICE_PRINCIPAL_NAME_LIST, &pszServicePrincipalNameList);

   // Tokenize the user provided SPN list checking for duplicates SPN values and 
   // upper casing SPN.
   aStr = strtok_r(pszServicePrincipalNameList, ",", &saveStrPtr);
   while (aStr != NULL)
   {
      GroomSpn(&aStr, &bIsServiceClass);

      if (bIsServiceClass)
      {
         LwStrToUpper(aStr);
         LwStrStr(pszServiceClassList, aStr, &isStrThere);
         if (!isStrThere)
         {
            if (pszServiceClassList)
               LwAllocateStringPrintf(&pszTempStr, "%s,%s", pszServiceClassList, aStr);
            else
               LwAllocateStringPrintf(&pszTempStr, "%s", aStr);
 
            LW_SAFE_FREE_STRING(pszServiceClassList);
            pszServiceClassList = pszTempStr;
            pszTempStr = NULL;
         }
      }
      else
      {
         LwStrStr(pszFqdnList, aStr, &isStrThere);
         if ((!isStrThere) || (strlen(aStr) != (strlen(isStrThere))))
         {
            if (pszFqdnList)
               LwAllocateStringPrintf(&pszTempStr, "%s,%s", pszFqdnList, aStr);
            else
               LwAllocateStringPrintf(&pszTempStr, "%s", aStr);

            LW_SAFE_FREE_STRING(pszFqdnList);
            pszFqdnList = pszTempStr;
            pszTempStr = NULL;
         }
      }

      aStr = strtok_r(NULL, ",", &saveStrPtr);
   }


   if (pszFqdnList && pszServiceClassList)
     LwAllocateStringPrintf(&pszNewServicePrincipalNameList, "%s,%s", pszServiceClassList, pszFqdnList);
   else if (pszFqdnList && !pszServiceClassList)
     LwAllocateStringPrintf(&pszNewServicePrincipalNameList, "%s", pszFqdnList);
   else
     LwAllocateStringPrintf(&pszNewServicePrincipalNameList, "%s", pszServiceClassList);

   // pszNewServicePrincipalNameList contains non-duplicated, uppercase service class.
   // Fully qualified SPN is added as is.
   *ppGroomedServicePrincipalList = pszNewServicePrincipalNameList;

   LW_SAFE_FREE_STRING(pszFqdnList);
   LW_SAFE_FREE_STRING(pszServiceClassList);

   return;
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

    if((action->newComputer.keytab) && (!action->newComputer.password)) {
        dwError = ADT_ERR_ARG_MISSING_PASSWD;
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
        dwError = LwStrDupOrNull("adtool created", &(action->newComputer.desc));
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

    PSTR dnsHostName[] = {
        action->newComputer.dnsHostName,
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

    PSTR userAccountControl[] = {
        "4128",
        NULL
    };

    AttrValsT avp[] = {
        { "objectClass", GetObjectClassVals(ObjectClassComputer) },
        { "name", name },
        { "cn", cn },
        { "sAMAccountName", samAccountName },
        { "description", desc },
        { "userAccountControl", userAccountControl },
        { "dnsHostName", dnsHostName },
        { NULL, NULL }
    };
    
    //remove the dnsHostName attribute if it has no value
    if(action->newComputer.dnsHostName == NULL)
    {
        avp[6].attr = NULL;
        avp[6].vals = NULL;
    }

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating computer %s ...\n",
                appContext->actionName, action->newComputer.dn);

    dwError = CreateADObject(appContext, action->newComputer.dn, avp);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating computer - done \n",
                appContext->actionName);

    if(appContext->gopts.isPrintDN) {
        PrintResult(appContext, LogLevelNone, "%s\n", action->newComputer.dn);
    }
    else {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "Computer %s has been created.\n", action->newComputer.name);
        }
    }

    if (action->newComputer.password)
    {
        dwError = SetMachinePassword(action);
        ADT_BAIL_ON_ERROR_STR(dwError, "Password set failed");

        PrintStderr(appContext, LogLevelVerbose, "Password set successfully.\n");
    }

    // If --spn option not provided, then default is host.
    dwError = SetNewComputerSPNAttribute(action);
    if (dwError)
    {
       PrintResult(appContext,
                   LogLevelNone,
                   "%s: Failed to update %s service principal name attribute.\n",
                   appContext->actionName, action->newComputer.name);
       ADT_BAIL_ON_ERROR_NP(dwError);
    }

    if (action->newComputer.keytab)
    {

       dwError = CreateNewComputerKeytabFile(action);
       if (dwError)
       {
          PrintResult(appContext,
                      LogLevelError,
                      "%s: Failed to create keytab file %s for %s.\n",
                      appContext->actionName, action->newComputer.keytab, action->newComputer.name);
          // Ignore the failure. Leave it up to the admin to delete new computer account and retry.
          dwError = 0;
       }
       else
       {
           PrintResult(appContext,
                       LogLevelInfo,
                       "%s: Keytab file %s created for computer %s.\n",
                       appContext->actionName, action->newComputer.keytab, action->newComputer.name);
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


static
DWORD  SetMachinePassword(IN AdtActionTP action)
{
    DWORD dwError = 0;
    PSTR pszPassword = NULL;
    PSTR pszDn = NULL;
    AttrValsT *avp = NULL;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    PrintStdout(appContext,
                LogLevelVerbose,
                "%s: SetMachinePassword. Name=%s Password=%s sAMAccountName=%s DnsHostName=%s\n",
                appContext->actionName, 
                action->newComputer.name,
                action->newComputer.password,
                action->newComputer.namePreWin2000,
                action->newComputer.dnsHostName);

    if (!action->newComputer.password)
    {
       dwError = ADT_ERR_ARG_MISSING_PASSWD;
       ADT_BAIL_ON_ERROR(dwError);
    }

    dwError = LwAllocateString(action->newComputer.password, &pszPassword);
    ADT_BAIL_ON_ERROR(dwError);

    if (!action->newComputer.dn)
    {
       dwError = ADT_ERR_ARG_MISSING_DN;
       ADT_BAIL_ON_ERROR(dwError);
    }

    dwError = LwAllocateString(action->newComputer.dn, &pszDn);
    ADT_BAIL_ON_ERROR(dwError);

    dwError = LwAllocateMemory(2 * sizeof(AttrValsT), OUT_PPVOID(&avp));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    avp[0].attr = "samAccountName";

    dwError = GetObjectAttrs(appContext, pszDn, avp);
    ADT_BAIL_ON_ERROR(dwError);

    if(!avp[0].vals || !avp[0].vals[0]) 
    {
        dwError = ADT_ERR_FAILED_AD_GET_ATTR;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = AdtNetUserSetPassword(appContext, avp[0].vals[0], pszPassword);
    ADT_BAIL_ON_ERROR(dwError);


    PrintStdout(appContext, LogLevelVerbose,
                "%s: SetMachinePassword. Changed password successfully.\n",
                appContext->actionName);

    cleanup:

        LW_SAFE_FREE_MEMORY(pszPassword);
        LW_SAFE_FREE_MEMORY(pszDn);

        return dwError;

    error:

        goto cleanup;
}

static
DWORD SetNewComputerSPNAttribute(IN AdtActionTP action)
{
   DWORD dwError = 0;
   DWORD dwMaxValues = 100;
   DWORD i, j = 0;
   PSTR aStr = NULL;
   PSTR saveStrPtr = NULL;
   PSTR pszDn = NULL;
   PSTR pszDomainFromDn = NULL;
   PSTR pszMachineName = NULL;
   AttrValsT *avp = NULL;
   AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
   BOOLEAN bIsServiceClass = FALSE;
   PSTR pszServicePrincipalNameList = NULL;

   GroomSpnList(action, &pszServicePrincipalNameList);

   PrintStdout(appContext, LogLevelVerbose,
               "%s: Setting SPN Attribute using: %s\n",
               appContext->actionName, pszServicePrincipalNameList);

   dwError = LwAllocateMemory(2 * sizeof(AttrValsT), OUT_PPVOID(&avp));
   ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

   dwError = LwStrDupOrNull("servicePrincipalName", &avp[0].attr);
   ADT_BAIL_ON_ERROR(dwError);

    dwError = LwAllocateMemory(dwMaxValues * sizeof(PSTR), OUT_PPVOID(&(avp[0].vals)));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    for (i = 0; i < dwMaxValues; i++)
       avp[0].vals[i] = NULL;

   dwError = LwStrDupOrNull(action->newComputer.dn, &pszDn);
   ADT_BAIL_ON_ERROR(dwError);

   dwError = LwStrDupOrNull(action->newComputer.name, &pszMachineName);
   ADT_BAIL_ON_ERROR(dwError);

   LwStrToLower(pszMachineName);

   dwError = GetDomainFromDN(pszDn, &pszDomainFromDn);
   ADT_BAIL_ON_ERROR(dwError);

   LwStrToLower(pszDomainFromDn);

   aStr = strtok_r(pszServicePrincipalNameList, ",", &saveStrPtr);
   i = 0;
   while ((aStr != NULL) && (i < dwMaxValues))
   {
      GroomSpn(&aStr, &bIsServiceClass);
      if (bIsServiceClass)
      {
         dwError = LwAllocateStringPrintf(&avp[0].vals[i], "%s/%s", aStr, pszMachineName);
         ADT_BAIL_ON_ERROR(dwError);

         dwError = LwAllocateStringPrintf(&avp[0].vals[i+1],"%s/%s.%s", aStr, pszMachineName, pszDomainFromDn);
         ADT_BAIL_ON_ERROR(dwError);
         i+=2;
      }
      else
      {
         dwError = LwAllocateStringPrintf(&avp[0].vals[i], "%s", aStr);
         ADT_BAIL_ON_ERROR(dwError);
         i++; 
      }
      aStr = strtok_r(NULL, ",", &saveStrPtr);
   }
 
   dwError = ModifyADObject(appContext, pszDn, avp, 2);
   ADT_BAIL_ON_ERROR(dwError);

   PrintStdout(appContext, LogLevelVerbose,
               "%s: Successfully updated servicePrincipalName.\n",
               appContext->actionName);

cleanup:
   LW_SAFE_FREE_STRING(pszServicePrincipalNameList);
   LW_SAFE_FREE_MEMORY(pszDn);
   LW_SAFE_FREE_MEMORY(pszMachineName);
   LW_SAFE_FREE_MEMORY(pszDomainFromDn);

   if (avp)
   {
      for (i = 0; avp[i].vals; ++i)
      {
         for (j = 0; avp[i].vals[j]; ++j)
            LW_SAFE_FREE_MEMORY(avp[i].vals[j]);

         LW_SAFE_FREE_MEMORY(avp[i].vals);
     }

     LW_SAFE_FREE_MEMORY(avp);
   }



   return dwError;

error:
   goto cleanup;
}
