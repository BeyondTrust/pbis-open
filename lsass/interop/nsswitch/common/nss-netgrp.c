/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        nss-group.c
 *
 * Abstract:
 *
 *        Name Server Switch (Likewise LSASS)
 *
 *        Handle NSS NetGroup Information (Common)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "lsanss.h"

static const DWORD MAX_NUM_ARTEFACTS = 500;

VOID
LsaNssClearEnumArtefactsState(
    HANDLE hLsaConnection,
    PLSA_ENUMARTEFACTS_STATE pState
    )
{
    if (pState->ppArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(
            pState->dwArtefactInfoLevel,
            pState->ppArtefactInfoList,
            pState->dwNumArtefacts
            );
        pState->ppArtefactInfoList = (HANDLE)NULL;
    }
    
    if (hLsaConnection && pState->hResume != (HANDLE)NULL)
    {
        LsaEndEnumNSSArtefacts(hLsaConnection, pState->hResume);
        pState->hResume = (HANDLE)NULL;
    }

    memset(pState, 0, sizeof(*pState));

    pState->dwArtefactInfoLevel = 0;
}

NSS_STATUS
LsaNssCommonNetgroupFindByName(
    PLSA_NSS_CACHED_HANDLE pConnection,
    PCSTR pszName,
    PSTR* ppszValue
    )
{
    NSS_STATUS status = NSS_STATUS_SUCCESS;
    HANDLE hLsaConnection = NULL;
    LSA_ENUMARTEFACTS_STATE state = {0};
    DWORD dwNumFound = 0;
    PVOID* ppInfoList = NULL;
    int iGroup;
    BOOLEAN bFound = FALSE;

    status = MAP_LSA_ERROR(NULL,
            LsaNssCommonEnsureConnected(pConnection));
    BAIL_ON_NSS_ERROR(status);
    hLsaConnection = pConnection->hLsaConnection;

    LsaNssClearEnumArtefactsState(hLsaConnection, &state);

    status = MAP_LSA_ERROR(NULL,
                           LsaBeginEnumNSSArtefacts(
                               hLsaConnection,
                               state.dwArtefactInfoLevel,
                               LSA_NIS_MAP_NAME_NETGROUPS,
                               LSA_NIS_MAP_QUERY_ALL,
                               MAX_NUM_ARTEFACTS,
                               &state.hResume));
    BAIL_ON_NSS_ERROR(status);

    status = MAP_LSA_ERROR(NULL,
                           LsaEnumNSSArtefacts(
                               hLsaConnection,
                               state.hResume,
                               &dwNumFound,
                               &ppInfoList));
    BAIL_ON_NSS_ERROR(status);

    for (iGroup = 0; iGroup < dwNumFound; iGroup++)
    {
        PLSA_NSS_ARTEFACT_INFO_0 pInfo = (PLSA_NSS_ARTEFACT_INFO_0) ppInfoList[iGroup];

        if (!strcmp(pInfo->pszName, pszName))
        {
            *ppszValue = pInfo->pszValue;
            pInfo->pszValue = NULL;
            bFound = TRUE;
        }
        LW_SAFE_FREE_MEMORY(pInfo->pszValue);
        LW_SAFE_FREE_MEMORY(ppInfoList[iGroup]);
    }

    LW_SAFE_FREE_MEMORY(ppInfoList);

    /* Nothing found, so raise an error */
    if (!bFound)
    {
        status = NSS_STATUS_NOTFOUND;
        BAIL_ON_NSS_ERROR(status);
    }

done:

    LsaNssClearEnumArtefactsState(hLsaConnection, &state);

    return status;

error:

    if (status != NSS_STATUS_TRYAGAIN && status != NSS_STATUS_NOTFOUND)
    {
        LsaNssCommonCloseConnection(pConnection);
    }

    goto done;
}

static
void
LsaNssSkipSpace(
    PSTR* ppszCursor
    )
{
    while (**ppszCursor && isspace((int) **ppszCursor))
    {
        (*ppszCursor)++;
    }
}

static
void
LsaNssNextDelim(
    PSTR* ppszCursor
    )
{
    while (**ppszCursor && **ppszCursor != ',' && **ppszCursor != ')')
    {
        (*ppszCursor)++;
    }
}

static void
LsaNssChomp(
    PSTR pszToken
    )
{
    PSTR pszEnd = pszToken + strlen(pszToken) - 1;

    while (pszEnd >= pszToken && isspace((int) *pszEnd))
    {
        *pszEnd = '\0';
        pszEnd--;
    }
}

static
PSTR
LsaNssGetToken(
    PSTR* ppszCursor
    )
{
    PSTR pszToken = NULL;

    LsaNssSkipSpace(ppszCursor);

    if (!**ppszCursor)
    {
        return NULL;
    }
    else
    {
        pszToken = *ppszCursor;
        LsaNssNextDelim(ppszCursor);
        if (**ppszCursor)
        {
            **ppszCursor = '\0';
            (*ppszCursor)++;
        }
        LsaNssChomp(pszToken);
        return pszToken;
    }
}

NSS_STATUS
LsaNssCommonNetgroupParse(
    PSTR* ppszCursor,
    LSA_NSS_NETGROUP_ENTRY_TYPE* pType,
    PSTR* ppszHost,
    PSTR* ppszUser,
    PSTR* ppszDomain,
    PSTR* ppszGroup
    )
{
    NSS_STATUS ret = NSS_STATUS_SUCCESS;

    LsaNssSkipSpace(ppszCursor);

    if (**ppszCursor == '(')
    {
        /* Move past the open parenthesis */
        (*ppszCursor)++;
        /* First token, the hostname */
        *ppszHost = LsaNssGetToken(ppszCursor);
        /* Second token, the username */
        *ppszUser = LsaNssGetToken(ppszCursor);
        /* Third token, the domain */
        *ppszDomain = LsaNssGetToken(ppszCursor);

        LsaNssSkipSpace(ppszCursor);

        /* Make sure we didn't run out of tokens */
        if (!*ppszHost || !*ppszUser || !*ppszDomain)
        {
            ret = NSS_STATUS_UNAVAIL;
            BAIL_ON_NSS_ERROR(ret);
        }

        /* If we are not at the end of the list */
        if (**ppszCursor)
        {
            /* There should have been a comma after the close parenthesis */
            if (**ppszCursor == ',')
            {
                (*ppszCursor)++;
            }
            else if (**ppszCursor != '(' && !isalpha((int)**ppszCursor))
            {
                ret = NSS_STATUS_UNAVAIL;
                BAIL_ON_NSS_ERROR(ret);
            }
        }

        /* Convert empty strings to NULL strings */
        if (!**ppszHost)
        {
            *ppszHost = NULL;
        }

        if (!**ppszUser)
        {
            *ppszUser = NULL;
        }

        if (!**ppszDomain)
        {
            *ppszDomain = NULL;
        }

        *pType = LSA_NSS_NETGROUP_ENTRY_TRIPLE;
    }
    else
    {
        /* Parse a group name */
        *ppszGroup = LsaNssGetToken(ppszCursor);
        if (!*ppszGroup || !**ppszGroup)
        {
            /* End of entries */
            *pType = LSA_NSS_NETGROUP_ENTRY_END;
        }
        else
        {
            /* Nested group */
            *pType = LSA_NSS_NETGROUP_ENTRY_GROUP;
        }
    }

error:

    return ret;
}
