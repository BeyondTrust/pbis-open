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
 *        search.c
 *
 * Abstract:
 *        Methods for searching for AD objects (DN resolution).
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
DWORD InitAdtSearchUserAction(IN AdtActionTP action)
{
    return InitBaseAction(action);
}

DWORD InitAdtSearchGroupAction(IN AdtActionTP action)
{
    return InitBaseAction(action);
}

DWORD InitAdtSearchOuAction(IN AdtActionTP action)
{
    return InitBaseAction(action);
}

DWORD InitAdtSearchComputerAction(IN AdtActionTP action)
{
    return InitBaseAction(action);
}

DWORD InitAdtSearchObjectAction(IN AdtActionTP action)
{
    return InitBaseAction(action);
}

/**
 * Validate method for all the search actions.
 */
static DWORD ValidateAdtSearchBase(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR dn = NULL;
    INT isDefault = 0;

    dwError = OpenADSearchConnectionDomain(action, &(action->searchUser.name));
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = OpenADSearchConnectionDN(action, &(action->searchUser.base));
    ADT_BAIL_ON_ERROR_NP(dwError);

    SwitchToSearchConnection(action);

    if (!action->searchUser.name) {
        dwError = ADT_ERR_ARG_MISSING_NAME;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = ProcessDash(&(action->searchUser.name));
    ADT_BAIL_ON_ERROR_NP(dwError);

    if (!action->searchUser.base) {
        isDefault = 1;
        dwError = LwStrDupOrNull((PCSTR) appContext->workConn->defaultNC, &(action->searchUser.base));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if (!action->searchUser.scope) {
        dwError = LwStrDupOrNull(ADT_SEARCH_SCOPE_SUB, &(action->searchUser.scope));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        action->searchUser.scopeN = LDAP_SCOPE_SUBTREE;
    }
    else {
        LwStrToLower(action->searchUser.scope);

        if(!strcmp((PCSTR) action->searchUser.scope, ADT_SEARCH_SCOPE_SUB)) {
            action->searchUser.scopeN = LDAP_SCOPE_SUBTREE;
        }
        else {
            if(!strcmp((PCSTR) action->searchUser.scope, ADT_SEARCH_SCOPE_ONE)) {
                action->searchUser.scopeN = LDAP_SCOPE_ONELEVEL;
            }
            else {
                if(!strcmp((PCSTR) action->searchUser.scope, ADT_SEARCH_SCOPE_BASE)) {
                    action->searchUser.scopeN = LDAP_SCOPE_BASE;
                }
                else {
                    dwError = ADT_ERR_INVALID_SEARCH_SCOPE;
                    ADT_BAIL_ON_ERROR_NP(dwError);
                }
            }
        }
    }

    dwError = ProcessDash(&(action->searchUser.base));
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!isDefault) {
        dwError = ResolveDN(appContext, ObjectClassAny, action->searchUser.base, &dn);
        ADT_BAIL_ON_ERROR_NP(dwError);
        LW_SAFE_FREE_MEMORY(action->searchUser.base);
        action->searchUser.base = dn;
    }

    cleanup:
        return dwError;

    error:
        LW_SAFE_FREE_MEMORY(dn);
        goto cleanup;
}

/**
 * Actions validate methods.
 */
DWORD ValidateAdtSearchUserAction(IN AdtActionTP action)
{
    return ValidateAdtSearchBase(action);
}

DWORD ValidateAdtSearchGroupAction(IN AdtActionTP action)
{
    return ValidateAdtSearchBase(action);
}

DWORD ValidateAdtSearchOuAction(IN AdtActionTP action)
{
    return ValidateAdtSearchBase(action);
}

DWORD ValidateAdtSearchComputerAction(IN AdtActionTP action)
{
    return ValidateAdtSearchBase(action);
}

DWORD ValidateAdtSearchObjectAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR dn = NULL;
    INT isDefault = 0;

    dwError = OpenADSearchConnectionDN(action, &(action->searchObject.base));
    ADT_BAIL_ON_ERROR_NP(dwError);

    SwitchToSearchConnection(action);

    if (!action->searchObject.filter) {
        dwError = LwStrDupOrNull("(objectClass=top)", &(action->searchObject.filter));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if (!action->searchObject.base) {
        isDefault = 1;
        dwError = LwStrDupOrNull((PCSTR) appContext->workConn->defaultNC, &(action->searchObject.base));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if (!action->searchObject.scope) {
        dwError = LwStrDupOrNull(ADT_SEARCH_SCOPE_SUB, &(action->searchObject.scope));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        action->searchObject.scopeN = LDAP_SCOPE_SUBTREE;
    }
    else {
        LwStrToLower(action->searchObject.scope);

        if(!strcmp((PCSTR) action->searchObject.scope, ADT_SEARCH_SCOPE_SUB)) {
            action->searchObject.scopeN = LDAP_SCOPE_SUBTREE;
        }
        else {
            if(!strcmp((PCSTR) action->searchObject.scope, ADT_SEARCH_SCOPE_ONE)) {
                action->searchObject.scopeN = LDAP_SCOPE_ONELEVEL;
            }
            else {
                if(!strcmp((PCSTR) action->searchObject.scope, ADT_SEARCH_SCOPE_BASE)) {
                    action->searchObject.scopeN = LDAP_SCOPE_BASE;
                }
                else {
                    dwError = ADT_ERR_INVALID_SEARCH_SCOPE;
                    ADT_BAIL_ON_ERROR_NP(dwError);
                }
            }
        }
    }

    dwError = ProcessDash(&(action->searchObject.base));
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!isDefault) {
        dwError = ResolveDN(appContext, ObjectClassAny, action->searchObject.base, &dn);
        ADT_BAIL_ON_ERROR_NP(dwError);
        LW_SAFE_FREE_MEMORY(action->searchObject.base);
        action->searchObject.base = dn;
    }

    cleanup:
        return dwError;

    error:
        LW_SAFE_FREE_MEMORY(dn);
        goto cleanup;
}

/**
 * Actions execute method.
 */
DWORD ExecuteAdtSearchUserAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR *outDNs = NULL;
    PSTR dn = NULL;
    INT i = 0;
    PSTR filter = NULL;

    PrintStderr(appContext, LogLevelVerbose, "%s: Searching for user %s in %s ...\n",
                appContext->actionName, action->searchUser.name, action->searchUser.base);

    if(IsDNComp(action->searchUser.name)) {
        dwError = ResolveDN(appContext, ObjectClassUser, action->searchUser.name, &dn);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }
    else {
        if (IsNVP(action->searchUser.name)) {
            dwError = LwAllocateStringPrintf(&filter,
                                             "(&(objectClass=user)(%s))",
                                             action->searchUser.name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }
        else {
            dwError
                    = LwAllocateStringPrintf(&filter,
                                             "(&(objectClass=user)(|(%s=%s)(CN=%s)))",
                                             IsUPN(action->searchUser.name)
                                                     ? "userPrincipalName"
                                                     : "sAMAccountName",
                                             action->searchUser.name,
                                             action->searchUser.name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        dwError = SearchForObject(appContext, ObjectClassUser,
                                  action->searchUser.scopeN, filter,
                                  action->searchUser.base, &outDNs);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelVerbose, "%s: Done searching for user \n",
                appContext->actionName);

    if(dn) {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "%s\n", dn);
            i = 1;
        }
    }
    else {
        for(i = 0; outDNs && outDNs[i]; ++i) {
            if(!appContext->gopts.isQuiet) {
                PrintResult(appContext, LogLevelNone, "%s\n", outDNs[i]);
            }
        }
    }

    if(!appContext->gopts.isPrintDN) {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "\nTotal users: %d\n", i);
        }
    }

    cleanup:
        if(outDNs) {
            for(i = 0; outDNs[i]; ++i) {
                LW_SAFE_FREE_MEMORY(outDNs[i]);
            }

            LW_SAFE_FREE_MEMORY(outDNs);
        }

        LW_SAFE_FREE_MEMORY(filter);
        LW_SAFE_FREE_MEMORY(dn);

        return dwError;

    error:
        goto cleanup;
}

DWORD ExecuteAdtSearchGroupAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR *outDNs = NULL;
    PSTR dn = NULL;
    INT i = 0;
    PSTR filter = NULL;

    PrintStderr(appContext, LogLevelVerbose, "%s: Searching for group %s in %s ...\n",
                appContext->actionName, action->searchGroup.name, action->searchGroup.base);

    if(IsDNComp(action->searchGroup.name)) {
        dwError = ResolveDN(appContext, ObjectClassGroup, action->searchGroup.name, &dn);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }
    else {
        if (IsNVP(action->searchGroup.name)) {
            dwError = LwAllocateStringPrintf(&filter,
                                             "(&(objectClass=group)(%s))",
                                             action->searchGroup.name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }
        else {
            dwError
                    = LwAllocateStringPrintf(&filter,
                                             "(&(objectClass=group)(|(sAMAccountName=%s)(CN=%s)))",
                                             action->searchGroup.name,
                                             action->searchGroup.name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        dwError = SearchForObject(appContext, ObjectClassGroup,
                                  action->searchGroup.scopeN, filter,
                                  action->searchGroup.base, &outDNs);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelVerbose, "%s: Done searching for group \n",
                appContext->actionName);

    if(dn) {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "%s\n", dn);
            i = 1;
        }
    }
    else {
        for(i = 0; outDNs && outDNs[i]; ++i) {
            if(!appContext->gopts.isQuiet) {
                PrintResult(appContext, LogLevelNone, "%s\n", outDNs[i]);
            }
        }
    }

    if(!appContext->gopts.isPrintDN) {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "\nTotal groups: %d\n", i);
        }
    }

    cleanup:
        if(outDNs) {
            for(i = 0; outDNs[i]; ++i) {
                LW_SAFE_FREE_MEMORY(outDNs[i]);
            }

            LW_SAFE_FREE_MEMORY(outDNs);
        }

        LW_SAFE_FREE_MEMORY(filter);
        LW_SAFE_FREE_MEMORY(dn);

        return dwError;

    error:
        goto cleanup;
}

DWORD ExecuteAdtSearchOuAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR *outDNs = NULL;
    PSTR dn = NULL;
    INT i = 0;
    PSTR filter = NULL;

    PrintStderr(appContext, LogLevelVerbose, "%s: Searching for organizational unit %s in %s ...\n",
                appContext->actionName, action->searchOu.name, action->searchOu.base);

    if(IsDNComp(action->searchOu.name)) {
        dwError = ResolveDN(appContext, ObjectClassOU, action->searchOu.name, &dn);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }
    else {
        if (IsNVP(action->searchOu.name)) {
            dwError
                    = LwAllocateStringPrintf(&filter,
                                             "(&(objectClass=organizationalUnit)(%s))",
                                             action->searchOu.name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }
        else {
            dwError
                    = LwAllocateStringPrintf(&filter,
                                             "(&(objectClass=organizationalUnit)(|(OU=%s)(name=%s)))",
                                             action->searchOu.name,
                                             action->searchOu.name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        dwError = SearchForObject(appContext, ObjectClassOU,
                                  action->searchOu.scopeN, filter,
                                  action->searchOu.base, &outDNs);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelVerbose, "%s: Done searching for organizational unit\n",
                appContext->actionName);

    if(dn) {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "%s\n", dn);
            i = 1;
        }
    }
    else {
        for(i = 0; outDNs && outDNs[i]; ++i) {
            if(!appContext->gopts.isQuiet) {
                PrintResult(appContext, LogLevelNone, "%s\n", outDNs[i]);
            }
        }
    }

    if(!appContext->gopts.isPrintDN) {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "\nTotal organizational units: %d\n", i);
        }
    }

    cleanup:
        if(outDNs) {
            for(i = 0; outDNs[i]; ++i) {
                LW_SAFE_FREE_MEMORY(outDNs[i]);
            }

            LW_SAFE_FREE_MEMORY(outDNs);
        }

        LW_SAFE_FREE_MEMORY(filter);
        LW_SAFE_FREE_MEMORY(dn);

        return dwError;

    error:
        goto cleanup;
}

DWORD ExecuteAdtSearchComputerAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR *outDNs = NULL;
    PSTR dn = NULL;
    INT i = 0;
    PSTR filter = NULL;

    PrintStderr(appContext,
                LogLevelVerbose,
                "%s: Searching for computer %s in %s ...\n",
                appContext->actionName,
                action->searchComputer.name,
                action->searchComputer.base);

    if (IsDNComp(action->searchComputer.name)) {
        dwError = ResolveDN(appContext,
                            ObjectClassComputer,
                            action->searchComputer.name,
                            &dn);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }
    else {
        if (IsNVP(action->searchComputer.name)) {
            dwError = LwAllocateStringPrintf(&filter,
                                             "(&(objectClass=computer)(%s))",
                                             action->searchComputer.name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }
        else {
            if (IsSPN(action->searchComputer.name)) {
                dwError
                        = LwAllocateStringPrintf(&filter,
                                                 "(&(objectClass=computer)(servicePrincipalName=%s))",
                                                 action->searchComputer.name);
            }
            else {
                dwError
                        = LwAllocateStringPrintf(&filter,
                                                 "(&(objectClass=computer)(sAMAccountName=%s$))",
                                                 action->searchComputer.name);
            }
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        dwError = SearchForObject(appContext,
                                  ObjectClassComputer,
                                  action->searchComputer.scopeN,
                                  filter,
                                  action->searchComputer.base,
                                  &outDNs);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelVerbose, "%s: Done searching for computer \n",
                appContext->actionName);

    if(dn) {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "%s\n", dn);
            i = 1;
        }
    }
    else {
        for(i = 0; outDNs && outDNs[i]; ++i) {
            if(!appContext->gopts.isQuiet) {
                PrintResult(appContext, LogLevelNone, "%s\n", outDNs[i]);
            }
        }
    }

    if(!appContext->gopts.isPrintDN) {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "\nTotal computers: %d\n", i);
        }
    }

    cleanup:
        if(outDNs) {
            for(i = 0; outDNs[i]; ++i) {
                LW_SAFE_FREE_MEMORY(outDNs[i]);
            }

            LW_SAFE_FREE_MEMORY(outDNs);
        }

        LW_SAFE_FREE_MEMORY(filter);
        LW_SAFE_FREE_MEMORY(dn);

        return dwError;

    error:
        goto cleanup;
}

DWORD ExecuteAdtSearchObjectAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR *outDNs = NULL;
    INT i = 0;

    PrintStderr(appContext, LogLevelVerbose, "%s: Searching for AD object in %s ...\n",
                appContext->actionName, action->searchObject.base);

    dwError = SearchForObject(appContext, ObjectClassAny,
                              action->searchObject.scopeN,
                              action->searchObject.filter,
                              action->searchObject.base,
                              &outDNs);

    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelVerbose, "%s: Done searching for AD object\n",
                appContext->actionName);

    for (i = 0; outDNs && outDNs[i]; ++i) {
        if (!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "%s\n", outDNs[i]);
        }
    }

    if(!appContext->gopts.isPrintDN) {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "\nTotal objects: %d\n", i);
        }
    }

    cleanup:
        if(outDNs) {
            for(i = 0; outDNs[i]; ++i) {
                LW_SAFE_FREE_MEMORY(outDNs[i]);
            }

            LW_SAFE_FREE_MEMORY(outDNs);
        }

        return dwError;

    error:
        goto cleanup;
}

/**
 * Actions clean up methods.
 */
DWORD CleanUpAdtSearchUserAction(IN AdtActionTP action)
{
    return CleanUpBaseAction(action);
}

DWORD CleanUpAdtSearchGroupAction(IN AdtActionTP action)
{
    return CleanUpBaseAction(action);
}

DWORD CleanUpAdtSearchOuAction(IN AdtActionTP action)
{
    return CleanUpBaseAction(action);
}

DWORD CleanUpAdtSearchComputerAction(IN AdtActionTP action)
{
    return CleanUpBaseAction(action);
}

DWORD CleanUpAdtSearchObjectAction(IN AdtActionTP action)
{
    return CleanUpBaseAction(action);
}
