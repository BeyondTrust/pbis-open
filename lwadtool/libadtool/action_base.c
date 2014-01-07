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
 *        commons.c
 *
 * Abstract:
 *         This file is intended to contain code common to all actions.
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Apr 12, 2010
 *
 */

#include "includes.h"

/**
 * Check if we are in multi-forest mode.
 *
 * @param action Action reference.
 * @return TRUE if we are in two-forest mode; FALSE otherwise.
 */
BOOL IsMultiForestMode(IN AdtActionTP action)
{
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    return
        (appContext->searchConn.conn != NULL) &&
        (appContext->searchConn.conn != appContext->modifyConn.conn);
}

/**
 * Switch AD connection.
 *
 * @param action Action reference.
 */
VOID SwitchConnection(IN AdtActionTP action)
{
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    if(appContext->workConn == &(appContext->searchConn)) {
        appContext->workConn = &(appContext->modifyConn);
    }
    else {
        appContext->workConn = &(appContext->searchConn);
    }
}

/**
 * Start using search connection.
 *
 * @param action Action reference.
 */
VOID SwitchToSearchConnection(IN AdtActionTP action)
{
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    if(appContext->searchConn.conn) {
        appContext->workConn = &(appContext->searchConn);
    }
}

/**
 * Start using search connection.
 *
 * @param action Action reference.
 */
VOID SwitchToModifyConnection(IN AdtActionTP action)
{
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    if(appContext->modifyConn.conn) {
        appContext->workConn = &(appContext->modifyConn);
    }
}

/**
 * Open second connection for multi-forest searches if the name
 * specified via DN.
 *
 * @param action Action reference.
 * @param name Domain name or DN/RDN.
 * return 0 on success; error code on failure.
 */
DWORD OpenADSearchConnectionDN(IN AdtActionTP action, IN OUT PSTR *name)
{
    DWORD dwError = ERROR_SUCCESS;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR domain = NULL;
    PSTR p = NULL;
    PSTR tmp = NULL;

    if(appContext->searchConn.conn || (*name == NULL)) {
        goto cleanup;
    }

    appContext->workConn = & (appContext->searchConn);

    if(IsDNComp(*name)) {
        dwError = LwStrDupOrNull((PCSTR) *name, &tmp);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

        LwStrToLower(tmp);
        p = strstr((PCSTR) tmp, "dc=");

        if(p) {
            //if(!DoesStrEndWith(*name, appContext->modifyConn.defaultNC, 1)) {
            if(!IsEqual(p, appContext->modifyConn.defaultNC, 1)) {
                dwError = GetDomainFromDN(*name, &domain);
                ADT_BAIL_ON_ERROR_NP(dwError);

                dwError = ProcessConnectArgs(appContext,
                                             NULL,
                                             appContext->copts.port,
                                             domain);
                ADT_BAIL_ON_ERROR_NP(dwError);

                dwError = ConnectAD(appContext);
                ADT_BAIL_ON_ERROR_NP(dwError);
            }
        }
    }

    cleanup:
        LW_SAFE_FREE_MEMORY(tmp);
        LW_SAFE_FREE_MEMORY(domain);
        appContext->workConn = & (appContext->modifyConn);
        return dwError;

    error:
        goto cleanup;
}

/**
 * Switch to connection that matches user's or group's UPN.
 *
 * @param action Action reference.
 * @param name Domain name or DN/RDN.
 */
VOID SwitchToMatchingConnection(IN AdtActionTP action, IN OUT PSTR *name)
{
    DWORD dwError = ERROR_SUCCESS;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR domain = NULL;
    PSTR domainComp = NULL;
    PSTR tmp = NULL;
    PSTR p = NULL;
    INT len = 0;

    if (IsUPN(*name)) {
        domain = GetRealmComp(*name);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(domain);

        if(domain == NULL) {
            dwError = LwStrDupOrNull(*name, &domain);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        if (DoesStrStartWith(domain, appContext->modifyConn.domainName, 1)) {
            appContext->workConn = &(appContext->modifyConn);
        }
        else {
            appContext->workConn = &(appContext->searchConn);
        }
    }
    else {
        if (IsBackSlashPresent(*name)) {
            p = strstr((PCSTR) *name, "\\");

            if (p) {
                len = p - *name;
                dwError = LwAllocateMemory(sizeof(CHAR) * (len + 1),
                                           OUT_PPVOID(&domainComp));
                ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

                strncpy(domainComp, (PCSTR) *name, len);
            }

            if (!IsDotPresent(domainComp)) {
                p = strstr((PCSTR) appContext->modifyConn.domainName, ".");

                dwError
                        = LwAllocateStringPrintf(&domain, "%s%s", domainComp, p);
                ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
            }
            else {
                dwError = LwStrDupOrNull((PCSTR) domainComp, &domain);
                ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
            }

            if (DoesStrStartWith(domain, appContext->modifyConn.domainName, 1)) {
                appContext->workConn = &(appContext->modifyConn);
            }
            else {
                appContext->workConn = &(appContext->searchConn);
            }
        }
        else {
            if(IsDNComp(*name)) {
                dwError = LwStrDupOrNull((PCSTR) *name, &tmp);
                ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

                LwStrToLower(tmp);
                p = strstr((PCSTR) tmp, "dc=");

                if(p) {
                    dwError = GetDomainFromDN(*name, &domain);
                    ADT_BAIL_ON_ERROR_NP(dwError);

                    if (DoesStrStartWith(domain, appContext->modifyConn.domainName, 1)) {
                        appContext->workConn = &(appContext->modifyConn);
                    }
                    else {
                        appContext->workConn = &(appContext->searchConn);
                    }
                }
            }
            else {
                appContext->workConn = &(appContext->modifyConn);
            }
        }
    }

    cleanup:
        LW_SAFE_FREE_MEMORY(tmp);
        LW_SAFE_FREE_MEMORY(domain);
        LW_SAFE_FREE_MEMORY(domainComp);
        return;

    error:
        goto cleanup;
}

/**
 * Open second connection for multi-forest searches if the name starts with
 * domain name or it is a UPN.
 *
 * @param action Action reference.
 * @param name Domain name or DN/RDN.
 * return 0 on success; error code on failure.
 */
DWORD OpenADSearchConnectionDomain(IN AdtActionTP action, IN OUT PSTR *name)
{
    DWORD dwError = ERROR_SUCCESS;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR domain = NULL;
    PSTR domainComp = NULL;
    PSTR tmp = NULL;
    PSTR p = NULL;
    INT len = 0;

    if(appContext->searchConn.conn || (*name == NULL)) {
        goto cleanup;
    }

    appContext->workConn = &(appContext->searchConn);

    if (IsUPN(*name)) {
        domain = GetRealmComp(*name);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(domain);

        if(domain == NULL) {
            dwError = LwStrDupOrNull(*name, &domain);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        if (!DoesStrStartWith(domain, appContext->modifyConn.domainName, 1)) {

            dwError = ProcessConnectArgs(appContext,
                                         NULL,
                                         appContext->copts.port,
                                         domain);
            ADT_BAIL_ON_ERROR_NP(dwError);

            dwError = ConnectAD(appContext);
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
    }
    else {
        if (IsBackSlashPresent(*name)) {
            p = strstr((PCSTR) *name, "\\");

            if (p) {
                len = p - *name;
                dwError = LwAllocateMemory(sizeof(CHAR) * (len + 1),
                                           OUT_PPVOID(&domainComp));
                ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

                strncpy(domainComp, (PCSTR) *name, len);
            }

            if (!IsDotPresent(domainComp)) {
                p = strstr((PCSTR) appContext->modifyConn.domainName, ".");

                dwError
                        = LwAllocateStringPrintf(&domain, "%s%s", domainComp, p);
                ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
            }
            else {
                dwError = LwStrDupOrNull((PCSTR) domainComp, &domain);
                ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
            }

            if (!DoesStrStartWith(domain, appContext->modifyConn.domainName, 1)) {
                dwError = ProcessConnectArgs(appContext,
                                             NULL,
                                             appContext->copts.port,
                                             domain);
                ADT_BAIL_ON_ERROR_NP(dwError);

                dwError = ConnectAD(appContext);
                ADT_BAIL_ON_ERROR_NP(dwError);
            }

            // Strip domain prefix from the name
            p = strstr((PCSTR) *name, "\\");

            dwError = LwStrDupOrNull((PCSTR) (p + 1), &tmp);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            LW_SAFE_FREE_MEMORY(*name);
            *name = tmp;
        }
        else {
            return OpenADSearchConnectionDN(action, name);
        }
    }

    cleanup:
        LW_SAFE_FREE_MEMORY(domain);
        LW_SAFE_FREE_MEMORY(domainComp);
        appContext->workConn = & (appContext->modifyConn);
        return dwError;

    error:
        goto cleanup;
}

/**
 * Base action's initialization method.
 *
 * @param action Action reference.
 * return 0 on success; error code on failure.
 */
DWORD InitBaseAction(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    dwError = ConnectAD(appContext);
    ADT_BAIL_ON_ERROR_NP(dwError);

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Base initialization method for actions that require netapi.
 *
 * @param action Action reference.
 * return 0 on success; error code on failure.
 */
DWORD InitBaseActionWithNetAPI(IN AdtActionTP action)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    if(appContext->aopts.isNonSec) {
        // Need credentials for netapi calls used in this action.
        PrintStderr(appContext,
                    LogLevelWarning,
                    "%s: Insecure access is not supported for this type of action. Will be authenticating via krb5 ticket instead.\n",
                    appContext->actionName);

        appContext->aopts.isNonSec = 0;
    }

    dwError = InitBaseAction(action);

    if(!dwError) {
       dwError = NetInitMemory();
    }

    return dwError;
}

/**
 * Base action's validate method.
 *
 * @param action Action reference.
 * return 0 on success; error code on failure.
 */
DWORD ValidateBaseAction(IN AdtActionTP action)
{
    DWORD dwError = 0;

    return dwError;
}


/**
 * Base action's execute method.
 *
 * @param action Action reference.
 * return 0 on success; error code on failure.
 */
DWORD ExecuteBaseAction(IN AdtActionTP action)
{
    DWORD dwError = 0;

    return dwError;
}

/**
 * Base action's clean up method.
 *
 * @param action Action reference.
 * return 0 on success; error code on failure.
 */
DWORD CleanUpBaseAction(IN AdtActionTP action)
{
    DWORD dwError = 0;

    return dwError;
}

/**
 * Base action's clean up method for cell-management actions.
 *
 * @param action Action reference.
 * return 0 on success; error code on failure.
 */
DWORD CleanUpBaseCellAction(IN AdtActionTP action)
{
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;

    if(appContext->execResult.base.returnCode == 500016) {
        PrintStderr(appContext, LogLevelError,
                    "The likely cause of the problem is that your AD does "
                    "not support RFC 2307 schema extensions. You should either "
                    "upgrade the schema or use the command in non-schema mode "
                    "(\"--non-schema\")\n");
    }

    return CleanUpBaseAction(action);
}

/**
 * Base clean up method for actions that require netapi.
 *
 * @param action Action reference.
 * return 0 on success; error code on failure.
 */
DWORD CleanUpBaseWithNetAPIAction(IN AdtActionTP action)
{
    NetDestroyMemory();
    return CleanUpBaseAction(action);
}

/*************************************************************/
/*                    Shared methods                         */
/*************************************************************/

/**
 * Create cell group. Schema mode, default cell.
 */
static DWORD
AdtAddGroupToCellSDefaultCell(IN AdtActionTP action, PSTR groupDN) {
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    INT i, j;
    AttrValsT *avp = NULL;
    BOOL propSet = FALSE;

    if(groupDN == NULL) {
        groupDN = action->addToCell.group;
    }

    PrintStderr(appContext, LogLevelVerbose, "%s: Since this is the default cell - modifying properties of AD group...\n",
                appContext->actionName);

    dwError = LwAllocateMemory(2 * sizeof(AttrValsT), OUT_PPVOID(&avp));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    avp[0].attr = "gidNumber";

    dwError = GetObjectAttrs(appContext, groupDN, avp);
    if(dwError && IsMultiForestMode(action)) {
        SwitchConnection(action);
        dwError = GetObjectAttrs(appContext, groupDN, avp);
    }
    ADT_BAIL_ON_ERROR_NP(dwError);

    if (appContext->oGID) {
        propSet = TRUE;

        if(!avp[0].vals) {
            dwError = LwAllocateMemory(2 * sizeof(PSTR), OUT_PPVOID(&(avp[0].vals)));
            ADT_BAIL_ON_ALLOC_FAILURE(!dwError);
        }
        else {
            LW_SAFE_FREE_MEMORY(avp[0].vals[0]);
        }

        dwError = LwStrDupOrNull((PCSTR) appContext->oGID, &(avp[0].vals[0]));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if(propSet) {
        dwError = ModifyADObject(appContext, groupDN, avp, 2);
        if(dwError && IsMultiForestMode(action)) {
            SwitchConnection(action);
            dwError = ModifyADObject(appContext, groupDN, avp, 2);
        }
        ADT_BAIL_ON_ERROR_NP(dwError);

        PrintStderr(appContext, LogLevelVerbose, "%s: Modifying properties of cell group - done \n",
                    appContext->actionName);
    }
    else {
        PrintStderr(appContext, LogLevelVerbose, "%s: Properties of the cell group do not need to be changed\n",
                    appContext->actionName);
    }

    if(appContext->gopts.isPrintDN) {
        PrintResult(appContext, LogLevelNone, "%s\n", groupDN);
    }
    else {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "Group %s has been added to PowerBroker cell\n", groupDN);
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

        return dwError;

    error:
        goto cleanup;
}

/**
 * Create cell group. (Schema mode)
 *
 * @param action Action reference.
 * @param odn Group DN
 * @param oid SID of the AD group
 * @param ogid Group ID
 * @param oname Name of the group
 * @return 0 - on success; error code on failure.
 */
DWORD AdtAddGroupToCellS(IN AdtActionTP action,
                         IN PSTR odn,
                         IN PSTR oid,
                         IN PSTR ogid,
                         IN PSTR oname,
                         IN PSTR groupDN)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR guidStr = NULL;

    if (appContext->isDefaultCell) {
        return AdtAddGroupToCellSDefaultCell(action, groupDN);
    }

    dwError = LwAllocateStringPrintf(&guidStr, "backLink=%s", oid);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    PSTR desc[] = {
        guidStr,
        "objectClass=centerisLikewiseGroup",
        NULL
    };

    PSTR gid[] = { ogid, NULL };

//    PSTR cnu[] = { oname, NULL };

    AttrValsT avp[] = {
        { "objectClass", GetObjectClassVals(ObjectClassCellGroup) },
 //       { "cn", cnu },
 //       { "name", cnu },
        { "keywords", desc },
        { "gidNumber", gid },
        /*
        { "displayName", empty },
        { "description", empty },
        */
        { NULL, NULL }
    };

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating object %s ...\n",
                appContext->actionName, odn);

    SwitchToModifyConnection(action);
    dwError = CreateADObject(appContext, odn, avp);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating object - done \n",
                appContext->actionName);

    if(appContext->gopts.isPrintDN) {
        PrintResult(appContext, LogLevelNone, "%s\n", action->addToCell.dn);
    }
    else {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "Group has been added to PowerBroker cell\n");
        }
    }

    cleanup:
        LW_SAFE_FREE_MEMORY(guidStr);
        return dwError;

    error:
        goto cleanup;
}

/**
 * Create cell group. (Non-schema mode)
 *
 * @param action Action reference.
 * @param odn Group DN
 * @param oid SID of the AD group
 * @param ogid Group ID
 * @param oname Name of the group
 * @return 0 - on success; error code on failure.
 */
DWORD AdtAddGroupToCellNS(IN AdtActionTP action,
                          IN PSTR odn,
                          IN PSTR oid,
                          IN PSTR ogid,
                          IN PSTR oname)
{
    DWORD dwError = 0;
    AppContextTP appContext = (AppContextTP) ((AdtActionBaseTP) action)->opaque;
    PSTR guidStr = NULL;
    PSTR gidNumStr = NULL;

    dwError = LwAllocateStringPrintf(&guidStr, "%s=%s", "backLink", oid);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    dwError = LwAllocateStringPrintf(&gidNumStr, "gidNumber=%s", ogid);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    PSTR desc[] = {
        guidStr,
        "objectClass=centerisLikewiseGroup",
        gidNumStr,
        "description=",
        "displayName=",
        NULL
    };

//    PSTR cnu[] = { oname, NULL };

    AttrValsT avp[] = {
        { "objectClass", GetObjectClassVals(ObjectClassCellGroupNS) },
//        { "cn", cnu },
//        { "name", cnu },
        { "keywords", desc },
        { NULL, NULL }
    };

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating object %s ...\n",
                appContext->actionName, odn);

    SwitchToModifyConnection(action);
    dwError = CreateADObject(appContext, odn, avp);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelVerbose, "%s: Creating object - done \n",
                appContext->actionName);

    if(appContext->gopts.isPrintDN) {
        PrintResult(appContext, LogLevelNone, "%s\n", odn);
    }
    else {
        if(!appContext->gopts.isQuiet) {
            PrintResult(appContext, LogLevelNone, "Group has been added to PowerBroker cell\n");
        }
    }

    cleanup:
        LW_SAFE_FREE_MEMORY(guidStr);
        return dwError;

    error:
        goto cleanup;
}


