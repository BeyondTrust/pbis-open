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
 *        Handle NSS Group Information
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "lsanss.h"
#include "externs.h"
#include <assert.h>

static const DWORD MAX_NUM_GROUPS = 500;
static LSA_ENUMGROUPS_STATE gEnumGroupsState = {0};

NSS_STATUS
_nss_lsass_setgrent(
    void
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonGroupSetgrent(&lsaConnection,
                                       &gEnumGroupsState);

    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_getgrent_r(
    struct group*  pResultGroup,
    char *         pszBuf,
    size_t         bufLen,
    int*           pErrorNumber
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonGroupGetgrent(
        &lsaConnection,
        &gEnumGroupsState,
        pResultGroup,
        pszBuf,
        bufLen,
        pErrorNumber);

    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_endgrent(
    void
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonGroupEndgrent(&lsaConnection, &gEnumGroupsState);

    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_getgrgid_r(
    gid_t          gid,
    struct group*  pResultGroup,
    char*          pszBuf,
    size_t         bufLen,
    int*           pErrorNumber
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonGroupGetgrgid(&lsaConnection,
                                       gid,
                                       pResultGroup,
                                       pszBuf,
                                       bufLen,
                                       pErrorNumber);

    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_getgrnam_r(
    const char *   pszGroupName,
    struct group * pResultGroup,
    char *         pszBuf,
    size_t         bufLen,
    int*           pErrorNumber
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonGroupGetgrnam(&lsaConnection,
                                       pszGroupName,
                                       pResultGroup,
                                       pszBuf,
                                       bufLen,
                                       pErrorNumber);

    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_initgroups_dyn(
    PCSTR     pszUserName,
    gid_t     groupGid,
    long int* pResultsSize,
    long int* pResultsCapacity,
    gid_t**   ppGidResults,
    long int  maxGroups,
    int*      pErrorNumber
    )
{
    int ret = NSS_STATUS_SUCCESS;
    size_t resultsCapacity = (size_t) *pResultsCapacity;
    size_t resultsExistingSize = (size_t) *pResultsSize;
    size_t resultsSize = 0;
    gid_t* pGidResults = *ppGidResults;
    gid_t* pGidResultsNew = NULL;

    NSS_LOCK();

    ret = LsaNssCommonGroupGetGroupsByUserName(
        &lsaConnection,
        pszUserName,
        resultsExistingSize,
        resultsCapacity,
        &resultsSize,
        pGidResults,
        pErrorNumber);

    if (ret != NSS_STATUS_SUCCESS)
        goto error;

    if (resultsSize > resultsCapacity)
    {
        /* More results were found than were stored, so
           reallocate array and try again. */
        pGidResultsNew = realloc(pGidResults, sizeof(*pGidResults) * resultsSize);
        if (!pGidResultsNew)
        {
            *pErrorNumber = ENOMEM;
            ret = NSS_STATUS_UNAVAIL;
            goto error;
        }
        else
        {
            pGidResults = pGidResultsNew;
            *ppGidResults = pGidResults;
        }

        /* The number of filled elements is whatever our old capacity was */
        resultsExistingSize = resultsCapacity;
        /* The new capacity is the number of available results */
        resultsCapacity = resultsSize;
        /* Try again */
        ret = LsaNssCommonGroupGetGroupsByUserName(
            &lsaConnection,
            pszUserName,
            resultsExistingSize,
            resultsCapacity,
            &resultsSize,
            pGidResults,
            pErrorNumber);
    }

    if (ret == NSS_STATUS_SUCCESS)
    {
        *pResultsSize = (long int) resultsSize;
        *pResultsCapacity = (long int) resultsCapacity;
    }

error:

    NSS_UNLOCK();

    return ret;
}

