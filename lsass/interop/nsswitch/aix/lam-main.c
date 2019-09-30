/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "includes.h"
#include "lam-group.h"
#include "lam-user.h"
#include "lam-auth.h"

void
LsaNssClearState(
    PVOID pState
    )
{
    PLSA_NSS_STATE pNssState = (PLSA_NSS_STATE)pState;

    LSA_LOG_PAM_DEBUG("Clearing LAM state");

    if (pNssState != NULL)
    {
        LW_SAFE_FREE_MEMORY(pNssState->pLastGroup);
        LW_SAFE_FREE_MEMORY(pNssState->pLastUser);
        LW_SAFE_FREE_STRING(pNssState->pszRegistryName);
    }
}

PVOID
LsaNssOpen(
    PCSTR pszName,
    PCSTR pszDomain,
    int mode,
    PSTR pszOptions
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_PAM_CONFIG pConfig = NULL;

    dwError = LsaPamGetConfig(&pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPamSetLogLevel(pConfig->dwLogLevel);

    LSA_LOG_PAM_DEBUG(
            "Open called for '%s' with options '%s'",
            pszName,
            pszOptions);

    LsaNssClearState(&gNssState);

    dwError = LwAllocateString(
        pszName,
        &gNssState.pszRegistryName);
    BAIL_ON_LSA_ERROR(dwError);

    if (!strcasecmp(pszOptions, "debug"))
    {
        LsaPamSetLogLevel(LSA_PAM_LOG_LEVEL_DEBUG);
    }

cleanup:

    LSA_LOG_PAM_DEBUG("Open finishing with code %u", dwError);
    if (pConfig)
    {
        LsaPamFreeConfig(pConfig);
    }

    if (dwError != LW_ERROR_SUCCESS)
    {
        LsaNssMapErrorCode(dwError, &errno);
        return NULL;
    }
    else
    {
        return &gNssState;
    }

error:

    goto cleanup;
}

static
int
_LsaNssGetEntry(
        PSTR pszKey,
        PSTR pszTable,
        PSTR* ppszAttributes,
        attrval_t* pResults,
        int iAttrCount
        )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    int iIndex = 0;

    LSA_LOG_PAM_DEBUG(
            "Getting %d entry/ies in table '%s' for user '%s",
            iAttrCount,
            pszTable,
            pszKey);

    for (iIndex = 0; iIndex < iAttrCount; iIndex++)
    {
        LSA_LOG_PAM_DEBUG("Attr %d - '%s'",
                iIndex,
                ppszAttributes[iIndex]);
    }

    dwError = LsaNssCommonEnsureConnected(&lsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    if (!strcmp(pszKey, "ALL"))
    {
        if (iAttrCount != 1)
        {
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (!strcmp(pszTable, "user"))
        {
            if (strcmp(ppszAttributes[0], S_USERS))
            {
                dwError = EINVAL;
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                dwError = LsaNssListUsers(
                        lsaConnection.hLsaConnection,
                        &pResults[0]);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!strcmp(pszTable, "group"))
        {
            if (strcmp(ppszAttributes[0], S_GROUPS))
            {
                dwError = EINVAL;
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                dwError = LsaNssListGroups(
                        lsaConnection.hLsaConnection,
                        &pResults[0]);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else
        {
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        if (!strcmp(pszTable, "user"))
        {
            dwError = LsaNssGetUserAttrs(
                    lsaConnection.hLsaConnection,
                    pszKey,
                    ppszAttributes,
                    pResults,
                    iAttrCount);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!strcmp(pszTable, "group"))
        {
            dwError = LsaNssGetGroupAttrs(
                    lsaConnection.hLsaConnection,
                    pszKey,
                    ppszAttributes,
                    pResults,
                    iAttrCount);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    LSA_LOG_PAM_DEBUG("Getentry finishing with code %u", dwError);
    if (dwError != LW_ERROR_SUCCESS)
    {
        LsaNssMapErrorCode(dwError, &errno);
        return -1;
    }
    else
        return 0;

error:
    LsaNssCommonCloseConnection(&lsaConnection);

    goto cleanup;
}

int
LsaNssGetEntry(
        PSTR pszKey,
        PSTR pszTable,
        PSTR* ppszAttributes,
        attrval_t* pResults,
        int iAttrCount
        )
{
    int rc = -1;
    
    NSS_LOCK();
    
    rc = _LsaNssGetEntry(pszKey, pszTable, ppszAttributes, pResults, iAttrCount);
    
    NSS_UNLOCK();
    
    return rc;
}

attrlist_t **
LsaNssGetSupportedAttrs(
        VOID
        )
{
    static attrlist_t pList[] = {
        {S_ID, AL_GROUPATTR, SEC_INT},
        {S_PWD, AL_GROUPATTR, SEC_CHAR},
        {S_ADMIN, AL_GROUPATTR, SEC_BOOL},
        {S_ADMS, AL_GROUPATTR, SEC_LIST},
        {S_USERS, AL_GROUPATTR, SEC_LIST},
        {"SID", AL_GROUPATTR, SEC_CHAR},

        {S_USERS, AL_USERATTR, SEC_LIST},
        {S_ID, AL_USERATTR, SEC_INT},
        {S_PGID, AL_USERATTR, SEC_INT},
        {S_PWD, AL_USERATTR, SEC_CHAR},
        {S_HOME, AL_USERATTR, SEC_CHAR},
        {S_SHELL, AL_USERATTR, SEC_CHAR},
        {S_REGISTRY, AL_USERATTR, SEC_CHAR},
        {S_GECOS, AL_USERATTR, SEC_CHAR},
        {S_PGRP, AL_USERATTR, SEC_CHAR},
        {S_GROUPS, AL_USERATTR, SEC_LIST},
        {S_GROUPSIDS, AL_USERATTR, SEC_LIST},
        {S_DAEMONCHK, AL_USERATTR, SEC_BOOL},
        {S_LOCKED, AL_USERATTR, SEC_BOOL},
        {"SID", AL_USERATTR, SEC_CHAR},
        {"UPN", AL_USERATTR, SEC_CHAR},
    };
    static attrlist_t *ppList[] = {
        &pList[0],
        &pList[1],
        &pList[2],
        &pList[3],
        &pList[4],
        &pList[5],
        &pList[6],
        &pList[7],
        &pList[8],
        &pList[9],
        &pList[10],
        &pList[11],
        &pList[12],
        &pList[13],
        &pList[14],
        &pList[15],
        &pList[16],
        &pList[17],
        &pList[18],
        &pList[19],
        &pList[20],
        NULL
    };

    LSA_LOG_PAM_DEBUG("Returning the list of supported LAM attributes");
    return ppList;
}

int
LsaNssInitialize(struct secmethod_table *methods)
{
    struct stat buf;
    
    memset(methods, 0, sizeof(*methods));
    methods->method_version = SECMETHOD_VERSION_520;
    methods->method_getgrgid = LsaNssGetGrGid;
    methods->method_getgrnam = LsaNssGetGrNam;
    methods->method_getgrset = LsaNssGetGrSet;
    methods->method_getgracct = LsaNssGetGrAcct;
    methods->method_getpwuid = LsaNssGetPwUid;
    methods->method_getpwnam = LsaNssGetPwNam;
    methods->method_getentry = LsaNssGetEntry;
    methods->method_attrlist = LsaNssGetSupportedAttrs;
    methods->method_authenticate = LsaNssAuthenticate;
    methods->method_open = LsaNssOpen;
    methods->method_close = LsaNssClearState;
    methods->method_passwdexpired = LsaNssIsPasswordExpired;
    methods->method_chpass = LsaNssChangePassword;

	/* Avoid normalizing account names as AIX now supports long account names */
	/* Allow old behaviour to be used when /etc/pbis/lam_normalize exists */
    if (stat("/etc/pbis/lam_normalize", &buf) == 0)
    {
        methods->method_normalize = LsaNssNormalizeUsername;
    }

    memset(&gNssState, 0, sizeof(gNssState));

    return AUTH_SUCCESS;
}
