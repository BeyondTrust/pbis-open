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
 *        netsvc-netgrp.c
 *
 * Abstract:
 *
 *        Name Server Switch (Likewise LSASS)
 *
 *        Handle NSS Net Group Information
 *
 * Authors:  Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "includes.h"

typedef struct _LSA_NSS_NETGROUP_LIST
{
    PSTR pszGroup;
    struct _LSA_NSS_NETGROUP_LIST* pNext;
} LSA_NSS_NETGROUP_LIST, *PLSA_NSS_NETGROUP_LIST;

typedef struct
{
    char* pBuffer;
    char* pCursor;
    PLSA_NSS_NETGROUP_LIST pExpand;
    PLSA_NSS_NETGROUP_LIST pSeen;
} LSA_NSS_NETGROUP_PRIVATE, *PLSA_NSS_NETGROUP_PRIVATE;

static
NSS_STATUS
LsaNssPushNetgroup(
    PLSA_NSS_NETGROUP_LIST* ppList,
    PCSTR pszGroup
    )
{
    NSS_STATUS status = NSS_STATUS_SUCCESS;
    PLSA_NSS_NETGROUP_LIST pLink = NULL;

    status = MAP_LSA_ERROR(NULL,
                        LwAllocateMemory(
                            sizeof(*pLink),
                            (void**) &pLink));
    BAIL_ON_NSS_ERROR(status);

    status = MAP_LSA_ERROR(NULL,
                        LwAllocateString(
                            pszGroup,
                            &pLink->pszGroup));
    BAIL_ON_NSS_ERROR(status);

    pLink->pNext = *ppList;
    *ppList = pLink;

error:

    return status;
}

static
NSS_STATUS
LsaNssPopNetgroup(
    PLSA_NSS_NETGROUP_LIST* ppList,
    PSTR* ppszGroup
    )
{
    NSS_STATUS status = NSS_STATUS_SUCCESS;
    PLSA_NSS_NETGROUP_LIST pLink = *ppList;

    if (!pLink)
    {
        status = NSS_STATUS_UNAVAIL;
        BAIL_ON_NSS_ERROR(status);
    }

    *ppList = pLink->pNext;
    *ppszGroup = pLink->pszGroup;
    LwFreeMemory(pLink);

error:

    return status;
}

static
BOOLEAN
LsaNssContainsNetgroup(
    PLSA_NSS_NETGROUP_LIST pList,
    PSTR pszGroup
    )
{
    PLSA_NSS_NETGROUP_LIST pLink = NULL;

    for (pLink = pList; pLink; pLink = pLink->pNext)
    {
        if (!strcmp(pLink->pszGroup, pszGroup))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static
NSS_STATUS
LsaNssFreeNetgroupList(
    PLSA_NSS_NETGROUP_LIST* ppList
    )
{
    NSS_STATUS status = NSS_STATUS_SUCCESS;
    PLSA_NSS_NETGROUP_LIST pLink, pNext;

    for (pLink = *ppList; pLink; pLink = pNext)
    {
        pNext = pLink->pNext;

        LwFreeMemory(pLink->pszGroup);
        LwFreeMemory(pLink);
    }

    *ppList = NULL;

    return status;
}

static
NSS_STATUS
LsaNssIrsNetgroupDestructor(
    PLSA_NSS_NETGROUP_PRIVATE pPrivate
    )
{
    NSS_STATUS status = NSS_STATUS_SUCCESS;

    LsaNssFreeNetgroupList(&pPrivate->pSeen);
    LsaNssFreeNetgroupList(&pPrivate->pExpand);

    LW_SAFE_FREE_MEMORY(pPrivate->pBuffer);

    LwFreeMemory(pPrivate);

    return status;
}

static
NSS_STATUS
LsaNssIrsNetgroupExpand(
    PLSA_NSS_NETGROUP_PRIVATE pPrivate
    )
{
    NSS_STATUS status = NSS_STATUS_SUCCESS;
    PSTR pszGroup = NULL;
    PSTR pszContents = NULL;

    while (pPrivate->pExpand)
    {
        LsaNssPopNetgroup(&pPrivate->pExpand, &pszGroup);

        status = LsaNssCommonNetgroupFindByName(
            &lsaConnection,
            pszGroup,
            &pszContents);
        if (status == NSS_STATUS_NOTFOUND)
        {
            /* Don't let bad entries stop expansion of valid ones */
            continue;
        }
        BAIL_ON_NSS_ERROR(status);

        if (pPrivate->pBuffer)
        {
            LwFreeMemory(pPrivate->pBuffer);
        }
        pPrivate->pBuffer = pPrivate->pCursor = pszContents;
        goto cleanup;
    }

    status = NSS_STATUS_NOTFOUND;
    BAIL_ON_NSS_ERROR(status);

cleanup:

    return status;

error:

    goto cleanup;
}

static
NSS_STATUS
LsaNssIrsNetgroupParse(
    PLSA_NSS_NETGROUP_PRIVATE pPrivate,
    PSTR* ppszOutHost,
    PSTR* ppszOutUser,
    PSTR* ppszOutDomain
    )
{
    NSS_STATUS status = NSS_STATUS_SUCCESS;
    PSTR pszHost = NULL;
    PSTR pszUser = NULL;
    PSTR pszDomain = NULL;
    PSTR pszGroup = NULL;
    LSA_NSS_NETGROUP_ENTRY_TYPE type;

    /* Keep parsing until we encounter an error
       or find a triple entry */

    while (1)
    {
        if (!pPrivate->pBuffer)
        {
            status = LsaNssIrsNetgroupExpand(pPrivate);
            BAIL_ON_NSS_ERROR(status);
        }

        status = LsaNssCommonNetgroupParse(
            &pPrivate->pCursor,
            &type,
            &pszHost,
            &pszUser,
            &pszDomain,
            &pszGroup);
        BAIL_ON_NSS_ERROR(status);

        switch (type)
        {
        case LSA_NSS_NETGROUP_ENTRY_GROUP:
            if (!LsaNssContainsNetgroup(pPrivate->pSeen, pszGroup))
            {
                status = LsaNssPushNetgroup(&pPrivate->pExpand, pszGroup);
                BAIL_ON_NSS_ERROR(status);
                status = LsaNssPushNetgroup(&pPrivate->pSeen, pszGroup);
                BAIL_ON_NSS_ERROR(status);
            }
            break;
        case LSA_NSS_NETGROUP_ENTRY_TRIPLE:
            *ppszOutUser = pszUser;
            *ppszOutHost = pszHost;
            *ppszOutDomain = pszDomain;
            goto cleanup;
        case LSA_NSS_NETGROUP_ENTRY_END:
            LwFreeMemory(pPrivate->pBuffer);
            pPrivate->pBuffer = NULL;
            break;
        }
    }

cleanup:

    return status;

error:

    if (status == NSS_STATUS_NOTFOUND)
    {
        LwFreeMemory(pPrivate->pBuffer);
        pPrivate->pBuffer = NULL;
    }

    goto cleanup;
}

static
NSS_STATUS
LsaNssIrsNetgroupSetnetgrent(
    PLSA_NSS_NETGROUP_PRIVATE pPrivate,
    PCSTR pszGroup
    )
{
    NSS_STATUS status = NSS_STATUS_SUCCESS;
    PSTR pGroupContents = NULL;

    status = LsaNssCommonNetgroupFindByName(
        &lsaConnection,
        pszGroup,
        &pGroupContents);
    BAIL_ON_NSS_ERROR(status);

    pPrivate->pBuffer = pGroupContents;
    pPrivate->pCursor = pPrivate->pBuffer;
    pGroupContents = NULL;

    status = LsaNssPushNetgroup(&pPrivate->pSeen, pszGroup);
    BAIL_ON_NSS_ERROR(status);

cleanup:

    LW_SAFE_FREE_STRING(pGroupContents);

    return status;

error:

    goto cleanup;
}

static
NSS_STATUS
LsaNssIrsNetgroupInnetgr(
    PLSA_NSS_NETGROUP_PRIVATE pPrivate,
    PCSTR pszGroup,
    PCSTR pszMatchHost,
    PCSTR pszMatchUser,
    PCSTR pszMatchDomain
    )
{
    NSS_STATUS status = NSS_STATUS_SUCCESS;
    PLSA_NSS_NETGROUP_PRIVATE pInner = NULL;
    PSTR pszHost = NULL;
    PSTR pszUser = NULL;
    PSTR pszDomain = NULL;

    status = MAP_LSA_ERROR(NULL,
                           LwAllocateMemory(
                               sizeof(*pInner),
                               (void**) &pInner));

    status = LsaNssPushNetgroup(&pInner->pExpand, pszGroup);
    BAIL_ON_NSS_ERROR(status);

    while (1)
    {
        status = LsaNssIrsNetgroupParse(
            pInner,
            &pszHost,
            &pszUser,
            &pszDomain);
        BAIL_ON_NSS_ERROR(status);

        if ((pszMatchHost == NULL || !strcmp(pszHost, pszMatchHost)) &&
            (pszMatchUser == NULL || !strcmp(pszUser, pszMatchUser)) &&
            (pszMatchDomain == NULL || !strcmp(pszDomain, pszMatchDomain)))
        {
            goto cleanup;
        }
    }

    status = NSS_STATUS_NOTFOUND;
    BAIL_ON_NSS_ERROR(status);

cleanup:

    LsaNssIrsNetgroupDestructor(pInner);

    return status;

error:

    goto cleanup;
}

void
ng_close(
    PLSA_NSS_NETGROUP_PRIVATE pPrivate
    )
{
    LsaNssIrsNetgroupDestructor(pPrivate);
}

int
ng_next(
    PLSA_NSS_NETGROUP_PRIVATE pPrivate,
    char **host,
    char **user,
    char **domain
    )
{
    return LsaNssIrsNetgroupParse(pPrivate, host, user, domain) == NSS_STATUS_SUCCESS;
}

int
ng_test(
    PLSA_NSS_NETGROUP_PRIVATE pPrivate,
    const char *group,
    const char *host,
    const char *user,
    const char *domain
    )
{
    return LsaNssIrsNetgroupInnetgr(pPrivate, group, host, user, domain) == NSS_STATUS_SUCCESS;
}

void
ng_rewind(
    PLSA_NSS_NETGROUP_PRIVATE pPrivate,
    const char *group
    )
{
    LsaNssIrsNetgroupSetnetgrent(pPrivate, group);
}

void
ng_minimize(
    PLSA_NSS_NETGROUP_PRIVATE pPrivate
    )
{
    return;
}

void *
ng_pvtinit(
    void
    )
{
    NSS_STATUS status = NSS_STATUS_SUCCESS;
    PLSA_NSS_NETGROUP_PRIVATE pPrivate = NULL;

    status = MAP_LSA_ERROR(NULL, LwAllocateMemory(sizeof(*pPrivate), (void**) &pPrivate));
    BAIL_ON_NSS_ERROR(status);

cleanup:

    return (void*) pPrivate;

error:

    LW_SAFE_FREE_MEMORY(pPrivate);

    goto cleanup;
}
