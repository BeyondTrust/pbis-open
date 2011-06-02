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
 *        ldap.c
 *
 * Abstract:
 *
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Apr 2, 2010
 *
 */

#include "includes.h"

//static PCSTR ATTR_TIME_FORMAT = "%Y%m%d%H%M%S";
static PCSTR ATTR_TIME_FORMAT = "%m/%d/%Y %H:%M:%S %Z";

static LDAPMessage*
adt_ldap_msg_append(LDAPMessage *tail, LDAPMessage *entry) {
    if(tail == NULL) {
        return entry->lm_chain_tail;
    }

    tail->lm_chain = entry;

    return entry->lm_chain_tail;
}

static DWORD
adt_ldap_search_ext_s(
    LDAP *ld,
    LDAP_CONST char *base,
    int scope,
    LDAP_CONST char *filter,
    char **attrs,
    int attrsonly,
    LDAPControl **sctrls,
    LDAPControl **cctrls,
    struct timeval *timeout,
    int sizelimit,
    LDAPMessage **res )
{
    DWORD dwError = 0;
    BOOL morePages = FALSE;
    int errCode = 0;
    int pageNum;
    unsigned long pageSize = 10000;
    struct berval *cookie = NULL;
    CHAR pagingCriticality = 'T';
    ber_int_t totalCount;

    LDAPMessage *result = NULL;
    LDAPMessage *head = NULL;
    LDAPMessage *tail = NULL;

    LDAPControl *pageControl = NULL;
    LDAPControl **returnedControls = NULL;
    LDAPControl *controls[2] = {
        NULL,
        NULL
    };

    do {
        dwError = ldap_create_page_control(ld,
                                           pageSize,
                                           cookie,
                                           pagingCriticality,
                                           &pageControl);
        ADT_BAIL_ON_ERROR_NP(dwError);

        controls[0] = pageControl;

        dwError = ldap_search_ext_s(ld,
                                    base,
                                    scope,
                                    filter,
                                    attrs,
                                    attrsonly,
                                    controls,
                                    NULL,
                                    timeout,
                                    sizelimit,
                                    &result);

        if ((dwError != LDAP_SUCCESS) & (dwError != LDAP_PARTIAL_RESULTS)) {
            ADT_BAIL_ON_ERROR_NP(dwError);
        }

        if(head == NULL) {
            head = result;
        }

        dwError = ldap_parse_result(ld,
                                    result,
                                    &errCode,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &returnedControls,
                                    FALSE);
        ADT_BAIL_ON_ERROR_NP(dwError);

        if (cookie != NULL) {
            ber_bvfree(cookie);
            cookie = NULL;
        }

        if (returnedControls == NULL) {
            break;
        }

        dwError = ldap_parse_page_control(ld,
                                          returnedControls,
                                          &totalCount,
                                          &cookie);

        ADT_BAIL_ON_ERROR_NP(dwError);

        if (cookie && (cookie->bv_val != NULL) && (strlen(cookie->bv_val) > 0)) {
            morePages = TRUE;
        }
        else {
            morePages = FALSE;
        }

        if (returnedControls != NULL) {
            ldap_controls_free(returnedControls);
            returnedControls = NULL;
        }

        controls[0] = NULL;
        ldap_control_free(pageControl);
        pageControl = NULL;

        tail = adt_ldap_msg_append(tail, result);

        pageNum = pageNum + 1;

    }
    while (morePages == TRUE);

    *res = head;

    cleanup:
        if (cookie != NULL) {
            ber_bvfree(cookie);
        }

        if (returnedControls != NULL) {
            ldap_controls_free(returnedControls);
        }

        if(pageControl != NULL) {
            ldap_control_free(pageControl);
        }

        return dwError;

    error:
        *res = NULL;
        goto cleanup;
}

/**
 * ObjectClass values.
 */
static ObjectClassAttrValT sObjClass[] = {
    {
        ObjectClassUser,
        { "top", "person", "organizationalPerson", "user", NULL }
    },
    {
        ObjectClassComputer,
        { "top", "person", "organizationalPerson", "user", "computer", NULL }
    },
    {
        ObjectClassGroup,
        { "top", "group", NULL }
    },
    {
        ObjectClassOU,
        { "top", "organizationalUnit", NULL }
    },
    {
        ObjectClassCell,
        { "top", "container", NULL }
    },
    {
        ObjectClassCellUsers,
        { "top", "container", NULL }
    },
    {
        ObjectClassCellGroups,
        { "top", "container", NULL }
    },
    {
        ObjectClassCellUser,
        { "top", "posixAccount", "leaf", "connectionPoint", "serviceConnectionPoint", NULL }
    },
    {
        ObjectClassCellGroup,
        { "top", "posixGroup", "leaf", "connectionPoint", "serviceConnectionPoint", NULL }
    },
    {
        ObjectClassCellUserNS,
        { "top", "leaf", "connectionPoint", "serviceConnectionPoint", NULL }
    },
    {
        ObjectClassCellGroupNS,
        { "top", "leaf", "connectionPoint", "serviceConnectionPoint", NULL }
    },
    {
        ObjectClassAny,
        {NULL}
    }
};

/**
 * Format values of known attributes.
 *
 * @param aname Attributes name.
 * @param vals Attributes values.
 * @param ovals Formatted values.
 * @return 0 on success; error code on failure.
 */
static DWORD
FormatKnownAttributes(IN AppContextTP appContext,
                      IN PCSTR aname,
                      IN PSTR *vals,
                      IN OUT PSTR *ovals)
{
    DWORD dwError = 0;
    unsigned long long int adtime = 0;
    time_t winTime = 0;
    struct tm localTime = {0};
    INT i;

    if(!strcmp("objectGUID", (PCSTR) aname)) {
        dwError = Guid2Str((PVOID) vals[0], &(ovals[0]));
        ADT_BAIL_ON_ERROR_NP(dwError);
    }
    else {
        if(!strcmp("objectSid", (PCSTR) aname)) {
            dwError = Sid2Str((PVOID) vals[0], &(ovals[0]));
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
        else {
            if(
                !strcmp("badPasswordTime", (PCSTR) aname) ||
                !strcmp("lastLogoff", (PCSTR) aname) ||
                !strcmp("lastLogon", (PCSTR) aname) ||
                !strcmp("pwdLastSet", (PCSTR) aname) ||
                !strcmp("accountExpires", (PCSTR) aname) ||
                !strcmp("lastLogonTimestamp", (PCSTR) aname) ||
                !strcmp("lockoutTime", (PCSTR) aname)
            ) {
                for (i = 0; vals[i] != NULL; ++i) {
                    if (appContext->rawTime) {
                        dwError = LwStrDupOrNull(vals[i], &(ovals[i]));
                        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
                    }
                    else {

                    dwError = Str2Ull(vals[i], 10, &adtime);
                        ADT_BAIL_ON_ERROR_NP(dwError);

                        winTime = LwNtTimeToWinTime(adtime);

                        if (winTime == 0) {
                            dwError = LwStrDupOrNull("01/01/1601 00:00:00 UNC",
                                                     &(ovals[i]));
                            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
                        }
                        else {
                            localtime_r(&winTime, &localTime);

                            dwError = LwAllocateMemory(128 * sizeof(char),
                                                       (PVOID) &(ovals[i]));
                            ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

                            strftime(ovals[i],
                                     127,
                                     ATTR_TIME_FORMAT,
                                     &localTime);
                        }
                    }
                }
            }
            else {
                if (!strcmp("whenCreated", (PCSTR) aname) ||
                    !strcmp("whenChanged", (PCSTR) aname) ||
                    !strcmp("dSCorePropagationData", (PCSTR) aname)
                ) {
                    for (i = 0; vals[i] != NULL; ++i) {
                        if (appContext->rawTime) {
                            dwError = LwStrDupOrNull(vals[i], &(ovals[i]));
                            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
                        }
                        else {
                            //“yyyyMMddHHmmss.0Z
                            dwError
                                = LwAllocateStringPrintf(&(ovals[i]),
                                                         "%c%c/%c%c/%c%c%c%c %c%c:%c%c:%c%c GMT",
                                                         vals[i][4],
                                                         vals[i][5],
                                                         vals[i][6],
                                                         vals[i][7],
                                                         vals[i][0],
                                                         vals[i][1],
                                                         vals[i][2],
                                                         vals[i][3],
                                                         vals[i][8],
                                                         vals[i][9],
                                                         vals[i][10],
                                                         vals[i][11],
                                                         vals[i][12],
                                                         vals[i][13]);
                            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
                        }
                    }
                }
                else {
                    for (i = 0; vals[i] != NULL; i++) {
                        if (IsPrintable(vals[i])) {
                            dwError = LwStrDupOrNull((PCSTR) vals[i],
                                                     &(ovals[i]));
                        }
                        else {
                            dwError = LwStrDupOrNull("<BINARY DATA>",
                                                     &(ovals[i]));
                        }
                        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
                    }
                }
            }
        }
    }

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Get values of the ObjectClass attribute.
 *
 * @param class Object class
 * @return Object class values.
 */
PSTR* GetObjectClassVals(ObjectClassT class) {
    INT i;

    for(i = 0; sObjClass[i].obj != ObjectClassAny; ++i) {
        if(sObjClass[i].obj == class) {
            return sObjClass[i].vals;
        }
    }

    return sObjClass[i].vals;
}

/**
 * Search for children of the provided object that are of the specified type.
 *
 * @param appContext Application context reference.
 * @param class Object class of the children
 * @param rdn Object's DN or RDN
 * @param overrideFilter Optional filter
 * @param scope Search scope
 * @param outDNs Null terminated array of all children. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
DWORD
SearchForObject(IN AppContextTP appContext, IN ObjectClassT class, IN INT scope, IN PSTR overrideFilter, IN PSTR rdn, OUT PSTR **outDNs) {
    DWORD dwError = 0;
    LDAP *ld = (LDAP *) appContext->workConn->conn;
    LDAPMessage *res = NULL, *e = NULL;
    PCSTR obClass = NULL;
    PSTR fqdn = rdn;
    INT count = 0;
    PSTR dn = NULL;
    PSTR *child = NULL;;
    INT i;
    PSTR filter = NULL;
    PSTR attrs[] = {
        "distinguishedName",
        NULL };

    switch(class) {
        case (ObjectClassUser):       /* class: user; id: samAccountName */
            obClass = "user";
            break;

        case (ObjectClassComputer):   /* class: computer; id: samAccountName$ */
            obClass = "computer";
            break;

        case (ObjectClassGroup):      /* class: group; id: samAccountName */
            obClass = "group";
            break;

        case (ObjectClassOU):         /* class: organizationalUnit; id: ou */
            obClass = "organizationalUnit";
            break;

        case (ObjectClassCell):       /* class: container; id: cn=$LikewiseIdentityCel */
        case (ObjectClassCellUsers):  /* class: container; id: cn=Users (child of ObjectClassCell) */
        case (ObjectClassCellGroups): /* class: container; id: cn=Groups (child of ObjectClassCell) */
            obClass = "container";
            break;

        case (ObjectClassCellUser): /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellUsers) */
        case (ObjectClassCellUserNS):   /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellUsers) */
        case (ObjectClassCellGroup):  /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellGroups) */
        case (ObjectClassCellGroupNS):  /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellGroups) */
            obClass = "serviceConnectionPoint";
            break;

        case (ObjectClassAny):        /* class: *; id: cn */
        default:
            obClass = "top";
            break;
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Searching for %s object ...\n",
                appContext->actionName, obClass);

    if(!overrideFilter) {
        dwError = LwAllocateStringPrintf(&filter, "(objectclass=%s)", obClass);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }
    else {
        filter = overrideFilter;
    }

    if(!DoesStrEndWith(rdn, appContext->workConn->defaultNC, 1)) {
        dwError = LwAllocateStringPrintf(&fqdn, "%s,%s", rdn, appContext->workConn->defaultNC);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Using search filter %s and search-base %s\n",
                appContext->actionName, filter, fqdn);

    if(scope == LDAP_SCOPE_BASE) {
        dwError = ldap_search_ext_s(ld, fqdn, scope,
                                        filter, attrs, 0, NULL, NULL, LDAP_NO_LIMIT,
                                        LDAP_NO_LIMIT, &res);
    }
    else {
        dwError = adt_ldap_search_ext_s(ld, fqdn, scope,
                                        filter, attrs, 0, NULL, NULL, LDAP_NO_LIMIT,
                                        LDAP_NO_LIMIT, &res);
    }

    if ((dwError != LDAP_SUCCESS) || (res == NULL)) {
        dwError += ADT_LDAP_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    count = ldap_count_entries(ld, res);

    if(!count) {
        *outDNs = NULL;
        goto cleanup;
    }

    dwError = LwAllocateMemory((count + 1) * sizeof(PSTR), OUT_PPVOID(&child));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    for (i = 0, e = ldap_first_entry(ld, res); e != NULL; e = ldap_next_entry(ld, e), ++i) {

        if ((dn = ldap_get_dn(ld, e)) != NULL) {
            dwError = LwStrDupOrNull((PCSTR) dn, &(child[i]));
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
            ldap_memfree(dn);
        }
    }
    child[i] = NULL;

    *outDNs = child;

    PrintStderr(appContext, LogLevelTrace, "%s: Done searching for %s object\n",
                appContext->actionName, obClass);

    cleanup:
        if (res) {
            ldap_msgfree(res);
        }

        if(!overrideFilter) {
            LW_SAFE_FREE_MEMORY(filter);
        }

        return dwError;

    error:
        if(child) {
            for(i = 0; child[i]; ++i) {
                LW_SAFE_FREE_MEMORY(child[i]);
            }

            LW_SAFE_FREE_MEMORY(child);;
        }
        goto cleanup;
}

/**
 * Get DNs of all children of the provided object that are of the specified type.
 *
 * @param appContext Application context reference.
 * @param class Object class of the children
 * @param rdn Object's DN or RDN
 * @param outDNs Null terminated array of all children. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
DWORD GetChildren(IN AppContextTP appContext, IN ObjectClassT class, IN PSTR rdn, OUT PSTR **outDNs) {
    DWORD dwError;

    PrintStderr(appContext, LogLevelTrace, "%s: Getting children of node: %s ...\n",
                appContext->actionName, rdn);

    dwError = SearchForObject(appContext, class, LDAP_SCOPE_ONELEVEL, NULL, rdn, outDNs);

    PrintStderr(appContext, LogLevelTrace, "%s: Done getting children.\n",
                appContext->actionName);

    return dwError;
}

/**
 * Resolve DN of the object.
 *
 * @param appContext Application context reference.
 * @param class Object class
 * @param rdn Object's DN or RDN
 * @param outDN Resolved DN or NULL on failure. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
DWORD ResolveDN(IN AppContextTP appContext, IN ObjectClassT class, IN PSTR rdn, OUT PSTR *outDN) {
    DWORD dwError = 0;
    LDAP *ld = (LDAP *) appContext->workConn->conn;
    LDAPMessage *res = NULL, *e = NULL;
    PSTR filter = NULL;
    PCSTR obClass = NULL;
    PSTR fqdn = rdn;
    PSTR dn = NULL;
    PSTR attrs[] = {
        "distinguishedName",
        NULL };

    PrintStderr(appContext, LogLevelTrace, "%s: Resolving RDN: %s ...\n",
                appContext->actionName, rdn);

    switch(class) {
        case (ObjectClassUser):       /* class: user; id: samAccountName */
            obClass = "user";
            break;

        case (ObjectClassComputer):   /* class: computer; id: samAccountName$ */
            obClass = "computer";
            break;

        case (ObjectClassGroup):      /* class: group; id: samAccountName */
            obClass = "group";
            break;

        case (ObjectClassOU):         /* class: organizationalUnit; id: ou */
            obClass = "organizationalUnit";
            break;

        case (ObjectClassCell):       /* class: container; id: cn=$LikewiseIdentityCel */
        case (ObjectClassCellUsers):  /* class: container; id: cn=Users (child of ObjectClassCell) */
        case (ObjectClassCellGroups): /* class: container; id: cn=Groups (child of ObjectClassCell) */
            obClass = "container";
            break;

        case (ObjectClassCellUser):     /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellUsers) */
        case (ObjectClassCellGroup):    /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellGroups) */
        case (ObjectClassCellUserNS):   /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellUsers) */
        case (ObjectClassCellGroupNS):  /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellGroups) */
            obClass = "serviceConnectionPoint";
            break;

        case (ObjectClassAny):        /* class: *; id: cn */
        default:
            obClass = "top";
            break;
    }

    dwError = LwAllocateStringPrintf(&filter, "(objectclass=%s)", obClass);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    if(!rdn || !rdn[0]) {
        dwError = LwAllocateStringPrintf(&fqdn, "%s", appContext->workConn->defaultNC);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }
    else {
        if(!DoesStrEndWith(rdn, appContext->workConn->defaultNC, 1)) {
            dwError = LwAllocateStringPrintf(&fqdn, "%s,%s", rdn, appContext->workConn->defaultNC);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Resolving RDN: %s in context %s ...\n",
                appContext->actionName, rdn, fqdn);

    dwError = ldap_search_ext_s(ld, fqdn, LDAP_SCOPE_BASE,
                                filter, attrs, 0, NULL, NULL, LDAP_NO_LIMIT,
                                LDAP_NO_LIMIT, &res);

    if ((dwError != LDAP_SUCCESS) || (res == NULL)) {
        dwError += ADT_LDAP_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    e = ldap_first_entry(ld, res);

    if (!e) {
        dwError = ADT_ERR_FAILED_AD_OBJ_NOT_FOUND;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dn = ldap_get_dn(ld, res);

    if(!dn) {
        dwError = ADT_ERR_FAILED_AD_OBJ_NOT_FOUND;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = LwStrDupOrNull(dn, outDN);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    e = ldap_next_entry(ld, res);
    if (e) {
        dwError = ADT_ERR_FAILED_AD_MULTIPLE_FOUND;
        LW_SAFE_FREE_MEMORY(*outDN);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: RDN resolved: %s\n",
                appContext->actionName, dn);

    cleanup:
        if (dn) {
            ldap_memfree(dn);
        }

        if (res) {
            ldap_msgfree(res);
        }

        LW_SAFE_FREE_MEMORY(filter);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Find object in AD.
 *
 * @param appContext Application context reference.
 * @param class Object class
 * @param inName Identification component
 * @param outDN Resolved DN or NULL on failure. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
DWORD LocateADObject(IN AppContextTP appContext, IN ObjectClassT class, IN PSTR inName, OUT PSTR *outDN) {
    DWORD dwError = 0;
    LDAP *ld = (LDAP *) appContext->workConn->conn;
    LDAPMessage *res = NULL, *e = NULL;
    PSTR filter = NULL;
    PCSTR obClass = NULL;
    PCSTR idComp = NULL;
    PCSTR extras = "";
    PSTR name = inName;
    PSTR dn = NULL;
    PSTR extraCheck = NULL;
    PSTR attrs[] = {
        "distinguishedName",
        NULL };

    PrintStderr(appContext, LogLevelTrace, "%s: Searching for object %s ...\n",
                appContext->actionName, inName);

    switch(class) {
        case (ObjectClassUser):       /* class: user; id: samAccountName */
            obClass = "user";
            idComp = "samAccountName";
            name = GetNameComp(inName);
            break;

        case (ObjectClassComputer):   /* class: computer; id: samAccountName$ */
            obClass = "computer";
            idComp = "samAccountName";
            extras = "$";
            break;

        case (ObjectClassGroup):      /* class: group; id: samAccountName */
            obClass = "group";
            idComp = "samAccountName";
            break;

        case (ObjectClassOU):         /* class: organizationalUnit; id: ou */
            obClass = "organizationalUnit";
            idComp = "ou";
            break;

        case (ObjectClassCell):       /* class: container; id: cn=$LikewiseIdentityCel */
            obClass = "container";
            idComp = "cn";
            name = "$LikewiseIdentityCel";
            break;

        case (ObjectClassCellUsers):  /* class: container; id: cn=Users (child of ObjectClassCell) */
            obClass = "container";
            idComp = "cn";
            name = "Users";
            break;

        case (ObjectClassCellGroups): /* class: container; id: cn=Groups (child of ObjectClassCell) */
            obClass = "container";
            idComp = "cn";
            name = "Groups";
            break;

        case (ObjectClassCellUser):   /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellUsers) */
        case (ObjectClassCellUserNS): /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellUsers) */
            obClass = "serviceConnectionPoint";
            idComp = "cn";
            break;

        case (ObjectClassCellGroup):  /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellGroups) */
        case (ObjectClassCellGroupNS):  /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellGroups) */
            obClass = "serviceConnectionPoint";
            idComp = "cn";
            break;

        case (ObjectClassAny):        /* class: *; id: cn */
        default:
            obClass = "*";
            idComp = "cn";
            break;
    }

    dwError = LwAllocateStringPrintf(&filter, "(&(objectclass=%s)(%s=%s%s))", obClass, idComp, name, extras);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    PrintStderr(appContext, LogLevelTrace, "%s: Searching for object %s in naming context %s ...\n",
                appContext->actionName, inName, appContext->workConn->defaultNC);

    dwError = adt_ldap_search_ext_s(ld, appContext->workConn->defaultNC, LDAP_SCOPE_SUBTREE,
                                filter, attrs, 0, NULL, NULL, LDAP_NO_LIMIT,
                                LDAP_NO_LIMIT, &res);

    if ((dwError != LDAP_SUCCESS) || (res == NULL)) {
        dwError += ADT_LDAP_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    e = ldap_first_entry(ld, res);

    if (!e) {
        dwError = ADT_ERR_FAILED_AD_OBJ_NOT_FOUND;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dn = ldap_get_dn(ld, res);

    if(!dn) {
        dwError = ADT_ERR_FAILED_AD_OBJ_NOT_FOUND;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    switch(class) {
        case (ObjectClassCellUsers):  /* class: container; id: cn=Users (child of ObjectClassCell) */
            if(!DoesStrStartWith(dn, "CN=Users,CN=$LikewiseIdentityCell", 1)) {
                dwError = ADT_ERR_FAILED_AD_OBJ_NOT_FOUND;
            }
            break;

        case (ObjectClassCellGroups): /* class: container; id: cn=Groups (child of ObjectClassCell) */
            if(!DoesStrStartWith(dn, "CN=Groups,CN=$LikewiseIdentityCell", 1)) {
                dwError = ADT_ERR_FAILED_AD_OBJ_NOT_FOUND;
            }
            break;

        case (ObjectClassCellUser):   /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellUsers) */
        case (ObjectClassCellUserNS):   /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellUsers) */
            dwError = LwAllocateStringPrintf(&extraCheck, "CN=%s,%s", name, "CN=Users,CN=$LikewiseIdentityCell");
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
            if(!DoesStrStartWith(dn, extraCheck, 1)) {
                dwError = ADT_ERR_FAILED_AD_OBJ_NOT_FOUND;
            }
            break;

        case (ObjectClassCellGroup):  /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellGroups) */
        case (ObjectClassCellGroupNS):  /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellGroups) */
            dwError = LwAllocateStringPrintf(&extraCheck, "CN=%s,%s", name, "CN=Groups,CN=$LikewiseIdentityCell");
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
            if(!DoesStrStartWith(dn, extraCheck, 1)) {
                dwError = ADT_ERR_FAILED_AD_OBJ_NOT_FOUND;
            }
            break;

        default:
            break;
    }

    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = LwStrDupOrNull(dn, outDN);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    e = ldap_next_entry(ld, res);
    if (e) {
        dwError = ADT_ERR_FAILED_AD_MULTIPLE_FOUND;
        LW_SAFE_FREE_MEMORY(*outDN);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Found %s\n",
                appContext->actionName, dn);


    cleanup:
        if (dn) {
            ldap_memfree(dn);
        }

        if (res) {
            ldap_msgfree(res);
        }

        LW_SAFE_FREE_MEMORY(filter);
        if(class == ObjectClassUser) {
            LW_SAFE_FREE_MEMORY(name);
        }
        LW_SAFE_FREE_MEMORY(extraCheck);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Get various properties of the root DSE.
 *
 * @param appContext Application context reference.
 * @return 0 on success; error code on failure.
 */
DWORD GetRootDseProps(IN AppContextTP appContext)
{
    DWORD dwError = 0;
    LDAP *ld = (LDAP *) appContext->workConn->conn;
    LDAPMessage *res = NULL, *e;
    PSTR realm = NULL;

    PSTR attrs[] = {
        "defaultNamingContext",
        "ldapServiceName",
        "isGlobalCatalogReady",
        "dnsHostName",
        NULL };
    PSTR *values = NULL;

    PrintStderr(appContext, LogLevelTrace, "%s: Getting default naming context and AD SPN ... \n",
                appContext->actionName);

    dwError = ldap_search_ext_s(ld, "", LDAP_SCOPE_BASE, NULL, attrs, 0, NULL,
                                NULL, LDAP_NO_LIMIT, LDAP_NO_LIMIT, &res);

    if (dwError != LDAP_SUCCESS) {
        dwError = ADT_ERR_FAILED_AD_GET_DEFAULT_NC;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    e = ldap_first_entry(ld, res);

    if (!e) {
        dwError = ADT_ERR_FAILED_AD_GET_DEFAULT_NC;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    values = ldap_get_values(ld, e, "defaultNamingContext");

    if (!values) {
        dwError = ADT_ERR_FAILED_AD_GET_DEFAULT_NC;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    if(appContext->workConn->defaultNC) {
        LW_SAFE_FREE_MEMORY(appContext->workConn->defaultNC);
    }

    dwError = LwStrDupOrNull(values[0], &(appContext->workConn->defaultNC));
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    PrintStderr(appContext, LogLevelTrace, "%s: Default naming context is %s\n",
                appContext->actionName, appContext->workConn->defaultNC);

    if(!appContext->workConn->serverName) {
        ldap_value_free(values);

        values = ldap_get_values(ld, e, "dnsHostName");

        if (!values || !values[0]) {
            PrintStderr(appContext, LogLevelWarning, "%s: Failed to get AD server DNS name. Continuing ...\n",
                        appContext->actionName);
        }

        PrintStderr(appContext, LogLevelTrace, "%s: AD server DNS name is %s\n",
                    appContext->actionName, values[0]);

        dwError = LwStrDupOrNull(values[0], &(appContext->workConn->serverName));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if(appContext->workConn->domainName) {
        LwStrToUpper(appContext->workConn->domainName);
    }
    else {
        ldap_value_free(values);

        values = ldap_get_values(ld, e, "ldapServiceName");

        if (!values || !values[0]) {
            dwError = ADT_ERR_FAILED_AD_GET_LDAP_SPN;
            ADT_BAIL_ON_ERROR_NP(dwError);
        }

        PrintStderr(appContext, LogLevelTrace, "%s: AD SPN is %s\n",
                    appContext->actionName, values[0]);

        realm = GetRealmComp(values[0]);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(realm);

        if(realm == NULL) {
            dwError = LwStrDupOrNull(values[0], &realm);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        LwStrToUpper(realm);

        dwError = LwStrDupOrNull(realm, &(appContext->workConn->domainName));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    // Check if the server is global catalog ready.
    ldap_value_free(values);

    values = ldap_get_values(ld, e, "isGlobalCatalogReady");

    if (!values || !values[0]) {
        dwError = ADT_ERR_FAILED_AD_GET_LDAP_SPN;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: isGlobalCatalogReady: %s\n",
                appContext->actionName, values[0]);

    if(DoesStrStartWith(values[0], "TRUE", 1)) {
        appContext->workConn->isGCReady = 1;
    }
    else {
        appContext->workConn->isGCReady = 0;
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Krb5 realm is %s\n",
                appContext->actionName, appContext->workConn->domainName);

    cleanup:
        LW_SAFE_FREE_MEMORY(realm);

        if (values) {
            ldap_value_free(values);
        }

        if (res) {
            ldap_msgfree(res);
        }

        return dwError;

    error:
        goto cleanup;
}

/**
 * Connect to AD server.
 *
 * @param appContext Application context reference.
 * @return 0 on success; error code on failure.
 */
DWORD ConnectAD(IN AppContextTP appContext) {
    DWORD dwError = 0;
    int rc = 3;
    int debug = 0;
    int sizelimit = 0;

    //CloseAD(appContext);

    PrintStderr(appContext,
                LogLevelTrace,
                "%s: Connecting to %s:%d ...\n",
                appContext->actionName,
                appContext->workConn->serverAddress,
                appContext->workConn->port);

    appContext->workConn->conn
            = (LDAP *) ldap_open(appContext->workConn->serverAddress,
                                 appContext->workConn->port);

    if (!appContext->workConn->conn) {
        dwError = ADT_ERR_FAILED_AD_OPEN;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Connected\n",
                appContext->actionName);

    switch(appContext->gopts.logLevel) {
        case(1):
                debug = 3;
                break;

        case(2):
                debug = 4;
                break;

        case(3):
        case(4):
                debug = 6;
                break;

        case(5):
                debug = 7;
                break;

        default:
                debug = 0;
                break;
    }

    dwError = ldap_set_option((LDAP *) appContext->workConn->conn,
                              LDAP_OPT_DEBUG_LEVEL, &debug);

    if (dwError) {
        dwError = ADT_ERR_FAILED_AD_SET_OPT;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = ldap_set_option((LDAP *) appContext->workConn->conn,
                              LDAP_OPT_PROTOCOL_VERSION, &rc);

    if (dwError) {
        dwError = ADT_ERR_FAILED_AD_SET_VER;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: LDAP version set to %d\n",
                appContext->actionName, rc);

    dwError = ldap_set_option((LDAP *) appContext->workConn->conn, LDAP_OPT_REFERRALS,
                              (void *) LDAP_OPT_OFF);

    if (dwError) {
        dwError = ADT_ERR_FAILED_AD_SET_REF;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace,
                "%s: LDAP referral policy is set (do not chase)\n",
                appContext->actionName);

    dwError = ldap_set_option( (LDAP *) appContext->workConn->conn,
                               LDAP_OPT_SIZELIMIT,
                               (void *) &sizelimit );

    if (dwError) {
        dwError = ADT_ERR_FAILED_AD_SET_SIZE_LIMIT;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace,
                "%s: LDAP response size limit is set to 0 (unlimited)\n",
                appContext->actionName);

    dwError = ldap_set_option((LDAP *) appContext->workConn->conn,
                              LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL,
                              (void *) LDAP_OPT_ON);

    if (dwError) {
        dwError = ADT_ERR_FAILED_AD_SET_GSS;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace,
                "%s: LDAP bind method is set (GSS)\n", appContext->actionName);

    PrintStderr(appContext, LogLevelTrace, "%s: Authenticating ...\n",
                appContext->actionName);

    /* Bind anonymously first. */
    dwError = ldap_simple_bind_s((LDAP *) appContext->workConn->conn, NULL, NULL);

    if (dwError) {
        dwError = ADT_ERR_FAILED_AD_BIND;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = GetRootDseProps(appContext);
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!appContext->isCredsSet) {
        dwError = ProcessK5Creds(appContext);
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    if(appContext->aopts.isNonSec) {
        if(!appContext->aopts.logonAs || !appContext->aopts.password) {
            dwError = ADT_ERR_ARG_MISSING_CREDS_FOR_BIND;
            ADT_BAIL_ON_ERROR_NP(dwError);
        }

        PrintStderr(appContext, LogLevelTrace, "%s: Trying simple bind...\n",
                    appContext->actionName);

        dwError = ldap_bind_s((LDAP *) appContext->workConn->conn,
                              appContext->aopts.logonAs,
                              appContext->aopts.password,
                              LDAP_AUTH_SIMPLE);
    }
    else {
        PrintStderr(appContext,
                    LogLevelTrace,
                    "%s: Binding via GSS. Using krb5 cache file %s\n",
                    appContext->actionName,
                    getenv("KRB5CCNAME"));

        dwError = ldap_gssapi_bind_s((LDAP *) appContext->workConn->conn, NULL, NULL);
    }

    if(appContext->workConn == &(appContext->modifyConn)) {
        if (dwError) {
            dwError += ADT_LDAP_ERR_BASE;
            ADT_BAIL_ON_ERROR(dwError);
        }
    }
    else {
        if (dwError) {
            dwError = ldap_simple_bind_s((LDAP *) appContext->workConn->conn,
                                         NULL,
                                         NULL);

            if (dwError) {
                dwError = ADT_ERR_FAILED_AD_BIND;
                ADT_BAIL_ON_ERROR_NP(dwError);
            }

            PrintStderr(appContext,
                        LogLevelWarning,
                        "%s: Failed to bind via GSS on the connection to the second AD server. Binding anonymously instead!!!\n",
                        appContext->actionName);
        }
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Bind succeeded\n",
                appContext->actionName);

    cleanup:
        return dwError;

    error:
        //CloseAD(appContext->connAd);
        goto cleanup;
}

/**
 * Close connection to AD server.
 *
 * @param appContext Application context reference.
 * @param conn Connection.
 */
VOID CloseADConnection(IN AppContextTP appContext, ADServerConnectionTP conn)
{
    if (conn) {
        if (conn->conn) {

            PrintStderr(appContext,
                        LogLevelTrace,
                        "%s: Closing AD connection to %s (%s)\n",
                        appContext->actionName, conn->domainName, conn->serverAddress);

            ldap_unbind_s((LDAP *) conn->conn);
            conn->conn = NULL;

            PrintStderr(appContext,
                        LogLevelTrace,
                        "%s: Done\n",
                        appContext->actionName);
        }

        LW_SAFE_FREE_MEMORY(conn->defaultNC);
        LW_SAFE_FREE_MEMORY(conn->domainName);
        LW_SAFE_FREE_MEMORY(conn->serverAddress);
        LW_SAFE_FREE_MEMORY(conn->serverName);
    }

    return;
}

/**
 * Close connection to AD server.
 *
 * @param appContext Application context reference.
 */
VOID CloseADConnections(IN AppContextTP appContext)
{
    CloseADConnection(appContext, &(appContext->searchConn));
    CloseADConnection(appContext, &(appContext->modifyConn));
    appContext->workConn = &(appContext->modifyConn);
}

/**
 * Create a new object in AD.
 *
 * @param appContext Application context reference.
 * @param avp Attribute-values pairs.
 * @param dn Object's DN.
 * @return 0 on success; error code on failure.
 */
DWORD CreateADObject(IN AppContextTP appContext, IN PSTR dn, IN AttrValsT *avp)
{
    DWORD dwError = 0;
    LDAP *ld = (LDAP *) appContext->workConn->conn;
    LDAPMod **mods;
    INT length;
    INT i;

    PrintStderr(appContext, LogLevelTrace, "%s: Creating new object %s in naming context %s ...\n",
                appContext->actionName, dn, appContext->workConn->defaultNC);

    for (length = 0; avp[length].attr != NULL; ++length);

    if(length <= 1) {
        dwError = ADT_ERR_INVALID_FUN_ARG;
        ADT_BAIL_ON_ERROR(dwError);
    }

    /**
     * Construct the array of LDAPMod structures representing the attributes
     * of the new entry.
     **/
    dwError = LwAllocateMemory((length + 1) * sizeof(LDAPMod *), OUT_PPVOID(&mods));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    for (i = 0; i < length; ++i) {
        dwError = LwAllocateMemory(sizeof(LDAPMod), (PVOID *) &mods[i]);
        ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

        mods[i]->mod_op = 0;
        mods[i]->mod_type = (PSTR) avp[i].attr;
        mods[i]->mod_values = (PSTR *) avp[i].vals;
    }

    mods[i] = NULL;

    if(!appContext->gopts.isReadOnly) {
        /* Perform the add operation. */
        dwError = ldap_add_ext_s(ld, dn, mods, NULL, NULL);
        if(dwError) {
            dwError += ADT_LDAP_ERR_BASE;
        }

        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: New object created\n",
                appContext->actionName);

    cleanup:
        for(i = 0; i < length; ++i) {
            LW_SAFE_FREE_MEMORY(mods[i]);
        }

        LW_SAFE_FREE_MEMORY(mods);
        return dwError;

    error:
        goto cleanup;
}

/**
 * Modify attributes of an object in AD.
 *
 * @param appContext Application context reference.
 * @param avp Attribute-values pairs.
 * @param dn Object's DN.
 * @param opType Operation type: 0 - add; 2 - replace; 1 - delete.
 * @return 0 on success; error code on failure.
 */
DWORD
ModifyADObject(IN AppContextTP appContext, IN PSTR dn, IN AttrValsT *avp, IN INT opType)
{
    DWORD dwError = 0;
    LDAP *ld = (LDAP *) appContext->workConn->conn;
    LDAPMod **mods;
    INT length;
    INT i;

    PrintStderr(appContext, LogLevelTrace, "%s: Modifying attributes of object %s in naming context %s ...\n",
                appContext->actionName, dn, appContext->workConn->defaultNC);

    for (length = 0; avp[length].attr != NULL; ++length);

    if(length < 1) {
        dwError = ADT_ERR_INVALID_FUN_ARG;
        ADT_BAIL_ON_ERROR(dwError);
    }

    /**
     * Construct the array of LDAPMod structures representing the attributes
     * of the new entry.
     **/
    dwError = LwAllocateMemory((length + 1) * sizeof(LDAPMod *), OUT_PPVOID(&mods));
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    for (i = 0; i < length; ++i) {
        dwError = LwAllocateMemory(sizeof(LDAPMod), (PVOID *) &mods[i]);
        ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

        mods[i]->mod_op = opType;
        mods[i]->mod_type = (PSTR) avp[i].attr;
        mods[i]->mod_values = (PSTR *) avp[i].vals;
    }

    mods[i] = NULL;

    /* Perform the add operation. */
    if(!appContext->gopts.isReadOnly) {
        dwError = ldap_modify_ext_s(ld, dn, mods, NULL, NULL);
        if(dwError) {
            dwError += ADT_LDAP_ERR_BASE;
        }
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Attributes have been modified\n",
                appContext->actionName);

    cleanup:
        for(i = 0; i < length; ++i) {
            LW_SAFE_FREE_MEMORY(mods[i]);
        }

        LW_SAFE_FREE_MEMORY(mods);
        return dwError;

    error:
        goto cleanup;
}

/**
 * Delete an AD object.
 *
 * @param appContext Application context reference.
 * @param dn Object to delete.
 * @param isRecursive If != 0 - delete all children.
 * @return 0 on success; error code on failure.
 */
DWORD DeleteADObject(IN AppContextTP appContext, IN PSTR dn, IN INT isRecursive)
{
    DWORD dwError = 0;
    LDAP *ld = (LDAP *) appContext->workConn->conn;
    PSTR *children = NULL;
    INT i;

    if(!dn) {
        goto cleanup;
    }

    if(isRecursive) {
        dwError = GetChildren(appContext, ObjectClassAny, dn, &children);
        ADT_BAIL_ON_ERROR_NP(dwError);
        for(i = 0; children && children[i]; ++i) {
            dwError = DeleteADObject(appContext, children[i], isRecursive);
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Deleting object %s in naming context %s ...\n",
                appContext->actionName, dn, appContext->workConn->defaultNC);

    /* Perform the delete operation. */
    if(!appContext->gopts.isReadOnly) {
        dwError = ldap_delete_ext_s( ld, dn, NULL, NULL );
        if(dwError == 66) {
            dwError = ADT_ERR_OBJECT_NOT_EMPTY;
        }
        else {
            if(dwError) {
                dwError += ADT_LDAP_ERR_BASE;
            }
        }
    }

    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelTrace, "%s: Done deleting object %s\n",
                appContext->actionName, dn);

    cleanup:
        if(children) {
            for(i = 0; children[i]; ++i) {
                LW_SAFE_FREE_MEMORY(children[i]);
            }

            LW_SAFE_FREE_MEMORY(children);
        }

        return dwError;

    error:
        goto cleanup;
}

/**
 * Get object's GUID.
 *
 * @param appContext Application context reference.
 * @param dn Object class
 * @param guid Object's guid. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
DWORD GetObjectGUID(IN AppContextTP appContext, IN PSTR dn, OUT PSTR *guid)
{
    DWORD dwError = 0;
    LDAP *ld = (LDAP *) appContext->workConn->conn;
    LDAPMessage *res = NULL, *e = NULL;
    PSTR *vals = NULL;
    PSTR attrs[] = {
        "objectGUID",
        NULL };

    PrintStderr(appContext, LogLevelTrace, "%s: Getting GUID of object %s in naming context %s ...\n",
                appContext->actionName, dn, appContext->workConn->defaultNC);

    dwError = ldap_search_ext_s(ld, dn, LDAP_SCOPE_BASE,
                                "(objectClass=*)", attrs, 0, NULL, NULL, LDAP_NO_LIMIT,
                                LDAP_NO_LIMIT, &res);

    if ((dwError != LDAP_SUCCESS) || (res == NULL)) {
        dwError += ADT_LDAP_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    e = ldap_first_entry(ld, res);

    if (!e) {
        dwError = ADT_ERR_FAILED_AD_OBJ_NOT_FOUND;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    if ((vals = ldap_get_values( ld, e, "objectGUID")) == NULL ) {
        dwError = ADT_ERR_FAILED_AD_GET_GUID;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = Guid2Str((PVOID) vals[0], guid);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelTrace, "%s: GUID is %s\n",
                appContext->actionName, *guid);

    cleanup:
        if (res) {
            ldap_msgfree(res);
        }

        if(vals) {
            ldap_value_free(vals);
        }

        return dwError;

    error:
        goto cleanup;
}

/**
 * Get object's SID.
 *
 * @param appContext Application context reference.
 * @param dn Object class
 * @param sid Object's SID. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
DWORD GetObjectSID(IN AppContextTP appContext, IN PSTR dn, OUT PSTR *sid) {
    DWORD dwError = 0;
    PVOID sidBytes = NULL;

    dwError = GetObjectSIDBytes(appContext, dn, &sidBytes);
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = Sid2Str(sidBytes, sid);
    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelTrace, "%s: SID is %s \n",
                appContext->actionName, *sid);

    cleanup:
        LW_SAFE_FREE_MEMORY(sidBytes);
        return dwError;

    error:
        goto cleanup;
}

/**
 * Get object's Likewise ID.
 *
 * @param appContext Application context reference.
 * @param dn Object class
 * @param id Object's hashed ID. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
DWORD GetObjectDerivedID(IN AppContextTP appContext, IN PSTR dn, OUT PSTR *sid) {
    DWORD dwError = 0;
    PVOID sidBytes = NULL;
    DWORD id;

    dwError = GetObjectSIDBytes(appContext, dn, &sidBytes);
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = Sid2Id(sidBytes, &id);
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = LwAllocateStringPrintf(sid, "%d", id);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    PrintStderr(appContext, LogLevelTrace, "%s: Hashed ID is %s \n",
                appContext->actionName, *sid);

    cleanup:
        LW_SAFE_FREE_MEMORY(sidBytes);
        return dwError;

    error:
        goto cleanup;
}

/**
 * Get object's SID in raw form.
 *
 * @param appContext Application context reference.
 * @param dn Object class
 * @param sid Object's SID. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
DWORD GetObjectSIDBytes(IN AppContextTP appContext, IN PSTR dn, OUT PVOID *sid)
{
    DWORD dwError = 0;
    LDAP *ld = (LDAP *) appContext->workConn->conn;
    LDAPMessage *res = NULL, *e = NULL;
    PSTR *vals = NULL;
    PSTR attrs[] = {
        "objectSid",
        NULL };
    size_t size = 0;

    PrintStderr(appContext, LogLevelTrace, "%s: Getting SID of object %s in naming context %s ...\n",
                appContext->actionName, dn, appContext->workConn->defaultNC);

    dwError = ldap_search_ext_s(ld, dn, LDAP_SCOPE_BASE,
                                "(objectClass=*)", attrs, 0, NULL, NULL, LDAP_NO_LIMIT,
                                LDAP_NO_LIMIT, &res);

    if ((dwError != LDAP_SUCCESS) || (res == NULL)) {
        dwError += ADT_LDAP_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    e = ldap_first_entry(ld, res);

    if (!e) {
        dwError = ADT_ERR_FAILED_AD_OBJ_NOT_FOUND;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    if ((vals = ldap_get_values( ld, e, "objectSid")) == NULL ) {
        dwError = ADT_ERR_FAILED_AD_GET_GUID;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    size = vals[0][1] * sizeof(DWORD) + 8;

    dwError = LwAllocateMemory(size, sid);
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    memcpy((PVOID) *sid, (PVOID)vals[0], size);

    PrintStderr(appContext, LogLevelTrace, "%s: Getting SID of object - done\n",
                appContext->actionName);

    cleanup:
        if (res) {
            ldap_msgfree(res);
        }

        if(vals) {
            ldap_value_free(vals);
        }

        return dwError;

    error:
        goto cleanup;
}

/**
 * Get all object's attributes.
 *
 * @param appContext Application context reference.
 * @param dn Object class
 * @param avp Attribute-values pairs. (Dynamically allocated. Caller must free avp).
 * @return 0 on success; error code on failure.
 */
DWORD GetAllObjectAttrs(IN AppContextTP appContext, IN PSTR dn, OUT AttrValsT **avpp)
{
    DWORD dwError = 0;
    LDAP *ld = (LDAP *) appContext->workConn->conn;
    LDAPMessage *res = NULL, *e = NULL;
    PSTR *vals = NULL;
    PSTR aname = NULL;
    BerElement *ber = NULL;
    INT i;
    AttrValsT *avp = NULL;

    PrintStderr(appContext, LogLevelTrace, "%s: Reading attributes of object %s in naming context %s ...\n",
                appContext->actionName, dn, appContext->workConn->defaultNC);

    dwError = ldap_search_ext_s(ld, dn, LDAP_SCOPE_BASE,
                                "(objectClass=*)", NULL, 0, NULL, NULL, LDAP_NO_LIMIT,
                                LDAP_NO_LIMIT, &res);

    if ((dwError != LDAP_SUCCESS) || (res == NULL)) {
        dwError += ADT_LDAP_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    e = ldap_first_entry(ld, res);

    if (!e) {
        dwError = ADT_ERR_FAILED_AD_OBJ_NOT_FOUND;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = LwAllocateMemory(LDAP_ATTRS_MAX * sizeof(AttrValsT) + 1, (PVOID) &avp);
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    for (i = 0, aname = ldap_first_attribute(ld, e, &ber);
            (aname != NULL) && (i < LDAP_ATTRS_MAX);
            aname = ldap_next_attribute(ld, e, ber), ++i)
    {
        dwError = LwStrDupOrNull((PCSTR) aname, &(avp[i].attr));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

        if ((vals = ldap_get_values(ld, e, aname)) != NULL) {
            dwError = LwAllocateMemory((ldap_count_values(vals) + 1)
                    * sizeof(PSTR), (PVOID) &(avp[i].vals));
            ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

            FormatKnownAttributes(appContext, aname, vals, avp[i].vals);
            ADT_BAIL_ON_ERROR_NP(dwError);

            ldap_value_free(vals);
            vals = NULL;
        }

        ldap_memfree(aname);
        aname = NULL;
    }

    *avpp = avp;

    PrintStderr(appContext, LogLevelTrace, "%s: Done reading attributes of object %s\n",
                appContext->actionName, dn);

    cleanup:
        if (res) {
            ldap_msgfree(res);
        }

        if(vals) {
            ldap_value_free(vals);
        }

        if(aname) {
            ldap_memfree(aname);
        }

        if(ber) {
            ber_free(ber, 0);
        }

        return dwError;

    error:
        goto cleanup;
}

/**
 * Get object's attributes.
 *
 * @param appContext Application context reference.
 * @param dn Object DN
 * @param avp Attribute-values pairs. (Dynamically allocated. Caller must free avp[i].vals).
 * @return 0 on success; error code on failure.
 */
DWORD GetObjectAttrs(IN AppContextTP appContext, IN PSTR dn, OUT AttrValsT *avp) {
    DWORD dwError = 0;
    LDAP *ld = (LDAP *) appContext->workConn->conn;
    LDAPMessage *res = NULL, *e = NULL;
    PSTR *vals = NULL;
    INT i, j;
    PSTR * attrs = NULL;
    INT len = 0, valLen = 0;

    for(i = 0; avp[i].attr; ++i);
    len = i + 1;

    dwError = LwAllocateMemory(len * sizeof(PSTR), (PVOID) &attrs);
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    for(i = 0; i < len; ++i) {
        attrs[i] = avp[i].attr;
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Reading attributes of object %s in naming context %s ...\n",
                appContext->actionName, dn, appContext->workConn->defaultNC);

    dwError = ldap_search_ext_s(ld, dn, LDAP_SCOPE_BASE,
                                "(objectClass=*)", attrs, 0, NULL, NULL, LDAP_NO_LIMIT,
                                LDAP_NO_LIMIT, &res);

    if ((dwError != LDAP_SUCCESS) || (res == NULL)) {
        dwError += ADT_LDAP_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    e = ldap_first_entry(ld, res);

    if (!e) {
        dwError = ADT_ERR_FAILED_AD_OBJ_NOT_FOUND;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    i = 0;

    do {
        if ((vals = ldap_get_values( ld, e, (PCSTR) attrs[i])) == NULL ) {
            LW_SAFE_FREE_MEMORY(avp[i].vals);
        }
        else {
            for(j = 0; vals[j]; ++j);
            valLen = j + 1;

            dwError = LwAllocateMemory(valLen * sizeof(PSTR), (PVOID) &(avp[i].vals));
            ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

            FormatKnownAttributes(appContext, attrs[i], vals, avp[i].vals);
            ADT_BAIL_ON_ERROR_NP(dwError);

            /*
            for(j = 0; j < valLen; ++j) {
                dwError = LwStrDupOrNull((PCSTR) vals[j], &(avp[i].vals[j]));
                ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
            }
            */
        }
    } while (attrs[++i]);

    PrintStderr(appContext, LogLevelTrace, "%s: Done reading attributes of object %s\n",
                appContext->actionName, dn);

    cleanup:
        LW_SAFE_FREE_MEMORY(attrs);

        if (res) {
            ldap_msgfree(res);
        }

        if(vals) {
            ldap_value_free(vals);
        }

        return dwError;

    error:
        goto cleanup;
}

/**
 * Locate AD user.
 *
 * @param appContext Application context reference.
 * @param name Name of the user. Can be in any format. Will be set to DN on success.
 * @return 0 on success; error code on failure.
 */
DWORD LocateADUser(IN AppContextTP appContext, IN OUT PSTR *name)
{
    DWORD dwError = 0;
    PSTR *outDNs = NULL;
    INT count = 0;
    PSTR filter = NULL;
    PSTR dn = NULL;
    INT i = 0;

    PrintStderr(appContext, LogLevelTrace, "%s: Searching for AD user %s in naming context %s...\n",
                appContext->actionName, *name, appContext->workConn->defaultNC);

    if(IsDNComp(*name)) {
        dwError = ResolveDN(appContext, ObjectClassUser, *name, &dn);
        ADT_BAIL_ON_ERROR_NP(dwError);

        LW_SAFE_FREE_MEMORY(*name);
        dwError = LwStrDupOrNull((PCSTR) dn, name);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }
    else {
        if(IsNVP(*name)) {
            dwError = LwAllocateStringPrintf(&filter, "(&(objectClass=user)(%s))", *name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }
        else {
            dwError = LwAllocateStringPrintf(&filter, "(&(objectClass=user)(%s=%s))", IsUPN(*name) ? "userPrincipalName" : "sAMAccountName", *name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        dwError = SearchForObject(appContext, ObjectClassUser,
                                  LDAP_SCOPE_SUBTREE, filter,
                                  appContext->workConn->defaultNC, &outDNs);

        ADT_BAIL_ON_ERROR_NP(dwError);

        for (i = 0; outDNs && outDNs[i]; ++i)
            ;
        count = i;

        if (count > 1) {
            dwError = ADT_ERR_MULTIPLE_USERS;
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
        else {
            if (!count) {
                dwError = ADT_ERR_FAILED_AD_USER_NOT_FOUND;
                ADT_BAIL_ON_ERROR_NP(dwError);
            }
        }

        LW_SAFE_FREE_MEMORY(*name);
        dwError = LwStrDupOrNull((PCSTR) outDNs[0], name);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done searching for AD user. Found DN: %s\n",
                appContext->actionName, *name);

    cleanup:
        if(outDNs) {
            for(i = 0; outDNs[i]; ++i) {
                LW_SAFE_FREE_MEMORY(outDNs[i]);
            }

            LW_SAFE_FREE_MEMORY(outDNs);
        }

        LW_SAFE_FREE_MEMORY(filter);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Locate AD computer.
 *
 * @param appContext Application context reference.
 * @param name Name of the computer. Can be in any format. Will be set to DN on success.
 * @return 0 on success; error code on failure.
 */
DWORD LocateADComputer(IN AppContextTP appContext, IN OUT PSTR *name)
{
    DWORD dwError = 0;
    PSTR *outDNs = NULL;
    INT count = 0;
    PSTR filter = NULL;
    PSTR dn = NULL;
    INT i = 0;

    PrintStderr(appContext, LogLevelTrace, "%s: Searching for AD computer %s in naming context %s...\n",
                appContext->actionName, *name, appContext->workConn->defaultNC);

    if(IsDNComp(*name)) {
        dwError = ResolveDN(appContext, ObjectClassUser, *name, &dn);
        ADT_BAIL_ON_ERROR_NP(dwError);

        LW_SAFE_FREE_MEMORY(*name);
        dwError = LwStrDupOrNull((PCSTR) dn, name);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }
    else {
        if(IsNVP(*name)) {
            dwError = LwAllocateStringPrintf(&filter, "(&(objectClass=computer)(%s))", *name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }
        else {
            if(IsSPN(*name)) {
                dwError = LwAllocateStringPrintf(&filter, "(&(objectClass=computer)(servicePrincipalName=%s))", *name);
            }
            else {
                if((*name)[strlen(*name) - 1] != '$') {
                    dwError = LwAllocateStringPrintf(&filter, "(&(objectClass=computer)(sAMAccountName=%s%s))", *name, "$");
                }
                else {
                    dwError = LwAllocateStringPrintf(&filter, "(&(objectClass=computer)(sAMAccountName=%s))", *name);
                }
            }
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        dwError = SearchForObject(appContext, ObjectClassComputer,
                                  LDAP_SCOPE_SUBTREE, filter,
                                  appContext->workConn->defaultNC, &outDNs);

        ADT_BAIL_ON_ERROR_NP(dwError);

        for (i = 0; outDNs && outDNs[i]; ++i)
            ;
        count = i;

        if (count > 1) {
            dwError = ADT_ERR_MULTIPLE_COMPUTERS;
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
        else {
            if (!count) {
                dwError = ADT_ERR_FAILED_AD_OBJ_NOT_FOUND;
                ADT_BAIL_ON_ERROR_NP(dwError);
            }
        }

        LW_SAFE_FREE_MEMORY(*name);
        dwError = LwStrDupOrNull((PCSTR) outDNs[0], name);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done searching for AD computer. Found DN: %s\n",
                appContext->actionName, *name);

    cleanup:
        if(outDNs) {
            for(i = 0; outDNs[i]; ++i) {
                LW_SAFE_FREE_MEMORY(outDNs[i]);
            }

            LW_SAFE_FREE_MEMORY(outDNs);
        }

        LW_SAFE_FREE_MEMORY(filter);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Locate AD group.
 *
 * @param appContext Application context reference.
 * @param name Name of the user. Can be in any format. Will be set to DN on success.
 * @return 0 on success; error code on failure.
 */
DWORD LocateADGroup(IN AppContextTP appContext, IN OUT PSTR *name)
{
    DWORD dwError = 0;
    PSTR *outDNs = NULL;
    PSTR dn = NULL;
    INT count = 0;
    PSTR filter = NULL;
    INT i = 0;

    PrintStderr(appContext, LogLevelTrace, "%s: Searching for AD group %s in naming context %s...\n",
                appContext->actionName, *name, appContext->workConn->defaultNC);

    if(IsDNComp(*name)) {
        dwError = ResolveDN(appContext, ObjectClassGroup, *name, &dn);
        ADT_BAIL_ON_ERROR_NP(dwError);

        LW_SAFE_FREE_MEMORY(*name);
        dwError = LwStrDupOrNull((PCSTR) dn, name);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }
    else {
        if(IsNVP(*name)) {
            dwError = LwAllocateStringPrintf(&filter, "(&(objectClass=group)(%s))", *name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }
        else {
            dwError = LwAllocateStringPrintf(&filter, "(&(objectClass=group)(%s=%s))", "sAMAccountName", *name);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        dwError = SearchForObject(appContext, ObjectClassGroup,
                                  LDAP_SCOPE_SUBTREE, filter,
                                  appContext->workConn->defaultNC, &outDNs);

        ADT_BAIL_ON_ERROR_NP(dwError);

        for (i = 0; outDNs && outDNs[i]; ++i);
        count = i;

        if (count > 1) {
            dwError = ADT_ERR_MULTIPLE_GROUPS;
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
        else {
            if (!count) {
                dwError = ADT_ERR_FAILED_AD_GROUP_NOT_FOUND;
                ADT_BAIL_ON_ERROR_NP(dwError);
            }
        }

        LW_SAFE_FREE_MEMORY(*name);
        dwError = LwStrDupOrNull((PCSTR) outDNs[0], name);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done searching for AD group. Found DN: %s\n",
                appContext->actionName, *name);

    cleanup:
        if(outDNs) {
            for(i = 0; outDNs[i]; ++i) {
                LW_SAFE_FREE_MEMORY(outDNs[i]);
            }

            LW_SAFE_FREE_MEMORY(outDNs);
        }

        LW_SAFE_FREE_MEMORY(filter);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Check whether the cell specified by dn parameter is in non-schema mode.
 *
 * @param appContext Application context reference.
 * @param name Name of the user. Can be in any format. Will be set to DN on success.
 * @param res Will be set to TRUE is the cell is in non-schema mode.
 * @return 0 on success; error code on failure.
 */
DWORD IsCellInNonSchemaMode(IN AppContextTP appContext, IN PSTR dn, OUT BOOL *res)
{
    DWORD dwError = 0;
    AttrValsT *avp = NULL;
    INT i, j;

    PrintStderr(appContext, LogLevelTrace, "%s: Checking the cell\'s mode ...\n",
                appContext->actionName);

    *res = FALSE;

    dwError = LwAllocateMemory(2 * sizeof(AttrValsT), (PVOID) &avp);
    ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

    avp[0].attr = "description";

    dwError = GetObjectAttrs(appContext, dn, avp);
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(!avp[0].vals || !avp[0].vals[0]) {
        dwError = ADT_ERR_ARG_MISSING_CELL_PROP;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    for (j = 0; avp[0].vals[j]; ++j) {
        if(DoesStrStartWith(avp[0].vals[j], "use2307Attrs=False", 1)) {
            *res = TRUE;
        }
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done checking the cell\'s mode. The cell is in %s mode\n",
                appContext->actionName, *res ? "non-schema" : "schema");

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
 * Get all cells the group is a member of.
 *
 * @param appContext Application context reference.
 * @param sb Search base. If NULL - the default naming context will be used.
 * @param name Name of the group.
 * @param type Object class.
 * @param cells DNs of the cells the user is a member of.
 * @return 0 on success; error code on failure.
 */
static DWORD
GetCellsForObject(IN AppContextTP appContext, IN PSTR sb, IN PSTR name, IN ObjectClassT type, OUT PSTR **cells)
{
    DWORD dwError = 0;
    PSTR *outDNs = NULL;
    PSTR *retCells = NULL;
    PSTR cellObj = NULL;
    PSTR cellDN = NULL;
    INT i, j;
    INT cellNum = 0;
    PSTR paramCN;
    PSTR paramType;

    *cells = NULL;

    switch(type) {
        case ObjectClassCellUser:
            paramCN = "Users";
            paramType = "user";
            break;

        case ObjectClassCellGroup:
            paramCN = "Groups";
            paramType = "group";
            break;

        default:
            dwError = ADT_ERR_INVALID_ARG;
            ADT_BAIL_ON_ERROR(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Searching for cells in naming context %s ...\n",
                appContext->actionName, appContext->workConn->defaultNC);

    dwError = SearchForObject(appContext,
                              ObjectClassCell,
                              LDAP_SCOPE_SUBTREE,
                              "(&(objectclass=container)(CN=$LikewiseIdentityCell))",
                              sb ? sb : appContext->workConn->defaultNC, &outDNs);

    ADT_BAIL_ON_ERROR_NP(dwError);

    PrintStderr(appContext, LogLevelTrace, "%s: Done searching for cells \n",
                appContext->actionName);

    for(i = 0; outDNs && outDNs[i]; ++i) {
        PrintStderr(appContext, LogLevelTrace,
                    "%s: Checking if the %s is a member of cell %s ...\n",
                    appContext->actionName, paramType, outDNs[i]);

        dwError = LwAllocateStringPrintf(&cellObj,
                                         "CN=%s,CN=%s,%s",
                                         name ? name : appContext->oName,
                                         paramCN,
                                         outDNs[i]);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

        if(!ResolveDN(appContext, type, cellObj, &cellDN)) {
            ++cellNum;
            PrintStderr(appContext, LogLevelTrace, "%s: YES\n", appContext->actionName);
        }
        else {
            PrintStderr(appContext, LogLevelTrace, "%s: NO\n", appContext->actionName);
        }

        LW_SAFE_FREE_MEMORY(cellObj);
        LW_SAFE_FREE_MEMORY(cellDN);
    }

    if (outDNs && cellNum) {
        dwError = LwAllocateMemory((cellNum + 1) * sizeof(PSTR), (PVOID) &retCells);
        ADT_BAIL_ON_ALLOC_FAILURE(!dwError);

        for (i = 0, j = 0; outDNs[i] && (j < cellNum); ++i) {
            dwError = LwAllocateStringPrintf(&cellObj,
                                             "CN=%s,CN=%s,%s",
                                             name ? name : appContext->oName,
                                             paramCN,
                                             outDNs[i]);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            if (!ResolveDN(appContext, type, cellObj, &cellDN)) {
                dwError = LwStrDupOrNull((PCSTR) cellDN, &(retCells[j]));
                ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
                ++j;
            }

            LW_SAFE_FREE_MEMORY(cellObj);
            LW_SAFE_FREE_MEMORY(cellDN);
        }

        *cells = retCells;
    }

    cleanup:
        if(outDNs) {
            for(i = 0; outDNs[i]; ++i) {
                LW_SAFE_FREE_MEMORY(outDNs[i]);
            }

            LW_SAFE_FREE_MEMORY(outDNs);
        }

        LW_SAFE_FREE_MEMORY(cellObj);
        LW_SAFE_FREE_MEMORY(cellDN);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Get all cells the user is a member of.
 *
 * @param appContext Application context reference.
 * @param sb Search base. If NULL - the default naming context will be used.
 * @param name Name of the user.
 * @param cells DNs of the cells the user is a member of.
 * @return 0 on success; error code on failure.
 */
DWORD GetCellsForUser(IN AppContextTP appContext, IN PSTR sb, IN PSTR name, OUT PSTR **cells)
{
    return GetCellsForObject(appContext, sb, name, ObjectClassCellUser, cells);
}

/**
 * Get all cells the group is a member of.
 *
 * @param appContext Application context reference.
 * @param sb Search base. If NULL - the default naming context will be used.
 * @param name Name of the group.
 * @param cells DNs of the cells the user is a member of.
 * @return 0 on success; error code on failure.
 */
DWORD GetCellsForGroup(IN AppContextTP appContext, IN PSTR sb, IN PSTR name, OUT PSTR **cells)
{
    return GetCellsForObject(appContext, sb, name, ObjectClassCellGroup, cells);
}

/**
 * Rename/move object.
 *
 * @param appContext Application context reference.
 * @param dn DN of the object to move.
 * @param newRDN New RDN of the object.
 * @param newParent New parent DN of the object.
 * @return 0 on success; error code on failure.
 */
DWORD MoveADObject(IN AppContextTP appContext, IN PSTR dn, IN PSTR newRDN, IN PSTR newParent)
{
    DWORD dwError = 0;
    LDAP *ld = (LDAP *) appContext->workConn->conn;

    PrintStderr(appContext, LogLevelTrace, "%s: Moving/renaming AD object %s to parent %s under name %s. Naming context is %s\n",
                appContext->actionName, dn, newParent, newRDN, appContext->workConn->defaultNC);

    if(!appContext->gopts.isReadOnly) {
        dwError = ldap_rename_s( ld, dn, newRDN, newParent, 1, NULL, NULL );

        if (dwError != LDAP_SUCCESS) {
            dwError += ADT_LDAP_ERR_BASE;
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done moving/renaming AD object\n",
                appContext->actionName);

    cleanup:
        return dwError;

    error:
        goto cleanup;
}
